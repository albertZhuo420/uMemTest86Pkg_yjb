// PassMark MemTest86
//
// Copyright (c) 2013-2016
//	This software is the confidential and proprietary information of PassMark
//	Software Pty. Ltd. ("Confidential Information").  You shall not disclose
//	such Confidential Information and shall use it only in accordance with
//	the terms of the license agreement you entered into with PassMark
//	Software.
//
// Program:
//	MemTest86
//
// Module:
//	Cpuid.c
//
// Author(s):
//	Keith Mah
//
// Description:
//	Function to retrieve CPUID for MemTest86
//
//   Some source code taken from the original MemTest86 source code by Chris Brady, cbrady@sgi.com
// History
//	27 Feb 2013: Initial version

/* cpuid.h - MemTest-86 Version 4.1
 *
 *      contains the data structures required for CPUID
 *      implementation.
 */
#ifndef _CPUID_H_
#define _CPUID_H_

#include <Uefi.h>

#define CPUID_VENDOR_LENGTH 3 /* 3 GPRs hold vendor ID */
#define CPUID_VENDOR_STR_LENGTH (CPUID_VENDOR_LENGTH * sizeof(UINT32) + 1)
#define CPUID_BRAND_LENGTH 12 /* 12 GPRs hold vendor ID */
#define CPUID_BRAND_STR_LENGTH (CPUID_BRAND_LENGTH * sizeof(UINT32) + 1)

#pragma pack(1)
/* Typedef for storing the Cache Information */
typedef union
{
   unsigned char ch[48];
   UINT32 uint[12];
   struct
   {
      UINT32 fill1 : 24; /* Bit 0 */
      UINT32 l1_i_sz : 8;
      UINT32 fill2 : 24;
      UINT32 l1_d_sz : 8;
      UINT32 fill3 : 16;
      UINT32 l2_sz : 16;
      UINT32 fill4 : 18;
      UINT32 l3_sz : 14;
      UINT32 fill5[8];
   } amd;
} cpuid_cache_info_t;

/* Typedef for storing the CPUID Vendor String */
typedef union
{
   /* Note: the extra byte in the char array is for '\0'. */
   char char_array[CPUID_VENDOR_STR_LENGTH];
   UINT32 uint32_array[CPUID_VENDOR_LENGTH];
} cpuid_vendor_string_t;

/* Typedef for storing the CPUID Brand String */
typedef union
{
   /* Note: the extra byte in the char array is for '\0'. */
   char char_array[CPUID_BRAND_STR_LENGTH];
   UINT32 uint32_array[CPUID_BRAND_LENGTH];
} cpuid_brand_string_t;

/* Typedef for storing CPUID Version */
typedef union
{
   UINT32 flat;
   struct
   {
      UINT32 stepping : 4; /* Bit 0 */
      UINT32 model : 4;
      UINT32 family : 4;
      UINT32 processorType : 2;
      UINT32 reserved1514 : 2;
      UINT32 extendedModel : 4;
      UINT32 extendedFamily : 8;
      UINT32 reserved3128 : 4; /* Bit 31 */
   } bits;
} cpuid_version_t;

/* Typedef for storing CPUID Processor Information */
typedef union
{
   UINT32 flat;
   struct
   {
      UINT32 brandIndex : 8; /* Bit 0 */
      UINT32 cflushLineSize : 8;
      UINT32 logicalProcessorCount : 8;
      UINT32 apicID : 8; /* Bit 31 */
   } bits;
} cpuid_proc_info_t;

/* Typedef for storing CPUID Feature flags */
typedef union
{
   UINT32 uint32_array[3];
   struct
   {
      UINT32 fpu : 1; /* EDX feature flags, bit 0 */
      UINT32 vme : 1;
      UINT32 de : 1;
      UINT32 pse : 1;
      UINT32 rdtsc : 1;
      UINT32 msr : 1;
      UINT32 pae : 1;
      UINT32 mce : 1;
      UINT32 cx8 : 1;
      UINT32 apic : 1;
      UINT32 bit10 : 1;
      UINT32 sep : 1;
      UINT32 mtrr : 1;
      UINT32 pge : 1;
      UINT32 mca : 1;
      UINT32 cmov : 1;
      UINT32 pat : 1;
      UINT32 pse36 : 1;
      UINT32 psn : 1;
      UINT32 cflush : 1;
      UINT32 bit20 : 1;
      UINT32 ds : 1;
      UINT32 acpi : 1;
      UINT32 mmx : 1;
      UINT32 fxsr : 1;
      UINT32 sse : 1;
      UINT32 sse2 : 1;
      UINT32 ss : 1;
      UINT32 htt : 1;
      UINT32 tm : 1;
      UINT32 ia64 : 1;
      UINT32 pbe : 1;  /* EDX feature flags, bit 31 */
      UINT32 sse3 : 1; /* ECX feature flags, bit 0 */
      UINT32 pclmulqdq : 1;
      UINT32 dtes64 : 1;
      UINT32 mon : 1;
      UINT32 ds_cpl : 1;
      UINT32 vmx : 1;
      UINT32 smx : 1;
      UINT32 est : 1;
      UINT32 tm2 : 1;
      UINT32 ssse3 : 1;
      UINT32 cnxt_id : 1;
      UINT32 sdbg : 1;
      UINT32 fma : 1;
      UINT32 cx16 : 1;
      UINT32 xtpr : 1;
      UINT32 pdcm : 1;
      UINT32 pcid : 1;
      UINT32 dca : 1;
      UINT32 sse4_1 : 1;
      UINT32 sse4_2 : 1;
      UINT32 x2apic : 1;
      UINT32 movbe : 1;
      UINT32 popcnt : 1;
      UINT32 tsc_deadline : 1;
      UINT32 aes : 1;
      UINT32 xsave : 1;
      UINT32 osxsave : 1;
      UINT32 avx : 1;
      UINT32 f16c : 1;
      UINT32 rdrnd : 1;
      UINT32 hypervisor : 1;
      UINT32 bits0_28 : 29;  /* EDX extended feature flags, bit 0 */
      UINT32 lm : 1;         /* Long Mode */
      UINT32 bits_30_31 : 2; /* EDX extended feature flags, bit 32 */
   } bits;
} cpuid_feature_flags_t;

typedef union
{
   UINTN raw;
   struct
   {
      UINT32 implementer : 8;
      UINT32 variant : 4;
      UINT32 architecture : 4;
      UINT32 partnum : 12;
      UINT32 revision : 4;
   } bits;
} midr_t;

typedef union
{
   UINTN raw;
   struct
   {
#ifdef MDE_CPU_AARCH64
      UINT32 affinity3 : 8;
#endif
      UINT32 res1 : 1;
      UINT32 uniprocessor : 1;
      UINT32 res0 : 5;
      UINT32 mt : 1;
      UINT32 affinity2 : 8;
      UINT32 affinity1 : 8;
      UINT32 affinity0 : 8;
   } bits;
} mpidr_t;

typedef struct
{
   /** Cache size in bytes */
   UINT32 size;
   /** Number of ways of associativity */
   UINT32 associativity;
   /** Number of sets */
   UINT32 sets;
   /** Number of partitions */
   UINT32 partitions;
   /** Line size in bytes */
   UINT32 line_size;
   /**
    * Binary characteristics of the cache (unified cache, inclusive cache, cache with complex indexing).
    *
    * @see CPUINFO_CACHE_UNIFIED, CPUINFO_CACHE_INCLUSIVE, CPUINFO_CACHE_COMPLEX_INDEXING
    */
   UINT32 flags;
   /** Index of the first logical processor that shares this cache */
   UINT32 processor_start;
   /** Number of logical processors that share this cache */
   UINT32 processor_count;
} cache_info_t;

/* An overall structure to cache all of the CPUID information */
struct cpu_ident
{
   UINT32 max_cpuid;
   UINT32 max_xcpuid;
   cpuid_version_t vers;
   cpuid_proc_info_t info;
   cpuid_feature_flags_t fid;
   cpuid_vendor_string_t vend_id;
   cpuid_brand_string_t brand_id;
   cpuid_cache_info_t cache_info;
   midr_t midr;
   mpidr_t mpidr;
   cache_info_t l1i;
   cache_info_t l1d;
   cache_info_t l2;
   cache_info_t l3;
};

struct cpuid4_eax
{
   UINT32 ctype : 5;
   UINT32 level : 3;
   UINT32 is_self_initializing : 1;
   UINT32 is_fully_associative : 1;
   UINT32 reserved : 4;
   UINT32 num_threads_sharing : 12;
   UINT32 num_cores_on_die : 6;
};

struct cpuid4_ebx
{
   UINT32 coherency_line_size : 12;
   UINT32 physical_line_partition : 10;
   UINT32 ways_of_associativity : 10;
};

struct cpuid4_ecx
{
   UINT32 number_of_sets : 32;
};

#pragma pack()

void get_cpuid(struct cpu_ident *cpu_id);

#define CPUID_FAMILY(cpuid) ((cpuid)->vers.bits.extendedFamily + (cpuid)->vers.bits.family)
#define CPUID_MODEL(cpuid) (((cpuid)->vers.bits.extendedModel << 4) + (cpuid)->vers.bits.model)
#define CPUID_STEPPING(cpuid) ((cpuid)->vers.bits.stepping)

#endif
