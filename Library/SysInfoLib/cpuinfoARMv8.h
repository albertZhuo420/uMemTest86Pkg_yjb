//PassMark MemTest86
// 
//Copyright (c) 2024
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
//	cpuinfoARMv8.h
//
//Author(s):
//	Keith Mah
//
//Description:
//	Detects and gathers information about Intel CPU(s).
//	Including speed, manufacturer, capabilities, available L2 cache, etc..
//
//  Based on source code from SysInfo

#define ARMV8_PMCR_E            (1 << 0) /* Enable all counters */
#define ARMV8_PMCR_P            (1 << 1) /* Reset all counters */
#define ARMV8_PMCR_C            (1 << 2) /* Cycle counter reset */
#define ARMV8_PMCR_D            (1 << 3) /* Clock divider. When enabled, PMCCNTR counts once every 64 clock cycles */

#define ARMV8_PMUSERENR_EN      (1 << 0) /* EL0 access enable */
#define ARMV8_PMUSERENR_CR      (1 << 2) /* Cycle counter read enable */
#define ARMV8_PMUSERENR_ER      (1 << 3) /* Event counter read enable */

#define ARMV8_PMCNTENSET_EL0_EN (1ULL << 31) /* Performance Monitors Count Enable Set register */

#define ARMV8_PMINTENSET_EL1_EN (1ULL << 31) /* PMCCNTR_EL0 overflow interrupt request enable bit */

#define ARMV8_PMCCFILTR_EL0_P   (1ULL << 31) /* PMCCFILTR_EL0 Privileged filtering bit. Controls counting in EL1 */
#define ARMV8_PMCCFILTR_EL0_U   (1ULL << 30) /* PMCCFILTR_EL0 User filtering bit. Controls counting in EL0 */
#define ARMV8_PMCCFILTR_EL0_NSK (1ULL << 29) /* PMCCFILTR_EL0 When EL3 is implemented: Non-secure EL1 (kernel) modes filtering bit. Controls counting in Non-secure EL1. */
#define ARMV8_PMCCFILTR_EL0_NSU (1ULL << 28) /* PMCCFILTR_EL0 When EL3 is implemented: Non-secure EL0 (Unprivileged) filtering bit. Controls counting in Non-secure EL0. */
#define ARMV8_PMCCFILTR_EL0_NSH (1ULL << 27) /* PMCCFILTR_EL0 When EL2 is implemented: EL2 (Hypervisor) filtering bit. Controls counting in EL2. */
#define ARMV8_PMCCFILTR_EL0_M   (1ULL << 26) /* PMCCFILTR_EL0 When EL3 is implemented: Secure EL3 filtering bit. */
#define ARMV8_PMCCFILTR_EL0_SH  (1ULL << 25) /* PMCCFILTR_EL0 When FEAT_SEL2 is implemented and EL3 is implemented: Secure EL2 filtering. */

VOID	EnableARMv8Cntr();