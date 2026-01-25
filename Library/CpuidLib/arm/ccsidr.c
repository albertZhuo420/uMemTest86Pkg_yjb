// PassMark MemTest86
//
// Copyright (c) 2025
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
//	ccsidr.c
//
// Author(s):
//	Keith Mah
//
// Description:
//	Function to retrieve cache information from CCSIDR register for ARM CPUS
//
//   Reference: ArmPkg/Universal/Smbios/ProcessorSubClassDxe/SmbiosProcessorArmCommon.c
//
//   Functions for processor information common to ARM and AARCH64.
//
//   Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
//   Copyright (c) 2021 - 2022, Ampere Computing LLC. All rights reserved.<BR>
//
//   SPDX-License-Identifier: BSD-2-Clause-Patent
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <IndustryStandard/ArmCache.h>

#include <Library/Cpuid.h>

#define CCSIDR_WRITE_THROUGH (1 << 31)
#define CCSIDR_WRITE_BACK (1 << 30)
#define CCSIDR_READ_ALLOCATE (1 << 29)
#define CCSIDR_WRITE_ALLOCATE (1 << 28)

#define BitExtract(dwReg, wIndexHigh, dwIndexLow) ((dwReg) >> (dwIndexLow) & ((1 << ((wIndexHigh) - (dwIndexLow) + 1)) - 1))
#define BitExtractULL(dwReg, wIndexHigh, dwIndexLow) (RShiftU64((UINT64)(dwReg), dwIndexLow) & (LShiftU64(1, ((wIndexHigh) - (dwIndexLow) + 1)) - 1))

typedef void *MT_HANDLE;
#define DEBUG_FILE_HANDLE ((MT_HANDLE) - 1)

extern EFI_STATUS
    EFIAPI
    AsciiFPrint(
        MT_HANDLE FileHandle,
        IN CONST CHAR8 *FormatString,
        ...);

/** Returns the maximum cache level implemented by the current CPU.

    @return The maximum cache level implemented.
**/
UINT8
GetMaxCacheLevel(
    VOID)
{
    CLIDR_DATA Clidr;
    UINT8 CacheLevel;
    UINT8 MaxCacheLevel;

    MaxCacheLevel = 0;

    // Read the CLIDR register to find out what caches are present.
    Clidr.Data = ReadCLIDR();

    // Get the cache type for the L1 cache. If it's 0, there are no caches.
    if (CLIDR_GET_CACHE_TYPE(Clidr.Data, 1) == ClidrCacheTypeNone)
    {
        return 0;
    }

    for (CacheLevel = 1; CacheLevel <= MAX_ARM_CACHE_LEVEL; CacheLevel++)
    {
        if (CLIDR_GET_CACHE_TYPE(Clidr.Data, CacheLevel) == ClidrCacheTypeNone)
        {
            MaxCacheLevel = CacheLevel;
            break;
        }
    }

    return MaxCacheLevel;
}

/** Returns whether or not the specified cache level has separate I/D caches.

    @param CacheLevel The cache level (L1, L2 etc.).

    @return TRUE if the cache level has separate I/D caches, FALSE otherwise.
**/
BOOLEAN
HasSeparateCaches(
    UINT8 CacheLevel)
{
    CLIDR_CACHE_TYPE CacheType;
    CLIDR_DATA Clidr;
    BOOLEAN SeparateCaches;

    SeparateCaches = FALSE;

    Clidr.Data = ReadCLIDR();

    CacheType = CLIDR_GET_CACHE_TYPE(Clidr.Data, CacheLevel - 1);

    if (CacheType == ClidrCacheTypeSeparate)
    {
        SeparateCaches = TRUE;
    }

    return SeparateCaches;
}

void get_ccsidr_cache_info(struct cpu_ident *cpu_id)
{
    EFI_STATUS Status;
    UINT8 CacheLevel;
    UINT8 MaxCacheLevel;
    BOOLEAN DataCacheType;
    BOOLEAN SeparateCaches;

    Status = EFI_SUCCESS;

    MaxCacheLevel = 0;

    // See if there's an L1 cache present.
    MaxCacheLevel = GetMaxCacheLevel();

    AsciiFPrint(DEBUG_FILE_HANDLE, "get_ccsidr_cache_info - max cache level: %d", MaxCacheLevel);
    if (MaxCacheLevel < 1)
    {
        return;
    }

    for (CacheLevel = 1; CacheLevel <= MaxCacheLevel; CacheLevel++)
    {
        SeparateCaches = HasSeparateCaches(CacheLevel);

        // At each level of cache, we can have a single type (unified, instruction or data),
        // or two types - separate data and instruction caches. If we have separate
        // instruction and data caches, then on the first iteration (CacheSubLevel = 0)
        // process the instruction cache.
        for (DataCacheType = 0; DataCacheType <= 1; DataCacheType++)
        {
            cache_info_t *ci = NULL;

            // If there's no separate data/instruction cache, skip the second iteration
            if ((DataCacheType == 1) && !SeparateCaches)
            {
                continue;
            }

            if (CacheLevel == 1)
                ci = DataCacheType ? &cpu_id->l1d : &cpu_id->l1i;
            else if (CacheLevel == 2)
                ci = &cpu_id->l2;
            else if (CacheLevel == 3)
                ci = &cpu_id->l3;
            else
                break;

            CCSIDR_DATA Ccsidr;
            CSSELR_DATA Csselr;

            Csselr.Data = 0;
            Csselr.Bits.Level = CacheLevel - 1;
            Csselr.Bits.InD = (!DataCacheType && SeparateCaches);

            Ccsidr.Data = ReadCCSIDR(Csselr.Data);

            if (ArmHasCcidx())
            {
                ci->associativity = (Ccsidr.BitsCcidxAA64.Associativity + 1);
                ci->line_size = (1 << (Ccsidr.BitsCcidxAA64.LineSize + 4));
                ci->sets = (Ccsidr.BitsCcidxAA64.NumSets + 1);
            }
            else
            {
                ci->associativity = (Ccsidr.BitsNonCcidx.Associativity + 1);
                ci->line_size = (1 << (Ccsidr.BitsNonCcidx.LineSize + 4));
                ci->sets = (Ccsidr.BitsNonCcidx.NumSets + 1);
            }
            ci->size = ci->associativity * ci->line_size * ci->sets;
            ci->flags = BitExtractULL(Ccsidr.Data, 31, 28);

            AsciiFPrint(DEBUG_FILE_HANDLE, "get_ccsidr_cache_info - L%d%c size(%d) assoc(%d) line(%d) sets(%d) attr(%04x) [ccsidr: %016lx]",
                        CacheLevel,
                        CacheLevel != 1 ? '\0' : (DataCacheType ? 'd' : 'i'),
                        ci->size,
                        ci->associativity,
                        ci->line_size,
                        ci->sets,
                        ci->flags,
                        Ccsidr.Data);
        }
    }
}