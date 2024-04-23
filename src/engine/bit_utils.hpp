#pragma once
#include <cassert>
#include <cstdint>

template< typename T >
inline bool Even(T num){
    return ((num & 1) == 0);
}

template< typename T >
inline bool Odd(T num){
    return ((num & 1) == 1);
}

template< typename T1, typename T2 >
inline bool BitEnabled(T1 word, T2 bit){
    return ((word & bit) != 0);
}

template< typename T1, typename T2 >
inline bool BitDisabled(T1 word, T2 bit){
    return ((word & bit) == 0);
}

template< typename T1, typename T2, typename T3 >
inline bool BitCmpMask(T1 word, T2 bit, T3 mask){
    return ((word & bit) == mask);
}

template< typename T1, typename T2 >
inline void SetBits(T1& word, T2 bits){
    word |= bits;
}

template< typename T1, typename T2 >
inline void ClrBits(T1& word, T2 bits){
    word &= ~bits;
}

template< typename baseT >
inline baseT Reverse(baseT x){
	assert(!"Type is unsupported.");
    return x;
}

template< >
inline uint64_t Reverse(uint64_t x){
	x = (x & 0x5555555555555555L) << 1 | (x >> 1) & 0x5555555555555555L;
	x = (x & 0x3333333333333333L) << 2 | (x >> 2) & 0x3333333333333333L;
	x = (x & 0x0f0f0f0f0f0f0f0fL) << 4 | (x >> 4) & 0x0f0f0f0f0f0f0f0fL;
	x = (x & 0x00ff00ff00ff00ffL) << 8 | (x >> 8) & 0x00ff00ff00ff00ffL;
	return ((x << 48) | ((x & 0xffff0000L) << 16) | ((x >> 16) & 0xffff0000L) | (x >> 48));
}

template< >
inline uint32_t Reverse(uint32_t x){
    x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
    x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
    x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
    x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
    return ((x >> 16) | (x << 16));
}
