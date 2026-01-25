// PassMark MemTest86
//
// Copyright (c) 2016
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
//	cpuinfoMSRIntel.c
//
// Author(s):
//	Keith Mah, Ian Robinson
//
// Description:
//	Detects and gathers information about Intel CPU(s).
//	Including speed, manufacturer, capabilities, available L2 cache, etc..
//
//   Based on source code from SysInfo

#include "uMemTest86.h"
#include "cpuinfoARMv8.h"

#include <Library/MemTestSupportLib.h>
#include <Library/MPSupportLib.h>

#include <Library/ArmLib.h>
#include <Library/ArmGenericTimerCounterLib.h>

VOID EnableARMv8Cntr()
{
    UINTN CntFreq;
    UINT64 value;

    UINTN ProcNum = MPSupportWhoAmI();

    DBGLOG(ProcNum, "ARMv8Cntr - Current EL: %x", ArmReadCurrentEL());

    CntFreq = ArmGenericTimerGetTimerFreq();

    DBGLOG(ProcNum, "ARMv8Cntr - cntfreq: %ld", CntFreq);

    value = ArmGenericTimerGetTimerVal();

    DBGLOG(ProcNum, "ARMv8Cntr - cntp_tval_el0: %ld", value);

    if (CntFreq == 0 || value == 0)
    {
        // Everything is ready, unmask and enable timer interrupts
        UINTN TimerCtrl = ArmGenericTimerGetTimerCtrlReg();

        DBGLOG(ProcNum, "ARMv8Cntr - CNTP_CTL_EL0: %016lx (enabled: %s, interrupt: %s)", TimerCtrl, (TimerCtrl & ARM_ARCH_TIMER_ENABLE) ? L"yes" : L"no", (TimerCtrl & ARM_ARCH_TIMER_IMASK) ? L"disabled" : L"enabled");

#ifndef QEMU
        /* Mask interrupt */
        TimerCtrl |= ARM_ARCH_TIMER_IMASK;
        /* Enable physical timer */
        TimerCtrl |= ARM_ARCH_TIMER_ENABLE;

        DBGLOG(ProcNum, "ARMv8Cntr - set CNTP_CTL_EL0: %016lx", TimerCtrl);
        ArmWriteCntpCtl(TimerCtrl);
#endif
    }
    /*Enable user-mode access to counters. */

    // Read PMUSERENR_EL0
    __asm__ __volatile__("MRS %0, PMUSERENR_EL0" : "=r"(value));

    DBGLOG(ProcNum, "ARMv8Cntr - pmuserenr: %016lx", value);

    value |= ARMV8_PMUSERENR_EN;
    value |= ARMV8_PMUSERENR_ER;
    value |= ARMV8_PMUSERENR_CR;

    DBGLOG(ProcNum, "ARMv8Cntr - set pmuserenr to %016lx", value);

    // Write PMUSERENR_EL0
    __asm__ __volatile__("MSR PMUSERENR_EL0, %0" : : "r"(ARMV8_PMUSERENR_EN | ARMV8_PMUSERENR_ER | ARMV8_PMUSERENR_CR));

    // Re-Read PMUSERENR_EL0
    __asm__ __volatile__("MRS %0, PMUSERENR_EL0" : "=r"(value));

    DBGLOG(ProcNum, "ARMv8Cntr - new pmuserenr: %016lx", value);

    /* Disable cycle counter overflow interrupt */

    // Read PMINTENSET_EL1, Performance Monitors Interrupt Enable Set register
    __asm__ __volatile__("MRS %0, PMINTENSET_EL1" : "=r"(value));

    DBGLOG(ProcNum, "ARMv8Cntr - pmintenset: %016lx", value);

    value &= ~ARMV8_PMINTENSET_EL1_EN;

    DBGLOG(ProcNum, "ARMv8Cntr - set pmintenset to %016lx", value);

    __asm__ __volatile__("MSR PMINTENSET_EL1, %0" : : "r"(value));

    // Re-Read PMINTENSET_EL1
    __asm__ __volatile__("MRS %0, PMINTENSET_EL1" : "=r"(value));

    DBGLOG(ProcNum, "ARMv8Cntr - new pmintenset: %016lx", value);

    /* Enable cycle counter register */

    // Read PMCNTENSET_EL0, Performance Monitors Count Enable Set register
    __asm__ __volatile__("MRS %0, PMCNTENSET_EL0" : "=r"(value));

    DBGLOG(ProcNum, "ARMv8Cntr - pmcntenset: %016lx", value);

    value |= ARMV8_PMCNTENSET_EL0_EN;

    DBGLOG(ProcNum, "ARMv8Cntr - set pmcntenset to %016lx", value);

    // Write PMCNTENSET_EL0
    __asm__ __volatile__("MSR PMCNTENSET_EL0, %0" : : "r"(value));

    // Re-read PMCNTENSET_EL0
    __asm__ __volatile__("MRS %0, PMCNTENSET_EL0" : "=r"(value));

    DBGLOG(ProcNum, "ARMv8Cntr - new pmcntenset: %016lx (cycle counter is %s)", value, (value & ARMV8_PMCNTENSET_EL0_EN) ? L"enabled" : L"disabled");

    /* Enable Performance Counter */

    // Read PMCR_EL0, Performance Monitors Control Register, EL0
    __asm__ __volatile__("MRS %0, PMCR_EL0" : "=r"(value));

    DBGLOG(ProcNum, "ARMv8Cntr - pmcr: %016lx", value);

    value |= ARMV8_PMCR_E;  /* Enable */
    value |= ARMV8_PMCR_C;  /* Cycle counter reset */
    value |= ARMV8_PMCR_P;  /* Reset all counters */
    value &= ~ARMV8_PMCR_D; /* Count every clock cycle */

    DBGLOG(ProcNum, "ARMv8Cntr - set pmcr to %016lx", value);

    // Set PMCR_EL0
    __asm__ __volatile__("MSR PMCR_EL0, %0" : : "r"(value));

    // Re-read PMCR_EL0
    __asm__ __volatile__("MRS %0, PMCR_EL0" : "=r"(value));

    DBGLOG(ProcNum, "ARMv8Cntr - new pmcr: %016lx", value);

    /* Disable Performance Monitors Cycle Count Filters */
#ifndef QEMU
    // Read PMCCFILTR_EL0, , Performance Monitors Cycle Count Filter Register
    __asm__ __volatile__("MRS %0, PMCCFILTR_EL0" : "=r"(value));

    DBGLOG(ProcNum, "ARMv8Cntr - pmccfiltr: %016lx", value);

    value &= ~ARMV8_PMCCFILTR_EL0_P;
    value &= ~ARMV8_PMCCFILTR_EL0_U;
    value &= ~ARMV8_PMCCFILTR_EL0_NSK;
    value &= ~ARMV8_PMCCFILTR_EL0_NSU;
    value |= ARMV8_PMCCFILTR_EL0_NSH;
    value &= ~ARMV8_PMCCFILTR_EL0_M;
    value &= ~ARMV8_PMCCFILTR_EL0_SH;

    DBGLOG(ProcNum, "ARMv8Cntr - set pmccfiltr to %016lx", value);

    __asm__ __volatile__("MSR PMCCFILTR_EL0, %0" : : "r"(value));

    // Re-Read PMCCFILTR_EL0
    __asm__ __volatile__("MRS %0, PMCCFILTR_EL0" : "=r"(value));

    DBGLOG(ProcNum, "ARMv8Cntr - new pmccfiltr: %016lx", value);
#endif
}