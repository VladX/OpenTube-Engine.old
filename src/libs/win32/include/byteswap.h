#ifndef _BYTESWAP_H
#define _BYTESWAP_H 1

#define __bswap_16(x)
#define __bswap_32(x)

#define bswap_16(x) __bswap_16 (x)
#define bswap_32(x) __bswap_32 (x)

#endif
