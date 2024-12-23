#include "tonccpy.h"
#include "gba_include.h"

#include <stddef.h>
#define BIT_MASK(len)		( BIT(len)-1 )

//! VRAM-safe cpy.
/*!	This version mimics memcpy in functionality, with 
	the benefit of working for VRAM as well. It is also 
	slightly faster than the original memcpy, but faster 
	implementations can be made.
	\param dst	Destination pointer.
	\param src	Source pointer.
	\param size	Fill-length in bytes.
	\return		\a dst.
	\note	The pointers and size need not be word-aligned.
*/
void *tonccpy(volatile void *dst, const volatile void *src, uint32_t size)
{
	if(size == 0 || dst==NULL || src==NULL)
		return (void*)dst;

	uint32_t count;
	uint16_t *dst16;		// hword destination
	uint8_t  *src8;		// byte source

	// Ideal case: copy by 4x words. Leaves tail for later.
	if( ((uint32_t)src|(uint32_t)dst)%4==0 && size>=4)
	{
		uint32_t *src32= (uint32_t*)src, *dst32= (uint32_t*)dst;

		count= size/4;
		uint32_t tmp= count&3;
		count /= 4;

		// Duff, bitch!
		switch(tmp) {
			do {	*dst32++ = *src32++;
		case 3:		*dst32++ = *src32++;
		case 2:		*dst32++ = *src32++;
		case 1:		*dst32++ = *src32++;
		case 0:		; }	while(count--);
		}

		// Check for tail
		size &= 3;
		if(size == 0)
			return (void*)dst;

		src8= (uint8_t*)src32;
		dst16= (uint16_t*)dst32;
	}
	else		// Unaligned.
	{
		uint32_t dstOfs= (uint32_t)dst&1;
		src8= (uint8_t*)src;
		dst16= (uint16_t*)(dst-dstOfs);

		// Head: 1 byte.
		if(dstOfs != 0)
		{
			*dst16= (*dst16 & 0xFF) | *src8++<<8;
			dst16++;
			if(--size==0)
				return (void*)dst;
		}
	}

	// Unaligned main: copy by 2x byte.
	count= size/2;
	while(count--)
	{
		*dst16++ = src8[0] | src8[1]<<8;
		src8 += 2;
	}

	// Tail: 1 byte.
	if(size&1)
		*dst16= (*dst16 &~ 0xFF) | *src8;

	return (void*)dst;
}

//! VRAM-safe memset, internal routine.
/*!	This version mimics memset in functionality, with 
	the benefit of working for VRAM as well. It is also 
	slightly faster than the original memset.
	\param dst	Destination pointer.
	\param fill	Word to fill with.
	\param size	Fill-length in bytes.
	\return		\a dst.
	\note	The \a dst pointer and \a size need not be 
		word-aligned. In the case of unaligned fills, \a fill 
		will be masked off to match the situation.
*/
void *__toncset(volatile void *dst, uint32_t fill, uint32_t size)
{
	if(dst==NULL || size==0)
		return (void*)dst;

	uint32_t left= (uint32_t)dst&3;
	uint32_t *dst32= (uint32_t*)(dst-left);
	uint32_t count, mask;

	// Unaligned head.
	if(left != 0)
	{
		// Adjust for very small stint.
		if(left+size<4)
		{
			mask= BIT_MASK(size*8)<<(left*8);	
			*dst32= (*dst32 &~ mask) | (fill & mask);
			return (void*)dst;
		}

		mask= BIT_MASK(left*8);
		*dst32= (*dst32 & mask) | (fill&~mask);
		dst32++;
		size -= 4-left;
	}

	// Main stint.
	count= size/4;
	uint32_t tmp= count&3;
	count /= 4;

	switch(tmp) {
		do {	*dst32++ = fill;
	case 3:		*dst32++ = fill;
	case 2:		*dst32++ = fill;
	case 1:		*dst32++ = fill;
	case 0:		; }	while(count--);
	}

	// Tail
	size &= 3;
	if(size)
	{
		mask= BIT_MASK(size*8);
		*dst32= (*dst32 &~ mask) | (fill & mask);
	}

	return (void*)dst;
}

