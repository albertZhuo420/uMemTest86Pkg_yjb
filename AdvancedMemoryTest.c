//PassMark MemTest86
//
//Copyright (c) 2014-2016
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
//	AdvancedMemoryTest.c
//
//Author(s):
//	Keith Mah
//
//Description:
//  This class is responsible for initialising and running a test which measures 
//  various memory access speeds.
//
//  Taken from PerformanceTest
//
//History
//	15 Jul 2014: Initial version

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemTestSupportLib.h>

#include "AdvancedMemoryTest.h"

extern UINTN				clks_msec;
extern UINT64				memsize;

#define MAX_NUM_SAMPLES				32

#if defined(MDE_CPU_X64) // For 64-bit build, code is in separate asm file
typedef struct mem_step_test_args {
    UINTN memoryPtr;
    UINTN numSteps;
    UINTN stepSize;
    UINTN maxMemoryPtr;
} ADV_MEM_STEP_TEST_ARGS;
extern void AdvMemTestStepReadAsm(ADV_MEM_STEP_TEST_ARGS *mta);
extern void AdvMemTestStepWriteAsm(ADV_MEM_STEP_TEST_ARGS *mta);
typedef struct mem_block_step_test_args {
      UINTN pBlock;
      UINTN dwLoops;
      UINTN DataItemsPerBlock;
} ADV_MEM_BLOCK_STEP_TEST_ARGS;
extern void AdvMemTestBlockStepReadBytesAsm(ADV_MEM_BLOCK_STEP_TEST_ARGS *mta);
extern void AdvMemTestBlockStepReadWordsAsm(ADV_MEM_BLOCK_STEP_TEST_ARGS *mta);
extern void AdvMemTestBlockStepReadDWordsAsm(ADV_MEM_BLOCK_STEP_TEST_ARGS *mta);
extern void AdvMemTestBlockStepReadQWordsAsm(ADV_MEM_BLOCK_STEP_TEST_ARGS *mta);
extern void AdvMemTestBlockStepWriteBytesAsm(ADV_MEM_BLOCK_STEP_TEST_ARGS *mta);
extern void AdvMemTestBlockStepWriteWordsAsm(ADV_MEM_BLOCK_STEP_TEST_ARGS *mta);
extern void AdvMemTestBlockStepWriteDWordsAsm(ADV_MEM_BLOCK_STEP_TEST_ARGS *mta);
extern void AdvMemTestBlockStepWriteQWordsAsm(ADV_MEM_BLOCK_STEP_TEST_ARGS *mta);
#endif

// Variables
STATIC UINT64			mStartTime;
STATIC UINT64			mStopTime;
STATIC int				mDataSize;
STATIC int				mTestState;
STATIC int				mTestMode;
STATIC int				mNumSteps;
STATIC UINT64			mAveSpeed;
STATIC UINT64			mTotalKBPerSec;
STATIC UINT8		  *	mMemoryArray;
STATIC BOOLEAN			mResultsAvailable;
STATIC BOOLEAN			mRead;
STATIC UINTN			mBlockSize;
STATIC UINTN			mMaxBlockSize;
STATIC UINTN			mArraySize;
STATIC UINTN			mStepSize;
STATIC UINTN			mMaxStepSize;
STATIC UINT32			mNumSamples;
STATIC UINT32			mCurrentIndex;

MEMORY_STATS			mMemoryStats[MAX_NUM_SAMPLES];

static __inline void startclock()
{
    // Set the start time
    mStartTime = MtSupportReadTSC();
}

static __inline void stopclock()
{
    mStopTime = MtSupportReadTSC();
}

static __inline UINT64 elapsed_time_us()
{
    UINT64 t;
    UINT64 clks_usec = clks_msec / 1000;
    /* We can't do the elapsed time unless the rdtsc instruction
     * is supported
     */ 

    if (mStopTime >= mStartTime)
        t= mStopTime - mStartTime;
    else // overflow?
        t= ((UINT64)-1) - (mStartTime - mStopTime + 1);

    t = DivU64x64Remainder(t, clks_usec, NULL);

    return t;
}

void Init()
{
    mResultsAvailable			= FALSE;
    mAveSpeed					= 0;
    mTotalKBPerSec			= 0;
    mNumSteps					= 0;
    mCurrentIndex			= 0;
    mNumSamples				= 0;
    mArraySize					= 0;
    mStepSize					= 1;

    mTestState				= NOT_STARTED;

    SetMem(mMemoryStats, sizeof(mMemoryStats), 0);
}

void CleanupTest()
{
    stopclock();
    mNumSamples = mCurrentIndex;

    // Cleanup
    if(mMemoryArray)
    {
        FreePool(mMemoryArray);
        mMemoryArray = NULL;
    }
}


int	NewStepTest()
{
    UINTN			StepSize	= 0;

    Init();

    // Take one quarter of available physical mem for a large array size

    //Cap this to 0.5GB, larger increases the likelyhood of the Alloc fails due to memory fragmentation
    if (DivU64x32(memsize, 4) > SIZE_512MB)
        mArraySize = SIZE_512MB;
    else
        mArraySize = (UINTN)(DivU64x32(memsize,4));


    mMaxStepSize	= mArraySize / 65536; //2^16

    // Allocate the memory
    mMemoryArray = (UINT8 *)AllocatePool (mArraySize );

    mNumSteps = 0;
    if(!mMemoryArray)
    {
    }
    else
    {
        // Work out how many steps we'll be taking
        for(StepSize = 1;StepSize <= mMaxStepSize;StepSize *= 2)
        {
            mNumSteps++;
        }

        mMaxStepSize = StepSize;
    }

    return mNumSteps;
}

int	NewBlockTest()
{
    UINT64 TotalPhys = 0;
    Init();
    
    TotalPhys = memsize;

    //Limit test to a gigabyte. 
    //Note that when TotalPhys >= 2GB the below loop will was getting stuck
    //when m_BlockSize wraps around the 32-bit mark and gets stuck at 0
    if (TotalPhys > SIZE_1GB)
        TotalPhys = SIZE_1GB;

    // How many steps will there be?
    //for(m_dwBlockSize = MIN_BLOCK_SIZE;m_dwBlockSize < m_MemoryStatus.dwTotalPhys;m_dwBlockSize *= 2)
    for(mBlockSize = MIN_BLOCK_SIZE;mBlockSize < TotalPhys;mBlockSize *= 2)
    {
        mMaxBlockSize = mBlockSize;
        mNumSteps++;
    }
    
    mBlockSize		= MIN_BLOCK_SIZE;
    return mNumSteps;
}

DISABLE_OPTIMISATIONS()
BOOLEAN Step()
{
    BOOLEAN		bRet				= TRUE;	
    UINT64		flKBPerSec			= 0;
    UINT64		flMicroSecsElapsed	= 0;

    /*WORD*			memoryPtr;
    //DWORD			StepSize;
    WORD*			maxMemoryPtr;
    WORD			byData = 0xff;
    int				test = (int)m_pMemoryArray;
    
    //StepSize		= m_StepSize;
    memoryPtr		= (WORD*)m_pMemoryArray;
    maxMemoryPtr	= (memoryPtr + m_ArraySize) - (10*m_StepSize);*/

    UINTN memoryPtr			= (UINTN)mMemoryArray;
    UINTN stepSize			= mStepSize;
    UINTN numSteps			= (stepSize/sizeof(UINTN))+1;
    UINTN sizeMinusXword	= (mArraySize-sizeof(UINTN));
    UINTN maxMemoryPtr		= (UINTN)(mMemoryArray+sizeMinusXword);
    INT64 bytesCopied		= ((sizeMinusXword/stepSize)*numSteps*sizeof(UINTN));

    AsciiFPrint(DEBUG_FILE_HANDLE, "Step start (memoryPtr=%p, numSteps=%d, size=%d)", memoryPtr, numSteps, sizeMinusXword);

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    AsmWbinvd();
#endif

    startclock();
    
#ifdef __GNUC__
#if defined(MDE_CPU_IA32)
    if(mRead)
    {
        __asm__ __volatile__(
            //Initialise value
            "mov %0, %%eax" "\n\t" // memoryPtr	
            "mov %0, %%ebx" "\n\t" // memoryPtr
            "mov %1, %%ecx" "\n\t" // numSteps Loop number of times of steps
            //mov edx
            "mov %2 ,%%esi" "\n\t" // stepSize
            "mov %3, %%edi" "\n\t" // maxMemoryPtr

            //Do work
            "start_outer_read_loop:" "\n\t"
            "start_inner_read_loop:" "\n\t"
            "mov (%%ebx), %%edx" "\n\t"
            "add %%esi, %%ebx" "\n\t"
            "cmp %%edi, %%ebx" "\n\t"
            "jl start_inner_read_loop" "\n\t"
            "mov %%eax,%%ebx" "\n\t"
            "dec %%ecx" "\n\t"
            "jnz start_outer_read_loop" "\n\t"
            //loop start_outer_read_loop //Goes to start_outer_loop while ecx > 0, also decrements ecx by 1
            :
            : "g"(memoryPtr), "g"(numSteps), "g" (stepSize), "g" (maxMemoryPtr)
            : "eax", "ebx", "ecx", "esi", "edi", "edx"
        );

    }
    else
    {
        __asm__ __volatile__(
            //Initialise value
            "mov %0,%%eax" "\n\t" // memoryPtr	
            "mov %0,%%ebx" "\n\t" // memoryPtr
            "mov %1,%%ecx" "\n\t" // numSteps //Loop number of times of steps
            "mov $0xFFFFFFFF,%%edx" "\n\t"  //Max DWORD
            "mov %2,%%esi" "\n\t" // stepSize
            "mov %3,%%edi" "\n\t" // maxMemoryPtr

            //Do work
            "start_outer_write_loop:" "\n\t"
            "start_inner_write_loop:" "\n\t"
            "mov %%edx,(%%ebx)" "\n\t"
            "add %%esi,%%ebx" "\n\t"
            "cmp %%edi,%%ebx" "\n\t"
            "jl start_inner_write_loop" "\n\t"
            "mov %%eax,%%ebx" "\n\t"
            "dec %%ecx" "\n\t"
            "jnz start_outer_write_loop" "\n\t"
            //loop start_outer_write_loop //Goes to start_outer_loop while ecx > 0, also decrements ecx by 1
            :
            : "g"(memoryPtr), "g"(numSteps), "g" (stepSize), "g" (maxMemoryPtr)
            : "eax", "ebx", "ecx", "esi", "edi", "edx", "memory"
        );
    }
#elif defined(MDE_CPU_X64)
    if(mRead)
    {
        __asm__ __volatile__(
            //Initialise value
            "mov %0, %%rax" "\n\t" // memoryPtr	
            "mov %0, %%rbx" "\n\t" // memoryPtr
            "mov %1, %%rcx" "\n\t" // numSteps Loop number of times of steps
            //mov edx
            "mov %2 ,%%rsi" "\n\t" // stepSize
            "mov %3, %%rdi" "\n\t" // maxMemoryPtr

            //Do work
            "start_outer_read_loop:" "\n\t"
            "start_inner_read_loop:" "\n\t"
            "mov (%%rbx), %%rdx" "\n\t"
            "add %%rsi, %%rbx" "\n\t"
            "cmp %%rdi, %%rbx" "\n\t"
            "jl start_inner_read_loop" "\n\t"
            "mov %%rax,%%rbx" "\n\t"
            "dec %%rcx" "\n\t"
            "jnz start_outer_read_loop" "\n\t"
            //loop start_outer_read_loop //Goes to start_outer_loop while ecx > 0, also decrements ecx by 1
            :
            : "g"(memoryPtr), "g"(numSteps), "g" (stepSize), "g" (maxMemoryPtr)
            : "rax", "rbx", "rcx", "rsi", "rdi", "rdx"
        );

    }
    else
    {
        __asm__ __volatile__(
            //Initialise value
            "mov %0,%%rax" "\n\t" // memoryPtr	
            "mov %0,%%rbx" "\n\t" // memoryPtr
            "mov %1,%%rcx" "\n\t" // numSteps //Loop number of times of steps
            "mov $0xFFFFFFFFFFFFFFFF,%%rdx" "\n\t" //Max QWORD
            "mov %2,%%rsi" "\n\t" // stepSize
            "mov %3,%%rdi" "\n\t" // maxMemoryPtr

            //Do work
            "start_outer_write_loop:" "\n\t"
            "start_inner_write_loop:" "\n\t"
            "mov %%rdx,(%%rbx)" "\n\t"
            "add %%rsi,%%rbx" "\n\t"
            "cmp %%rdi,%%rbx" "\n\t"
            "jl start_inner_write_loop" "\n\t"
            "mov %%rax,%%rbx" "\n\t"
            "dec %%rcx" "\n\t"
            "jnz start_outer_write_loop" "\n\t"
            //loop start_outer_write_loop //Goes to start_outer_loop while ecx > 0, also decrements ecx by 1
            :
            : "g"(memoryPtr), "g"(numSteps), "g" (stepSize), "g" (maxMemoryPtr)
            : "rax", "rbx", "rcx", "rsi", "rdi", "rdx", "memory"
        );
    }
#elif defined(MDE_CPU_AARCH64)
    if (mRead)
    {
        __asm__ __volatile__(
            //Initialise value
            "mov x0, %0" "\n\t" // memoryPtr
            "mov x1, x0" "\n\t"
            "mov x2, %1" "\n\t" // numSteps
            //mov edx
            "mov x3, %2" "\n\t" // stepSize
            "mov x4, %3" "\n\t" // maxMemoryPtr

            //Do work
            "start_outer_read_loop :" "\n\t"
            "  start_inner_read_loop:" "\n\t"
            "   ldr x5, [x1]" "\n\t"
            "	add x1,x1,x3" "\n\t"
            "	cmp x1, x4" "\n\t"
            "	blt start_inner_read_loop" "\n\t"
            "  mov x1, x0" "\n\t"
            "  subs x2,x2,#1" "\n\t"
            "  bgt start_outer_read_loop" "\n\t"
            //loop start_outer_read_loop //Goes to start_outer_loop while ecx > 0, also decrements ecx by 1
            :
            : "r"(memoryPtr), "r"(numSteps), "r" (stepSize), "r" (maxMemoryPtr)
            : "x0", "x1", "x2", "x3", "x4", "x5"
        );

    }
    else
    {
        __asm__ __volatile__(
            //Initialise value
            "mov x0, %0" "\n\t" // memoryPtr
            "mov x1, x0" "\n\t"
            "mov x2, %1" "\n\t" // numSteps
            //mov edx
            "mov x3, %2" "\n\t" // stepSize
            "mov x4, %3" "\n\t" // maxMemoryPtr
            "mov x5, #0xffffffffffffffff" "\n\t"

            //Do work
            "start_outer_write_loop :" "\n\t"
            "  start_inner_write_loop:" "\n\t"
            "   str x5, [x1]" "\n\t"
            "	add x1,x1,x3" "\n\t"
            "	cmp x1, x4" "\n\t"
            "	blt start_inner_write_loop" "\n\t"
            "  mov x1, x0" "\n\t"
            "  subs x2,x2,#1" "\n\t"
            "  bgt start_outer_write_loop " "\n\t"
            //loop start_outer_write_loop //Goes to start_outer_loop while ecx > 0, also decrements ecx by 1
            :
            : "r"(memoryPtr), "r"(numSteps), "r" (stepSize), "r" (maxMemoryPtr)
            : "x0", "x1", "x2", "x3", "x4", "x5", "memory"
        );
    }
#endif
#elif _MSC_VER
#if defined(MDE_CPU_IA32)
    if(mRead)
    {
        _asm 
        {
            //Save the CPU registers we are going to play with
            push eax	//Pointer to start of array
            push ebx	//pointer to current array location
            push ecx	//Outer loop counter
            push edx	//write the data to, or other temporary stuff
            push esi	//m_StepSize
            push edi	//maxMemoryPtr


            //Initialise value
            mov eax, memoryPtr	
            mov ebx, memoryPtr
            mov ecx, numSteps //Loop number of times of steps
            //mov edx
            mov esi, stepSize
            mov edi, maxMemoryPtr

            //Do work
            start_outer_read_loop:
            start_inner_read_loop:
            mov edx, DWORD PTR [ebx]
            add ebx, esi
            cmp ebx, edi
            jl start_inner_read_loop
            mov ebx, eax
            dec ecx
            jnz start_outer_read_loop
            //loop start_outer_read_loop //Goes to start_outer_loop while ecx > 0, also decrements ecx by 1


            pop edi
            pop esi
            pop edx
            pop ecx
            pop ebx
            pop eax
        }

    }
    else
    {

        _asm 
        {
            //Save the CPU registers we are going to play with
            push eax	//Pointer to start of array
            push ebx	//pointer to current array location
            push ecx	//Outer loop counter
            push edx	//write the data to, or other temporary stuff
            push esi	//m_StepSize
            push edi	//maxMemoryPtr


            //Initialise value
            mov eax, memoryPtr	
            mov ebx, memoryPtr
            mov ecx, numSteps //Loop number of times of steps
            mov edx, 0xFFFFFFFF //Max DWORD
            mov esi, stepSize
            mov edi, maxMemoryPtr

            //Do work
            start_outer_write_loop:
            start_inner_write_loop:
            mov DWORD PTR [ebx], edx
            add ebx, esi
            cmp ebx, edi
            jl start_inner_write_loop
            mov ebx, eax
            dec ecx
            jnz start_outer_write_loop
            //loop start_outer_write_loop //Goes to start_outer_loop while ecx > 0, also decrements ecx by 1


            pop edi
            pop esi
            pop edx
            pop ecx
            pop ebx
            pop eax
        }
    }
#elif defined(MDE_CPU_X64)
    {
        ADV_MEM_STEP_TEST_ARGS mta;
        mta.memoryPtr = memoryPtr;
        mta.numSteps = numSteps;
        mta.stepSize = stepSize;
        mta.maxMemoryPtr = maxMemoryPtr;
        if(mRead)
        {
            AdvMemTestStepReadAsm(&mta);
        }
        else
        {
            AdvMemTestStepWriteAsm(&mta);
        }
    }
#endif
#endif

    stopclock();

    flMicroSecsElapsed	= elapsed_time_us();
    
    AsciiFPrint(DEBUG_FILE_HANDLE, "Step finished (bytesCopied=%ld, elapsed=%ld)", bytesCopied, flMicroSecsElapsed);
    if (flMicroSecsElapsed == 0)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "** Warning elapsed time = %ldus)", flMicroSecsElapsed);
        flMicroSecsElapsed = 1;
    }

    flKBPerSec				= DivU64x64Remainder(MultU64x32(bytesCopied,1000000), MultU64x32(flMicroSecsElapsed, 1024), NULL);

    UpdateStats(flKBPerSec, mStepSize); 
    mTotalKBPerSec	+= flKBPerSec;

    // Finished?
    if(mCurrentIndex >= (UINT32)mNumSteps)
    {
        mTestState		= COMPLETE;
        mResultsAvailable = TRUE;
    }
    else
    {
        mStepSize *= 2;
    }

    //m_nDummy = byData;

    return bRet;
}

BOOLEAN StepBlock()
{
    int			nDataSizeBytes		= 0;
    BOOLEAN		bRet				= TRUE;
    UINTN		DataItemsPerBlock = 0;
    UINT64		dwNumBytesProcessed	= 0;
    UINT64		flMicroSecsElapsed	= 0;
    UINT64		flKBPerSec			= 0;
    UINTN		dwLoops				= 0;
    UINT8	  *	pBlock;

    switch(mDataSize)
    {
    case BYTE_BITS:
        nDataSizeBytes = 1;
        break;
    case WORD_BITS:
        nDataSizeBytes = 2;
        break;
    case DWORD_BITS:
        nDataSizeBytes = 4;
        break;
    case QWORD_BITS:
        nDataSizeBytes = 8;
        break;
    default:
        return bRet;
    }
    
    pBlock = (UINT8 *)AllocatePool(mBlockSize);
    if (pBlock == NULL)
        return bRet;

    DataItemsPerBlock = mBlockSize / nDataSizeBytes;
    // Make sure we always cycle through 32 MB of memory
    if(mBlockSize < SIZE_512MB)		
        dwLoops	= SIZE_512MB / mBlockSize;
    else 
        dwLoops	= 1;

    
    AsciiFPrint(DEBUG_FILE_HANDLE, "StepBlock start (pBlock=%p, DataItemsPerBlock=%d, dwLoops=%d mRead=%d)", pBlock, DataItemsPerBlock, dwLoops, mRead);

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    AsmWbinvd();
#endif

    startclock();


#ifdef __GNUC__
#if defined(MDE_CPU_IA32)
    switch (mDataSize) 
    {
    case BYTE_BITS:
        if (mRead) 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%eax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%esi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%edi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;

                //Do work
                "read_byte_outer:" "\n\t"			//do {
                "mov %%eax, %%ebx" "\n\t"			//	currentPointer = startPointer;
                "mov %%edi, %%ecx" "\n\t"			//	innerCounter = numInnerLoops;
                "read_byte_inner:" "\n\t"			//	do {
                "movb (%%ebx), %%dl" "\n\t"			//		ByteRegister = *currentPointer; //Read a Byte
                "inc %%ebx" "\n\t"					//		currentPointer++; //move by number of bytes read
                "dec %%ecx" "\n\t"					//		innerCounter--;
                "jnz read_byte_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%esi" "\n\t"					//	outerCounter--;
                "jnz read_byte_outer" "\n\t"		//} while (outerCounter != 0);
                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "eax", "esi", "edi", "ebx", "ecx", "dl"
            );
        }
        else 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%eax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%esi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%edi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;
                "mov $0xFF,%%dl" "\n\t"				//Max Byte

                //Do work
                "write_byte_outer:" "\n\t"			//do {
                "mov %%eax, %%ebx" "\n\t"			//	currentPointer = startPointer;
                "mov %%edi, %%ecx" "\n\t"			//	innerCounter = numInnerLoops;
                "write_byte_inner:" "\n\t"			//	do {
                "movb %%dl, (%%ebx)" "\n\t" 		//		*currentPointer = ByteRegister; //Write a Byte
                "inc %%ebx" "\n\t"					//		currentPointer++; //move by number of bytes written
                "dec %%ecx" "\n\t"					//		innerCounter--;
                "jnz write_byte_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%esi" "\n\t"					//	outerCounter--;
                "jnz write_byte_outer" "\n\t"		//} while (outerCounter != 0);
                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "eax", "esi", "edi", "ebx", "ecx", "dl", "memory"
            );
        }
        break;

    case WORD_BITS:
        if (mRead) 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%eax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%esi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%edi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;

                //Do work
                "read_word_outer:" "\n\t"			//do {
                "mov %%eax,%%ebx" "\n\t"			//	currentPointer = startPointer;
                "mov %%edi,%%ecx" "\n\t"			//	innerCounter = numInnerLoops;
                "read_word_inner:" "\n\t"			//	do {
                "movw (%%ebx),%%dx" "\n\t"			//		WordRegister = *((WORD*)currentPointer); //Read a WORD
                "add $2, %%ebx" "\n\t"				//		currentPointer+=2; //move by number of bytes read
                "dec %%ecx" "\n\t"					//		innerCounter--;
                "jnz read_word_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%esi" "\n\t"					//	outerCounter--;
                "jnz read_word_outer" "\n\t"		//} while (outerCounter != 0);

                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "eax", "esi", "edi", "ebx", "ecx", "dx"
            );
        }
        else 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%eax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%esi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%edi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;
                "mov $0xFFFF,%%dx" "\n\t"			//Max Word

                //Do work
                "write_word_outer:" "\n\t"			//do {
                "mov %%eax,%%ebx" "\n\t"			//	currentPointer = startPointer;
                "mov %%edi,%%ecx" "\n\t"			//	innerCounter = numInnerLoops;
                "write_word_inner:" "\n\t"			//	do {
                "movw %%dx, (%%ebx)" "\n\t" 		//		*((WORD*)currentPointer) = WordRegister; //Write a WORD
                "add $2,%%ebx" "\n\t"				//		currentPointer+=2; //move by number of bytes written
                "dec %%ecx" "\n\t"					//		innerCounter--;
                "jnz write_word_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%esi" "\n\t"					//	outerCounter--;
                "jnz write_word_outer" "\n\t"		//} while (outerCounter != 0);

                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "eax", "esi", "edi", "ebx", "ecx", "dx", "memory"
            );
        }
        break;

    case DWORD_BITS:
        if (mRead) 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%eax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%esi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%edi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;

                //Do work
                "read_dword_outer:" "\n\t"			//do {
                "mov %%eax,%%ebx" "\n\t"			//	currentPointer = startPointer;
                "mov %%edi,%%ecx" "\n\t"			//	innerCounter = numInnerLoops;
                "read_dword_inner:" "\n\t"			//	do {
                "movl (%%ebx),%%edx" "\n\t"			//		DWORDRegister = *((DWORD*)currentPointer); //Read a DWORD
                "add $4,%%ebx" "\n\t"				//		currentPointer+=4; //move by number of bytes read
                "dec %%ecx" "\n\t"					//		innerCounter--;
                "jnz read_dword_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%esi" "\n\t"					//	outerCounter--;
                "jnz read_dword_outer" "\n\t"		//} while (outerCounter != 0);

                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "eax", "esi", "edi", "ebx", "ecx", "edx"
            );
        }
        else 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%eax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%esi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%edi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;
                "mov $0xFFFFFFFF,%%edx" "\n\t" 		//Max DWORD

                //Do work
                "write_dword_outer:" "\n\t"			//do {
                "mov %%eax,%%ebx" "\n\t"			//	currentPointer = startPointer;
                "mov %%edi,%%ecx" "\n\t"			//	innerCounter = numInnerLoops;
                "write_dword_inner:" "\n\t"			//	do {
                "movl %%edx, (%%ebx)" "\n\t" 		//		*((DWORD*)currentPointer) = DWORDRegister; //Write a DWORD
                "add $4,%%ebx" "\n\t"				//		currentPointer+=4; //move by number of bytes written
                "dec %%ecx" "\n\t"					//		innerCounter--;
                "jnz write_dword_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%esi" "\n\t"					//	outerCounter--;
                "jnz write_dword_outer" "\n\t"		//} while (outerCounter != 0);

                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "eax", "esi", "edi", "ebx", "ecx", "edx", "memory"
            );
        }
        break;

    case QWORD_BITS:
        if (mRead) 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%eax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%esi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%edi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;

                //Do work
                "read_qword_outer:" "\n\t"			//do {
                "mov %%eax,%%ebx" "\n\t"			//	currentPointer = startPointer;
                "mov %%edi,%%ecx" "\n\t"			//	innerCounter = numInnerLoops;
                "read_qword_inner:" "\n\t"			//	do {
                                                    //		//Can't do QWORDS on 32-bit, do 2 DWORDS instead
                                                    //		//This does mean it's nearly identical to the DWORD test
                "movl (%%ebx),%%edx" "\n\t"			//		DWORDRegister = *((DWORD*)currentPointer); //Read a DWORD
                "add $4,%%ebx" "\n\t"				//		currentPointer+=4; //move by number of bytes read
                "movl (%%ebx),%%edx" "\n\t"			//		DWORDRegister = *((DWORD*)currentPointer); //Read a DWORD
                "add $4,%%ebx" "\n\t"				//		currentPointer+=4; //move by number of bytes read
                "dec %%ecx" "\n\t"					//		innerCounter--;
                "jnz read_qword_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%esi" "\n\t"					//	outerCounter--;
                "jnz read_qword_outer" "\n\t"		//} while (outerCounter != 0);

                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "eax", "esi", "edi", "ebx", "ecx", "edx"
            );
        }
        else 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%eax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%esi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%edi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;
                "mov $0xFFFFFFFF,%%edx" "\n\t" 		//Max DWORD

                //Do work
                "write_qword_outer:" "\n\t"			//do {
                "mov %%eax,%%ebx" "\n\t"				//	currentPointer = startPointer;
                "mov %%edi,%%ecx" "\n\t"				//	innerCounter = numInnerLoops;
                "write_qword_inner:" "\n\t"			//	do {
                                                    //		//Can't do QWORDS on 32-bit, do 2 DWORDS instead
                                                    //		//This does mean it's nearly identical to the DWORD test
                "movl %%edx,(%%ebx)" "\n\t"			//		*((DWORD*)currentPointer) = DWORDRegister; //Write a DWORD
                "add $4,%%ebx" "\n\t"				//		currentPointer+=4; //move by number of bytes written
                "movl %%edx,(%%ebx)" "\n\t"			//		*((DWORD*)currentPointer) = DWORDRegister; //Write a DWORD
                "add $4,%%ebx" "\n\t"				//		currentPointer+=4; //move by number of bytes written
                "dec %%ecx" "\n\t"					//		innerCounter--;
                "jnz write_qword_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%esi" "\n\t"					//	outerCounter--;
                "jnz write_qword_outer" "\n\t"		//} while (outerCounter != 0);

                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "eax", "esi", "edi", "ebx", "ecx", "edx", "memory"
            );
        }
        break;
    }
#elif defined(MDE_CPU_X64)
switch (mDataSize) 
    {
    case BYTE_BITS:
        if (mRead) 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%rax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%rsi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%rdi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;

                //Do work
                "read_byte_outer:" "\n\t"			//do {
                "mov %%rax, %%rbx" "\n\t"			//	currentPointer = startPointer;
                "mov %%rdi, %%rcx" "\n\t"			//	innerCounter = numInnerLoops;
                "read_byte_inner:" "\n\t"			//	do {
                "movb (%%rbx), %%dl" "\n\t"			//		ByteRegister = *currentPointer; //Read a Byte
                "inc %%rbx" "\n\t"					//		currentPointer++; //move by number of bytes read
                "dec %%rcx" "\n\t"					//		innerCounter--;
                "jnz read_byte_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%rsi" "\n\t"					//	outerCounter--;
                "jnz read_byte_outer" "\n\t"		//} while (outerCounter != 0);
                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "rax", "rsi", "rdi", "rbx", "rcx", "dl"
            );
        }
        else 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%rax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%rsi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%rdi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;
                "mov $0xFF,%%dl" "\n\t"				//Max Byte

                //Do work
                "write_byte_outer:" "\n\t"			//do {
                "mov %%rax, %%rbx" "\n\t"			//	currentPointer = startPointer;
                "mov %%rdi, %%rcx" "\n\t"			//	innerCounter = numInnerLoops;
                "write_byte_inner:" "\n\t"			//	do {
                "movb %%dl, (%%rbx)" "\n\t" 		//		*currentPointer = ByteRegister; //Write a Byte
                "inc %%rbx" "\n\t"					//		currentPointer++; //move by number of bytes written
                "dec %%rcx" "\n\t"					//		innerCounter--;
                "jnz write_byte_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%rsi" "\n\t"					//	outerCounter--;
                "jnz write_byte_outer" "\n\t"		//} while (outerCounter != 0);
                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "rax", "rsi", "rdi", "rbx", "rcx", "dl", "memory"
            );
        }
        break;

    case WORD_BITS:
        if (mRead) 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%rax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%rsi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%rdi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;

                //Do work
                "read_word_outer:" "\n\t"			//do {
                "mov %%rax,%%rbx" "\n\t"			//	currentPointer = startPointer;
                "mov %%rdi,%%rcx" "\n\t"			//	innerCounter = numInnerLoops;
                "read_word_inner:" "\n\t"			//	do {
                "movw (%%rbx),%%dx" "\n\t"			//		WordRegister = *((WORD*)currentPointer); //Read a WORD
                "add $2, %%rbx" "\n\t"				//		currentPointer+=2; //move by number of bytes read
                "dec %%rcx" "\n\t"					//		innerCounter--;
                "jnz read_word_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%rsi" "\n\t"					//	outerCounter--;
                "jnz read_word_outer" "\n\t"		//} while (outerCounter != 0);

                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "rax", "rsi", "rdi", "rbx", "rcx", "dx"
            );
        }
        else 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%rax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%rsi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%rdi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;
                "mov $0xFFFF,%%dx" "\n\t"			//Max Word

                //Do work
                "write_word_outer:" "\n\t"			//do {
                "mov %%rax,%%rbx" "\n\t"			//	currentPointer = startPointer;
                "mov %%rdi,%%rcx" "\n\t"			//	innerCounter = numInnerLoops;
                "write_word_inner:" "\n\t"			//	do {
                "movw %%dx, (%%rbx)" "\n\t" 		//		*((WORD*)currentPointer) = WordRegister; //Write a WORD
                "add $2,%%rbx" "\n\t"				//		currentPointer+=2; //move by number of bytes written
                "dec %%rcx" "\n\t"					//		innerCounter--;
                "jnz write_word_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%rsi" "\n\t"					//	outerCounter--;
                "jnz write_word_outer" "\n\t"		//} while (outerCounter != 0);

                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "rax", "rsi", "rdi", "rbx", "rcx", "dx", "memory"
            );
        }
        break;

    case DWORD_BITS:
        if (mRead) 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%rax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%rsi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%rdi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;

                //Do work
                "read_dword_outer:" "\n\t"			//do {
                "mov %%rax,%%rbx" "\n\t"			//	currentPointer = startPointer;
                "mov %%rdi,%%rcx" "\n\t"			//	innerCounter = numInnerLoops;
                "read_dword_inner:" "\n\t"			//	do {
                "movl (%%rbx),%%edx" "\n\t"			//		DWORDRegister = *((DWORD*)currentPointer); //Read a DWORD
                "add $4,%%rbx" "\n\t"				//		currentPointer+=4; //move by number of bytes read
                "dec %%rcx" "\n\t"					//		innerCounter--;
                "jnz read_dword_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%rsi" "\n\t"					//	outerCounter--;
                "jnz read_dword_outer" "\n\t"		//} while (outerCounter != 0);

                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "rax", "rsi", "rdi", "rbx", "rcx", "edx"
            );
        }
        else 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%rax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%rsi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%rdi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;
                "mov $0xFFFFFFFF,%%edx" "\n\t" 		//Max DWORD

                //Do work
                "write_dword_outer:" "\n\t"			//do {
                "mov %%rax,%%rbx" "\n\t"			//	currentPointer = startPointer;
                "mov %%rdi,%%rcx" "\n\t"			//	innerCounter = numInnerLoops;
                "write_dword_inner:" "\n\t"			//	do {
                "movl %%edx, (%%rbx)" "\n\t" 		//		*((DWORD*)currentPointer) = DWORDRegister; //Write a DWORD
                "add $4,%%rbx" "\n\t"				//		currentPointer+=4; //move by number of bytes written
                "dec %%rcx" "\n\t"					//		innerCounter--;
                "jnz write_dword_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%rsi" "\n\t"					//	outerCounter--;
                "jnz write_dword_outer" "\n\t"		//} while (outerCounter != 0);

                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "rax", "rsi", "rdi", "rbx", "rcx", "edx", "memory"
            );
        }
        break;

    case QWORD_BITS:
        if (mRead) 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%rax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%rsi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%rdi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;

                //Do work
                "read_qword_outer:" "\n\t"			//do {
                "mov %%rax,%%rbx" "\n\t"			//	currentPointer = startPointer;
                "mov %%rdi,%%rcx" "\n\t"			//	innerCounter = numInnerLoops;
                "read_qword_inner:" "\n\t"			//	do {
                "movq (%%rbx),%%rdx" "\n\t"			//		QWordRegister = *((QWORD*)currentPointer); //Read a QWORD
                "add $8,%%rbx" "\n\t"				//		currentPointer+=8; //move by number of bytes read
                "dec %%rcx" "\n\t"					//		innerCounter--;
                "jnz read_qword_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%rsi" "\n\t"					//	outerCounter--;
                "jnz read_qword_outer" "\n\t"		//} while (outerCounter != 0);

                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "rax", "rsi", "rdi", "rbx", "rcx", "rdx"
            );
        }
        else 
        {
            __asm__ __volatile__(
                //Initialise value
                "mov %0,%%rax" "\n\t" 				//startPointer = pBlock;
                "mov %1,%%rsi" "\n\t" 				//outerCounter = dwLoops;
                "mov %2,%%rdi" "\n\t" 				//numInnerLoops = DataItemsPerBlock;
                "mov $0xFFFFFFFFFFFFFFFF,%%rdx" "\n\t" //Max QWORD

                //Do work
                "write_qword_outer:" "\n\t"			//do {
                "mov %%rax,%%rbx" "\n\t"				//	currentPointer = startPointer;
                "mov %%rdi,%%rcx" "\n\t"				//	innerCounter = numInnerLoops;
                "write_qword_inner:" "\n\t"			//	do {
                "movq %%rdx,(%%rbx)" "\n\t"			//		*((QWORD*)currentPointer) = QWordRegister; //Write a QWORD
                "add $8,%%rbx" "\n\t"				//		currentPointer+=8; //move by number of bytes written
                "dec %%rcx" "\n\t"					//		innerCounter--;
                "jnz write_qword_inner" "\n\t"		//	} while (innerCounter != 0);
                "dec %%rsi" "\n\t"					//	outerCounter--;
                "jnz write_qword_outer" "\n\t"		//} while (outerCounter != 0);

                :
                : "g"(pBlock), "g"(dwLoops), "g" (DataItemsPerBlock)
                : "rax", "rsi", "rdi", "rbx", "rcx", "rdx", "memory"
            );
        }
        break;
    }
#elif defined(MDE_CPU_AARCH64)
    switch (mDataSize)
    {
    case BYTE_BITS:
        if (mRead)
        {			
            __asm__ __volatile__(
                //Initialise value
                "mov x0, %0" "\n\t" // pBlock
                "mov x1, %1" "\n\t" // dwLoops
                "mov x2, %2" "\n\t" // DataItemsPerBlock

                //Do work
                "read_byte_outer :" "\n\t"		//do {
                "  mov x3, x0" "\n\t"			//	currentPointer = startPointer;
                "  mov x4, x2" "\n\t"			//	innerCounter = numInnerLoops;
                "  read_byte_inner:" "\n\t"	//	do {
                "   ldrb w5, [x3]" "\n\t"		//		ByteRegister = *currentPointer; //Read a Byte
                "	add x3,x3,#1" "\n\t"		//		currentPointer++; //move by number of bytes read
                "   subs x4,x4,#1" "\n\t"		//		innerCounter--;
                "   bgt read_byte_inner" "\n\t"	//	} while (innerCounter != 0);
                "  subs x1,x1,#1" "\n\t"		//	outerCounter--;
                "  bgt read_byte_outer" "\n\t"	//} while (outerCounter != 0);
                :
                : "r"(pBlock), "r"(dwLoops), "r" (DataItemsPerBlock)
                : "x0", "x1", "x2", "x3", "x4", "w5"
            );
        }
        else
        {
            __asm__ __volatile__(
                //Initialise value
                "mov x0, %0" "\n\t" // pBlock
                "mov x1, %1" "\n\t" // dwLoops
                "mov x2, %2" "\n\t" // DataItemsPerBlock
                "mov w5, 0xFF" "\n\t" //Max Byte

                //Do work
                "write_byte_outer :" "\n\t"		//do {
                "  mov x3, x0" "\n\t"			//	currentPointer = startPointer;
                "  mov x4, x2" "\n\t"			//	innerCounter = numInnerLoops;
                "  write_byte_inner:" "\n\t"	//	do {
                "   strb w5, [x3]" "\n\t"		//		*currentPointer = ByteRegister; //Write a Byte
                "	add x3,x3,#1" "\n\t"		//		currentPointer++; //move by number of bytes read
                "   subs x4,x4,#1" "\n\t"		//		innerCounter--;
                "   bgt write_byte_inner" "\n\t"	//	} while (innerCounter != 0);
                "  subs x1,x1,#1" "\n\t"		//	outerCounter--;
                "  bgt write_byte_outer" "\n\t"	//} while (outerCounter != 0);
                :
                : "r"(pBlock), "r"(dwLoops), "r" (DataItemsPerBlock)
                : "x0", "x1", "x2", "x3", "x4", "w5", "memory"
            );
        }
        break;

    case WORD_BITS:
        if (mRead)
        {			
            __asm__ __volatile__(
                //Initialise value
                "mov x0, %0" "\n\t" // pBlock
                "mov x1, %1" "\n\t" // dwLoops
                "mov x2, %2" "\n\t" // DataItemsPerBlock

                //Do work
                "read_word_outer :" "\n\t"		//do {
                "  mov x3, x0" "\n\t"			//	currentPointer = startPointer;
                "  mov x4, x2" "\n\t"			//	innerCounter = numInnerLoops;
                "  read_word_inner:" "\n\t"	//	do {
                "   ldrh w5, [x3]" "\n\t"		//		WordRegister = *((WORD*)currentPointer); //Read a WORD
                "	add x3,x3,#2" "\n\t"		//		currentPointer+=2; //move by number of bytes read
                "   subs x4,x4,#1" "\n\t"		//		innerCounter--;
                "   bgt read_word_inner" "\n\t"	//	} while (innerCounter != 0);
                "  subs x1,x1,#1" "\n\t"		//	outerCounter--;
                "  bgt read_word_outer" "\n\t"	//} while (outerCounter != 0);
                :
                : "r"(pBlock), "r"(dwLoops), "r" (DataItemsPerBlock)
                : "x0", "x1", "x2", "x3", "x4", "w5"
            );
        }
        else
        {
            __asm__ __volatile__(
                //Initialise value
                "mov x0, %0" "\n\t" // pBlock
                "mov x1, %1" "\n\t" // dwLoops
                "mov x2, %2" "\n\t" // DataItemsPerBlock
                "mov w5, 0xFFFF" "\n\t" //Max Word

                //Do work
                "write_word_outer :" "\n\t"		//do {
                "  mov x3, x0" "\n\t"			//	currentPointer = startPointer;
                "  mov x4, x2" "\n\t"			//	innerCounter = numInnerLoops;
                "  write_word_inner:" "\n\t"	//	do {
                "   strh w5, [x3]" "\n\t"		//		*((WORD*)currentPointer) = WordRegister; //Write a WORD
                "	add x3,x3,#2" "\n\t"		//		currentPointer+=2; //move by number of bytes written
                "   subs x4,x4,#1" "\n\t"		//		innerCounter--;
                "   bgt write_word_inner" "\n\t"	//	} while (innerCounter != 0);
                "  subs x1,x1,#1" "\n\t"		//	outerCounter--;
                "  bgt write_word_outer" "\n\t"	//} while (outerCounter != 0);
                :
                : "r"(pBlock), "r"(dwLoops), "r" (DataItemsPerBlock)
                : "x0", "x1", "x2", "x3", "x4", "w5", "memory"
            );

        }
        break;

    case DWORD_BITS:
        if (mRead)
        {			
            __asm__ __volatile__(
                //Initialise value
                "mov x0, %0" "\n\t" // pBlock
                "mov x1, %1" "\n\t" // dwLoops
                "mov x2, %2" "\n\t" // DataItemsPerBlock

                //Do work
                "read_dword_outer :" "\n\t"		//do {
                "  mov x3, x0" "\n\t"			//	currentPointer = startPointer;
                "  mov x4, x2" "\n\t"			//	innerCounter = numInnerLoops;
                "  read_dword_inner:" "\n\t"	//	do {
                "   ldr w5, [x3]" "\n\t"		//		DWORDRegister = *((DWORD*)currentPointer); //Read a DWORD
                "	add x3,x3,#4" "\n\t"		//		currentPointer+=4; //move by number of bytes read
                "   subs x4,x4,#1" "\n\t"		//		innerCounter--;
                "   bgt read_dword_inner" "\n\t"	//	} while (innerCounter != 0);
                "  subs x1,x1,#1" "\n\t"		//	outerCounter--;
                "  bgt read_dword_outer" "\n\t"	//} while (outerCounter != 0);
                :
                : "r"(pBlock), "r"(dwLoops), "r" (DataItemsPerBlock)
                : "x0", "x1", "x2", "x3", "x4", "w5"
            );
        }
        else
        {			
            __asm__ __volatile__(
                //Initialise value
                "mov x0, %0" "\n\t" // pBlock
                "mov x1, %1" "\n\t" // dwLoops
                "mov x2, %2" "\n\t" // DataItemsPerBlock
                "mov w5, 0xFFFFFFFF" "\n\t" //Max DWORD

                //Do work
                "write_dword_outer :" "\n\t"		//do {
                "  mov x3, x0" "\n\t"			//	currentPointer = startPointer;
                "  mov x4, x2" "\n\t"			//	innerCounter = numInnerLoops;
                "  write_dword_inner:" "\n\t"	//	do {
                "   str w5, [x3]" "\n\t"		//		*((DWORD*)currentPointer) = DWORDRegister; //Write a DWORD
                "	add x3,x3,#4" "\n\t"		//		currentPointer+=4; //move by number of bytes written
                "   subs x4,x4,#1" "\n\t"		//		innerCounter--;
                "   bgt write_dword_inner" "\n\t"	//	} while (innerCounter != 0);
                "  subs x1,x1,#1" "\n\t"		//	outerCounter--;
                "  bgt write_dword_outer" "\n\t"	//} while (outerCounter != 0);
                :
                : "r"(pBlock), "r"(dwLoops), "r" (DataItemsPerBlock)
                : "x0", "x1", "x2", "x3", "x4", "w5", "memory"
            );
        }
        break;

    case QWORD_BITS:
        if (mRead)
        {
            __asm__ __volatile__(
                //Initialise value
                "mov x0, %0" "\n\t" // pBlock
                "mov x1, %1" "\n\t" // dwLoops
                "mov x2, %2" "\n\t" // DataItemsPerBlock

                //Do work
                "read_qword_outer :" "\n\t"		//do {
                "  mov x3, x0" "\n\t"			//	currentPointer = startPointer;
                "  mov x4, x2" "\n\t"			//	innerCounter = numInnerLoops;
                "  read_qword_inner:" "\n\t"	//	do {
                "   ldr x5, [x3]" "\n\t"		//		DWORDRegister = *((DWORD*)currentPointer); //Read a DWORD
                "	add x3,x3,#8" "\n\t"		//		currentPointer+=8; //move by number of bytes read
                "   subs x4,x4,#1" "\n\t"		//		innerCounter--;
                "   bgt read_qword_inner" "\n\t"	//	} while (innerCounter != 0);
                "  subs x1,x1,#1" "\n\t"		//	outerCounter--;
                "  bgt read_qword_outer" "\n\t"	//} while (outerCounter != 0);
                :
                : "r"(pBlock), "r"(dwLoops), "r" (DataItemsPerBlock)
                : "x0", "x1", "x2", "x3", "x4", "x5"
            );
        }
        else
        {
            __asm__ __volatile__(
                //Initialise value
                "mov x0, %0" "\n\t" // pBlock
                "mov x1, %1" "\n\t" // dwLoops
                "mov x2, %2" "\n\t" // DataItemsPerBlock
                "mov x5, 0xFFFFFFFFFFFFFFFF" "\n\t" //Max QWORD

                //Do work
                "write_qword_outer :" "\n\t"		//do {
                "  mov x3, x0" "\n\t"			//	currentPointer = startPointer;
                "  mov x4, x2" "\n\t"			//	innerCounter = numInnerLoops;
                "  write_qword_inner:" "\n\t"	//	do {
                "   str x5, [x3]" "\n\t"		//		*((QWORD*)currentPointer) = QWORDRegister; //Write a QWORD
                "	add x3,x3,#8" "\n\t"		//		currentPointer+=8; //move by number of bytes written
                "   subs x4,x4,#1" "\n\t"		//		innerCounter--;
                "   bgt write_qword_inner" "\n\t"	//	} while (innerCounter != 0);
                "  subs x1,x1,#1" "\n\t"		//	outerCounter--;
                "  bgt write_qword_outer" "\n\t"	//} while (outerCounter != 0);
                :
                : "r"(pBlock), "r"(dwLoops), "r" (DataItemsPerBlock)
                : "x0", "x1", "x2", "x3", "x4", "x5", "memory"
            );
        }
        break;
    }
#endif
#elif defined(_MSC_VER)
#if defined(MDE_CPU_IA32)
    switch (mDataSize) 
    {
    case BYTE_BITS:
        if (mRead) 
        {
            _asm 
            {
                //Save the CPU registers we are going to play with
                push eax	//Byte Pointer to start of array
                push ebx	//Byte pointer to current array location
                push ecx	//inner loop counter
                push dx		//write the data to, or other temporary stuff
                push esi	//outer loops (counter)
                push edi	//inner loops

                //Initialise values
                mov eax, pBlock				//startPointer = pBlock;
                mov esi, dwLoops			//outerCounter = dwLoops;
                mov edi, DataItemsPerBlock	//numInnerLoops = DataItemsPerBlock;

                //Do work
                read_byte_outer:		//do {
                mov ebx, eax			//	currentPointer = startPointer;
                mov ecx, edi			//	innerCounter = numInnerLoops;
                read_byte_inner:		//	do {
                mov dl, BYTE PTR [ebx]	//		ByteRegister = *currentPointer; //Read a Byte
                inc ebx					//		currentPointer++; //move by number of bytes read
                dec ecx					//		innerCounter--;
                jnz read_byte_inner		//	} while (innerCounter != 0);
                dec esi					//	outerCounter--;
                jnz read_byte_outer		//} while (outerCounter != 0);

                //pop back off
                pop edi
                pop esi
                pop dx
                pop ecx
                pop ebx
                pop eax
            }
        }
        else 
        {
            _asm 
            {
                //Save the CPU registers we are going to play with
                push eax	//Byte Pointer to start of array
                push ebx	//Byte pointer to current array location
                push ecx	//inner loop counter
                push dx		//write the data to, or other temporary stuff
                push esi	//outer loops (counter)
                push edi	//inner loops

                //Initialise values
                mov eax, pBlock				//startPointer = pBlock;
                mov esi, dwLoops			//outerCounter = dwLoops;
                mov edi, DataItemsPerBlock	//numInnerLoops = DataItemsPerBlock;
                mov dl,  0xFF				//Max Byte

                //Do work
                write_byte_outer:		//do {
                mov ebx, eax			//	currentPointer = startPointer;
                mov ecx, edi			//	innerCounter = numInnerLoops;
                write_byte_inner:		//	do {
                mov BYTE PTR [ebx], dl 	//		 *currentPointer = ByteRegister; //Write a Byte
                inc ebx					//		currentPointer++; //move by number of bytes written
                dec ecx					//		innerCounter--;
                jnz write_byte_inner	//	} while (innerCounter != 0);
                dec esi					//	outerCounter--;
                jnz write_byte_outer	//} while (outerCounter != 0);

                //pop back off
                pop edi
                pop esi
                pop dx
                pop ecx
                pop ebx
                pop eax
            }
        }
        break;

    case WORD_BITS:
        if (mRead) 
        {
            _asm 
            {
                //Save the CPU registers we are going to play with
                push eax	//Byte Pointer to start of array
                push ebx	//Byte pointer to current array location
                push ecx	//inner loop counter
                push dx		//write the data to, or other temporary stuff
                push esi	//outer loops (counter)
                push edi	//inner loops

                //Initialise values
                mov eax, pBlock				//startPointer = pBlock;
                mov esi, dwLoops			//outerCounter = dwLoops;
                mov edi, DataItemsPerBlock	//numInnerLoops = DataItemsPerBlock;

                //Do work
                read_word_outer:		//do {
                mov ebx, eax			//	currentPointer = startPointer;
                mov ecx, edi			//	innerCounter = numInnerLoops;
                read_word_inner:		//	do {
                mov dx, WORD PTR [ebx]	//		WordRegister = *((WORD*)currentPointer); //Read a WORD
                add ebx, 2				//		currentPointer+=2; //move by number of bytes read
                dec ecx					//		innerCounter--;
                jnz read_word_inner		//	} while (innerCounter != 0);
                dec esi					//	outerCounter--;
                jnz read_word_outer		//} while (outerCounter != 0);

                //pop back off
                pop edi
                pop esi
                pop dx
                pop ecx
                pop ebx
                pop eax
            }
        }
        else 
        {
            _asm 
            {
                //Save the CPU registers we are going to play with
                push eax	//Byte Pointer to start of array
                push ebx	//Byte pointer to current array location
                push ecx	//inner loop counter
                push dx		//write the data to, or other temporary stuff
                push esi	//outer loops (counter)
                push edi	//inner loops

                //Initialise values
                mov eax, pBlock				//startPointer = pBlock;
                mov esi, dwLoops			//outerCounter = dwLoops;
                mov edi, DataItemsPerBlock	//numInnerLoops = DataItemsPerBlock;
                mov dx,  0xFFFF				//Max Word

                //Do work
                write_word_outer:		//do {
                mov ebx, eax			//	currentPointer = startPointer;
                mov ecx, edi			//	innerCounter = numInnerLoops;
                write_word_inner:		//	do {
                mov WORD PTR [ebx], dx 	//		*((WORD*)currentPointer) = WordRegister; //Write a WORD
                add ebx, 2				//		currentPointer+=2; //move by number of bytes written
                dec ecx					//		innerCounter--;
                jnz write_word_inner	//	} while (innerCounter != 0);
                dec esi					//	outerCounter--;
                jnz write_word_outer	//} while (outerCounter != 0);

                //pop back off
                pop edi
                pop esi
                pop dx
                pop ecx
                pop ebx
                pop eax
            }
        }
        break;

    case DWORD_BITS:
        if (mRead) 
        {
            _asm 
            {
                //Save the CPU registers we are going to play with
                push eax	//Byte Pointer to start of array
                push ebx	//Byte pointer to current array location
                push ecx	//inner loop counter
                push edx	//write the data to, or other temporary stuff
                push esi	//outer loops (counter)
                push edi	//inner loops

                //Initialise values
                mov eax, pBlock				//startPointer = pBlock;
                mov esi, dwLoops			//outerCounter = dwLoops;
                mov edi, DataItemsPerBlock	//numInnerLoops = DataItemsPerBlock;

                //Do work
                read_dword_outer:			//do {
                mov ebx, eax				//	currentPointer = startPointer;
                mov ecx, edi				//	innerCounter = numInnerLoops;
                read_dword_inner:			//	do {
                mov edx, DWORD PTR [ebx]	//		DWORDRegister = *((DWORD*)currentPointer); //Read a DWORD
                add ebx, 4					//		currentPointer+=4; //move by number of bytes read
                dec ecx						//		innerCounter--;
                jnz read_dword_inner		//	} while (innerCounter != 0);
                dec esi						//	outerCounter--;
                jnz read_dword_outer		//} while (outerCounter != 0);

                //pop back off
                pop edi
                pop esi
                pop edx
                pop ecx
                pop ebx
                pop eax
            }
        }
        else 
        {
            _asm 
            {
                //Save the CPU registers we are going to play with
                push eax	//Byte Pointer to start of array
                push ebx	//Byte pointer to current array location
                push ecx	//inner loop counter
                push edx	//write the data to, or other temporary stuff
                push esi	//outer loops (counter)
                push edi	//inner loops

                //Initialise values
                mov eax, pBlock				//startPointer = pBlock;
                mov esi, dwLoops			//outerCounter = dwLoops;
                mov edi, DataItemsPerBlock	//numInnerLoops = DataItemsPerBlock;
                mov edx, 0xFFFFFFFF			//Max DWORD

                //Do work
                write_dword_outer:			//do {
                mov ebx, eax				//	currentPointer = startPointer;
                mov ecx, edi				//	innerCounter = numInnerLoops;
                write_dword_inner:			//	do {
                mov DWORD PTR [ebx], edx	//		*((DWORD*)currentPointer) = DWORDRegister; //Write a DWORD
                add ebx, 4					//		currentPointer+=4; //move by number of bytes written
                dec ecx						//		innerCounter--;
                jnz write_dword_inner		//	} while (innerCounter != 0);
                dec esi						//	outerCounter--;
                jnz write_dword_outer		//} while (outerCounter != 0);

                //pop back off
                pop edi
                pop esi
                pop edx
                pop ecx
                pop ebx
                pop eax
            }
        }
        break;

    case QWORD_BITS:
        if (mRead) 
        {
            _asm 
            {
                //Save the CPU registers we are going to play with
                push eax	//Byte Pointer to start of array
                push ebx	//Byte pointer to current array location
                push ecx	//inner loop counter
                push edx	//write the data to, or other temporary stuff
                push esi	//outer loops (counter)
                push edi	//inner loops

                //Initialise values
                mov eax, pBlock				//startPointer = pBlock;
                mov esi, dwLoops			//outerCounter = dwLoops;
                mov edi, DataItemsPerBlock	//numInnerLoops = DataItemsPerBlock;

                //Do work
                read_qword_outer:			//do {
                mov ebx, eax				//	currentPointer = startPointer;
                mov ecx, edi				//	innerCounter = numInnerLoops;
                read_qword_inner:			//	do {
                                            //		//Can't do QWORDS on 32-bit, do 2 DWORDS instead
                                            //		//This does mean it's nearly identical to the DWORD test
                mov edx, DWORD PTR [ebx]	//		DWORDRegister = *((DWORD*)currentPointer); //Read a DWORD
                add ebx, 4					//		currentPointer+=4; //move by number of bytes read
                mov edx, DWORD PTR [ebx]	//		DWORDRegister = *((DWORD*)currentPointer); //Read a DWORD
                add ebx, 4					//		currentPointer+=4; //move by number of bytes read
                dec ecx						//		innerCounter--;
                jnz read_qword_inner			//	} while (innerCounter != 0);
                dec esi						//	outerCounter--;
                jnz read_qword_outer			//} while (outerCounter != 0);

                //pop back off
                pop edi
                pop esi
                pop edx
                pop ecx
                pop ebx
                pop eax
            }
        }
        else 
        {
            _asm 
            {
                //Save the CPU registers we are going to play with
                push eax	//Byte Pointer to start of array
                push ebx	//Byte pointer to current array location
                push ecx	//inner loop counter
                push edx	//write the data to, or other temporary stuff
                push esi	//outer loops (counter)
                push edi	//inner loops

                //Initialise values
                mov eax, pBlock				//startPointer = pBlock;
                mov esi, dwLoops			//outerCounter = dwLoops;
                mov edi, DataItemsPerBlock	//numInnerLoops = DataItemsPerBlock;
                mov edx, 0xFFFFFFFF			//Max DWORD

                //Do work
                write_qword_outer:			//do {
                mov ebx, eax				//	currentPointer = startPointer;
                mov ecx, edi				//	innerCounter = numInnerLoops;
                write_qword_inner:			//	do {
                                            //		//Can't do QWORDS on 32-bit, do 2 DWORDS instead
                                            //		//This does mean it's nearly identical to the DWORD test
                mov DWORD PTR [ebx], edx	//		*((DWORD*)currentPointer) = WordRegister; //Write a DWORD
                add ebx, 4					//		currentPointer+=4; //move by number of bytes written
                mov DWORD PTR [ebx], edx	//		*((DWORD*)currentPointer) = WordRegister; //Write a DWORD
                add ebx, 4					//		currentPointer+=4; //move by number of bytes written
                dec ecx						//		innerCounter--;
                jnz write_qword_inner		//	} while (innerCounter != 0);
                dec esi						//	outerCounter--;
                jnz write_qword_outer		//} while (outerCounter != 0);

                //pop back off
                pop edi
                pop esi
                pop edx
                pop ecx
                pop ebx
                pop eax
            }
        }
        break;
    }
#elif defined(MDE_CPU_X64)
    {
        ADV_MEM_BLOCK_STEP_TEST_ARGS mta;
        mta.pBlock				= (UINTN)pBlock;
        mta.dwLoops				= (UINTN)dwLoops;
        mta.DataItemsPerBlock	= (UINTN)DataItemsPerBlock;
        switch (mDataSize) 
        {
        case BYTE_BITS:
            if (mRead) 
            {
                AdvMemTestBlockStepReadBytesAsm(&mta);
            }
            else 
            {
                AdvMemTestBlockStepWriteBytesAsm(&mta);
            }
            break;

        case WORD_BITS:
            if (mRead) 
            {
                AdvMemTestBlockStepReadWordsAsm(&mta);
            }
            else 
            {
                AdvMemTestBlockStepWriteWordsAsm(&mta);
            }
            break;

        case DWORD_BITS:
            if (mRead) 
            {
                AdvMemTestBlockStepReadDWordsAsm(&mta);
            }
            else 
            {
                AdvMemTestBlockStepWriteDWordsAsm(&mta);
            }
            break;

        case QWORD_BITS:
            if (mRead) 
            {
                AdvMemTestBlockStepReadQWordsAsm(&mta);
            }
            else 
            {
                AdvMemTestBlockStepWriteQWordsAsm(&mta);
            }
            break;
        }
    }
#endif
#endif
    stopclock();

    //dwNumBytesProcessed = 32 * ONE_MEGABYTE;
    dwNumBytesProcessed = MultU64x32(MultU64x64((UINT64)dwLoops,(UINT64)DataItemsPerBlock),nDataSizeBytes);
    flMicroSecsElapsed	= elapsed_time_us();

    AsciiFPrint(DEBUG_FILE_HANDLE, "StepBlock finished (bytes=%ld, elapsed=%ld)", dwNumBytesProcessed, flMicroSecsElapsed);
    if (flMicroSecsElapsed == 0)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "** Warning elapsed time = %ldus)", flMicroSecsElapsed);
        flMicroSecsElapsed = 1;
    }

    flKBPerSec			= DivU64x64Remainder(MultU64x32(dwNumBytesProcessed, 1000000),MultU64x32(flMicroSecsElapsed,1024),NULL);
    UpdateStats(flKBPerSec, mBlockSize);	

    mTotalKBPerSec	+= flKBPerSec;
    mBlockSize			*= 2;
    mNumSamples		= mCurrentIndex;
    FreePool(pBlock);

    return bRet;
}
ENABLE_OPTIMISATIONS()

BOOLEAN UpdateStats(UINT64 flKBPerSecond, UINTN StepSize)
{
    if (mCurrentIndex >= MAX_NUM_SAMPLES)
        return FALSE;

    mMemoryStats[mCurrentIndex].dKBPerSecond		= flKBPerSecond;

    mMemoryStats[mCurrentIndex].StepSize			= StepSize;

    mCurrentIndex++;

    return TRUE;
}

UINTN GetNumSamples()
{
    return mCurrentIndex;
}

BOOLEAN GetSample(UINTN index, MEMORY_STATS *pSample)
{
    if (index >= mCurrentIndex)
        return FALSE;

    CopyMem(pSample, &mMemoryStats[index], sizeof(MEMORY_STATS));

    return TRUE;
}

void SetDataSize(int nVal)
{
    mDataSize = nVal;
}

void SetReadWrite(BOOLEAN bVal)
{
    mRead = bVal;
}

BOOLEAN GetReadWrite()
{
    return mRead;
}

void SetTestMode(int nVal)
{
    mTestMode = nVal;
}

int GetTestMode()
{
    return mTestMode;
}

UINTN GetCurrentStepSize()
{
    return mStepSize;
}

UINTN GetBlockSize()
{
    return mBlockSize;
}

UINTN GetMaxBlockSize()
{
    return mMaxBlockSize;
}

int	GetDataSize()
{
    return mDataSize;
}

UINTN GetArraySize()
{
    return mArraySize;
}

void SetAveSpeed(UINT64 fVal)
{
    mAveSpeed = fVal;
}

UINT64 GetAveSpeed()
{
    return mAveSpeed;
}

UINT64 GetTotalKBPerSec()
{
    return mTotalKBPerSec;
}

void SetTestState(int nVal)
{
    mTestState = nVal;
}

int GetTestState()
{
    return mTestState;
}

void SetResultsAvailable(BOOLEAN bVal)
{
    mResultsAvailable = bVal;
}