#include <stdint.h>
typedef double float64_t;
#include <stdlib.h>

int _u8_encode(
    uint64_t *input, size_t input_sz,		  //
    unsigned char **output, size_t *output_sz,	  //
    void *(*realloc_func)(void *, size_t, size_t) //
);
int _u8_decode(
    unsigned char *input, size_t input_sz,	  //
    uint64_t **output, size_t *output_sz,	  //
    void *(*realloc_func)(void *, size_t, size_t) //
);
int _f8_encode(
    float64_t *input, size_t input_sz,	       //
    unsigned char **output, size_t *output_sz, //
    void *(*alloc_func)(size_t, size_t)	       //
);
int _f8_decode(
    float64_t *input, size_t input_sz,	       //
    unsigned char **output, size_t *output_sz, //
    void *(*alloc_func)(size_t, size_t)	       //
);
int _zstd_decode(
    unsigned char *input, size_t input_sz,	  //
    unsigned char **output, size_t *output_sz,	  //
    void *(*realloc_func)(void *, size_t, size_t) //
);
int _zstd_encode(
    unsigned char *input, size_t input_sz,	  //
    unsigned char **output, size_t *output_sz,	  //
    void *(*realloc_func)(void *, size_t, size_t) //
);
