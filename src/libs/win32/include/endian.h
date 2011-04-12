#ifndef _ENDIAN_H
#define _ENDIAN_H 1

#define __LITTLE_ENDIAN	1234
#define __BIG_ENDIAN	4321
#define __PDP_ENDIAN	3412

#define __BYTE_ORDER __LITTLE_ENDIAN

#undef ___bswap_constant_32
#define ___bswap_constant_32(x) \
	((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
	 (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

#undef bswap_32
#define bswap_32(X) ___bswap_constant_32(X)

#endif
