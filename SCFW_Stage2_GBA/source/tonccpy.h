#ifndef TONCCPY_H
#define TONCCPY_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

//! Quadruple a byte to form a word: 0x12 -> 0x12121212.
static inline uint32_t quad8(uint8_t x) {
	return x*0x01010101;
}

void *tonccpy(volatile void* dst, const volatile void* src, uint32_t size);
void *__toncset(volatile void *dst, uint32_t fill, uint32_t size);

static inline void* toncset(volatile void* dst, uint8_t fill, uint32_t count) {
	return __toncset(dst, quad8(fill), count);
}

#ifdef __cplusplus
}
#endif
#endif