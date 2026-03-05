#ifndef COMPRESS_H
#define COMPRESS_H
#include <stdint.h>
#include <stddef.h>
#define COMPRESS_BOUND(n) ((n)+((n)/8)+64)
#define COMPRESS_FAIL 0
int compress_page(const uint8_t* src, size_t src_len, uint8_t* dst, size_t dst_len);
int decompress_page(const uint8_t* src, size_t src_len, uint8_t* dst, size_t dst_len);
#endif
