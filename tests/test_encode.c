#include <stdbool.h>
#include <stdint.h>
typedef double float64_t;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h> // for MIN/MAX
#include <time.h>

extern int _u8_encode(
    uint64_t *input, size_t input_sz,		  //
    unsigned char **output, size_t *output_sz,	  //
    void *(*realloc_func)(void *, size_t, size_t) //
);
extern int _u8_decode(
    unsigned char *input, size_t input_sz,	  //
    unsigned char **output, size_t *output_sz,	  //
    void *(*realloc_func)(void *, size_t, size_t) //
);

extern int _zstd_decode(
    unsigned char *input, size_t input_sz,	  //
    unsigned char **output, size_t *output_sz,	  //
    void *(*realloc_func)(void *, size_t, size_t) //
);

extern int _zstd_encode(
    unsigned char *input, size_t input_sz,	  //
    unsigned char **output, size_t *output_sz,	  //
    void *(*realloc_func)(void *, size_t, size_t) //
);

static double clock_gettime_ms(struct timespec *s) { return (double)s->tv_sec * 10e3 + (double)s->tv_nsec / 10e3; }

static void *test_realloc(void *old_ptr, size_t old_sz, size_t new_sz)
{
	if (old_ptr == NULL)
		return calloc(1, new_sz);

	return realloc(old_ptr, new_sz);
}

static void test_print_bytes(char *prefix, unsigned char *p, int size)
{
	printf("%s ", prefix);
	for (int i = 0; i < size; i++) {
		printf("%02hhX ", p[i]);
	}
	printf("\n");
}

#define PATTERN_RAND 0
#define PATTERN_ORDERED 1
#define PATTERN_ZERO 2

typedef struct Case {
	const char *name;
	bool skip;
	int pattern;

	size_t base;

	unsigned char *in;
	size_t in_sz;

	unsigned char *out;
	size_t out_sz;

	typeof(_u8_encode) *u8_encode;
	typeof(_u8_decode) *u8_decode;
} Case;

static bool ok = true;

static void run_memcpy()
{
	printf("running bench  [u8 / memcpy]");

	struct timespec tstart = {0, 0}, tend = {0, 0};

	uint64_t en_bytes = 0;
	double en_ms = 0;

	size_t input_sz = 20480 * 8, nround = 5000;
	uint64_t *input = malloc(input_sz);
	uint64_t *output = malloc(input_sz);
	for (int round = 0; round < nround; ++round) {
		clock_gettime(CLOCK_MONOTONIC, &tstart);
		memmove(output, input, input_sz);
		clock_gettime(CLOCK_MONOTONIC, &tend);
		en_ms = clock_gettime_ms(&tend) - clock_gettime_ms(&tstart);
		en_bytes += input_sz;
	}

	const double _1MiB = 1024 * 1024;
	const double _1GiB = 1024 * _1MiB;
	printf(
	    "\t  ... [%.2fGiB/s | %.2fMiB -> %.2fMiB]\n",
	    (1.0 * nround * input_sz / _1GiB) / (en_ms / 10e3),
	    (1.0 * nround * input_sz / _1MiB),
	    (double)en_bytes / _1MiB);
}

static void run_zstd()
{
	printf("running bench  [u8 / zstd]");

	size_t input_sz = 20480 * 8, nround = 5000;
	unsigned char *input = malloc(input_sz);

	uint64_t en_bytes = 0;
	double en_ms = 0, de_ms = 0;

	for (int round = 0; round < nround; ++round) {

		unsigned char *out1 = NULL, *out2 = NULL;
		struct timespec tstart = {0, 0}, tend = {0, 0};
		size_t out_sz1 = 0, out_sz2 = 0;

		for (int i = 0; i < input_sz; ++i) {
			input[i] = rand();
		}

		clock_gettime(CLOCK_MONOTONIC, &tstart);
		int ret1 = _zstd_encode(input, input_sz, &out1, &out_sz1, test_realloc);
		clock_gettime(CLOCK_MONOTONIC, &tend);
		en_ms = clock_gettime_ms(&tend) - clock_gettime_ms(&tstart);
		en_bytes += out_sz1;

		clock_gettime(CLOCK_MONOTONIC, &tstart);
		int ret2 = _zstd_decode(out1, out_sz1, &out2, &out_sz2, test_realloc);
		clock_gettime(CLOCK_MONOTONIC, &tend);
		de_ms = clock_gettime_ms(&tend) - clock_gettime_ms(&tstart);

		if (memcmp(input, out2, input_sz) != 0) {
			printf("\n		buffer compare not match \n");
			test_print_bytes("I ", (unsigned char *)input, input_sz * 8);
			test_print_bytes("O ", (unsigned char *)out2, out_sz2);
		}

		free(out1);
		free(out2);
	}

	const double _1MiB = 1024 * 1024;
	const double _1GiB = 1024 * _1MiB;
	printf(
	    "\t  ... [decode %.2fGiB/s encode %.2fGiB/s | %.2fMiB -> %.2fMiB]\n",
	    (1.0 * nround * input_sz / _1GiB) / (en_ms / 10e3),
	    (1.0 * nround * input_sz / _1GiB) / (de_ms / 10e3),
	    (1.0 * nround * input_sz / _1MiB),
	    (double)en_bytes / _1MiB);

	free(input);
}

static void run(Case *c)
{
	if (c->skip)
		return;

	printf("running encode [%s]", c->name);

	size_t out_sz = 0;
	unsigned char *out = NULL;

	int ret = c->u8_encode((uint64_t *)c->in, c->in_sz, &out, &out_sz, test_realloc);
	if (ret != 0) {
		printf("\n		error %d", ret);
		goto err;
	}

	int size = MIN(c->out_sz, out_sz);
	if (c->out_sz != out_sz || memcmp(out, c->out, size) != 0) {
		printf("\n		buffer compare not match \n");
		test_print_bytes("actual", out, out_sz);
		test_print_bytes("expect", c->out, c->out_sz);
		goto err;
	}

	printf(" \t  ... OK \n");

	free(out);
	out_sz = 0;
	out = NULL;

	printf("running decode [%s]", c->name);

	ret = c->u8_decode(c->out, c->out_sz, &out, &out_sz, test_realloc);
	if (ret != 0) {
		printf("\n		error %d", ret);
		goto err;
	}

	size = MIN(c->in_sz * 8, out_sz);
	if (c->in_sz * 8 != out_sz || memcmp(out, c->in, size) != 0) {
		printf("\n		buffer compare not match \n");
		test_print_bytes("actual", out, out_sz);
		test_print_bytes("expect", c->in, c->in_sz * 8);
		goto err;
	}

	printf(" \t  ... OK \n");

	free(out);

	return;

err:
	printf("\n");
	ok = false;
	free(out);
}

static void run_rand_u8(Case *c)
{
	if (c->skip)
		return;

	printf("running bench  [%s]", c->name);

	size_t out1_sz = 0, out2_sz = 0;
	unsigned char *out1 = NULL, *out2 = NULL;

	size_t input_sz = 20480, nround = 5000;
	uint64_t *input = malloc(input_sz * MAX(c->base, 8));

	uint64_t en_bytes = 0;
	double en_ms = 0, de_ms = 0;

	for (int round = 0; round < nround; ++round) {
		for (int i = 0; i < input_sz; ++i) {
			if (c->pattern == PATTERN_RAND)
				input[i] = ((uint64_t)rand() << 32 | rand());
			else if (c->pattern == PATTERN_ORDERED)
				input[i] = i;
			else if (c->pattern == PATTERN_ZERO)
				input[i] = 0;
		}

		struct timespec tstart = {0, 0}, tend = {0, 0};

		clock_gettime(CLOCK_MONOTONIC, &tstart);
		int ret1 = c->u8_encode(input, input_sz, &out1, &out1_sz, test_realloc);
		clock_gettime(CLOCK_MONOTONIC, &tend);
		en_ms = clock_gettime_ms(&tend) - clock_gettime_ms(&tstart);
		en_bytes += out1_sz;

		clock_gettime(CLOCK_MONOTONIC, &tstart);
		int ret2 = c->u8_decode(out1, out1_sz, &out2, &out2_sz, test_realloc);
		clock_gettime(CLOCK_MONOTONIC, &tend);
		de_ms = clock_gettime_ms(&tend) - clock_gettime_ms(&tstart);

		if (ret1 != 0 || ret2 != 0) {
			printf("\n		error encode %d decode %d", ret1, ret2);
			goto err;
		}

		if (memcmp(input, out2, input_sz * 8) != 0) {
			printf("\n		buffer compare not match \n");
			test_print_bytes("I ", (unsigned char *)input, input_sz * 8);
			test_print_bytes("O ", (unsigned char *)out2, out2_sz);
			goto err;
		}

		free(out1), free(out2);
	}

	const double _1MiB = 1024 * 1024;
	const double _1GiB = 1024 * _1MiB;
	printf(
	    "\t  ... [decode %.2fGiB/s encode %.2fGiB/s | %.2fMiB -> %.2fMiB]\n",
	    (1.0 * c->base * nround * input_sz / _1GiB) / (en_ms / 10e3),
	    (1.0 * c->base * nround * input_sz / _1GiB) / (de_ms / 10e3),
	    (1.0 * c->base * nround * input_sz / _1MiB),
	    (double)en_bytes / _1MiB);

	free(input);
	return;

err:
	ok = false;
	free(input);
	free(out1), free(out2);
	printf("\n");
}

int main()
{
	srand(0);

	run(&(Case){
	    .name = "u8 / normal",
	    .base = 8,
	    .in = (unsigned char *)(int64_t[]){1, 2, 3},
	    .in_sz = 3,
	    .out =
		(unsigned char[]){
		    // clang-format off
			0b00000001 | 0b00010000,						// header DI8 SZ1
			0x03,											// count
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // first value
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // delta

			// last  = 2  next = 3  delta = 1
			// dod   = delta - (next - last) = 0
			// data: 0b(1bit), padding = 8-1 = 7bit
			0b00000000,

		    // clang-format on
		},

	    .out_sz = 1 + 1 + 8 * 2 + 1,
	    .u8_encode = _u8_encode,
	    .u8_decode = _u8_decode,
	});

	run(&(Case){
	    .name = "u8 / signbit",
	    .base = 8,
	    .in = (unsigned char *)(int64_t[]){1, 2, 1},
	    .in_sz = 3,
	    .out =
		(unsigned char[]){
		    // clang-format off
			0b00000001 | 0b00010000,						// header DI8 SZ1
			0x03,											// count
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // first value
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // delta

			// last = 2  next = 1  delta = -1
			// dod  = delta - (next - last) = -2
			// data: 10b(2bit)  + 1b(1bit) + 000010b(6bit), padding = 16 - 9 = 7bit
			0b10100001,
			0b00000000,
		    // clang-format on
		},
	    .out_sz = 1 + 1 + 8 * 2 + 2,
	    .u8_encode = _u8_encode,
	    .u8_decode = _u8_decode,
	});

	run(&(Case){
	    .name = "u8 / overflow",
	    .base = 8,
	    .in =
		(unsigned char *)(int64_t[]){
		    0x6b8b4567327b23c6,
		    0x643c986966334873,
		    0x74b0dc5119495cff,
		},
	    .in_sz = 3,
	    .out =
		(unsigned char[]){
		    // clang-format off
			0b00000001 | 0b00010000,						// header DI8 SZ1
			3,												// count
			0xc6, 0x23, 0x7b, 0x32, 0x67, 0x45, 0x8b, 0x6b, // first value
			0xad, 0x24, 0xb8, 0x33, 0x02, 0x53, 0xb1, 0xf8, // delta
			// index = 3, dod = +xxxx
			// data: 111110(6bit), 0b(1bit) + 0x17c2f0e57f5defdf(64bit)
			0b11111000 | 0b00 | 0b1, 0xbf, 0xde, 0xba, 0xff, 0xcb, 0xe1, 0x84, 0x5c
		    // clang-format on
		},
	    .out_sz = 1 + 1 + 8 * 2 + 1 + 8,
	    .u8_encode = _u8_encode,
	    .u8_decode = _u8_decode,
	});

	run_rand_u8(&(Case){
	    .name = "u8 / rand",
	    .base = 8,
	    .pattern = PATTERN_RAND,
	    .u8_encode = _u8_encode,
	    .u8_decode = _u8_decode,
	});

	run_rand_u8(&(Case){
	    .name = "u8 / ordered",
	    .base = 8,
	    .pattern = PATTERN_ORDERED,
	    .u8_encode = _u8_encode,
	    .u8_decode = _u8_decode,
	});

	run_memcpy();
	run_zstd();

	return !ok;
}
