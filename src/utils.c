#include "utils.h"

/* [1] Memory helpers */
/** [1.1] fast_memset: Fills a memory block with a specified byte value. */
void fast_memset(void *dest, unsigned char val, size_t n) {
    unsigned char *p = (unsigned char *)dest;
    while (n--) *p++ = val;
}

/** [1.2] fast_memcpy: Copies memory from source to destination. Assumes non-overlapping regions. */
void fast_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
}

/* [2] String helpers */
/** [2.1] tiny_strlen: Returns the length of a null-terminated string. */
size_t tiny_strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return (size_t)(p - s);
}

/** [2.2] strcpy_s: Safely copies a string into a destination buffer. Ensures null termination and avoids overflow. */
void strcpy_s(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t i = 0;
    while (i < dst_size - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

/* [3] Number formatting */
/** [3.1] int_to_dec: Converts an integer to a decimal string. Handles negative values and INT32_MIN edge case. */
int int_to_dec(char *out, int value) {
    char buf[16];
    int pos = 0;
    bool neg = false;
    if (value < 0) {
        neg = true;
        if (value == INT32_MIN) {
            const char *min_str = "2147483648";
            int i = 0;
            out[pos++] = '-';
            while (min_str[i]) out[pos++] = min_str[i++];
            out[pos] = '\0';
            return pos;
        }
        value = -value;
    }
    int i = 0;
    do {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    } while (value > 0);
    if (neg) buf[i++] = '-';
    for (int j = i - 1; j >= 0; j--) out[pos++] = buf[j];
    out[pos] = '\0';
    return pos;
}

/** [3.2] append_uint_zero_pad: Converts an unsigned int to a zero-padded string. */
int append_uint_zero_pad(char *dst, unsigned int value, int width) {
    char tmp[16];
    int len = int_to_dec(tmp, (int)value);
    int pad = width - len;
    int pos = 0;
    while (pad-- > 0) dst[pos++] = '0';
    for (int i = 0; i < len; i++) dst[pos++] = tmp[i];
    dst[pos] = '\0';
    return pos;
}

/** [3.3] format_float_two_decimals: Formats a double with two decimal places. Rounds the value and handles negative numbers. */
int format_float_two_decimals(char *out, double val) {
    if (!out) return 0;
    bool neg = false;
    if (val < 0.0) { neg = true; val = -val; }
    long long v = (long long)(val * 100.0 + 0.5);
    long long whole = v / 100;
    int frac = (int)(v % 100);
    if (frac < 0) frac = -frac;
    int pos = 0;
    if (neg) out[pos++] = '-';
    pos += int_to_dec(&out[pos], (int)whole);
    out[pos++] = '.';
    out[pos++] = '0' + (frac / 10);
    out[pos++] = '0' + (frac % 10);
    out[pos] = '\0';
    return pos;
}

/** [3.4] format_float_one_decimal: Formats a float with one decimal place. */
int format_float_one_decimal(char *out, float val) {
    if (!out) return 0;
    bool neg = false;
    if (val < 0.0f) { neg = true; val = -val; }
    int v = (int)(val * 10.0f + 0.5f);
    int whole = v / 10;
    int frac = v % 10;
    int pos = 0;
    if (neg) out[pos++] = '-';
    pos += int_to_dec(&out[pos], whole);
    out[pos++] = '.';
    out[pos++] = '0' + frac;
    out[pos] = '\0';
    return pos;
}

/* [4] Safe append */
/** [4.1] safe_append_str: Appends a string to a buffer at a given position. Uses strcpy_s internally to ensure safety. */
int safe_append_str(char *dst, int dst_size, int pos, const char *src) {
    if (!dst || dst_size <= 0) return pos;
    if (!src) return pos;
    if (pos < 0) pos = (int)tiny_strlen(dst);
    int avail = dst_size - pos;
    if (avail <= 0) return pos;
    strcpy_s(&dst[pos], (size_t)avail, src);
    return (int)tiny_strlen(dst);
}

/* [5] Byte -> hex helper */
/** [5.1] u8_to_hex_sp: Converts a byte to a hex string with a trailing space. Example: 0x3F → "3F " */
void u8_to_hex_sp(char *out, uint8_t v) {
    static const char hex[] = "0123456789ABCDEF";
    out[0] = hex[(v >> 4) & 0xF];
    out[1] = hex[v & 0xF];
    out[2] = ' ';
}