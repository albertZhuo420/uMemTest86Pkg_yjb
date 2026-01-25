//PassMark MemTest86
//
//Copyright (c) 2013-2016
//	This software is the confidential and proprietary information of PassMark
//	Software Pty. Ltd. ("Confidential Information").  You shall not disclose
//	such Confidential Information and shall use it only in accordance with
//	the terms of the license agreement you entered into with PassMark
//	Software.
//
//Program:
//	MemTest86
//
//Module:
//	Rand.c
//
//Author(s):
//	Keith Mah
//
//Description:
//	Random number generator
//
//

#include <Uefi.h>

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
#include <xmmintrin.h>	//SSE intrinsics
#include <emmintrin.h>	//SSE2 intrinsics
#elif defined(MDE_CPU_AARCH64)
#include <sse2neon.h>
#endif

#include <Library/MPSupportLib.h>

#ifdef __GNUC__
#        define ALIGN(X) __attribute__ ((aligned(X)))
#elif defined(_MSC_VER)
#        define ALIGN(X) __declspec(align(X))
#else
#        error Unknown compiler, unknown alignment attribute!
#endif

// Based on code from http://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor

static unsigned int *SEED; //[MAX_CPUS*16];
static unsigned long long *SEED64; // [MAX_CPUS*8];

ALIGN(16) static unsigned int *SEED128_ua; // [MAX_CPUS*4], unaligned;
ALIGN(16) static __m128i *SEED128; // [MAX_CPUS*4], aligned;

static const unsigned int mult[4] = { 214013, 17405, 214013, 69069 };
ALIGN(16) static unsigned int MULT[8];
static __m128i* pMULT;

static const unsigned int gadd[4] = { 2531011, 10395331, 13737667, 1 };
ALIGN(16) static unsigned int GADD[8];
static __m128i* pGADD;

static const unsigned int mask[4] = { 0xFFFFFFFF, 0, 0xFFFFFFFF, 0 };
ALIGN(16) static unsigned int MASK[8];
static __m128i* pMASK;

static const unsigned int masklo[4] = { 0x00007FFF, 0x00007FFF, 0x00007FFF, 0x00007FFF };
ALIGN(16) static unsigned int MASKLO[8];
static __m128i* pMASKLO;

static int s_numcpus = 0;

void	rand_cleanup()
{
   if (SEED) FreePages(SEED, (UINTN)(EFI_SIZE_TO_PAGES (s_numcpus * 16 * sizeof(unsigned int))));
   SEED = NULL;
   if (SEED64) FreePages(SEED64, (UINTN)(EFI_SIZE_TO_PAGES (s_numcpus * 8 * sizeof(unsigned long long))));
   SEED64 = NULL;

   if (SEED128_ua) FreePages(SEED128_ua, (UINTN)(EFI_SIZE_TO_PAGES((s_numcpus * 4 + 1)* sizeof(__m128i))));
   SEED128_ua = NULL;
   SEED128 = NULL;

   s_numcpus = 0;
}

void	rand_init(int numcpu)
{
   rand_cleanup();   
   SEED = (unsigned int *)AllocatePages((UINTN)(EFI_SIZE_TO_PAGES (numcpu * 16 * sizeof(unsigned int))));
   SEED64 = (unsigned long long *)AllocatePages((UINTN)(EFI_SIZE_TO_PAGES (numcpu * 8 * sizeof(unsigned long long))));
   SEED128_ua = (unsigned int *)AllocatePages((UINTN)(EFI_SIZE_TO_PAGES((numcpu * 4 + 1) * sizeof(__m128i))));
   SEED128 = (__m128i *)(((UINTN)SEED128_ua + 0xF) & ~(0xF));

   // Initialize SSE constants
   pMULT = (__m128i *)(((UINTN)MULT + 0xF) & ~(0xF));
   CopyMem(pMULT, mult, sizeof(mult));
   
   pGADD = (__m128i *)(((UINTN)GADD + 0xF) & ~(0xF));
   CopyMem(pGADD, gadd, sizeof(gadd));

   pMASK = (__m128i *)(((UINTN)MASK + 0xF) & ~(0xF));
   CopyMem(pMASK, mask, sizeof(mask));

   pMASKLO = (__m128i *)(((UINTN)MASKLO + 0xF) & ~(0xF));
   CopyMem(pMASKLO, masklo, sizeof(masklo));

   s_numcpus = numcpu;
}

unsigned int rand( int cpu ) /* returns a random 32-bit integer */
{
#if 0
   static unsigned int a = 18000, b = 30903;
   int me;

   me = cpu*16;

   SEED_X[me] = a*(SEED_X[me]&65535) + (SEED_X[me]>>16);
   SEED_Y[me] = b*(SEED_Y[me]&65535) + (SEED_Y[me]>>16);

   return ((SEED_X[me]<<16) + (SEED_Y[me]&65535));
#endif
   int me;

   me = cpu*16;
   SEED[me] = (214013*SEED[me]+2531011);

   return SEED[me];
}

void	rand_seed(unsigned int seed, int cpu)
{
   int me;

   me = cpu*16;
#if 0
   SEED_X[me] = seed1;   
   SEED_Y[me] = seed2;
#endif
   SEED[me] = seed;
}

unsigned long long rand64( int cpu )           /* returns a random 32-bit integer */
{
   int me;

   me = cpu*8;
   SEED64[me] = (MultU64x64(SEED64[me],6364136223846793005ull) + 1442695040888963407ull);
   return SEED64[me];
}

void	rand64_seed(unsigned int seed, int cpu)
{
   int me;
   me = cpu*8;

   SEED64[me] = seed;
}

void rand_seed_sse( unsigned int seed, int cpu )
{
   int me;

   me = cpu*4;
   
   int* pseed32 = (int*)&SEED128[me];

   pseed32[0] = seed;
   pseed32[1] = seed + 1;
   pseed32[2] = seed;
   pseed32[3] = seed + 1;
}

void rand_sse( int cpu, unsigned int* result )
{
	// workaround for unaligned stack. SSE variables must be 16-byte aligned
	ALIGN(16) unsigned int cur_seed_split[8];

	ALIGN(16) unsigned int multiplier[8];

	ALIGN(16) unsigned int adder[8];

	ALIGN(16) unsigned int mod_mask[8];

	ALIGN(16) unsigned int sra_mask[8];

	__m128i *pcur_seed_split = (__m128i *)(((UINTN)cur_seed_split + 0xF) & ~(0xF));

	__m128i *pmultiplier = (__m128i *)(((UINTN)multiplier + 0xF) & ~(0xF));

	__m128i *padder = (__m128i *)(((UINTN)adder + 0xF) & ~(0xF));

	__m128i *pmod_mask = (__m128i *)(((UINTN)mod_mask + 0xF) & ~(0xF));

	__m128i *psra_mask = (__m128i *)(((UINTN)sra_mask + 0xF) & ~(0xF));


	int me;

	me = cpu*4;

	*padder = _mm_load_si128(pGADD);

	*pmultiplier = _mm_load_si128(pMULT);

	*pmod_mask = _mm_load_si128(pMASK);

	*psra_mask = _mm_load_si128( pMASKLO );

	*pcur_seed_split = _mm_shuffle_epi32(SEED128[me], _MM_SHUFFLE(2, 3, 0, 1));



	SEED128[me] = _mm_mul_epu32(SEED128[me], *pmultiplier);

	*pmultiplier = _mm_shuffle_epi32( *pmultiplier, _MM_SHUFFLE( 2, 3, 0, 1 ) );

	*pcur_seed_split = _mm_mul_epu32( *pcur_seed_split, *pmultiplier );


	SEED128[me] = _mm_and_si128(SEED128[me], *pmod_mask); // cur_seed = cur_seed & mod_mask

	*pcur_seed_split = _mm_and_si128( *pcur_seed_split, *pmod_mask ); // cur_seed_split = cur_seed_split & mod_mask

	*pcur_seed_split = _mm_shuffle_epi32( *pcur_seed_split, _MM_SHUFFLE( 2, 3, 0, 1 ) );

	SEED128[me] = _mm_or_si128(SEED128[me], *pcur_seed_split); // cur_seed = cur_seed | cur_seed_split 

	SEED128[me] = _mm_add_epi32(SEED128[me], *padder); // cur_seed = cur_seed + adder


	_mm_storeu_si128( (__m128i*) result, SEED128[me]); // results = cur_seed

	return;

}