#include "encode.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <zstd.h> // for ZSTD support

#define TE_VER_MASK 0b11000000 /* 4 binary version */
#define TE__SZ_MASK 0b00110000 /* 4 type of size */
#define TE___D_MASK 0b00001111 /* 2^4 type of payload */

static const uint8_t __placeholder__ __attribute__((unused)) = 0, //
    TE_VER = 0b00000000,					  //
    TE_SZ1 = 0b00010000,					  // 1 bytes length
    TE_SZ2 = 0b00100000,					  // 2 bytes length
    TE_SZ3 = 0b00110000,					  // 3 bytes length
    TE_DI8 = 0b00000001,					  // encoded int8   (8bytes)
    TE_DF8 = 0b00000010,					  // encoded float8 (8bytes)
    TE_ZST = 0b00001000,					  // encoded with zstd
    __placeholder2__ __attribute__((unused)) = 0;

int _zstd_encode(
    unsigned char *input, size_t input_sz,	  //
    unsigned char **output, size_t *output_sz,	  //
    void *(*realloc_func)(void *, size_t, size_t) //
)
{
	size_t dn = ZSTD_compressBound(input_sz);
	unsigned char *d = realloc_func(NULL, 0, dn);
	dn = ZSTD_compress(d, dn, input, input_sz, 0);

	*output = d;
	*output_sz = dn;

	return 0;
}

int _zstd_decode(
    unsigned char *input, size_t input_sz,	  //
    unsigned char **output, size_t *output_sz,	  //
    void *(*realloc_func)(void *, size_t, size_t) //
)
{
	int dn = ZSTD_getFrameContentSize(input, input_sz);
	unsigned char *d = alloca(dn);
	dn = ZSTD_decompress(d, dn, input, input_sz);
	if (dn < 0) {
		const char *e = ZSTD_getErrorName(dn);
		printf("%s\n", e);
	}

	if (*output_sz < dn) {
		*output = realloc_func(*output, *output_sz, dn);
	}

	memcpy((*output), d, dn);
	*output_sz = dn;
	return 0;
}

typedef struct BitStream {
	uint8_t *buffer;
	size_t buffer_size;
	size_t buffer_offset_current;

	uint8_t bits_current;
	uint8_t bits_current_remaining; // (0, 8]
} BitStream;

static BitStream
bitstream_create(void *source, size_t size, size_t offset, void *(*realloc_func)(void *, size_t, size_t))
{
	return (BitStream){
	    .buffer = source,
	    .buffer_size = size,
	    .buffer_offset_current = offset,
	    .bits_current = 0,
	    .bits_current_remaining = 8,
	};
}

static void bitstream_flush(BitStream *bs)
{
	if (bs->bits_current_remaining == 8)
		return;

	if (bs->buffer_size <= bs->buffer_offset_current) {
		// ERROR!
	}

	bs->buffer[bs->buffer_offset_current++] = bs->bits_current;
	bs->bits_current = 0, bs->bits_current_remaining = 8;
}

static void bitstream_write_bit_n(BitStream *bs, uint8_t bits, uint8_t n)
{
	if (bs->bits_current_remaining >= n) { // can write to bs->current_value
		bs->bits_current_remaining -= n;
		bs->bits_current |= (bits & ((1 << n) - 1)) << bs->bits_current_remaining;

		if (bs->bits_current_remaining == 0) {
			bitstream_flush(bs);
		}

		return;
	}

	// can not write to bs->current_value, split to 2 parts
	uint8_t part = n - bs->bits_current_remaining;
	uint8_t b1 = bits >> part;	 // save to bs->buffer[bs->buffer_size]
	uint8_t b2 = bits << (8 - part); // save to bs->current_value

	bs->bits_current |= b1;
	bitstream_flush(bs);

	bs->bits_current = b2;
	bs->bits_current_remaining -= part;
}

static void bitstream_write_64_n(BitStream *bs, uint64_t bits, uint8_t n)
{
	for (uint8_t i = 0; i < 8; ++i) {
		uint8_t off = (uint8_t[]){0, 8, 16, 24, 32, 40, 48, 56}[i];

		if (n < off)
			break;

		uint8_t part = n - off;
		bitstream_write_bit_n(bs, (uint8_t)(bits >> off), part > 8 ? 8 : part);
	}
}

static inline uint8_t bitstream_read_bit_incurrent_value_n(BitStream *bs, unsigned char n)
{
	bs->bits_current_remaining -= n;
	return (bs->buffer[bs->buffer_offset_current] >> bs->bits_current_remaining) & ((1 << n) - 1);
}

static uint8_t bitstream_read_bit_n(BitStream *bs, unsigned char n)
{
	if (n == 0)
		return 0;

	uint8_t ret = 0;

	if (bs->bits_current_remaining >= n) { // read from bs->current_value
		ret = bitstream_read_bit_incurrent_value_n(bs, n);

		if (bs->bits_current_remaining == 0) {
			bs->bits_current_remaining = 8;
			bs->buffer_offset_current++;
		}

		return ret;
	}

	// bs->current_value dose not have enough bits to read. split the reading
	uint8_t part = n - bs->bits_current_remaining;
	ret = bitstream_read_bit_incurrent_value_n(bs, bs->bits_current_remaining) << part;
	bs->buffer_offset_current++;
	bs->bits_current_remaining = 8;
	ret |= bitstream_read_bit_incurrent_value_n(bs, part);

	return ret;
}

static inline uint64_t bitstream_read_64_n(BitStream *bs, uint8_t n)
{
	uint64_t ret = 0;

	for (uint8_t i = 0; i < 8; ++i) {
		uint8_t off = (const uint8_t[]){0, 8, 16, 24, 32, 40, 48, 56}[i];

		if (n < off)
			break;

		uint8_t part = n - off;
		ret |= (uint64_t)bitstream_read_bit_n(bs, part > 8 ? 8 : part) << off;
	}

	return ret;
}

// the delta of delta encoded for int datatype
//
// binary format:
//   [[1-3bytes], [1*SZ], [1*SZ],         [bit stream]]
//    ^ count     ^       ^ delta (v1-v0) ^ the real data
//                |
//            the first value
//
// bitstream format:
//   control information
//     0b0      = delta of delta is equal with previous value. value size = 0
//     0b10     = delta of delta w (-63, 64)                 . value size = 7
//     0b110    = delta of delta w (-255, 256)               . value size = 9
//     0b1110   = delta of delta w (-2047, 2048)             . value size = 12
//     0b11110  = delta of delta w (i32 min, i32 max)        . value size = 32
//     0b111110 = delta of delta w (i64 min, i64 max)        . value size = 64
int _u8_encode(
    uint64_t *input, size_t input_sz,		  //
    unsigned char **output, size_t *output_sz,	  //
    void *(*realloc_func)(void *, size_t, size_t) //
)
{
	// encoding type = uint64_t
	uint8_t header = TE_VER | TE_DI8;

	// set output size
	uint8_t output_len_size = 0;
	if (input_sz > 0xFFFFFF)
		return -1; // will overflow
	else if (input_sz > 0x00FFFF)
		header |= TE_SZ3, output_len_size = 3;
	else if (input_sz > 0x0000FF)
		header |= TE_SZ2, output_len_size = 2;
	else
		header |= TE_SZ1, output_len_size = 1;

	// alloc memory
	*output_sz = 1 /* header */ + output_len_size /* length */ + input_sz * 8 * 1.2 /* value */;
	*output = realloc_func(NULL, 0, *output_sz);
	unsigned char *output_buffer = *output;

	// write the header out
	output_buffer[0] = header;
	output_buffer += 1;

	// write out the input count
	uint32_t input_entity_size = input_sz;
	memcpy(output_buffer, &input_entity_size, output_len_size);
	output_buffer += output_len_size;

	// write out the first value
	memcpy(output_buffer, input, 8);
	output_buffer += 8;

	// write the first delta value out (v1 - v0).
	__int128 delta = input[1] - input[0];
	int64_t dod = (int64_t)delta;
	memcpy(output_buffer, &dod, 8);
	output_buffer += 8;

	// structure the output bitstream
	BitStream bs = bitstream_create(output_buffer, output_buffer - *output, 0, realloc_func);
	for (size_t i = 2; i < input_entity_size; ++i) {
		// double_delta = (v[i] - v[i-1]) - (v[i-1] - v[i-2])
		//              = v[i] -2*v[i-1] + v[i-2]
		int64_t double_delta = input[i] - 2 * input[i - 1] + input[i - 2];

		uint8_t control = 0, control_len = 0, value_len_without_sign = 0;
		if (double_delta == 0)
			control = 0b0, control_len = 1, value_len_without_sign = 0;
		else if (-63 < double_delta && double_delta < 64)
			control = 0b10, control_len = 2, value_len_without_sign = 6;
		else if (-255 < double_delta && double_delta < 256)
			control = 0b110, control_len = 3, value_len_without_sign = 8;
		else if (-2047 < double_delta && double_delta < 2048)
			control = 0b1110, control_len = 4, value_len_without_sign = 11;
		else if (INT32_MIN < double_delta && double_delta < INT32_MAX)
			control = 0b11110, control_len = 5, value_len_without_sign = 31;
		else
			control = 0b111110, control_len = 6, value_len_without_sign = 63;

		uint8_t sign = double_delta < 0;
		double_delta = llabs(double_delta);

		bitstream_write_bit_n(&bs, control, control_len);
		if (value_len_without_sign != 0) {
			bitstream_write_bit_n(&bs, sign, 1);
			bitstream_write_64_n(&bs, double_delta, value_len_without_sign);
		}
	}

	bitstream_flush(&bs);
	*output_sz = bs.buffer_offset_current + 1 + output_len_size + 8 + 8;
	return 0;
}

int _u8_decode(
    unsigned char *input, size_t input_sz,	  //
    uint64_t **output, size_t *output_sz,	  //
    void *(*realloc_func)(void *, size_t, size_t) //
)
{
	uint8_t header = input[0];
	input += 1;

	if ((header & TE_VER_MASK) != TE_VER)
		return -1;

	if ((header & TE___D_MASK) != TE_DI8) {
		return -1;
	}

	int8_t encode_sz_len = 0;
	switch (header & TE__SZ_MASK) {
	case TE_SZ1:
		encode_sz_len = 1;
		break;
	case TE_SZ2:
		encode_sz_len = 2;
		break;
	case TE_SZ3:
		encode_sz_len = 3;
		break;
	default:
		return -1;
	}

	uint32_t output_entity_num = 0;
	memcpy(&output_entity_num, input, encode_sz_len);
	input += encode_sz_len;

	// alloc memcpy
	*output_sz = output_entity_num * 8;
	*output = realloc_func(NULL, 0, *output_sz);

	// the first value
	int64_t last = ((int64_t *)input)[0];
	input += 8;
	(*output)[0] = last;

	// the second value
	int64_t delta = ((int64_t *)input)[0];
	input += 8;
	last = last + delta;
	(*output)[1] = last;

	BitStream bs = bitstream_create(input, input_sz - 1 - encode_sz_len - 2 * 8, 0, NULL);
	for (size_t i = 2; i < output_entity_num; ++i) {
		for (uint8_t value_size_index = 0; value_size_index < 6; ++value_size_index) {

			uint8_t control = bitstream_read_bit_n(&bs, 1);
			if ((control & 0b01) == 0b01) // control bit not 0b, get next value size
				continue;

			const uint8_t value_size = (uint8_t[]){0, 6, 8, 11, 31, 63}[value_size_index];

			int64_t sign = 0, double_delta = 0;
			if (value_size != 0) {
				sign = bitstream_read_bit_n(&bs, 1);
				double_delta = bitstream_read_64_n(&bs, value_size);

				double_delta *= (sign != 0) ? -1 : 1;
				delta += double_delta;
			}

			last += delta;
			(*output)[i] = last;

			break;
		}
	}

	return 0;
}

int _f8_encode(
    float64_t *input, size_t input_sz,	       //
    unsigned char **output, size_t *output_sz, //
    void *(*alloc_func)(size_t, size_t)	       //
)
{
	return 0;
}

int _f8_decode(
    float64_t *input, size_t input_sz,	       //
    unsigned char **output, size_t *output_sz, //
    void *(*alloc_func)(size_t, size_t)	       //
)
{
	return 0;
}
