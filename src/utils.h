#ifndef UTILS_H                          // [1] Header guard to prevent multiple inclusion
#define UTILS_H

#include <stddef.h>                      // [2] For size_t type
#include <stdint.h>                      // [3] For fixed-width integer types
#include <stdbool.h>                     // [4] For boolean type support

/* [5] Memory helpers */
/** [5.1] fast_memset: Fills a memory block with a specified byte value. */
void fast_memset(void *dest, unsigned char val, size_t n);
/** [5.2] fast_memcpy: Copies memory from source to destination. Assumes non-overlapping regions. */
void fast_memcpy(void *dest, const void *src, size_t n);

/* [6] String helpers */
/** [6.1] tiny_strlen: Returns the length of a null-terminated string. */
size_t tiny_strlen(const char *s);
/** [6.2] strcpy_s: Safely copies a string into a destination buffer. Ensures null termination and avoids overflow. */
void strcpy_s(char *dst, size_t dst_size, const char *src);

/* [7] Number formatting */
/** [7.1] int_to_dec: Converts an integer to a decimal string. Returns number of characters written (excluding null terminator). */
int int_to_dec(char *out, int value);
/** [7.2] append_uint_zero_pad: Converts an unsigned int to a zero-padded string. Useful for formatting time, dates, or identifiers. */
int append_uint_zero_pad(char *dst, unsigned int value, int width);
/** [7.3] format_float_two_decimals: Formats a double with two decimal places. Ideal for prices, measurements, or precise values. */
int format_float_two_decimals(char *out, double val);
/** [7.4] format_float_one_decimal: Formats a float with one decimal place. Commonly used for temperature or rounded values. */
int format_float_one_decimal(char *out, float val);

/* [8] Safe append */
/** [8.1] safe_append_str: Appends a string to a buffer at a given position. Uses strcpy_s internally to ensure safety. */
int safe_append_str(char *dst, int dst_size, int pos, const char *src);

/* [9] Byte to hex helper */
/** [9.1] u8_to_hex_sp: Converts a byte to a hex string with a trailing space. Example: 0x3F → "3F " */
void u8_to_hex_sp(char *out, uint8_t v);

/* [10] INT32_MIN definition */
#ifndef INT32_MIN                        // [10.1] Define INT32_MIN manually if not available
#define INT32_MIN (-2147483647 - 1)      // [10.2] Minimum value for a 32-bit signed integer
#endif

#endif /* UTILS_H */                    // [11] End of header guard