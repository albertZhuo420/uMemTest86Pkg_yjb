// PassMark SysInfo
//
// Copyright (c) 2008-2017
//	This software is the confidential and proprietary information of PassMark
//	Software Pty. Ltd. ("Confidential Information").  You shall not disclose
//	such Confidential Information and shall use it only in accordance with
//	the terms of the license agreement you entered into with PassMark
//	Software.
//
// Program:
//	SysInfo
//
// Module:
//	cpuinfo_specifications.h
//
// Author(s):
//	Ian Robinson
//
// Description:
//   The data for the CPU specifications, including Intel TJunction values for Passmark temperature monitoring
//
//	REQUIRES UPDATING FOR NEW CPUS
//
// History
//   15-Sep-2008 Module created
//   21-Aug-2009 Updated
//  Last date updated: 22 April 2015 IR
//  TO DO - Carrizo
//

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PROCESS TO UPDATE THIS FILE
//
//	(1) Run: http://www.passmark.com/baselines/admin_cputype_strings.php?date_from=03+October+2011&date_to=&list= with the last date as the start date
//			misses some, use SELECT distinct name,family,model,released FROM passmark_baselines8.cpuDetails order by family,model limit 10000;
//
// (2) For each new CPU udapte using the follwoing reference sites
//
//		Google:
//			keene site:http://valid.canardpc.com
//			"F.23.C" -intel site:http://valid.canardpc.com
//		http://en.wikipedia.org/wiki/CPU_socket#Intel_Sockets
//		http://en.wikipedia.org/wiki/Intel_Xeon
//		http://en.wikipedia.org/wiki/Opteron
//		http://ark.intel.com/ProductCollection.aspx?familyId=594
//		http://www.amd.com/us-en/Processors/ProductInformation/0,,30_118_8796_15226,00.html
//		http://www.amd.com/us-en/Processors/ProductInformation/0,,30_118_8796_14266,00.html
//		http://www.amd.com/us/products/desktop/processors/athlon-ii-x2/Pages/AMD-athlon-ii-x2-processor-model-numbers-feature-comparison.aspx
//		http://en.wikipedia.org/wiki/List_of_AMD_Athlon_X2_microprocessors
//		http://geekinfo.googlecode.com/svn-history/r50/branches/2.0/x86processor.rb
//		http://ark.intel.com/cpu.aspx?groupId=34739
//		http://www.tomshardware.com/news/intel-dts-specs,6517.html
//		http://www.chiplist.com/Core_2/
//		http://en.wikipedia.org/wiki/List_of_Intel_Core_2_microprocessors#.22Merom.22_.28low-voltage.2C_65_nm.29
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include <tchar.h>
#else
#define _T(x) L##x
#define TCHAR CHAR16
#endif

enum eCPUCodename
{
	CPU_BLANK = 0,
	CPU_CONROE,
	CPU_MEROM,
	CPU_KENTSFIELD,
	CPU_WOODCREST,
	CPU_CLOVERTOWN,
	CPU_TIGERTON,
	CPU_TIGERTON_LV,
	CPU_WOLFDALE,
	CPU_PENRYN,
	CPU_YORKFIELD,
	CPU_HARPERTOWN,
	CPU_BLOOMFIELD,
	CPU_GAINESTOWN,
	CPU_DIAMONDVILLE,
	CPU_SILVERTHORNE,
	CPU_DUNNINGTON,
	CPU_LYNNFIELD,
	CPU_GULFTOWN,
	CPU_CLAWHAMMER,
	CPU_SLEDGEHAMMER,
	CPU_NEWCASTLE,
	CPU_PARIS,
	CPU_OAKVILLE,
	CPU_SONORA,
	CPU_PALERMO,
	CPU_WINCHESTER,
	CPU_ITALY,
	CPU_TOLEDO,
	CPU_DENMARK,
	CPU_LANCASTER,
	CPU_NEWARK,
	CPU_TROY,
	CPU_SAN_DIEGO,
	CPU_VENUS,
	CPU_MANCHESTER,
	CPU_ALBANY,
	CPU_VENICE,
	CPU_SANTA_ROSA,
	CPU_WINDSOR,
	CPU_SANTA_ANA,
	CPU_RICHMOND,
	CPU_KEENE,
	CPU_ORLEANS,
	CPU_MANILA,
	CPU_TYLER,
	CPU_BRISBANE,
	CPU_SHERMAN,
	CPU_HURON,
	CPU_LIMA,
	CPU_SPARTA,
	CPU_BUDAPEST,
	CPU_BARCELONA,
	CPU_AGENA,
	CPU_TOLIMAN,
	CPU_KUMA,
	CPU_CALLISTO,
	CPU_HEKA,
	CPU_DENEB,
	CPU_SHANGHAI,
	CPU_GRIFFIN,
	CPU_SABLE,
	CPU_REGOR,
	CPU_SARGAS,
	CPU_ISTANBUL,
	CPU_MAGNY_COURS,
	CPU_YONAH,
	CPU_RANA,
	CPU_PROPUS,
	CPU_CLARKDALE,
	CPU_ARRANDALE,
	CPU_PINEVIEW,
	CPU_CASPIAN,
	CPU_CLARKSFIELD,
	CPU_WESTMERE_EP,
	CPU_THUBAN,
	CPU_SANDYBRIDGE,
	CPU_DESNA,
	CPU_ONTARIO,
	CPU_ZACATE,
	CPU_LLANO,
	CPU_LYNX,
	CPU_SABINE,
	CPU_BOBCAT,
	CPU_BULLDOZER,
	CPU_VALENCIA,
	CPU_INTERLAGOS,
	CPU_SANDY_BRIDGE_E,
	CPU_CHAMPLAIN,
	CPU_GENEVA,
	CPU_WESTMERE_EX,
	CPU_CEDAR_TRAIL,
	CPU_IVYBRIDGE,
	CPU_TRINITY,
	CPU_LISBON,
	CPU_ZURICH,
	CPU_CEDARVIEW,
	CPU_TUNNEL_CREEK,
	CPU_LINCROFT,
	CPU_HASWELL,
	CPU_CLOVERVIEW,
	CPU_RICHLAND,
	CPU_KABINI,
	CPU_TEMASH,
	CPU_DELHI,
	CPU_SEOUL,
	CPU_ABU_DHABI,
	CPU_KYOTO,
	CPU_BAY_TRAIL_D,
	CPU_BAY_TRAIL_M,
	CPU_BAY_TRAIL_T,
	CPU_IVYBRIDGE_EP,
	CPU_VISHERA,
	CPU_HONDO,
	CPU_CRYSTAL_WELL,
	CPU_BAY_TRAIL,
	CPU_CENTERTON,
	CPU_KAVERI,
	CPU_AVOTON,
	CPU_NEHALEM_EX,
	CPU_BROADWELL,
	CPU_BEEMA,
	CPU_MULLINS,
	CPU_STEPPE_EAGLE,
	CPU_BALD_EAGLE,
	CPU_HASWELL_E,
	CPU_RANGELEY,
	CPU_CHERRYTRAIL,
	CPU_BRASWELL,
	CPU_KNIGHTSLANDING,
	CPU_SKYLAKE,
	CPU_CARRIZO,
	CPU_CARRIZO_L,
	CPU_MERLIN_FALCON,
	CPU_BROADWELL_E,
	CPU_APOLLO_LAKE,
	CPU_GODAVARI,
	CPU_STONEY_RIDGE,
	CPU_CARRIZO_DDR4,
	CPU_BRISTOL_RIDGE,
	CPU_RYZEN,
	CPU_COFFEE_LAKE,
	CPU_KABY_LAKE,
	CPU_COMET_LAKE,
	CPU_ICE_LAKE,
	CPU_TIGER_LAKE,
	CPU_ZEN3_V,
	CPU_ZEN3_C,
	CPU_ALDER_LAKE,
	CPU_RAPTOR_LAKE,
	CPU_ZEN4_R,
	CPU_CODENAME_MARKER_END
};

#if defined(MAIN_MODULE_CPU)
TCHAR *g_szCPUCodename[] = {
	_T(""),
	_T("Conroe"),
	_T("Merom"),
	_T("Kentsfield"),
	_T("Woodcrest"),
	_T("Clovertown"),
	_T("Tigerton"),
	_T("Tigerton LV"),
	_T("Wolfdale"),
	_T("Penryn"),
	_T("Yorkfield"),
	_T("Harpertown"),
	_T("Bloomfield"),
	_T("Gainestown"),
	_T("Diamondville"),
	_T("Silverthorne"),
	_T("Dunnington"),
	_T("Lynnfield"),
	_T("Gulftown"),
	_T("ClawHammer"),
	_T("Sledgehammer"),
	_T("Newcastle"),
	_T("Paris"),
	_T("Oakville"),
	_T("Sonora"),
	_T("Palermo"),
	_T("Winchester"),
	_T("Italy"),
	_T("Toledo"),
	_T("Denmark"),
	_T("Lancaster"),
	_T("Newark"),
	_T("Troy"),
	_T("San Diego"),
	_T("Venus"),
	_T("Manchester"),
	_T("Albany"),
	_T("Venice"),
	_T("Santa Rosa"),
	_T("Windsor"),
	_T("Santa Ana"),
	_T("Richmond"),
	_T("Keene"),
	_T("Orleans"),
	_T("Manila"),
	_T("Tyler"),
	_T("Brisbane"),
	_T("Sherman"),
	_T("Huron"),
	_T("Lima"),
	_T("Sparta"),
	_T("Budapest"),
	_T("Barcelona"),
	_T("Agena"),
	_T("Toliman"),
	_T("Kuma"),
	_T("Callisto"),
	_T("Heka"),
	_T("Deneb"),
	_T("Shanghai"),
	_T("Griffin"),
	_T("Sable"),
	_T("Regor"),
	_T("Sargas"),
	_T("Istanbul"),
	_T("Magny-Cours"),
	_T("Yonah"),
	_T("Rana"),
	_T("Propus"),
	_T("Clarkdale"),
	_T("Arrandale"),	  // BIT6010190001
	_T("Pineview"),		  // BIT6010200001
	_T("Caspian"),		  // BIT6010200001
	_T("Clarksfield"),	  // BIT6010250003
	_T("Westmere-EP"),	  // BIT6010250003
	_T("Thuban"),		  // BIT6010250003
	_T("Sandy Bridge"),	  // BIT6010280000
	_T("Desna"),		  // SI101014
	_T("Ontario"),		  // SI101014
	_T("Zacate"),		  // SI101014
	_T("Llano"),		  // SI101014
	_T("Lynx"),			  // SI101014
	_T("Sabine"),		  // SI101014
	_T("Bobcat"),		  // SI101021
	_T("Bulldozer"),	  // SI101021
	_T("Valencia"),		  // SI101021
	_T("Interlagos"),	  // SI101021
	_T("Sandy Bridge-E"), // SI101021
	_T("Champlain"),	  // SI101022d
	_T("Geneva"),		  // SI101022d
	_T("Westmere-EX"),	  // SI101022d
	_T("Cedar Trail"),	  // SI101022d
	_T("Ivy Bridge"),	  // SI101023
	_T("Trinity"),		  // SI101029
	_T("Lisbon"),		  // SI101029
	_T("Zurich"),		  // SI101029
	_T("Cedarview"),	  // SI101029
	_T("Tunnel Creek"),	  // SI101029
	_T("Lincroft"),		  // SI101029
	_T("Haswell"),		  // SI101031
	_T("Cloverview"),	  // SI101036
	_T("Richland"),		  // SI101036
	_T("Kabini"),		  // SI101036
	_T("Temash"),		  // SI101036
	_T("Delhi"),		  // SI101036
	_T("Seoul"),		  // SI101036
	_T("Abu Dhabi"),	  // SI101036
	_T("Kyoto"),		  // SI101036
	_T("Bay Trail-D"),	  // SI101037
	_T("Bay Trail-M"),	  // SI101037
	_T("Bay Trail-T"),	  // SI101037
	_T("Ivy Bridge-EP"),  // SI101039
	_T("Vishera"),		  // SI101047
	_T("Hondo"),		  // SI101047
	_T("Crystal Well"),	  // SI101047
	_T("Bay Trail"),	  // SI101047
	_T("Centerton"),	  // SI101047
	_T("Kaveri"),		  // SI101049
	_T("Avoton"),		  // SI101066
	_T("Nehalem EX"),	  // SI101066
	_T("Broadwell"),	  // SI101073
	_T("Beema"),		  // SI101075
	_T("Mullins"),		  // SI101075
	_T("Steppe Eagle"),	  // SI101075
	_T("Bald Eagle"),
	_T("Haswell-E"),	   // SI101075
	_T("Rangeley"),		   // SI101098
	_T("Cherry Trail"),	   // SI101100
	_T("Braswell"),		   // SI101100
	_T("Knights Landing"), // SI101100
	_T("Skylake"),		   // SI101106
	_T("Carrizo"),		   // SI101109
	_T("Carrizo-L"),	   // SI101109
	_T("Merlin Falcon"),
	_T("Broadwell-E"),
	_T("Apollo Lake"),
	_T("Godavari"),
	_T("Stoney Ridge"),
	_T("Carrizo"), // Request by AMD to use "Carrizo" for Carrizo DDR4 cores
	_T("Carrizo"), // Request by AMD to use "Carrizo" for Bristol Ridge cores
	_T("Ryzen"),
	_T("Coffee Lake"),
	_T("Kaby Lake"),
	_T("Comet Lake"),
	_T("Ice Lake"),
	_T("Tiger Lake"),
	_T("Zen 3 (Vermeer)"),
	_T("Zen 3 (Cezanne)"),
	_T("Alder Lake"),
	_T("Raptor Lake"),
	_T("Zen 4 (Raphael)")};
#else
extern TCHAR *g_szCPUCodename[];
#endif

enum eCPUSocket
{
	CPU_S_BLANK = 0,
	CPU_S_754,
	CPU_S_939,
	CPU_S_940,
	CPU_S_949,
	CPU_S_AM2_940_PIN,
	CPU_S_AM2,
	CPU_S_AM2_PLUS_940_PIN,
	CPU_S_AM3_938_PIN,
	CPU_S_AM3,
	CPU_S_ASB1,
	CPU_S_BGA956,
	CPU_S_F_1207,
	CPU_S_FCBGA6,
	CPU_S_LGA1156,
	CPU_S_LGA1366,
	CPU_S_LGA771,
	CPU_S_LGA775,
	CPU_S_PBGA437,
	CPU_S_PBGA441,
	CPU_S_PGA478_BGA479,
	CPU_S_PGA604,
	CPU_S_PPGA478,
	CPU_S_PPGA478_PPGA479,
	CPU_S_PPGA479,
	CPU_S_S1_638,
	CPU_S_S1,
	CPU_S_SOCKET_G34,
	CPU_S_SOCKET_M_478,
	CPU_S_SOCKET_M,
	CPU_S_SOCKET_P_478,
	CPU_S_SOCKET_P,
	CPU_S_PBGA479,
	CPU_S_PLGA775,
	CPU_S_604,
	CPU_S_AM2_PLUS_938_PIN,
	CPU_BGA1288,
	CPU_PGA988,
	CPU_BGA1288_PGA988,
	CPU_FCBGA559,
	CPU_S_S1G3,
	CPU_S_989,
	CPU_S_LGA1155, // BIT6010280000
	CPU_S_FCLGA1155,
	CPU_S_FCBGA1224_FCPGA988,
	CPU_S_FCPGA988,
	CPU_S_FT1,
	CPU_S_FM1,
	CPU_S_FS1,
	CPU_S_AM3PLUS,
	CPU_S_C32,
	CPU_S_LGA2011,
	CPU_S_LGA1356,
	CPU_S_BGA1288,
	CPU_S_PGA988,
	CPU_S_S1G4,
	CPU_S_ASB2,
	CPU_S_LGA1567,
	CPU_S_SOCKET_G2,
	CPU_S_BGA_1023,
	CPU_S_RPGA988B,
	CPU_S_BGA1023_RPGA988B,
	CPU_S_RPGA988,
	CPU_S_FP2,
	CPU_S_FM2,
	CPU_S_FCBGA1168,
	CPU_S_LGA1150,
	CPU_S_FCBGA1364,
	CPU_S_FCPGA946,
	CPU_S_FT3,
	CPU_S_FT3_769_BGA,
	CPU_S_G34,
	CPU_S_FCBGA1170,
	CPU_S_FM2_PLUS,
	CPU_S_AM1,
	CPU_S_FS1r2,
	CPU_S_FP2_BGA,
	CPU_FC_BGA_1283,
	CPU_S_SOCKET_G3,
	CPU_S_FP3,
	CPU_S_FT3b,
	CPU_S_UTFCBGA1380,
	CPU_S_LGA2011_V3,
	CPU_S_FCBGA1667,
	CPU_S_UTFCBGA592,
	CPU_S_FCBGA1234,
	CPU_S_LGA1151,
	CPU_S_FP4,
	CPU_S_AM4,
	CPU_S_LGA2066,
	CPU_S_FP5,
	CPU_S_LGA1200,
	CPU_S_FCBGA1528,
	CPU_S_FCBGA1440,
	CPU_S_FCBGA1377,
	CPU_S_LGA1151_2,
	CPU_S_FCBGA1526,
	CPU_S_FCBGA1449,
	CPU_S_FCBGA1598,
	CPU_S_LGA1700,
	CPU_S_AM5,
	CPU_S_FCBGA1744,
	CPU_S_MARKER_END

};

#if defined(MAIN_MODULE_CPU)
TCHAR *g_szCPUSocket[] = {
	_T(""),
	_T("754"),
	_T("939"),
	_T("940"),
	_T("949"),
	_T("AM2 (940-pin)"),
	_T("AM2"),
	_T("AM2+ (940-pin)"),
	_T("AM3 (938-pin)"),
	_T("AM3"),
	_T("ASB1"),
	_T("BGA956"),
	_T("F (1207)"),
	_T("FCBGA6"),
	_T("LGA1156"),
	_T("LGA1366"),
	_T("LGA771"),
	_T("LGA775"),
	_T("PBGA437"),
	_T("PBGA441"),
	_T("PGA478/BGA479"),
	_T("PGA604"),
	_T("PPGA478"),
	_T("PPGA478/PPGA479"),
	_T("PPGA479"),
	_T("S1 (638)"),
	_T("S1"),
	_T("Socket G34"),
	_T("Socket M (478)"),
	_T("Socket M"),
	_T("Socket P (478)"),
	_T("Socket P"),
	_T("PBGA479"),
	_T("PLGA775"),
	_T("604"),
	_T("AM2+ (938-pin)"),
	_T("BGA1288"),
	_T("PGA988"),
	_T("BGA1288/PGA988"),
	_T("FCBGA559"),
	_T("S1G3"),
	_T("989"),
	_T("LGA1155"),
	_T("FCLGA1155"),
	_T("FCBGA1224/FCPGA988"),
	_T("FCPGA988"),
	_T("FT1"),
	_T("FM1"),
	_T("FS1"),
	_T("AM3+"),				// SI101021
	_T("Socket C32"),		// SI101021
	_T("LGA 2011"),			// SI101021
	_T("LGA 1356"),			// SI101021
	_T("BGA1288"),			// SI101021
	_T("PGA988"),			// SI101021
	_T("S1G4"),				// SI101022d
	_T("ASB2"),				// SI101022d
	_T("LGA1567"),			// SI101022d
	_T("Socket G2"),		// SI101022d
	_T("BGA-1023"),			// SI101022d
	_T("rPGA988B"),			// SI101022d
	_T("BGA1023/rPGA988B"), // SI101022d
	_T("rPGA988"),			// SI101022d
	_T("FP2"),				// SI101029
	_T("FM2"),				// SI101029
	_T("FCBGA1168"),		// SI101032
	_T("LGA1150"),			// SI101036
	_T("FCBGA1364"),		// SI101036
	_T("FCPGA946"),			// SI101036
	_T("FT3"),				// SI101036
	_T("FT3 (769-BGA)"),	// SI101036
	_T("G34"),				// SI101036
	_T("FCBGA1170"),		// SI101047
	_T("FM2+"),				// SI101049
	_T("AM1"),				// SI101066
	_T("FS1r2"),			// SI101066
	_T("FP2 (BGA)"),		// SI101066
	_T("BGA 1283"),			// SI101066
	_T("Socket G3"),		// SI101066
	_T("FP3"),				// SI101075
	_T("FT3b"),				// SI101075
	_T("UTFCBGA1380"),		// SI101075
	_T("LGA2011-v3"),		// SI101077
	_T("BGA 1667"),			// SI101100
	_T("BGA 592"),			// SI101100
	_T("BGA 1234"),			// SI101100
	_T("LGA 1151"),			// SI101106
	_T("FP4"),				// SI101075
	_T("AM4"),				// SI101075
	_T("LGA 2066"),			// SI101106
	_T("FP5"),
	_T("LGA 1200"),
	_T("BGA 1528"),
	_T("BGA 1440"),
	_T("BGA 1377"),
	_T("LGA 1151-2"), // SI101106
	_T("BGA 1526"),
	_T("BGA 1449"),
	_T("BGA 1598"),
	_T("LGA 1700"),
	_T("AM5 (LGA 1718)"),
	_T("BGA 1744"),
	_T(""),
};
#else
extern TCHAR *g_szCPUSocket[];
#endif

enum eCPUFabrication
{
	CPU_F_BLANK = 0,
	CPU_F_130,
	CPU_F_90,
	CPU_F_65,
	CPU_F_45,
	CPU_F_32,
	CPU_F_40,
	CPU_F_22, // SI101023
	CPU_F_28, // SI101036
	CPU_F_14, // SI101073
	CPU_F_12,
	CPU_F_10,
	CPU_F_7,
	CPU_F_5,
	CPU_F_MARKER_END
};

#if defined(MAIN_MODULE_CPU)
TCHAR *g_szCPUFabrication[] = {
	_T(""),
	_T("130nm"),
	_T("90nm"),
	_T("65nm"),
	_T("45nm"),
	_T("32nm"),
	_T("40nm"),
	_T("22nm"),
	_T("28nm"),
	_T("14nm"),
	_T("12nm"),
	_T("10nm"),
	_T("7nm"),
	_T("5nm"),
	_T(""),
};
#else
extern TCHAR *g_szCPUFabrication[];
#endif

enum eCPUStepping
{
	CPU_ST_BLANK,
	CPU_ST_A1,
	CPU_ST_A1_A3,
	CPU_ST_A1_G0,
	CPU_ST_B0,
	CPU_ST_B1,
	CPU_ST_B2,
	CPU_ST_B2_B3,
	CPU_ST_B2_B4,
	CPU_ST_B3,
	CPU_ST_BH_E4,
	CPU_ST_BH_F2,
	CPU_ST_BH_G2,
	CPU_ST_C0,
	CPU_ST_C0_C1,
	CPU_ST_C1,
	CPU_ST_D0,
	CPU_ST_DH7_CG,
	CPU_ST_DH8_D0,
	CPU_ST_DH8_E3,
	CPU_ST_DH_E3,
	CPU_ST_DH_E6,
	CPU_ST_DH_F2,
	CPU_ST_E0,
	CPU_ST_E0_E1,
	CPU_ST_E1,
	CPU_ST_E4,
	CPU_ST_E6,
	CPU_ST_G0,
	CPU_ST_JH9_E6,
	CPU_ST_JH_E6,
	CPU_ST_JH_F3,
	CPU_ST_L2,
	CPU_ST_M0,
	CPU_ST_M0_C0,
	CPU_ST_M0_M1,
	CPU_ST_M1,
	CPU_ST_R0,
	CPU_ST_SH7_B3,
	CPU_ST_SH7_C0,
	CPU_ST_SH7_CG,
	CPU_ST_SH8_E4,
	CPU_ST_SH8_E5,
	CPU_ST_SH_E4,
	CPU_ST_LG_B1,
	CPU_ST_BL_C2,
	CPU_ST_A0,
	CPU_ST_C2,
	CPU_ST_K0,
	CPU_ST_D2, // BIT6010280000
	CPU_ST_R2, // SI101077
	CPU_ST_MARKER_END
};

#if defined(MAIN_MODULE_CPU)
TCHAR *g_szCPUStepping[] = {
	_T(""),
	_T("A1"),
	_T("A1/A3"),
	_T("A1/G0"),
	_T("B0"),
	_T("B1"),
	_T("B2"),
	_T("B2/B3"),
	_T("B2/B4"),
	_T("B3"),
	_T("BH-E4"),
	_T("BH-F2"),
	_T("BH-G2"),
	_T("C0"),
	_T("C0/C1"),
	_T("C1"),
	_T("D0"),
	_T("DH7-CG"),
	_T("DH8-D0"),
	_T("DH8-E3"),
	_T("DH-E3"),
	_T("DH-E6"),
	_T("DH-F2"),
	_T("E0"),
	_T("E0/E1"),
	_T("E1"),
	_T("E4"),
	_T("E6"),
	_T("G0"),
	_T("JH9-E6"),
	_T("JH-E6"),
	_T("JH-F3"),
	_T("L2"),
	_T("M0"),
	_T("M0/C0"),
	_T("M0/M1"),
	_T("M1"),
	_T("R0"),
	_T("SH7-B3"),
	_T("SH7-C0"),
	_T("SH7-CG"),
	_T("SH8-E4"),
	_T("SH8-E5"),
	_T("SH-E4"),
	_T("LG-B1"),
	_T("BL-C2"),
	_T("A0"),
	_T("C2"),
	_T("K0"),
	_T("D2"),
	_T("R2"),
	_T(""),
};
#else
extern TCHAR *g_szCPUStepping[];
#endif

// Update this table for cpuS
//

static CPU_SPECIFICATION CPU_Specificaton[] =
	{
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL F:0 - F:4 NOT DONE
		/*
		GenuineIntel	 0xf	 0x0	 0x7	 Intel(R) Pentium(R) 4 CPU 1300MHz
		GenuineIntel	 0xf	 0x0	 0x7	 Intel(R) Pentium(R) 4 CPU 1400MHz
		GenuineIntel	 0xf	 0x0	 0x7	 Intel(R) Pentium(R) 4 CPU 1500MHz
		GenuineIntel	 0xf	 0x0	 0xa	 Intel(R) Pentium(R) 4 CPU 1300MHz
		GenuineIntel	 0xf	 0x0	 0xa	 Intel(R) Pentium(R) 4 CPU 1400MHz
		GenuineIntel	 0xf	 0x0	 0xa	 Intel(R) Pentium(R) 4 CPU 1500MHz
		GenuineIntel	 0xf	 0x0	 0xa	 Intel(R) Pentium(R) 4 CPU 1600MHz
		GenuineIntel	 0xf	 0x0	 0xa	 Intel(R) Pentium(R) 4 CPU 1700MHz
		GenuineIntel	 0xf	 0x0	 0xa	 Intel(R) Pentium(R) 4 CPU 1800MHz
		GenuineIntel	 0xf	 0x0	 0xa	 Intel(R) Xeon(TM) CPU 1500MHz
		GenuineIntel	 0xf	 0x1	 0x2	 Intel(R) Pentium(R) 4 CPU 1.40GHz
		GenuineIntel	 0xf	 0x1	 0x2	 Intel(R) Pentium(R) 4 CPU 1.50GHz
		GenuineIntel	 0xf	 0x1	 0x2	 Intel(R) Pentium(R) 4 CPU 1.60GHz
		GenuineIntel	 0xf	 0x1	 0x2	 Intel(R) Pentium(R) 4 CPU 1.70GHz
		GenuineIntel	 0xf	 0x1	 0x2	 Intel(R) Pentium(R) 4 CPU 1.80GHz
		GenuineIntel	 0xf	 0x1	 0x2	 Intel(R) Pentium(R) 4 CPU 1.90GHz
		GenuineIntel	 0xf	 0x1	 0x2	 Intel(R) Pentium(R) 4 CPU 2.00GHz
		GenuineIntel	 0xf	 0x1	 0x2	 Intel(R) Xeon(TM) CPU 1.70GHz
		GenuineIntel	 0xf	 0x1	 0x2	 Intel(R) Xeon(TM) CPU 2.00GHz
		GenuineIntel	 0xf	 0x1	 0x3	 Intel(R) Celeron(R) CPU 1.70GHz
		GenuineIntel	 0xf	 0x1	 0x3	 Intel(R) Celeron(R) CPU 1.80GHz
		GenuineIntel	 0xf	 0x1	 0x3	 Intel(R) Pentium(R) 4 CPU 1.60GHz
		GenuineIntel	 0xf	 0x1	 0x3	 Intel(R) Pentium(R) 4 CPU 1.70GHz
		GenuineIntel	 0xf	 0x1	 0x3	 Intel(R) Pentium(R) 4 CPU 1.80GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 CPU 1.60GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 CPU 1.70GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 CPU 1.80GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 CPU 2.00GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 CPU 2.20GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 CPU 2.26GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 CPU 2.40GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 CPU 2.53GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 Family CPU 2.53GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 Mobile CPU 1.40GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 Mobile CPU 1.60GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 Mobile CPU 1.70GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 Mobile CPU 1.80GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 Mobile CPU 1.90GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) Pentium(R) 4 Mobile CPU 2.00GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) XEON(TM) CPU 2.00GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) XEON(TM) CPU 2.20GHz
		GenuineIntel	 0xf	 0x2	 0x4	 Intel(R) XEON(TM) CPU 2.40GHz
		GenuineIntel	 0xf	 0x2	 0x5	 Intel(R) Pentium(R) 4 CPU 2.26GHz
		GenuineIntel	 0xf	 0x2	 0x5	 Intel(R) Pentium(R) 4 CPU 2.40GHz
		GenuineIntel	 0xf	 0x2	 0x5	 Intel(R) Pentium(R) 4 CPU 2.80GHz
		GenuineIntel	 0xf	 0x2	 0x5	 Intel(R) Pentium(R) 4 CPU 3.20GHz
		GenuineIntel	 0xf	 0x2	 0x5	 Intel(R) Pentium(R) 4 CPU 3.40GHz
		GenuineIntel	 0xf	 0x2	 0x5	 Intel(R) Pentium(R) 4 CPU 3.46GHz
		GenuineIntel	 0xf	 0x2	 0x5	 Intel(R) Xeon(TM) CPU 2.40GHz
		GenuineIntel	 0xf	 0x2	 0x5	 Intel(R) Xeon(TM) CPU 2.66GHz
		GenuineIntel	 0xf	 0x2	 0x5	 Intel(R) Xeon(TM) CPU 2.80GHz
		GenuineIntel	 0xf	 0x2	 0x5	 Intel(R) Xeon(TM) CPU 3.06GHz
		GenuineIntel	 0xf	 0x2	 0x5	 Intel(R) Xeon(TM) CPU 3.20GHz
		GenuineIntel	 0xf	 0x2	 0x5	 Pentium 4
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Celeron(R) CPU 2.00GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Celeron(R) CPU 2.20GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Celeron(R) CPU 2.30GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Celeron(R) CPU 2.40GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Pentium(R) 4 CPU 1.80GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Pentium(R) 4 CPU 1.90GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Pentium(R) 4 CPU 2.00GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Pentium(R) 4 CPU 2.20GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Pentium(R) 4 CPU 2.26GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Pentium(R) 4 CPU 2.40GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Pentium(R) 4 CPU 2.50GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Pentium(R) 4 CPU 2.53GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Pentium(R) 4 CPU 2.60GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Pentium(R) 4 CPU 2.66GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Pentium(R) 4 CPU 2.80GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Pentium(R) 4 CPU 3.06GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Xeon(TM) CPU 1.60GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Xeon(TM) CPU 1.80GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Xeon(TM) CPU 2.00GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Xeon(TM) CPU 2.40GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Xeon(TM) CPU 2.60GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Xeon(TM) CPU 2.66GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Xeon(TM) CPU 2.80GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Intel(R) Xeon(TM) CPU 3.06GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Mobile Intel(R) Celeron(R) CPU 1.50GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Mobile Intel(R) Celeron(R) CPU 1.60GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Mobile Intel(R) Celeron(R) CPU 1.80GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Mobile Intel(R) Celeron(R) CPU 2.00GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Mobile Intel(R) Pentium(R) 4 - M CPU 1.70GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Mobile Intel(R) Pentium(R) 4 - M CPU 1.80GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Mobile Intel(R) Pentium(R) 4 - M CPU 1.90GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Mobile Intel(R) Pentium(R) 4 - M CPU 2.00GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Mobile Intel(R) Pentium(R) 4 - M CPU 2.20GHz
		GenuineIntel	 0xf	 0x2	 0x7	 Mobile Intel(R) Pentium(R) 4 - M CPU 2.40GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Celeron(R) CPU 1.80GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Celeron(R) CPU 2.00GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Celeron(R) CPU 2.20GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Celeron(R) CPU 2.30GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Celeron(R) CPU 2.40GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Celeron(R) CPU 2.50GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Celeron(R) CPU 2.60GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Celeron(R) CPU 2.70GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Celeron(R) CPU 2.80GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Pentium(R) 4 CPU 1.80GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Pentium(R) 4 CPU 2.00GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Pentium(R) 4 CPU 2.20GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Pentium(R) 4 CPU 2.26GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Pentium(R) 4 CPU 2.40GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Pentium(R) 4 CPU 2.50GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Pentium(R) 4 CPU 2.53GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Pentium(R) 4 CPU 2.60GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Pentium(R) 4 CPU 2.66GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Pentium(R) 4 CPU 2.80GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Pentium(R) 4 CPU 3.00GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Pentium(R) 4 CPU 3.06GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Pentium(R) 4 CPU 3.20GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Pentium(R) 4 CPU 3.40GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Xeon(TM) CPU 2.40GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Xeon(TM) CPU 2.66GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Xeon(TM) CPU 2.80GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Intel(R) Xeon(TM) CPU 3.06GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Celeron(R) CPU 1.20GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Celeron(R) CPU 1.70GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Celeron(R) CPU 2.00GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Celeron(R) CPU 2.20GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Celeron(R) CPU 2.40GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Celeron(R) CPU 2.50GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Pentium(R) 4 - M CPU 2.00GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Pentium(R) 4 - M CPU 2.20GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Pentium(R) 4 - M CPU 2.40GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Pentium(R) 4 - M CPU 2.50GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Pentium(R) 4 CPU 2.40GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Pentium(R) 4 CPU 2.66GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Pentium(R) 4 CPU 2.80GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Pentium(R) 4 CPU 3.06GHz
		GenuineIntel	 0xf	 0x2	 0x9	 Mobile Intel(R) Pentium(R) 4 CPU 3.20GHz
		GenuineIntel	 0xf	 0x3	 0x3	 Intel(R) Celeron(R) CPU 2.26GHz
		GenuineIntel	 0xf	 0x3	 0x3	 Intel(R) Celeron(R) CPU 2.40GHz
		GenuineIntel	 0xf	 0x3	 0x3	 Intel(R) Celeron(R) CPU 2.53GHz
		GenuineIntel	 0xf	 0x3	 0x3	 Intel(R) Celeron(R) CPU 2.66GHz
		GenuineIntel	 0xf	 0x3	 0x3	 Intel(R) Celeron(R) CPU 2.80GHz
		GenuineIntel	 0xf	 0x3	 0x3	 Intel(R) Pentium(R) 4 CPU 2.40GHz
		GenuineIntel	 0xf	 0x3	 0x3	 Intel(R) Pentium(R) 4 CPU 2.80GHz
		GenuineIntel	 0xf	 0x3	 0x3	 Intel(R) Pentium(R) 4 CPU 3.00GHz
		GenuineIntel	 0xf	 0x3	 0x3	 Intel(R) Pentium(R) 4 CPU 3.20GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Genuine Intel(R) CPU 3.20GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Celeron(R) CPU 2.26GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Celeron(R) CPU 2.40GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Celeron(R) CPU 2.53GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Celeron(R) CPU 2.66GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Celeron(R) CPU 2.80GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Celeron(R) CPU 2.93GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Celeron(R) CPU 3.06GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Pentium(R) 4 CPU 2.40GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Pentium(R) 4 CPU 2.66GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Pentium(R) 4 CPU 2.80GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Pentium(R) 4 CPU 2.93GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Pentium(R) 4 CPU 3.00GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Pentium(R) 4 CPU 3.20GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Pentium(R) 4 CPU 3.40GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Pentium(R) 4 CPU 3.60GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Xeon(TM) CPU 2.80GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Xeon(TM) CPU 3.00GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Xeon(TM) CPU 3.20GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Xeon(TM) CPU 3.40GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Intel(R) Xeon(TM) CPU 3.60GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Mobile Intel(R) Pentium(R) 4 CPU 2.80GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Mobile Intel(R) Pentium(R) 4 CPU 3.06GHz
		GenuineIntel	 0xf	 0x3	 0x4	 Mobile Intel(R) Pentium(R) 4 CPU 3.20GHz
		GenuineIntel	 0xf	 0x3	 0x7	 Genuine Intel(R) CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Genuine Intel(R) CPU 3.60GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Celeron(R) CPU 2.13GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Celeron(R) CPU 2.26GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Celeron(R) CPU 2.40GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Celeron(R) CPU 2.53GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Celeron(R) CPU 2.66GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Celeron(R) CPU 2.80GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Celeron(R) CPU 2.93GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Celeron(R) CPU 3.06GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Celeron(R) CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Pentium(R) 4 CPU 2.40GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Pentium(R) 4 CPU 2.66GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Pentium(R) 4 CPU 2.80GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Pentium(R) 4 CPU 2.93GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Pentium(R) 4 CPU 3.00GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Pentium(R) 4 CPU 3.06GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Pentium(R) 4 CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Pentium(R) 4 CPU 3.40GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Pentium(R) 4 CPU 3.60GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Pentium(R) 4 CPU 3.80GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Xeon(TM) CPU 2.80GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Xeon(TM) CPU 3.00GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Xeon(TM) CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Xeon(TM) CPU 3.40GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Intel(R) Xeon(TM) CPU 3.60GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Mobile Intel(R) Pentium(R) 4 CPU 2.80GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Mobile Intel(R) Pentium(R) 4 CPU 3.06GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Mobile Intel(R) Pentium(R) 4 CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0x1	 Mobile Intel(R) Pentium(R) 4 CPU 3.33GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Genuine Intel(R) CPU 3.00GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Genuine Intel(R) CPU 3.60GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Genuine Intel(R) CPU 3.73GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Intel(R) Pentium(R) 4 CPU 2.80GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Intel(R) Pentium(R) 4 CPU 3.00GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Intel(R) Pentium(R) 4 CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Intel(R) Pentium(R) 4 CPU 3.40GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Intel(R) Pentium(R) 4 CPU 3.60GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Intel(R) Pentium(R) 4 CPU 3.73GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Intel(R) Pentium(R) 4 CPU 3.80GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Intel(R) Xeon(TM) CPU 2.80GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Intel(R) Xeon(TM) CPU 3.00GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Intel(R) Xeon(TM) CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Intel(R) Xeon(TM) CPU 3.40GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Intel(R) Xeon(TM) CPU 3.60GHz
		GenuineIntel	 0xf	 0x4	 0x3	 Intel(R) Xeon(TM) CPU 3.80GHz
		GenuineIntel	 0xf	 0x4	 0x4	 Genuine Intel(R) CPU 3.00GHz
		GenuineIntel	 0xf	 0x4	 0x4	 Genuine Intel(R) CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0x4	 Intel(R) Pentium(R) D CPU 2.80GHz
		GenuineIntel	 0xf	 0x4	 0x4	 Intel(R) Pentium(R) D CPU 3.00GHz
		GenuineIntel	 0xf	 0x4	 0x4	 Intel(R) Pentium(R) D CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0x7	 Genuine Intel(R) CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0x7	 Intel(R) Pentium(R) D CPU 2.66GHz
		GenuineIntel	 0xf	 0x4	 0x7	 Intel(R) Pentium(R) D CPU 2.80GHz
		GenuineIntel	 0xf	 0x4	 0x7	 Intel(R) Pentium(R) D CPU 3.00GHz
		GenuineIntel	 0xf	 0x4	 0x7	 Intel(R) Pentium(R) D CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0x8	 Intel(R) Xeon(TM) CPU 2.80GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Celeron(R) CPU 2.13GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Celeron(R) CPU 2.26GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Celeron(R) CPU 2.53GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Celeron(R) CPU 2.66GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Celeron(R) CPU 2.80GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Celeron(R) CPU 2.93GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Celeron(R) CPU 3.06GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Celeron(R) CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Celeron(R) CPU 3.33GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Pentium(R) 4 CPU 2.66GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Pentium(R) 4 CPU 2.80GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Pentium(R) 4 CPU 2.93GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Pentium(R) 4 CPU 3.00GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Pentium(R) 4 CPU 3.06GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Pentium(R) 4 CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0x9	 Intel(R) Pentium(R) 4 CPU 3.40GHz
		GenuineIntel	 0xf	 0x4	 0xa	 Genuine Intel(R) CPU 3.80GHz
		GenuineIntel	 0xf	 0x4	 0xa	 Intel(R) Pentium(R) 4 CPU 3.00GHz
		GenuineIntel	 0xf	 0x4	 0xa	 Intel(R) Pentium(R) 4 CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0xa	 Intel(R) Pentium(R) 4 CPU 3.40GHz
		GenuineIntel	 0xf	 0x4	 0xa	 Intel(R) Pentium(R) 4 CPU 3.60GHz
		GenuineIntel	 0xf	 0x4	 0xa	 Intel(R) Pentium(R) 4 CPU 3.80GHz
		GenuineIntel	 0xf	 0x4	 0xa	 Intel(R) Xeon(TM) CPU 2.80GHz
		GenuineIntel	 0xf	 0x4	 0xa	 Intel(R) Xeon(TM) CPU 3.00GHz
		GenuineIntel	 0xf	 0x4	 0xa	 Intel(R) Xeon(TM) CPU 3.20GHz
		GenuineIntel	 0xf	 0x4	 0xa	 Intel(R) Xeon(TM) CPU 3.40GHz
		GenuineIntel	 0xf	 0x4	 0xa	 Intel(R) Xeon(TM) CPU 3.60GHz
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// NOT DONE

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL F:6
		/*
		GenuineIntel	 0xf	 0x6	 0x2	 Genuine Intel(R) CPU 3.20GHz
		GenuineIntel	 0xf	 0x6	 0x2	 Genuine Intel(R) CPU 3.40GHz
		GenuineIntel	 0xf	 0x6	 0x2	 Genuine Intel(R) CPU 3.46GHz
		GenuineIntel	 0xf	 0x6	 0x2	 Intel(R) Pentium(R) 4 CPU 3.00GHz
		GenuineIntel	 0xf	 0x6	 0x2	 Intel(R) Pentium(R) 4 CPU 3.20GHz
		GenuineIntel	 0xf	 0x6	 0x2	 Intel(R) Pentium(R) 4 CPU 3.40GHz
		GenuineIntel	 0xf	 0x6	 0x2	 Intel(R) Pentium(R) 4 CPU 3.60GHz
		GenuineIntel	 0xf	 0x6	 0x2	 Intel(R) Pentium(R) D CPU 2.80GHz
		GenuineIntel	 0xf	 0x6	 0x2	 Intel(R) Pentium(R) D CPU 3.00GHz
		GenuineIntel	 0xf	 0x6	 0x2	 Intel(R) Pentium(R) D CPU 3.20GHz
		GenuineIntel	 0xf	 0x6	 0x2	 Intel(R) Pentium(R) D CPU 3.40GHz
		GenuineIntel	 0xf	 0x6	 0x2	 Intel(R) Pentium(R) D CPU 3.46GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Genuine Intel(R) CPU 3.60GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Genuine Intel(R) CPU 3.73GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Celeron(R) D CPU 3.06GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Celeron(R) D CPU 3.20GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Celeron(R) D CPU 3.33GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Pentium(R) 4 CPU 3.20GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Pentium(R) 4 CPU 3.40GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Pentium(R) 4 CPU 3.60GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Pentium(R) D CPU 2.80GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Pentium(R) D CPU 3.00GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Pentium(R) D CPU 3.20GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Pentium(R) D CPU 3.40GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Pentium(R) D CPU 3.60GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Pentium(R) D CPU 3.73GHz
		GenuineIntel	 0xf	 0x6	 0x5	 Genuine Intel(R) CPU 3.33GHz
		GenuineIntel	 0xf	 0x6	 0x5	 Intel(R) Celeron(R) D CPU 3.06GHz
		GenuineIntel	 0xf	 0x6	 0x5	 Intel(R) Celeron(R) D CPU 3.20GHz
		GenuineIntel	 0xf	 0x6	 0x5	 Intel(R) Celeron(R) D CPU 3.33GHz
		GenuineIntel	 0xf	 0x6	 0x5	 Intel(R) Celeron(R) D CPU 3.46GHz
		GenuineIntel	 0xf	 0x6	 0x5	 Intel(R) Pentium(R) 4 CPU 3.00GHz
		GenuineIntel	 0xf	 0x6	 0x5	 Intel(R) Pentium(R) 4 CPU 3.20GHz
		GenuineIntel	 0xf	 0x6	 0x5	 Intel(R) Pentium(R) 4 CPU 3.40GHz
		GenuineIntel	 0xf	 0x6	 0x5	 Intel(R) Pentium(R) D CPU 2.80GHz
		GenuineIntel	 0xf	 0x6	 0x5	 Intel(R) Pentium(R) D CPU 3.00GHz
		GenuineIntel	 0xf	 0x6	 0x5	 Intel(R) Pentium(R) D CPU 3.20GHz
		GenuineIntel	 0xf	 0x6	 0x5	 Intel(R) Pentium(R) D CPU 3.40GHz
		GenuineIntel	 0xf	 0x6	 0x5	 Intel(R) Pentium(R) D CPU 3.60GHz
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		/*
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Xeon(TM) CPU 3.00GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Xeon(TM) CPU 3.20GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Xeon(TM) CPU 3.73GHz
		GenuineIntel	 0xf	 0x6	 0x4	 Intel(R) Xeon(TM) MV CPU 3.20GHz
		*/
		/* Not included as could be a number of CPUs
		//	Dempsey	//51xx series
		{_T("Intel(R) Xeon(TM) CPU"),0,0xF,0x6,0x4,_T("Dempsey"),_T("C1"),CPU_S_LGA771,CPU_F_65,-1},
		{_T("Intel(R) Xeon(TM) CPU"),0,0xF,0x6,-1,_T("Dempsey"),_T("C0"),CPU_S_LGA771,CPU_F_65,-1},

		//	Tulsa	//71xx series
		{_T("Intel(R) Xeon(R) CPU 71"),0,0xF,0x6,0x8,_T("Tulsa"),_T("B0"),_T("PPGA604"),CPU_F_65,100},
		*/

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:7 - 6:d
		/*
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		//NOT DONE
		GenuineIntel	 0x6	 0x7	 0x2	 Pentium III
		GenuineIntel	 0x6	 0x7	 0x3	 Pentium III
		GenuineIntel	 0x6	 0x8	 0x1	 Pentium III
		GenuineIntel	 0x6	 0x8	 0x3	 Pentium III
		GenuineIntel	 0x6	 0x8	 0x6	 Pentium III
		GenuineIntel	 0x6	 0x8	 0xa	 Pentium III
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Celeron(R) M processor 1200MHz
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Celeron(R) M processor 1300MHz
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Celeron(R) M processor 1400MHz
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Celeron(R) M processor 1500MHz
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Celeron(R) M processor 600MHz
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Celeron(R) processor 600MHz
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Pentium(R) M processor 1000MHz
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Pentium(R) M processor 1100MHz
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Pentium(R) M processor 1200MHz
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Pentium(R) M processor 1300MHz
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Pentium(R) M processor 1400MHz
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Pentium(R) M processor 1500MHz
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Pentium(R) M processor 1600MHz
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Pentium(R) M processor 1700MHz
		GenuineIntel	 0x6	 0x9	 0x5	 Intel(R) Pentium(R) M processor 900MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Celeron(TM) CPU 1000MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Celeron(TM) CPU 1066MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Celeron(TM) CPU 1100MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Celeron(TM) CPU 1133MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Celeron(TM) CPU 1200MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Celeron(TM) CPU 1300MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Pentium(R) III CPU 1200MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Pentium(R) III CPU family 1266MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Pentium(R) III Mobile CPU 1000MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Pentium(R) III Mobile CPU 1066MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Pentium(R) III Mobile CPU 1133MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Pentium(R) III Mobile CPU 1200MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Pentium(R) III Mobile CPU 750MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Pentium(R) III Mobile CPU 800MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Pentium(R) III Mobile CPU 866MHz
		GenuineIntel	 0x6	 0xb	 0x1	 Intel(R) Pentium(R) III Mobile CPU 933MHz
		GenuineIntel	 0x6	 0xb	 0x4	 Intel(R) Celeron(TM) CPU 1300MHz
		GenuineIntel	 0x6	 0xb	 0x4	 Intel(R) Celeron(TM) CPU 1400MHz
		GenuineIntel	 0x6	 0xb	 0x4	 Intel(R) Pentium(R) III CPU - S 1266MHz
		GenuineIntel	 0x6	 0xb	 0x4	 Intel(R) Pentium(R) III CPU - S 1400MHz
		GenuineIntel	 0x6	 0xb	 0x4	 Intel(R) Pentium(R) III CPU - S 933MHz
		GenuineIntel	 0x6	 0xb	 0x4	 Mobile Intel(R) Celeron(TM) CPU 1200MHz
		GenuineIntel	 0x6	 0xb	 0x4	 Mobile Intel(R) Pentium(R) III CPU - M 1133MHz
		GenuineIntel	 0x6	 0xb	 0x4	 Mobile Intel(R) Pentium(R) III CPU - M 1200MHz
		GenuineIntel	 0x6	 0xb	 0x4	 Mobile Intel(R) Pentium(R) III CPU - M 1333MHz
		GenuineIntel	 0x6	 0xb	 0x4	 Mobile Intel(R) Pentium(R) III CPU - M 866MHz
		GenuineIntel	 0x6	 0xb	 0x4	 Mobile Intel(R) Pentium(R) III CPU - M 933MHz
		*/

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:e
		/*
		GenuineIntel	 0x6	 0xd	 0x6	 Intel(R) Celeron(R) M processor 1.30GHz
		GenuineIntel	 0x6	 0xd	 0x6	 Intel(R) Celeron(R) M processor 1.40GHz
		GenuineIntel	 0x6	 0xd	 0x6	 Intel(R) Pentium(R) M processor 1.10GHz
		GenuineIntel	 0x6	 0xd	 0x6	 Intel(R) Pentium(R) M processor 1.40GHz
		GenuineIntel	 0x6	 0xd	 0x6	 Intel(R) Pentium(R) M processor 1.50GHz
		GenuineIntel	 0x6	 0xd	 0x6	 Intel(R) Pentium(R) M processor 1.60GHz
		GenuineIntel	 0x6	 0xd	 0x6	 Intel(R) Pentium(R) M processor 1.70GHz
		GenuineIntel	 0x6	 0xd	 0x6	 Intel(R) Pentium(R) M processor 1.80GHz
		GenuineIntel	 0x6	 0xd	 0x6	 Intel(R) Pentium(R) M processor 2.00GHz
		GenuineIntel	 0x6	 0xd	 0x6	 Intel(R) Pentium(R) M processor 2.10GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Genuine Intel(R) processor 600MHz
		GenuineIntel	 0x6	 0xd	 0x8	 Genuine Intel(R) processor 800MHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Celeron(R) M processor 1.00GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Celeron(R) M processor 1.30GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Celeron(R) M processor 1.40GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Celeron(R) M processor 1.50GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Celeron(R) M processor 1.60GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Celeron(R) M processor 1.70GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Celeron(R) M processor 900MHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Pentium(R) M processor 1.10GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Pentium(R) M processor 1.20GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Pentium(R) M processor 1.30GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Pentium(R) M processor 1.50GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Pentium(R) M processor 1.60GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Pentium(R) M processor 1.70GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Pentium(R) M processor 1.73GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Pentium(R) M processor 1.80GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Pentium(R) M processor 1.86GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Pentium(R) M processor 2.00GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Pentium(R) M processor 2.13GHz
		GenuineIntel	 0x6	 0xd	 0x8	 Intel(R) Pentium(R) M processor 2.26GHz
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Not done (as 2004 release date) - Nothing ealier done

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:e
		/*
		GenuineIntel	 0x6	 0xe	 0x4	 Intel(R) Pentium(R) M CPU 000 @ 2.00GHz
		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU 1500 @ 2.00GHz
		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU L2400 @ 1.66GHz	//100C

		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU T1200 @ 1.50GHz
		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU T1300 @ 1.66GHz	//100C
		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU T1350 @ 1.86GHz
		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU T1400 @ 1.83GHz	//100C

		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU T2050 @ 1.60GHz
		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU T2250 @ 1.73GHz
		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU T2300 @ 1.66GHz
		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU T2400 @ 1.83GHz
		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU T2500 @ 2.00GHz
		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU T2600 @ 2.16GHz
		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU U1300 @ 1.06GHz	//100C
		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU U1400 @ 1.20GHz	//100C
		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU U2400 @ 1.06GHz	//100C
		GenuineIntel	 0x6	 0xe	 0x8	 Genuine Intel(R) CPU U2500 @ 1.20GHz	//100C
		GenuineIntel	 0x6	 0xe	 0x8	 Intel(R) Celeron(R) M CPU 410 @ 1.46GHz
		GenuineIntel	 0x6	 0xe	 0x8	 Intel(R) Celeron(R) M CPU 420 @ 1.60GHz
		GenuineIntel	 0x6	 0xe	 0x8	 Intel(R) Celeron(R) M CPU 430 @ 1.73GHz


		GenuineIntel	 0x6	 0xe	 0xc	 Genuine Intel(R) CPU T2060 @ 1.60GHz
		GenuineIntel	 0x6	 0xe	 0xc	 Genuine Intel(R) CPU T2080 @ 1.73GHz
		GenuineIntel	 0x6	 0xe	 0xc	 Genuine Intel(R) CPU T2130 @ 1.86GHz

		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Core(TM) Duo CPU T2250 @ 1.73GHz
		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Core(TM) Duo CPU T2300 @ 1.66GHz
		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Core(TM) Duo CPU T2350 @ 1.86GHz
		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Core(TM) Duo CPU T2400 @ 1.83GHz
		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Core(TM) Duo CPU T2450 @ 2.00GHz
		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Core(TM) Duo CPU T2500 @ 2.00GHz
		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Core(TM) Duo CPU T2600 @ 2.16GHz
		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Core(TM) Duo CPU T2700 @ 2.33GHz

		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Celeron(R) M CPU 430 @ 1.73GHz
		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Celeron(R) M CPU 440 @ 1.86GHz
		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Celeron(R) M CPU 443 @ 1.20GHz

		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Core(TM) Duo CPU L2400 @ 1.66GHz
		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Core(TM) Duo CPU L2500 @ 1.83GHz

		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Core(TM) Duo CPU U2400 @ 1.06GHz
		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Core(TM) Duo CPU U2500 @ 1.20GHz

		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Core(TM) Solo CPU U1400 @ 1.20GHz
		GenuineIntel	 0x6	 0xe	 0xc	 Intel(R) Core(TM) Solo CPU U1500 @ 1.33GHz
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Genuine Intel(R) CPU T2"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA478, CPU_F_65, -1},
		{_T("Genuine Intel(R) CPU T2"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA478, CPU_F_65, -1},
		{_T("Pentium(R) Dual CPU T2300"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Pentium(R) Dual CPU T2300"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Pentium(R) Dual CPU T2400"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Pentium(R) Dual CPU T2400"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Pentium(R) Dual CPU T2500"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Pentium(R) Dual CPU T2500"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Pentium(R) Dual CPU T2600"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Pentium(R) Dual CPU T2600"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Pentium(R) Dual CPU T2700"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Pentium(R) Dual CPU T2700"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Pentium(R) Dual CPU T2"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA478, CPU_F_65, -1},
		{_T("Pentium(R) Dual CPU T2"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA478, CPU_F_65, -1},

		{_T("Celeron(R) M CPU 410"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA478, CPU_F_65, 100},
		{_T("Celeron(R) M CPU 420"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA478, CPU_F_65, 100},
		{_T("Celeron(R) M CPU 430"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA478, CPU_F_65, 100},
		{_T("Celeron(R) M CPU 430"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA478, CPU_F_65, 100},
		{_T("Celeron(R) M CPU 440"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100},
		{_T("Celeron(R) M CPU 450"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA478, CPU_F_65, 100},
		{_T("Celeron(R) M CPU 423"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA479, CPU_F_65, 100},
		{_T("Celeron(R) M CPU 423"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA479, CPU_F_65, 100},
		{_T("Celeron(R) M CPU 443"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA479, CPU_F_65, 100},

		{_T("Intel(R) CPU T1200"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, 2, CPU_F_65, -1},
		{_T("Intel(R) CPU T1300"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Intel(R) CPU T1300"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Intel(R) CPU T1350"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA478, CPU_F_65, -1},
		{_T("Intel(R) CPU T1400"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Intel(R) CPU T1400"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA478_PPGA479, CPU_F_65, 100}, // Datasheet

		{_T("Core(TM) Duo CPU L2"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM) Duo CPU L2"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA479, CPU_F_65, 100}, // Datasheet

		{_T("Core(TM) Duo CPU U2"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM) Duo CPU U2"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA479, CPU_F_65, 100}, // Datasheet

		{_T("Intel(R) CPU U1"), 0, 6, 0xE, 0x8, CPU_YONAH, CPU_ST_C0, CPU_S_PPGA479, CPU_F_65, 100}, // Datasheet
		{_T("Intel(R) CPU U1"), 0, 6, 0xE, 0xC, CPU_YONAH, CPU_ST_D0, CPU_S_PPGA479, CPU_F_65, 100}, // Datasheet

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:F
		/*
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		//Not clasified as could be multiple CPUs
		GenuineIntel	 0x6	 0xf	 0x4	 Genuine Intel(R) CPU @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0x4	 Genuine Intel(R) CPU @ 2.13GHz
		GenuineIntel	 0x6	 0xf	 0x4	 Genuine Intel(R) CPU @ 2.40GHz
		GenuineIntel	 0x6	 0xf	 0x4	 Genuine Intel(R) CPU @ 2.66GHz
		GenuineIntel	 0x6	 0xf	 0x4	 Genuine Intel(R) CPU @ 2.93GHz
		GenuineIntel	 0x6	 0xf	 0x5	 Genuine Intel(R) CPU @ 2.40GHz
		GenuineIntel	 0x6	 0xf	 0x5	 Genuine Intel(R) CPU @ 2.66GHz
		GenuineIntel	 0x6	 0xf	 0x9	 Genuine Intel(R) CPU @ 2.33GHz
		GenuineIntel	 0x6	 0xf	 0x2	 Intel(R) Core(TM)2 CPU @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0x9	 Intel(R) Core(TM)2 Duo CPU @ 2.66GHz
		*/

		/*
		GenuineIntel	 0x6	 0xf	 0x2	 Genuine Intel(R) CPU 2140 @ 1.60GHz			//Pentium
		GenuineIntel	 0x6	 0xf	 0x2	 Genuine Intel(R) CPU 2160 @ 1.80GHz			//Pentium

		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Pentium(R) Dual CPU E2140 @ 1.60GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Pentium(R) Dual CPU E2160 @ 1.80GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Pentium(R) Dual CPU E2140 @ 1.60GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Pentium(R) Dual CPU E2160 @ 1.80GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Pentium(R) Dual CPU E2180 @ 2.00GHz

		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Pentium(R) Dual CPU E2200 @ 2.20GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Pentium(R) Dual CPU E2220 @ 2.40GHz

		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Pentium(R) Dual CPU T2310 @ 1.46GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Pentium(R) Dual CPU T2330 @ 1.60GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Pentium(R) Dual CPU T2370 @ 1.73GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Pentium(R) Dual CPU T2390 @ 1.86GHz

		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Pentium(R) Dual CPU T2410 @ 2.00GHz

		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Pentium(R) Dual CPU T3200 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Pentium(R) Dual CPU T3400 @ 2.16GHz
		*/

		//	CONROE - Desktop Dual 65nm Aug 2006
		{_T("Genuine Intel(R) CPU 2"), 0, 6, 0xF, 0x2, CPU_CONROE, CPU_ST_L2, CPU_S_LGA775, CPU_F_65, -1},
		{_T("Pentium(R) Dual  CPU  E2"), 0, 6, 0xF, 0x2, CPU_CONROE, CPU_ST_L2, CPU_S_LGA775, CPU_F_65, -1},
		{_T("Pentium(R) Dual  CPU  E2"), 0, 6, 0xF, 0xD, CPU_CONROE, CPU_ST_M0, CPU_S_LGA775, CPU_F_65, -1},
		{_T("Pentium(R) Dual  CPU  E2"), 0, 6, 0xF, 0xB, CPU_CONROE, CPU_ST_G0, CPU_S_LGA775, CPU_F_65, -1},

		//	Merom
		{_T("Pentium(R) Dual CPU T"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P_478, CPU_F_65, -1},

		/*
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Celeron(R) M CPU 520 @ 1.60GHz
		GenuineIntel	 0x6	 0xf	 0xa	 Intel(R) Celeron(R) M CPU 530 @ 1.73GHz

		GenuineIntel	 0x6	 0xf	 0xd	 Genuine Intel(R) CPU 575 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Genuine Intel(R) CPU 585 @ 2.16GHz

		GenuineIntel	 0x6	 0xf	 0xd	 Genuine Intel(R) CPU T1400 @ 1.73GHz	//Celeron
		GenuineIntel	 0x6	 0xf	 0xd	 Genuine Intel(R) CPU T1500 @ 1.86GHz	//Celeron
		GenuineIntel	 0x6	 0xf	 0xd	 Genuine Intel(R) CPU T1600 @ 1.66GHz	//Celeron

		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Celeron(R) CPU E1200 @ 1.60GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Celeron(R) CPU E1400 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Celeron(R) CPU E1500 @ 2.20GHz
		*/
		//	MEROM - Laptop 65nm
		{_T("Celeron(R) M CPU 52"), 0, 6, 0xF, 0x6, CPU_MEROM, CPU_ST_B2, CPU_S_SOCKET_M, CPU_F_65, 100}, // Datasheet
		{_T("Celeron(R) M CPU 53"), 0, 6, 0xF, 0xa, CPU_MEROM, CPU_ST_E1, CPU_S_SOCKET_M, CPU_F_65, 100}, // Datasheet

		{_T("Intel(R) CPU 575"), 0, 6, 0x16, 0xd, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P, CPU_F_65, 100}, // Datasheet
		{_T("Intel(R) CPU 585"), 0, 6, 0x16, 0xd, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P, CPU_F_65, 100}, // Datasheet

		{_T("Intel(R) CPU T1"), 0, 6, 0x16, 0xd, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P_478, CPU_F_65, 100},

		//	CONROE - Desktop Dual 65nm Aug 2006
		{_T("Intel(R) Celeron(R) CPU E1"), 0, 6, 0xF, 0xd, CPU_CONROE, CPU_ST_M0, CPU_S_SOCKET_M, CPU_F_65, 100}, // Datasheet

		/*
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Duo CPU E4700 @ 2.60GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU E4400 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU E4500 @ 2.20GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU E4600 @ 2.40GHz

		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Duo CPU E6540 @ 2.33GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Duo CPU E6550 @ 2.33GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Duo CPU E6750 @ 2.66GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Duo CPU E6850 @ 3.00GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU E6400 @ 2.13GHz
		*/
		{_T("Core(TM)2 Duo CPU E4"), 0, 6, 0xF, 0x2, CPU_CONROE, CPU_ST_L2, CPU_S_LGA775, CPU_F_65, 70},
		{_T("Core(TM)2 Duo CPU E4"), 0, 6, 0xF, 0xB, CPU_CONROE, CPU_ST_G0, CPU_S_LGA775, CPU_F_65, -1},
		{_T("Core(TM)2 Duo CPU E4"), 0, 6, 0xF, 0xD, CPU_CONROE, CPU_ST_M0, CPU_S_LGA775, CPU_F_65, 80},
		{_T("Core(TM)2 Duo CPU E6"), 0, 6, 0xF, 0x2, CPU_CONROE, CPU_ST_L2, CPU_S_LGA775, CPU_F_65, 70},
		{_T("Core(TM)2 Duo CPU E6"), 0, 6, 0xF, 0xB, CPU_CONROE, CPU_ST_G0, CPU_S_LGA775, CPU_F_65, -1},
		{_T("Core(TM)2 Duo CPU E6"), 0, 6, 0xF, 0xD, CPU_CONROE, CPU_ST_M0, CPU_S_LGA775, CPU_F_65, 80},

		/*
		GenuineIntel	 0x6	 0xf	 0x2	 Intel(R) Core(TM)2 CPU 4300 @ 1.80GHz
		GenuineIntel	 0x6	 0xf	 0x2	 Intel(R) Core(TM)2 CPU 4400 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0x2	 Intel(R) Core(TM)2 CPU 6300 @ 1.86GHz
		GenuineIntel	 0x6	 0xf	 0x2	 Intel(R) Core(TM)2 CPU 6400 @ 2.13GHz
		GenuineIntel	 0x6	 0xf	 0x5	 Intel(R) Core(TM)2 CPU 6600 @ 2.40GHz
		GenuineIntel	 0x6	 0xf	 0x5	 Intel(R) Core(TM)2 CPU 6700 @ 2.66GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Core(TM)2 CPU 6300 @ 1.86GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Core(TM)2 CPU 6320 @ 1.86GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Core(TM)2 CPU 6400 @ 2.13GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Core(TM)2 CPU 6420 @ 2.13GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Core(TM)2 CPU 6600 @ 2.40GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Core(TM)2 CPU 6700 @ 2.66GHz

		Model

		E6000 and E4000 series
		70°C Target Tj (B2/B3/L2) 2->L2, 6->B2
		80°C Target Tj (G0/M0)
		*/
		{_T("Core(TM)2 CPU 4"), 0, 6, 0xF, 0x2, CPU_CONROE, CPU_ST_L2, CPU_S_LGA775, CPU_F_65, 70}, // SI101099 //E4xxx
		{_T("Core(TM)2 CPU 6"), 0, 6, 0xF, 0x6, CPU_CONROE, CPU_ST_B2, CPU_S_LGA775, CPU_F_65, 70}, // SI101099 //E6xxx
		{_T("Core(TM)2 CPU 6"), 0, 6, 0xF, 0x5, CPU_CONROE, CPU_ST_B1, CPU_S_LGA775, CPU_F_65, -1},
		{_T("Core(TM)2 CPU 6"), 0, 6, 0xF, 0x2, CPU_CONROE, CPU_ST_L2, CPU_S_LGA775, CPU_F_65, 70}, // SI101099

		/*
		GenuineIntel	 0x6	 0xf	 0x2	 Intel(R) Core(TM)2 CPU T5300 @ 1.73GHz
		GenuineIntel	 0x6	 0xf	 0x2	 Intel(R) Core(TM)2 CPU T5500 @ 1.66GHz
		GenuineIntel	 0x6	 0xf	 0x2	 Intel(R) Core(TM)2 CPU T5600 @ 1.83GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Core(TM)2 CPU T5200 @ 1.60GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Core(TM)2 CPU T5500 @ 1.66GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Core(TM)2 CPU T5600 @ 1.83GHz

		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU T5250 @ 1.50GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU T5270 @ 1.40GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU T5450 @ 1.66GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU T5470 @ 1.60GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU T5550 @ 1.83GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU T5670 @ 1.80GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU T5750 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU T5800 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU T5850 @ 2.16GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU T5870 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU T5900 @ 2.20GHz
		*/
		{_T("Core(TM)2 CPU T5600"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_M, CPU_F_65, 100},	  // Datasheet
		{_T("Core(TM)2 CPU T5600"), 0, 6, 0xF, 0x6, CPU_MEROM, CPU_ST_B2, CPU_S_SOCKET_M, CPU_F_65, 100},	  // Datasheet
		{_T("Core(TM)2 CPU T5600"), 0, 6, 0xF, 0x2, CPU_MEROM, CPU_ST_L2, CPU_S_SOCKET_M, CPU_F_65, 100},	  // Datasheet
		{_T("Core(TM)2 CPU T5500"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_M, CPU_F_65, 100},	  // Datasheet
		{_T("Core(TM)2 CPU T5500"), 0, 6, 0xF, 0x6, CPU_MEROM, CPU_ST_B2, CPU_S_SOCKET_M, CPU_F_65, 100},	  // Datasheet
		{_T("Core(TM)2 CPU T5500"), 0, 6, 0xF, 0x2, CPU_MEROM, CPU_ST_L2, CPU_S_SOCKET_M, CPU_F_65, 100},	  // Datasheet
		{_T("Core(TM)2 CPU T5500"), 0, 6, 0xF, -1, CPU_MEROM, CPU_ST_BLANK, CPU_S_SOCKET_M, CPU_F_65, 100},	  // Datasheet
		{_T("Core(TM)2 CPU T5300"), 0, 6, 0xF, 0x2, CPU_MEROM, CPU_ST_L2, CPU_S_SOCKET_M_478, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM)2 CPU T5200"), 0, 6, 0xF, 0x6, CPU_MEROM, CPU_ST_B2, CPU_S_SOCKET_M, CPU_F_65, 85},	  // Datasheet

		{_T("Core(TM)2 Duo CPU T5250"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P_478, CPU_F_65, 85},  // Datasheet
		{_T("Core(TM)2 Duo CPU T5270"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P_478, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM)2 Duo CPU T5450"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P_478, CPU_F_65, 85},  // Datasheet
		{_T("Core(TM)2 Duo CPU T5470"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P_478, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM)2 Duo CPU T5550"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P_478, CPU_F_65, 85},  // Datasheet
		{_T("Core(TM)2 Duo CPU T5670"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P_478, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM)2 Duo CPU T5750"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P_478, CPU_F_65, 85},  // Datasheet
		{_T("Core(TM)2 Duo CPU T5800"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P_478, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM)2 Duo CPU T5850"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P_478, CPU_F_65, 85},  // Datasheet
		{_T("Core(TM)2 Duo CPU T5870"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P_478, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM)2 Duo CPU T5900"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P_478, CPU_F_65, 100}, // Datasheet

		/*
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Duo CPU T7800 @ 2.60GHz
		GenuineIntel	 0x6	 0xf	 0xa	 Intel(R) Core(TM)2 Duo CPU T7700 @ 2.40GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Duo CPU T7700 @ 2.40GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Duo CPU T7500 @ 2.20GHz
		GenuineIntel	 0x6	 0xf	 0xa	 Intel(R) Core(TM)2 Duo CPU T7500 @ 2.20GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Duo CPU T7300 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0xa	 Intel(R) Core(TM)2 Duo CPU T7300 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU T7250 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU T7100 @ 1.80GHz

		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Core(TM)2 CPU T7600 @ 2.33GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Core(TM)2 CPU T7400 @ 2.16GHz
		GenuineIntel	 0x6	 0xf	 0x5	 Intel(R) Core(TM)2 CPU T7200 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Core(TM)2 CPU T7200 @ 2.00GHz
		*/
		{_T("Core(TM)2 Duo CPU T7800"), 0, 6, 0xF, 0xB, CPU_MEROM, CPU_ST_G0, CPU_S_SOCKET_P, CPU_F_65, 100},	 // Datasheet
		{_T("Core(TM)2 Duo CPU T7700"), 0, 6, 0xF, 0xA, CPU_MEROM, CPU_ST_E1, CPU_S_SOCKET_P, CPU_F_65, 100},	 // Datasheet
		{_T("Core(TM)2 Duo CPU T7700"), 0, 6, 0xF, 0xB, CPU_MEROM, CPU_ST_G0, CPU_S_SOCKET_P, CPU_F_65, 100},	 // Datasheet
		{_T("Core(TM)2 Duo CPU T7500"), 0, 6, 0xF, 0xA, CPU_MEROM, CPU_ST_E1, CPU_S_SOCKET_P, CPU_F_65, 100},	 // Datasheet
		{_T("Core(TM)2 Duo CPU T7500"), 0, 6, 0xF, 0xB, CPU_MEROM, CPU_ST_G0, CPU_S_SOCKET_P, CPU_F_65, 100},	 // Datasheet
		{_T("Core(TM)2 Duo CPU T7300"), 0, 6, 0xF, 0xA, CPU_MEROM, CPU_ST_E1, CPU_S_SOCKET_P, CPU_F_65, 100},	 // Datasheet
		{_T("Core(TM)2 Duo CPU T7300"), 0, 6, 0xF, 0xB, CPU_MEROM, CPU_ST_G0, CPU_S_SOCKET_P, CPU_F_65, 100},	 // Datasheet
		{_T("Core(TM)2 Duo CPU T7250"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0_M1, CPU_S_SOCKET_P, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM)2 Duo CPU T7100"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0_M1, CPU_S_SOCKET_P, CPU_F_65, 100}, // Datasheet

		{_T("Core(TM)2 CPU T7600"), 0, 6, 0xF, 0x6, CPU_MEROM, CPU_ST_B2, CPU_S_SOCKET_M, CPU_F_65, 100},	// Datasheet
		{_T("Core(TM)2 CPU T7400"), 0, 6, 0xF, 0x6, CPU_MEROM, CPU_ST_B2, CPU_S_SOCKET_M, CPU_F_65, 100},	// Datasheet
		{_T("Core(TM)2 CPU T7200"), 0, 6, 0xF, 0x6, CPU_MEROM, CPU_ST_B2, CPU_S_SOCKET_M, CPU_F_65, 100},	// Datasheet
		{_T("Core(TM)2 CPU T7200"), 0, 6, 0xF, -1, CPU_MEROM, CPU_ST_BLANK, CPU_S_SOCKET_M, CPU_F_65, 100}, // Datasheet

		/*
		GenuineIntel	 0x6	 0xf	 0x2	 Intel(R) Core(TM)2 CPU U7500 @ 1.06GHz
		GenuineIntel	 0x6	 0xf	 0x2	 Intel(R) Core(TM)2 CPU U7600 @ 1.20GHz
		GenuineIntel	 0x6	 0xf	 0x2	 Intel(R) Core(TM)2 CPU U7700 @ 1.33GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU U7500 @ 1.06GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU U7600 @ 1.20GHz
		GenuineIntel	 0x6	 0xf	 0xd	 Intel(R) Core(TM)2 Duo CPU U7700 @ 1.33GHz
		*/
		{_T("Core(TM)2 CPU U7"), 0, 6, 0xF, 0x2, CPU_MEROM, CPU_ST_L2, CPU_S_SOCKET_P, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM)2 CPU U7"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M0, CPU_S_SOCKET_P, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM)2 Duo CPU U7"), 0, 6, 0xF, 0xD, CPU_MEROM, CPU_ST_M1, CPU_S_SOCKET_P, CPU_F_65, 100},

		/*
		GenuineIntel	 0x6	 0xf	 0x5	 Intel(R) Core(TM)2 CPU X6800 @ 2.93GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Core(TM)2 CPU X6800 @ 2.93GHz
		*/
		{_T("Core(TM)2 CPU X6"), 0, 6, 0xF, 0x5, CPU_CONROE, CPU_ST_B1, CPU_S_LGA775, CPU_F_65, 75},
		{_T("Core(TM)2 CPU X6"), 0, 6, 0xF, 0x6, CPU_CONROE, CPU_ST_B2, CPU_S_LGA775, CPU_F_65, 75},

		/*
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Duo CPU L7100 @ 1.20GHz
		GenuineIntel	 0x6	 0xf	 0xa	 Intel(R) Core(TM)2 Duo CPU L7300 @ 1.40GHz
		GenuineIntel	 0x6	 0xf	 0xa	 Intel(R) Core(TM)2 Duo CPU L7500 @ 1.60GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Duo CPU L7500 @ 1.60GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Duo CPU L7700 @ 1.80GHz
		*/
		{_T("Core(TM)2 Duo CPU L7100"), 0, 6, 0xF, 0xB, CPU_MEROM, CPU_ST_G0, CPU_S_BLANK, CPU_F_65, 100},	  // Datasheet
		{_T("Core(TM)2 Duo CPU L7200"), 0, 6, 0xF, 0x6, CPU_MEROM, CPU_ST_B2, CPU_S_SOCKET_M, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM)2 Duo CPU L7300"), 0, 6, 0xF, 0xA, CPU_MEROM, CPU_ST_E1, CPU_S_SOCKET_P, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM)2 Duo CPU L7400"), 0, 6, 0xF, 0x6, CPU_MEROM, CPU_ST_B2, CPU_S_SOCKET_M, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM)2 Duo CPU L7500"), 0, 6, 0xF, 0xB, CPU_MEROM, CPU_ST_G0, CPU_S_SOCKET_P, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM)2 Duo CPU L7500"), 0, 6, 0xF, 0xA, CPU_MEROM, CPU_ST_E0, CPU_S_SOCKET_P, CPU_F_65, 100}, // Datasheet
		{_T("Core(TM)2 Duo CPU L7700"), 0, 6, 0xF, 0xB, CPU_MEROM, CPU_ST_G0, CPU_S_BLANK, CPU_F_65, 100},

		/*
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Duo CPU P7500 @ 1.60GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Duo CPU P7700 @ 1.80GHz
		*/
		// not done

		/*
		GenuineIntel	 0x6	 0xf	 0xa	 Intel(R) Core(TM)2 Extreme CPU X7800 @ 2.60GHz
		GenuineIntel	 0x6	 0xf	 0xa	 Intel(R) Core(TM)2 Extreme CPU X7850 @ 2.80GHz
		GenuineIntel	 0x6	 0xf	 0xa	 Intel(R) Core(TM)2 Extreme CPU X7900 @ 2.80GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Extreme CPU X7900 @ 2.80GHz
		*/
		{_T("Core(TM)2 Extreme CPU X7"), 0, 6, 0xF, 0xA, CPU_MEROM, CPU_ST_E1, CPU_S_SOCKET_P, CPU_F_65, 100}, // Datasheet X7800/X7900
		{_T("Core(TM)2 Extreme CPU X7"), 0, 6, 0xF, 0xB, CPU_MEROM, CPU_ST_G0, CPU_S_SOCKET_P, CPU_F_65, 100}, // Datasheet X7800/X7900

		/*
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Quad CPU Q6600 @ 2.40GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Quad CPU Q6700 @ 2.66GHz
		*/
		{_T("Core(TM)2 Quad CPU Q6"), 0, 6, 0xF, 0x4, CPU_KENTSFIELD, CPU_ST_B0, CPU_S_LGA775, CPU_F_65, 80},
		{_T("Core(TM)2 Quad CPU Q6"), 0, 6, 0xF, 0x7, CPU_KENTSFIELD, CPU_ST_B3, CPU_S_LGA775, CPU_F_65, 80},
		{_T("Core(TM)2 Quad CPU Q6"), 0, 6, 0xF, 0xB, CPU_KENTSFIELD, CPU_ST_G0, CPU_S_LGA775, CPU_F_65, 90},

		/*
		GenuineIntel	 0x6	 0xf	 0x7	 Intel(R) Core(TM)2 Quad CPU @ 2.40GHz
		GenuineIntel	 0x6	 0xf	 0x7	 Intel(R) Core(TM)2 Quad CPU @ 2.66GHz
		GenuineIntel	 0x6	 0xf	 0x7	 Intel(R) Core(TM)2 Quad CPU @ 2.93GHz
		*/
		{_T("Core(TM)2 Quad CPU @ 2.66GHz"), 0, 6, 0xF, 0x7, CPU_KENTSFIELD, CPU_ST_B3, CPU_S_LGA775, CPU_F_65, -1}, // QX6700
		{_T("Core(TM)2 Quad CPU @ 2.93GHz"), 0, 6, 0xF, 0x7, CPU_KENTSFIELD, CPU_ST_B3, CPU_S_LGA775, CPU_F_65, -1}, // QX6800

		/*
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Extreme CPU Q6800 @ 2.93GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Core(TM)2 Extreme CPU Q6850 @ 3.00GHz
		*/
		{_T("Core(TM)2 Extreme CPU Q68"), 0, 6, 0xF, 0x7, CPU_KENTSFIELD, CPU_ST_B3, CPU_S_LGA775, CPU_F_65, 80},
		{_T("Core(TM)2 Extreme CPU Q68"), 0, 6, 0xF, 0xB, CPU_KENTSFIELD, CPU_ST_G0, CPU_S_LGA775, CPU_F_65, 80},
		{_T("Core(TM)2 Extreme CPU Q6"), 0, 6, 0xF, 0x7, CPU_KENTSFIELD, CPU_ST_B3, CPU_S_LGA775, CPU_F_65, 90},
		{_T("Core(TM)2 Extreme CPU Q6"), 0, 6, 0xF, 0xB, CPU_KENTSFIELD, CPU_ST_G0, CPU_S_LGA775, CPU_F_65, 80},

		// XEON
		/*
		GenuineIntel	 0x6	 0xf	 0x2	 Intel(R) Xeon(R) CPU 3050 @ 2.13GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Xeon(R) CPU 3050 @ 2.13GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Xeon(R) CPU 3060 @ 2.40GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU 3065 @ 2.33GHz
		*/
		{_T("Xeon(R) CPU 3070"), 0, 6, 0xF, 0x6, CPU_CONROE, CPU_ST_B2_B4, CPU_S_LGA775, CPU_F_65, 80},
		{_T("Xeon(R) CPU 3065"), 0, 6, 0xF, 0xB, CPU_CONROE, CPU_ST_G0, CPU_S_PLGA775, CPU_F_65, 90},
		{_T("Xeon(R) CPU 3060"), 0, 6, 0xF, 0x6, CPU_CONROE, CPU_ST_B2_B4, CPU_S_PLGA775, CPU_F_65, 80},
		{_T("Xeon(R) CPU 3050"), 0, 6, 0xF, 0x2, CPU_CONROE, CPU_ST_L2, CPU_S_LGA775, CPU_F_65, -1},
		{_T("Xeon(R) CPU 3050"), 0, 6, 0xF, 0x6, CPU_CONROE, CPU_ST_B2, CPU_S_LGA775, CPU_F_65, 80},
		{_T("Xeon(R) CPU 3040"), 0, 6, 0xF, 0x2, CPU_CONROE, CPU_ST_L2, CPU_S_LGA775, CPU_F_65, -1},
		{_T("Xeon(R) CPU 3040"), 0, 6, 0xF, 0x6, CPU_CONROE, CPU_ST_B2, CPU_S_PLGA775, CPU_F_65, 80},

		/*
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Xeon(R) CPU 5110 @ 1.60GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Xeon(R) CPU 5120 @ 1.86GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Xeon(R) CPU 5130 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Xeon(R) CPU 5140 @ 2.33GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Xeon(R) CPU 5150 @ 2.66GHz
		GenuineIntel	 0x6	 0xf	 0x5	 Intel(R) Xeon(R) CPU 5160 @ 3.00GHz
		GenuineIntel	 0x6	 0xf	 0x6	 Intel(R) Xeon(R) CPU 5160 @ 3.00GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU 5110 @ 1.60GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU 5120 @ 1.86GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU 5130 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU 5140 @ 2.33GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU 5160 @ 3.00GHz
		*/
		//	WOODCREST (LV)
		{_T("Xeon(R) CPU 5148"), 0, 6, 0xF, 0xB, CPU_WOODCREST, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 80},
		{_T("Xeon(R) CPU 5148"), 0, 6, 0xF, 0x6, CPU_WOODCREST, CPU_ST_B2, CPU_S_LGA771, CPU_F_65, -1},
		{_T("Xeon(R) CPU 5138"), 0, 6, 0xF, 0xB, CPU_WOODCREST, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 100},
		{_T("Xeon(R) CPU 5128"), 0, 6, 0xF, 0xB, CPU_WOODCREST, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, -1},
		{_T("Xeon(R) CPU 5128"), 0, 6, 0xF, 0x6, CPU_WOODCREST, CPU_ST_B2, CPU_S_LGA771, CPU_F_65, -1},
		//	WOODCREST
		{_T("Xeon(R) CPU 5160"), 0, 6, 0xF, 0xB, CPU_WOODCREST, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 80},
		{_T("Xeon(R) CPU 5160"), 0, 6, 0xF, 0x6, CPU_WOODCREST, CPU_ST_B2, CPU_S_LGA771, CPU_F_65, -1},
		{_T("Xeon(R) CPU 5150"), 0, 6, 0xF, 0xB, CPU_WOODCREST, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 80},
		{_T("Xeon(R) CPU 5150"), 0, 6, 0xF, 0x6, CPU_WOODCREST, CPU_ST_B2, CPU_S_LGA771, CPU_F_65, -1},
		{_T("Xeon(R) CPU 5140"), 0, 6, 0xF, 0xB, CPU_WOODCREST, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 80},
		{_T("Xeon(R) CPU 5140"), 0, 6, 0xF, 0x6, CPU_WOODCREST, CPU_ST_B2, CPU_S_LGA771, CPU_F_65, -1},
		{_T("Xeon(R) CPU 5130"), 0, 6, 0xF, 0xB, CPU_WOODCREST, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 80},
		{_T("Xeon(R) CPU 5130"), 0, 6, 0xF, 0x6, CPU_WOODCREST, CPU_ST_B2, CPU_S_LGA771, CPU_F_65, -1},
		{_T("Xeon(R) CPU 5120"), 0, 6, 0xF, 0xB, CPU_WOODCREST, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 80},
		{_T("Xeon(R) CPU 5120"), 0, 6, 0xF, 0x6, CPU_WOODCREST, CPU_ST_B2, CPU_S_LGA771, CPU_F_65, -1},
		{_T("Xeon(R) CPU 5110"), 0, 6, 0xF, 0xB, CPU_WOODCREST, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 80},
		{_T("Xeon(R) CPU 5110"), 0, 6, 0xF, 0x6, CPU_WOODCREST, CPU_ST_B2, CPU_S_LGA771, CPU_F_65, -1},

		/*
		GenuineIntel	 0x6	 0xf	 0x7	 Intel(R) Xeon(R) CPU E5310 @ 1.60GHz
		GenuineIntel	 0x6	 0xf	 0x7	 Intel(R) Xeon(R) CPU E5320 @ 1.86GHz
		GenuineIntel	 0x6	 0xf	 0x7	 Intel(R) Xeon(R) CPU E5335 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0x7	 Intel(R) Xeon(R) CPU E5345 @ 2.33GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU E5310 @ 1.60GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU E5320 @ 1.86GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU E5335 @ 2.00GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU E5345 @ 2.33GHz
		*/
		//	CLOVERTOWN
		{_T("Xeon(R) CPU E5345"), 0, 6, 0xF, 0x7, CPU_CLOVERTOWN, CPU_ST_B3, CPU_S_LGA771, CPU_F_65, 80},
		{_T("Xeon(R) CPU E5345"), 0, 6, 0xF, 0xB, CPU_CLOVERTOWN, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 80},
		{_T("Xeon(R) CPU E5335"), 0, 6, 0xF, 0x7, CPU_CLOVERTOWN, CPU_ST_B3, CPU_S_LGA771, CPU_F_65, 80},
		{_T("Xeon(R) CPU E5335"), 0, 6, 0xF, 0xB, CPU_CLOVERTOWN, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 80},
		{_T("Xeon(R) CPU E5320"), 0, 6, 0xF, 0x7, CPU_CLOVERTOWN, CPU_ST_B2_B3, CPU_S_LGA771, CPU_F_65, 80},
		{_T("Xeon(R) CPU E5320"), 0, 6, 0xF, 0xB, CPU_CLOVERTOWN, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 80},
		{_T("Xeon(R) CPU E5310"), 0, 6, 0xF, 0x7, CPU_CLOVERTOWN, CPU_ST_B3, CPU_S_LGA771, CPU_F_65, 80},
		{_T("Xeon(R) CPU E5310"), 0, 6, 0xF, 0xB, CPU_CLOVERTOWN, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 80},

		// Tigerton
		{_T("Xeon(R) CPU E7210"), 0, 6, 0xF, 0xB, CPU_TIGERTON, CPU_ST_G0, CPU_S_604, CPU_F_65, 80},
		{_T("Xeon(R) CPU E7220"), 0, 6, 0xF, 0xB, CPU_TIGERTON, CPU_ST_G0, CPU_S_604, CPU_F_65, 80},

		{_T("Xeon(R) CPU E73"), 0, 6, 0xF, 0xB, CPU_TIGERTON, CPU_ST_G0, CPU_S_604, CPU_F_65, 80},

		/*
		GenuineIntel	 0x6	 0xf	 0x7	 Intel(R) Xeon(R) CPU X3210 @ 2.13GHz
		GenuineIntel	 0x6	 0xf	 0x7	 Intel(R) Xeon(R) CPU X3220 @ 2.40GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU X3210 @ 2.13GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU X3220 @ 2.40GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU X3230 @ 2.66GHz

		GenuineIntel	 0x6	 0xf	 0x7	 Intel(R) Xeon(R) CPU X5355 @ 2.66GHz
		GenuineIntel	 0x6	 0xf	 0x7	 Intel(R) Xeon(R) CPU X5365 @ 3.00GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU X5355 @ 2.66GHz
		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU X5365 @ 3.00GHz

		GenuineIntel	 0x6	 0xf	 0xb	 Intel(R) Xeon(R) CPU X7350 @ 2.93GHz
		*/
		{_T("Xeon(R) CPU X32"), 0, 6, 0xF, 0x7, CPU_KENTSFIELD, CPU_ST_B3, CPU_S_LGA775, CPU_F_65, 90},
		{_T("Xeon(R) CPU X32"), 0, 6, 0xF, 0xB, CPU_KENTSFIELD, CPU_ST_G0, CPU_S_LGA775, CPU_F_65, 90},

		{_T("Xeon(R) CPU X5365"), 0, 6, 0xF, 0xB, CPU_CLOVERTOWN, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, -1}, // 90C or 95C
		{_T("Xeon(R) CPU X5365"), 0, 6, 0xF, 0x7, CPU_CLOVERTOWN, CPU_ST_B3, CPU_S_LGA771, CPU_F_65, -1}, // 90C or 95C
		{_T("Xeon(R) CPU X5355"), 0, 6, 0xF, 0xB, CPU_CLOVERTOWN, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, -1}, // 90C or 95C
		{_T("Xeon(R) CPU X5355"), 0, 6, 0xF, 0x7, CPU_CLOVERTOWN, CPU_ST_B3, CPU_S_LGA771, CPU_F_65, -1}, // 90C or 95C

		{_T("Xeon(R) CPU X73"), 0, 6, 0xF, 0xB, CPU_TIGERTON, CPU_ST_G0, CPU_S_604, CPU_F_65, 90},

		//	CLOVERTOWN
		{_T("Xeon(R) CPU L5335"), 0, 6, 0xF, 0xB, CPU_CLOVERTOWN, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 70},
		{_T("Xeon(R) CPU L5320"), 0, 6, 0xF, 0xB, CPU_CLOVERTOWN, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 70},
		{_T("Xeon(R) CPU L5320"), 0, 6, 0xF, 0x7, CPU_CLOVERTOWN, CPU_ST_B3, CPU_S_LGA771, CPU_F_65, 70},
		{_T("Xeon(R) CPU L5318"), 0, 6, 0xF, 0xB, CPU_CLOVERTOWN, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 95}, // Low power
		{_T("Xeon(R) CPU L5310"), 0, 6, 0xF, 0xB, CPU_CLOVERTOWN, CPU_ST_G0, CPU_S_LGA771, CPU_F_65, 70},

		{_T("Xeon(R) CPU L73"), 0, 6, 0xF, 0xB, CPU_TIGERTON_LV, CPU_ST_G0, CPU_S_604, CPU_F_65, 80},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:16
		/*
		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Celeron(R) CPU 220 @ 1.20GHz
		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Celeron(R) CPU 420 @ 1.60GHz
		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Celeron(R) CPU 430 @ 1.80GHz
		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Celeron(R) CPU 440 @ 2.00GHz
		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Celeron(R) CPU 450 @ 2.20GHz
		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Celeron(R) D CPU 220 @ 1.20GHz
		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Celeron(R) D CPU 430 @ 1.80GHz

		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Celeron(R) CPU 530 @ 1.73GHz
		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Celeron(R) CPU 540 @ 1.86GHz
		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Celeron(R) CPU 550 @ 2.00GHz
		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Celeron(R) CPU 560 @ 2.13GHz
		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Celeron(R) CPU 570 @ 2.26GHz


		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Celeron(R) M CPU 520 @ 1.60GHz
		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Celeron(R) M CPU 530 @ 1.73GHz

		GenuineIntel	 0x6	 0x16	 0x1	 Intel(R) Core(TM)2 Solo CPU U2100 @ 1.06GHz
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Celeron(R) CPU 2"), 0, 6, 0x16, 0x1, CPU_CONROE, CPU_ST_A1, CPU_S_PBGA479, CPU_F_65, 100}, // Datasheet
		{_T("Celeron(R) CPU 4"), 0, 6, 0x16, 0x1, CPU_CONROE, CPU_ST_A1, CPU_S_LGA775, CPU_F_65, -1},
		{_T("Celeron(R) D CPU 2"), 0, 6, 0x16, 0x1, CPU_CONROE, CPU_ST_A1, CPU_S_PBGA479, CPU_F_65, 100}, // Datasheet
		{_T("Celeron(R) D CPU 4"), 0, 6, 0x16, 0x1, CPU_CONROE, CPU_ST_A1, CPU_S_LGA775, CPU_F_65, -1},

		{_T("Celeron(R) CPU 530"), 0, 6, 0x16, 0x1, CPU_MEROM, CPU_ST_A1, CPU_S_SOCKET_P, CPU_F_65, 100},	 // Datasheet
		{_T("Celeron(R) CPU 540"), 0, 6, 0x16, 0x1, CPU_MEROM, CPU_ST_A1_A3, CPU_S_SOCKET_P, CPU_F_65, 100}, // Datasheet
		{_T("Celeron(R) CPU 550"), 0, 6, 0x16, 0x1, CPU_MEROM, CPU_ST_A1_G0, CPU_S_SOCKET_P, CPU_F_65, 100}, // Datasheet
		{_T("Celeron(R) CPU 560"), 0, 6, 0x16, 0x1, CPU_MEROM, CPU_ST_A1, CPU_S_SOCKET_P, CPU_F_65, 100},	 // Datasheet
		{_T("Celeron(R) CPU 570"), 0, 6, 0x16, 0x1, CPU_MEROM, CPU_ST_A1, CPU_S_SOCKET_P, CPU_F_65, 100},	 // Datasheet

		{_T("Celeron(R) M CPU 5"), 0, 6, 0x16, 0x1, CPU_MEROM, CPU_ST_A1, CPU_S_SOCKET_M, CPU_F_65, 100}, // Datasheet

		{_T("Core(TM)2 Solo CPU U2"), 0, 6, 0x16, 0x1, CPU_MEROM, CPU_ST_A1, CPU_S_FCBGA6, CPU_F_65, 100}, // Datasheet

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:17
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		/*
		//Pentium
		45nm Wolfdale
		A R0
		LGA775
		GenuineIntel	 0x6	 0x17	 0x6	 Pentium(R) Dual-Core CPU E5200 @ 2.50GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Pentium(R) CPU E5200 @ 2.50GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Pentium(R) CPU E5300 @ 2.60GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Pentium(R) CPU E5400 @ 2.70GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Pentium(R) CPU E6300 @ 2.80GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Pentium III Class
		GenuineIntel	 0x6	 0x17	 0xa	 Pentium(R) Dual-Core CPU E5200 @ 2.50GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Pentium(R) Dual-Core CPU E5300 @ 2.60GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Pentium(R) Dual-Core CPU E5400 @ 2.70GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Pentium(R) Dual-Core CPU E6300 @ 2.80GHz

		CPU manufacturer: GenuineIntel
		CPU Type: Intel(R) Celeron(R) CPU E3200 @ 2.40GHz
		CPUID: Family 6, Model 17, Stepping A

		Penryn
		45nm
		Socket P (478)
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Celeron(R) CPU 900 @ 2.20GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Pentium(R) Dual-Core CPU T4200 @ 2.00GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Pentium(R) Dual-Core CPU T4300 @ 2.10GHz
		*/

		/*
		GenuineIntel, 0x6, 0x17, 0x6, Pentium(R) Dual-Core CPU E2210 @ 2.20GHz,
		GenuineIntel, 0x6, 0x17, 0xa, Pentium(R) Dual-Core CPU E6500 @ 2.93GHz,
		GenuineIntel, 0x6, 0x17, 0xa, Pentium(R) Dual-Core CPU E6600 @ 3.06GHz,
		*/

		{_T("Pentium(R) Dual-Core CPU E"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_R0, CPU_S_LGA775, CPU_F_45, -1},
		{_T("Pentium(R) CPU E"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_R0, CPU_S_LGA775, CPU_F_45, -1},
		{_T("Pentium(R) Dual-Core CPU E"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_R0, CPU_S_LGA775, CPU_F_45, -1},
		{_T("Pentium(R) Dual-Core CPU E"), 0, 6, 0x17, -1, CPU_WOLFDALE, CPU_ST_BLANK, CPU_S_LGA775, CPU_F_45, -1},
		{_T("Pentium(R) CPU E"), 0, 6, 0x17, -1, CPU_WOLFDALE, CPU_ST_BLANK, CPU_S_LGA775, CPU_F_45, -1},
		{_T("Pentium(R) Dual-Core CPU E"), 0, 6, 0x17, -1, CPU_WOLFDALE, CPU_ST_BLANK, CPU_S_LGA775, CPU_F_45, -1},

		{_T("Intel(R) Celeron(R) CPU E3"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_R0, CPU_S_LGA775, CPU_F_45, 74}, // BIT6010140000//BIT6010200001

		/*
		GenuineIntel, 0x6, 0x17, 0xa, Pentium(R) Dual-Core CPU T4400 @ 2.20GHz,
		GenuineIntel, 0x6, 0x17, 0xa, Pentium(R) Dual-Core CPU T4500 @ 2.30GHz,
		GenuineIntel, 0x6, 0x17, 0xa, Celeron(R) Dual-Core CPU T3000 @ 1.80GHz,
		GenuineIntel, 0x6, 0x17, 0xa, Celeron(R) Dual-Core CPU T3100 @ 1.90GHz,
		GenuineIntel, 0x6, 0x17, 0xa, Intel(R) Celeron(R) CPU 723 @ 1.20GHz,
		GenuineIntel, 0x6, 0x17, 0xa, Intel(R) Celeron(R) CPU 743 @ 1.30GHz,

		GenuineIntel, 0x6, 0x17, 0xa, Genuine Intel(R) CPU U2300 @ 1.20GHz,
		GenuineIntel, 0x6, 0x17, 0xa, Genuine Intel(R) CPU U4100 @ 1.30GHz,
		GenuineIntel, 0x6, 0x17, 0xa, Genuine Intel(R) CPU U7300 @ 1.30GHz,
		*/

		{_T("Celeron(R) CPU 7"), 0, 6, 0x17, 0xa, CPU_PENRYN, CPU_ST_R0, CPU_S_BGA956, CPU_F_45, 100},						  // BIT6010200001
		{_T("Celeron(R) CPU 7"), 0, 6, 0x17, -1, CPU_PENRYN, CPU_ST_BLANK, CPU_S_BGA956, CPU_F_45, 100},					  // BIT6010200001
		{_T("Celeron(R) CPU 900"), 0, 6, 0x17, 0xA, CPU_PENRYN, CPU_ST_R0, CPU_S_SOCKET_P_478, CPU_F_45, 105},				  // BIT6010200001
		{_T("Celeron(R) CPU 900"), 0, 6, 0x17, -1, CPU_PENRYN, CPU_ST_BLANK, CPU_S_SOCKET_P_478, CPU_F_45, 105},			  // BIT6010200001
		{_T("Pentium(R) Dual-Core CPU T"), 0, 6, 0x17, 0xA, CPU_PENRYN, CPU_ST_R0, CPU_S_SOCKET_P_478, CPU_F_45, 105},		  // BIT6010200001
		{_T("Pentium(R) Dual-Core CPU T"), 0, 6, 0x17, -1, CPU_PENRYN, CPU_ST_BLANK, CPU_S_SOCKET_P_478, CPU_F_45, 105},	  // BIT6010200001
		{_T("Celeron(R) Dual-Core CPU T3000"), 0, 6, 0x17, 0xA, CPU_PENRYN, CPU_ST_R0, CPU_S_PGA478_BGA479, CPU_F_45, 105},	  // BIT6010200001
		{_T("Celeron(R) Dual-Core CPU T3000"), 0, 6, 0x17, -1, CPU_PENRYN, CPU_ST_BLANK, CPU_S_PGA478_BGA479, CPU_F_45, 105}, // BIT6010200002
		{_T("Celeron(R) Dual-Core CPU T3100"), 0, 6, 0x17, 0xA, CPU_PENRYN, CPU_ST_R0, CPU_S_PGA478_BGA479, CPU_F_45, 105},	  // BIT6010200002
		{_T("Celeron(R) Dual-Core CPU T3100"), 0, 6, 0x17, -1, CPU_PENRYN, CPU_ST_BLANK, CPU_S_PGA478_BGA479, CPU_F_45, 105}, // BIT6010200001
		{_T("Celeron(R) Dual-Core CPU T3300"), 0, 6, 0x17, 0xA, CPU_PENRYN, CPU_ST_R0, CPU_S_SOCKET_P_478, CPU_F_45, 100},	  // BIT6010200001
		{_T("Celeron(R) Dual-Core CPU T3300"), 0, 6, 0x17, -1, CPU_PENRYN, CPU_ST_BLANK, CPU_S_SOCKET_P_478, CPU_F_45, 100},  // BIT6010200001

		/*
		GenuineIntel	 0x6	 0x17	 0x1	 Genuine Intel(R) CPU @ 2.66GHz
		GenuineIntel	 0x6	 0x17	 0x4	 Genuine Intel(R) CPU @ 2.10GHz
		GenuineIntel	 0x6	 0x17	 0x4	 Genuine Intel(R) CPU @ 2.50GHz
		GenuineIntel	 0x6	 0x17	 0x4	 Genuine Intel(R) CPU @ 2.53GHz
		GenuineIntel	 0x6	 0x17	 0x4	 Genuine Intel(R) CPU @ 2.60GHz
		GenuineIntel	 0x6	 0x17	 0x4	 Genuine Intel(R) CPU @ 2.80GHz
		GenuineIntel	 0x6	 0x17	 0x4	 Genuine Intel(R) CPU @ 2.83GHz
		*/

		/*
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU @ 2.53GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU E7200 @ 2.53GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU E7300 @ 2.66GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU E8135 @ 2.40GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU E8200 @ 2.66GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU E8235 @ 2.80GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU E8300 @ 2.83GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU E8335 @ 2.66GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU E8400 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU E8435 @ 3.06GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU E8500 @ 3.16GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU E7400 @ 2.80GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU E7500 @ 2.93GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU E7600 @ 3.06GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU E8135 @ 2.66GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU E8335 @ 2.93GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU E8400 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU E8435 @ 3.06GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU E8500 @ 3.16GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU E8600 @ 3.33GHz

		GenuineIntel, 0x6, 0x17, 0xa, Intel(R) Core(TM)2 CPU E7600 @ 3.06GHz,
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 CPU E7400 @ 2.80GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 CPU E7500 @ 2.93GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 CPU E8400 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 CPU E8500 @ 3.16GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 CPU E8600 @ 3.33GHz
		*/

		//	WOLFDALE - Desktop Dual 45nm Jan 2008
		{_T("Core(TM)2 Duo CPU E7"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_M0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Duo CPU E7"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_R0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Duo CPU E8"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_C0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Duo CPU E8"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_E0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 CPU E7"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_M0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 CPU E7"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_R0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 CPU E8"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_C0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 CPU E8"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_E0, CPU_S_LGA775, CPU_F_45, 100},

		/*
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU L9300 @ 1.60GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU L9400 @ 1.86GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU L9400 @ 1.86GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU L9600 @ 2.13GHz
		*/
		//	Penryn - Laptop 45nm //SL9xxx
		{_T("Core(TM)2 Duo CPU L9"), 0, 6, 0x17, 0x6, CPU_PENRYN, CPU_ST_C0, CPU_S_BGA956, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU L9"), 0, 6, 0x17, 0xA, CPU_PENRYN, CPU_ST_E0, CPU_S_BGA956, CPU_F_45, 105},

		/*
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU P7350 @ 2.00GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU P7370 @ 2.00GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU P7450 @ 2.13GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU P7350 @ 2.00GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU P7450 @ 2.13GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU P7550 @ 2.26GHz

		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU P8400 @ 2.26GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU P8600 @ 2.40GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU P8400 @ 2.26GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU P8600 @ 2.40GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU P8700 @ 2.53GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU P8800 @ 2.66GHz

		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU P9300 @ 2.26GHz	//SP9300
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU P9400 @ 2.40GHz //SP9400
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU P9500 @ 2.53GHz //P9500
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU P9500 @ 2.53GHz //P9500
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU P9600 @ 2.66GHz //P9600
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU P9700 @ 2.80GHz //P9700

		*/
		//	Penryn - Laptop 45nm
		{_T("Core(TM)2 Duo CPU P7"), 0, 6, 0x17, 6, CPU_PENRYN, CPU_ST_C0, CPU_S_SOCKET_P_478, CPU_F_45, 90},
		{_T("Core(TM)2 Duo CPU P7"), 0, 6, 0x17, 0xA, CPU_PENRYN, CPU_ST_R0, CPU_S_SOCKET_P_478, CPU_F_45, 90},
		{_T("Core(TM)2 Duo CPU P8"), 0, 6, 0x17, 6, CPU_PENRYN, CPU_ST_C0, CPU_S_PGA478_BGA479, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU P8"), 0, 6, 0x17, 0xa, CPU_PENRYN, CPU_ST_R0, CPU_S_PGA478_BGA479, CPU_F_45, 105},

		{_T("Core(TM)2 Duo CPU P9500"), 0, 6, 0x17, 0x6, CPU_PENRYN, CPU_ST_C0, CPU_S_SOCKET_P_478, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU P9500"), 0, 6, 0x17, 0xa, CPU_PENRYN, CPU_ST_E0_E1, CPU_S_SOCKET_P_478, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU P9600 @ 2.66"), 0, 6, 0x17, 0x6, CPU_PENRYN, CPU_ST_C0, CPU_S_SOCKET_P_478, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU P9600 @ 2.66"), 0, 6, 0x17, 0xa, CPU_PENRYN, CPU_ST_E0, CPU_S_SOCKET_P_478, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU P9700"), 0, 6, 0x17, 0x6, CPU_PENRYN, CPU_ST_C0, CPU_S_SOCKET_P_478, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU P9700"), 0, 6, 0x17, 0xa, CPU_PENRYN, CPU_ST_E0_E1, CPU_S_SOCKET_P_478, CPU_F_45, 105},

		// small form factor versions //SP9300/SP9400/SP9600
		{_T("Core(TM)2 Duo CPU P9300 @ 2.26"), 0, 6, 0x17, 0x6, CPU_PENRYN, CPU_ST_C0, CPU_S_BGA956, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU P9300 @ 2.26"), 0, 6, 0x17, 0xa, CPU_PENRYN, CPU_ST_E0, CPU_S_BGA956, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU P9400 @ 2.40"), 0, 6, 0x17, 0x6, CPU_PENRYN, CPU_ST_C0, CPU_S_BGA956, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU P9400 @ 2.40"), 0, 6, 0x17, 0xa, CPU_PENRYN, CPU_ST_E0, CPU_S_BGA956, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU P9600 @ 2.53"), 0, 6, 0x17, 0x6, CPU_PENRYN, CPU_ST_C0, CPU_S_BGA956, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU P9600 @ 2.53"), 0, 6, 0x17, 0xa, CPU_PENRYN, CPU_ST_E0, CPU_S_BGA956, CPU_F_45, 105},

		/*
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU T6400 @ 2.00GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU T6500 @ 2.10GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU T6570 @ 2.10GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU T6600 @ 2.20GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU T6670 @ 2.20GHz

		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU T8100 @ 2.10GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU T8300 @ 2.40GHz

		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU T9300 @ 2.50GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU T9400 @ 2.53GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU T9500 @ 2.60GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU T9600 @ 2.80GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU T9400 @ 2.53GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU T9550 @ 2.66GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU T9600 @ 2.80GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU T9800 @ 2.93GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU T9900 @ 3.06GHz

		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 CPU T6400 @ 2.00GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 CPU T6570 @ 2.10GHz

		*/
		//	Penryn - Laptop 45nm
		{_T("Core(TM)2 CPU T6"), 0, 6, 0x17, 0xa, CPU_PENRYN, CPU_ST_R0, CPU_S_PGA478_BGA479, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU T6"), 0, 6, 0x17, 0xa, CPU_PENRYN, CPU_ST_R0, CPU_S_PGA478_BGA479, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU T8"), 0, 6, 0x17, 0x6, CPU_PENRYN, CPU_ST_M0_C0, CPU_S_PGA478_BGA479, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU T9"), 0, 6, 0x17, 0xA, CPU_PENRYN, CPU_ST_E0, CPU_S_PGA478_BGA479, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU T9"), 0, 6, 0x17, 0x6, CPU_PENRYN, CPU_ST_M0_C0, CPU_S_PGA478_BGA479, CPU_F_45, 105},

		/*
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU U9300 @ 1.20GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Duo CPU U9400 @ 1.40GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU U9400 @ 1.40GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Duo CPU U9600 @ 1.60GHz
		*/
		//	Penryn - Laptop 45nm //SU9300/SU9400/SU9600
		{_T("Core(TM)2 Duo CPU U9300"), 0, 6, 0x17, 0x6, CPU_PENRYN, CPU_ST_M0, CPU_S_BGA956, CPU_F_45, 105}, // Correction //BIT6010060002
		{_T("Core(TM)2 Duo CPU U9400"), 0, 6, 0x17, 0x6, CPU_PENRYN, CPU_ST_M0, CPU_S_BGA956, CPU_F_45, 105},
		{_T("Core(TM)2 Duo CPU U9400"), 0, 6, 0x17, 0xa, CPU_PENRYN, CPU_ST_R0, CPU_S_BGA956, CPU_F_45, 105}, // Correction //BIT6010060002
		{_T("Core(TM)2 Duo CPU U9600"), 0, 6, 0x17, 0xa, CPU_PENRYN, CPU_ST_R0, CPU_S_BGA956, CPU_F_45, 105},

		/*
		GenuineIntel	 0x6	 0x17	 0xa	 Genuine Intel(R) CPU U2700 @ 1.30GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Solo CPU U3500 @ 1.40GHz
		*/
		//	Penryn - Laptop 45nm	//SUxxxx
		{_T("Core(TM)2 Solo CPU U3"), 0, 6, 0x17, 0x6, CPU_PENRYN, CPU_ST_M0, CPU_S_BGA956, CPU_F_45, 100},
		{_T("Core(TM)2 Solo CPU U3"), 0, 6, 0x17, 0xA, CPU_PENRYN, CPU_ST_R0, CPU_S_BGA956, CPU_F_45, 100},

		/*
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Extreme CPU @ 2.26GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Extreme CPU X9000 @ 2.80GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Extreme CPU X9100 @ 3.06GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Extreme CPU X9650 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Extreme CPU X9770 @ 3.20GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Extreme CPU X9775 @ 3.20GHz
		GenuineIntel	 0x6	 0x17	 0x7	 Intel(R) Core(TM)2 Extreme CPU X9650 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0x7	 Intel(R) Core(TM)2 Extreme CPU X9770 @ 3.20GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Extreme CPU @ 2.26GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Extreme CPU Q9300 @ 2.53GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Extreme CPU X9100 @ 3.06GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Extreme CPU X9750 @ 3.16GHz
		*/
		//	Yorkfield - Desktop Quad 45nm Mar 2008
		{_T("Core(TM)2 Extreme CPU X9775"), 0, 6, 0x17, 0x6, CPU_YORKFIELD, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, 85},
		{_T("Core(TM)2 Extreme CPU X9770"), 0, 6, 0x17, 0x7, CPU_YORKFIELD, CPU_ST_C1, CPU_S_LGA775, CPU_F_45, 85},
		{_T("Core(TM)2 Extreme CPU X9650"), 0, 6, 0x17, 0x6, CPU_YORKFIELD, CPU_ST_C0, CPU_S_LGA775, CPU_F_45, 95},
		{_T("Core(TM)2 Extreme CPU X9650"), 0, 6, 0x17, 0x7, CPU_YORKFIELD, CPU_ST_C1, CPU_S_LGA775, CPU_F_45, 95},
		{_T("Core(TM)2 Extreme CPU Q9300"), 0, 6, 0x17, -1, CPU_YORKFIELD, CPU_ST_BLANK, CPU_S_LGA775, CPU_F_45, -1},

		//	Penryn - Laptop 45nm
		{_T("Core(TM)2 Extreme CPU X9100"), 0, 6, 0x17, 0x6, CPU_PENRYN, CPU_ST_C0, CPU_S_SOCKET_P_478, CPU_F_45, 105}, // Datasheet X9100
		{_T("Core(TM)2 Extreme CPU X9000"), 0, 6, 0x17, 0x6, CPU_PENRYN, CPU_ST_C0, CPU_S_SOCKET_P_478, CPU_F_45, 105}, // http://download.intel.com/design/mobile/datashts/31891401.pdf
		{_T("Core(TM)2 Extreme CPU X9100"), 0, 6, 0x17, 0xa, CPU_PENRYN, CPU_ST_E0, CPU_S_SOCKET_P_478, CPU_F_45, 105}, // Datasheet X9100
		{_T("Core(TM)2 Extreme CPU X9000"), 0, 6, 0x17, 0xa, CPU_PENRYN, CPU_ST_E0, CPU_S_SOCKET_P_478, CPU_F_45, 105}, // http://download.intel.com/design/mobile/datashts/31891401.pdf

		/*
		GenuineIntel	 0x6	 0x17	 0x7	 Intel(R) Core(TM)2 Quad CPU Q8200 @ 2.33GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Quad CPU Q8200 @ 2.33GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Quad CPU Q8300 @ 2.50GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Quad CPU Q8400 @ 2.66GHz

		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Quad CPU Q9000 @ 2.00GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Quad CPU Q9100 @ 2.26GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Quad CPU Q9300 @ 2.50GHz
		GenuineIntel	 0x6	 0x17	 0x7	 Intel(R) Core(TM)2 Quad CPU Q9300 @ 2.50GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Quad CPU Q9400 @ 2.66GHz

		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Quad CPU Q9450 @ 2.66GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Core(TM)2 Quad CPU Q9550 @ 2.83GHz
		GenuineIntel	 0x6	 0x17	 0x7	 Intel(R) Core(TM)2 Quad CPU Q9450 @ 2.66GHz
		GenuineIntel	 0x6	 0x17	 0x7	 Intel(R) Core(TM)2 Quad CPU Q9550 @ 2.83GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Quad CPU Q9550 @ 2.83GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Core(TM)2 Quad CPU Q9650 @ 3.00GHz

		GenuineIntel, 0x6, 0x17, 0xa, Intel(R) Core(TM)2 Quad CPU Q9505 @ 2.83GHz,
		*/
		//	Yorkfield - Desktop Quad 45nm Mar 2008
		{_T("Core(TM)2 Quad CPU Q8"), 0, 6, 0x17, 0x6, CPU_YORKFIELD, CPU_ST_C0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q8"), 0, 6, 0x17, 0x7, CPU_YORKFIELD, CPU_ST_M1, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q8"), 0, 6, 0x17, 0xA, CPU_YORKFIELD, CPU_ST_R0, CPU_S_LGA775, CPU_F_45, 100},

		{_T("Core(TM)2 Quad CPU Q9000"), 0, 6, 0x17, 0x6, CPU_YORKFIELD, CPU_ST_C0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q9000"), 0, 6, 0x17, 0xA, CPU_YORKFIELD, CPU_ST_R0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q9100"), 0, 6, 0x17, 0x6, CPU_YORKFIELD, CPU_ST_C0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q9100"), 0, 6, 0x17, 0xA, CPU_YORKFIELD, CPU_ST_R0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q9200"), 0, 6, 0x17, 0x6, CPU_YORKFIELD, CPU_ST_C0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q9200"), 0, 6, 0x17, 0xA, CPU_YORKFIELD, CPU_ST_R0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q9300"), 0, 6, 0x17, 0x6, CPU_YORKFIELD, CPU_ST_C0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q9300"), 0, 6, 0x17, 0xA, CPU_YORKFIELD, CPU_ST_R0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q9400"), 0, 6, 0x17, 0x6, CPU_YORKFIELD, CPU_ST_C0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q9400"), 0, 6, 0x17, 0xA, CPU_YORKFIELD, CPU_ST_R0, CPU_S_LGA775, CPU_F_45, 100},

		{_T("Core(TM)2 Quad CPU Q9450"), 0, 6, 0x17, 0x6, CPU_YORKFIELD, CPU_ST_C0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q9450"), 0, 6, 0x17, 0x7, CPU_YORKFIELD, CPU_ST_C1, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q9450"), 0, 6, 0x17, 0xA, CPU_YORKFIELD, CPU_ST_E0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q9550"), 0, 6, 0x17, 0x6, CPU_YORKFIELD, CPU_ST_C0, CPU_S_LGA775, CPU_F_45, 100},

		{_T("Core(TM)2 Quad CPU Q9505"), 0, 6, 0x17, 0xA, CPU_YORKFIELD, CPU_ST_R0, CPU_S_LGA775, CPU_F_45, -1},

		{_T("Core(TM)2 Quad CPU Q9550"), 0, 6, 0x17, 0x7, CPU_YORKFIELD, CPU_ST_C1, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q9550"), 0, 6, 0x17, 0xA, CPU_YORKFIELD, CPU_ST_E0, CPU_S_LGA775, CPU_F_45, 100},
		{_T("Core(TM)2 Quad CPU Q9650"), 0, 6, 0x17, 0xA, CPU_YORKFIELD, CPU_ST_E0, CPU_S_LGA775, CPU_F_45, 100},

		/*
		//XEON
		//XEON 3100 series
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU E3110 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU E3110 @ 3.00GHz
		//XEON 3300 series
		GenuineIntel	 0x6	 0x17	 0x7	 Intel(R) Xeon(R) CPU X3350 @ 2.66GHz
		GenuineIntel	 0x6	 0x17	 0x7	 Intel(R) Xeon(R) CPU X3360 @ 2.83GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU X3350 @ 2.66GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU X3360 @ 2.83GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU X3370 @ 3.00GHz
		//XEON 5200 series
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU L5238 @ 2.66GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU L5240 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU X5260 @ 3.33GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU X5272 @ 3.40GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU X5260 @ 3.33GHz
		//XEON 5400 series
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU E5405 @ 2.00GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU E5410 @ 2.33GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU E5420 @ 2.50GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU E5430 @ 2.66GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU E5440 @ 2.83GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU E5450 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU E5462 @ 2.80GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU E5472 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU L5410 @ 2.33GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU L5420 @ 2.50GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU X5450 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU X5460 @ 3.16GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU X5472 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0x6	 Intel(R) Xeon(R) CPU X5482 @ 3.20GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU E5405 @ 2.00GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU E5410 @ 2.33GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU E5420 @ 2.50GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU E5430 @ 2.66GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU E5440 @ 2.83GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU E5450 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU E5472 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU X5450 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU X5460 @ 3.16GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU X5470 @ 3.33GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU X5472 @ 3.00GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU X5482 @ 3.20GHz
		GenuineIntel	 0x6	 0x17	 0xa	 Intel(R) Xeon(R) CPU X5492 @ 3.40GHz
		*/
		{_T("Xeon(R) CPU L3014"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, 95},

		{_T("Xeon(R) CPU E31"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_E0, CPU_S_LGA775, CPU_F_45, 95},
		{_T("Xeon(R) CPU E31"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_C0, CPU_S_LGA775, CPU_F_45, 95},

		{_T("Xeon(R) CPU X33"), 0, 6, 0x17, 0x6, CPU_YORKFIELD, CPU_ST_M0, CPU_S_LGA775, CPU_F_45, 95},
		{_T("Xeon(R) CPU X33"), 0, 6, 0x17, 0x7, CPU_YORKFIELD, CPU_ST_C1, CPU_S_LGA775, CPU_F_45, 95},
		{_T("Xeon(R) CPU X33"), 0, 6, 0x17, 0xA, CPU_YORKFIELD, CPU_ST_E0, CPU_S_LGA775, CPU_F_45, 95},
		{_T("Xeon(R) CPU L33"), 0, 6, 0x17, 0xA, CPU_YORKFIELD, CPU_ST_E0, CPU_S_LGA775, CPU_F_45, 95},

		{_T("Xeon(R) CPU X52"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, 90},
		{_T("Xeon(R) CPU X52"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, 90},
		{_T("Xeon(R) CPU L5240"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, 70},
		{_T("Xeon(R) CPU L5240"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, 70},
		{_T("Xeon(R) CPU L5238"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, 95},
		{_T("Xeon(R) CPU L5238"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, 95},
		{_T("Xeon(R) CPU L5215"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, 95},
		{_T("Xeon(R) CPU L5215"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, 95},
		{_T("Xeon(R) CPU E5200"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_M0, CPU_S_LGA775, CPU_F_45, -1},
		{_T("Xeon(R) CPU E5240"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, 90},
		{_T("Xeon(R) CPU E5240"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, 90},
		{_T("Xeon(R) CPU E5220"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, -1},
		{_T("Xeon(R) CPU E5220"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, -1},
		{_T("Xeon(R) CPU E5205"), 0, 6, 0x17, 0xA, CPU_WOLFDALE, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, -1},
		{_T("Xeon(R) CPU E5205"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, -1},
		{_T("Xeon(R) CPU E52"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, -1},
		{_T("Xeon(R) CPU L52"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, -1},
		{_T("Xeon(R) CPU X52"), 0, 6, 0x17, 0x6, CPU_WOLFDALE, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, -1},
		{_T("Xeon(R) CPU E52"), 0, 6, 0x17, 0xa, CPU_WOLFDALE, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, -1},
		{_T("Xeon(R) CPU L52"), 0, 6, 0x17, 0xa, CPU_WOLFDALE, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, -1},
		{_T("Xeon(R) CPU X52"), 0, 6, 0x17, 0xa, CPU_WOLFDALE, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, -1},

		{_T("Xeon(R) CPU X54"), 0, 6, 0x17, 0xA, CPU_HARPERTOWN, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, 85},
		{_T("Xeon(R) CPU X54"), 0, 6, 0x17, 0x6, CPU_HARPERTOWN, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, 85},
		{_T("Xeon(R) CPU L5430"), 0, 6, 0x17, 0xA, CPU_HARPERTOWN, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, 85},
		{_T("Xeon(R) CPU L5430"), 0, 6, 0x17, 0x6, CPU_HARPERTOWN, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, 70},
		{_T("Xeon(R) CPU L5420"), 0, 6, 0x17, 0xA, CPU_HARPERTOWN, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, 70},
		{_T("Xeon(R) CPU L5420"), 0, 6, 0x17, 0x6, CPU_HARPERTOWN, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, 70},
		{_T("Xeon(R) CPU L5410"), 0, 6, 0x17, 0xA, CPU_HARPERTOWN, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, 70},
		{_T("Xeon(R) CPU L5410"), 0, 6, 0x17, 0x6, CPU_HARPERTOWN, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, 70},
		{_T("Xeon(R) CPU L5408"), 0, 6, 0x17, 0x6, CPU_HARPERTOWN, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, 95},
		{_T("Xeon(R) CPU L54"), 0, 6, 0x17, 0xA, CPU_HARPERTOWN, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, -1},
		{_T("Xeon(R) CPU L54"), 0, 6, 0x17, 0x6, CPU_HARPERTOWN, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, -1},
		{_T("Xeon(R) CPU E54"), 0, 6, 0x17, 0xA, CPU_HARPERTOWN, CPU_ST_E0, CPU_S_LGA771, CPU_F_45, 85},
		{_T("Xeon(R) CPU E54"), 0, 6, 0x17, 0x6, CPU_HARPERTOWN, CPU_ST_C0, CPU_S_LGA771, CPU_F_45, 85},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:1A
		/*
		GenuineIntel	 0x6	 0x1a	 0x2	 Genuine Intel(R) CPU @ 0000 @ 2.13GHz
		GenuineIntel	 0x6	 0x1a	 0x2	 Genuine Intel(R) CPU @ 0000 @ 2.40GHz
		GenuineIntel	 0x6	 0x1a	 0x2	 Genuine Intel(R) CPU @ 0000 @ 2.67GHz
		GenuineIntel	 0x6	 0x1a	 0x2	 Genuine Intel(R) CPU @ 0000 @ 2.93GHz
		GenuineIntel	 0x6	 0x1a	 0x4	 Genuine Intel(R) CPU 000 @ 2.67GHz
		GenuineIntel	 0x6	 0x1a	 0x4	 Genuine Intel(R) CPU 000 @ 2.80GHz
		GenuineIntel	 0x6	 0x1a	 0x4	 Genuine Intel(R) CPU 000 @ 2.93GHz
		GenuineIntel	 0x6	 0x1a	 0x4	 Genuine Intel(R) CPU 000 @ 3.20GHz
		GenuineIntel	 0x6	 0x1a	 0x4	 Intel(R) Core(TM) i7 CPU 920 @ 2.67GHz
		GenuineIntel	 0x6	 0x1a	 0x4	 Intel(R) Core(TM) i7 CPU 940 @ 2.93GHz
		GenuineIntel	 0x6	 0x1a	 0x4	 Intel(R) Core(TM) i7 CPU 965 @ 3.20GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Core(TM) CPU 965 @ 3.20GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Core(TM) i7 CPU @ 9200 @ 2.67GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Core(TM) i7 CPU @ 9500 @ 3.07GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Core(TM) i7 CPU @ 9750 @ 3.33GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Core(TM) i7 CPU 920 @ 2.67GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Core(TM) i7 CPU 950 @ 3.07GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Core(TM) i7 CPU 965 @ 3.20GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Core(TM) i7 CPU 975 @ 3.33GHz
		GenuineIntel	 0x6	 0x1a	 0x4	 Intel(R) Xeon(R) CPU E5504 @ 2.00GHz
		GenuineIntel	 0x6	 0x1a	 0x4	 Intel(R) Xeon(R) CPU W 570 @ 3.20GHz
		GenuineIntel	 0x6	 0x1a	 0x4	 Intel(R) Xeon(R) CPU W5580 @ 3.20GHz
		GenuineIntel	 0x6	 0x1a	 0x4	 Intel(R) Xeon(R) CPU X5550 @ 2.67GHz
		GenuineIntel	 0x6	 0x1a	 0x4	 Intel(R) Xeon(R) CPU X5560 @ 2.80GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU 000 @ 3.20GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU E5502 @ 1.87GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU E5504 @ 2.00GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU E5506 @ 2.13GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU E5520 @ 2.27GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU E5530 @ 2.40GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU E5540 @ 2.53GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU W3503 @ 2.40GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU W3505 @ 2.53GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU W3520 @ 2.67GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU W3540 @ 2.93GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU W3570 @ 3.20GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU W5580 @ 3.20GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU X5550 @ 2.67GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU X5560 @ 2.80GHz
		GenuineIntel	 0x6	 0x1a	 0x5	 Intel(R) Xeon(R) CPU X5570 @ 2.93GHz
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		//{_T("Genuine Intel(R) CPU @ 0000"),0,6,0x1A,0x2,CPU_BLOOMFIELD,_T("B0"),CPU_S_LGA1366,CPU_F_45,-1},		//debugging
		//{_T("Genuine Intel(R) CPU 000 @"),0,6,0x1A,0x4,CPU_BLOOMFIELD,CPU_ST_C0_C1,CPU_S_LGA1366,CPU_F_45,-1},		//debugging
		{_T("Core(TM) i7 CPU 9"), 0, 6, 0x1A, 4, CPU_BLOOMFIELD, CPU_ST_C0_C1, CPU_S_LGA1366, CPU_F_45, -1},
		{_T("Core(TM) i7 CPU 9"), 0, 6, 0x1A, 5, CPU_BLOOMFIELD, CPU_ST_D0, CPU_S_LGA1366, CPU_F_45, -1},
		{_T("Core(TM) i7 CPU 9"), 0, 6, 0x1A, -1, CPU_BLOOMFIELD, CPU_ST_BLANK, CPU_S_LGA1366, CPU_F_45, -1},

		{_T("Xeon(R) CPU W35"), 0, 6, 0x1A, 4, CPU_BLOOMFIELD, CPU_ST_C0_C1, CPU_S_LGA1366, CPU_F_45, -1},
		{_T("Xeon(R) CPU W35"), 0, 6, 0x1A, 5, CPU_BLOOMFIELD, CPU_ST_D0, CPU_S_LGA1366, CPU_F_45, -1},

		{_T("Xeon(R) CPU E55"), 0, 6, 0x1A, 4, CPU_GAINESTOWN, CPU_ST_C0_C1, CPU_S_LGA1366, CPU_F_45, -1},
		{_T("Xeon(R) CPU E55"), 0, 6, 0x1A, 5, CPU_GAINESTOWN, CPU_ST_D0, CPU_S_LGA1366, CPU_F_45, -1},
		{_T("Xeon(R) CPU E55"), 0, 6, 0x1A, -1, CPU_GAINESTOWN, CPU_ST_BLANK, CPU_S_LGA1366, CPU_F_45, -1},
		{_T("Xeon(R) CPU L55"), 0, 6, 0x1A, 4, CPU_GAINESTOWN, CPU_ST_C0_C1, CPU_S_LGA1366, CPU_F_45, -1},
		{_T("Xeon(R) CPU L55"), 0, 6, 0x1A, 5, CPU_GAINESTOWN, CPU_ST_D0, CPU_S_LGA1366, CPU_F_45, -1},
		{_T("Xeon(R) CPU L55"), 0, 6, 0x1A, -1, CPU_GAINESTOWN, CPU_ST_BLANK, CPU_S_LGA1366, CPU_F_45, -1},
		{_T("Xeon(R) CPU X55"), 0, 6, 0x1A, 4, CPU_GAINESTOWN, CPU_ST_C0_C1, CPU_S_LGA1366, CPU_F_45, -1},
		{_T("Xeon(R) CPU X55"), 0, 6, 0x1A, 5, CPU_GAINESTOWN, CPU_ST_D0, CPU_S_LGA1366, CPU_F_45, -1},
		{_T("Xeon(R) CPU X55"), 0, 6, 0x1A, -1, CPU_GAINESTOWN, CPU_ST_BLANK, CPU_S_LGA1366, CPU_F_45, -1},
		{_T("Xeon(R) CPU W55"), 0, 6, 0x1A, 4, CPU_GAINESTOWN, CPU_ST_C0_C1, CPU_S_LGA1366, CPU_F_45, -1},
		{_T("Xeon(R) CPU W55"), 0, 6, 0x1A, 5, CPU_GAINESTOWN, CPU_ST_D0, CPU_S_LGA1366, CPU_F_45, -1},
		{_T("Xeon(R) CPU W55"), 0, 6, 0x1A, -1, CPU_GAINESTOWN, CPU_ST_BLANK, CPU_S_LGA1366, CPU_F_45, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:1C ATOM
		/*
		GenuineIntel	 0x6	 0x1c	 0x2	 Genuine Intel(R) CPU 230 @ 1.60GHz
		GenuineIntel	 0x6	 0x1c	 0x2	 Genuine Intel(R) CPU N270 @ 1.60GHz
		GenuineIntel	 0x6	 0x1c	 0x2	 Intel(R) Atom(TM) CPU 230 @ 1.60GHz
		GenuineIntel	 0x6	 0x1c	 0x2	 Intel(R) Atom(TM) CPU 330 @ 1.60GHz
		GenuineIntel	 0x6	 0x1c	 0x2	 Intel(R) Atom(TM) CPU N270 @ 1.60GHz
		GenuineIntel	 0x6	 0x1c	 0x2	 Intel(R) Atom(TM) CPU N280 @ 1.66GHz
		GenuineIntel	 0x6	 0x1c	 0x2	 Intel(R) Atom(TM) CPU Z520 @ 1.33GHz
		GenuineIntel	 0x6	 0x1c	 0x2	 Intel(R) Atom(TM) CPU Z530 @ 1.60GHz
		GenuineIntel	 0x6	 0x1c	 0x2	 Intel(R) Core(TM) CPU N270 @ 1.60GHz
		*/

		/*
		GenuineIntel, 0x6, 0x1c, 0xa, Intel(R) Atom(TM) CPU D510 @ 1.66GHz,
		GenuineIntel, 0x6, 0x1c, 0xa, Intel(R) Atom(TM) CPU N450 @ 1.66GHz,
		*/

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Intel(R) CPU 230"), 0, 6, 0x1C, 0x2, CPU_DIAMONDVILLE, CPU_ST_C0, CPU_S_PBGA437, CPU_F_45, 85},		   // Tj = 90C - from Intel site  http://ark.intel.com/Product.aspx?id=35635
		{_T("Intel(R) Atom(TM) CPU  230"), 0, 6, 0x1C, 0x2, CPU_DIAMONDVILLE, CPU_ST_C0, CPU_S_PBGA437, CPU_F_45, 85}, // Tj = 90C - from Intel site  http://ark.intel.com/Product.aspx?id=35635
		{_T("Intel(R) Atom(TM) CPU  330"), 0, 6, 0x1C, 0x2, CPU_DIAMONDVILLE, CPU_ST_C0, CPU_S_PBGA437, CPU_F_45, 85}, // http://ark.intel.com/Product.aspx?id=35641
		{_T("Intel(R) CPU N270"), 0, 6, 0x1C, 0x2, CPU_DIAMONDVILLE, CPU_ST_C0, CPU_S_PBGA437, CPU_F_45, 90},		   // Atom N270 series: Tj = 90C - from datasheet
		{_T("Intel(R) Atom(TM) CPU N270"), 0, 6, 0x1C, 0x2, CPU_DIAMONDVILLE, CPU_ST_C0, CPU_S_PBGA437, CPU_F_45, 90}, // Atom N270 series: Tj = 90C - from datasheet
		{_T("Intel(R) Core(TM) CPU N270"), 0, 6, 0x1C, 0x2, CPU_DIAMONDVILLE, CPU_ST_C0, CPU_S_PBGA437, CPU_F_45, 90}, // Atom N270 series: Tj = 90C - from datasheet
		{_T("Intel(R) Atom(TM) CPU N280"), 0, 6, 0x1C, 0x2, CPU_DIAMONDVILLE, CPU_ST_C0, CPU_S_PBGA437, CPU_F_45, 90}, // Tj = 90C - from Intel site  //http://ark.intel.com/Product.aspx?id=41411

		{_T("Intel(R) Atom(TM) CPU Z5"), 0, 6, 0x1C, 0x2, CPU_SILVERTHORNE, CPU_ST_C0, CPU_S_PBGA441, CPU_F_45, 90},

		{_T("Atom(TM) CPU N4"), 0, 6, 0x1C, 0xa, CPU_PINEVIEW, CPU_ST_A0, CPU_FCBGA559, CPU_F_45, 100},	  // BIT6010200001
		{_T("Atom(TM) CPU D510"), 0, 6, 0x1C, 0xa, CPU_PINEVIEW, CPU_ST_B0, CPU_FCBGA559, CPU_F_45, 100}, // BIT6010200001

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:1D
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Xeon(R) CPU X74"), 0, 6, 0x1D, 0x1, CPU_DUNNINGTON, CPU_ST_A1, CPU_S_PGA604, CPU_F_45, 80},
		{_T("Xeon(R) CPU L74"), 0, 6, 0x1D, 0x1, CPU_DUNNINGTON, CPU_ST_A1, CPU_S_PGA604, CPU_F_45, 80},
		{_T("Xeon(R) CPU E74"), 0, 6, 0x1D, 0x1, CPU_DUNNINGTON, CPU_ST_A1, CPU_S_PGA604, CPU_F_45, 90},
		{_T("Xeon(R) CPU X74"), 0, 6, 0x1D, -1, CPU_DUNNINGTON, CPU_ST_BLANK, CPU_S_PGA604, CPU_F_45, 80},
		{_T("Xeon(R) CPU L74"), 0, 6, 0x1D, -1, CPU_DUNNINGTON, CPU_ST_BLANK, CPU_S_PGA604, CPU_F_45, 80},
		{_T("Xeon(R) CPU E74"), 0, 6, 0x1D, -1, CPU_DUNNINGTON, CPU_ST_BLANK, CPU_S_PGA604, CPU_F_45, 90},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:1e
		/*
		GenuineIntel	 0x6	 0x1e	 0x5	 Intel(R) Core(TM) CPU 870 @ 2.93GHz
		GenuineIntel, 0x6, 0x1e, 0x5, Intel(R) Core(TM) CPU 750 @ 2.67GHz,
		GenuineIntel, 0x6, 0x1e, 0x5, Intel(R) Core(TM) CPU 860 @ 2.80GHz,
		GenuineIntel, 0x6, 0x1e, 0x5, Intel(R) Core(TM) i5 CPU 750 @ 2.67GHz,
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Core(TM) i7 CPU 8"), 0, 6, 0x1e, 0x5, CPU_LYNNFIELD, CPU_ST_B1, CPU_S_LGA1156, CPU_F_45, -1},
		{_T("Core(TM) CPU 8"), 0, 6, 0x1e, 0x5, CPU_LYNNFIELD, CPU_ST_B1, CPU_S_LGA1156, CPU_F_45, -1},
		{_T("Core(TM) CPU 8"), 0, 6, 0x1e, -1, CPU_LYNNFIELD, CPU_ST_BLANK, CPU_S_LGA1156, CPU_F_45, -1},
		{_T("Core(TM) CPU 7"), 0, 6, 0x1e, 0x5, CPU_LYNNFIELD, CPU_ST_B1, CPU_S_LGA1156, CPU_F_45, -1},		 // BIT601014000
		{_T("Core(TM) i5 CPU 7"), 0, 6, 0x1e, 0x5, CPU_LYNNFIELD, CPU_ST_B1, CPU_S_LGA1156, CPU_F_45, -1},	 // BIT601014000
		{_T("Core(TM) i5 CPU S 7"), 0, 6, 0x1e, 0x5, CPU_LYNNFIELD, CPU_ST_B1, CPU_S_LGA1156, CPU_F_45, -1}, // SI101029
		{_T("Core(TM) i7 CPU K 8"), 0, 6, 0x1e, 0x5, CPU_LYNNFIELD, CPU_ST_B1, CPU_S_LGA1156, CPU_F_45, -1}, // BIT6010250003
		{_T("Core(TM) i7 CPU K 8"), 0, 6, 0x1e, 0x5, CPU_LYNNFIELD, CPU_ST_B1, CPU_S_LGA1156, CPU_F_45, -1}, // BIT6010250003

		{_T("Xeon(R) CPU X34"), 0, 6, 0x1e, 0x5, CPU_LYNNFIELD, CPU_ST_B1, CPU_S_LGA1156, CPU_F_45, -1}, // BIT6010170000

		{_T("Core(TM) i7 CPU Q 7"), 0, 6, 0x1e, 0x5, CPU_CLARKSFIELD, CPU_ST_B1, CPU_S_989, CPU_F_45, -1}, // BIT6010250003
		{_T("Core(TM) i7 CPU Q 8"), 0, 6, 0x1e, 0x5, CPU_CLARKSFIELD, CPU_ST_B1, CPU_S_989, CPU_F_45, -1}, // BIT6010250003
		{_T("Core(TM) i7 CPU X 9"), 0, 6, 0x1e, 0x5, CPU_CLARKSFIELD, CPU_ST_B1, CPU_S_989, CPU_F_45, -1}, // BIT6010250003

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:25

		/*
		GenuineIntel, 0x6, 0x25, 0x2, Intel(R) Pentium(R) CPU G6950 @ 2.80GHz,
		GenuineIntel, 0x6, 0x25, 0x2, Intel(R) Core(TM) i3 CPU 530 @ 2.93GHz,
		GenuineIntel, 0x6, 0x25, 0x2, Intel(R) Core(TM) i5 CPU 650 @ 3.20GHz,
		GenuineIntel, 0x6, 0x25, 0x2, Intel(R) Core(TM) i5 CPU 661 @ 3.33GHz,
		GenuineIntel, 0x6, 0x25, 0x2, Intel(R) Core(TM) i3 CPU 540 @ 3.07GHz,

		GenuineIntel, 0x6, 0x25, 0x2, Intel(R) Core(TM) i7 CPU M 620 @ 2.67GHz,
		GenuineIntel, 0x6, 0x25, 0x2, Intel(R) Core(TM) i5 CPU M 430 @ 2.27GHz,
		GenuineIntel, 0x6, 0x25, 0x2, Intel(R) Core(TM) i5 CPU M 520 @ 2.40GHz,
		GenuineIntel, 0x6, 0x25, 0x2, Intel(R) Core(TM) i5 CPU M 540 @ 2.53GHz,
		GenuineIntel, 0x6, 0x25, 0x2, Intel(R) Core(TM) i3 CPU M 330 @ 2.13GHz,
		GenuineIntel, 0x6, 0x25, 0x2, Intel(R) Core(TM) i3 CPU M 350 @ 2.27GHz,
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Clarkdale - desktop
		{_T("Pentium(R) CPU G"), 0, 6, 0x25, 0x2, CPU_CLARKDALE, CPU_ST_C2, CPU_S_LGA1156, CPU_F_32, -1},	 // BIT6010190001//BIT6010200001
		{_T("Core(TM) i5 CPU 6"), 0, 6, 0x25, 0x2, CPU_CLARKDALE, CPU_ST_C2, CPU_S_LGA1156, CPU_F_32, -1},	 // BIT6010200001
		{_T("Core(TM) i5 CPU 6"), 0, 6, 0x25, -1, CPU_CLARKDALE, CPU_ST_BLANK, CPU_S_LGA1156, CPU_F_32, -1}, // BIT6010190001
		{_T("Core(TM) i3 CPU 5"), 0, 6, 0x25, 0x2, CPU_CLARKDALE, CPU_ST_C2, CPU_S_LGA1156, CPU_F_32, -1},	 // BIT6010200001
		{_T("Core(TM) i3 CPU 5"), 0, 6, 0x25, -1, CPU_CLARKDALE, CPU_ST_BLANK, CPU_S_LGA1156, CPU_F_32, -1}, // BIT6010190001

		{_T("Core(TM) i5 CPU K 6"), 0, 6, 0x25, 0x5, CPU_CLARKDALE, CPU_ST_K0, CPU_S_LGA1156, CPU_F_32, -1}, // BIT6010190001

		// Arrandale - mobile
		{_T("Core(TM) i7 CPU M"), 0, 6, 0x25, 0x2, CPU_ARRANDALE, CPU_ST_C2, CPU_BGA1288_PGA988, CPU_F_32, 105},	 // BIT6010200001
		{_T("Core(TM) i7 CPU M"), 0, 6, 0x25, -1, CPU_ARRANDALE, CPU_ST_BLANK, CPU_BGA1288_PGA988, CPU_F_32, 105},	 // BIT6010200001
		{_T("Core(TM) i5 CPU M 5"), 0, 6, 0x25, 0x2, CPU_ARRANDALE, CPU_ST_C2, CPU_BGA1288_PGA988, CPU_F_32, 105},	 // BIT6010200001
		{_T("Core(TM) i5 CPU M 5"), 0, 6, 0x25, -1, CPU_ARRANDALE, CPU_ST_BLANK, CPU_BGA1288_PGA988, CPU_F_32, 105}, // BIT6010200001
		{_T("Core(TM) i5 CPU M 4"), 0, 6, 0x25, 0x2, CPU_ARRANDALE, CPU_ST_C2, CPU_BGA1288_PGA988, CPU_F_32, 105},	 // BIT6010200001
		{_T("Core(TM) i5 CPU M 4"), 0, 6, 0x25, -1, CPU_ARRANDALE, CPU_ST_BLANK, CPU_BGA1288_PGA988, CPU_F_32, 105}, // BIT6010200001
		{_T("Core(TM) i3 CPU M 3"), 0, 6, 0x25, 0x2, CPU_ARRANDALE, CPU_ST_C2, CPU_BGA1288_PGA988, CPU_F_32, -1},	 // BIT6010200001
		{_T("Core(TM) i3 CPU M 3"), 0, 6, 0x25, -1, CPU_ARRANDALE, CPU_ST_BLANK, CPU_BGA1288_PGA988, CPU_F_32, -1},	 // BIT6010200001

		{_T("Core(TM) i7 CPU U 6"), 0, 6, 0x25, -1, CPU_ARRANDALE, CPU_ST_BLANK, CPU_BGA1288, CPU_F_32, -1},		// SI101022
		{_T("Core(TM) i5 CPU U 5"), 0, 6, 0x25, -1, CPU_ARRANDALE, CPU_ST_BLANK, CPU_BGA1288, CPU_F_32, -1},		// SI101022
		{_T("Core(TM) i5 CPU U 4"), 0, 6, 0x25, -1, CPU_ARRANDALE, CPU_ST_BLANK, CPU_BGA1288_PGA988, CPU_F_32, -1}, // SI101022

		{_T("Core(TM) i7 CPU L 6"), 0, 6, 0x25, -1, CPU_ARRANDALE, CPU_ST_BLANK, CPU_BGA1288, CPU_F_32, -1}, // SI101022b

		{_T("Core(TM) i7 CPU E 6"), 0, 6, 0x25, -1, CPU_ARRANDALE, CPU_ST_BLANK, CPU_BGA1288, CPU_F_32, -1}, // SI101022d

		/*
		Select  Intel® Pentium® Processor U5600 (3M Cache, 1.33 GHz) Launched  No  18 W  N/A  Yes  No
		Select  Intel® Pentium® Processor U5400 (3M Cache, 1.20 GHz) Launched  No  18 W  $225  Yes  No
		Select  Intel® Pentium® Processor P6300 (3M Cache, 2.27 Cache) Launched  No  35 W  $134  Yes  No
		Select  Intel® Pentium® Processor P6200 (3M Cache, 2.13 GHz) Launched  No  35 W  $134  Yes  No
		Select  Intel® Pentium® Processor P6100 (3M Cache, 2.00 GHz) Launched  No  35 W  $134  Yes  No
		Select  Intel® Pentium® Processor P6000 (3M Cache, 1.86 GHz) Launched  No  35 W  $125  Yes  No
		*/
		{_T("CPU U6"), 0, 6, 0x25, -1, CPU_ARRANDALE, CPU_ST_BLANK, CPU_S_BGA1288, CPU_F_32, -1}, // SI101021
		{_T("CPU P6"), 0, 6, 0x25, -1, CPU_ARRANDALE, CPU_ST_BLANK, CPU_S_PGA988, CPU_F_32, -1},  // SI101021

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:26 ATOM
		/*
		Intel(R) Atom(TM) CPU E680 @ 1.60GHz	0 	6 	38 	1 	2012-04-23 (customer complaint for E680T)
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Tunnel Creek
		{_T("Atom(TM) CPU E680T"), 0, 6, 0x26, -1, CPU_TUNNEL_CREEK, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_45, 110}, // SI101034 - added
		{_T("Atom(TM) CPU E680"), 0, 6, 0x26, -1, CPU_TUNNEL_CREEK, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_45, 90},	// SI101034 - correction for Model number. TJunction added for temperature
																												// Still need to add socket and other CPUs

		// Lincroft
		{_T("Atom(TM) CPU Z6"), 0, 6, 0x26, -1, CPU_LINCROFT, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_45, -1}, // SI101034 - correction

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:2A Sandy Bridge	//BIT6010280000
		/*
		Intel(R) Core(TM) i7-2600 CPU @ 3.40GHz
		Intel(R) Core(TM) i7-2600K CPU @ 3.40GHz
		Intel(R) Core(TM) i7-2620M CPU @ 2.70GHz
		Intel(R) Core(TM) i7-2630QM CPU @ 2.00GHz

		Intel(R) Core(TM) i5-2300 CPU @ 2.80GHz
		Intel(R) Core(TM) i5-2400 CPU @ 3.10GHz
		Intel(R) Core(TM) i5-2400S CPU @ 2.50GHz
		Intel(R) Core(TM) i5-2410M CPU @ 2.30GHz
		Intel(R) Core(TM) i5-2500 CPU @ 3.30GHz
		Intel(R) Core(TM) i5-2500K CPU @ 3.30GHz
		Intel(R) Core(TM) i5-2540M CPU @ 2.60GHz

		Intel(R) Core(TM) i3-2100


		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Intel® Core™ i5-2540M Processor (3M Cache, 2.60 GHz)  No  35 Watts 2C / 4T Yes Yes 2.0 N/A Announced
		{_T("i3-2350M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_RPGA988, CPU_F_32, -1},		   // SI101022d
		{_T("i3-2330M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA1023_RPGA988B, CPU_F_32, -1}, // SI101022d
		{_T("i3-2328M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA1023_RPGA988B, CPU_F_32, -1}, // SI101036
		{_T("i3-2312M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_RPGA988B, CPU_F_32, -1},		   // SI101022d
		{_T("i3-2310M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA1023_RPGA988B, CPU_F_32, -1}, // SI101022d
		{_T("i3-2367M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1},		   // SI101022d
		{_T("i3-2357M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1},		   // SI101022d
		{_T("i3-2370M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_RPGA988, CPU_F_32, -1},		   // SI101022d
		{_T("i3-2377M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_RPGA988, CPU_F_32, -1},		   // SI101029

		{_T("i3-2330E"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_RPGA988, CPU_F_32, -1},  // SI101022d
		{_T("i3-2310E"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d

		{_T("i3-2340UE"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d

		// Intel® Core™ i3-2120 Processor (3M Cache, 3.30 GHz)  Yes  65 Watts 2C / 4T Yes Yes No N/A Announced
		// Intel® Core™ i3-2100T Processor (3M Cache, 2.50 GHz)  No  35 Watts 2C / 4T Yes Yes No N/A Announced
		// Intel® Core™ i3-2100 Processor (3M Cache, 3.10 GHz)  No  65 Watts 2C / 4T Yes Yes No N/A Announced
		{_T("i3-2120T"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},  // SI101022d
		{_T("i3-2100T"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},  // SI101022d
		{_T("i3-2130 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1}, // SI101022d
		{_T("i3-2125 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1}, // SI101022d
		{_T("i3-2120 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1}, // SI101022d
		{_T("i3-2105 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1}, // SI101022d
		{_T("i3-2102 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1}, // SI101022d
		{_T("i3-2100 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1}, // SI101022d

		// Intel® Core™ i5-2537M Processor (3M Cache, 1.40 GHz)  No  17 Watts 2C / 4T Yes Yes 2.0 N/A Announced
		// Intel® Core™ i5-2520M Processor (3M Cache, 2.50 GHz)  No  35 Watts 2C / 4T Yes Yes 2.0 N/A Announced
		// Intel® Core™ i5-2515E Processor (3M Cache, 2.50 GHz)  Yes  35 Watts 2C / 4T Yes Yes 2.0 N/A Announced
		// Intel® Core™ i5-2510E Processor (3M Cache, 2.50 GHz)  Yes  35 Watts 2C / 4T Yes Yes 2.0 N/A Announced
		{_T("i5-2557M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i5-2537M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i5-2467M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i5-2540M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i5-2520M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i5-2450M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i5-2435M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i5-2430M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i5-2410M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d

		{_T("i5-2510E"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCPGA988, CPU_F_32, -1}, // SI101022d
		{_T("i5-2515E"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d

		// Intel® Core™ i5-2500T Processor (6M Cache, 2.30 GHz)  No  45 Watts 4C / 4T Yes Yes 2.0 $216.00 Launched
		// Intel® Core™ i5-2500S Processor (6M Cache, 2.70 GHz)  No  65 Watts 4C / 4T Yes Yes 2.0 $216.00 Launched
		// Intel® Core™ i5-2500K Processor (6M Cache, 3.30 GHz)  No  95 Watts 4C / 4T Yes Yes 2.0 $216.00 Launched
		// Intel® Core™ i5-2500 Processor (6M Cache, 3.30 GHz)  No  95 Watts 4C / 4T Yes Yes 2.0 $205.00 Launched
		// Intel® Core™ i5-2400S Processor (6M Cache, 2.50 GHz)  No  65 Watts 4C / 4T Yes Yes 2.0 $195.00 Launched
		// Intel® Core™ i5-2400 Processor (6M Cache, 3.10 GHz)  Yes  95 Watts 4C / 4T Yes Yes 2.0 $184.00 Launched
		// Intel® Core™ i5-2390T Processor (3M Cache, 2.70 GHz)  No  35 Watts 2C / 4T Yes Yes 2.0 $195.00 Launched
		// Intel® Core™ i5-2300 Processor (6M Cache, 2.80 GHz)  No  95 Watts 4C / 4T Yes Yes 2.0 $177.00 Launched
		// Intel(R) Core(TM) i5-2515E CPU @ 2.50GHz	"1 	"	0x06	2A
		// Intel(R) Core(TM) i5-2540K CPU @ 3.30GHz	"2 	"	0x06	2A
		{_T("i5-2550K"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101025
		{_T("i5-2500K"), 0, 6, 0x2A, 0x7, CPU_SANDYBRIDGE, CPU_ST_D2, CPU_S_LGA1155, CPU_F_32, -1},		 // SI101022d
		{_T("i5-2500K"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2500 P"), 0, 6, 0x2A, 0x7, CPU_SANDYBRIDGE, CPU_ST_D2, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2500 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2500S"), 0, 6, 0x2A, 0x7, CPU_SANDYBRIDGE, CPU_ST_D2, CPU_S_LGA1155, CPU_F_32, -1},		 // SI101022d
		{_T("i5-2500S"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2500T"), 0, 6, 0x2A, 0x7, CPU_SANDYBRIDGE, CPU_ST_D2, CPU_S_LGA1155, CPU_F_32, -1},		 // SI101022d
		{_T("i5-2500T"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2515E"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2550 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2400 P"), 0, 6, 0x2A, 0x7, CPU_SANDYBRIDGE, CPU_ST_D2, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2400 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2405S"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2400S"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2450 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2450P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101025
		{_T("i5-2390T"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCLGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2380 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCLGA1155, CPU_F_32, -1}, // SI101022d
		{_T("i5-2380P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCLGA1155, CPU_F_32, -1},	 // SI101025
		{_T("i5-2320 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2310 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2300 P"), 0, 6, 0x2A, 0x7, CPU_SANDYBRIDGE, CPU_ST_D2, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d
		{_T("i5-2300 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},	 // SI101022d

		// Intel® Core™ i7-2920XM Processor Extreme Edition (8M Cache, 2.50 GHz)  No  55 Watts 4C / 8T Yes Yes 2.0 $1096.00 Launched
		// Intel(R) Core(TM) i7-2960XM CPU @ 2.70GHz	"38 	"	0x06	2A
		{_T("i7-2920XM"), 0, 6, 0x2A, 0x7, CPU_SANDYBRIDGE, CPU_ST_D2, CPU_S_FCPGA988, CPU_F_32, -1},	// SI101022d
		{_T("i7-2920XM"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCPGA988, CPU_F_32, -1}, // SI101022d
		{_T("i7-2960XM"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCPGA988, CPU_F_32, -1}, // SI101022d

		// Intel® Core™ i7-2820QM Processor (8M Cache, 2.30 GHz)  No  45 Watts 4C / 8T Yes Yes 2.0 $568.00 Launched
		// Intel® Core™ i7-2720QM Processor (6M Cache, 2.20 GHz)  No  45 Watts 4C / 8T Yes Yes 2.0 $378.00 Launched
		// Intel® Core™ i7-2715QE Processor (6M Cache, 2.10 GHz)  Yes  45 Watts 4C / 8T Yes Yes 2.0 N/A Launched
		// Intel® Core™ i7-2710QE Processor (6M Cache, 2.10 GHz)  Yes  45 Watts 4C / 8T Yes Yes 2.0 N/A Launched
		{_T("i7-2860QM"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCBGA1224_FCPGA988, CPU_F_32, -1}, // SI101022d
		{_T("i7-2840QM"), 0, 6, 0x2A, 0x7, CPU_SANDYBRIDGE, CPU_ST_D2, CPU_S_FCBGA1224_FCPGA988, CPU_F_32, -1},	  // SI101036
		{_T("i7-2820QM"), 0, 6, 0x2A, 0x7, CPU_SANDYBRIDGE, CPU_ST_D2, CPU_S_FCBGA1224_FCPGA988, CPU_F_32, -1},	  // SI101022d
		{_T("i7-2820QM"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCBGA1224_FCPGA988, CPU_F_32, -1}, // SI101022d
		{_T("i7-2760QM"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCBGA1224_FCPGA988, CPU_F_32, -1}, // SI101022d
		{_T("i7-2720QM"), 0, 6, 0x2A, 0x7, CPU_SANDYBRIDGE, CPU_ST_D2, CPU_S_FCBGA1224_FCPGA988, CPU_F_32, -1},	  // SI101022d
		{_T("i7-2720QM"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCBGA1224_FCPGA988, CPU_F_32, -1}, // SI101022d
		{_T("i7-2675QM"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCBGA1224_FCPGA988, CPU_F_32, -1}, // SI101022d
		{_T("i7-2670QM"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCBGA1224_FCPGA988, CPU_F_32, -1}, // SI101022d
		{_T("i7-2635QM"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCBGA1224_FCPGA988, CPU_F_32, -1}, // SI101022d
		{_T("i7-2630QM"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCBGA1224_FCPGA988, CPU_F_32, -1}, // SI101022d

		{_T("i7-2715QE"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i7-2710QE"), 0, 6, 0x2A, 0x7, CPU_SANDYBRIDGE, CPU_ST_D2, CPU_S_FCPGA988, CPU_F_32, -1},	// SI101022d
		{_T("i7-2710QE"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCPGA988, CPU_F_32, -1}, // SI101022d

		// Intel® Core™ i7-2657M Processor (4M Cache, 1.60 GHz)  No  17 Watts 2C / 4T Yes Yes 2.0 N/A Announced
		// Intel® Core™ i7-2649M Processor (4M Cache, 2.30 GHz)  No  25 Watts 2C / 4T Yes Yes 2.0 N/A Announced
		// Intel® Core™ i7-2629M Processor (4M Cache, 2.10 GHz)  No  25 Watts 2C / 4T Yes Yes 2.0 N/A Announced
		// Intel® Core™ i7-2620M Processor (4M Cache, 2.70 GHz)  No  35 Watts 2C / 4T Yes Yes 2.0 N/A Announced
		// Intel® Core™ i7-2617M Processor (4M Cache, 1.50 GHz)  No  17 Watts 2C / 4T Yes Yes 2.0 N/A Announced
		// Intel(R) i7-2637M CPU @ 1.70GHz	"3 	"	0x06	2A
		// Intel(R) Core(TM) i7-2640M CPU @ 2.80GHz	"84 	"	0x06	2A
		{_T("i7-2640M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i7-2620M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i7-2649M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i7-2629M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i7-2677M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i7-2637M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i7-2657M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("i7-2617M"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d

		{_T("i7-2655LE"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d

		{_T("i7-2610UE"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d

		// Intel(R) Core(TM) i7-2700K CPU @ 3.50GHz	"412 	"	0x06	2A
		// Intel® Core™ i7-2600S Processor (8M Cache, 2.80 GHz)  No  65 Watts 4C / 8T Yes Yes 2.0 $306.00 Launched
		// Intel® Core™ i7-2600K Processor (8M Cache, 3.40 GHz)  No  95 Watts 4C / 8T Yes Yes 2.0 $317.00 Launched
		// Intel® Core™ i7-2600 Processor (8M Cache, 3.40 GHz)  Yes  95 Watts 4C / 8T Yes Yes 2.0 $294.00 Launched
		{_T("i7-2700K"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},
		{_T("i7-2600K"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},
		{_T("i7-2600 P"), 0, 6, 0x2A, 0x7, CPU_SANDYBRIDGE, CPU_ST_D2, CPU_S_LGA1155, CPU_F_32, -1},
		{_T("i7-2600 P"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},
		{_T("i7-2600S"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1},
		{_T("i7-2600"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1}, // SI101029

		// Celeron Sandy bridge - MOBILE
		/*
		Intel(R) Celeron(R) CPU 847 @ 1.10GHz	"3 	"	0x06	2A
		Intel(R) Celeron(R) CPU 857 @ 1.20GHz	"2 	"	0x06	2A

		Intel(R) Celeron(R) CPU B810 @ 1.60GHz	"6 	"	0x06	2A
		Intel(R) Celeron(R) CPU B815 @ 1.60GHz	"2 	"	0x06	2A
		Intel(R) Celeron(R) CPU B840 @ 1.90GHz	"2 	"	0x06	2A
		*/
		{_T("Celeron(R) CPU 7"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d
		{_T("Celeron(R) CPU 8"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d

		{_T("Celeron(R) CPU B8"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_RPGA988B, CPU_F_32, -1}, // SI101022d
		{_T("Celeron(R) CPU B7"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_RPGA988B, CPU_F_32, -1}, // SI101022d

		// Celeron Sandy bridge - desktop		//SI101022d
		/*
		Intel(R) Celeron(R) CPU G440 @ 1.60GHz	"1 	"	0x06	2A
		Intel(R) Celeron(R) CPU G460 @ 1.80GHz	"1 	"	0x06	2A
		Intel(R) Celeron(R) CPU G540 @ 2.50GHz	"6 	"	0x06	2A
		*/
		{_T("Celeron(R) CPU G"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1}, // SI101022d

		// Intel(R) Pentium(R) CPU B960 @ 2.20GHz	"9 	"	0x06	 2A
		{_T("Pentium(R) CPU 9"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_BGA_1023, CPU_F_32, -1}, // SI101022d

		{_T("Pentium(R) CPU B9"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_FCPGA988, CPU_F_32, -1}, // SI101022d

		/*
		Intel(R) Pentium(R) CPU G630 @ 2.70GHz	"18 	"	0x06	2A
		Intel(R) Pentium(R) CPU G630T @ 2.30GHz	"3 	"	0x06	2A
		Intel(R) Pentium(R) CPU G860 @ 3.00GHz	"5 	"	0x06	2A
		*/
		{_T("Pentium(R) CPU G"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1}, // SI101022d
		{_T("Pentium(R) CPU 3"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1}, // SI101022d

		// Sand Bridge Xeon
		// LGA1155
		// Intel(R) Xeon(R) CPU E31285 @ 3.40GHz	"1 	"	0x06	2A
		// Intel(R) Xeon(R) CPU E31290 @ 3.60GHz	"2 	"	0x06	2A
		{_T("Xeon(R) CPU E31"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_32, -1}, // SI101022d

		// LGA2011
		{_T("Xeon(R) CPU E51"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_32, -1},	  // SI101022d
		{_T("Xeon(R) CPU E5-16"), 0, 6, 0x2D, -1, CPU_SANDY_BRIDGE_E, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_32, -1}, // SI101029
		{_T("Xeon(R) CPU E5-26"), 0, 6, 0x2D, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_32, -1},	  // SI101022d

		// LGA1356
		{_T("Xeon(R) CPU E524"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1356, CPU_F_32, -1},  // SI101022d
		{_T("Xeon(R) CPU E5-24"), 0, 6, 0x2D, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1356, CPU_F_32, -1}, // SI101022d

		// LGA2011
		{_T("Xeon(R) CPU E526"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_32, -1},  // SI101022d
		{_T("Xeon(R) CPU E546"), 0, 6, 0x2A, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_32, -1},  // SI101022d
		{_T("Xeon(R) CPU E5-46"), 0, 6, 0x2D, -1, CPU_SANDYBRIDGE, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_32, -1}, // SI101022d

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:2C
		/*
		Core i7-970 SLBVF (B1)  3.2 GHz 1/1/1/1/2/2[Note 1] 6 6 × 256 KB 12 MB 1 × 4.8 GT/s QPI 24× 3 × DDR3-1066 0.8–1.375 V 130 W LGA 1366 July 19, 2010
		Core i7-980 SLBYU (B1) 3.33 GHz 1/1/1/1/2/2 6 6 × 256 KB 12 MB 1 × 4.8 GT/s QPI 25× 3 × DDR3-1066 0.8–1.300 V 130 W LGA 1366 June 26, 2011
		Core i7-980X SLBUZ (B1) 3.33 GHz 1/1/1/1/2/2 6 6 × 256 KB 12 MB 1 × 6.4 GT/s QPI 25× 3 × DDR3-1066 0.8–1.375 V 130 W LGA 1366 March 16, 2010
		Core i7-990X SLBVZ (B1) 3.47 GHz 1/1/1/1/2/2 6 6 × 256 KB 12 MB 1 × 6.4 GT/s QPI 26× 3 × DDR3-1066 0.8–1.375 V 130 W LGA 1366 February 14, 2011
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("i7 CPU 970"), 0, 6, 0x2C, 0x2, CPU_GULFTOWN, CPU_ST_B1, CPU_S_LGA1366, CPU_F_32, -1},	   // 6 cores, 12 trheads	//BIT6010120008
		{_T("i7 CPU 980"), 0, 6, 0x2C, -1, CPU_GULFTOWN, CPU_ST_BLANK, CPU_S_LGA1366, CPU_F_32, -1},   // SI101021
		{_T("i7 CPU X 980"), 0, 6, 0x2C, 0x2, CPU_GULFTOWN, CPU_ST_B1, CPU_S_LGA1366, CPU_F_32, -1},   // BIT6010250003
		{_T("i7 CPU X 990"), 0, 6, 0x2C, -1, CPU_GULFTOWN, CPU_ST_BLANK, CPU_S_LGA1366, CPU_F_32, -1}, // SI101021

		{_T("Xeon(R) CPU E56"), 0, 6, 0x2C, 0x1, CPU_WESTMERE_EP, CPU_ST_B0, CPU_S_LGA1366, CPU_F_32, -1}, // BIT6010250003
		{_T("Xeon(R) CPU E56"), 0, 6, 0x2C, 0x2, CPU_WESTMERE_EP, CPU_ST_B1, CPU_S_LGA1366, CPU_F_32, -1}, // BIT6010250003
		{_T("Xeon(R) CPU X56"), 0, 6, 0x2C, 0x1, CPU_WESTMERE_EP, CPU_ST_B0, CPU_S_LGA1366, CPU_F_32, -1}, // BIT6010250003
		{_T("Xeon(R) CPU X56"), 0, 6, 0x2C, 0x2, CPU_WESTMERE_EP, CPU_ST_B1, CPU_S_LGA1366, CPU_F_32, -1}, // BIT6010250003
		{_T("Xeon(R) CPU W36"), 0, 6, 0x2C, 0x1, CPU_WESTMERE_EP, CPU_ST_B0, CPU_S_LGA1366, CPU_F_32, -1}, // BIT6010250003
		{_T("Xeon(R) CPU W36"), 0, 6, 0x2C, 0x2, CPU_WESTMERE_EP, CPU_ST_B1, CPU_S_LGA1366, CPU_F_32, -1}, // BIT6010250003
		{_T("Xeon(R) CPU L56"), 0, 6, 0x2C, 0x1, CPU_WESTMERE_EP, CPU_ST_B0, CPU_S_LGA1366, CPU_F_32, -1}, // BIT6010250003
		{_T("Xeon(R) CPU L56"), 0, 6, 0x2C, 0x2, CPU_WESTMERE_EP, CPU_ST_B1, CPU_S_LGA1366, CPU_F_32, -1}, // BIT6010250003
		{_T("Xeon(R) CPU W56"), 0, 6, 0x2C, 0x2, CPU_WESTMERE_EP, CPU_ST_B1, CPU_S_LGA1366, CPU_F_32, -1}, // SI101066

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:2D
		/*
		Core i7-3930K SR0H9 (C1) 6 3.2 GHz 3/3/4/4/6/6[Note 2] 6 × 256 KB 12 MB N/A N/A 130 W LGA 2011 DMI 2.0 November 14, 2011 CM8061901100802 BX80619I73930K
		Core i7-3960X SR0GW (C1) 6 3.3 GHz 3/3/4/4/6/6 6 × 256 KB 15 MB N/A N/A 130 W LGA 2011 DMI 2.0 November 14, 2011 CM8061907184018 BX80619I73960X
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Sandy Bridge-E	//SI101021
		{_T("i7-3820"), 0, 6, 0x2D, -1, CPU_SANDY_BRIDGE_E, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_32, -1}, // SI101029
		{_T("i7-3930K"), 0, 6, 0x2D, -1, CPU_SANDY_BRIDGE_E, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_32, -1},
		{_T("i7-3960X"), 0, 6, 0x2D, -1, CPU_SANDY_BRIDGE_E, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_32, -1},

		// Xeon
		// Intel Xeon CPU E5 //SI101021
		{_T("CPU E5-4"), 0, 6, 0x2D, -1, CPU_SANDY_BRIDGE_E, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_32, -1},
		{_T("CPU E5-26"), 0, 6, 0x2D, -1, CPU_SANDY_BRIDGE_E, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_32, -1},
		{_T("CPU E5-24"), 0, 6, 0x2D, -1, CPU_SANDY_BRIDGE_E, CPU_ST_BLANK, CPU_S_LGA1356, CPU_F_32, -1},
		{_T("CPU E5-16"), 0, 6, 0x2D, -1, CPU_SANDY_BRIDGE_E, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_32, -1},
		{_T("CPU E5-14"), 0, 6, 0x2D, -1, CPU_SANDY_BRIDGE_E, CPU_ST_BLANK, CPU_S_LGA1356, CPU_F_32, -1}, // SI101036
		{_T("CPU E5-12"), 0, 6, 0x2D, -1, CPU_SANDY_BRIDGE_E, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_32, -1}, // SI101036

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:2E
		/*
		Intel(R) Xeon(R) CPU X6550 @ 2.00GHz
		Intel(R) Xeon(R) CPU X7550 @ 2.00GHz
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Nehalem EX
		{_T("X7"), 0, 6, 0x2E, -1, CPU_NEHALEM_EX, CPU_ST_BLANK, CPU_S_LGA1567, CPU_F_45, -1}, // SI101066
		{_T("X6"), 0, 6, 0x2E, -1, CPU_NEHALEM_EX, CPU_ST_BLANK, CPU_S_LGA1567, CPU_F_45, -1}, // SI101066
		{_T("E7"), 0, 6, 0x2E, -1, CPU_NEHALEM_EX, CPU_ST_BLANK, CPU_S_LGA1567, CPU_F_45, -1}, // SI101066
		{_T("E6"), 0, 6, 0x2E, -1, CPU_NEHALEM_EX, CPU_ST_BLANK, CPU_S_LGA1567, CPU_F_45, -1}, // SI101066

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:2F
		/*
		Intel(R) Xeon(R) CPU E7- 4830 @ 2.13GHz	"1 	"	0x06	2F
		Intel(R) Xeon(R) CPU E7- 4850 @ 2.00GHz	"3 	"	0x06	2F
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Westemere-EX
		{_T("Xeon(R) CPU E7- 4"), 0, 6, 0x2F, -1, CPU_WESTMERE_EX, CPU_ST_BLANK, CPU_S_LGA1567, CPU_F_32, -1}, // SI101025

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:35
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// Cloverview
		{_T("Atom(TM) CPU Z2"), 0, 6, 0x35, -1, CPU_CLOVERVIEW, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1}, // SI101036

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:36
		/*
		Intel(R) Atom(TM) CPU D2700 @ 2.13GHz	"6 	"	0x06	36
		Intel(R) Atom(TM) CPU N2600 @ 1.60GHz	"1 	"	0x06	36
		Intel(R) Atom(TM) CPU N2800 @ 1.86GHz	"3 	"	0x06	36
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Cedarview
		{_T("Atom(TM) CPU D"), 0, 6, 0x36, -1, CPU_CEDARVIEW, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, 100}, // D2500/D2550/D2700 all 100C Tjunction SI101036
		{_T("Atom(TM) CPU N"), 0, 6, 0x36, -1, CPU_CEDARVIEW, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, 100}, // SI101064 - added TjMax to supprot temp monitoring

		/*
		ATOM processors

		Bonnell (microarchitecture) http://en.wikipedia.org/wiki/Bonnell_(microarchitecture)
			Silverthorne
			Diamondville
			Pineview
			Tunnel Creek
			Lincroft
			Stellarton
			Sodaville
			Cedarview
		Silvermont
			Merrifield intended for smartphones
			Bay Trail aimed at tablets
		*/

		/* Not sure of cpuid - TO DO
		//Centerton - Microserver
		Intel® Atom™ Processor S1260 (1MB Cache, 2.00 GHz) Launched  Q4'12  8.5 W  TRAY: $64.00
		Intel® Atom™ Processor S1240 (1MB Cache, 1.60 GHz) Launched  Q4'12  6.1 W  TRAY: $64.00
		Intel® Atom™ Processor S1220 (1MB Cache, 1.60 GHz)
		*/

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:37 - Bay Trail
		/*
			Bay Trail aimed at tablets
			Desktop processors (Bay Trail-D):	Pentium J2850, Celeron J1850, Celeron J1750
			Mobile processors (Bay Trail-M):	Pentium N3510, Celeron N2910, N2810, N2805
			Tablet processors (Bay Trail-T):	Atom Z3770

			Intel(R) Celeron(R) CPU N2810 @ 2.00GHz, Family 6, Model 0x37, Stepping 2
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Bay Trail-D
		{_T("J2900"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_D, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 105},	  // SI101100
		{_T("J2850"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_D, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 100},	  // SI101100
		{_T("Pentium J2"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_D, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, -1}, // SI101075 //SI101037 - just a guess
		{_T("CPU J2"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_D, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, -1},	  // SI101075

		{_T("J1900"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_D, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 105},	  // SI101100
		{_T("J1850"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_D, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 100},	  // SI101100
		{_T("J1800"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_D, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 105},	  // SI101100
		{_T("J1750"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_D, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 100},	  // SI101100
		{_T("Celeron J1"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_D, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, -1}, // SI101037
		{_T("CPU J1"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_D, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, -1},	  // SI101075

		// Bay Trail-M
		{_T("Pentium N3"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 100},		  // SI101037 - based on qwanta supplied log file //SI101100 - Tj added
		{_T("Pentium(R) CPU N3"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 100}, // SI101066//SI101100 - Tj added

		{_T("N2840"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 100},  // SI101100
		{_T("N2830"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 100},  // SI101100
		{_T("N2820"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 105},  // SI101100
		{_T("N2815"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 105},  // SI101100
		{_T("N2810"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 100},  // SI101100
		{_T("N2808"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 100},  // SI101100
		{_T("N2807"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 105},  // SI101100
		{_T("N2806"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 105},  // SI101100
		{_T("N2805"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 80},   // SI101100
		{_T("CPU N28"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, -1}, // SI101075

		{_T("N2940"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 100},	  // SI101100
		{_T("N2930"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 100},	  // SI101100
		{_T("N2920"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 105},	  // SI101100
		{_T("N2910"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 100},	  // SI101100
		{_T("CPU N29"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, -1},	  // SI101075
		{_T("Celeron N2"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, -1}, // SI101037

		{_T("CPU N35"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, 100}, // SI101075//SI101100 - Tj added
		{_T("CPU N38"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_M, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, -1},	// SI101075

		// Bay Trail-CR?
		{_T("3735F"), 0, 6, 0x37, -1, CPU_BAY_TRAIL, CPU_ST_BLANK, CPU_S_UTFCBGA592, CPU_F_22, -1}, // SI101100
		{_T("3735G"), 0, 6, 0x37, -1, CPU_BAY_TRAIL, CPU_ST_BLANK, CPU_S_UTFCBGA592, CPU_F_22, -1}, // SI101100
		{_T("3736F"), 0, 6, 0x37, -1, CPU_BAY_TRAIL, CPU_ST_BLANK, CPU_S_UTFCBGA592, CPU_F_22, -1}, // SI101100
		{_T("3736G"), 0, 6, 0x37, -1, CPU_BAY_TRAIL, CPU_ST_BLANK, CPU_S_UTFCBGA592, CPU_F_22, -1}, // SI101100

		// Bay Trail-T
		{_T("Z3775 "), 0, 6, 0x37, -1, CPU_BAY_TRAIL_T, CPU_ST_BLANK, CPU_S_UTFCBGA1380, CPU_F_22, -1}, // SI101075
		{_T("Z3770D"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_T, CPU_ST_BLANK, CPU_S_UTFCBGA1380, CPU_F_22, 90}, // SI101039//SI101100 Tj added
		{_T("Z3770 "), 0, 6, 0x37, -1, CPU_BAY_TRAIL_T, CPU_ST_BLANK, CPU_S_UTFCBGA1380, CPU_F_22, 90}, // SI101037//SI101100 Tj added
		{_T("Z3745 "), 0, 6, 0x37, -1, CPU_BAY_TRAIL_T, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, 90},		// SI101066//SI101100 Tj added
		{_T("Z3740D"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_T, CPU_ST_BLANK, CPU_S_UTFCBGA1380, CPU_F_22, 90}, // SI101039//SI101100 Tj added
		{_T("Z3740 "), 0, 6, 0x37, -1, CPU_BAY_TRAIL_T, CPU_ST_BLANK, CPU_S_UTFCBGA1380, CPU_F_22, 90}, // SI101039

		{_T("Z3680D"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_T, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101039 - Android only
		{_T("Z3680 "), 0, 6, 0x37, -1, CPU_BAY_TRAIL_T, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101039 - Android only

		{_T("Z379"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_T, CPU_ST_BLANK, CPU_S_UTFCBGA1380, CPU_F_22, -1}, // SI101075

		{_T("Z37"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_T, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101075
		{_T("Z36"), 0, 6, 0x37, -1, CPU_BAY_TRAIL_T, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101075

		// Bay Trail
		// Atom e.g Intel® Atom™ Processor E3825 FCBGA1170
		{_T("E38"), 0, 6, 0x37, -1, CPU_BAY_TRAIL, CPU_ST_BLANK, CPU_S_FCBGA1170, CPU_F_22, -1}, // SI101047

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:3A
		/*
		Ivy Bridge

		Intel(R) Xeon(R) CPU E3-1220 V2 @ 3.10GHz	"	15 	"	"6 	"	"58 	"	0x3A	"9 	"	24/07/2012
		Intel(R) Xeon(R) CPU E3-1225 V2 @ 3.20GHz	"	21 	"	"6 	"	"58 	"	0x3A	"9 	"	10/09/2012
		Intel(R) Xeon(R) CPU E3-1230 V2 @ 3.30GHz	"	191 	"	"6 	"	"58 	"	0x3A	"9 	"	3/05/2012
		Intel(R) Xeon(R) CPU E3-1240 V2 @ 3.40GHz	"	42 	"	"6 	"	"58 	"	0x3A	"9 	"	21/07/2012
		Intel(R) Xeon(R) CPU E3-1245 V2 @ 3.40GHz	"	50 	"	"6 	"	"58 	"	0x3A	"9 	"	30/05/2012
		Intel(R) Xeon(R) CPU E3-1265L V2 @ 2.50GHz	"	1 	"	"6 	"	"58 	"	0x3A	"9 	"	5/09/2012
		Intel(R) Xeon(R) CPU E3-1270 V2 @ 3.50GHz	"	26 	"	"6 	"	"58 	"	0x3A	"9 	"	17/07/2012
		Intel(R) Xeon(R) CPU E3-1275 V2 @ 3.50GHz	"	14 	"	"6 	"	"58 	"	0x3A	"8 	"	11/04/2012
		Intel(R) Xeon(R) CPU E3-1290 V2 @ 3.70GHz	"	6 	"	"6 	"	"58 	"	0x3A	"9 	"	14/05/2012
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		//{_T("Core(TM) i3-32"),0,6,0x3A,-1,CPU_IVYBRIDGE,CPU_ST_BLANK,CPU_S_BLANK,CPU_F_22,-1},			//SI101023
		//{_T("Core(TM) i5-33"),0,6,0x3A,-1,CPU_IVYBRIDGE,CPU_ST_BLANK,CPU_S_BLANK,CPU_F_22,-1},			//SI101023
		//{_T("Core(TM) i5-34"),0,6,0x3A,-1,CPU_IVYBRIDGE,CPU_ST_BLANK,CPU_S_BLANK,CPU_F_22,-1},			//SI101023
		//{_T("Core(TM) i5-35"),0,6,0x3A,-1,CPU_IVYBRIDGE,CPU_ST_BLANK,CPU_S_BLANK,CPU_F_22,-1},			//SI101023
		//{_T("Core(TM) i7-37"),0,6,0x3A,-1,CPU_IVYBRIDGE,CPU_ST_BLANK,CPU_S_BLANK,CPU_F_22,-1},			//SI101023

		// Intel(R) Xeon(R) CPU E3-1230 V2 @ 3.30GHz
		// LGA1155 Ivy-bridge
		// V2 in name seperates them from otherwise identically named Sandy Bridge chips
		// (although the on chip naming seems to have changed to add a dash in the name?)

		// Server
		{_T("E3-12"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1}, // SI101039

		// Desktop
		{_T("i7-3770K"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1}, // SI101025
		{_T("i7-3770 "), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1}, // SI101025
		{_T("i7-3770S"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1}, // SI101025
		{_T("i7-3770T"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1}, // SI101025

		{_T("i5-3570K"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3570 "), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3570S"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3570T"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3550 "), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3550S"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, 103}, // SI101025 //SI101036 Tjunction added as customer claimed doesn't work
		{_T("i5-3475S"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3470 "), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3470S"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3470T"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3450 "), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3450S"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3350P"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101036
		{_T("i5-3335S"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101036
		{_T("i5-3330 "), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3330S"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3240 "), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3225 "), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025
		{_T("i5-3220 "), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1},	 // SI101025

		{_T("i3-3240 "), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1}, // SI101025
		{_T("i3-3220 "), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1}, // SI101036
		{_T("i3-3220T"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1}, // SI101036
		{_T("i3-3225 "), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1}, // SI101036

		// Celerons
		{_T("G16"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1}, // SI101039

		{_T("927UE"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101039
		{_T("1047UE"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101039
		{_T("1037U"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101039
		{_T("1020M"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101039
		{_T("1020E"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101039
		{_T("1019Y"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101039
		{_T("1017U"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101039
		{_T("1007U"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101039
		{_T("1005M"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101039
		{_T("1000M"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101039

		// Pentiums
		{_T("G20"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1}, // SI101039
		{_T("G21"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_LGA1155, CPU_F_22, -1}, // SI101039

		{_T("2129Y"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101039
		{_T("2127U"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101039
		{_T("2117U"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101039
		{_T("2030M"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101039
		{_T("2020M"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101039

		// Mobile
		{_T("i7-3940XM"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101036
		{_T("i7-3920XM"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101025
		{_T("i7-3840QM"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101029
		{_T("i7-3820QM"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101025
		{_T("i7-3740QM"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101036
		{_T("i7-3720QM"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101025
		{_T("i7-3632QM"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101025
		{_T("i7-3615QE"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101036
		{_T("i7-3615QM"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101025
		{_T("i7-3612QE"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101036
		{_T("i7-3612QM"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101025
		{_T("i7-3610QE"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101036
		{_T("i7-3610QM"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101025
		{_T("i7-3687U"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101036
		{_T("i7-3667U"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101025
		{_T("i7-3555LE"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101036
		{_T("i7-3537U"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101036
		{_T("i7-3540M"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101025
		{_T("i7-3517UE"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101036
		{_T("i7-3517U"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101025
		{_T("i7-3520M"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101025
		{_T("i5-3437U"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101036
		{_T("i5-3427U"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101025
		{_T("i5-3317U"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101025
		{_T("i5-3360M"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101025
		{_T("i5-3320M"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101025
		{_T("i3-3217U"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101025
		{_T("i3-3317U"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101025
		{_T("i5-3210M"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101025
		{_T("i3-3110M"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},  // SI101025

		// Catch-all
		{_T("i7-3"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101039
		{_T("i5-3"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101039
		{_T("i3-3"), 0, 6, 0x3A, -1, CPU_IVYBRIDGE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101039

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:3C
		/*
		Haswell - 06_3CH, 06_45H, 06_46H
		(4th Generation Intel Core Processor and Intel Xeon Processor E3-1200 v3 Product Family based on Intel microarchitecture Haswell)

		Intel(R) Core(TM) i7-4700EQ CPU @ 2.40GHz	"	1 	"	"6 	"	"60 	"	0x3C	"3 	"	2/06/2013
		Intel(R) Core(TM) i7-4700MQ CPU @ 2.40GHz	"	123 	"	"6 	"	"60 	"	0x3C	"3 	"	2/06/2013
		Intel(R) Core(TM) i7-4770 CPU @ 3.40GHz	"	157 	"	"6 	"	"60 	"	0x3C	"3 	"	2/06/2013
		Intel(R) Core(TM) i7-4770K CPU @ 3.50GHz	"	662 	"	"6 	"	"60 	"	0x3C	"3 	"	2/06/2013
		Intel(R) Core(TM) i7-4770S CPU @ 3.10GHz	"	22 	"	"6 	"	"60 	"	0x3C	"3 	"	2/06/2013

		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// Server
		{_T("E3-1285L v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101036
		{_T("E3-1285 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	 // SI101036
		{_T("E3-1280 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	 // SI101036
		{_T("E3-1275 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	 // SI101036
		{_T("E3-1270 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	 // SI101036
		{_T("E3-1268L v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101036
		{_T("E3-1265L v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101036
		{_T("E3-1245 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	 // SI101036
		{_T("E3-1240 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	 // SI101036
		{_T("E3-1230L v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101036
		{_T("E3-1231 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	 // SI101066
		{_T("E3-1230 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	 // SI101036
		{_T("E3-1225 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	 // SI101036
		{_T("E3-1220 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	 // SI101036
		{_T("E3-1220L v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101066
		{_T("E3-1231 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	 // SI101066

		{_T("E3-1286 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	   // SI101075
		{_T("E3-1286L v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},   // SI101075
		{_T("E3-1284L v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101098 correction //SI101075
		{_T("E3-1281 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	   // SI101075
		{_T("E3-1276 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	   // SI101075
		{_T("E3-1275L v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},   // SI101075
		{_T("E3-1271 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	   // SI101075
		{_T("E3-1246 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	   // SI101075
		{_T("E3-1241 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	   // SI101075
		{_T("E3-1230L v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},   // SI101075
		{_T("E3-1226 v3"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},	   // SI101075

		{_T("E3-12"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101098

		// Desktop
		{_T("i7-4790 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101049	//Haswell refresh
		{_T("i7-4790K"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i7-4790S"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i7-4790T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i7-4785T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i7-4771 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i7-4790K"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i7-4790K"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i7-4770TE"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101036
		{_T("i7-4770T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101036
		{_T("i7-4770S"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101036
		{_T("i7-4770K"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101036
		{_T("i7-4770 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101036
		{_T("i7-4765T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101036

		{_T("i7-4720HQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101098
		{_T("i7-4722HQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101098

		{_T("i5-4690 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101049
		{_T("i5-4690K"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i5-4690S"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i5-4690T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i5-4670T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101036
		{_T("i5-4670S"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101036
		{_T("i5-4670K"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101036
		{_T("i5-4670 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101036
		{_T("i5-4590 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101049
		{_T("i5-4590S"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i5-4590T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i5-4570TE"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101036
		{_T("i5-4570T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101036
		{_T("i5-4570 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101036

		{_T("i5-4570S"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101066
		{_T("i5-4460S"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101066
		{_T("i5-4460T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101066
		{_T("i5-4440"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066

		{_T("i5-4460 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101049
		{_T("i5-4440S"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101047
		{_T("i5-4430S"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101036
		{_T("i5-4430 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101036

		{_T("i3-4370T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101100
		{_T("i3-4370 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101079
		{_T("i3-4360T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101079
		{_T("i3-4360 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i3-4350 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i3-4340 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i3-4330 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i3-4350T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i3-4330T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i3-4340TE"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101066
		{_T("i3-4330TE"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101066
		{_T("i3-4170 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101079
		{_T("i3-4170T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101079
		{_T("i3-4160 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101079
		{_T("i3-4160T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101079
		{_T("i3-4150 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i3-4130 "), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i3-4150T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066
		{_T("i3-4130T"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1},  // SI101066

		// Low end desktop - Pentiums
		//{_T("G3430"),0,6,0x3C,-1,CPU_HASWELL,CPU_ST_BLANK,CPU_S_LGA1150,CPU_F_22,-1},					//SI101036
		//{_T("G3420"),0,6,0x3C,-1,CPU_HASWELL,CPU_ST_BLANK,CPU_S_LGA1150,CPU_F_22,-1},					//SI101036
		//{_T("G3220"),0,6,0x3C,-1,CPU_HASWELL,CPU_ST_BLANK,CPU_S_LGA1150,CPU_F_22,-1},					//SI101036
		{_T("G34"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101066
		{_T("G33"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101066
		{_T("G32"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101066

		//{_T("G1830"),0,6,0x3C,-1,CPU_HASWELL,CPU_ST_BLANK,CPU_S_LGA1150,CPU_F_22,-1},					//SI101049	//Haswell refresh
		//{_T("G1820"),0,6,0x3C,-1,CPU_HASWELL,CPU_ST_BLANK,CPU_S_LGA1150,CPU_F_22,-1},					//SI101049	//Haswell refresh
		{_T("G18"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_22, -1}, // SI101066

		// Mobile
		{_T("i7-4950HQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101066
		{_T("i7-4940MX"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101050
		{_T("i7-4930MX"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCPGA946, CPU_F_22, -1}, // SI101036
		{_T("i7-4910MQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101050
		{_T("i7-4900MQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCPGA946, CPU_F_22, -1}, // SI101036

		{_T("i7-4860EQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101066
		{_T("i7-4860HQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101050
		{_T("i7-4850EQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101066
		{_T("i7-4850HQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101066
		{_T("i7-4810MQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	// SI101050
		{_T("i7-4800MQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCPGA946, CPU_F_22, -1}, // SI101036

		{_T("i7-4760HQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4750HQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4712MQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4712HQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4710MQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4710HQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4702HQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101036
		{_T("i7-4702MQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCPGA946, CPU_F_22, -1},	 // SI101036
		{_T("i7-4702EC"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCPGA946, CPU_F_22, -1},	 // SI101066
		{_T("i7-4701EQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCPGA946, CPU_F_22, -1},	 // SI101066
		{_T("i7-4700MQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCPGA946, CPU_F_22, -1},	 // SI101036
		{_T("i7-4700HQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101036
		{_T("i7-4700EQ"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101036
		{_T("i7-4700EC"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066

		{_T("i7-4650U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101036

		{_T("i7-4610M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101050
		{_T("i7-4600M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101047
		{_T("i7-4600M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101047

		{_T("i7-4558U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101036
		{_T("i7-4550U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101036

		{_T("i7-4610Y"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066
		{_T("i7-4600U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066

		{_T("i7-4578U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101079
		{_T("i7-4510U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101066

		{_T("i5-4402EC"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066
		{_T("i5-4422E"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	 // SI101066
		{_T("i5-4410E"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	 // SI101066
		{_T("i5-4402E"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	 // SI101066
		{_T("i5-4402EC"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066
		{_T("i5-4400E"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},	 // SI101066

		{_T("i5-4360U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101050
		{_T("i5-4350U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101066
		{_T("i5-4340M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101050
		{_T("i5-4350U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101036
		{_T("i5-4330M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101047
		{_T("i5-4310M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101050
		{_T("i5-4310U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101050
		{_T("i5-4308U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101079
		{_T("i5-4300M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101047

		{_T("i5-4302Y"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066
		{_T("i5-4300Y"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066
		{_T("i5-4300U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066

		{_T("i5-4278U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101079
		{_T("i5-4260U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101036
		{_T("i5-4220Y"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101036
		{_T("i5-4210H"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101079
		{_T("i5-4210M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101036
		{_T("i5-4210U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101036
		{_T("i5-4210Y"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101036
		{_T("i5-4202Y"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101036

		{_T("i5-4288U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101036
		{_T("i5-4258U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101036
																									// Intel(R) core(TM) i5-4250U @1.30GHz 1.90GHz. support ticket NBS-64423-865
		{_T("i5-4250U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101032	//Tjunction = 100
		{_T("i5-4200H"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101047
		{_T("i5-4200Y"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101036
		{_T("i5-4200U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101036
		{_T("i5-4200M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101047

		{_T("i3-4158U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101036
		{_T("i3-4100U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101036
		{_T("i3-4100M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101047
		{_T("i3-4120U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101066
		{_T("i3-4112E"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101066
		{_T("i3-4110E"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101066
		{_T("i3-4102E"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101066
		{_T("i3-4100E"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101066
		{_T("i3-4110M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101066

		{_T("i3-4010Y"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101036
		{_T("i3-4010U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101036
		{_T("i3-4000M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101047
		{_T("i3-4030Y"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101066
		{_T("i3-4020Y"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101066
		{_T("i3-4012Y"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101066
		{_T("i3-4030U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101066
		{_T("i3-4025U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101066
		{_T("i3-4005U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},		// SI101066

		// Pentiums
		{_T("3560M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066
		{_T("3550M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066

		// Celerons
		{_T("2970M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066
		{_T("2950M"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066

		// Pentiums
		{_T("3561Y"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066
		{_T("3560Y"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066
		{_T("3558U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066
		{_T("3556U"), 0, 6, 0x3C, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066

		// Celerons
		{_T("2955U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066
		{_T("2957U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066
		{_T("2981U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066
		{_T("2980U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066
		{_T("2961Y"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101066

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:3D
		/*
		06_3DH : Next Generation Intel Core Processor

		CPU Type: Intel(R) Core(TM) i5-5###U CPU @ 2.20GHz ?????????
		CPUID: Family 6, Model 3D, Stepping 4

		Intel(R) Core(TM) i7-5500U CPU @ 2.40GHz,

		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Broadwell socket types added SI101100
		// Broadwell-M
		{_T("5Y10"), 0, 6, 0x3D, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1234, CPU_F_14, -1}, // SI101073 - start adding Broadwell - not much info available yet
		{_T("5Y70"), 0, 6, 0x3D, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1234, CPU_F_14, -1}, // SI101073
		{_T("5Y1"), 0, 6, 0x3D, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1234, CPU_F_14, -1},  // SI101088
		{_T("5Y3"), 0, 6, 0x3D, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1234, CPU_F_14, -1},  // SI101088
		{_T("5Y5"), 0, 6, 0x3D, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1234, CPU_F_14, -1},  // SI101088
		{_T("5Y7"), 0, 6, 0x3D, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1234, CPU_F_14, -1},  // SI101088

		// Broadwell-U
		/*
		i7-5557U
		i7-5550U
		i7-5500U
		i7-5650U
		i7-5600U
		i5-5287U
		i5-5257U
		i5-5250U
		i5-5200U
		i5-5350U
		i5-5300U
		i3-5157U
		i3-5010U
		i3-5005U
		Pentium® Processor 3805U
		Celeron® Processor 3755U
		Celeron® Processor 3205U
		*/
		{_T("i7-5"), 0, 6, 0x3D, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_14, -1},  // SI101088
		{_T("i5-5"), 0, 6, 0x3D, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_14, -1},  // SI101080
		{_T("i3-5"), 0, 6, 0x3D, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_14, -1},  // SI101091
		{_T("3805U"), 0, 6, 0x3D, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_14, -1}, // SI101091	//Pentium
		{_T("3825U"), 0, 6, 0x3D, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_14, -1}, // SI101100
		{_T("3755U"), 0, 6, 0x3D, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_14, -1}, // SI101091	//Celeron
		{_T("3205U"), 0, 6, 0x3D, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_14, -1}, // SI101091	//Celeron

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:3E
		/*
		06_3EH : Next Generation Intel Xeon Processor E7 Family based on Intel® microarchitecture code name Ivy	Bridge-EP
		06_3EH : Intel Xeon Processor E5-1600 v2/E5-2600 v2 Product Families based on Intel® microarchitecture code name Ivy Bridge-EP, Intel Core i7-49xx Processor Extreme Edition

		Intel Core i7-4930K CPU @ 3.40GHz, i7-4960X,i7-4820K

		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// Ivy Bridge-E
		// Ivy Bridge-E. Desktop
		{_T("i7-49"), 0, 6, 0x3E, -1, CPU_IVYBRIDGE_EP, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_22, -1}, // SI101039
		{_T("i7-48"), 0, 6, 0x3E, -1, CPU_IVYBRIDGE_EP, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_22, -1}, // SI101039

		// Ivy Bridge-EN
		{_T("E5-14"), 0, 6, 0x3E, -1, CPU_IVYBRIDGE_EP, CPU_ST_BLANK, CPU_S_LGA1356, CPU_F_22, -1}, // SI101066
		{_T("E5-24"), 0, 6, 0x3E, -1, CPU_IVYBRIDGE_EP, CPU_ST_BLANK, CPU_S_LGA1356, CPU_F_22, -1}, // SI101066

		// Ivy Bridge-EP
		{_T("E5-16"), 0, 6, 0x3E, -1, CPU_IVYBRIDGE_EP, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_22, -1},	  // SI101039
		{_T("E5-26"), 0, 6, 0x3E, -1, CPU_IVYBRIDGE_EP, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_22, -1},	  // SI101039
		{_T("E5-46"), 0, 6, 0x3E, -1, CPU_IVYBRIDGE_EP, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_22, -1},	  // SI101066
																									  // Genuine Intel(R) CPU E2697V @ 2.70GHz
		{_T("CPU E26"), 0, 6, 0x3E, -1, CPU_IVYBRIDGE_EP, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_22, -1}, // SI101066

		// Ivy Bridge-EX
		{_T("E7-28"), 0, 6, 0x3E, -1, CPU_IVYBRIDGE_EP, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_22, -1}, // SI101066
		{_T("E7-48"), 0, 6, 0x3E, -1, CPU_IVYBRIDGE_EP, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_22, -1}, // SI101066
		{_T("E7-88"), 0, 6, 0x3E, -1, CPU_IVYBRIDGE_EP, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_22, -1}, // SI101066

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:3F
		/*
		06_3FH : Future Generation Intel Xeon Processor
			Intel® Xeon® Processor Family (CPUID DisplayFamily_DisplayModel = 06_3F), based on Haswell microarchitecture

		Intel(R) Core(TM) i7-5930K CPU @ 3.50GHz	2 	6 	63 	2 	2014-08-30
		Intel(R) Core(TM) i7-5960X CPU @ 3.00GHz	5 	6 	63 	2 	2014-08-28
		Intel(R) Xeon(R) CPU E5-2670 v3 @ 2.30GHz	2 	6 	63 	2 	2014-08-31
		Intel(R) Xeon(R) CPU E5-2697 v3 @ 2.60GHz	2 	6 	63 	2 	2014-08-28
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// Intel Core i7-5960X, -5930K And -5820K
		{_T("i7-58"), 0, 6, 0x3F, 2, CPU_HASWELL_E, CPU_ST_R2, CPU_S_LGA2011_V3, CPU_F_22, -1},		// SI101077
		{_T("i7-58"), 0, 6, 0x3F, -1, CPU_HASWELL_E, CPU_ST_BLANK, CPU_S_LGA2011_V3, CPU_F_22, -1}, // SI101077
		{_T("i7-59"), 0, 6, 0x3F, 2, CPU_HASWELL_E, CPU_ST_R2, CPU_S_LGA2011_V3, CPU_F_22, -1},		// SI101077
		{_T("i7-59"), 0, 6, 0x3F, -1, CPU_HASWELL_E, CPU_ST_BLANK, CPU_S_LGA2011_V3, CPU_F_22, -1}, // SI101077

		// Xeons
		{_T("E5-26"), 0, 6, 0x3F, -1, CPU_HASWELL_E, CPU_ST_BLANK, CPU_S_LGA2011_V3, CPU_F_22, -1}, // SI101097 correction //SI101079 - correction (not Haswell E) //SI101077
		{_T("E5-24"), 0, 6, 0x3F, -1, CPU_HASWELL_E, CPU_ST_BLANK, CPU_S_LGA1356, CPU_F_22, -1},	// SI101098
		{_T("E5-16"), 0, 6, 0x3F, -1, CPU_HASWELL_E, CPU_ST_BLANK, CPU_S_LGA2011_V3, CPU_F_22, -1}, // SI101079
		{_T("E5-14"), 0, 6, 0x3F, -1, CPU_HASWELL_E, CPU_ST_BLANK, CPU_S_LGA1356, CPU_F_22, -1},	// SI101098

		{_T("E7-4"), 0, 6, 0x3F, -1, CPU_HASWELL_E, CPU_ST_BLANK, CPU_S_LGA2011_V3, CPU_F_22, -1}, // SI101097
		{_T("E7-8"), 0, 6, 0x3F, -1, CPU_HASWELL_E, CPU_ST_BLANK, CPU_S_LGA2011_V3, CPU_F_22, -1}, // SI101097

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 06_45H, 06_46H (same grouping as 06_3C)
		/*
		4th Generation Intel Core Processor and Intel Xeon Processor E3-1200 v3 Product Family based on Intel® microarchitecture code name Haswell
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// 06_45H e.g Intel Celeron 2955U @ 1.40GHz

		// Haswell-ULT" (SiP, 22 nm)
		{_T("Celeron 2955U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101047
		{_T("Celeron 2957U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("Celeron 2980U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("Celeron 2981U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("2961Y"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1},			 // SI101075

		{_T("Pentium 3560Y"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101047
		{_T("Pentium 3556U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101047

		{_T("3561Y"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101075
		{_T("3558U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_22, -1}, // SI101075

		//"Haswell-ULT" (SiP, 22 nm)
		{_T("i3-4158U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i3-4005U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i3-4010U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i3-4025U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i3-4030U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i3-4100U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i3-4120U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066

		//"Haswell-ULX" (SiP, 22 nm)
		{_T("i3-4010Y"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101047
		{_T("i3-4012Y"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i3-4020Y"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i3-4030Y"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066

		//"Haswell-H" (22 nm)
		{_T("i3-4100E"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i3-4110E"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i3-4102E"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i3-4112E"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066

		//"Haswell-ULT" (SiP, dual-core, 22 nm)
		// PT database shows some of these these as 6.45h (not 6.46h)
		{_T("i5-4258U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i5-4288U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i5-4278U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i5-4200U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i5-4210U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i5-4250U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i5-4260U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i5-4300U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i5-4310U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i5-4350U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i5-4360U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075

		//"Haswell-MB" (quad-core, 22 nm)
		{_T("i5-4200M"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066
		{_T("i5-4210M"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066
		{_T("i5-4300M"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066
		{_T("i5-4310M"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066
		{_T("i5-4330M"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066
		{_T("i5-4340M"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066

		//"Haswell-ULT" (SiP, dual-core, 22 nm)
		{_T("i5-4258U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i5-4288U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i5-4200U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i5-4210U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i5-4250U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i5-4260U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i5-4300U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i5-4310U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i5-4350U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i5-4360U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066

		//"Haswell-ULX" (SiP, dual-core, 22 nm)
		{_T("i5-4200Y"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i5-4202Y"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i5-4210Y"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i5-4220Y"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i5-4300Y"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i5-4302Y"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066

		//"Haswell-H" (22 nm)
		{_T("i5-4200H"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1},	 // SI101066
		{_T("i5-4400E"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1},	 // SI101066
		{_T("i5-4410E"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1},	 // SI101066
		{_T("i5-4402E"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1},	 // SI101066
		{_T("i5-4402EC"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i5-4422E"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1},	 // SI101066

		//"Haswell-ULT" (SiP, dual-core, 22 nm)
		{_T("i7-4558U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i7-4500U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i7-4510U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i7-4550U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i7-4600U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075
		{_T("i7-4650U"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075

		//"Haswell-ULX" (SiP, dual-core, 22 nm)
		{_T("i7-4610Y"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101075

		{_T("i7-47"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101075
		{_T("i7-48"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101075
		{_T("i7-49"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101075

		{_T("i5-42"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101075
		{_T("i5-43"), 0, 6, 0x45, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101075

		//"Haswell-ULT" (SiP, dual-core, 22 nm)
		{_T("i7-4558U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i7-4500U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i7-4510U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i7-4550U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i7-4600U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066
		{_T("i7-4650U"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066

		//"Haswell-ULX" (SiP, dual-core, 22 nm)
		{_T("i7-4610Y"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1168, CPU_F_22, -1}, // SI101066

		//"Haswell-MB" (quad-core, 22 nm)
		{_T("i7-4700MQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066
		{_T("i7-4702MQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066
		{_T("i7-4710MQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066
		{_T("i7-4712MQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066
		{_T("i7-4800MQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066
		{_T("i7-4810MQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066
		{_T("i7-4900MQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066
		{_T("i7-4910MQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066
		{_T("i7-4930MX"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066
		{_T("i7-4940MX"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_SOCKET_G3, CPU_F_22, -1}, // SI101066

		//"Haswell-H" (MCP, quad-core, 22 nm)
		{_T("i7-4700HQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4702HQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4710HQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4712HQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4750HQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4760HQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4850HQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4860HQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4950HQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4960HQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4700EC"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4700EQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4701EQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4850EQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4860EQ"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-4702EC"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066

		{_T("i7-47"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-48"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066
		{_T("i7-49"), 0, 6, 0x46, -1, CPU_HASWELL, CPU_ST_BLANK, CPU_S_FCBGA1364, CPU_F_22, -1}, // SI101066

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:47
		// 5th generation Intel Core processors, Intel Xeon processor E3 - 1200 v4 product family based on
		// Broadwell microarchitecture
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// Centerton
		//{ _T("Atom S12"), 0, 6, 0x47, -1, CPU_CENTERTON, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1 },			//SI101047 //Removed SI101109 - seems this is incorrect

		// Desktop Broadwell
		{_T("i7-5"), 0, 6, 0x47, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_14, -1}, // SI101109
		{_T("i5-5"), 0, 6, 0x47, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_14, -1}, // SI101109
		{_T("i3-5"), 0, 6, 0x47, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_14, -1}, // SI101109

		// Xeon Broadwell
		{_T("Xeon"), 0, 6, 0x47, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_LGA1150, CPU_F_14, -1}, // SI101109

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:4C  Intel® Atom™ processor Z8000 series, "Airmont Microarchitecture"
		/*
		Intel(R) Atom(TM) x7-Z8700 CPU @ 1.60GHz
		Intel(R) Pentium(R) CPU N3700 @ 1.60GHz
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// CherryTrail
		{_T("x7-Z8700"), 0, 6, 0x4C, -1, CPU_CHERRYTRAIL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_14, 90}, // SI101100
		{_T("x5-Z8500"), 0, 6, 0x4C, -1, CPU_CHERRYTRAIL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_14, 90}, // SI101100
		{_T("x5-Z8300"), 0, 6, 0x4C, -1, CPU_CHERRYTRAIL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_14, 90}, // SI101100
		{_T("-Z8"), 0, 6, 0x4C, -1, CPU_CHERRYTRAIL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_14, -1},		// SI101100

		// Braswell
		{_T("N3700"), 0, 6, 0x4C, -1, CPU_BRASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_14, 90},  // SI101100
		{_T("N3000"), 0, 6, 0x4C, -1, CPU_BRASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_14, 90},  // SI101100
		{_T("N3050"), 0, 6, 0x4C, -1, CPU_BRASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_14, 90},  // SI101100
		{_T("N3150"), 0, 6, 0x4C, -1, CPU_BRASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_14, 90},  // SI101100
		{_T("CPU N3"), 0, 6, 0x4C, -1, CPU_BRASWELL, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_14, -1}, // SI101100

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:4D  Intel Atom Processor C2000, E3000 series (grouped with 06_37H)
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// CPU_AVOTON, CPU_FC_BGA_1283
		{_T("C2750"), 0, 6, 0x4D, -1, CPU_AVOTON, CPU_ST_BLANK, CPU_FC_BGA_1283, CPU_F_22, -1}, // Correction SI101098 //SI101066
		{_T("C2730"), 0, 6, 0x4D, -1, CPU_AVOTON, CPU_ST_BLANK, CPU_FC_BGA_1283, CPU_F_22, -1}, // SI101098
		{_T("C2550"), 0, 6, 0x4D, -1, CPU_AVOTON, CPU_ST_BLANK, CPU_FC_BGA_1283, CPU_F_22, -1}, // SI101098
		{_T("C2530"), 0, 6, 0x4D, -1, CPU_AVOTON, CPU_ST_BLANK, CPU_FC_BGA_1283, CPU_F_22, -1}, // SI101098

		// Rangeley
		// Intel(R) Atom(TM) CPU C2338 @ 1.74GHz
		{_T("C2758"), 0, 6, 0x4D, -1, CPU_RANGELEY, CPU_ST_BLANK, CPU_FC_BGA_1283, CPU_F_22, -1}, // SI101098
		{_T("C2738"), 0, 6, 0x4D, -1, CPU_RANGELEY, CPU_ST_BLANK, CPU_FC_BGA_1283, CPU_F_22, -1}, // SI101098
		{_T("C2718"), 0, 6, 0x4D, -1, CPU_RANGELEY, CPU_ST_BLANK, CPU_FC_BGA_1283, CPU_F_22, -1}, // SI101098
		{_T("C2558"), 0, 6, 0x4D, -1, CPU_RANGELEY, CPU_ST_BLANK, CPU_FC_BGA_1283, CPU_F_22, -1}, // SI101098
		{_T("C2538"), 0, 6, 0x4D, -1, CPU_RANGELEY, CPU_ST_BLANK, CPU_FC_BGA_1283, CPU_F_22, -1}, // SI101098
		{_T("C2518"), 0, 6, 0x4D, -1, CPU_RANGELEY, CPU_ST_BLANK, CPU_FC_BGA_1283, CPU_F_22, -1}, // SI101098
		{_T("C2508"), 0, 6, 0x4D, -1, CPU_RANGELEY, CPU_ST_BLANK, CPU_FC_BGA_1283, CPU_F_22, -1}, // SI101098
		{_T("C2358"), 0, 6, 0x4D, -1, CPU_RANGELEY, CPU_ST_BLANK, CPU_FC_BGA_1283, CPU_F_22, -1}, // SI101098
		{_T("C2338"), 0, 6, 0x4D, -1, CPU_RANGELEY, CPU_ST_BLANK, CPU_FC_BGA_1283, CPU_F_22, -1}, // SI101098
		{_T("C2308"), 0, 6, 0x4D, -1, CPU_RANGELEY, CPU_ST_BLANK, CPU_FC_BGA_1283, CPU_F_22, -1}, // SI101098

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:4E  Skylake (maybe BGA variants???)
		// Intel(R) Core(TM) m7-6Y75 CPU @ 1.20GHz [2967.8 MHz], - Skylake Y ?
		/*
		CPU:
		CPU manufacturer: GenuineIntel
		CPU Type: Intel(R) Core(TM) m7-6Y75 CPU @ 1.20GHz
		CPUID: Family 6, Model 4E, Stepping 3
		Physical CPU's: 1
		Cores per CPU: 2
		Hyperthreading: Enabled
		CPU features: MMX SSE SSE2 SSE3 SSSE3 SSE4.1 SSE4.2 DEP PAE Intel64 VMX SMX AES AVX AVX2 XOP FMA3
		Clock frequencies:
		-  Measured CPU speed: 2967.8 MHz
		Cache per CPU package:
		-  L1 Instruction Cache: 4 x 32 KB
		-  L1 data cache: 4 x 32 KB
		-  L2 cache: 4 x 256 KB
		-  L3 cache: 4 MB

		Graphics
		Intel(R) HD Graphics 515
		Chip Type: Intel(R) HD Graphics Family
		DAC Type: Internal
		Memory: 1024MB
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("m7"), 0, 6, 0x4E, -1, CPU_SKYLAKE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_14, -1}, // SI101108
		{_T("m5"), 0, 6, 0x4E, -1, CPU_SKYLAKE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_14, -1}, // SI101108 - a guess
		{_T("m3"), 0, 6, 0x4E, -1, CPU_SKYLAKE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_14, -1}, // SI101108 - a guess

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:4F
		/*
		06_4FH : Future Generation Intel Xeon processor based on Broadwell microarchitecture
		Intel® Xeon® Processor Family (CPUID DisplayFamily_DisplayModel = 06_4F), based on Broadwell microarchitecture

		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// Intel Core i7-5960X, -5930K And -5820K
		//  { _T("i7-58"), 0, 6, 0x4F, 2, CPU_BROADWELL_E, CPU_ST_R2, CPU_S_LGA2011_V3, CPU_F_14, -1 },						//SI101112
		{_T("i7-58"), 0, 6, 0x4F, -1, CPU_BROADWELL_E, CPU_ST_BLANK, CPU_S_LGA2011_V3, CPU_F_14, -1}, // SI101112
																									  //{ _T("i7-59"), 0, 6, 0x4F, 2, CPU_BROADWELL_E, CPU_ST_R2, CPU_S_LGA2011_V3, CPU_F_14, -1 },						//SI101112
		{_T("i7-59"), 0, 6, 0x4F, -1, CPU_BROADWELL_E, CPU_ST_BLANK, CPU_S_LGA2011_V3, CPU_F_14, -1}, // SI101112

		// Xeons
		{_T("E5-26"), 0, 6, 0x4F, -1, CPU_BROADWELL_E, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_14, -1}, // SI101112
																								   //  { _T("E5-24"), 0, 6, 0x4F, -1, CPU_BROADWELL_E, CPU_ST_BLANK, CPU_S_LGA1356, CPU_F_14, -1 },						//SI101112
		{_T("E5-16"), 0, 6, 0x4F, -1, CPU_BROADWELL_E, CPU_ST_BLANK, CPU_S_LGA2011, CPU_F_14, -1}, // SI101112
																								   //  { _T("E5-14"), 0, 6, 0x4F, -1, CPU_BROADWELL_E, CPU_ST_BLANK, CPU_S_LGA1356, CPU_F_14, -1 },						//SI101112

		{_T("E7-4"), 0, 6, 0x4F, -1, CPU_BROADWELL_E, CPU_ST_BLANK, CPU_S_LGA2011_V3, CPU_F_14, -1}, // SI101112
		{_T("E7-8"), 0, 6, 0x4F, -1, CPU_BROADWELL_E, CPU_ST_BLANK, CPU_S_LGA2011_V3, CPU_F_14, -1}, // SI101112

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:56  Broadwell 14nm
		/*
		Intel(R) Xeon(R) CPU D-1540 @ 2.00GHz
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("D-1520"), 0, 6, 0x56, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1667, CPU_F_14, -1}, // SI101100						//SI101098
		{_T("D-1540"), 0, 6, 0x56, -1, CPU_BROADWELL, CPU_ST_BLANK, CPU_S_FCBGA1667, CPU_F_14, -1}, // SI101100						//SI101098

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:57  Knights Landing (2nd gen Phi Xeon)
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Xeon"), 0, 6, 0x57, -1, CPU_KNIGHTSLANDING, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_14, -1}, // SI101100

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:5C  Apollo Lake
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("CPU @ 1.10GHz"), 0, 6, 0x5C, -1, CPU_APOLLO_LAKE, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_14, -1},

		// Skylake-X
		// i9-7900X, i9-7920X, i9-7940X, i9-7960X, i9-7980XE
		{_T("i9-7"), 0, 6, 0x55, -1, CPU_SKYLAKE, CPU_ST_BLANK, CPU_S_LGA2066, CPU_F_14, -1},
		{_T("i7-7"), 0, 6, 0x55, -1, CPU_SKYLAKE, CPU_ST_BLANK, CPU_S_LGA2066, CPU_F_14, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// INTEL 6:5E  Skylake
		// Intel(R) Core(TM) i5-6500T CPU @ 2.50GHz,
		/*
		CPU:
		CPU manufacturer : GenuineIntel
		CPU Type : Genuine Intel(R) CPU 0000 @ 1.60GHz
		CPUID: Family 6, Model 5E, Stepping 1
		Physical CPU's: 1
		Cores per CPU : 4
		Hyperthreading : Enabled
		CPU features : MMX SSE SSE2 SSE3 SSSE3 SSE4.1 SSE4.2 DEP PAE Intel64 VMX SMX AES AVX AVX2 XOP FMA3
		Clock frequencies :
		-Measured CPU speed : 2196.4 MHz

		Graphics
		Intel Skylake HD Graphics DT GT2
		Chip Type : Intel(R) Skylake Desktop Graphics Controller
		DAC Type : Internal
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("i7"), 0, 6, 0x5E, -1, CPU_SKYLAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1}, // SI101106
		{_T("i5"), 0, 6, 0x5E, -1, CPU_SKYLAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1}, // SI101106
		{_T("i3"), 0, 6, 0x5E, -1, CPU_SKYLAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1}, // SI101106

		// Kaby Lake X
		{_T("i7-7740X"), 0, 6, 0x9E, -1, CPU_SKYLAKE, CPU_ST_BLANK, CPU_S_LGA2066, CPU_F_14, -1},
		{_T("i5-7640X"), 0, 6, 0x9E, -1, CPU_SKYLAKE, CPU_ST_BLANK, CPU_S_LGA2066, CPU_F_14, -1},

		// 7th gen Intel Kaby Lake
		{_T("i7-7"), 0, 6, 0x9E, -1, CPU_KABY_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1},
		{_T("i5-7"), 0, 6, 0x9E, -1, CPU_KABY_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1},
		{_T("i3-7"), 0, 6, 0x9E, -1, CPU_KABY_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1},

		// 8th Gen Intel  Coffee Lake
		{_T("i7-8"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1},
		{_T("i5-8"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1},
		{_T("i3-8"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1},
		{_T("G5"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1},		// Pentium Gold
		{_T("G4"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1},		// Celeron
		{_T("E-21"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151_2, CPU_F_14, -1}, // Xeon

		/*
		{ _T("i7-8700K"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1 },
		{ _T("i7-8700"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1 },
		{ _T("i5-8600K"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1 },
		{ _T("i5-8400"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1 },
		{ _T("i3-8350K"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1 },
		{ _T("i3-8100"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1 },
		*/

		// 9th gen mobile - FCBGA1440
		{_T("i9-9980HK"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1440, CPU_F_14, -1},
		{_T("i9-9880H"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1440, CPU_F_14, -1},
		{_T("i7-9850H"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1440, CPU_F_14, -1},
		{_T("i7-9750H"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1440, CPU_F_14, -1},
		{_T("i5-9400H"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1440, CPU_F_14, -1},
		{_T("i5-9300H"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1440, CPU_F_14, -1},

		// 9th Gen Intel Coffee Lake still
		{_T("E-22"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1}, // Xeon 2288G etc
		{_T("i9-9"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1},
		{_T("i7-9"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1},
		{_T("i5-9"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1},
		{_T("i3-9"), 0, 6, 0x9E, -1, CPU_COFFEE_LAKE, CPU_ST_BLANK, CPU_S_LGA1151, CPU_F_14, -1},

		// 10th Gen Intel Comet lake
		//  S/H models family 0xA5
		// Mobile
		{_T("W-10"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1440, CPU_F_14, -1}, // Xeon mobile
		{_T("i9-10980HK"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1440, CPU_F_14, -1},
		{_T("i9-10885H"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1440, CPU_F_14, -1},
		{_T("i7-10750H"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1440, CPU_F_14, -1},
		{_T("i7-108"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1440, CPU_F_14, -1},
		{_T("i5-10400H"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1440, CPU_F_14, -1},
		{_T("i5-10300H"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1440, CPU_F_14, -1},
		{_T("i5-10200H"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1440, CPU_F_14, -1},

		// Desktop
		{_T("i9-109"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_LGA1200, CPU_F_14, -1},
		{_T("i9-108"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_LGA1200, CPU_F_14, -1},
		{_T("i7-107"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_LGA1200, CPU_F_14, -1},
		{_T("i5-10"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_LGA1200, CPU_F_14, -1},
		{_T("i3-10"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_LGA1200, CPU_F_14, -1},
		{_T("G6"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_LGA1200, CPU_F_14, -1},	 // Pentium gold
		{_T("G5"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_LGA1200, CPU_F_14, -1},	 // Celeron
		{_T("W-12"), 0, 6, 0xA5, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_LGA1200, CPU_F_14, -1}, // Xeon w-1290P to 1250

		// U models family 0x8E
		{_T("i7-10"), 0, 6, 0x8E, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1528, CPU_F_14, -1},
		{_T("i5-10"), 0, 6, 0x8E, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1528, CPU_F_14, -1},
		{_T("i3-10"), 0, 6, 0x8E, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1528, CPU_F_14, -1},
		{_T("6405U"), 0, 6, 0x8E, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1528, CPU_F_14, -1},
		{_T("5305U"), 0, 6, 0x8E, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1528, CPU_F_14, -1},
		{_T("5205U"), 0, 6, 0x8E, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1528, CPU_F_14, -1},

		// 10th Gen Ice lake
		{_T("i7-10"), 0, 6, 0x7E, -1, CPU_ICE_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1526, CPU_F_10, -1},
		{_T("i5-10"), 0, 6, 0x7E, -1, CPU_ICE_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1526, CPU_F_10, -1},
		{_T("i3-10"), 0, 6, 0x7E, -1, CPU_ICE_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1526, CPU_F_10, -1},

		// 11th Gen Tiger lake
		{_T("i9-11"), 0, 6, 0x8C, -1, CPU_TIGER_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1449, CPU_F_10, -1},
		{_T("i7-11"), 0, 6, 0x8C, -1, CPU_TIGER_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1449, CPU_F_10, -1},
		{_T("i5-11"), 0, 6, 0x8C, -1, CPU_TIGER_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1449, CPU_F_10, -1},
		{_T("i3-11"), 0, 6, 0x8C, -1, CPU_TIGER_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1449, CPU_F_10, -1},

		// 12th gen Alder Lake
		{_T("i9-12"), 0, 6, 0x97, -1, CPU_ALDER_LAKE, CPU_ST_BLANK, CPU_S_LGA1700, CPU_F_10, -1},
		{_T("i7-12"), 0, 6, 0x97, -1, CPU_ALDER_LAKE, CPU_ST_BLANK, CPU_S_LGA1700, CPU_F_10, -1},
		{_T("i5-12"), 0, 6, 0x97, -1, CPU_ALDER_LAKE, CPU_ST_BLANK, CPU_S_LGA1700, CPU_F_10, -1},
		{_T("i3-12"), 0, 6, 0x97, -1, CPU_ALDER_LAKE, CPU_ST_BLANK, CPU_S_LGA1700, CPU_F_10, -1},

		// 12th gen Alder lake mobile
		{_T("i9-12"), 0, 6, 0x9A, -1, CPU_ALDER_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1744, CPU_F_10, -1},
		{_T("i7-12"), 0, 6, 0x9A, -1, CPU_ALDER_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1744, CPU_F_10, -1},
		{_T("i5-12"), 0, 6, 0x9A, -1, CPU_ALDER_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1744, CPU_F_10, -1},
		{_T("i3-12"), 0, 6, 0x9A, -1, CPU_ALDER_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1744, CPU_F_10, -1},

		// 13th gen Raptor Lake
		{_T("i9-13"), 0, 6, 0xB7, -1, CPU_RAPTOR_LAKE, CPU_ST_BLANK, CPU_S_LGA1700, CPU_F_10, -1},
		{_T("i7-13"), 0, 6, 0xB7, -1, CPU_RAPTOR_LAKE, CPU_ST_BLANK, CPU_S_LGA1700, CPU_F_10, -1},
		{_T("i5-13"), 0, 6, 0xB7, -1, CPU_RAPTOR_LAKE, CPU_ST_BLANK, CPU_S_LGA1700, CPU_F_10, -1},
		{_T("i3-13"), 0, 6, 0xB7, -1, CPU_RAPTOR_LAKE, CPU_ST_BLANK, CPU_S_LGA1700, CPU_F_10, -1},

		//{ _T("i7-10510Y"), 0, 6, 0x8E, -1, CPU_COMET_LAKE, CPU_ST_BLANK, CPU_S_FCBGA1377, CPU_F_14, -1 }, AMBER lake

		// http://en.wikipedia.org/wiki/Comparison_of_AMD_processors
		// Google:
		//	keene site:http://valid.canardpc.com
		//   "F.23" -intel site:http://valid.canardpc.com

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 6:x - NOT DONE
		/*
		AuthenticAMD	 0x6	 0x1	 0x2	 AMD-K7(tm) Processor
		AuthenticAMD	 0x6	 0x2	 0x1	 AMD Athlon(tm) Processor
		AuthenticAMD	 0x6	 0x2	 0x2	 AMD Athlon(tm) Processor
		AuthenticAMD	 0x6	 0x3	 0x0	 AMD Duron(tm) Processor
		AuthenticAMD	 0x6	 0x3	 0x1	 AMD Duron(tm)
		AuthenticAMD	 0x6	 0x3	 0x1	 AMD Duron(tm) Processor
		AuthenticAMD	 0x6	 0x4	 0x2	 AMD Athlon(tm)
		AuthenticAMD	 0x6	 0x4	 0x2	 AMD Athlon(tm) Processor
		AuthenticAMD	 0x6	 0x4	 0x2	 AMD Athlon(TM)Processor
		AuthenticAMD	 0x6	 0x4	 0x4	 AMD Athlon(tm)
		AuthenticAMD	 0x6	 0x4	 0x4	 AMD Athlon(tm) Processor
		AuthenticAMD	 0x6	 0x6	 0x1	 AMD Athlon(tm) Processor
		AuthenticAMD	 0x6	 0x6	 0x1	 mobile AMD Athlon(tm) 4
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(tm)
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(tm) 4 Processor
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(tm) MP
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(tm) Processor
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(tm) XP
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(tm) XP 1500+
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(tm) XP 1600+
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(TM) XP 1700+
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(tm) XP 1800+
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(tm) XP 1900+
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(tm) XP 2000+
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(tm) XP 2100+
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(tm) XP processor
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(tm) XP processor 1800+
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(tm) XP processor 2000+
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(TM) XP1700+
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(TM) XP1900+
		AuthenticAMD	 0x6	 0x6	 0x2	 AMD Athlon(TM) XP2100+
		AuthenticAMD	 0x6	 0x7	 0x0	 AMD Athlon(tm) Processor
		AuthenticAMD	 0x6	 0x7	 0x0	 AMD Duron(tm) processor
		AuthenticAMD	 0x6	 0x7	 0x1	 AMD Duron(tm)
		AuthenticAMD	 0x6	 0x7	 0x1	 AMD Duron(tm) processor
		AuthenticAMD	 0x6	 0x7	 0x1	 AMD Duron(TM)Processor
		AuthenticAMD	 0x6	 0x7	 0x1	 mobile AMD Duron(tm)
		AuthenticAMD	 0x6	 0x8	 0x0	 AMD Athlon(tm)
		AuthenticAMD	 0x6	 0x8	 0x0	 AMD Athlon(tm) 1500+
		AuthenticAMD	 0x6	 0x8	 0x0	 AMD Athlon(tm) MP qwsossor
		AuthenticAMD	 0x6	 0x8	 0x0	 AMD Athlon(tm) Processor
		AuthenticAMD	 0x6	 0x8	 0x0	 AMD Athlon(tm) XP
		AuthenticAMD	 0x6	 0x8	 0x0	 AMD Athlon(tm) XP 1700+
		AuthenticAMD	 0x6	 0x8	 0x0	 AMD Athlon(tm) XP 1800+
		AuthenticAMD	 0x6	 0x8	 0x0	 AMD Athlon(tm) XP 1900+
		AuthenticAMD	 0x6	 0x8	 0x0	 AMD Athlon(TM) XP 2000+
		AuthenticAMD	 0x6	 0x8	 0x0	 AMD Athlon(tm) XP 2200+
		AuthenticAMD	 0x6	 0x8	 0x0	 mobile AMD Athlon (tm) 1800+
		AuthenticAMD	 0x6	 0x8	 0x0	 Mobile AMD Athlon(tm) XP 1500+
		AuthenticAMD	 0x6	 0x8	 0x0	 Mobile AMD Athlon(tm) XP 1600+
		AuthenticAMD	 0x6	 0x8	 0x0	 Mobile AMD Athlon(tm) XP 1800+
		AuthenticAMD	 0x6	 0x8	 0x0	 mobile AMD Athlon(tm) XP 1900+
		AuthenticAMD	 0x6	 0x8	 0x0	 mobile AMD Athlon(tm) XP-M 1800+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm)
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) 1800+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) 2000+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) MP
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) MP 2000+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) MP 2600+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) Processor
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) Proswssor
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) XP
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) XP 1500+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) XP 1700+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(TM) XP 1800+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) XP 1900+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) XP 2000+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) XP 2100+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(TM) XP 2200+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) XP 2300+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(TM) XP 2400+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) XP 2600+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) XP 2700+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Athlon(tm) XP 2800+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Duron(tm)
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Duron(tm) processor
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Geode NX
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD K7 processor
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Sempron(tm)
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Sempron(tm) 2200+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Sempron(tm) 2300+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Sempron(tm) 2400+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Sempron(tm) 2500+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Sempron(TM) 2600+
		AuthenticAMD	 0x6	 0x8	 0x1	 AMD Sempron(tm) 2800+
		AuthenticAMD	 0x6	 0x8	 0x1	 mobile AMD Athlon(tm) MP-M 2000+
		AuthenticAMD	 0x6	 0x8	 0x1	 Mobile AMD Athlon(tm) XP 2000+
		AuthenticAMD	 0x6	 0x8	 0x1	 mobile AMD Athlon(tm) XP 2200+
		AuthenticAMD	 0x6	 0x8	 0x1	 mobile AMD Athlon(tm) XP2200+
		AuthenticAMD	 0x6	 0x8	 0x1	 mobile AMD Athlon(tm) XP-M 1600+
		AuthenticAMD	 0x6	 0x8	 0x1	 TURBO 3200+X COMB SERIES
		AuthenticAMD	 0x6	 0x8	 0x1	 Turbo 3400L Comb featuring VIG4 and AMD NX1750
		AuthenticAMD	 0x6	 0x8	 0x1	 Unknown CPU Type
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon XP-M
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm)
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) 2500+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) 2800+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) 3100+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) MP
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) MP 2600+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) MP 2800+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) processor
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) Pros}ssor
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) prosussor
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP 1500+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP 1700+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP 1800+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP 1900+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP 2000+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP 2100+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP 2200+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(TM) XP 2400+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP 2500+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP 2600+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP 2700+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP 2800+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP 2900+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(TM) XP 3000+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP 3100+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP 3200+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Athlon(tm) XP rrsossor
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD K7 processo
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Sempron(tm)
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Sempron(tm) 2200+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Sempron(tm) 2800+
		AuthenticAMD	 0x6	 0xa	 0x0	 AMD Sempron(tm) 3000+
		AuthenticAMD	 0x6	 0xa	 0x0	 Mobile AMD Athlon (tm) XP-M 2200+
		AuthenticAMD	 0x6	 0xa	 0x0	 Mobile AMD Athlon(tm) XP 2400+
		AuthenticAMD	 0x6	 0xa	 0x0	 Mobile AMD Athlon(tm) XP 2500+
		AuthenticAMD	 0x6	 0xa	 0x0	 mobile AMD Athlon(tm) XP 2600+
		AuthenticAMD	 0x6	 0xa	 0x0	 Mobile AMD Athlon(tm) XP 3000+
		AuthenticAMD	 0x6	 0xa	 0x0	 mobile AMD Athlon(tm) XP2400+
		AuthenticAMD	 0x6	 0xa	 0x0	 mobile AMD Athlon(tm) XP2500+
		AuthenticAMD	 0x6	 0xa	 0x0	 mobile AMD Athlon(tm) XP2800+
		AuthenticAMD	 0x6	 0xa	 0x0	 mobile AMD Athlon(tm) XP-M (LV) 2000+
		AuthenticAMD	 0x6	 0xa	 0x0	 mobile AMD Athlon(tm) XP-M (LV) 2600+
		AuthenticAMD	 0x6	 0xa	 0x0	 mobile AMD Athlon(tm) XP-M 2000+
		AuthenticAMD	 0x6	 0xa	 0x0	 mobile AMD Athlon(tm) XP-M 2200+
		AuthenticAMD	 0x6	 0xa	 0x0	 mobile AMD Athlon(tm) XP-M 2400+
		AuthenticAMD	 0x6	 0xa	 0x0	 mobile AMD Athlon(tm) XP-M 2600+
		AuthenticAMD	 0x6	 0xa	 0x0	 mobile AMD Athlon(tm) XP-M 2800+
		AuthenticAMD	 0x6	 0xa	 0x0	 mobile AMD Athlon(tm) XP-M 3200+
		AuthenticAMD	 0x6	 0xa	 0x0	 Unknown CPU Typ
		AuthenticAMD	 0x6	 0xa	 0x0	 Unknown CPU Type
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Not done

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:4
		/*
		AuthenticAMD	 0xf	 0x4	 0x8	 AMD Athlon(tm) 64 Processor 2800+
		AuthenticAMD	 0xf	 0x4	 0x8	 AMD Athlon(tm) 64 Processor 3000+
		AuthenticAMD	 0xf	 0x4	 0x8	 AMD Athlon(tm) 64 Processor 3200+
		AuthenticAMD	 0xf	 0x4	 0x8	 AMD Athlon(tm) 64 Processor 3400+
		AuthenticAMD	 0xf	 0x4	 0x8	 AMD Athlon(tm) XP Processor 3000+
		AuthenticAMD	 0xf	 0x4	 0x8	 AMD Engineering Sample
		AuthenticAMD	 0xf	 0x4	 0xa	 AMD Athlon(tm) 64 Processor 2800+
		AuthenticAMD	 0xf	 0x4	 0xa	 AMD Athlon(tm) 64 Processor 3000+
		AuthenticAMD	 0xf	 0x4	 0xa	 AMD Athlon(tm) 64 Processor 3200+
		AuthenticAMD	 0xf	 0x4	 0xa	 AMD Athlon(tm) 64 Processor 3300+
		AuthenticAMD	 0xf	 0x4	 0xa	 AMD Athlon(tm) 64 Processor 3400+
		AuthenticAMD	 0xf	 0x4	 0xa	 AMD Athlon(tm) 64 Processor 3700+
		AuthenticAMD	 0xf	 0x4	 0xa	 AMD Athlon(tm) XP Processor 2800+
		AuthenticAMD	 0xf	 0x4	 0xa	 Mobile AMD Athlon(tm) 64 Processor 3000+
		AuthenticAMD	 0xf	 0x4	 0xa	 Mobile AMD Athlon(tm) 64 Processor 3200+
		AuthenticAMD	 0xf	 0x4	 0xa	 Mobile AMD Athlon(tm) 64 Processor 3400+
		AuthenticAMD	 0xf	 0x4	 0xa	 Mobile AMD Athlon(tm) XP-M Processor 2800+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("AMD Athlon(tm) 64"), 0, 0xF, 0x4, 0x8, CPU_CLAWHAMMER, CPU_ST_SH7_C0, CPU_S_754, CPU_F_130, -1},
		{_T("AMD Athlon(tm) 64"), 0, 0xF, 0x4, 0xa, CPU_CLAWHAMMER, CPU_ST_SH7_CG, CPU_S_939, CPU_F_130, -1},
		{_T("Mobile AMD Athlon(tm) 64"), 0, 0xF, 0x4, 0xa, CPU_CLAWHAMMER, CPU_ST_SH7_CG, CPU_S_754, CPU_F_130, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:5
		/*
		AuthenticAMD	 0xf	 0x5	 0x1	 AMD Opteron(tm) Processor 142
		AuthenticAMD	 0xf	 0x5	 0x1	 AMD Opteron(tm) Processor 240
		AuthenticAMD	 0xf	 0x5	 0x8	 AMD Athlon(tm) 64 FX-51 Processor
		AuthenticAMD	 0xf	 0x5	 0x8	 AMD Opteron(tm) Processor 144
		AuthenticAMD	 0xf	 0x5	 0x8	 AMD Opteron(tm) Processor 246
		AuthenticAMD	 0xf	 0x5	 0x8	 AMD Opteron(tm) Processor 248
		AuthenticAMD	 0xf	 0x5	 0xa	 AMD Athlon(tm) 64 FX-51 Processor
		AuthenticAMD	 0xf	 0x5	 0xa	 AMD Athlon(tm) 64 FX-53 Processor
		AuthenticAMD	 0xf	 0x5	 0xa	 AMD Opteron(tm) Processor 242
		AuthenticAMD	 0xf	 0x5	 0xa	 AMD Opteron(tm) Processor 244
		AuthenticAMD	 0xf	 0x5	 0xa	 AMD Opteron(tm) Processor 246
		AuthenticAMD	 0xf	 0x5	 0xa	 AMD Opteron(tm) Processor 250
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Opteron(tm) Processor"), 0, 0xF, 0x5, 0x1, CPU_SLEDGEHAMMER, CPU_ST_SH7_B3, CPU_S_940, CPU_F_130, -1},
		{_T("Opteron(tm) Processor"), 0, 0xF, 0x5, 0x8, CPU_SLEDGEHAMMER, CPU_ST_SH7_C0, CPU_S_940, CPU_F_130, -1},
		{_T("Opteron(tm) Processor"), 0, 0xF, 0x5, 0xa, CPU_SLEDGEHAMMER, CPU_ST_SH7_CG, CPU_S_940, CPU_F_130, -1},

		{_T("Athlon(tm) 64 FX-"), 0, 0xF, 0x5, 0x1, CPU_SLEDGEHAMMER, CPU_ST_SH7_B3, CPU_S_940, CPU_F_130, -1},
		{_T("Athlon(tm) 64 FX-"), 0, 0xF, 0x5, 0x8, CPU_SLEDGEHAMMER, CPU_ST_SH7_C0, CPU_S_940, CPU_F_130, -1},
		{_T("Athlon(tm) 64 FX-"), 0, 0xF, 0x5, 0xa, CPU_SLEDGEHAMMER, CPU_ST_SH7_CG, CPU_S_940, CPU_F_130, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:7
		/*
		AuthenticAMD	 0xf	 0x7	 0xa	 AMD Athlon(tm) 64 FX-53 Processor
		AuthenticAMD	 0xf	 0x7	 0xa	 AMD Athlon(tm) 64 FX-55 Processor
		AuthenticAMD	 0xf	 0x7	 0xa	 AMD Athlon(tm) 64 Processor 3500+
		AuthenticAMD	 0xf	 0x7	 0xa	 AMD Athlon(tm) 64 Processor 4000+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 FX"), 0, 0xF, 0x7, 0xa, CPU_CLAWHAMMER, CPU_ST_SH7_CG, CPU_S_939, CPU_F_130, -1},
		{_T("Athlon(tm) 64 Processor"), 0, 0xF, 0x7, 0xa, CPU_CLAWHAMMER, CPU_ST_SH7_CG, CPU_S_939, CPU_F_130, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:C
		/*
		AuthenticAMD	 0xf	 0xc	 0x0	 AMD Athlon(tm) 64 Processor 2800+
		AuthenticAMD	 0xf	 0xc	 0x0	 AMD Athlon(tm) 64 Processor 3000+
		AuthenticAMD	 0xf	 0xc	 0x0	 AMD Athlon(tm) 64 Processor 3200+
		AuthenticAMD	 0xf	 0xc	 0x0	 AMD Athlon(tm) 64 Processor 3300+
		AuthenticAMD	 0xf	 0xc	 0x0	 AMD Athlon(tm) 64 Processor 3400+
		AuthenticAMD	 0xf	 0xc	 0x0	 AMD Athlon(tm) XP Processor 2800+
		AuthenticAMD	 0xf	 0xc	 0x0	 AMD Athlon(tm) XP Processor 3000+
		AuthenticAMD	 0xf	 0xc	 0x0	 AMD Sempron(tm) Processor 3100+
		AuthenticAMD	 0xf	 0xc	 0x0	 Mobile AMD Athlon 64 Processor 2800+
		AuthenticAMD	 0xf	 0xc	 0x0	 Mobile AMD Athlon(tm) XP-M Processor 2800+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 Processor"), 0, 0xF, 0xc, 0x0, CPU_NEWCASTLE, CPU_ST_DH7_CG, CPU_S_754, CPU_F_130, -1},

		{_T("Sempron(tm) Processor"), 0, 0xF, 0xc, 0x0, CPU_PARIS, CPU_ST_DH7_CG, CPU_S_754, CPU_F_130, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:e
		/*
		AuthenticAMD	 0xf	 0xe	 0x0	 AMD Athlon(tm) 64 Processor 3000+
		AuthenticAMD	 0xf	 0xe	 0x0	 AMD Athlon(tm) 64 Processor 3400+
		AuthenticAMD	 0xf	 0xe	 0x0	 AMD Athlon(tm) XP Processor 3000+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 Processor 3"), 0, 0xF, 0xe, 0x0, CPU_NEWCASTLE, CPU_ST_DH7_CG, CPU_S_754, CPU_F_130, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:f
		/*
		AuthenticAMD	 0xf	 0xf	 0x0	 AMD Athlon(tm) 64 Processor 3200+
		AuthenticAMD	 0xf	 0xf	 0x0	 AMD Athlon(tm) 64 Processor 3400+
		AuthenticAMD	 0xf	 0xf	 0x0	 AMD Athlon(tm) 64 Processor 3500+
		AuthenticAMD	 0xf	 0xf	 0x0	 AMD Athlon(tm) 64 Processor 3800+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 Processor 3"), 0, 0xF, 0xf, 0x0, CPU_NEWCASTLE, CPU_ST_DH7_CG, CPU_S_939, CPU_F_130, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:1C
		/*
		AuthenticAMD	 0xf	 0x1c	 0x0	 AMD Sempron(tm) Processor 2600+
		AuthenticAMD	 0xf	 0x1c	 0x0	 AMD Sempron(tm) Processor 2800+
		AuthenticAMD	 0xf	 0x1c	 0x0	 AMD Sempron(tm) Processor 3000+
		AuthenticAMD	 0xf	 0x1c	 0x0	 AMD Sempron(tm) Processor 3100+
		AuthenticAMD	 0xf	 0x1c	 0x0	 AMD Sempron(tm) Processor 3300+
		AuthenticAMD	 0xf	 0x1c	 0x0	 AMD Sempron(tm) Processor 3400+
		AuthenticAMD	 0xf	 0x1c	 0x0	 Mobile AMD Athlon 64 Processor 3000+
		AuthenticAMD	 0xf	 0x1c	 0x0	 Mobile AMD Sempron(tm) 3100+
		AuthenticAMD	 0xf	 0x1c	 0x0	 Mobile AMD Sempron(tm) Processor 2600+
		AuthenticAMD	 0xf	 0x1c	 0x0	 Mobile AMD Sempron(tm) Processor 2800+
		AuthenticAMD	 0xf	 0x1c	 0x0	 Mobile AMD Sempron(tm) Processor 3000+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Mobile AMD Athlon 64"), 0, 0xF, 0x1c, 0x0, CPU_OAKVILLE, CPU_ST_DH8_D0, CPU_S_754, CPU_F_90, -1},

		{_T("Mobile AMD Sempron(tm)"), 0, 0xF, 0x1c, 0x0, CPU_SONORA, CPU_ST_DH8_D0, CPU_S_754, CPU_F_90, -1}, // Note: order important

		{_T("Sempron(tm) Processor 3"), 0, 0xF, 0x1c, 0x0, CPU_PALERMO, CPU_ST_DH8_D0, CPU_S_754, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:1F
		/*
		AuthenticAMD	 0xf	 0x1f	 0x0	 AMD Athlon(tm) 64 Processor 3000+
		AuthenticAMD	 0xf	 0x1f	 0x0	 AMD Athlon(tm) 64 Processor 3200+
		AuthenticAMD	 0xf	 0x1f	 0x0	 AMD Athlon(tm) 64 Processor 3500+
		AuthenticAMD	 0xf	 0x1f	 0x0	 AMD Sempron(tm) Processor 3000+
		AuthenticAMD	 0xf	 0x1f	 0x0	 AMD Sempron(tm) Processor 3200+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 Processor 3"), 0, 0xF, 0x1f, 0x0, CPU_WINCHESTER, CPU_ST_DH8_D0, CPU_S_939, CPU_F_90, -1},
		{_T("Sempron(tm) Processor 3"), 0, 0xF, 0x1f, 0x0, CPU_WINCHESTER, CPU_ST_DH8_D0, CPU_S_939, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:21
		/*
		AuthenticAMD	 0xf	 0x21	 0x0	 Dual Core AMD Opteron(tm) Processor 870
		AuthenticAMD	 0xf	 0x21	 0x2	 Dual Core AMD Opteron(tm) Processor 265
		AuthenticAMD	 0xf	 0x21	 0x2	 Dual Core AMD Opteron(tm) Processor 270
		AuthenticAMD	 0xf	 0x21	 0x2	 Dual Core AMD Opteron(tm) Processor 275
		AuthenticAMD	 0xf	 0x21	 0x2	 Dual Core AMD Opteron(tm) Processor 280
		AuthenticAMD	 0xf	 0x21	 0x2	 Dual Core AMD Opteron(tm) Processor 285
		AuthenticAMD	 0xf	 0x21	 0x2	 Dual Core AMD Opteron(tm) Processor 285 SE
		AuthenticAMD	 0xf	 0x21	 0x2	 Dual Core AMD Opteron(tm) Processor 290
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Dual Core AMD Opteron(tm)"), 0, 0xF, 0x21, 2, CPU_ITALY, CPU_ST_JH9_E6, CPU_S_949, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:23
		/*
		AuthenticAMD	 0xf	 0x23	 0x2	 AMD Athlon(tm) 64 FX-60 Dual Core Processor
		AuthenticAMD	 0xf	 0x23	 0x2	 AMD Athlon(tm) 64 X2 Dual Core 4400+
		AuthenticAMD	 0xf	 0x23	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 3800+
		AuthenticAMD	 0xf	 0x23	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 4200+
		AuthenticAMD	 0xf	 0x23	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 4400+
		AuthenticAMD	 0xf	 0x23	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 4600+
		AuthenticAMD	 0xf	 0x23	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 4800+
		AuthenticAMD	 0xf	 0x23	 0x2	 AMD Athlon(tm) 64 X2 Processor 4600+
		AuthenticAMD	 0xf	 0x23	 0x2	 AMD Athlon(tm)64 X2 Dual Core Processor 3800+
		AuthenticAMD	 0xf	 0x23	 0x2	 AMD Athlon(tm)64 X2 Dual Core Processor 4200+
		AuthenticAMD	 0xf	 0x23	 0x2	 AMD Athlon(tm)64 X2 Dual Core Processor 4400+
		AuthenticAMD	 0xf	 0x23	 0x2	 AMD Hammer Family processor - Model Unknown
		AuthenticAMD	 0xf	 0x23	 0x2	 AMD Processor Model Unknown
		AuthenticAMD	 0xf	 0x23	 0x2	 Dual Core AMD Opteron(tm) Processor 165
		AuthenticAMD	 0xf	 0x23	 0x2	 Dual Core AMD Opteron(tm) Processor 170
		AuthenticAMD	 0xf	 0x23	 0x2	 Dual Core AMD Opteron(tm) Processor 175
		AuthenticAMD	 0xf	 0x23	 0x2	 Dual Core AMD Opteron(tm) Processor 180
		AuthenticAMD	 0xf	 0x23	 0x2	 Dual Core AMD Opteron(tm) Processor 185
		AuthenticAMD	 0xf	 0x23	 0x2	 Dual Core AMD Opteron(tm) Processor 280
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 FX-"), 0, 0xF, 0x23, 2, CPU_TOLEDO, CPU_ST_JH_E6, CPU_S_939, CPU_F_90, -1},
		{_T("Athlon(tm) 64 X2"), 0, 0xF, 0x23, 2, CPU_TOLEDO, CPU_ST_JH_E6, CPU_S_939, CPU_F_90, -1},
		{_T("Athlon(tm)64 X2"), 0, 0xF, 0x23, 2, CPU_TOLEDO, CPU_ST_JH_E6, CPU_S_939, CPU_F_90, -1},

		{_T("Dual Core AMD Opteron(tm)"), 0, 0xF, 0x23, 2, CPU_DENMARK, CPU_ST_JH9_E6, CPU_S_939, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:24
		/*
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile ML-28
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile ML-30
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile ML-32
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile ML-34
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile ML-37
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile ML-40
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile Technology ML-28
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile Technology ML-30
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile Technology ML-32
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile Technology ML-34
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile Technology ML-37
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile Technology ML-40
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile Technology ML-42
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile Technology ML-44
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile Technology MT-28
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile Technology MT-30
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile Technology MT-32
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile Technology MT-34
		AuthenticAMD	 0xf	 0x24	 0x2	 AMD Turion(tm) 64 Mobile Technology MT-37
		AuthenticAMD	 0xf	 0x24	 0x2	 Mobile AMD Athlon(tm) 64 Processor 3200+
		AuthenticAMD	 0xf	 0x24	 0x2	 Mobile AMD Athlon(tm) 64 Processor 3400+
		AuthenticAMD	 0xf	 0x24	 0x2	 Mobile AMD Athlon(tm) 64 Processor 3700+
		AuthenticAMD	 0xf	 0x24	 0x2	 Mobile AMD Athlon(tm) 64 Processor 4000+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Turion(tm) 64 Mobile"), 0, 0xF, 0x24, 0x2, CPU_LANCASTER, CPU_ST_SH8_E5, CPU_S_754, CPU_F_90, -1},

		{_T("Mobile AMD Athlon(tm) 64"), 0, 0xF, 0x24, 0x2, CPU_NEWARK, CPU_ST_SH8_E5, CPU_S_754, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:25
		/*
		AuthenticAMD	 0xf	 0x25	 0x1	 AMD Opteron(tm) Processor 246
		AuthenticAMD	 0xf	 0x25	 0x1	 AMD Opteron(tm) Processor 252
		AuthenticAMD	 0xf	 0x25	 0x1	 AMD Opteron(tm) Processor 254
		AuthenticAMD	 0xf	 0x25	 0x1	 AMD Opteron(tm) Processor 256
		AuthenticAMD	 0xf	 0x25	 0x1	 AMD Opteron(tm) Processor 852
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Opteron(tm) Processor"), 0, 0xF, 0x25, 1, CPU_TROY, CPU_ST_E4, CPU_S_940, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:27
		/*
		AuthenticAMD	 0xf	 0x27	 0x1	 AMD Athlon(tm) 64 FX-55 Processor
		AuthenticAMD	 0xf	 0x27	 0x1	 AMD Athlon(tm) 64 FX-57 Processor
		AuthenticAMD	 0xf	 0x27	 0x1	 AMD Athlon(tm) 64 Processor 3200+
		AuthenticAMD	 0xf	 0x27	 0x1	 AMD Athlon(tm) 64 Processor 3500+
		AuthenticAMD	 0xf	 0x27	 0x1	 AMD Athlon(tm) 64 Processor 3700+
		AuthenticAMD	 0xf	 0x27	 0x1	 AMD Athlon(tm) 64 Processor 4000+
		AuthenticAMD	 0xf	 0x27	 0x1	 AMD Opteron(tm) Processor 144
		AuthenticAMD	 0xf	 0x27	 0x1	 AMD Opteron(tm) Processor 146
		AuthenticAMD	 0xf	 0x27	 0x1	 AMD Opteron(tm) Processor 148
		AuthenticAMD	 0xf	 0x27	 0x1	 AMD Opteron(tm) Processor 150
		AuthenticAMD	 0xf	 0x27	 0x1	 AMD Opteron(tm) Processor 152
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 Processor"), 0, 0xF, 0x27, 1, CPU_SAN_DIEGO, CPU_ST_SH_E4, CPU_S_939, CPU_F_90, -1},
		{_T("Athlon(tm) 64 Processor"), 0, 0xF, 0x27, -1, CPU_SAN_DIEGO, CPU_ST_BLANK, CPU_S_939, CPU_F_90, -1},
		{_T("Athlon(tm) 64 FX-"), 0, 0xF, 0x27, 1, CPU_SAN_DIEGO, CPU_ST_SH8_E4, CPU_S_939, CPU_F_90, -1},
		{_T("Athlon(tm) 64 FX-"), 0, 0xF, 0x27, -1, CPU_SAN_DIEGO, CPU_ST_BLANK, CPU_S_939, CPU_F_90, -1},

		{_T("Opteron Processor"), 0, 0xF, 0x27, 0x1, CPU_VENUS, CPU_ST_E4, CPU_S_939, CPU_F_90, -1},
		{_T("Opteron Processor"), 0, 0xF, 0x27, -1, CPU_VENUS, CPU_ST_BLANK, CPU_S_939, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:2b
		/*
		AuthenticAMD	 0xf	 0x2b	 0x1	 AMD Athlon(tm) 64 X2 Dual Core 4200+
		AuthenticAMD	 0xf	 0x2b	 0x1	 AMD Athlon(tm) 64 X2 Dual Core Processor 3600+
		AuthenticAMD	 0xf	 0x2b	 0x1	 AMD Athlon(tm) 64 X2 Dual Core Processor 3800+
		AuthenticAMD	 0xf	 0x2b	 0x1	 AMD Athlon(tm) 64 X2 Dual Core Processor 4200+
		AuthenticAMD	 0xf	 0x2b	 0x1	 AMD Athlon(tm) 64 X2 Dual Core Processor 4600+
		AuthenticAMD	 0xf	 0x2b	 0x1	 AMD Athlon(tm) 64 X2 Processor 4200+
		AuthenticAMD	 0xf	 0x2b	 0x1	 AMD Athlon(tm)64 X2 Dual Core Processor 3800+
		AuthenticAMD	 0xf	 0x2b	 0x1	 AMD Athlon(tm)64 X2 Dual Core Processor 4200+
		AuthenticAMD	 0xf	 0x2b	 0x1	 AMD Athlon(tm)64 X2 Dual Core Processor 4600+
		AuthenticAMD	 0xf	 0x2b	 0x1	 AMD Hammer Family processor - Model Unknown
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 X2"), 0, 0xF, 0x2b, 1, CPU_MANCHESTER, CPU_ST_BH_E4, CPU_S_939, CPU_F_90, -1},
		{_T("Athlon(tm)64 X2"), 0, 0xF, 0x2b, 1, CPU_MANCHESTER, CPU_ST_BH_E4, CPU_S_939, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:2c
		/*
		AuthenticAMD	 0xf	 0x2c	 0x2	 AMD Unknown Processor
		AuthenticAMD	 0xf	 0x2c	 0x2	 BOSSKILLER(TM) 64bit Sempron- NBT(R) Engine
		AuthenticAMD	 0xf	 0x2c	 0x2	 Mobile AMD Sempron(tm) 2800+
		AuthenticAMD	 0xf	 0x2c	 0x2	 Mobile AMD Sempron(tm) 3100+
		AuthenticAMD	 0xf	 0x2c	 0x2	 Mobile AMD Sempron(tm) Processor 2800+
		AuthenticAMD	 0xf	 0x2c	 0x2	 Mobile AMD Sempron(tm) Processor 3000+
		AuthenticAMD	 0xf	 0x2c	 0x2	 Mobile AMD Sempron(tm) Processor 3100+
		AuthenticAMD	 0xf	 0x2c	 0x2	 Mobile AMD Sempron(tm) Processor 3300+
		AuthenticAMD	 0xf	 0x2c	 0x0	 AMD Athlon(tm) 64 Processor 3400+
		AuthenticAMD	 0xf	 0x2c	 0x0	 AMD Sempron(tm) Processor 3000+
		AuthenticAMD	 0xf	 0x2c	 0x0	 AMD Sempron(tm) Processor 3100+
		AuthenticAMD	 0xf	 0x2c	 0x0	 AMD Sempron(tm) Processor 3300+
		AuthenticAMD	 0xf	 0x2c	 0x2	 AMD Athlon(tm) 64 Processor 3000+
		AuthenticAMD	 0xf	 0x2c	 0x2	 AMD Athlon(tm) 64 Processor 3200+
		AuthenticAMD	 0xf	 0x2c	 0x2	 AMD Hammer Family processor - Model Unknown
		AuthenticAMD	 0xf	 0x2c	 0x2	 AMD Processor Model Unknown
		AuthenticAMD	 0xf	 0x2c	 0x2	 AMD Sempron(tm) Processor 2500+
		AuthenticAMD	 0xf	 0x2c	 0x2	 AMD Sempron(tm) Processor 2600+
		AuthenticAMD	 0xf	 0x2c	 0x2	 AMD Sempron(tm) Processor 2800+
		AuthenticAMD	 0xf	 0x2c	 0x2	 AMD Sempron(tm) Processor 3000+
		AuthenticAMD	 0xf	 0x2c	 0x2	 AMD Sempron(tm) Processor 3100+
		AuthenticAMD	 0xf	 0x2c	 0x2	 AMD Sempron(tm) Processor 3300+
		AuthenticAMD	 0xf	 0x2c	 0x2	 AMD Sempron(tm) Processor 3400+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Mobile AMD Sempron"), 0, 0xF, 0x2C, 0x2, CPU_ALBANY, CPU_ST_DH_E6, CPU_S_754, CPU_F_90, -1}, // Note: order important
		{_T("Mobile AMD Sempron"), 0, 0xF, 0x2C, -1, CPU_ALBANY, CPU_ST_BLANK, CPU_S_754, CPU_F_90, -1},

		{_T("Athlon(tm) 64 Processor 3"), 0, 0xF, 0x2c, 0x0, CPU_VENICE, CPU_ST_DH_E3, CPU_S_754, CPU_F_90, -1},
		{_T("Athlon(tm) 64 Processor 3"), 0, 0xF, 0x2c, 0x2, CPU_VENICE, CPU_ST_DH_E6, CPU_S_754, CPU_F_90, -1},

		{_T("Sempron(tm) Processor 3"), 0, 0xF, 0x2c, 0x2, CPU_PALERMO, CPU_ST_DH_E6, CPU_S_754, CPU_F_90, -1},
		{_T("Sempron(tm) Processor 3"), 0, 0xF, 0x2c, -1, CPU_PALERMO, CPU_ST_BLANK, CPU_S_754, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:2f
		/*
		AuthenticAMD	 0xf	 0x2f	 0x0	 AMD Athlon(tm) 64 Processor 3000+
		AuthenticAMD	 0xf	 0x2f	 0x0	 AMD Athlon(tm) 64 Processor 3200+
		AuthenticAMD	 0xf	 0x2f	 0x0	 AMD Athlon(tm) 64 Processor 3400+
		AuthenticAMD	 0xf	 0x2f	 0x0	 AMD Athlon(tm) 64 Processor 3500+
		AuthenticAMD	 0xf	 0x2f	 0x0	 AMD Athlon(tm) 64 Processor 3800+
		AuthenticAMD	 0xf	 0x2f	 0x0	 AMD Sempron(tm) Processor 3000+
		AuthenticAMD	 0xf	 0x2f	 0x0	 AMD Sempron(tm) Processor 3200+
		AuthenticAMD	 0xf	 0x2f	 0x2	 AMD Athlon(tm) 64 Processor 3000+
		AuthenticAMD	 0xf	 0x2f	 0x2	 AMD Athlon(tm) 64 Processor 3200+
		AuthenticAMD	 0xf	 0x2f	 0x2	 AMD Athlon(tm) 64 Processor 3400+
		AuthenticAMD	 0xf	 0x2f	 0x2	 AMD Athlon(tm) 64 Processor 3500+
		AuthenticAMD	 0xf	 0x2f	 0x2	 AMD Athlon(tm) 64 Processor 3800+
		AuthenticAMD	 0xf	 0x2f	 0x2	 AMD Sempron(tm) Processor 3000+
		AuthenticAMD	 0xf	 0x2f	 0x2	 AMD Sempron(tm) Processor 3200+
		AuthenticAMD	 0xf	 0x2f	 0x2	 AMD Sempron(tm) Processor 3400+
		AuthenticAMD	 0xf	 0x2f	 0x2	 AMD Sempron(tm) Processor 3500+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 Processor 3"), 0, 0xF, 0x2f, 0x0, CPU_VENICE, CPU_ST_DH_E3, CPU_S_939, CPU_F_90, -1},
		{_T("Athlon(tm) 64 Processor 3"), 0, 0xF, 0x2f, 0x2, CPU_VENICE, CPU_ST_DH_E6, CPU_S_939, CPU_F_90, -1},

		{_T("Sempron(tm) Processor 3"), 0, 0xF, 0x2f, 0x0, CPU_PALERMO, CPU_ST_DH8_E3, CPU_S_939, CPU_F_90, -1},
		{_T("Sempron(tm) Processor 3"), 0, 0xF, 0x2f, 0x2, CPU_PALERMO, CPU_ST_E6, CPU_S_939, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:27
		/*
		AuthenticAMD	 0xf	 0x37	 0x2	 AMD Athlon(tm) 64 Processor 3700+
		AuthenticAMD	 0xf	 0x37	 0x2	 AMD Athlon(tm) 64 Processor 4000+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 Processor"), 0, 0xF, 0x37, 1, CPU_SAN_DIEGO, CPU_ST_SH_E4, CPU_S_939, CPU_F_90, -1},
		{_T("Athlon(tm) 64 Processor"), 0, 0xF, 0x37, -1, CPU_SAN_DIEGO, CPU_ST_BLANK, CPU_S_939, CPU_F_90, -1},
		{_T("Athlon(tm) 64 FX-"), 0, 0xF, 0x37, 1, CPU_SAN_DIEGO, CPU_ST_SH8_E4, CPU_S_939, CPU_F_90, -1},
		{_T("Athlon(tm) 64 FX-"), 0, 0xF, 0x37, -1, CPU_SAN_DIEGO, CPU_ST_BLANK, CPU_S_939, CPU_F_90, -1},

		{_T("AMD Opteron Processor"), 0, 0xF, 0x37, 0x1, CPU_VENUS, CPU_ST_E4, CPU_S_939, CPU_F_90, -1},
		{_T("AMD Opteron Processor"), 0, 0xF, 0x37, -1, CPU_VENUS, CPU_ST_BLANK, CPU_S_939, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:3F
		/*
		AuthenticAMD	 0xf	 0x3f	 0x2	 AMD Athlon(tm) 64 Processor 3200+
		AuthenticAMD	 0xf	 0x3f	 0x2	 AMD Athlon(tm) 64 Processor 3500+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 Processor"), 0, 0xF, 0x3F, -1, CPU_MANCHESTER, CPU_ST_BLANK, CPU_S_939, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:41
		/*
		AuthenticAMD	 0xf	 0x41	 0x2	 Dual-Core AMD Opteron(tm) Processor 2210
		AuthenticAMD	 0xf	 0x41	 0x2	 Dual-Core AMD Opteron(tm) Processor 2212
		AuthenticAMD	 0xf	 0x41	 0x2	 Dual-Core AMD Opteron(tm) Processor 2214
		AuthenticAMD	 0xf	 0x41	 0x2	 Dual-Core AMD Opteron(tm) Processor 2216
		AuthenticAMD	 0xf	 0x41	 0x2	 Dual-Core AMD Opteron(tm) Processor 2218
		AuthenticAMD	 0xf	 0x41	 0x3	 Dual-Core AMD Opteron(tm) Processor 2216
		AuthenticAMD	 0xf	 0x41	 0x3	 Dual-Core AMD Opteron(tm) Processor 2218
		AuthenticAMD	 0xf	 0x41	 0x3	 Dual-Core AMD Opteron(tm) Processor 2220
		AuthenticAMD	 0xf	 0x41	 0x3	 Dual-Core AMD Opteron(tm) Processor 2222
		AuthenticAMD	 0xf	 0x41	 0x3	 Dual-Core AMD Opteron(tm) Processor 2222 SE
		AuthenticAMD	 0xf	 0x41	 0x3	 Dual-Core AMD Opteron(tm) Processor 8222
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Opteron(tm) Processor 22"), 0, 0xF, 0x41, -1, CPU_SANTA_ROSA, CPU_ST_BLANK, CPU_S_F_1207, CPU_F_90, -1},
		{_T("Opteron(tm) Processor 82"), 0, 0xF, 0x41, -1, CPU_SANTA_ROSA, CPU_ST_BLANK, CPU_S_F_1207, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:43
		/*
		AuthenticAMD	 0xf	 0x43	 0x2	 AMD Athlon(tm) 64 FX-62 Dual Core Processor
		AuthenticAMD	 0xf	 0x43	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 3800+
		AuthenticAMD	 0xf	 0x43	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 4000+
		AuthenticAMD	 0xf	 0x43	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 4400+
		AuthenticAMD	 0xf	 0x43	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 4600+
		AuthenticAMD	 0xf	 0x43	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 4800+
		AuthenticAMD	 0xf	 0x43	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 5000+
		AuthenticAMD	 0xf	 0x43	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 5200+
		AuthenticAMD	 0xf	 0x43	 0x2	 Dual-Core AMD Opteron(tm) Processor 1210
		AuthenticAMD	 0xf	 0x43	 0x2	 Dual-Core AMD Opteron(tm) Processor 1216
		AuthenticAMD	 0xf	 0x43	 0x3	 AMD Athlon(tm) 64 X2 Dual Core Processor 3800+
		AuthenticAMD	 0xf	 0x43	 0x3	 AMD Athlon(tm) 64 X2 Dual Core Processor 4600+
		AuthenticAMD	 0xf	 0x43	 0x3	 AMD Athlon(tm) 64 X2 Dual Core Processor 5000+
		AuthenticAMD	 0xf	 0x43	 0x3	 AMD Athlon(tm) 64 X2 Dual Core Processor 5200+
		AuthenticAMD	 0xf	 0x43	 0x3	 AMD Athlon(tm) 64 X2 Dual Core Processor 5400+
		AuthenticAMD	 0xf	 0x43	 0x3	 AMD Athlon(tm) 64 X2 Dual Core Processor 5600+
		AuthenticAMD	 0xf	 0x43	 0x3	 AMD Athlon(tm) 64 X2 Dual Core Processor 6000+
		AuthenticAMD	 0xf	 0x43	 0x3	 AMD Athlon(tm) 64 X2 Dual Core Processor 6400+
		AuthenticAMD	 0xf	 0x43	 0x3	 AMD Processor model unknown
		AuthenticAMD	 0xf	 0x43	 0x3	 Dual-Core AMD Opteron(tm) Processor 1212
		AuthenticAMD	 0xf	 0x43	 0x3	 Dual-Core AMD Opteron(tm) Processor 1216
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 FX-62 Dual Core"), 0, 0xF, 0x43, -1, CPU_WINDSOR, CPU_ST_BLANK, CPU_S_AM2, CPU_F_90, -1},
		{_T("Athlon(tm) 64 X2 Dual Core"), 0, 0xF, 0x43, -1, CPU_WINDSOR, CPU_ST_BLANK, CPU_S_AM2, CPU_F_90, -1},

		{_T("Opteron(tm) Processor 12"), 0, 0xF, 0x43, -1, CPU_SANTA_ANA, CPU_ST_BLANK, CPU_S_AM2, CPU_F_90, -1}, // Note: 13xx, 1= 1-way, 3=Quad core

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:48
		/*
		AuthenticAMD	 0xf	 0x48	 0x2	 AMD Athlon(tm) 64 X2 Dual-Core Processor TK-55
		AuthenticAMD	 0xf	 0x48	 0x2	 AMD Turion(tm) 64 X2
		AuthenticAMD	 0xf	 0x48	 0x2	 AMD Turion(tm) 64 X2 Mobile Technology TL-50
		AuthenticAMD	 0xf	 0x48	 0x2	 AMD Turion(tm) 64 X2 Mobile Technology TL-52
		AuthenticAMD	 0xf	 0x48	 0x2	 AMD Turion(tm) 64 X2 Mobile Technology TL-56
		AuthenticAMD	 0xf	 0x48	 0x2	 AMD Turion(tm) 64 X2 Mobile Technology TL-60
		AuthenticAMD	 0xf	 0x48	 0x2	 AMD Turion(tm) 64 X2 Mobile Technology TL-64
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("AMD Athlon(tm) 64 X2"), 0, 0xF, 0x48, 0x2, CPU_BLANK, CPU_ST_BH_F2, CPU_S_S1_638, CPU_F_90, -1}, // Trinidad or Taylor

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:4b
		/*
		AuthenticAMD	 0xf	 0x4b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 3600+
		AuthenticAMD	 0xf	 0x4b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 3800+
		AuthenticAMD	 0xf	 0x4b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 4200+
		AuthenticAMD	 0xf	 0x4b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 4600+
		AuthenticAMD	 0xf	 0x4b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 5000+
		AuthenticAMD	 0xf	 0x4b	 0x2	 AMD Athlon(tm) 64 X2 Processor 4200+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 X2 Dual Core"), 0, 0xF, 0x4b, 2, CPU_WINDSOR, CPU_ST_BH_F2, CPU_S_AM2, CPU_F_90, -1},
		{_T("Athlon(tm) 64 X2 Dual Core"), 0, 0xF, 0x4b, -1, CPU_WINDSOR, CPU_ST_BLANK, CPU_S_AM2, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:4c
		/*
		AuthenticAMD	 0xf	 0x4c	 0x2	 AMD Turion(tm) 64
		AuthenticAMD	 0xf	 0x4c	 0x2	 AMD Turion(tm) 64 Mobile Technology MK-36
		AuthenticAMD	 0xf	 0x4c	 0x2	 AMD Turion(tm) 64 Mobile Technology MK-38
		AuthenticAMD	 0xf	 0x4c	 0x2	 Mobile AMD Sempron(tm)
		AuthenticAMD	 0xf	 0x4c	 0x2	 Mobile AMD Sempron(tm) 3600+
		AuthenticAMD	 0xf	 0x4c	 0x2	 Mobile AMD Sempron(tm) Processor 3200+
		AuthenticAMD	 0xf	 0x4c	 0x2	 Mobile AMD Sempron(tm) Processor 3400+
		AuthenticAMD	 0xf	 0x4c	 0x2	 Mobile AMD Sempron(tm) Processor 3500+
		AuthenticAMD	 0xf	 0x4c	 0x2	 Mobile AMD Sempron(tm) Processor 3600+
		AuthenticAMD	 0xf	 0x4c	 0x2	 Mobile AMD Sempron(tm) Processor 3800+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Turion 64 Mobile Technology MK-"), 0, 0xF, 0x4C, 2, CPU_RICHMOND, CPU_ST_DH_F2, CPU_S_S1, CPU_F_90, -1},
		{_T("Mobile AMD Sempron(tm)"), 0, 0xF, 0x4C, 2, CPU_KEENE, CPU_ST_DH_F2, CPU_S_S1, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:4f
		/*
		AuthenticAMD	 0xf	 0x4f	 0x2	 AMD Athlon(tm) 64 Processor 3000+
		AuthenticAMD	 0xf	 0x4f	 0x2	 AMD Athlon(tm) 64 Processor 3200+
		AuthenticAMD	 0xf	 0x4f	 0x2	 AMD Athlon(tm) 64 Processor 3500+
		AuthenticAMD	 0xf	 0x4f	 0x2	 AMD Athlon(tm) 64 Processor 3800+
		AuthenticAMD	 0xf	 0x4f	 0x2	 AMD Sempron(tm) Processor 2800+
		AuthenticAMD	 0xf	 0x4f	 0x2	 AMD Sempron(tm) Processor 3000+
		AuthenticAMD	 0xf	 0x4f	 0x2	 AMD Sempron(tm) Processor 3200+
		AuthenticAMD	 0xf	 0x4f	 0x2	 AMD Sempron(tm) Processor 3400+
		AuthenticAMD	 0xf	 0x4f	 0x2	 AMD Sempron(tm) Processor 3500+
		AuthenticAMD	 0xf	 0x4f	 0x2	 AMD Sempron(tm) Processor 3600+
		AuthenticAMD	 0xf	 0x4f	 0x2	 AMD Sempron(tm) Processor 3800+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 Processor 3"), 0, 0xF, 0x4f, 0x2, CPU_ORLEANS, CPU_ST_DH_F2, CPU_S_AM2, CPU_F_90, -1},
		{_T("Athlon(tm) 64 Processor 3"), 0, 0xF, 0x4f, -1, CPU_ORLEANS, CPU_ST_BLANK, CPU_S_AM2, CPU_F_90, -1},

		{_T("Sempron(tm) Processor"), 0, 0xF, 0x4f, 0x2, CPU_MANILA, CPU_ST_DH_F2, CPU_S_AM2, CPU_F_90, -1},
		{_T("Sempron(tm) Processor"), 0, 0xF, 0x4f, -1, CPU_MANILA, CPU_ST_BLANK, CPU_S_AM2, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:5f
		/*
		AuthenticAMD	 0xf	 0x5f	 0x2	 AMD Athlon(tm) 64 Processor 3000+
		AuthenticAMD	 0xf	 0x5f	 0x2	 AMD Athlon(tm) 64 Processor 3200+
		AuthenticAMD	 0xf	 0x5f	 0x2	 AMD Athlon(tm) 64 Processor 3500+
		AuthenticAMD	 0xf	 0x5f	 0x2	 AMD Athlon(tm) 64 Processor 3800+
		AuthenticAMD	 0xf	 0x5f	 0x2	 AMD Sempron(tm) Processor 3200+
		AuthenticAMD	 0xf	 0x5f	 0x2	 AMD Sempron(tm) Processor 3400+
		AuthenticAMD	 0xf	 0x5f	 0x2	 AMD Sempron(tm) Processor 3600+
		AuthenticAMD	 0xf	 0x5f	 0x3	 AMD Athlon(tm) 64 Processor 3500+
		AuthenticAMD	 0xf	 0x5f	 0x3	 AMD Athlon(tm) 64 Processor 3800+
		AuthenticAMD	 0xf	 0x5f	 0x3	 AMD Athlon(tm) 64 Processor 4000+
		AuthenticAMD	 0xf	 0x5f	 0x3	 AMD Athlon(tm) Processor LE-1600
		AuthenticAMD	 0xf	 0x5f	 0x3	 AMD Athlon(tm) Processor LE-1620
		AuthenticAMD	 0xf	 0x5f	 0x3	 AMD Athlon(tm) Processor LE-1640
		AuthenticAMD	 0xf	 0x5f	 0x3	 AMD Processor model unknown
		AuthenticAMD	 0xf	 0x5f	 0x3	 AMD Sempron(tm) Processor LE-1600
		AuthenticAMD	 0xf	 0x5f	 0x3	 AMD Sempron(tm) Processor LE-1620
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 Processor 3"), 0, 0xF, 0x5f, 0x2, CPU_ORLEANS, CPU_ST_DH_F2, CPU_S_AM2, CPU_F_90, -1},
		{_T("Athlon(tm) 64 Processor 3"), 0, 0xF, 0x5f, -1, CPU_ORLEANS, CPU_ST_BLANK, CPU_S_AM2, CPU_F_90, -1},
		{_T("Athlon(tm) Processor LE-1600"), 0, 0xF, 0x5f, -1, CPU_ORLEANS, CPU_ST_BLANK, CPU_S_AM2, CPU_F_90, -1},
		{_T("Athlon(tm) Processor LE-1620"), 0, 0xF, 0x5f, -1, CPU_ORLEANS, CPU_ST_BLANK, CPU_S_AM2, CPU_F_90, -1},
		{_T("Athlon(tm) Processor LE-1640"), 0, 0xF, 0x5f, -1, CPU_ORLEANS, CPU_ST_BLANK, CPU_S_AM2, CPU_F_90, -1},

		{_T("Sempron(tm) Processor 3"), 0, 0xF, 0x5f, 0x2, CPU_MANILA, CPU_ST_DH_F2, CPU_S_AM2, CPU_F_90, -1},
		{_T("Sempron(tm) Processor 3"), 0, 0xF, 0x5f, -1, CPU_MANILA, CPU_ST_BLANK, CPU_S_AM2, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:68
		/*
		AuthenticAMD	 0xf	 0x68	 0x1	 AMD Athlon(tm) 64 X2 Dual Core Processor TK-53
		AuthenticAMD	 0xf	 0x68	 0x1	 AMD Athlon(tm) 64 X2 Dual-Core Processor TK-53
		AuthenticAMD	 0xf	 0x68	 0x1	 AMD Athlon(tm) 64 X2 Dual-Core Processor TK-55
		AuthenticAMD	 0xf	 0x68	 0x1	 AMD Processor model unknown
		AuthenticAMD	 0xf	 0x68	 0x1	 AMD Turion(tm) 64 X2
		AuthenticAMD	 0xf	 0x68	 0x1	 AMD Turion(tm) 64 X2 Mobile Technology TL-56
		AuthenticAMD	 0xf	 0x68	 0x1	 AMD Turion(tm) 64 X2 Mobile Technology TL-58
		AuthenticAMD	 0xf	 0x68	 0x1	 AMD Turion(tm) 64 X2 Mobile Technology TL-60
		AuthenticAMD	 0xf	 0x68	 0x1	 AMD Turion(tm) 64 X2 Mobile Technology TL-64
		AuthenticAMD	 0xf	 0x68	 0x1	 AMD Turion(tm) 64 X2 Mobile Technology TL-66
		AuthenticAMD	 0xf	 0x68	 0x1	 AMD Turion(tm) 64 X2 TL-58
		AuthenticAMD	 0xf	 0x68	 0x2	 AMD Athlon(tm) 64 X2 Dual-Core Processor TK-42
		AuthenticAMD	 0xf	 0x68	 0x2	 AMD Athlon(tm) 64 X2 Dual-Core Processor TK-57
		AuthenticAMD	 0xf	 0x68	 0x2	 AMD Turion(tm) 64 X2 Mobile Technology TL-58
		AuthenticAMD	 0xf	 0x68	 0x2	 AMD Turion(tm) 64 X2 Mobile Technology TL-60
		AuthenticAMD	 0xf	 0x68	 0x2	 AMD Turion(tm) 64 X2 Mobile Technology TL-62
		AuthenticAMD	 0xf	 0x68	 0x2	 AMD Turion(tm) 64 X2 Mobile Technology TL-64
		AuthenticAMD	 0xf	 0x68	 0x2	 AMD Turion(tm) 64 X2 Mobile Technology TL-66
		AuthenticAMD	 0xf	 0x68	 0x2	 AMD Turion(tm) 64 X2 Mobile Technology TL-68
		AuthenticAMD	 0xf	 0x68	 0x2	 AMD Turion(tm) 64 X2 TL-58
		AuthenticAMD	 0xf	 0x68	 0x2	 AMD Turion(tm) 64 X2 TL-60
		AuthenticAMD	 0xf	 0x68	 0x2	 AMD Turion(tm) 64 X2 TL-62
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("64 X2 Dual Core Processor TK"), 0, 0xF, 0x68, -1, CPU_TYLER, CPU_ST_BLANK, CPU_S_S1_638, CPU_F_65, -1},
		{_T("64 X2 Mobile Technology TL-"), 0, 0xF, 0x68, -1, CPU_TYLER, CPU_ST_BLANK, CPU_S_S1_638, CPU_F_65, -1},
		{_T("AMD Turion(tm) 64 X2 TL-"), 0, 0xF, 0x68, -1, CPU_TYLER, CPU_ST_BLANK, CPU_S_S1_638, CPU_F_65, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:6b
		/*
		AuthenticAMD	 0xf	 0x6b	 0x1	 AMD Athlon(tm) 64 X2 Dual Core Processor 3600+
		AuthenticAMD	 0xf	 0x6b	 0x1	 AMD Athlon(tm) 64 X2 Dual Core Processor 4000+
		AuthenticAMD	 0xf	 0x6b	 0x1	 AMD Athlon(tm) 64 X2 Dual Core Processor 4200+
		AuthenticAMD	 0xf	 0x6b	 0x1	 AMD Athlon(tm) 64 X2 Dual Core Processor 4400+
		AuthenticAMD	 0xf	 0x6b	 0x1	 AMD Athlon(tm) 64 X2 Dual Core Processor 4800+
		AuthenticAMD	 0xf	 0x6b	 0x1	 AMD Athlon(tm) 64 X2 Dual Core Processor 5000+
		AuthenticAMD	 0xf	 0x6b	 0x1	 AMD Athlon(tm) 64 X2 Dual Core Processor BE-2350
		AuthenticAMD	 0xf	 0x6b	 0x1	 AMD Athlon(tm) X2 Dual Core Processor BE-2300
		AuthenticAMD	 0xf	 0x6b	 0x1	 AMD Athlon(tm) X2 Dual Core Processor BE-2350
		AuthenticAMD	 0xf	 0x6b	 0x1	 AMD Processor model unknown
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core CPU 5000+
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 4200+
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 4400+
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 4600+
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 4800+
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 5000+
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 5200+
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 5400+
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 5600+
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 5800+
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) 64 X2 Dual Core Processor 6000+
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) Dual Core Processor 4050e
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) Dual Core Processor 4450B
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) Dual Core Processor 4450e
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) Dual Core Processor 4850e
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) Dual Core Processor 5000B
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) Dual Core Processor 5050e
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) Dual Core Processor 5200B
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) Dual Core Processor 5400B
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) Neo X2 Dual Core Processor L335
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) X2 Dual Core Processor BE-2300
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) X2 Dual Core Processor BE-2350
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) X2 Dual Core Processor BE-2400
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Athlon(tm) X2 Dual Core Processor BE-2450
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Processor model unknown
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Sempron(tm) Dual Core Processor 2100
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Sempron(tm) Dual Core Processor 2200
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Sempron(tm) Dual Core Processor 4700
		AuthenticAMD	 0xf	 0x6b	 0x2	 AMD Turion(tm) Neo X2 Dual Core Processor L625
		AuthenticAMD	 0xf	 0x6b	 0x2	 Athlon 64 Dual Core 5000+
		AuthenticAMD	 0xf	 0x6b	 0x2	 Athlon(tm) Dual Core Processor 4850e
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("X2 Dual Core Processor BE-2"), 0, 0xF, 0x6B, -1, CPU_BRISBANE, CPU_ST_BLANK, CPU_S_AM2_940_PIN, CPU_F_65, -1},		 // Athlon
		{_T("Athlon(tm) Dual Core Processor 4"), 0, 0xF, 0x6B, -1, CPU_BRISBANE, CPU_ST_BLANK, CPU_S_AM2_940_PIN, CPU_F_65, -1}, // Athlon
		{_T("Athlon(tm) Dual Core Processor 5"), 0, 0xF, 0x6B, -1, CPU_BRISBANE, CPU_ST_BLANK, CPU_S_AM2_940_PIN, CPU_F_65, -1}, // Athlon
		{_T("Athlon(tm) 64 X2 Dual Core"), 0, 0xF, 0x6B, 2, CPU_BRISBANE, CPU_ST_BH_G2, CPU_S_AM2_940_PIN, CPU_F_65, -1},
		{_T("Athlon(tm) 64 X2 Dual Core"), 0, 0xF, 0x6B, -1, CPU_BRISBANE, CPU_ST_BLANK, CPU_S_AM2_940_PIN, CPU_F_65, -1},

		{_T("Sempron(tm) Dual Core Processor 2"), 0, 0xF, 0x6B, 0x2, CPU_SHERMAN, CPU_ST_BH_G2, CPU_S_S1, CPU_F_65, -1}, // Sempron
		{_T("Sempron(tm) Dual Core Processor 4"), 0, 0xF, 0x6B, 0x2, CPU_SHERMAN, CPU_ST_BH_G2, CPU_S_S1, CPU_F_65, -1}, // Sempron

		{_T("Neo X2 Dual Core Processor L335"), 0, 0xF, 0x6B, -1, CPU_BLANK, CPU_ST_BLANK, CPU_S_ASB1, CPU_F_65, -1}, // Athlon
		{_T("Neo X2 Dual Core Processor L625"), 0, 0xF, 0x6B, -1, CPU_BLANK, CPU_ST_BLANK, CPU_S_ASB1, CPU_F_65, -1}, // Turion

		{_T("Sempron(tm) Processor 200U"), 0, 0xF, 0x6B, -1, CPU_HURON, CPU_ST_BLANK, CPU_S_ASB1, CPU_F_65, -1},
		{_T("Sempron(tm) Processor 210U"), 0, 0xF, 0x6B, -1, CPU_HURON, CPU_ST_BLANK, CPU_S_ASB1, CPU_F_65, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:6b
		/*
		AuthenticAMD	 0xf	 0x6f	 0x2	 AMD Sempron(tm) Processor 210U
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Sempron(tm) Processor 210U"), 0, 0xF, 0x6F, -1, CPU_HURON, CPU_ST_BLANK, CPU_S_ASB1, CPU_F_65, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:7c
		/*
		AuthenticAMD	 0xf	 0x7c	 0x2	 AMD Athlon(tm) Processor L110				//For a single specific manufacturer - no details
		AuthenticAMD	 0xf	 0x7c	 0x2	 AMD Athlon(tm) Processor TF-20
		AuthenticAMD	 0xf	 0x7c	 0x2	 Mobile AMD Sempron(tm) Processor 3600+
		AuthenticAMD	 0xf	 0x7c	 0x2	 Mobile AMD Sempron(tm) Processor 3800+
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) Processor TF-20"), 0, 0xF, 0x7C, -1, CPU_HURON, CPU_ST_BLANK, CPU_S_ASB1, CPU_F_65, -1},

		{_T("Mobile AMD Sempron(tm)"), 0, 0xF, 0x7C, -1, CPU_TYLER, CPU_ST_BLANK, CPU_S_S1, CPU_F_65, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:7f
		/*
		AuthenticAMD	 0xf	 0x7f	 0x1	 AMD Athlon(tm) 64 Processor 3500+
		AuthenticAMD	 0xf	 0x7f	 0x1	 AMD Athlon(tm) 64 Processor 3800+
		AuthenticAMD	 0xf	 0x7f	 0x1	 AMD Processor model unknown
		AuthenticAMD	 0xf	 0x7f	 0x1	 AMD Sempron(tm) Processor LE-1100
		AuthenticAMD	 0xf	 0x7f	 0x1	 AMD Sempron(tm) Processor LE-1150
		AuthenticAMD	 0xf	 0x7f	 0x1	 AMD Sempron(tm) Processor LE-1200
		AuthenticAMD	 0xf	 0x7f	 0x2	 AMD Athlon(tm) Neo Processor MV-40
		AuthenticAMD	 0xf	 0x7f	 0x2	 AMD Athlon(tm) Processor 1640B
		AuthenticAMD	 0xf	 0x7f	 0x2	 AMD Athlon(tm) Processor 2650e
		AuthenticAMD	 0xf	 0x7f	 0x2	 AMD Athlon(tm) Processor LE-1640
		AuthenticAMD	 0xf	 0x7f	 0x2	 AMD Athlon(tm) Processor LE-1660
		AuthenticAMD	 0xf	 0x7f	 0x2	 AMD Sempron(tm) Processor LE-1250
		AuthenticAMD	 0xf	 0x7f	 0x2	 AMD Sempron(tm) Processor LE-1300
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("AMD Athlon(tm) 64 Processor"), 0, 0xF, 0x7F, -1, CPU_LIMA, CPU_ST_BLANK, CPU_S_AM2, CPU_F_65, -1},
		{_T("Athlon(tm) Processor 1640B"), 0, 0xF, 0x7F, -1, CPU_LIMA, CPU_ST_BLANK, CPU_S_AM2, CPU_F_65, -1},
		{_T("Athlon(tm) Processor 2650e"), 0, 0xF, 0x7F, -1, CPU_LIMA, CPU_ST_BLANK, CPU_S_AM2, CPU_F_65, -1},
		{_T("Athlon(tm) Processor LE-16"), 0, 0xF, 0x7F, -1, CPU_LIMA, CPU_ST_BLANK, CPU_S_AM2, CPU_F_65, -1},

		{_T("Sempron(tm) Processor LE-1"), 0, 0xF, 0x7F, -1, CPU_SPARTA, CPU_ST_BLANK, CPU_S_AM2, CPU_F_65, -1},

		{_T("Athlon(tm) Neo Processor MV-40"), 0, 0xF, 0x7F, -1, CPU_HURON, CPU_ST_BLANK, CPU_S_ASB1, CPU_F_65, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD F:C1
		/*
		AuthenticAMD	 0xf	 0xc1	 0x3	 AMD Athlon(tm) 64 FX-70 Processor
		AuthenticAMD	 0xf	 0xc1	 0x3	 AMD Athlon(tm) 64 FX-72 Processor
		AuthenticAMD	 0xf	 0xc1	 0x3	 AMD Athlon(tm) 64 FX-74 Processor
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Athlon(tm) 64 FX-70 Processor"), 0, 0xF, 0xc1, 0x3, CPU_WINDSOR, CPU_ST_JH_F3, CPU_S_F_1207, CPU_F_90, -1},
		{_T("Athlon(tm) 64 FX-72 Processor"), 0, 0xF, 0xc1, 0x3, CPU_WINDSOR, CPU_ST_JH_F3, CPU_S_F_1207, CPU_F_90, -1},
		{_T("Athlon(tm) 64 FX-74 Processor"), 0, 0xF, 0xc1, 0x3, CPU_WINDSOR, CPU_ST_JH_F3, CPU_S_F_1207, CPU_F_90, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 10:2
		/*
		AuthenticAMD	 0x10	 0x2	 0x2	 AMD Phenom(tm) 8400 Triple-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x2	 AMD Phenom(tm) 8600 Triple-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x2	 AMD Phenom(tm) 9100e Quad-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x2	 AMD Phenom(tm) 9500 Quad-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x2	 AMD Phenom(tm) 9600 Quad-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x2	 AMD Phenom(tm) X4 Quad-Core Processor GP-9500
		AuthenticAMD	 0x10	 0x2	 0x2	 AMD Phenom(tm) X4 Quad-Core Processor GP-9600
		AuthenticAMD	 0x10	 0x2	 0x2	 Quad-Core AMD Opteron(tm) Processor 2352
		AuthenticAMD	 0x10	 0x2	 0x2	 Quad-Core AMD Opteron(tm) Processor 2354
		AuthenticAMD	 0x10	 0x2	 0x2	 Quad-Core AMD Opteron(tm) Processor 2356
		AuthenticAMD	 0x10	 0x2	 0x2	 Quad-Core AMD Opteron(tm) Processor 8346 HE
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Athlon(tm) 7450 Dual-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Athlon(tm) 7550 Dual-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Athlon(tm) 7750 Dual-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Athlon(tm) 7850 Dual-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 7950 Quad-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 8250e Triple-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 8450 Triple-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 8550 Triple-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 8600B Triple-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 8650 Triple-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 8750 Triple-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 8750B Triple-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 9150e Quad-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 9350e Quad-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 9550 Quad-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 9600B Quad-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 9650 Quad-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 9750 Quad-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 9850 Quad-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) 9950 Quad-Core Processor
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) X2 Dual-Core Processor GP-7730
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Phenom(tm) X4 Quad-Core Processor GP-9830
		AuthenticAMD	 0x10	 0x2	 0x3	 AMD Processor model unknown
		AuthenticAMD	 0x10	 0x2	 0x3	 Quad-Core AMD Opteron(tm) Processor 1352
		AuthenticAMD	 0x10	 0x2	 0x3	 Quad-Core AMD Opteron(tm) Processor 1354
		AuthenticAMD	 0x10	 0x2	 0x3	 Quad-Core AMD Opteron(tm) Processor 1356
		AuthenticAMD	 0x10	 0x2	 0x3	 Quad-Core AMD Opteron(tm) Processor 2350
		AuthenticAMD	 0x10	 0x2	 0x3	 Quad-Core AMD Opteron(tm) Processor 2354
		AuthenticAMD	 0x10	 0x2	 0x3	 Quad-Core AMD Opteron(tm) Processor 8356
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		//	Budapest: Opteron Quad Core K10 - 65nm (3rd generation) Quad core
		{_T("Opteron(tm) Processor 13"), 0, 0x10, 2, -1, CPU_BUDAPEST, CPU_ST_BLANK, CPU_S_AM2, CPU_F_65, -1}, // Note: 13xx, 1= 1-way, 3=Quad core

		//	Barcelona: Opteron Quad Core K10 - 65nm (3rd generation) Quad core
		{_T("Opteron(tm) Processor 23"), 0, 0x10, 2, -1, CPU_BARCELONA, CPU_ST_BLANK, CPU_S_F_1207, CPU_F_65, -1},
		{_T("Opteron(tm) Processor 83"), 0, 0x10, 2, -1, CPU_BARCELONA, CPU_ST_BLANK, CPU_S_F_1207, CPU_F_65, -1},

		//	Agena: Phenom X4 quad-core
		{_T("AMD Phenom(tm) 9"), 0, 0x10, 2, -1, CPU_AGENA, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_65, -1},
		{_T("X4 Quad-Core Processor GP-9"), 0, 0x10, 2, -1, CPU_AGENA, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_65, -1},	  // Phenom
		{_T("X4 Quad-Core Processor GP-983"), 0, 0x10, 2, -1, CPU_AGENA, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_65, -1}, // Phenom 9850 Black edition

		//	Toliman: Phenom X3 triple-core
		{_T("AMD Phenom(tm) 8"), 0, 0x10, 2, -1, CPU_TOLIMAN, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_65, -1},

		//	Kuma: AMD Athlon™ X2 Dual-Core Processors - 65nm Phenom based
		{_T("Athlon(tm) 6500 Dual-Core"), 0, 0x10, 2, -1, CPU_KUMA, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_65, -1},
		{_T("Athlon(tm) 7450 Dual-Core"), 0, 0x10, 2, -1, CPU_KUMA, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_65, -1},
		{_T("Athlon(tm) 7550 Dual-Core"), 0, 0x10, 2, -1, CPU_KUMA, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_65, -1},
		{_T("Athlon(tm) 7750 Dual-Core"), 0, 0x10, 2, -1, CPU_KUMA, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_65, -1},
		{_T("Athlon(tm) 7850 Dual-Core"), 0, 0x10, 2, -1, CPU_KUMA, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_65, -1},
		{_T("Phenom(tm) 7950 Quad-Core"), 0, 0x10, 2, -1, CPU_KUMA, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_65, -1},
		{_T("X2 Dual-Core Processor GP-773"), 0, 0x10, 2, -1, CPU_KUMA, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_65, -1}, // Phenom 7750 Black edition

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 10:4
		/*
		AuthenticAMD	 0x10	 0x4	 0x1	 AMD Engineering Sample
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X2 550 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X3 705e Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X3 710 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X3 720 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X3 B75 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X4 05e Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X4 10 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X4 20 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X4 805 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X4 810 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X4 905e Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X4 910 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X4 920 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X4 925 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X4 940 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X4 945 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X4 955 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X4 B45 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) II X4 B50 Processor
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Phenom(tm) X4 Quad-Core Processor GS-5560
		AuthenticAMD	 0x10	 0x4	 0x2	 AMD Processor model unknown
		AuthenticAMD	 0x10	 0x4	 0x2	 Quad-Core AMD Opteron(tm) Processor 2376
		AuthenticAMD	 0x10	 0x4	 0x2	 Quad-Core AMD Opteron(tm) Processor 2378
		AuthenticAMD	 0x10	 0x4	 0x2	 Quad-Core AMD Opteron(tm) Processor 2380
		AuthenticAMD	 0x10	 0x4	 0x2	 Quad-Core AMD Opteron(tm) Processor 2382
		AuthenticAMD	 0x10	 0x4	 0x2	 Quad-Core AMD Opteron(tm) Processor 2384
		AuthenticAMD	 0x10	 0x4	 0x2	 Quad-Core AMD Opteron(tm) Processor 2386 SE
		AMD Phenom(tm) II X3 B73 Processor
		AMD Phenom(tm) II X4 B45 Processor
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// Callisto
		{_T("Phenom(tm) II X2 5"), 0, 0x10, 4, -1, CPU_CALLISTO, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1},

		//	Heka
		{_T("Phenom(tm) II X3 7"), 0, 0x10, 4, -1, CPU_HEKA, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1},
		{_T("Phenom(tm) II X3 B73"), 0, 0x10, 4, -1, CPU_HEKA, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1}, // SI101022b
		{_T("Phenom(tm) II X3 B75"), 0, 0x10, 4, -1, CPU_HEKA, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1}, // SI101029
		{_T("Phenom(tm) II X3 B77"), 0, 0x10, 4, -1, CPU_HEKA, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1}, // SI101029

		// Deneb
		{_T("Phenom(tm) II X4 8"), 0, 0x10, 4, -1, CPU_DENEB, CPU_ST_BLANK, CPU_S_AM3_938_PIN, CPU_F_45, -1},
		{_T("Phenom(tm) II X4 90"), 0, 0x10, 4, -1, CPU_DENEB, CPU_ST_BLANK, CPU_S_AM3_938_PIN, CPU_F_45, -1},
		{_T("Phenom(tm) II X4 910"), 0, 0x10, 4, -1, CPU_DENEB, CPU_ST_BLANK, CPU_S_AM3_938_PIN, CPU_F_45, -1},
		{_T("Phenom(tm) II X4 920"), 0, 0x10, 4, -1, CPU_DENEB, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_45, -1},
		{_T("Phenom(tm) II X4 940"), 0, 0x10, 4, -1, CPU_DENEB, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_45, -1},
		{_T("Phenom(tm) II X4 945"), 0, 0x10, 4, -1, CPU_DENEB, CPU_ST_BLANK, CPU_S_AM3_938_PIN, CPU_F_45, -1},
		{_T("Phenom(tm) II X4 95"), 0, 0x10, 4, -1, CPU_DENEB, CPU_ST_BLANK, CPU_S_AM3_938_PIN, CPU_F_45, -1},
		{_T("Phenom(tm) II X4 96"), 0, 0x10, 4, -1, CPU_DENEB, CPU_ST_BLANK, CPU_S_AM3_938_PIN, CPU_F_45, -1},
		{_T("Phenom(tm) II X4 B"), 0, 0x10, 4, -1, CPU_DENEB, CPU_ST_BLANK, CPU_S_AM2_PLUS_938_PIN, CPU_F_45, -1},
		{_T("Phenom II 42 TWKR"), 0, 0x10, 4, -1, CPU_DENEB, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_45, -1},

		{_T("Phenom(tm) II X4 B4"), 0, 0x10, 4, -1, CPU_DENEB, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1}, // SI101022b

		//	Opteron Quad Core - 45nm - codename "Shanghai"
		{_T("Opteron(tm) Processor 13"), 0, 0x10, 4, -1, CPU_SHANGHAI, CPU_ST_BLANK, CPU_S_AM2, CPU_F_45, -1},	  // Note: 13xx, 1= 1-way, 3=Quad core	//BIT6010120008
		{_T("Opteron(tm) Processor 23"), 0, 0x10, 4, -1, CPU_SHANGHAI, CPU_ST_BLANK, CPU_S_F_1207, CPU_F_45, -1}, // BIT6010120008
		{_T("Opteron(tm) Processor 83"), 0, 0x10, 4, -1, CPU_SHANGHAI, CPU_ST_BLANK, CPU_S_F_1207, CPU_F_45, -1}, // BIT6010120008

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 10:5
		/*
		AuthenticAMD, 0x10, 0x5, 0x2, AMD Athlon(tm) II X3 425 Processor,
		AuthenticAMD, 0x10, 0x5, 0x2, AMD Athlon(tm) II X3 435 Processor,
		AuthenticAMD, 0x10, 0x5, 0x2, AMD Athlon(tm) II X4 620 Processor,
		AuthenticAMD, 0x10, 0x5, 0x2, AMD Athlon(tm) II X4 630 Processor,
		AuthenticAMD, 0x10, 0x5, 0x2, AMD Athlon(tm) II X4 635 Processor,
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		//	Rana
		{_T("Athlon(tm) II X3 4"), 0, 0x10, 5, 2, CPU_RANA, CPU_ST_BL_C2, CPU_S_AM2_PLUS_940_PIN, CPU_F_45, -1}, // BIT6010140000

		// Propus
		{_T("Athlon(tm) II X4 6"), 0, 0x10, 5, -1, CPU_PROPUS, CPU_ST_BLANK, CPU_S_AM3_938_PIN, CPU_F_45, -1}, // SI101045 bug fix

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 10:6
		/*
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Athlon(tm) II X2 215 Processor,
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Athlon(tm) II X2 235e Processor,
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Athlon(tm) II X2 240 Processor,
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Athlon(tm) II X2 240e Processor,
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Athlon(tm) II X2 245 Processor,
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Athlon(tm) II X2 250 Processor,
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Athlon(tm) II X2 250u Processor,
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Athlon(tm) II X2 255 Processor,
		AuthenticAMD	 0x10	 0x6	 0x2	 AMD Athlon(tm) X2 240 Processor
		AuthenticAMD	 0x10	 0x6	 0x2	 AMD Athlon(tm) X2 245 Processor
		AuthenticAMD	 0x10	 0x6	 0x2	 AMD Athlon(tm) X2 250 Processor
		AuthenticAMD	 0x10	 0x6	 0x2	 AMD Athlon(tm) X2 440
		AuthenticAMD	 0x10	 0x6	 0x2	 AMD Sempron(tm) Processor 140 Processor

		AuthenticAMD, 0x10, 0x6, 0x2, AMD Athlon(tm) II X2 4400e Processor,

		AuthenticAMD, 0x10, 0x6, 0x2, AMD Sempron(tm) 140 Processor,
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Sempron(tm) Processor 140 Processor,

		AuthenticAMD, 0x10, 0x6, 0x2, AMD Sempron(tm) M100,
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Athlon(tm) II Dual-Core M300,
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Athlon(tm) II Dual-Core M320,
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Turion(tm) II Dual-Core Mobile M500,
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Turion(tm) II Dual-Core Mobile M520,
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Turion(tm) II Ultra Dual-Core Mobile M600,
		AuthenticAMD, 0x10, 0x6, 0x2, AMD Turion(tm) II Ultra Dual-Core Mobile M620,

		AMD Turion(tm) II Dual-Core Mobile M500,319 	,16 	,6 	,2 	,28 Oct 2009 10:49:41 EDT
		AMD Turion(tm) II Dual-Core Mobile M520,264 	,16 	,6 	,2 	,24 Oct 2009 00:30:05 EDT
		AMD Turion(tm) II Dual-Core Mobile M540,2 	,16 	,6 	,2 	,17 Aug 2011 21:03:10 EDT
		AMD Turion(tm) II Ultra Dual-Core Mobile M600,173 	,16 	,6 	,2 	,25 Oct 2009 23:48:28 EDT
		AMD Turion(tm) II Ultra Dual-Core Mobile M620,103 	,16 	,6 	,2 	,4 Feb 2010 04:52:56 EST
		AMD Turion(tm) II Ultra Dual-Core Mobile M640,11 	,16 	,6 	,2 	,3 Nov 2009 06:28:01 EST
		AMD Turion(tm) II Ultra Dual-Core Mobile M660,8 	,16 	,6 	,2 	,14 Mar 2010 18:59:35 EDT,
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// Regor - AMD Athlon™ X2 Dual-Core Processors - 45nm Phenom based Regor
		{_T("AMD Athlon(tm) II X2 2"), 0, 0x10, 6, -1, CPU_REGOR, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1},
		{_T("Athlon(tm) II X2 4400e"), 0, 0x10, 6, 0x2, CPU_REGOR, CPU_ST_BL_C2, CPU_S_AM2_PLUS_940_PIN, CPU_F_45, -1}, // BIT6010200001
		{_T("Athlon(tm) II X2 4300e"), 0, 0x10, 6, -1, CPU_REGOR, CPU_ST_BLANK, CPU_S_AM2_PLUS_940_PIN, CPU_F_45, -1},	// SI101022d
		{_T("AMD Athlon(tm) X2 2"), 0, 0x10, 6, -1, CPU_REGOR, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1},
		{_T("AMD Athlon(tm) X2 4"), 0, 0x10, 6, -1, CPU_REGOR, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1},
		{_T("AMD Sempron(tm) X2 180"), 0, 0x10, 6, -1, CPU_REGOR, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1}, // SI101022d

		// Sargas
		{_T("Sempron(tm) Processor 140"), 0, 0x10, 6, -1, CPU_SARGAS, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1},
		{_T("AMD Sempron(tm) Processor 140"), 0, 0x10, 6, -1, CPU_SARGAS, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1}, // BIT6010200001
		{_T("Sempron(tm) Processor 130"), 0, 0x10, 6, -1, CPU_SARGAS, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1},	  // SI101025

		// Caspian
		{_T("AMD Sempron(tm) M"), 0, 0x10, 6, -1, CPU_CASPIAN, CPU_ST_BLANK, CPU_S_S1G3, CPU_F_45, -1},					// BIT6010200001
		{_T("Athlon(tm) II Dual-Core M"), 0, 0x10, 6, -1, CPU_CASPIAN, CPU_ST_BLANK, CPU_S_S1G3, CPU_F_45, -1},			// BIT6010200001
		{_T("Turion(tm) II Dual-Core Mobile M"), 0, 0x10, 6, -1, CPU_CASPIAN, CPU_ST_BLANK, CPU_S_S1G3, CPU_F_45, -1},	// BIT6010200001
		{_T("n(tm) II Ultra Dual-Core Mobile M"), 0, 0x10, 6, -1, CPU_CASPIAN, CPU_ST_BLANK, CPU_S_S1G3, CPU_F_45, -1}, // BIT6010200001	//Turion

		// Champlain	//SI101022d
		/*
		AMD Turion(tm) II P520 Dual-Core Processor,137 	,16 	,6 	,3 	,22 Jun 2010 13:55:13 EDT
		AMD Turion(tm) II P540 Dual-Core Processor,172 	,16 	,6 	,3 	,24 Sep 2010 21:57:44 EDT
		AMD Turion(tm) II P560 Dual-Core Processor,42 	,16 	,6 	,3 	,28 Jan 2011 00:48:44 EST
		AMD Turion(tm) II N530 Dual-Core Processor,20 	,16 	,6 	,3 	,9 Jun 2010 09:51:44 EDT
		AMD Turion(tm) II N550 Dual-Core Processor,2 	,16 	,6 	,3 	,16 Mar 2011 13:47:48 EDT
		AMD Athlon(tm) II Neo K125
		AMD Athlon(tm) II Neo K145 Processor
		AMD Athlon(tm) II Neo K325
		AMD Athlon(tm) II Neo K345 Dual-Core Processor
		*/
		{_T("Turion(tm) II P"), 0, 0x10, 6, -1, CPU_CHAMPLAIN, CPU_ST_BLANK, CPU_S_S1G4, CPU_F_45, -1}, // BIT6010200001
		{_T("Turion(tm) II N"), 0, 0x10, 6, -1, CPU_CHAMPLAIN, CPU_ST_BLANK, CPU_S_S1G4, CPU_F_45, -1}, // BIT6010200001

		{_T("Athlon(tm) II Neo K"), 0, 0x10, 6, -1, CPU_CHAMPLAIN, CPU_ST_BLANK, CPU_S_ASB2, CPU_F_45, -1}, // SI101023

		{_T("AMD Athlon(tm) II N370"), 0, 0x10, 6, -1, CPU_CHAMPLAIN, CPU_ST_BLANK, CPU_S_S1G4, CPU_F_45, -1}, // SI101029

		// Geneva	//SI101022d
		/*
		AMD Turion(tm) II Neo K625 Dual-Core Processor,35 	,16 	,6 	,3 	,16 Jun 2010 00:46:39 EDT
		AMD Turion(tm) II Neo K685 Dual-Core Processor,6 	,16 	,6 	,3 	,12 Jan 2011 16:16:39 EST
		AMD Turion(tm) II Neo N40L Dual-Core Processor,4 	,16 	,6 	,3 	,18 Dec 2011 13:00:05 EST
		AMD Turion(tm) II Neo N54L Dual-Core Processor,1 	,16 	,6 	,3 	,27 Oct 2011 21:22:25 EDT
		*/
		{_T("AMD Turion(tm) II Neo K"), 0, 0x10, 6, -1, CPU_GENEVA, CPU_ST_BLANK, CPU_S_ASB2, CPU_F_45, -1}, // BIT6010200001
		{_T("AMD Turion(tm) II Neo N"), 0, 0x10, 6, -1, CPU_GENEVA, CPU_ST_BLANK, CPU_S_ASB2, CPU_F_45, -1}, // BIT6010200001

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 10:8
		/*
		AuthenticAMD	 0x10	 0x8	 0x0	 Six-Core AMD Opteron(tm) Processor 2431
		AuthenticAMD	 0x10	 0x8	 0x0	 Six-Core AMD Opteron(tm) Processor 8435
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		//	Six-Core AMD OpteronTM processor - 45nm - codename "Istanbul"
		{_T("Six-Core AMD Opteron(tm)"), 0, 0x10, 8, -1, CPU_ISTANBUL, CPU_ST_BLANK, CPU_S_F_1207, CPU_F_45, -1},
		{_T("AMD Opteron"), 0, 0x10, 8, -1, CPU_ISTANBUL, CPU_ST_BLANK, CPU_S_F_1207, CPU_F_45, -1}, // BIT6010250003

		////////////////
		// Lisbon - TO DO
		////////////////

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 10:9
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		//	"Magny-Cours" 12-core 45nm Opteron 6100 series
		{_T("Opteron(tm) Processor 61"), 0, 0x10, 9, -1, CPU_MAGNY_COURS, CPU_ST_BLANK, CPU_S_SOCKET_G34, CPU_F_45, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 10:A
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		//	"THUBAN" Hexa-core 45nm
		{_T("Phenom(tm) II X6"), 0, 0x10, 0xA, -1, CPU_THUBAN, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1},
		{_T("AMD Six-Core Processor"), 0, 0x10, 0xA, -1, CPU_THUBAN, CPU_ST_BLANK, CPU_S_AM3, CPU_F_45, -1}, // SI101022d

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 11:3
		/*
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Athlon (tm) QI-46
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Athlon Dual-Core QL-60
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Athlon Dual-Core QL-62
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Athlon Dual-Core QL-64
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Athlon(tm) QI-46
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Athlon(tm) X2 Dual-Core QL-60
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Athlon(tm) X2 Dual-Core QL-62
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Athlon(tm) X2 Dual-Core QL-64
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Athlon(tm) X2 Dual-Core QL-65
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Athlon(tm)X2 DualCore QL-60
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Athlon(tm)X2 DualCore QL-64
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Sempron(tm) SI-40
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Sempron(tm) SI-42
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion Dual-Core RM-70
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion Dual-Core RM-72
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion Dual-Core RM-75
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion(tm) X2 Dual-Core Mobile RM-70
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion(tm) X2 Dual-Core Mobile RM-72
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion(tm) X2 Dual-Core Mobile RM-74
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion(tm) X2 Dual-Core Mobile RM-75
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion(tm) X2 Dual-Core Mobile RM-77
		AuthenticAMD, 0x11, 0x3, 0x1, AMD Turion(tm)X2 Dual Core Mobile RM-70,
		AuthenticAMD, 0x11, 0x3, 0x1, AMD Turion(tm)X2 Dual Core Mobile RM-72,
		AuthenticAMD, 0x11, 0x3, 0x1, AMD Turion(tm)X2 Dual Core Mobile RM-74,
		AuthenticAMD, 0x11, 0x3, 0x1, AMD Turion(tm)X2 Dual Core Mobile RM-76,
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion(tm) X2 Ultra Dual-Core Mobile ZM-80
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion(tm) X2 Ultra Dual-Core Mobile ZM-82
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion(tm) X2 Ultra Dual-Core Mobile ZM-84
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion(tm) X2 Ultra Dual-Core Mobile ZM-85
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion(tm) X2 Ultra Dual-Core Mobile ZM-86
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion(tm) X2 Ultra Dual-Core Mobile ZM-87
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion(tm)X2 Dual Core Mobile RM-70
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion(tm)X2 Dual Core Mobile RM-72
		AuthenticAMD	 0x11	 0x3	 0x1	 AMD Turion(tm)X2 Ultra DualCore Mobile ZM-82
		AuthenticAMD	 0x11	 0x3	 0x1	 Unknown
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// Maybe these should all be renamed "Lion". Griffin is the generic term for K11 AMDs (Barcelona for K10)
		//	AMD K11 CPUs
		{_T("AMD Athlon (tm) QI-46"), 0, 0x11, 3, -1, CPU_GRIFFIN, CPU_ST_BLANK, CPU_S_S1, CPU_F_65, -1},
		{_T("AMD Athlon(tm) QI-46"), 0, 0x11, 3, -1, CPU_GRIFFIN, CPU_ST_BLANK, CPU_S_S1, CPU_F_65, -1},
		{_T("AMD Athlon Dual-Core QL-6"), 0, 0x11, 3, -1, CPU_GRIFFIN, CPU_ST_BLANK, CPU_S_S1, CPU_F_65, -1},
		{_T("Athlon(tm) X2 Dual-Core QL-6"), 0, 0x11, 3, -1, CPU_GRIFFIN, CPU_ST_BLANK, CPU_S_S1, CPU_F_65, -1},
		{_T("Athlon(tm)X2 DualCore QL-6"), 0, 0x11, 3, -1, CPU_GRIFFIN, CPU_ST_BLANK, CPU_S_S1, CPU_F_65, -1},

		{_T("AMD Sempron(tm) SI-"), 0, 0x11, 3, -1, CPU_SABLE, CPU_ST_BLANK, CPU_S_S1, CPU_F_65, -1},

		{_T("Turion Dual-Core RM-7"), 0, 0x11, 3, 1, CPU_GRIFFIN, CPU_ST_LG_B1, CPU_S_S1, CPU_F_65, -1},		   // BIT6010140000
		{_T("X2 Dual-Core Mobile RM-7"), 0, 0x11, 3, 1, CPU_GRIFFIN, CPU_ST_LG_B1, CPU_S_S1, CPU_F_65, -1},		   // Turion	//BIT6010140000
		{_T("X2 Dual Core Mobile RM-7"), 0, 0x11, 3, 1, CPU_GRIFFIN, CPU_ST_LG_B1, CPU_S_S1, CPU_F_65, -1},		   // Turion	//BIT6010140000
		{_T("X2 Ultra Dual-Core Mobile ZM-8"), 0, 0x11, 3, -1, CPU_GRIFFIN, CPU_ST_BLANK, CPU_S_S1, CPU_F_65, -1}, // Turion

		// AMD Turion Dual-Core ZM-82	"1 	"	0x11	3
		// AMD Turion Dual-Core ZM-85	"2 	"	0x11	3
		{_T("Turion Dual-Core ZM-8"), 0, 0x11, 3, -1, CPU_GRIFFIN, CPU_ST_BLANK, CPU_S_S1, CPU_F_65, -1}, // Turion		//SI101022d

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 12h microarchitecture - called Fusion (or Llano)
		//	AMD K12 CPUs - Accelerated Processing Units (APU)
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// CPU_LYNX,
		// Lynx All models support: MMX, SSE, SSE2, SSE3, SSE4a, Enhanced 3DNow!, NX bit, AMD64, Cool'n'Quiet, AMD-V, Turbo Core (AMD equivalent of Intel Turbo Boost)
		// E2-3200 B0 2 2.4 GHz N/A 2 × 512 KB 24x 0.9125 - 1.4125 HD 6370D 160:8:4 443 MHz DDR3-1600 5 GT/s 65 W Socket FM1 4th quarter 2011
		// A4-3300 B0 2 2.5 GHz N/A 2 × 1 MB 25x 0.9125 - 1.4125 HD 6410D 160:8:4 443 MHz DDR3-1600 5 GT/s 65 W Socket FM1 3rd quarter 2011
		// A4-3400 B0 2 2.7 GHz N/A 2 × 1 MB 27x 0.9125 - 1.4125 HD 6410D 160:8:4 600 MHz DDR3-1600 5 GT/s 65 W Socket FM1 3rd quarter 2011
		// A6-3500 B0 3 2.1 GHz 2.4 GHz 3 × 1 MB 21x 0.9125 - 1.4125 HD 6530D 320:16:8 443 MHz DDR3-1866 5 GT/s 65 W Socket FM1 3rd quarter 2011
		// A6-3600 B0 4 2.1 GHz 2.4 GHz 4 × 1 MB 21x 0.9125 - 1.4125 HD 6530D 320:16:8 443 MHz DDR3-1866 5 GT/s 65 W Socket FM1 3rd quarter 2011
		// A6-3650 B0 4 2.6 GHz N/A 4 × 1 MB 26x 0.9125 - 1.4125 HD 6530D 320:16:8 443 MHz DDR3-1866 5 GT/s 100 W Socket FM1 June 30, 2011 AD3650WNZ43GX AD3650WNGXBOX
		// A8-3800 B0 4 2.4 GHz 2.7 GHz 4 × 1 MB 24x 0.9125 - 1.4125 HD 6550D 400:20:8 600 MHz DDR3-1866 5 GT/s 65 W Socket FM1 3rd quarter 2011
		// A8-3850 B0 4 2.9 GHz N/A 4 × 1 MB 29x 0.9125 - 1.4125 HD 6550D 400:20:8 600 MHz DDR3-1866 5 GT/s 100 W Socket FM1 June 30, 2011 AD3850WNZ43GX AD3850WNGXBOX
		// SI101025 - changed from Lynx to the common broader name Llano
		{_T("AMD E2-3200 APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("AMD A4-3300 APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("AMD A4-3400 APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("AMD A6-3400 APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1}, // SI101022d
		{_T("AMD A6-3420 APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("AMD A6-3500 APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("AMD A6-3600 APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1}, // SI101022d
		{_T("AMD A6-3620 APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("AMD A6-3650 APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1}, // SI101022d
		{_T("AMD A6-3670 APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("AMD A8-3800 APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("AMD A8-3820 APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1}, // SI101022d
		{_T("AMD A8-3850 APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("AMD A8-3870 APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1}, // SI101022d

		// CPU_SABINE,
		// Sabine All models support: SSE, SSE2, SSE3, SSE4a, Enhanced 3DNow!, NX bit, AMD64, PowerNow!, AMD-V
		// E2-3000M B0 2 1.8 GHz 2.4 GHz 2 × 512 KB 18x 0.9125 - 1.4125 HD 6380G 160:8:4 400 MHz DDR3-1333 2.5 GT/s 35 W Socket FS1 Q3 2011 EM3000DDX22GX
		// A4-3300M B0 2 1.9 GHz 2.5 GHz 2 × 1 MB 19x 0.9125 - 1.4125 HD 6480G 240:12:4 444 MHz DDR3-1333 2.5 GT/s 35 W Socket FS1 June 14, 2011 AM3300DDX23GX
		// A4-3310MX B0 2 2.1 GHz 2.5 GHz 2 × 1 MB 21x 0.9125 - 1.4125 HD 6480G 240:12:4 444 MHz DDR3-1333 2.5 GT/s 45 W Socket FS1 June 14, 2011 AM3310HLX23GX
		// A6-3400M B0 4 1.4 GHz 2.3 GHz 4 × 1 MB 14x 0.9125 - 1.4125 HD 6520G 320:16:8 400 MHz DDR3-1333 2.5 GT/s 35 W Socket FS1 June 14, 2011 AM3400DDX43GX
		// A6-3410MX B0 4 1.6 GHz 2.3 GHz 4 × 1 MB 16x 0.9125 - 1.4125 HD 6520G 320:16:8 400 MHz DDR3-1600 2.5 GT/s 45 W Socket FS1 June 14, 2011 AM3410HLX43GX
		// A8-3500M B0 4 1.5 GHz 2.4 GHz 4 × 1 MB 15x 0.9125 - 1.4125 HD 6620G 400:20:8 444 MHz DDR3-1333 2.5 GT/s 35 W Socket FS1 June 14, 2011 AM3500DDX43GX
		// A8-3510MX B0 4 1.8 GHz 2.5 GHz 4 × 1 MB 18x 0.9125 - 1.4125 HD 6620G 400:20:8 444 MHz DDR3-1600 2.5 GT/s 45 W Socket FS1 June 14, 2011 AM3510HLX43GX
		// A8-3530MX B0 4 1.9 GHz 2.6 GHz 4 × 1 MB 19x 0.9125 - 1.4125 HD 6620G 400:20:8 444 MHz DDR3-1600 2.5 GT/s 45 W Socket FS1 June 14, 2011 AM3530HLX43GX
		// SI101025 - changed from Sabine to the common broader name Llano
		{_T("E2-3000M APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("A4-3300M APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("A4-3305M APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1}, // SI101022d
		{_T("A4-3310MX APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("A4-3320M APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1}, // SI101025
		{_T("A6-3400M APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("A6-3410MX APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("A6-3420M APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},	 // SI101022d
		{_T("A6-3430MX APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1}, // SI101025
		{_T("A8-3550MX APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1}, // SI101025
		{_T("A8-3500M APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("A8-3510MX APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("A8-3520M APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1}, // SI101022d
		{_T("A8-3530MX APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1},
		{_T("A4-3330MX APU"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1}, // SI101047

		// AMD Athlon(tm) II X4 641 Quad-Core Processor	1 	18 	1 	0 	28 Mar 2012 15:57:26 EDT
		// AMD Athlon(tm) II X4 651 Quad-Core Processor	6 	18 	1 	0 	26 Feb 2012 01:59:26 EST
		{_T("Athlon(tm) II X4 6"), 0, 0x12, 1, -1, CPU_LLANO, CPU_ST_BLANK, CPU_S_FM1, CPU_F_32, -1}, // SI101024

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 14h CPUs - Bobcat (microarchitecture)
		/*
		AMD Bobcat Family 14h microarchitecture - The Family 14h microarchitecture, codenamed Bobcat, is a
		new distinct line from the original AMD64 microarchitectures, which is aimed in the 1 W to 10 W power
		category; the microprocessor core is in fact a very simplified x86 core. Ontario and Zacate were the
		first designs which implemented it.
		*/
		/*
		C-Series C-30 1000 1 9 512 80 277 11 UVD 3 1066
		C-Series C-50 1000 2 9 1024 80 276 11 UVD 3 1066
		C-Series C-60 1000/1.333 (turbo) 2 9 1024 80 276/400 (turbo) 11 UVD 3 1066
		E-Series E-240 1500 1 18 512 80 500 11 UVD 3 1066
		E-Series E-300 1300 2 18 1024 80 500 11 UVD 3 1066
		E-Series E-350 1600 2 18 1024 80 492 11 UVD 3 1066
		E-Series E-450 1650 2 18 1024 80 508/600 (turbo) 11 UVD 3 1333[7]
		G-Series T-24L 800 1 5 512 80  ?  ?  ? 1066
		G-Series T-30L 1400 1 18 512 80  ?  ?  ? 1333
		G-Series T-40N 1000 2 9 1024 80 276 11 UVD 3 1066
		G-Series T-44R 1200 1 9 512 80 276 11 UVD 3 1066
		G-Series T-48L 1400 2 18 1024 80  ?  ?  ? 1066
		G-Series T-48N 1400 2 18 1024 80 492 11 UVD 3 1066
		G-Series T-52R 1500 1 18 512 80 492 11 UVD 3 1066
		G-Series T-56N 1600 2 18 1024 80 492 11 UVD 3 1066
		Z-Series Z-01 1000 2 5.9 1024 80 276 11 UVD 3 1066
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		//_T("Zacate"),		//SI101021
		//"Zacate" (40nm)All models support: SSE, SSE2, SSE3, SSSE3, SSE4a, NX bit, AMD64, PowerNow!, AMD-V
		// E-240 B0 1 1.5 GHz N/A 512 KB 15× 1.175 - 1.35 HD 6310 80:8:4 500 MHz DDR3-1066 2.5 GT/s 18 W Socket FT1 (BGA-413) January 4, 2011 EME240GBB12GT
		// E-300  2 1.3 GHz 2 × 512 KB 13×  488 MHz Q3 2011
		// E-350 B0 1.6 GHz 16× 1.25 - 1.35 492 MHz January 4, 2011 EME350GBB22GT
		// E-450  1.65 GHz 16.5×  HD 6320 508–600 MHz DDR3-1333 Q3 2011
		{_T("AMD E-240"), 0, 0x14, 1, -1, CPU_ZACATE, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1},
		{_T("AMD E-300"), 0, 0x14, 1, -1, CPU_ZACATE, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1},
		{_T("AMD E-350"), 0, 0x14, 1, -1, CPU_ZACATE, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1},
		{_T("AMD E-450"), 0, 0x14, 1, -1, CPU_ZACATE, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1},
		// AMD G-T52R
		{_T("AMD G-T"), 0, 0x14, 1, -1, CPU_ZACATE, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1},

		// Brazos 2.0 (?)
		{_T("AMD E1-1200 APU"), 0, 0x14, 2, -1, CPU_ZACATE, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1}, // SI101029
		{_T("AMD E2-1800 APU"), 0, 0x14, 2, -1, CPU_ZACATE, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1}, // SI101029

		{_T("AMD E1-1500 APU"), 0, 0x14, 2, -1, CPU_ZACATE, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1}, // SI101047
		{_T("AMD E2-2000 APU"), 0, 0x14, 2, -1, CPU_ZACATE, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1}, // SI101047

		//_T("Desna"),		//SI101021
		//"Desna" (40nm)All models support: SSE, SSE2, SSE3, SSSE3, SSE4a, NX bit, AMD64, PowerNow!, AMD-V
		// Z-01 B0 2 1.0 GHz N/A 2 x 512 KB 10×  HD 6250 80:8:4 276 MHz DDR3-1066 2.5 GT/s 5.9 W Socket FT1 (BGA-413) June 1, 2011
		{_T("AMD Z-01"), 0, 0x14, 2, -1, CPU_DESNA, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1}, // SI101022d correction

		//_T("Ontario"),		//SI101021
		//"Ontario" (40nm)All models support: SSE, SSE2, SSE3, SSSE3, SSE4a, NX bit, AMD64, PowerNow!, AMD-V
		// C-30 B0 1 1.2 GHz N/A 512 KB 12× 1.25 - 1.35 HD 6250 80:8:4 276 MHz DDR3-1066 2.5 GT/s 9 W Socket FT1 (BGA-413) January 4, 2011 CMC30AFPB12GT
		{_T("AMD C-30"), 0, 0x14, 2, -1, CPU_ONTARIO, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1}, // SI101022d correction
		{_T("AMD C-50"), 0, 0x14, 2, -1, CPU_ONTARIO, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1}, // SI101022d correction
		{_T("AMD C-60"), 0, 0x14, 2, -1, CPU_ONTARIO, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1}, // SI101022d correction
		{_T("AMD C-70"), 0, 0x14, 2, -1, CPU_ONTARIO, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1}, // SI101047

		// Brazos-T - "Hondo" (2012, 40 nm)
		{_T("AMD Z-60"), 0, 0x14, 2, -1, CPU_HONDO, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1}, // SI101022d correction

		// AMD G-T40E Processor	"1 	"	0x14	2
		{_T("AMD G-T"), 0, 0x14, 2, -1, CPU_ONTARIO, CPU_ST_BLANK, CPU_S_FT1, CPU_F_40, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 15h.1 CPUs - Bulldozer (microarchitecture)
		/*
		The Family 15h microarchitecture, codenamed Bulldozer, is the latest successor of the Family 10h
		microarchitecture by way of Fusion Family 12h. Bulldozer is designed for processors in the 10 W to
		100 W category, implementing XOP, FMA4 and CVT16 instruction sets.[1] Orochi was the first design
		which implemented it.
		*/
		/*
		FX-8170 FX-8150 FX-8120 FX-8100 FX-6120 FX-6100 FX-4170 FX-4120 FX-4100
		AMD plans two series of Bulldozer based processors for servers: Opteron 4200 series (code named Valencia, with up to 8 cores) and Opteron 6200 series (code named Interlagos, with up to 16 cores)
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// SI1021
		{_T("FX-4100"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX-4120"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX-4170"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX-6100"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX-6120"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX-8100"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX-8120"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX-8150"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX-8170"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},

		// SU 1030
		// Re-added above block with (tm) included. Not sure if above is ever valid but (tm) is very common.
		{_T("FX(tm)-4100"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX(tm)-4120"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX(tm)-4170"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX(tm)-6100"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX(tm)-6120"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX(tm)-8100"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX(tm)-8120"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX(tm)-8150"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},
		{_T("FX(tm)-8170"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},

		{_T("FX(tm)-4170"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1}, // SI101025
		{_T("FX(tm)-6120"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1}, // SI101029
		{_T("FX(tm)-6200"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1}, // SI101025
		{_T("FX(tm)-8140"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1}, // SI101025

		// Zambezi
		{_T("FX-4130"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},	// SI101047
		{_T("FX-B4150"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1}, // SI101047
		{_T("FX-6130"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},	// SI101047
		{_T("FX-4150"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},	// SI101047
		{_T("FX-4200"), 0, 0x15, 1, -1, CPU_BULLDOZER, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},	// SI101047

		// AMD plans two series of Bulldozer based processors for servers:
		// Opteron 41xx series (code named Lisbon)
		{_T("Opteron(tm) Processor 41"), 0, 0x15, 8, -1, CPU_LISBON, CPU_ST_BLANK, CPU_S_C32, CPU_F_45, -1}, // SI101029
																											 // Opteron 4200 series (code named Valencia, with up to 8 cores) and
		{_T("Opteron(tm) Processor 42"), 0, 0x15, 1, -1, CPU_VALENCIA, CPU_ST_BLANK, CPU_S_C32, CPU_F_32, -1},
		// Opteron 62xx series (code named Interlagos, with up to 16 cores).
		{_T("Opteron(TM) Processor 62"), 0, 0x15, 1, -1, CPU_INTERLAGOS, CPU_ST_BLANK, CPU_S_SOCKET_G34, CPU_F_32, -1},
		{_T("Opteron(tm) Processor 62"), 0, 0x15, 1, -1, CPU_INTERLAGOS, CPU_ST_BLANK, CPU_S_SOCKET_G34, CPU_F_32, -1}, // SI101100
																														// Opteron 32xx series (code named Zurich)
		{_T("Opteron(tm) Processor 32"), 0, 0x15, 1, -1, CPU_ZURICH, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},		// SI101029
																														// For the server segment, the existing socket G34 (LGA1974) and socket C32 (LGA1207) will be used.

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 15h.2 CPUs - Piledriver (microarchitecture)

		// Vishera
		{_T("FX-4300 Quad-Core"), 0, 0x15, 2, -1, CPU_VISHERA, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},	// SI101047
		{_T("FX-4350 Quad-Core"), 0, 0x15, 2, -1, CPU_VISHERA, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},	// SI101047
		{_T("FX-6300 Six-Core"), 0, 0x15, 2, -1, CPU_VISHERA, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},	// SI101047
		{_T("FX-6350 Six-Core"), 0, 0x15, 2, -1, CPU_VISHERA, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},	// SI101047
		{_T("FX-8350 Eight-Core"), 0, 0x15, 2, -1, CPU_VISHERA, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1}, // SI101047
		{_T("FX-8320 Eight-Core"), 0, 0x15, 2, -1, CPU_VISHERA, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1}, // SI101047
		{_T("FX-8300 Eight-Core"), 0, 0x15, 2, -1, CPU_VISHERA, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1}, // SI101047
		{_T("FX-9370 Eight-Core"), 0, 0x15, 2, -1, CPU_VISHERA, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1}, // SI101047
		{_T("FX-9590 Eight-Core"), 0, 0x15, 2, -1, CPU_VISHERA, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1}, // SI101047

		{_T("FX(tm)-8310"), 0, 0x15, 2, -1, CPU_VISHERA, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1}, // SI101100
		{_T("FX-83"), 0, 0x15, 2, -1, CPU_VISHERA, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},		 // SI101100
		{_T("FX-43"), 0, 0x15, 2, -1, CPU_VISHERA, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},		 // SI101100

		// Opteron (32nm based on Piledriver)
		{_T("Opteron 33"), 0, 0x15, 2, -1, CPU_DELHI, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1},	// SI101047
		{_T("Opteron 43"), 0, 0x15, 2, -1, CPU_SEOUL, CPU_ST_BLANK, CPU_S_C32, CPU_F_32, -1},		// SI101047
		{_T("Opteron 63"), 0, 0x15, 2, -1, CPU_ABU_DHABI, CPU_ST_BLANK, CPU_S_G34, CPU_F_32, -1},	// SI101047
		{_T("Processor 33"), 0, 0x15, 2, -1, CPU_DELHI, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1}, // SI101075
		{_T("Processor 43"), 0, 0x15, 2, -1, CPU_SEOUL, CPU_ST_BLANK, CPU_S_C32, CPU_F_32, -1},		// SI101075
		{_T("Processor 63"), 0, 0x15, 2, -1, CPU_ABU_DHABI, CPU_ST_BLANK, CPU_S_G34, CPU_F_32, -1}, // SI101075

		// Opteron (32nm based on Piledriver) Interlagos
		{_T("Opteron 33"), 0, 0x15, 2, -1, CPU_DELHI, CPU_ST_BLANK, CPU_S_AM3PLUS, CPU_F_32, -1}, // SI101047

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 15h.10h CPUs - Piledriver cores - Comal trinity 2012, 32nm
		/*
		AMD A10-4600M APU with Radeon(tm) HD Graphics	"	265 	"	"21 	"	0x15	"16 	"	0x10	"1 	"	11/04/2012
		AMD A10-4655M APU with Radeon(tm) HD Graphics	"	10 	"	"21 	"	0x15	"16 	"	0x10	"1 	"	18/07/2012
		AMD A10-5700 APU with Radeon(tm) HD Graphics	"	112 	"	"21 	"	0x15	"16 	"	0x10	"1 	"	22/06/2012
		AMD A10-5800K APU with Radeon(tm) HD Graphics	"	655 	"	"21 	"	0x15	"16 	"	0x10	"1 	"	4/10/2012
		AMD A4-4300M APU with Radeon(tm) HD Graphics	"	20 	"	"21 	"	0x15	"16 	"	0x10	"1 	"	1/05/2012
		AMD A4-5300 APU with Radeon(tm) HD Graphics	"	78 	"	"21 	"	0x15	"16 	"	0x10	"1 	"	7/10/2012
		AMD A6-4400M APU with Radeon(tm) HD Graphics	"	144 	"	"21 	"	0x15	"16 	"	0x10	"1 	"	27/04/2012
		AMD A6-4455M APU with Radeon(tm) HD Graphics	"	31 	"	"21 	"	0x15	"16 	"	0x10	"1 	"	6/07/2012
		AMD A8-4500M APU with Radeon(tm) HD Graphics	"	325 	"	"21 	"	0x15	"16 	"	0x10	"1 	"	14/05/2012
		AMD A8-4555M APU with Radeon(tm) HD Graphics	"	22 	"	"21 	"	0x15	"16 	"	0x10	"1 	"	9/10/2012
		AMD A8-5500 APU with Radeon(tm) HD Graphics	"	131 	"	"21 	"	0x15	"16 	"	0x10	"1 	"	22/06/2012
		AMD A8-5600K APU with Radeon(tm) HD Graphics	"	192 	"	"21 	"	0x15	"16 	"	0x10	"1 	"	8/10/2012
		AMD R-464L APU with Radeon(tm) HD Graphics	"	2 	"	"21 	"	0x15	"16 	"	0x10	"1 	"	22/08/2012
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Virgo - Trinity (desktop)
		{_T("Athlon X2"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1},  // SI101066
		{_T("Athlon X4"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1},  // SI101066
		{_T("FirePro A3"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101066

		{_T("A4-5300 APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1},	  // SI101036
		{_T("A4-5300B APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1},  // SI101036
		{_T("A6-5400K APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1},  // SI101036
		{_T("A6-5400B APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1},  // SI101036
		{_T("A8-5500 APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1},	  // SI101029
		{_T("A8-5500B APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1},  // SI101036
		{_T("A8-5600K APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1},  // SI101036
		{_T("A10-5700 APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1},  // SI101029
		{_T("A10-5800K APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036
		{_T("A10-5800 APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1},  // SI101036
		{_T("A10-5800B APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Comal - Trinity (mobile)
		{_T("A10-4600M APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1}, // SI101036
		{_T("A8-4500M APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1},	// SI101036
		{_T("A6-4400M APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1},	// SI101036
		{_T("A6-4300M APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1},	// SI101036

		{_T("A10-4655M APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1}, // SI101036
		{_T("A8-4555M APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1},	// SI101036
		{_T("A6-4455M APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1},	// SI101036
		{_T("A4-4355M APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1},	// SI101036

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Comal - Trinity (embedded)
		{_T("R-252F APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1}, // SI101036
		{_T("R-260H APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1}, // SI101036
		{_T("R-268D APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1}, // SI101036
		{_T("R-272F APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1}, // SI101036
		{_T("R-452L APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1}, // SI101036
		{_T("R-460H APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1}, // SI101036
		{_T("R-460L APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1}, // SI101100
		{_T("R-464L APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1}, // SI101036

		// Trinity
		{_T("Athlon X4 740 Quad Core"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1},  // SI101047
		{_T("Athlon X4 750K Quad Core"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1}, // SI101047
		{_T("A10-4657M APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1},			   // SI101047
		{_T("A10-5800B APU"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1},			   // SI101047
		{_T("Athlon X2 340 Dual Core"), 0, 0x15, 0x10, -1, CPU_TRINITY, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_32, -1},  // SI101047

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 15h.13h CPUs - Piledriver cores - Richland
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Richland (desktop)
		{_T("Sempron X2"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101100

		{_T("Athlon X2 350"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1},	// SI101100
		{_T("Athlon X2 370K"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101047
		{_T("Athlon X4 750"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1},	// SI101100
		{_T("Athlon X4 760K"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101047

		{_T("A4-4000 "), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036
		{_T("A4-4020 "), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036
		{_T("A4-6300 "), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101047
		{_T("A4-6300B"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101047
		{_T("A4-6320 "), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101047

		{_T("A4 PRO-7300B"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101100
		{_T("A4-7300 "), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1},	  // SI101100

		{_T("A6-6400K"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036
		{_T("A6-6400B"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036
		{_T("A6-6420K"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036
		{_T("A6-6420B"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036

		{_T("A8-6500T"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036
		{_T("A8-6500 "), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036
		{_T("A8-6500B"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036
		{_T("A8-6600K"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036

		{_T("A10-6700T"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036
		{_T("A10-6790K"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101047
		{_T("A10-6700 "), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036
		{_T("A10-6790K"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036
		{_T("A10-6800K"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101036
		{_T("A10-6800B"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FM2, CPU_F_32, -1}, // SI101066

		// Mobile
		{_T("A4-5150M APU"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FS1r2, CPU_F_32, -1},	   // SI101047
		{_T("A6-5350M APU"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FS1r2, CPU_F_32, -1},	   // SI101047
		{_T("A6-5357M APU"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FP2_BGA, CPU_F_32, -1},  // SI101047
		{_T("A8-5550M APU"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FS1r2, CPU_F_32, -1},	   // SI101047
		{_T("A8-5557M APU"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FP2_BGA, CPU_F_32, -1},  // SI101047
		{_T("A10-5750M APU"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FS1r2, CPU_F_32, -1},   // SI101047
		{_T("A10-5757M APU"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FP2_BGA, CPU_F_32, -1}, // SI101047

		{_T("A4-5145M APU"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FP2_BGA, CPU_F_32, -1},  // SI101066
		{_T("A6-5345M APU"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FP2_BGA, CPU_F_32, -1},  // SI101066
		{_T("A8-5545M APU"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FP2_BGA, CPU_F_32, -1},  // SI101047
		{_T("A10-5745M APU"), 0, 0x15, 0x13, -1, CPU_RICHLAND, CPU_ST_BLANK, CPU_S_FP2_BGA, CPU_F_32, -1}, // SI101047

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 15h.30 CPUs - Kaveri cores (Steamroller architecture)
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Kaveri (A10-7850K, A10-7800, A10-7700K, A8-7600, A6-7400K, A4-7300) Desktop
		{_T("A10-7850K"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1}, // SI101049
		{_T("A10-7800"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},  // SI101049
		{_T("A10-7700K"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1}, // SI101049
		{_T("A8-7650K"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},  // SI101100
		{_T("A8-7600B"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},  // SI101075
		{_T("A8-7600"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},	  // SI101049
		{_T("A8-7500"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},	  // SI101049
		{_T("A8-7050"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},	  // SI101075
		{_T("A6-7400K"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},  // SI101049
		{_T("A6-7400B"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},  // SI101075
		{_T("A4-7300"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},	  // SI101049

		{_T("7800B"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1}, // SI101075
		{_T("7850B"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1}, // SI101075

		{_T("A10 PRO-7850B"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1}, // SI101100
		{_T("A10 PRO-7800B"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1}, // SI101100
		{_T("A8 PRO-8650B"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},  // SI101109
		{_T("A8 PRO-7600B"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},  // SI101100
		{_T("A6 PRO-8550B"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},  // SI101109
		{_T("A6 PRO-7400B"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},  // SI101100
		{_T("A4 PRO-7350B"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},  // SI101100
		{_T("X4 840"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},		  // SI101100
		{_T("X4 860K"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},		  // SI101100
		{_T("X2 450"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},		  // SI101100

		// Kaveri mobile
		{_T("A6-7000"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FP3, CPU_F_28, -1},		 // SI101075
		{_T("A6 Pro-7050B"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FP3, CPU_F_28, -1},	 // SI101075
		{_T("A8-7100"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FP3, CPU_F_28, -1},		 // SI101075
		{_T("A8 Pro-7150B"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FP3, CPU_F_28, -1},	 // SI101075
		{_T("A10-7300"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FP3, CPU_F_28, -1},		 // SI101075
		{_T("A10 Pro-7350B"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FP3, CPU_F_28, -1}, // SI101075
		{_T("FX-7500"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FP3, CPU_F_28, -1},		 // SI101075
		{_T("A8-7200P"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FP3, CPU_F_28, -1},		 // SI101075
		{_T("A10-7400P"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FP3, CPU_F_28, -1},	 // SI101075
		{_T("FX-7600P"), 0, 0x15, 0x30, -1, CPU_KAVERI, CPU_ST_BLANK, CPU_S_FP3, CPU_F_28, -1},		 // SI101075

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 15h.38 CPUs - Godavari cores (Steamroller architecture)
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		{_T("A10-8850K"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A10-8850"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A10-8750 "), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A10-7890K"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A10-7870K"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A10-7860K"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A8-8650K"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A8-8650 "), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A6-8550K"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A6-8650"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A6-8550B"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A4-8350B"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A6-7470K"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},

		{_T("A8-7670K"), 0, 0x15, 0x30, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A10-8850B"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A10-8750B"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("A4 PRO-8350B"), 0, 0x15, 0x30, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},

		{_T("X4 880K"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("X4 870K"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("X4 850"), 0, 0x15, 0x38, -1, CPU_GODAVARI, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},

		/*
		AMD RX-427BB with AMD Radeon(tm) R7 Graphics
		RX - 427BB
		RX - 425BB
		RX - 225FB
		RX - 427NB
		RX - 219NB
		*/
		{_T("RX-4"), 0, 0x15, 0x30, -1, CPU_BALD_EAGLE, CPU_ST_BLANK, CPU_S_FP3, CPU_F_28, -1}, // SI101100
		{_T("RX-2"), 0, 0x15, 0x30, -1, CPU_BALD_EAGLE, CPU_ST_BLANK, CPU_S_FP3, CPU_F_28, -1}, // SI101100

		/*
		AMD Embedded R-Series RX-421BD Radeon R7
		RX-216GD
		RX-216TD
		RX-416GD
		RX-418GD
		RX-421BD
		RX-421ND
		*/
		{_T("RX-4"), 0, 0x15, 0x60, -1, CPU_MERLIN_FALCON, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("RX-2"), 0, 0x15, 0x60, -1, CPU_MERLIN_FALCON, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 15h.60 CPUs - Carrizo DDR3 cores - Excavator architecture
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("A6-8500P"), 0, 0x15, 0x60, -1, CPU_CARRIZO, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A6-8500B"), 0, 0x15, 0x60, -1, CPU_CARRIZO, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A8-8600P"), 0, 0x15, 0x60, -1, CPU_CARRIZO, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A8-8600B"), 0, 0x15, 0x60, -1, CPU_CARRIZO, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A10-8700P"), 0, 0x15, 0x60, -1, CPU_CARRIZO, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A10-8700B"), 0, 0x15, 0x60, -1, CPU_CARRIZO, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("FX-8800P"), 0, 0x15, 0x60, -1, CPU_CARRIZO, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A12-8800B"), 0, 0x15, 0x60, -1, CPU_CARRIZO, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A10-8780P"), 0, 0x15, 0x60, -1, CPU_CARRIZO, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},

		{_T("A10 Extreme Edition"), 0, 0x15, 0x60, -1, CPU_CARRIZO, CPU_ST_BLANK, CPU_S_AM4, CPU_F_28, -1},

		{_T("X4 835"), 0, 0x15, 0x60, -1, CPU_CARRIZO, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},
		{_T("X4 845"), 0, 0x15, 0x60, -1, CPU_CARRIZO, CPU_ST_BLANK, CPU_S_FM2_PLUS, CPU_F_28, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 15h.60 CPUs - Carrizo DDR4 cores - Excavator architecture
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("A12-8830B"), 0, 0x15, 0x60, -1, CPU_CARRIZO_DDR4, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A10-8730B"), 0, 0x15, 0x60, -1, CPU_CARRIZO_DDR4, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A6-8530B"), 0, 0x15, 0x60, -1, CPU_CARRIZO_DDR4, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},

		{_T("A12-8870E"), 0, 0x15, 0x60, -1, CPU_CARRIZO_DDR4, CPU_ST_BLANK, CPU_S_AM4, CPU_F_28, -1},
		{_T("A10-8770E"), 0, 0x15, 0x60, -1, CPU_CARRIZO_DDR4, CPU_ST_BLANK, CPU_S_AM4, CPU_F_28, -1},
		{_T("A6-8570E"), 0, 0x15, 0x60, -1, CPU_CARRIZO_DDR4, CPU_ST_BLANK, CPU_S_AM4, CPU_F_28, -1},

		{_T("A12-8870"), 0, 0x15, 0x60, -1, CPU_CARRIZO_DDR4, CPU_ST_BLANK, CPU_S_AM4, CPU_F_28, -1},
		{_T("A10-8770"), 0, 0x15, 0x60, -1, CPU_CARRIZO_DDR4, CPU_ST_BLANK, CPU_S_AM4, CPU_F_28, -1},
		{_T("A10-8770"), 0, 0x15, 0x60, -1, CPU_CARRIZO_DDR4, CPU_ST_BLANK, CPU_S_AM4, CPU_F_28, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 15h.65 CPUs - Bristol Ridge (Carrizo) cores - Excavator architecture
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("A10-9600P"), 0, 0x15, 0x65, -1, CPU_BRISTOL_RIDGE, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A10-9630P"), 0, 0x15, 0x65, -1, CPU_BRISTOL_RIDGE, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A12-9700P"), 0, 0x15, 0x65, -1, CPU_BRISTOL_RIDGE, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A12-9730P"), 0, 0x15, 0x65, -1, CPU_BRISTOL_RIDGE, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("FX-9800P"), 0, 0x15, 0x65, -1, CPU_BRISTOL_RIDGE, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("FX-9830P"), 0, 0x15, 0x65, -1, CPU_BRISTOL_RIDGE, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},

		{_T("A6-9500"), 0, 0x15, 0x65, -1, CPU_BRISTOL_RIDGE, CPU_ST_BLANK, CPU_S_AM4, CPU_F_28, -1},
		{_T("A8-9600"), 0, 0x15, 0x65, -1, CPU_BRISTOL_RIDGE, CPU_ST_BLANK, CPU_S_AM4, CPU_F_28, -1},
		{_T("A10-9700"), 0, 0x15, 0x65, -1, CPU_BRISTOL_RIDGE, CPU_ST_BLANK, CPU_S_AM4, CPU_F_28, -1},
		{_T("A12-9800"), 0, 0x15, 0x65, -1, CPU_BRISTOL_RIDGE, CPU_ST_BLANK, CPU_S_AM4, CPU_F_28, -1},

		{_T("X4 950"), 0, 0x15, 0x65, -1, CPU_BRISTOL_RIDGE, CPU_ST_BLANK, CPU_S_AM4, CPU_F_28, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 15h.70 CPUs - Stoney Ridge cores - Excavator architecture
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("E2-9010"), 0, 0x15, 0x70, -1, CPU_STONEY_RIDGE, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A6-9210"), 0, 0x15, 0x70, -1, CPU_STONEY_RIDGE, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A9-9410"), 0, 0x15, 0x70, -1, CPU_STONEY_RIDGE, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 16h.0 CPUs - Jaguar cores - Kabini (2013 28nm) Sockey FT3 (BGA)
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Kabini, Mainstream APU
		{_T("E1-2"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1}, // SI101066
		{_T("E2-3"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1}, // SI101066
		{_T("A4-5"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1}, // SI101066
		{_T("A6-5"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1}, // SI101066

		// Kabini" (2014, 28 nm)
		{_T("5150 APU"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_AM1, CPU_F_28, -1}, // SI101066
		{_T("5350 APU"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_AM1, CPU_F_28, -1}, // SI101066
		{_T("5370 APU"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_AM1, CPU_F_28, -1},
		{_T("2650 APU"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_AM1, CPU_F_28, -1}, // SI101066
		{_T("3850 APU"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_AM1, CPU_F_28, -1}, // SI101066

		// Temash, Elite Mobility APU
		{_T("A4-1200 APU"), 0, 0x16, 0x0, -1, CPU_TEMASH, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1}, // SI101036
		{_T("A4-1250 APU"), 0, 0x16, 0x0, -1, CPU_TEMASH, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1}, // SI101036
		{_T("A4-1350 APU"), 0, 0x16, 0x0, -1, CPU_TEMASH, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1},
		{_T("A6-1450 APU"), 0, 0x16, 0x0, -1, CPU_TEMASH, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1}, // SI101036

		// Kabini, Mainstream
		{_T("E1-2100"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1},
		{_T("E1-2200"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1},
		{_T("E1-2500"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1},
		{_T("E2-3000"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1},
		{_T("E2-3800"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1},
		{_T("A4-5000"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1},
		{_T("A4-5100"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1},
		{_T("A6-5200"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1},
		{_T("A4 Pro-3340B"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1},

		// Kabini, embedded
		{_T("GX-420CA"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3_769_BGA, CPU_F_28, -1}, // SI101036
		{_T("GX-416RA"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3_769_BGA, CPU_F_28, -1}, // SI101036
		{_T("GX-415GA"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3_769_BGA, CPU_F_28, -1}, // SI101036
		{_T("GX-217GA"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3_769_BGA, CPU_F_28, -1}, // SI101036
		{_T("GX-210HA"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3_769_BGA, CPU_F_28, -1}, // SI101036
		{_T("GX-210JA"), 0, 0x16, 0x0, -1, CPU_KABINI, CPU_ST_BLANK, CPU_S_FT3_769_BGA, CPU_F_28, -1}, // SI101036

		// Operton
		{_T("X1150"), 0, 0x16, 0x0, -1, CPU_KYOTO, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1}, // SI101036
		{_T("X2150"), 0, 0x16, 0x0, -1, CPU_KYOTO, CPU_ST_BLANK, CPU_S_FT3, CPU_F_28, -1}, // SI101036

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 16h.30 CPUs - Puma cores
		/*
		CPU_BEEMA,
		CPU_MULLINS,
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Beema Notebook APU: 0x16, 0x30
		{_T("E1-6010"), 0, 0x16, 0x30, -1, CPU_BEEMA, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1}, // SI101075
		{_T("E1-6015"), 0, 0x16, 0x30, -1, CPU_BEEMA, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},
		{_T("E1-6050"), 0, 0x16, 0x30, -1, CPU_BEEMA, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},
		{_T("E2-6110"), 0, 0x16, 0x30, -1, CPU_BEEMA, CPU_ST_BLANK, CPU_S_AM1, CPU_F_28, -1},	// SI101075
		{_T("A4-6210"), 0, 0x16, 0x30, -1, CPU_BEEMA, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},	// SI101075
		{_T("A4-6250J"), 0, 0x16, 0x30, -1, CPU_BEEMA, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1}, // SI101100
		{_T("A6-6310"), 0, 0x16, 0x30, -1, CPU_BEEMA, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},	// SI101075
		{_T("A8-6410"), 0, 0x16, 0x30, -1, CPU_BEEMA, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},	// SI101075
		{_T("A4 Pro-3350B"), 0, 0x16, 0x30, -1, CPU_BEEMA, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},

		// Beema, Tablet/2-in-1 APU
		{_T("E1 Micro-6200T"), 0, 0x16, 0x30, -1, CPU_MULLINS, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1}, // SI101075
		{_T("A4 Micro-6400T"), 0, 0x16, 0x30, -1, CPU_MULLINS, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1}, // SI101075
		{_T("A6 Micro-6500T"), 0, 0x16, 0x30, -1, CPU_MULLINS, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},
		{_T("A10 Micro-6700T"), 0, 0x16, 0x30, -1, CPU_MULLINS, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1}, // SI101075

		// Embedded SOC with Radeon(TM) XXX Graphics
		{_T("GX-212JC"), 0, 0x16, 0x30, -1, CPU_BLANK, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_28, -1}, // SI101075
		{_T("GX-424CC"), 0, 0x16, 0x30, -1, CPU_BLANK, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_28, -1}, // SI101075

		//???? - from PT database
		{_T("310 APU"), 0, 0x16, 0x30, -1, CPU_BLANK, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_28, -1},		 // SI101075
		{_T("410 APU"), 0, 0x16, 0x30, -1, CPU_BLANK, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_28, -1},		 // SI101075
		{_T("HP Hexa-Core"), 0, 0x16, 0x30, -1, CPU_BLANK, CPU_ST_BLANK, CPU_S_BLANK, CPU_F_28, -1}, // SI101100

		// Steppe Eagle, SoC
		{_T("GX-210JC"), 0, 0x16, 0x30, -1, CPU_STEPPE_EAGLE, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},
		{_T("GX-212JC"), 0, 0x16, 0x30, -1, CPU_STEPPE_EAGLE, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},
		{_T("GX-216HC"), 0, 0x16, 0x30, -1, CPU_STEPPE_EAGLE, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},
		{_T("GX-222GC"), 0, 0x16, 0x30, -1, CPU_STEPPE_EAGLE, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},
		{_T("GX-412HC"), 0, 0x16, 0x30, -1, CPU_STEPPE_EAGLE, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},
		{_T("GX-424CC"), 0, 0x16, 0x30, -1, CPU_STEPPE_EAGLE, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},

		// Carrizo-L
		{_T("E1-7010"), 0, 0x16, 0x30, -1, CPU_CARRIZO_L, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("E2-7110"), 0, 0x16, 0x30, -1, CPU_CARRIZO_L, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A4-7210"), 0, 0x16, 0x30, -1, CPU_CARRIZO_L, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},
		{_T("A6-7310"), 0, 0x16, 0x30, -1, CPU_CARRIZO_L, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},
		{_T("A8-7410"), 0, 0x16, 0x30, -1, CPU_CARRIZO_L, CPU_ST_BLANK, CPU_S_FP4, CPU_F_28, -1},

		{_T("PRO A4-3350B"), 0, 0x16, 0x30, -1, CPU_CARRIZO_L, CPU_ST_BLANK, CPU_S_FT3b, CPU_F_28, -1},

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// AMD 17h.0 CPUs - Zen cores - Ryzen (2017 14nm) Socket AM4
		/*
		 */
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		{_T("Ryzen 7 1500X"), 0, 0x17, 1, -1, CPU_RYZEN, CPU_ST_BLANK, CPU_S_AM4, CPU_F_14, -1},
		{_T("Ryzen 7 1600X"), 0, 0x17, 1, -1, CPU_RYZEN, CPU_ST_BLANK, CPU_S_AM4, CPU_F_14, -1},
		{_T("Ryzen 7 1700"), 0, 0x17, 1, -1, CPU_RYZEN, CPU_ST_BLANK, CPU_S_AM4, CPU_F_14, -1},
		{_T("Ryzen 7 1700X"), 0, 0x17, 1, -1, CPU_RYZEN, CPU_ST_BLANK, CPU_S_AM4, CPU_F_14, -1},
		{_T("Ryzen 7 1800X"), 0, 0x17, 1, -1, CPU_RYZEN, CPU_ST_BLANK, CPU_S_AM4, CPU_F_14, -1},

		// Mobile chips use different socket
		{_T("Ryzen 5 2500U"), 0, 0x17, 0x11, -1, CPU_RYZEN, CPU_ST_BLANK, CPU_S_FP5, CPU_F_14, -1},
		{_T("Ryzen 7 2700U"), 0, 0x17, 0x11, -1, CPU_RYZEN, CPU_ST_BLANK, CPU_S_FP5, CPU_F_14, -1},
		{_T("Ryzen 3 2200U"), 0, 0x17, 0x11, -1, CPU_RYZEN, CPU_ST_BLANK, CPU_S_FP5, CPU_F_14, -1},
		//{ _T("Ryzen 3 PRO 2300U"), 0, 0x17, 1, -1, CPU_RYZEN, CPU_ST_BLANK, CPU_S_FP5, CPU_F_14, -1 },
		//{ _T("Ryzen 5 PRO 2500U"), 0, 0x17, 1, -1, CPU_RYZEN, CPU_ST_BLANK, CPU_S_FP5, CPU_F_14, -1 },
		//{ _T("Ryzen 7 PRO 2500U"), 0, 0x17, 1, -1, CPU_RYZEN, CPU_ST_BLANK, CPU_S_FP5, CPU_F_14, -1 },

		{_T("Ryzen"), 0, 0x17, 0x10, -1, CPU_RYZEN, CPU_ST_BLANK, CPU_S_AM4, CPU_F_14, -1},
		{_T("Ryzen"), 0, 0x17, 0x8, -1, CPU_RYZEN, CPU_ST_BLANK, CPU_S_AM4, CPU_F_12, -1}, // 12nm like Ryzen Threadripper 2990WX

		// Ryzen 5000 Series Desktop Cezanne - 5700G, 5600G,
		{_T("Ryzen"), 0, 0x19, 0x50, -1, CPU_ZEN3_C, CPU_ST_BLANK, CPU_S_AM4, CPU_F_7, -1},

		// Ryzen 5000 Series Desktop Vemeer - 5950X , 5900X, 5800X, 5600X
		{_T("Ryzen"), 0, 0x19, 0x21, -1, CPU_ZEN3_V, CPU_ST_BLANK, CPU_S_AM4, CPU_F_7, -1},

		// Ryzen 7000 series
		{_T("Ryzen"), 0, 0x19, 0x61, -1, CPU_ZEN4_R, CPU_ST_BLANK, CPU_S_AM5, CPU_F_5, -1},

		// Alder Lake

		//{ _T("Ryzen Threadripper 2990WX"), 0, 0x17, 0x8, -1, CPU_RYZEN, CPU_ST_BLANK, CPU_S_AM4, CPU_F_14, -1 },
		// CPU_SPECIFICATION

		/*
		PUMA - to do - not enough info available yet

		Desktop/Mobile
		A6 6310 4 1.8 GHz 2.4 GHz 2 MB Radeon R4 128:?:? 800 MHz 15W DDR3L-1866
		A4 6210 N/A Radeon R3 600 MHz DDR3L-1600
		E2 6110 1.5 GHz Radeon R2 500 MHz
		E1 6010 2 1.35 GHz 1 MB 350 MHz 10 W DDR3L-1333

		Tablet
		A10 Micro 6700T 4 1.2 GHz 2.2 GHz 2 MB Radeon R6 128:?:? 500 MHz 4.5 W 2.8 W DDR3L-1333
		A4 Micro 6400T 1.0 GHz 1.6 GHz Radeon R3 350 MHz
		E1 Micro 6200T 2 1.4 GHz 1 MB Radeon R2 300 MHz 3.95 W DDR3L-1066
		*/

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// VIA 6:x
		/*
		CentaurHauls	 0x6	 0x9	 0x8	 686 Gen
		CentaurHauls	 0x6	 0x9	 0xa	 686 Gen
		CentaurHauls	 0x6	 0xa	 0x9	 686 Gen
		CentaurHauls	 0x6	 0xd	 0x0	 686 Gen
		CentaurHauls	 0x6	 0xf	 0x2	 686 Gen
		CentaurHauls	 0x6	 0xf	 0x3	 686 Gen
		CentaurHauls	 0x6	 0xf	 0x8	 686 Gen
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// NOT DONE
		// NOT DONE
		//- VIA Nano 1000/2000/3000, Eden X2, Nano X2/X3, QuadCore.
		// VIA C7-D Processor 1500MHz	"1 	"	0x06	D
		// VIA Nano X2 L4050 @ 1.4 GHz	"1 	"	0x06	F
		// VIA Nano X2 U4025 @ 1.2 GHz	"1 	"	0x06	F
		// VIA C3 Ezra	"	0 	"	"6 	"	"8 	"
		// VIA Esther processor 1300MHz	"	0 	"	"6 	"	"10 	"
		// VIA Eden Processor 800MHz	"	0 	"	"6 	"	"13 		"
		// VIA Nano L2007@1600MHz	"	0 	"	"6 	"	"15 	"

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// Transmeta f:2
		/*
		GenuineTMx86	 0xf	 0x2	 0x4	 TransMeta - Unknown type
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// NOT DONE

		// Terminator
		{_T(""), -1, -1, -1, -1, 0, 0, 0, 0, -1},
};
