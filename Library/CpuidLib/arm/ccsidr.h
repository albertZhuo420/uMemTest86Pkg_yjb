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
//	ccsidr.h
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
#pragma once

void get_ccsidr_cache_info(struct cpu_ident *cpu_id);