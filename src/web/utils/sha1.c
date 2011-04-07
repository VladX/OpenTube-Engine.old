#include <common_functions.h>

#ifdef HAVE_OPENSSL_SHA1
 #include <openssl/sha.h>
#else

/*
 *  Description:
 *      This file implements the Secure Hashing Algorithm 1 as
 *      defined in FIPS PUB 180-1 published April 17, 1995.
 *
 *      The SHA-1, produces a 160-bit message digest for a given
 *      data stream.  It should take about 2**n steps to find a
 *      message with the same digest as a given message and
 *      2**(n/2) to find any two messages with the same digest,
 *      when n is the digest size in bits.  Therefore, this
 *      algorithm can serve as a means of providing a
 *      "fingerprint" for a message.
 *
 *  Portability Issues:
 *      SHA-1 is defined in terms of 32-bit "words".  This code
 *      uses <stdint.h> (included via "sha1.h" to define 32 and 8
 *      bit unsigned integer types.  If your C compiler does not
 *      support 32 bit unsigned integers, this code is not
 *      appropriate.
 *
 *  Caveats:
 *      SHA-1 is designed to work with messages less than 2^64 bits
 *      long.  Although SHA-1 allows a message digest to be generated
 *      for messages of any number of bits less than 2^64, this
 *      implementation only works with messages with a length that is
 *      a multiple of the size of an 8-bit character.
 *
 */

#include <stdint.h>

#define SHA1HashSize 20

typedef struct SHA1Context
{
	uint32_t Intermediate_Hash[SHA1HashSize/4];
	uint32_t Length_Low;
	uint32_t Length_High;
	int_least16_t Message_Block_Index;
	uint8_t Message_Block[64];
	int Computed;
	int Corrupted;
} SHA1Context;


#define SHA1CircularShift(bits,word) (((word) << (bits)) | ((word) >> (32-(bits))))


static inline void SHA1ProcessMessageBlock (SHA1Context * context)
{
	static const uint32_t K[] = {0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6};
	int t;
	uint32_t temp;
	uint32_t W[80];
	uint32_t A, B, C, D, E;
	
	for (t = 0; t < 16; t++)
	{
		W[t] = context->Message_Block[t * 4] << 24;
		W[t] |= context->Message_Block[t * 4 + 1] << 16;
		W[t] |= context->Message_Block[t * 4 + 2] << 8;
		W[t] |= context->Message_Block[t * 4 + 3];
	}
	
	for (t = 16; t < 80; t++)
	{
	   W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
	}
	
	A = context->Intermediate_Hash[0];
	B = context->Intermediate_Hash[1];
	C = context->Intermediate_Hash[2];
	D = context->Intermediate_Hash[3];
	E = context->Intermediate_Hash[4];
	
	for (t = 0; t < 20; t++)
	{
		temp =  SHA1CircularShift(5,A) +
				((B & C) | ((~B) & D)) + E + W[t] + K[0];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		
		B = A;
		A = temp;
	}
	
	for(t = 20; t < 40; t++)
	{
		temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}
	
	for(t = 40; t < 60; t++)
	{
		temp = SHA1CircularShift(5,A) +
			   ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}
	
	for(t = 60; t < 80; t++)
	{
		temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}
	
	context->Intermediate_Hash[0] += A;
	context->Intermediate_Hash[1] += B;
	context->Intermediate_Hash[2] += C;
	context->Intermediate_Hash[3] += D;
	context->Intermediate_Hash[4] += E;
	
	context->Message_Block_Index = 0;
}

static inline void SHA1PadMessage (SHA1Context * context)
{
	if (context->Message_Block_Index > 55)
	{
		context->Message_Block[context->Message_Block_Index++] = 0x80;
		while (context->Message_Block_Index < 64)
		{
			context->Message_Block[context->Message_Block_Index++] = 0;
		}
		
		SHA1ProcessMessageBlock(context);
		
		while (context->Message_Block_Index < 56)
		{
			context->Message_Block[context->Message_Block_Index++] = 0;
		}
	}
	else
	{
		context->Message_Block[context->Message_Block_Index++] = 0x80;
		while (context->Message_Block_Index < 56)
		{
			context->Message_Block[context->Message_Block_Index++] = 0;
		}
	}
	
	context->Message_Block[56] = context->Length_High >> 24;
	context->Message_Block[57] = context->Length_High >> 16;
	context->Message_Block[58] = context->Length_High >> 8;
	context->Message_Block[59] = context->Length_High;
	context->Message_Block[60] = context->Length_Low >> 24;
	context->Message_Block[61] = context->Length_Low >> 16;
	context->Message_Block[62] = context->Length_Low >> 8;
	context->Message_Block[63] = context->Length_Low;
	
	SHA1ProcessMessageBlock(context);
}

static inline void SHA1Reset (SHA1Context * context)
{
	context->Length_Low = 0;
	context->Length_High = 0;
	context->Message_Block_Index = 0;
	
	context->Intermediate_Hash[0] = 0x67452301;
	context->Intermediate_Hash[1] = 0xEFCDAB89;
	context->Intermediate_Hash[2] = 0x98BADCFE;
	context->Intermediate_Hash[3] = 0x10325476;
	context->Intermediate_Hash[4] = 0xC3D2E1F0;
	
	context->Computed = 0;
	context->Corrupted = 0;
}

static inline void SHA1Result (SHA1Context * context, uint8_t Message_Digest[SHA1HashSize])
{
	int i;
	
	if (!context->Computed)
	{
		SHA1PadMessage(context);
		for(i=0; i<64; ++i)
		{
			context->Message_Block[i] = 0;
		}
		context->Length_Low = 0;
		context->Length_High = 0;
		context->Computed = 1;

	}

	for(i = 0; i < SHA1HashSize; ++i)
	{
		Message_Digest[i] = context->Intermediate_Hash[i >> 2] >> 8 * ( 3 - ( i & 0x03 ) );
	}
}

static inline void SHA1Input (SHA1Context * context, const uint8_t * message_array, unsigned int length)
{
	while (length-- && !context->Corrupted)
	{
		context->Message_Block[context->Message_Block_Index++] = (* message_array & 0xFF);
		
		context->Length_Low += 8;
		if (context->Length_Low == 0)
		{
			context->Length_High++;
			if (context->Length_High == 0)
			{
				context->Corrupted = 1;
			}
		}
		
		if (context->Message_Block_Index == 64)
		{
			SHA1ProcessMessageBlock(context);
		}
		
		message_array++;
	}
}
#endif

void sha1 (const void * data, uint length, uchar * result)
{
	#ifdef HAVE_OPENSSL_SHA1
	SHA_CTX ctx;
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, data, length);
	SHA1_Final(result, &ctx);
	#else
	SHA1Context ctx;
	SHA1Reset(&ctx);
	SHA1Input(&ctx, data, length);
	SHA1Result(&ctx, result);
	#endif
}
