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
//	Rand.h
//
//Author(s):
//	Keith Mah
//
//Description:
//	Random number generator
//

#ifndef _RAND_H_
#define _RAND_H_

// rand_cleanup
//
// Clean up rand library
extern void	rand_cleanup();

// rand_init
//
// Initialize rand library
extern void	rand_init(int numcpu);

// rand
//
// Generate random integer value
extern unsigned int rand( int cpu );

// rand_seed
//
// Set random integer seed
extern void	rand_seed(unsigned int seed, int cpu);

// rand64
//
// Generate random 64-bit integer value
extern unsigned long long rand64( int cpu );

// rand_seed
//
// Set random 64-bit integer seed
extern void	rand64_seed(unsigned int seed, int cpu);

// rand_seed_sse
//
// Set random 128-bit integer seed
extern void rand_seed_sse( unsigned int seed, int cpu );

// rand_sse
//
// Generate random 128-bit integer value
extern void rand_sse( int cpu, unsigned int* result );

/* return a random float >= 0 and < 1 */
#define rand_float          ((double)rand() / 4294967296.0)

#endif