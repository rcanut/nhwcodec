/*
 * FPAQ memory-buffer implementation.
 *
 * Public API:
 *   int fpaq_compress(const uint8_t *input, size_t input_size,
 *                     uint8_t **output, size_t *output_size);
 *   int fpaq_decompress(const uint8_t *input, size_t input_size,
 *                       uint8_t **output, size_t *output_size);
 *
 * Both functions return 0 on success and -1 on error.
 * The returned buffer is allocated with malloc/realloc and must be freed
 * by the caller with free().
 */
 
 //derived from fpaq0 and Kanzi FPAQ

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define TOP     0xFFFFFFFFU
#define PSCALE  65536U
#define HEADER_SIZE 12U

typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
    uint8_t byte;
    int bits;
    int error;
} BitWriter;

typedef struct {
    const uint8_t *data;
    size_t size;
    size_t pos;
    uint8_t byte;
    int bits;
} BitReader;

typedef struct {
    uint32_t low, high;
    uint64_t pending;
    BitWriter bw;
    uint16_t probs[4][256];
} Encoder;

typedef struct {
    uint32_t low, high, code;
    BitReader br;
    uint16_t probs[4][256];
} Decoder;

/* Append one byte to a dynamically growing output buffer. */
static int buffer_append(BitWriter *w, uint8_t value)
{
    if (w->error)
        return -1;

    if (w->size == w->capacity) {
        size_t new_capacity = w->capacity ? w->capacity * 2 : 256;

        if (new_capacity < w->capacity) {
            w->error = 1;
            return -1;
        }

        uint8_t *p = (uint8_t *)realloc(w->data, new_capacity);
        if (!p) {
            w->error = 1;
            return -1;
        }

        w->data = p;
        w->capacity = new_capacity;
    }

    w->data[w->size++] = value;
    return 0;
}

/* Bit writer */

static void put_bit(BitWriter *w, int bit)
{
    if (w->error)
        return;

    w->byte = (uint8_t)((w->byte << 1) | (bit & 1));
    w->bits++;

    if (w->bits == 8) {
        (void)buffer_append(w, w->byte);
        w->byte = 0;
        w->bits = 0;
    }
}

static void flush_bits(BitWriter *w)
{
    if (w->error)
        return;

    if (w->bits) {
        w->byte = (uint8_t)(w->byte << (8 - w->bits));
        (void)buffer_append(w, w->byte);
        w->byte = 0;
        w->bits = 0;
    }
}

/* Bit reader. Like the original file version, reads past the end as zeroes. */

static int get_bit(BitReader *r)
{
    if (r->bits == 0) {
        int c = 0;
        if (r->pos < r->size)
            c = r->data[r->pos++];

        r->byte = (uint8_t)c;
        r->bits = 8;
    }

    int bit = (r->byte >> 7) & 1;
    r->byte <<= 1;
    r->bits--;

    return bit;
}

/* Probability initialization */

static void init_probs(uint16_t p[4][256])
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 256; j++)
            p[i][j] = 32768;
}

/* Probability update: same adaptation rule as the original */

static void update_prob(uint16_t *p, int bit)
{
    unsigned v = *p;

    if (bit == 0)
        v -= v >> 6;
    else if (v < 65472)
        v += (65472 - v + 63) >> 6;

    *p = (uint16_t)v;
}

/* Arithmetic encoder */

static void output_bit_plus_pending(Encoder *e, int bit)
{
    put_bit(&e->bw, bit);

    while (e->pending) {
        put_bit(&e->bw, !bit);
        e->pending--;
    }
}

/* Arithmetic decoder */

static void decode_bit(Decoder *d, int *bit, uint16_t *prob)
{
    uint64_t range = (uint64_t)d->high - d->low + 1;

    uint32_t split = d->low +
        (uint32_t)((range * (PSCALE - *prob)) >> 16);

    if (split <= d->low)
        split = d->low + 1;

    if (split > d->high)
        split = d->high;

    if (d->code < split) {
        *bit = 0;
        d->high = split - 1;
    } else {
        *bit = 1;
        d->low = split;
    }

    update_prob(prob, *bit);

    for (;;) {
        if (d->high < 0x80000000U) {
            /* E1 */
        } else if (d->low >= 0x80000000U) {
            /* E2 */
            d->code -= 0x80000000U;
            d->low -= 0x80000000U;
            d->high -= 0x80000000U;
        } else if (d->low >= 0x40000000U &&
                   d->high < 0xC0000000U) {
            /* E3 */
            d->code -= 0x40000000U;
            d->low -= 0x40000000U;
            d->high -= 0x40000000U;
        } else {
            break;
        }

        d->low <<= 1;
        d->high = (d->high << 1) | 1U;
        d->code = (d->code << 1) | (uint32_t)get_bit(&d->br);
    }
}

/* Decode one byte */

static uint8_t decode_byte(Decoder *d)
{
    int ctx = 1;
    uint16_t *p = d->probs[0];

    for (int i = 0; i < 8; i++) {
        int bit;
        decode_bit(d, &bit, &p[ctx]);
        ctx = (ctx << 1) | bit;
    }

    p = d->probs[((ctx & 255) >> 6) & 3];
    (void)p;

    return (uint8_t)ctx;
}

static void decoder_init(Decoder *d, const uint8_t *data, size_t size)
{
    memset(d, 0, sizeof(*d));
    d->low = 0;
    d->high = TOP;
    d->br.data = data;
    d->br.size = size;
    init_probs(d->probs);

    for (int i = 0; i < 32; i++)
        d->code = (d->code << 1) | (uint32_t)get_bit(&d->br);
}

/*
 * Decompress an FPAQ memory buffer.
 *
 * The uncompressed size is read from the stream header. The caller must
 * free(*output) after a successful call.
 */
int fpaq_decompress(const uint8_t *input, unsigned short input_size,
                    uint8_t **output, unsigned short *output_size)
{
    if (!input || !output || !output_size)
        return -1;

    *output = NULL;
    *output_size = 0;

    if (input_size < HEADER_SIZE || memcmp(input, "FPAQ", 4) != 0)
        return -1;

    uint64_t decoded_size = 0;
    for (int i = 0; i < 8; i++)
        decoded_size = (decoded_size << 8) | input[4 + i];

    if (decoded_size > (uint64_t)SIZE_MAX)
        return -1;

    size_t size = (size_t)decoded_size;
    uint8_t *result = (uint8_t *)malloc(size ? size : 1);
    if (!result)
        return -1;

    Decoder d;
    decoder_init(&d, input + HEADER_SIZE, input_size - HEADER_SIZE);

    for (size_t i = 0; i < size; i++)
        result[i] = decode_byte(&d);

    *output = result;
    *output_size = size;
    return 0;
}
