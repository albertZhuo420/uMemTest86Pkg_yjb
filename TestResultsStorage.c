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
//	TestResultsStorage.c
//
//Author(s):
//	Keith Mah
//
//Description:
//	Routine declarations for storing and retrieving test results from disk storage
//
//
//History
//	07 Aug 2014: Initial version

#include <Library/BaseMemoryLib.h>
#include "uMemTest86.h"
#include "TestResultsStorage.h"
#include "sha2.h"

#define HASH_SALT "4`<!(?<LG>erx,R;0g% 6RL>?{ &3!g0DaHL`njNUZ]Ugm`#;qKxYp80D{tw#HH>KgXf*!3p)hIuh^)12eOo9o=E#*+gvh,9gL#N45#7#Dq1aiYq,`XOQiP>4&.nEaU6"
#define HASH_SIZE SHA256_DIGEST_LENGTH
#define HASH_STRING_SIZE SHA256_DIGEST_STRING_LENGTH

// ExtractTestInfo
//
// Extract TestInfo YAML node
BOOLEAN ExtractTestInfo(yaml_document_t *document, yaml_node_t *node, TestResultsMetaInfo *resultsInfo);

// ExtractVersion
//
// Extract Version YAML node
BOOLEAN ExtractVersion(yaml_document_t *document, yaml_node_t *node, MTVersion *mtVersion);

// ExtractMemTestHeader
//
// Extract MemTestHeader YAML node
BOOLEAN ExtractMemTestHeader(yaml_document_t *document, yaml_node_t *node, MEM_RESULTS *results);

// ExtractChecksum
//
// Extract Checksum YAML node
BOOLEAN ExtractChecksum(yaml_document_t *document, yaml_node_t *node, CHAR8 Hash[HASH_STRING_SIZE]);

void TraverseNode(yaml_document_t *document, int index);

// CalculateHash
//
// Calculate hash of benchmark data and store in string form
VOID CalculateHash(TestResultsMetaInfo *TestInfo, MEM_RESULTS *Header, UINT64 SampleX[], UINT64 SampleY[], int NumSamples, CHAR8 Hash[HASH_STRING_SIZE]);

BOOLEAN ExtractTestInfo(yaml_document_t *document, yaml_node_t *node, TestResultsMetaInfo *resultsInfo)
{
	int k;
	SetMem(resultsInfo,sizeof(*resultsInfo),0);

	if (node->type != YAML_MAPPING_NODE)
	{
		AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid TestInfo node type: %d", node->type);
		return FALSE;
	}

	for (k = 0; k < (node->data.mapping.pairs.top - node->data.mapping.pairs.start); k ++) {
		yaml_node_t *keynode = yaml_document_get_node(document, node->data.mapping.pairs.start[k].key);
		if (keynode->type == YAML_SCALAR_NODE)
		{
			yaml_node_t *valuenode = yaml_document_get_node(document, node->data.mapping.pairs.start[k].value);
			if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "Type") == 0 && keynode->data.scalar.length <= 32)
				resultsInfo->iTestType = (int)AsciiStrDecimalToUintn((CHAR8*)valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "StartTime" ) == 0 && keynode->data.scalar.length <= 32)
				resultsInfo->i64TestStartTime = AsciiStrDecimalToUint64((CHAR8*)valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "NumCPU" ) == 0 && keynode->data.scalar.length <= 32)
				resultsInfo->uiNumCPU = (UINT32)AsciiStrDecimalToUintn((CHAR8*)valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "CPUType") == 0 && keynode->data.scalar.length <= 64)
				UnicodeSPrint(resultsInfo->szCPUType, sizeof(resultsInfo->szCPUType), L"%a", valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "CPUSpeed") == 0 && keynode->data.scalar.length <= 32)
				UnicodeSPrint(resultsInfo->szCPUSpeed, sizeof(resultsInfo->szCPUSpeed), L"%a", valuenode->data.scalar.value);
		}
	}

#if 0
	AsciiFPrint(DEBUG_FILE_HANDLE, "\n"
									"TestInfo:\n"
									" iTestType = %d\n"
									" i64TestStartTime = %ld\n"
									" szComputerName = %s\n"
									" uiNumCPU = %d\n"
									" szCPUType = %s\n"
									" szCPUSpeed = %s\n",
									resultsInfo->iTestType,
									resultsInfo->i64TestStartTime,
									resultsInfo->szComputerName,
									resultsInfo->uiNumCPU,
									resultsInfo->szCPUType,
									resultsInfo->szCPUSpeed);
#endif
	return TRUE;
}

BOOLEAN ExtractVersion(yaml_document_t *document, yaml_node_t *node, MTVersion *mtVersion)
{
	int k;
	SetMem(mtVersion,sizeof(*mtVersion),0);

	if (node->type != YAML_MAPPING_NODE)
	{
		AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid Version node type: %d", node->type);
		return FALSE;
	}

	for (k = 0; k < (node->data.mapping.pairs.top - node->data.mapping.pairs.start); k ++) {
		yaml_node_t *keynode = yaml_document_get_node(document, node->data.mapping.pairs.start[k].key);
		if (keynode->type == YAML_SCALAR_NODE)
		{
			yaml_node_t *valuenode = yaml_document_get_node(document, node->data.mapping.pairs.start[k].value);
			if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "Major") == 0 && keynode->data.scalar.length <= 32)
				mtVersion->m_Major = (UINT16)AsciiStrDecimalToUintn((CHAR8*)valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "Minor") == 0 && keynode->data.scalar.length <= 32)
				mtVersion->m_Minor = (UINT16)AsciiStrDecimalToUintn((CHAR8*)valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "Build") == 0 && keynode->data.scalar.length <= 32)
				mtVersion->m_Build  = (UINT16)AsciiStrDecimalToUintn((CHAR8*)valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "SpecialBuildName") == 0 && keynode->data.scalar.length <= 32)
				UnicodeSPrint(mtVersion->m_SpecialBuildName, sizeof(mtVersion->m_SpecialBuildName), L"%a", valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "Architecture") == 0 && keynode->data.scalar.length <= 32)
				mtVersion->m_ptArchitecture = AsciiStrDecimalToUintn((CHAR8*)valuenode->data.scalar.value);
		}
	}

#if 0
	AsciiFPrint(DEBUG_FILE_HANDLE, "\n"
									"Version:\n"
									" m_Major = %d\n"
									" m_Minor = %d\n"
									" m_Build = %d\n"
									" m_SpecialBuildName = %s\n"
									" m_ptArchitecture = %d",
									mtVersion->m_Major,
									mtVersion->m_Minor,
									mtVersion->m_Build,
									mtVersion->m_SpecialBuildName,
									mtVersion->m_ptArchitecture);
#endif
	return TRUE;
}

BOOLEAN ExtractMemTestHeader(yaml_document_t *document, yaml_node_t *node, MEM_RESULTS *results)
{	
	int k;
	SetMem(results,sizeof(*results),0);

	if (node->type != YAML_MAPPING_NODE)
	{
		AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid MemTestHeader node type: %d", node->type);
		return FALSE;
	}

	for (k = 0; k < (node->data.mapping.pairs.top - node->data.mapping.pairs.start); k ++) {
		yaml_node_t *keynode = yaml_document_get_node(document, node->data.mapping.pairs.start[k].key);
		if (keynode->type == YAML_SCALAR_NODE)
		{
			yaml_node_t *valuenode = yaml_document_get_node(document, node->data.mapping.pairs.start[k].value);
			if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "TotalPhys") == 0 && keynode->data.scalar.length <= 32)
				results->TotalPhys = AsciiStrDecimalToUint64((CHAR8*)valuenode->data.scalar.value );
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "TestMode") == 0 && keynode->data.scalar.length <= 32)
				results->iTestMode = (int)AsciiStrDecimalToUintn((CHAR8*)valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "AveSpeed") == 0 && keynode->data.scalar.length <= 32)
				results->fAveSpeed = AsciiStrDecimalToUint64((CHAR8*)valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "AveCPULoad") == 0 && keynode->data.scalar.length <= 32)
				results->fAveCPULoad = AsciiStrDecimalToUintn((CHAR8*)valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "ModuleManuf") == 0 && keynode->data.scalar.length <= 64)
				UnicodeSPrint(results->szModuleManuf, sizeof(results->szModuleManuf), L"%a", valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "ModulePartNo") == 0 && keynode->data.scalar.length <= 32)
				UnicodeSPrint(results->szModulePartNo, sizeof(results->szModulePartNo), L"%a", valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "RAMType") == 0 && keynode->data.scalar.length <= 32)
				UnicodeSPrint(results->szRAMType, sizeof(results->szRAMType), L"%a", valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "RAMTiming") == 0 && keynode->data.scalar.length <= 32)
				UnicodeSPrint(results->szRAMTiming, sizeof(results->szRAMTiming), L"%a", valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "IsRead") == 0 && keynode->data.scalar.length <= 32)
				results->bRead = AsciiStrCmp((CHAR8*)valuenode->data.scalar.value, "true") == 0;
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "ArraySize") == 0 && keynode->data.scalar.length <= 32)
				results->ArraySize = AsciiStrDecimalToUint64((CHAR8*)valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "MinBlock") == 0 && keynode->data.scalar.length <= 32)
				results->MinBlock= AsciiStrDecimalToUint64((CHAR8*)valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "MaxBlock") == 0 && keynode->data.scalar.length <= 32)
				results->MaxBlock = AsciiStrDecimalToUint64((CHAR8*)valuenode->data.scalar.value);
			else if (AsciiStrCmp((CHAR8*)keynode->data.scalar.value, "AccessDataSize") == 0 && keynode->data.scalar.length <= 32)
				results->dwAccessDataSize = (int)AsciiStrDecimalToUintn((CHAR8*)valuenode->data.scalar.value);
		}
	}

#if 0
	AsciiFPrint(DEBUG_FILE_HANDLE, "\n"
									"MemTestHeader:\n"
									" TotalPhys = %ld\n"
									" iTestMode = %d\n"
									" fAveSpeed = %ld\n"
									" fAveCPULoad = %ld\n"
									" szModuleManuf = %s\n"
									" szModulePartNo = %s\n"
									" szRAMType = %s\n"
									" szRAMTiming = %s\n"
									" bRead = %s\n"
									" ArraySize = %ld\n"
									" MinBlock = %ld\n"
									" MaxBlock = %ld\n"
									" dwAccessDataSize = %d\n",
									results->TotalPhys,
									results->iTestMode,
									results->fAveSpeed,
									results->fAveCPULoad,
									results->szModuleManuf,
									results->szModulePartNo,
									results->szRAMType,
									results->szRAMTiming,
									results->bRead ? L"true" : L"false",
									(UINT64)results->ArraySize,
									(UINT64)results->MinBlock,
									(UINT64)results->MaxBlock,
									results->dwAccessDataSize);
#endif
	return TRUE;

}

BOOLEAN ExtractChecksum(yaml_document_t *document, yaml_node_t *node, CHAR8 Hash[HASH_STRING_SIZE])
{
	SetMem(Hash,HASH_STRING_SIZE,0);

	if (node->type != YAML_SCALAR_NODE)
	{
		AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid Checksum node type: %d", node->type);
		return FALSE;
	}

	if (node->data.scalar.length >= HASH_STRING_SIZE)
	{
		AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid Checksum length: %d", node->data.scalar.length);
		return FALSE;
	}

	AsciiStrCpyS(Hash, HASH_STRING_SIZE, (CHAR8*)node->data.scalar.value);

	return TRUE;
}

void TraverseNode(yaml_document_t *document, int index)
{
	yaml_node_t *node = yaml_document_get_node(document, index);
	int k;

	if (node == NULL)
		return;

	switch (node->type) {
        case YAML_SCALAR_NODE:
			AsciiFPrint(DEBUG_FILE_HANDLE, "[Node %d] Got scalar (value %a)", index, node->data.scalar.value);
            break;
        case YAML_SEQUENCE_NODE:
			AsciiFPrint(DEBUG_FILE_HANDLE, "[Node %d] Got sequence (%d items)", index, (node->data.sequence.items.top - node->data.sequence.items.start));

			for (k = 0; k < (node->data.sequence.items.top - node->data.sequence.items.start); k ++)
			{
				TraverseNode(document, node->data.sequence.items.start[k]);
            }
            break;
        case YAML_MAPPING_NODE:
			AsciiFPrint(DEBUG_FILE_HANDLE, "[Node %d] Got mapping (%d items)", index, (node->data.mapping.pairs.top - node->data.mapping.pairs.start));

			for (k = 0; k < (node->data.mapping.pairs.top - node->data.mapping.pairs.start); k ++) {
				TraverseNode(document, node->data.mapping.pairs.start[k].key);
				TraverseNode(document, node->data.mapping.pairs.start[k].value);
			}
            break;
        default:
            break;
	} 
}

 BOOLEAN IsPrintable(int c)
 {
	return c == 0x9 || c == 0xA || c == 0xD || (c >= 0x20 && c <= 0x7E) // 8
																		// bit
			|| c == 0x85 || (c >= 0xA0 && c <= 0xD7FF) || (c >= 0xE000 && c <= 0xFFFD) // 16
																						// bit
			|| (c >= 0x10000 && c <= 0x10FFFF); // 32 bit
}

BOOLEAN IsPrintableStr(CHAR16 *Str)
{
	UINTN Length;
	UINTN i;

	if (Str == NULL)
		return FALSE;

	Length = StrLen(Str);
	for (i = 0; i < Length; i++)
	{
		if (!IsPrintable(Str[i]))
			return FALSE;
	}
	return TRUE;
}

BOOLEAN ValidateBenchResults(MT_HANDLE FileHandle)
{
	EFI_STATUS		Status = EFI_SUCCESS;
	UINTN			Size = 256;
	BOOLEAN			Error = FALSE;
	BOOLEAN			Ascii;
	CHAR16 ReadLine[256];
  
	for (;!MtSupportEof(FileHandle);Size = 256) {
		CHAR16 *LinePtr = ReadLine;
		SetMem(ReadLine,Size,0);
		Status = MtSupportReadLine(FileHandle, ReadLine, &Size, TRUE, &Ascii);
		
		if (Status == EFI_BUFFER_TOO_SMALL) { // If line is greater than 256 characters, assume the results are invalid
			Error = TRUE;
			break;
		}
		if (EFI_ERROR(Status)) {
			Error = TRUE;
			break;
		}

		//
		// Ignore the pad spaces (space or tab) 
		//
		while ( ((UINTN)LinePtr < (UINTN)ReadLine + Size) || (*LinePtr == L' ') || (*LinePtr == L'\t')) {
		   LinePtr++;
	   }

	   // Line is nothing but spaces
	   if ((UINTN)LinePtr >= (UINTN)ReadLine + Size)
	   {
		   Error = TRUE;
		   break;
	   }

	   	// YAML only allows printable ASCII characters
		if (!IsPrintableStr(LinePtr))
		{
			Error = TRUE;
			break;
		}

#if 0
		if (StrnCmp(LinePtr, L"---", 3) == 0)
		{
			
		}
		else if (StrnCmp(LinePtr, L"...", 3) == 0)
		{
		}
		else if (StrnCmp(LinePtr, L"TestInfo:", 9) == 0)
		{
		}
		else if (StrnCmp(LinePtr, L"Version:", 8) == 0)
		{
		}
		else if (StrnCmp(LinePtr, L"MemTestHeader:", 14) == 0)
		{
		}
		else if (StrnCmp(LinePtr, L"Results:", 8) == 0) 
		{
		}
		else if (StrnCmp(LinePtr, L"Checksum:", 9) == 0)
		{
		}
		else { // UNKNOWN
		}
#endif
	}
	MtSupportSetPosition(FileHandle, 0);
	return Error;
}

BOOLEAN LoadBenchResults(MT_HANDLE FileHandle, TestResultsMetaInfo *TestInfo, MTVersion *VersionInfo, MEM_RESULTS *Header, UINT64 SampleX[], UINT64 SampleY[], int *NumSamples)
{
	yaml_parser_t parser;
	yaml_document_t document;
	yaml_node_t *rootnode;
	yaml_node_t *testinfonode;
	yaml_node_t *versionnode;
	yaml_node_t *headernode;
	yaml_node_t *resultsnode;
	yaml_node_t *chksumnode;

	TestResultsMetaInfo TempTestInfo;
	MTVersion TempVersionInfo;
	MEM_RESULTS TempHeader;
	UINT64 TempSampleX[32];
	UINT64 TempSampleY[32];
	int TempNumSamples = 0;

	CHAR8 StoredHash[HASH_STRING_SIZE];
	CHAR8 CalculatedHash[HASH_STRING_SIZE];

	UINT64	flKBPerSec			= 0;
	UINT64	StepSize			= 0;

	int k;

	SetMem(&TempTestInfo, sizeof(TempTestInfo), 0);
	SetMem(&TempVersionInfo, sizeof(TempVersionInfo), 0);
	SetMem(&TempHeader, sizeof(TempHeader), 0);
	SetMem(TempSampleX, sizeof(TempSampleX), 0);
	SetMem(TempSampleY, sizeof(TempSampleY), 0);

	// Do sanity check first
	if (!ValidateBenchResults(FileHandle))
	{
		AsciiFPrint(DEBUG_FILE_HANDLE, "Benchmark results file is invalid");
		return FALSE;
	}

	yaml_parser_initialize(&parser);

    yaml_parser_set_input_file(&parser, FileHandle);

  if (!yaml_parser_load(&parser, &document))
	  goto parser_error;

  rootnode = yaml_document_get_node(&document, 1);
  if (rootnode == NULL)
  {
	  AsciiFPrint(DEBUG_FILE_HANDLE, "Root node does not exist");
	  goto parser_error;
  }

  if (rootnode->type != YAML_MAPPING_NODE)
  {
	  AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid root node  type: %d", rootnode->type);
	  goto parser_error;
  }

  if ((rootnode->data.mapping.pairs.top - rootnode->data.mapping.pairs.start) < 5)
  {
	  AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid number of sections: %d", (rootnode->data.mapping.pairs.top - rootnode->data.mapping.pairs.start));
	  goto parser_error;
  }

  testinfonode = yaml_document_get_node(&document, rootnode->data.mapping.pairs.start[0].key);
  if (testinfonode == NULL)
  {
	  AsciiFPrint(DEBUG_FILE_HANDLE, "TestInfo node does not exist");
	  goto parser_error;
  }

  if (testinfonode->type != YAML_SCALAR_NODE || AsciiStrCmp((CHAR8*)testinfonode->data.scalar.value, "TestInfo") != 0)
  {
	  AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid section (%d, %a), expected \"TestInfo\"", testinfonode->type, testinfonode->type == YAML_SCALAR_NODE ? (CHAR8*)testinfonode->data.scalar.value : "");
	  goto parser_error;
  }

  if (!ExtractTestInfo(&document,yaml_document_get_node(&document, rootnode->data.mapping.pairs.start[0].value), &TempTestInfo ))
	  goto parser_error;

  versionnode = yaml_document_get_node(&document, rootnode->data.mapping.pairs.start[1].key);
  if (versionnode == NULL)
  {
	  AsciiFPrint(DEBUG_FILE_HANDLE, "Version node does not exist");
	  goto parser_error;
  }

  if (versionnode->type !=YAML_SCALAR_NODE || AsciiStrCmp((CHAR8*)versionnode->data.scalar.value, "Version") != 0)
  {
	  AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid section (%d, %a), expected \"Version\"", versionnode->type, versionnode->type == YAML_SCALAR_NODE ? (CHAR8*)versionnode->data.scalar.value : "");
	  goto parser_error;
  }
  
  if (!ExtractVersion(&document,yaml_document_get_node(&document, rootnode->data.mapping.pairs.start[1].value), &TempVersionInfo ))
	  goto parser_error;

  headernode = yaml_document_get_node(&document, rootnode->data.mapping.pairs.start[2].key);
  if (headernode == NULL)
  {
	  AsciiFPrint(DEBUG_FILE_HANDLE, "Header node does not exist");
	  goto parser_error;
  }

  if (headernode->type !=YAML_SCALAR_NODE || AsciiStrCmp((CHAR8*)headernode->data.scalar.value, "MemTestHeader") != 0)
  {
	  AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid section (%d, %a), expected \"MemTestHeader\"", headernode->type, headernode->type == YAML_SCALAR_NODE ? (CHAR8*)headernode->data.scalar.value : "");
	  goto parser_error;
  }

  if (!ExtractMemTestHeader(&document,yaml_document_get_node(&document, rootnode->data.mapping.pairs.start[2].value), &TempHeader ))
	  goto parser_error;
  
  resultsnode = yaml_document_get_node(&document, rootnode->data.mapping.pairs.start[3].key);
  if (resultsnode == NULL)
  {
	  AsciiFPrint(DEBUG_FILE_HANDLE, "Results node does not exist");
	  goto parser_error;
  }
  if (resultsnode->type !=YAML_SCALAR_NODE || AsciiStrCmp((CHAR8*)resultsnode->data.scalar.value, "Results") != 0)
  {
	  AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid section (%d, %a), expected \"Results\"", resultsnode->type, resultsnode->type == YAML_SCALAR_NODE ? (CHAR8*)resultsnode->data.scalar.value : "");
	  goto parser_error;
  }

	resultsnode = yaml_document_get_node(&document, rootnode->data.mapping.pairs.start[3].value);
	if (resultsnode == NULL)
	{
		AsciiFPrint(DEBUG_FILE_HANDLE, "Results node does not exist");
		goto parser_error;
	}

	if (resultsnode->type != YAML_SEQUENCE_NODE)
	{
		AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid results node type: %d", resultsnode->type);
		goto parser_error;
	}

	TempNumSamples = (int)(resultsnode->data.sequence.items.top - resultsnode->data.sequence.items.start);
	if (TempNumSamples > 32)
	{
		AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid number of samples: %d", TempNumSamples);
		goto parser_error;
	}

	for (k = 0; k < TempNumSamples; k ++)
	{
		yaml_node_t *samplenode;
		yaml_node_t *bandwidthnode;
		yaml_node_t *stepsizenode;

		samplenode = yaml_document_get_node(&document, resultsnode->data.sequence.items.start[k]);
		if (samplenode == NULL)
		{
			AsciiFPrint(DEBUG_FILE_HANDLE, "Sample node does not exist");
			goto parser_error;
		}

		if (samplenode->type != YAML_SEQUENCE_NODE)
		{
			AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid sample node type: %d", samplenode->type);
			goto parser_error;
		}

		if ( (samplenode->data.sequence.items.top - samplenode->data.sequence.items.start) != 2 )
		{
			AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid sample node item count: %d", (samplenode->data.sequence.items.top - samplenode->data.sequence.items.start));
			goto parser_error;
		}

		bandwidthnode = yaml_document_get_node(&document, samplenode->data.sequence.items.start[0]);
		if (bandwidthnode == NULL)
		{
			AsciiFPrint(DEBUG_FILE_HANDLE, "Bandwidth node does not exist");
			goto parser_error;
		}

		if (bandwidthnode->data.scalar.length <= 32)
			flKBPerSec = AsciiStrDecimalToUint64((CHAR8 *)bandwidthnode->data.scalar.value );

		stepsizenode = yaml_document_get_node(&document, samplenode->data.sequence.items.start[1]);
		if (stepsizenode == NULL)
		{
			AsciiFPrint(DEBUG_FILE_HANDLE, "Step Size node does not exist");
			goto parser_error;
		}

		if (stepsizenode->data.scalar.length <= 32)
			StepSize = AsciiStrDecimalToUintn((CHAR8*)stepsizenode->data.scalar.value );
	  
		TempSampleX[k] = StepSize;

		TempSampleY[k] = flKBPerSec;
	}
	
  chksumnode = yaml_document_get_node(&document, rootnode->data.mapping.pairs.start[4].key);
  if (chksumnode == NULL)
  {
	  AsciiFPrint(DEBUG_FILE_HANDLE, "Checksum node does not exist");
	  goto parser_error;
  }

  if (chksumnode->type !=YAML_SCALAR_NODE || AsciiStrCmp((CHAR8*)chksumnode->data.scalar.value, "Checksum") != 0)
  {
	  AsciiFPrint(DEBUG_FILE_HANDLE, "Invalid section (%d, %a), expected \"Checksum\"", chksumnode->type, chksumnode->type == YAML_SCALAR_NODE ? (CHAR8*)chksumnode->data.scalar.value : "");
	  goto parser_error;
  }
  
  if (!ExtractChecksum(&document,yaml_document_get_node(&document, rootnode->data.mapping.pairs.start[4].value), StoredHash ))
	  goto parser_error;

  CalculateHash( &TempTestInfo, &TempHeader, TempSampleX, TempSampleY, TempNumSamples, CalculatedHash);

  if (AsciiStrCmp(StoredHash, CalculatedHash) != 0)
  {
	  AsciiFPrint(DEBUG_FILE_HANDLE, "Failed integrity check");
	  goto parser_error;
  }

  yaml_document_delete(&document);
  yaml_parser_delete(&parser);
  
  if (TestInfo)
	  CopyMem(TestInfo, &TempTestInfo, sizeof(TempTestInfo));

  if  (VersionInfo)
	  CopyMem(VersionInfo, &TempVersionInfo, sizeof(TempVersionInfo));

  if (Header)
	  CopyMem(Header, &TempHeader, sizeof(TempHeader));
  
  if (NumSamples)
  {
	  if (*NumSamples >= TempNumSamples)
	  {
		  if (SampleX)
			  CopyMem(SampleX, TempSampleX, TempNumSamples * sizeof(TempSampleX[0]));

		  if (SampleY)
			  CopyMem(SampleY, TempSampleY, TempNumSamples * sizeof(TempSampleY[0]));
	  }
	  *NumSamples = TempNumSamples;
  }

  return TRUE;

parser_error:

    /* Display a parser error message. */
    switch (parser.error)
    {
        case YAML_MEMORY_ERROR:
            AsciiFPrint(DEBUG_FILE_HANDLE, "Memory error: Not enough memory for parsing\n");
            break;

        case YAML_READER_ERROR:
            if (parser.problem_value != -1) {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Reader error: %a: #%X at %d\n", parser.problem,
                        parser.problem_value, parser.problem_offset);
            }
            else {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Reader error: %a at %d\n", parser.problem,
                        parser.problem_offset);
            }
            break;

        case YAML_SCANNER_ERROR:
            if (parser.context) {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Scanner error: %a at line %d, column %d\n"
                        "%s at line %d, column %d\n", parser.context,
                        parser.context_mark.line+1, parser.context_mark.column+1,
                        parser.problem, parser.problem_mark.line+1,
                        parser.problem_mark.column+1);
            }
            else {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Scanner error: %a at line %d, column %d\n",
                        parser.problem, parser.problem_mark.line+1,
                        parser.problem_mark.column+1);
            }
            break;

        case YAML_PARSER_ERROR:
            if (parser.context) {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Parser error: %a at line %d, column %d\n"
                        "%s at line %d, column %d\n", parser.context,
                        parser.context_mark.line+1, parser.context_mark.column+1,
                        parser.problem, parser.problem_mark.line+1,
                        parser.problem_mark.column+1);
            }
            else {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Parser error: %a at line %d, column %d\n",
                        parser.problem, parser.problem_mark.line+1,
                        parser.problem_mark.column+1);
            }
            break;

        default:
            break;
    }

	yaml_document_delete(&document);
    // yaml_event_delete(&event);
    yaml_parser_delete(&parser);

    return FALSE;
}

BOOLEAN EmitBeginStream(yaml_emitter_t *emitter)
{
        yaml_event_t event;
        yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
        if (!yaml_emitter_emit(emitter, &event))
                return FALSE;
        return TRUE;
}


BOOLEAN EmitEndStream(yaml_emitter_t *emitter)
{
        yaml_event_t event;
        yaml_stream_end_event_initialize(&event);
        if (!yaml_emitter_emit(emitter, &event))
                return FALSE;
        return TRUE;
}


BOOLEAN EmitBeginDoc(yaml_emitter_t *emitter)
{
        yaml_event_t event;
        yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 0);
        if (!yaml_emitter_emit(emitter, &event))
                return FALSE;
        return TRUE;
}


BOOLEAN EmitEndDoc(yaml_emitter_t *emitter)
{
        yaml_event_t event;
        yaml_document_end_event_initialize(&event, 0);
        if (!yaml_emitter_emit(emitter, &event))
                return FALSE;
        return TRUE;
}


BOOLEAN EmitBeginMap(yaml_emitter_t *emitter)
{
        yaml_event_t event;
        yaml_mapping_start_event_initialize(&event, NULL, NULL, 0, YAML_BLOCK_MAPPING_STYLE);
        if (!yaml_emitter_emit(emitter, &event))
                return FALSE;
        return TRUE;
}


BOOLEAN EmitEndMap(yaml_emitter_t *emitter)
{
        yaml_event_t event;
        yaml_mapping_end_event_initialize(&event);
        if (!yaml_emitter_emit(emitter, &event))
                return FALSE;
        return TRUE;
}


BOOLEAN EmitMapKeyAsciiVal(yaml_emitter_t *emitter, const char* pKey, const char* pVal)
{
        yaml_event_t event;
        yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)pKey, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
        if (!yaml_emitter_emit(emitter, &event))
                return FALSE;
        if (pVal != NULL)
        {
                yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)pVal, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
                if (!yaml_emitter_emit(emitter, &event))
                        return FALSE;
        }
        return TRUE;
}

BOOLEAN EmitMapKeyUnicodeVal(yaml_emitter_t *emitter, const char* pKey, CHAR16* pVal)
{
        yaml_event_t event;
		CHAR8 ValBuf[256];
		AsciiSPrint(ValBuf, sizeof(ValBuf), "%s", pVal);

        yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)pKey, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
        if (!yaml_emitter_emit(emitter, &event))
                return FALSE;
        if (pVal != NULL)
        {
                yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)ValBuf, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
                if (!yaml_emitter_emit(emitter, &event))
                        return FALSE;
        }
        return TRUE;
}

BOOLEAN EmitMapKeyIntVal(yaml_emitter_t *emitter, const char* pKey, INT64 val)
{
	yaml_event_t event;
	CHAR8 ValBuf[32];
	AsciiSPrint(ValBuf, sizeof(ValBuf), "%d", val);

    yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)pKey, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
            return FALSE;

	yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)ValBuf, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
			return FALSE;

    return TRUE;
}

BOOLEAN EmitMapKeyInt64Val(yaml_emitter_t *emitter, const char* pKey, INT64 val)
{
	yaml_event_t event;
	CHAR8 ValBuf[32];
	AsciiSPrint(ValBuf, sizeof(ValBuf), "%ld", val);

    yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)pKey, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
            return FALSE;

	yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)ValBuf, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
			return FALSE;

    return TRUE;
}

BOOLEAN EmitMapKeyBoolVal(yaml_emitter_t *emitter, const char* pKey, BOOLEAN Val)
{
	yaml_event_t event;

    yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)pKey, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
            return FALSE;

	yaml_scalar_event_initialize(&event, NULL, NULL, Val == FALSE ? (yaml_char_t *)"false" : (yaml_char_t *)"true", -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
			return FALSE;

    return TRUE;
}

BOOLEAN EmitMapKeyTimeVal(yaml_emitter_t *emitter, const char* pKey, EFI_TIME *pVal)
{
	yaml_event_t event;
	CHAR8 ValBuf[32];
	AsciiSPrint(ValBuf, sizeof(ValBuf), "%04d%02d%02d%02d%02d%02d", pVal->Year, pVal->Month, pVal->Day, pVal->Hour, pVal->Minute, pVal->Second);

    yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)pKey, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
            return FALSE;

	yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)ValBuf, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
			return FALSE;

    return TRUE;
}



BOOLEAN EmitBeginBlockSeq(yaml_emitter_t *emitter)
{
        yaml_event_t event;
        yaml_sequence_start_event_initialize(&event, NULL, NULL, 0, YAML_BLOCK_SEQUENCE_STYLE);
        if (!yaml_emitter_emit(emitter, &event))
                return FALSE;
        return TRUE;
}

BOOLEAN EmitBeginFlowSeq(yaml_emitter_t *emitter)
{
    yaml_event_t event;
    yaml_sequence_start_event_initialize(&event, NULL, NULL, 0, YAML_FLOW_SEQUENCE_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
            return FALSE;
    return TRUE;
}


BOOLEAN EmitEndSeq(yaml_emitter_t *emitter)
{
    yaml_event_t event;
    yaml_sequence_end_event_initialize(&event);
    if (!yaml_emitter_emit(emitter, &event))
            return FALSE;
    return TRUE;
}

BOOLEAN EmitResultsCommon(yaml_emitter_t *emitter, TestResultsMetaInfo *resultsInfo)
{
	EmitMapKeyAsciiVal(emitter, "TestInfo", NULL);
	EmitBeginMap(emitter);
	EmitMapKeyIntVal(emitter, "Type", resultsInfo->iTestType);
	EmitMapKeyInt64Val(emitter, "StartTime", resultsInfo->i64TestStartTime);
	EmitMapKeyIntVal(emitter, "NumCPU", resultsInfo->uiNumCPU);
	EmitMapKeyUnicodeVal(emitter, "CPUType", resultsInfo->szCPUType);
	EmitMapKeyUnicodeVal(emitter, "CPUSpeed", resultsInfo->szCPUSpeed);
	EmitEndMap(emitter);

	EmitMapKeyAsciiVal(emitter, "Version", NULL);
	EmitBeginMap(emitter);
	EmitMapKeyIntVal(emitter, "Major", PROGRAM_VERSION_MAJOR);
	EmitMapKeyIntVal(emitter, "Minor", PROGRAM_VERSION_MINOR);
	EmitMapKeyIntVal(emitter, "Build", PROGRAM_VERSION_BUILD);
#ifdef MDE_CPU_X64
	EmitMapKeyIntVal(emitter, "Architecture", 64);
#elif defined(MDE_CPU_IA32)
	EmitMapKeyIntVal(emitter, "Architecture", 32);
#endif	
	EmitEndMap(emitter);

	return TRUE;
}
void EmitMemHeader(yaml_emitter_t *emitter, MEM_RESULTS *header)
{
	EmitMapKeyAsciiVal(emitter, "MemTestHeader", NULL);
	EmitBeginMap(emitter);
 
	EmitMapKeyInt64Val(emitter, "TotalPhys", header->TotalPhys);
	EmitMapKeyInt64Val(emitter, "TestMode", header->iTestMode);
	EmitMapKeyInt64Val(emitter, "AveSpeed", header->fAveSpeed);

	EmitMapKeyUnicodeVal(emitter, "ModuleManuf", header->szModuleManuf);
	EmitMapKeyUnicodeVal(emitter, "ModulePartNo", header->szModulePartNo);

	EmitMapKeyUnicodeVal(emitter, "RAMType", header->szRAMType);
	EmitMapKeyUnicodeVal(emitter, "RAMTiming", header->szRAMTiming);

	EmitMapKeyBoolVal(emitter, "IsRead", header->bRead);
	// Step size data
	EmitMapKeyInt64Val(emitter, "ArraySize", header->ArraySize);
	// Block size data
	EmitMapKeyInt64Val(emitter, "MinBlock", header->MinBlock);
	EmitMapKeyInt64Val(emitter, "MaxBlock", header->MaxBlock);
	EmitMapKeyInt64Val(emitter, "AccessDataSize", header->dwAccessDataSize);

	EmitEndMap(emitter);
}

void EmitMemResults(yaml_emitter_t *emitter, UINT64 SampleX[], UINT64 SampleY[], int NumSamples)
{
	int i;
	CHAR8 TextBuf[32];

	EmitMapKeyAsciiVal(emitter, "Results", NULL);
	EmitBeginBlockSeq(emitter);
	for(i=0;i<NumSamples;i++)
	{
		EmitBeginFlowSeq(emitter);
		AsciiSPrint(TextBuf, sizeof(TextBuf), "%ld", SampleY[i]);
		EmitMapKeyAsciiVal(emitter, TextBuf, NULL);
		AsciiSPrint(TextBuf, sizeof(TextBuf), "%ld", SampleX[i]);
		EmitMapKeyAsciiVal(emitter, TextBuf, NULL);
		EmitEndSeq(emitter);
	}
	EmitEndSeq(emitter);
}

void EmitHash(yaml_emitter_t *emitter, CHAR8 Hash[HASH_STRING_SIZE])
{
	EmitMapKeyAsciiVal(emitter, "Checksum", Hash);
}

BOOLEAN SaveBenchResults(MT_HANDLE FileHandle, TestResultsMetaInfo *TestInfo, MEM_RESULTS *Header, UINT64 SampleX[], UINT64 SampleY[], int NumSamples)
{
	BOOLEAN	bRet = TRUE;
	CHAR8 Hash[HASH_STRING_SIZE];
    yaml_emitter_t emitter;

    /* Clear the objects. */

    SetMem(&emitter, sizeof(emitter), 0);

	/* Initialize the emitter objects. */
    if (!yaml_emitter_initialize(&emitter)) {
        return FALSE;
    }

    /* Set the emitter parameters. */

    yaml_emitter_set_output_file(&emitter, FileHandle);

	yaml_emitter_set_canonical(&emitter,0);

	EmitBeginStream(&emitter);
	EmitBeginDoc(&emitter);

	EmitBeginMap(&emitter);
	EmitResultsCommon(&emitter, TestInfo);
	EmitMemHeader(&emitter, Header);
	EmitMemResults(&emitter, SampleX, SampleY, NumSamples);
	CalculateHash(TestInfo, Header, SampleX, SampleY, NumSamples, Hash);
	EmitHash(&emitter, Hash);
	EmitEndMap(&emitter);

	EmitEndDoc(&emitter);
	EmitEndStream(&emitter);

    yaml_emitter_delete(&emitter);

	return bRet;
}

VOID CalculateHash(TestResultsMetaInfo *TestInfo, MEM_RESULTS *Header, UINT64 SampleX[], UINT64 SampleY[], int NumSamples, CHAR8 Hash[HASH_STRING_SIZE])
{
	SHA256_CTX	ctx256;
	int i;

	SetMem(Hash, HASH_STRING_SIZE, 0);

	SHA256_Init(&ctx256);

	SHA256_Update(&ctx256, (unsigned char *)&TestInfo->iTestType, sizeof(TestInfo->iTestType));
	SHA256_Update(&ctx256, (unsigned char *)&TestInfo->i64TestStartTime, sizeof(TestInfo->i64TestStartTime));
	SHA256_Update(&ctx256, (unsigned char *)&TestInfo->uiNumCPU, sizeof(TestInfo->uiNumCPU));
	SHA256_Update(&ctx256, (unsigned char *)TestInfo->szCPUType, StrLen(TestInfo->szCPUType) * sizeof(CHAR16));
	SHA256_Update(&ctx256, (unsigned char *)TestInfo->szCPUSpeed, StrLen(TestInfo->szCPUSpeed) * sizeof(CHAR16));

	SHA256_Update(&ctx256, (unsigned char *)&Header->TotalPhys, sizeof(Header->TotalPhys));
	SHA256_Update(&ctx256, (unsigned char *)&Header->iTestMode, sizeof(Header->iTestMode));
	SHA256_Update(&ctx256, (unsigned char *)&Header->fAveSpeed, sizeof(Header->fAveSpeed));
	SHA256_Update(&ctx256, (unsigned char *)Header->szModuleManuf, StrLen(Header->szModuleManuf) * sizeof(CHAR16));
	SHA256_Update(&ctx256, (unsigned char *)Header->szModulePartNo, StrLen(Header->szModulePartNo) * sizeof(CHAR16));
	SHA256_Update(&ctx256, (unsigned char *)Header->szRAMType, StrLen(Header->szRAMType) * sizeof(CHAR16));
	SHA256_Update(&ctx256, (unsigned char *)Header->szRAMTiming, StrLen(Header->szRAMTiming) * sizeof(CHAR16));
	SHA256_Update(&ctx256, (unsigned char *)&Header->bRead, sizeof(Header->bRead));
	SHA256_Update(&ctx256, (unsigned char *)&Header->ArraySize, sizeof(Header->ArraySize));
	SHA256_Update(&ctx256, (unsigned char *)&Header->MinBlock, sizeof(Header->MinBlock));
	SHA256_Update(&ctx256, (unsigned char *)&Header->MaxBlock, sizeof(Header->MaxBlock));
	SHA256_Update(&ctx256, (unsigned char *)&Header->dwAccessDataSize, sizeof(Header->dwAccessDataSize));

	for (i = 0; i < NumSamples; i++)
	{
		SHA256_Update(&ctx256, (unsigned char *)&SampleX[i], sizeof(SampleX[i]));
		SHA256_Update(&ctx256, (unsigned char *)&SampleY[i], sizeof(SampleY[i]));
	}

	SHA256_Update(&ctx256, (unsigned char *)HASH_SALT, AsciiStrLen(HASH_SALT));

	SHA256_End(&ctx256, Hash);
#if 0
	AsciiFPrint(DEBUG_FILE_HANDLE, "\n"
									"TestInfo:\n"
									" iTestType = %d\n"
									" i64TestStartTime = %ld\n"
									" szComputerName = %s\n"
									" uiNumCPU = %d\n"
									" szCPUType = %s\n"
									" szCPUSpeed = %s\n",
									TestInfo->iTestType,
									TestInfo->i64TestStartTime,
									TestInfo->szComputerName,
									TestInfo->uiNumCPU,
									TestInfo->szCPUType,
									TestInfo->szCPUSpeed);

	AsciiFPrint(DEBUG_FILE_HANDLE, "\n"
									"MemTestHeader:\n"
									" TotalPhys = %ld\n"
									" iTestMode = %d\n"
									" fAveSpeed = %ld\n"
									" fAveCPULoad = %ld\n"
									" szModuleManuf = %s\n"
									" szModulePartNo = %s\n"
									" szRAMType = %s\n"
									" szRAMTiming = %s\n"
									" bRead = %s\n"
									" ArraySize = %ld\n"
									" MinBlock = %ld\n"
									" MaxBlock = %ld\n"
									" dwAccessDataSize = %d\n",
									Header->TotalPhys,
									Header->iTestMode,
									Header->fAveSpeed,
									Header->fAveCPULoad,
									Header->szModuleManuf,
									Header->szModulePartNo,
									Header->szRAMType,
									Header->szRAMTiming,
									Header->bRead ? L"true" : L"false",
									(UINT64)Header->ArraySize,
									(UINT64)Header->MinBlock,
									(UINT64)Header->MaxBlock,
									Header->dwAccessDataSize);
#endif
}

BOOLEAN TimestampToEFITime(UINT64 Timestamp, EFI_TIME *EFITime)
{
	UINT64 Quotient = 0;

	if (EFITime == NULL)
		return FALSE;

	SetMem(EFITime, sizeof(*EFITime), 0);

	Quotient = DivU64x64Remainder(Timestamp, 10000000000ull, NULL);
	EFITime->Year = (UINT16)Quotient;
	Timestamp -= MultU64x32(10000000000ull,EFITime->Year);

	Quotient = DivU64x32(Timestamp, 100000000);
	EFITime->Month = (UINT8)Quotient;
	Timestamp -= EFITime->Month * 100000000;

	Quotient = DivU64x32(Timestamp, 1000000);
	EFITime->Day = (UINT8)Quotient;
	Timestamp -= EFITime->Day * 1000000;

	Quotient = DivU64x32(Timestamp, 10000);
	EFITime->Hour = (UINT8)Quotient;
	Timestamp -= EFITime->Hour * 10000;

	Quotient = DivU64x32(Timestamp, 100);
	EFITime->Minute = (UINT8)Quotient;
	Timestamp -= EFITime->Minute * 100;

	EFITime->Second = (UINT8)Timestamp;

	return TRUE;
}