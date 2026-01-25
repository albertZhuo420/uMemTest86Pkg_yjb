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
//	controller.h
//
// Author(s):
//	Keith Mah
//
// Description:
//	Functions for detecting ECC support and polling ECC errors
//

#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

/* controller ECC capabilities and mode */
#define __ECC_UNEXPECTED 1 /* Unknown ECC capability present */
#define __ECC_DETECT 2     /* Can detect ECC errors */
#define __ECC_CORRECT 4    /* Can correct some ECC errors */
#define __ECC_SCRUB 8      /* Can scrub corrected ECC errors */
#define __ECC_CHIPKILL 16  /* Can corrected multi-errors */
#define __ECC_INBAND 32
#define __ECC_UNKNOWN (1UL << 31)

#define ECC_NONE 0                    /* Doesnt support ECC (or is BIOS disabled) */
#define ECC_UNKNOWN __ECC_UNKNOWN     /* Unknown error correcting ability/status */
#define ECC_RESERVED __ECC_UNEXPECTED /* Reserved ECC type */
#define ECC_DETECT __ECC_DETECT
#define ECC_CORRECT (__ECC_DETECT | __ECC_CORRECT)
#define ECC_CHIPKILL (__ECC_DETECT | __ECC_CORRECT | __ECC_CHIPKILL)
#define ECC_SCRUB (__ECC_DETECT | __ECC_CORRECT | __ECC_SCRUB)
#define ECC_ANY (__ECC_DETECT | __ECC_CORRECT | __ECC_CHIPKILL | __ECC_SCRUB | __ECC_INBAND)

int find_mem_controller(void);

void free_mem_controller(void);

char *get_mem_ctrl_name();

unsigned int get_mem_ctrl_cap();

unsigned int get_mem_ctrl_mode();

unsigned int get_mem_ctrl_channel_mode();

int get_mem_ctrl_num_channels();

int get_mem_ctrl_num_dimms_per_channel();

int get_mem_ctrl_num_slots();

unsigned char get_mem_ctrl_num_ranks(unsigned int dimm);

unsigned char get_mem_ctrl_chip_width(unsigned int dimm);

int get_mem_ctrl_decode_supported();

int get_mem_ctrl_timings(unsigned int *memclk, unsigned int *ctrlclk, unsigned char *cas, unsigned char *rcd, unsigned char *rp, unsigned char *ras);

/*
 * Address decoding
 */
int decode_addr(unsigned long long addr, unsigned long long ebits, unsigned long esize, unsigned char form_factor, int *pskt, int *pch, int *pdimm, int *prank, int *pchip, int *pbank, int *prow, int *pcol, unsigned long long *subaddr);

/*
 * ECC polling
 */
void set_ecc_polling(int index, int val);

int poll_temp(void);

void poll_errors(void);

/*
 * Inject ECC errors
 */
void inject_nhm(int index, int uncorrectable);

void inject_amd64(int index, int uncorrectable);

BOOLEAN inject_ryzen(int index, int uncorrectable, int enable);

BOOLEAN inject_ryzen_zen4(int index, int uncorrectable, int enable);

BOOLEAN inject_ryzen_zen5(int index, int uncorrectable, int enable);

void inject_e3haswell(int index, int uncorrectable, int enable);

void inject_e3sb(int index, int uncorrectable, int enable);

void inject_broadwell_h(int index, int uncorrectable, int enable);

void inject_skylake(int index, int uncorrectable, int enable);

void inject_c2000atom(int index, int uncorrectable);

void inject_apollolake(int index, int uncorrectable, int enable);

BOOLEAN inject_kabylake(int index, int uncorrectable, int enable);

BOOLEAN inject_coffeelake(int index, int uncorrectable, int enable);

BOOLEAN inject_cometlake(int index, int uncorrectable, int enable);

BOOLEAN inject_icelake(int index, int uncorrectable, int enable);

BOOLEAN inject_rocketlake(int index, int uncorrectable, int enable);

BOOLEAN inject_tigerlake(int index, int uncorrectable, int enable);

BOOLEAN inject_tigerlake_h(int index, int uncorrectable, int enable);

BOOLEAN inject_alderlake(int index, int uncorrectable, int enable);

BOOLEAN inject_meteorlake(int index, int uncorrectable, int enable);

void inject_elkhartlake(int index, int uncorrectable, int enable);

void inject_e5sb(int index, int uncorrectable, int enable);

BOOLEAN inject_broadwell(int index, int uncorrectable, int enable);

#endif
