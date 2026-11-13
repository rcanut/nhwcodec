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
#define HEADER_SIZE 2

typedef struct {
    const uint8_t *data;
    size_t size;
    size_t pos;
    uint8_t byte;
    int bits;
} BitReader;

typedef struct {
	uint8_t context_index;
	uint8_t context_decrease;
    uint32_t low, high, code;
    BitReader br;
    uint16_t probs[4][256];
} Decoder;

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

static void update_prob(uint16_t *p, int bit, uint8_t ctx_dec)
{
    unsigned v = *p;

    if (bit == 0)
	{
		v -= v >> 4;
		
		if (ctx_dec) v += v >> 6;
		else v += 18;
	}
    else if (v < 64950)
	{
		v += (64950 - v + 63) >> 4;
		
		if (ctx_dec) v -= v >> 6;
		else v -= 18;
	}
	
	if (v < 0) v = 0;
	else if (v > 65535) v = 65535;

    *p = (uint16_t)v;
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

    update_prob(prob, *bit, d->context_decrease);

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
    int context2, ctx = 1;
    uint16_t *p = d->probs[d->context_index];
	
	context2 = d->context_index;

    for (int i = 0; i < 8; i++) {
        int bit;
        decode_bit(d, &bit, &p[ctx]);
        ctx = (ctx << 1) | bit;
    }

	d->context_index = ((ctx & 255) >> 6) & 3;
	
	if (abs(d->context_index-context2)==3 && !d->context_decrease) d->context_decrease = 1;
	else d->context_decrease = 0;

    return (uint8_t)ctx;
}

static void decoder_init(Decoder *d, const uint8_t *data, size_t size)
{
    memset(d, 0, sizeof(*d));
    d->low = 0;
    d->high = TOP;
    d->br.data = data;
    d->br.size = size;
	d->context_index = 0;
	d->context_decrease = 0;
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

    if (input_size < HEADER_SIZE)
        return -1;

    uint64_t decoded_size = 0;
        
	decoded_size = (input[0] << 8) | input[1];

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
