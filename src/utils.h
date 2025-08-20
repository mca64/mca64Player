#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>  /* For size_t type */
#include <stdint.h>  /* For fixed-width integer types */
#include <stdbool.h> /* For boolean type support */

/* [1] Memory helpers */
void fast_memset(void *dest, unsigned char val, size_t n); /* Fill memory with a byte value */
void fast_memcpy(void *dest, const void *src, size_t n);   /* Copy memory (non-overlapping) */

/* [2] String helpers */
size_t tiny_strlen(const char *s);                         /* Get length of null-terminated string */
void strcpy_s(char *dst, size_t dst_size, const char *src);/* Safe string copy with null-termination */

/* [3] Number formatting */
int int_to_dec(char *out, int value);                      /* Convert int to decimal string */
int append_uint_zero_pad(char *dst, unsigned int value, int width); /* Unsigned int to zero-padded string */
int format_float_two_decimals(char *out, double val);      /* Format double with two decimals */
int format_float_one_decimal(char *out, float val);        /* Format float with one decimal */

/* [4] Safe append */
int safe_append_str(char *dst, int dst_size, int pos, const char *src); /* Append string at position */

/* [5] Byte to hex helper */
void u8_to_hex_sp(char *out, uint8_t v);                   /* Byte to hex string with trailing space */

#ifndef INT32_MIN
#define INT32_MIN (-2147483647 - 1)                        /* Minimum value for a 32-bit signed integer */
#endif

#endif /* UTILS_H */