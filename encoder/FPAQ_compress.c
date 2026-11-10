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
        v -= v >> 4;
    else if (v < 65472)
        v += (65472 - v + 63) >> 4;
	
	if (v > 65535) v = 65535;

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

static void encode_bit(Encoder *e, int bit, uint16_t *prob)
{
    uint64_t range = (uint64_t)e->high - e->low + 1;

    uint32_t split = e->low +
        (uint32_t)((range * (PSCALE - *prob)) >> 16);

    if (split <= e->low)
        split = e->low + 1;

    if (split > e->high)
        split = e->high;

    if (bit == 0)
        e->high = split - 1;
    else
        e->low = split;

    update_prob(prob, bit);

    for (;;) {
        if (e->high < 0x80000000U) {
            output_bit_plus_pending(e, 0);
        } else if (e->low >= 0x80000000U) {
            output_bit_plus_pending(e, 1);
            e->low -= 0x80000000U;
            e->high -= 0x80000000U;
        } else if (e->low >= 0x40000000U &&
                   e->high < 0xC0000000U) {
            e->pending++;
            e->low -= 0x40000000U;
            e->high -= 0x40000000U;
        } else {
            break;
        }

        e->low <<= 1;
        e->high = (e->high << 1) | 1U;
    }
}

/* Encode one byte */

static void encode_byte(Encoder *e, uint8_t value)
{
    int ctx = 1;
    uint16_t *p = e->probs[0];

    for (int i = 7; i >= 0; i--) {
        int bit = (value >> i) & 1;
        encode_bit(e, bit, &p[ctx]);
        ctx = (ctx << 1) | bit;
    }

    p = e->probs[(value >> 4) & 3];
    (void)p;
}

/* Initialization */

static void encoder_init(Encoder *e)
{
    memset(e, 0, sizeof(*e));
    e->low = 0;
    e->high = TOP;
    init_probs(e->probs);
}

/* Finalization */

static void encoder_finish(Encoder *e)
{
    e->pending++;
    output_bit_plus_pending(e,
        e->low < 0x40000000U ? 0 : 1);
    flush_bits(&e->bw);
}

/*
 * Compress an input memory buffer.
 *
 * The compressed stream has the same format as the file version:
 * 4-byte "FPAQ" magic, 8-byte big-endian uncompressed size, then
 * the arithmetic-coded payload.
 *
 * On success, *output is malloc-allocated and *output_size is its size.
 * The caller must free(*output).
 */
int fpaq_compress(const uint8_t *input, size_t input_size,
                  uint8_t **output, unsigned short *output_size)
{
    if (!output || !output_size || (!input && input_size != 0))
        return -1;

    *output = NULL;
    *output_size = 0;

    if (sizeof(size_t) > sizeof(uint64_t) &&
        input_size > (size_t)UINT64_MAX)
        return -1;

    BitWriter header;
    memset(&header, 0, sizeof(header));

    (void)buffer_append(&header, 'F');
    (void)buffer_append(&header, 'P');
    (void)buffer_append(&header, 'A');
    (void)buffer_append(&header, 'Q');

    uint64_t n = (uint64_t)input_size;
    for (int i = 7; i >= 0; i--)
        (void)buffer_append(&header, (uint8_t)(n >> (i * 8)));

    if (header.error) {
        free(header.data);
        return -1;
    }

    Encoder e;
    encoder_init(&e);

    for (size_t i = 0; i < input_size; i++) {
        encode_byte(&e, input[i]);
        if (e.bw.error) {
            free(header.data);
            free(e.bw.data);
            return -1;
        }
    }

    encoder_finish(&e);

    if (e.bw.error || e.bw.size > SIZE_MAX - header.size) {
        free(header.data);
        free(e.bw.data);
        return -1;
    }

    size_t total_size = header.size + e.bw.size;
    uint8_t *result = (uint8_t *)realloc(header.data, total_size ? total_size : 1);
    if (!result) {
        free(header.data);
        free(e.bw.data);
        return -1;
    }

    if (e.bw.size)
        memcpy(result + header.size, e.bw.data, e.bw.size);

    free(e.bw.data);
    *output = result;
    *output_size = total_size;
    return 0;
}

