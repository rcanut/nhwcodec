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
	uint8_t context_index;
	uint8_t context_decrease;
    uint32_t low, high;
    uint64_t pending;
    BitWriter bw;
    uint16_t probs[4][256];
} Encoder;

/* Append one byte to a dynamically growing output buffer. */
static int buffer_append(BitWriter *w, uint8_t value)
{
    if (w->error)
        return -1;

    if (w->size == w->capacity) {
        size_t new_capacity = w->capacity ? w->capacity * 2 : 1024;

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

static const uint16_t initial_probs[4][256] = {
    {32768, 6389, 7946, 22175, 26012, 36729, 40434, 1876, 32049, 49850, 53835, 11260, 62334, 19984, 23698, 1705, 24584, 45304, 53037, 21028, 40454, 39488, 23825, 32019, 40478, 61620, 58059, 36670, 34171, 30106, 194, 58982, 28957, 26083, 37687, 57111, 30771, 39994, 39260, 49805, 32332, 43048, 42682, 23274, 46308, 25501, 24252, 30957, 49730, 42335, 46028, 61892, 31405, 54930, 32768, 36993, 32841, 32835, 27960, 28592, 25012, 32768, 32768, 6554, 31573, 31949, 28087, 32689, 32745, 22365, 32526, 59648, 46317, 15972, 57678, 5134, 62099, 3729, 54866, 7303, 13000, 50421, 23715, 44497, 36958, 39010, 33562, 35992, 29818, 31355, 27998, 28575, 28336, 27381, 28548, 43077, 26526, 20669, 48818, 23932, 44684, 24802, 40924, 2140, 25365, 28509, 46327, 3222, 25263, 30088, 34464, 32955, 28805, 25170, 22665, 21767, 30374, 25799, 33007, 23516, 25278, 20669, 32768, 32768, 32768, 32768, 6554, 32768, 55331, 2870, 61505, 878, 50332, 3769, 62231, 641, 62347, 8528, 43496, 3050, 63600, 4050, 56455, 60219, 39818, 44478, 20827, 14043, 36290, 54825, 7788, 24161, 41995, 60771, 37634, 24099, 43512, 51771, 28882, 20973, 52563, 39214, 34668, 27617, 45691, 39267, 33407, 26714, 42066, 33928, 35019, 26880, 34491, 31582, 31640, 30607, 38871, 37918, 44569, 41629, 42868, 37231, 35068, 29794, 37224, 37153, 35845, 31055, 32768, 41224, 25915, 52899, 22686, 61895, 18204, 51493, 22686, 36310, 23570, 32768, 32768, 40187, 30448, 23270, 31226, 54149, 5623, 29622, 27100, 22686, 41951, 26269, 35845, 53208, 2800, 25683, 29720, 30687, 28811, 32768, 27668, 37827, 33144, 34996, 28320, 30767, 23761, 28969, 19393, 25767, 20925, 32365, 30540, 30190, 30120, 32768, 29887, 37027, 29887, 29341, 24702, 32768, 41506, 14043, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 6554, 32768, 32768, 32768},
    {32768, 26606, 38584, 38703, 34208, 34747, 35073, 20955, 39626, 38052, 36205, 38406, 39799, 36833, 22825, 37830, 49291, 10435, 47373, 19186, 41638, 20088, 40534, 39424, 53661, 13943, 44676, 15001, 43907, 13902, 56252, 9113, 34066, 48599, 20420, 33208, 37891, 42975, 20313, 31854, 34492, 40649, 25765, 23749, 41957, 38471, 23043, 43756, 43866, 39876, 22534, 29462, 38623, 40412, 23476, 26568, 35027, 37336, 19635, 27974, 40309, 47127, 15696, 27407, 20471, 33322, 35342, 39498, 17944, 17154, 36929, 29007, 43317, 33153, 42688, 37000, 22489, 19549, 40321, 14133, 8346, 54516, 22163, 45015, 20408, 36703, 25884, 44877, 21023, 40690, 28306, 41593, 22360, 36888, 24942, 56145, 38192, 41268, 33616, 40189, 25074, 28351, 27069, 25058, 39564, 30286, 40221, 40929, 24430, 27989, 26711, 27085, 16801, 18156, 20270, 25647, 22552, 24645, 18682, 20590, 31838, 36263, 41422, 42749, 18926, 27887, 28629, 32129, 24494, 28941, 24892, 27329, 25597, 36853, 32929, 40634, 27374, 29524, 21533, 31406, 24618, 30662, 22925, 30160, 38973, 33565, 28879, 20611, 40583, 38475, 28434, 38762, 30195, 35695, 41168, 25278, 28540, 40945, 29576, 17561, 51216, 34800, 37623, 15769, 46957, 34691, 32041, 29701, 36156, 33045, 30344, 16697, 43063, 31591, 41575, 18204, 50736, 39102, 38568, 30926, 41802, 35105, 35036, 35660, 30894, 31046, 33264, 21220, 29948, 26783, 32241, 60581, 64999, 31226, 33970, 32768, 35549, 37060, 36151, 39882, 32437, 30278, 28879, 30069, 29708, 26251, 33361, 30855, 42537, 41863, 26068, 29357, 32768, 37515, 31160, 37443, 29154, 33662, 29642, 30812, 29501, 27430, 26728, 29341, 13617, 26077, 13661, 23564, 12495, 29635, 14053, 36198, 27471, 26956, 24884, 26231, 18760, 30263, 16909, 26672, 43541, 34668, 26279, 46399, 35630, 41439, 29924, 44776, 23400, 28826, 34654, 23860, 22999, 31719, 42754, 31470},
    {32768, 35096, 33902, 20233, 26625, 40484, 47806, 517, 18598, 38246, 32388, 38366, 52325, 29682, 18470, 475, 18428, 29995, 38687, 28451, 44474, 33208, 33741, 26365, 44998, 43563, 33342, 31585, 30714, 19547, 8073, 32768, 20829, 23093, 25101, 34643, 41110, 36109, 29701, 36118, 36441, 34983, 37000, 23228, 48139, 25998, 25371, 27845, 48119, 33128, 39094, 42060, 34659, 35855, 32059, 33445, 33007, 32271, 26464, 22825, 26322, 3641, 32768, 32768, 25726, 36431, 22144, 29603, 26067, 16582, 39184, 33858, 44801, 33790, 40697, 21458, 31859, 26370, 49122, 15723, 31641, 36667, 37007, 36174, 37878, 33556, 27230, 38573, 30253, 35951, 29768, 33759, 30731, 30288, 30536, 35957, 50441, 39723, 27685, 31347, 30004, 34015, 37744, 22680, 38968, 27439, 32255, 25975, 36852, 30435, 31818, 34509, 35658, 32768, 31878, 30475, 27355, 27061, 31660, 18780, 22141, 22282, 32768, 32768, 32768, 32768, 32768, 32768, 31451, 28829, 31480, 29270, 16953, 28950, 38529, 30438, 38652, 28811, 30647, 32768, 44133, 29708, 27143, 36979, 38173, 44056, 31715, 31779, 39289, 45517, 24670, 35845, 23637, 42419, 45906, 30954, 21287, 27000, 26330, 24627, 47975, 29666, 24618, 40793, 38991, 36067, 33758, 34912, 35184, 46008, 29945, 35459, 32194, 38011, 40173, 30178, 42089, 47709, 40818, 40222, 43512, 36210, 38485, 33152, 41614, 35440, 36914, 38129, 37176, 33566, 32768, 49572, 39010, 34668, 38173, 33648, 26497, 24201, 36843, 29806, 39131, 34275, 31797, 34268, 40170, 35040, 20141, 35512, 30610, 45642, 44948, 31062, 35219, 36339, 22523, 35521, 33120, 33727, 30666, 36404, 31929, 29567, 34343, 32453, 30305, 30704, 33875, 32610, 31902, 32585, 28142, 34345, 30558, 30301, 29855, 29549, 26883, 29906, 29147, 21995, 1311, 2521, 17348, 61895, 6554, 6554, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768},
    {32768, 31343, 48596, 42917, 47052, 33713, 60133, 1604, 38729, 35771, 29970, 37943, 57258, 46331, 22917, 87, 22317, 29684, 36407, 31607, 36536, 31309, 33628, 33578, 52814, 48004, 43646, 37676, 32258, 32019, 1478, 32768, 20829, 23226, 50750, 43494, 38496, 40189, 35302, 43752, 33241, 38775, 36870, 17822, 43554, 28798, 25077, 38145, 48188, 38467, 35273, 37351, 34926, 38992, 37162, 35714, 30626, 33232, 29595, 32308, 10745, 3641, 32768, 32768, 21370, 27883, 12603, 17265, 46162, 10690, 48279, 34736, 51587, 25576, 47989, 17829, 49864, 17763, 50999, 17265, 20176, 51992, 27369, 42045, 27742, 39105, 28574, 43989, 29262, 36799, 27238, 40669, 24004, 34752, 28986, 49375, 58982, 22686, 28248, 32768, 25586, 46646, 35602, 32233, 37765, 36474, 35216, 35496, 34121, 32252, 35433, 35788, 26018, 24905, 19600, 24431, 29031, 28986, 28436, 28209, 16225, 17727, 3641, 32768, 32768, 32768, 32768, 32768, 30518, 18849, 42634, 19471, 17460, 48656, 19471, 32768, 51949, 54388, 30100, 20852, 35681, 23331, 25254, 28325, 40478, 31093, 31248, 17941, 25428, 43915, 23981, 22334, 38926, 52166, 39706, 29441, 45452, 43657, 32343, 27430, 55797, 35102, 27143, 15595, 44988, 40737, 38515, 25479, 50417, 41741, 44018, 21624, 41148, 36040, 29147, 19270, 40147, 41913, 43075, 28393, 43927, 40478, 38709, 21410, 30828, 26896, 33823, 22021, 32187, 28375, 36540, 55832, 32768, 6554, 32768, 6554, 48188, 42850, 26526, 51493, 35681, 46327, 43254, 26322, 46646, 28875, 27525, 30602, 41807, 36095, 42617, 26728, 38529, 29047, 33098, 34724, 36151, 28615, 28094, 37801, 25314, 34527, 31193, 33495, 19274, 25021, 20637, 30388, 22588, 26667, 18538, 23972, 31012, 32166, 31800, 31304, 28855, 28824, 26951, 30844, 29393, 27661, 24030, 17348, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768},
};

static void init_probs(uint16_t p[4][256])
{
    //memcpy(p, initial_probs, sizeof(initial_probs));
	
	for (int i = 0; i < 4; i++)
        for (int j = 0; j < 256; j++)
            p[i][j] = (initial_probs[i][j] + 32768) >> 1;
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

    update_prob(prob, bit, e->context_decrease);

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
    int context2, ctx = 1;
    uint16_t *p = e->probs[e->context_index];
	
	context2 = e->context_index;

    for (int i = 7; i >= 0; i--) {
        int bit = (value >> i) & 1;
        encode_bit(e, bit, &p[ctx]);
        ctx = (ctx << 1) | bit;
    }

	e->context_index = (value >> 6) & 3;
	
	if (abs(e->context_index-context2)==3 && !e->context_decrease) e->context_decrease = 1;
	else e->context_decrease = 0;
}

/* Initialization */

static void encoder_init(Encoder *e)
{
    memset(e, 0, sizeof(*e));
    e->low = 0;
    e->high = TOP;
	e->context_index = 0;
	e->context_decrease = 0;
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

  /*(void)buffer_append(&header, 'F');
    (void)buffer_append(&header, 'P');
    (void)buffer_append(&header, 'A');
    (void)buffer_append(&header, 'Q');*/

    uint64_t n = (uint64_t)input_size;
    for (int i = 1; i >= 0; i--)
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

