// PassMark SysInfo
//
// Copyright (c) 2016
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
//	JedecIDList.c
//
// Author(s):
//	Keith Mah
//
// Description:
//   Functions to obtain the JEDEC manufacture name from the JEDEC ID code
//

#ifdef WIN32
#pragma optimize("", on)

#include <tchar.h>
#include <windows.h>
#include <Windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include "resource.h"
#include "SysInfoPrivate.h"
#else
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <uMemTest86.h>

#define memset(buf, val, size) SetMem(buf, size, val)
#define _T(x) L##x
#define _tcscpy(dest, src) StrCpyS(dest, sizeof(dest) / sizeof(dest[0]), src)
#define _tcscpy_s(dest, maxchars, src) StrCpyS(dest, maxchars, src)
#define _tcsncpy(dest, src, count) StrnCpyS(dest, sizeof(dest) / sizeof(dest[0]), src, count)
#define _tcsncpy_s(dest, maxchars, src, count) StrnCpyS(dest, maxchars, src, count)
#define _tcslen StrLen
#define wchar_t CHAR16
#define ULONGLONG UINT64
#define bool BOOLEAN
#define true TRUE
#define false FALSE
#endif

static const wchar_t *BANK1_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("AMD"),
    _T("AMI"),
    _T("Fairchild"),
    _T("RAMXEED Limited"),
    _T("GTE"),
    _T("Harris"),
    _T("Hitachi"),
    _T("Inmos"),
    _T("Intel"),
    _T("I.T.T."),
    _T("Intersil"),
    _T("Monolithic Memories"),
    _T("Mostek"),
    _T("Freescale (Motorola)"),
    _T("National"),
    _T("NEC"),
    _T("RCA"),
    _T("Raytheon"),
    _T("Synaptics"),
    _T("Seeq"),
    _T("NXP (Philips)"),
    _T("Synertek"),
    _T("Texas Instruments"),
    _T("Kioxia Corporation"),
    _T("Xicor"),
    _T("Zilog"),
    _T("Eurotechnique"),
    _T("Mitsubishi"),
    _T("Lucent (AT&T)"),
    _T("Exel"),
    _T("Atmel"),
    _T("STMicroelectronics"),
    _T("Lattice Semi."),
    _T("NCR"),
    _T("Wafer Scale Integration"),
    _T("IBM"),
    _T("Tristar"),
    _T("Visic"),
    _T("Intl. CMOS Technology"),
    _T("SSSI"),
    _T("MicrochipTechnology"),
    _T("Ricoh Ltd"),
    _T("VLSI"),
    _T("Micron Technology"),
    _T("SK Hynix"),
    _T("OKI Semiconductor"),
    _T("ACTEL"),
    _T("Sharp"),
    _T("Catalyst"),
    _T("Panasonic"),
    _T("IDT"),
    _T("Cypress"),
    _T("DEC"),
    _T("LSI Logic"),
    _T("Zarlink (Plessey)"),
    _T("UTMC"),
    _T("Thinking Machine"),
    _T("Thomson CSF"),
    _T("Integrated CMOS (Vertex)"),
    _T("Honeywell"),
    _T("Tektronix"),
    _T("Oracle Corporation"),
    _T("SST"),
    _T("ProMos/Mosel Vitelic"),
    _T("Infineon (Siemens)"),
    _T("Macronix"),
    _T("Xerox"),
    _T("Plus Logic"),
    _T("SanDisk Technologies Inc"),
    _T("Elan Circuit Tech."),
    _T("European Silicon Str."),
    _T("Apple Computer"),
    _T("Xilinx"),
    _T("Compaq"),
    _T("Protocol Engines"),
    _T("SCI"),
    _T("ABLIC"),
    _T("Samsung"),
    _T("I3 Design System"),
    _T("Klic"),
    _T("Crosspoint Solutions"),
    _T("Alliance Memory Inc"),
    _T("Tandem"),
    _T("Hewlett-Packard"),
    _T("Intg. Silicon Solutions"),
    _T("Brooktree"),
    _T("New Media"),
    _T("MHS Electronic"),
    _T("Performance Semi."),
    _T("Winbond Electronic"),
    _T("Kawasaki Steel"),
    _T("Bright Micro"),
    _T("TECMAR"),
    _T("Exar"),
    _T("PCMCIA"),
    _T("LG Semi (Goldstar)"),
    _T("Northern Telecom"),
    _T("Sanyo"),
    _T("Array Microsystems"),
    _T("Crystal Semiconductor"),
    _T("Analog Devices"),
    _T("PMC-Sierra"),
    _T("Asparix"),
    _T("Convex Computer"),
    _T("Quality Semiconductor"),
    _T("Nimbus Technology"),
    _T("Transwitch"),
    _T("Micronas (ITT Intermetall)"),
    _T("Cannon"),
    _T("Altera"),
    _T("NEXCOM"),
    _T("QUALCOMM"),
    _T("Sony"),
    _T("Cray Research"),
    _T("AMS(Austria Micro)"),
    _T("Vitesse"),
    _T("Aster Electronics"),
    _T("Bay Networks (Synoptic)"),
    _T("Zentrum/ZMD"),
    _T("TRW"),
    _T("Thesys"),
    _T("Solbourne Computer"),
    _T("Allied-Signal"),
    _T("Dialog Semiconductor"),
    _T("Media Vision"),
    _T("Numonyx Corporation")};

static const wchar_t *BANK2_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("Cirrus Logic"),
    _T("National Instruments"),
    _T("ILC Data Device"),
    _T("Alcatel Mietec"),
    _T("Micro Linear"),
    _T("Univ. of NC"),
    _T("JTAG Technologies"),
    _T("BAE Systems (Loral)"),
    _T("Nchip"),
    _T("Galileo Tech"),
    _T("Bestlink Systems"),
    _T("Graychip"),
    _T("GENNUM"),
    _T("Imagination Technologies Limited"),
    _T("Robert Bosch"),
    _T("Chip Express"),
    _T("DATARAM"),
    _T("United Microelectronics Corp."),
    _T("TCSI"),
    _T("Smart Modular"),
    _T("Hughes Aircraft"),
    _T("Lanstar Semiconductor"),
    _T("Qlogic"),
    _T("Kingston"),
    _T("Music Semi"),
    _T("Ericsson Components"),
    _T("SpaSE"),
    _T("Eon Silicon Devices"),
    _T("Integrated Silicon Solution (ISSI)"),
    _T("DoD"),
    _T("Integ. Memories Tech."),
    _T("Corollary Inc."),
    _T("Dallas Semiconductor"),
    _T("Omnivision"),
    _T("EIV(Switzerland)"),
    _T("Novatel Wireless"),
    _T("Zarlink (Mitel)"),
    _T("Clearpoint"),
    _T("Cabletron"),
    _T("STEC (Silicon Tech)"),
    _T("Vanguard"),
    _T("Hagiwara Sys-Com"),
    _T("Vantis"),
    _T("Celestica"),
    _T("Century"),
    _T("Hal Computers"),
    _T("Rohm Company Ltd"),
    _T("Juniper Networks"),
    _T("Libit Signal Processing"),
    _T("Mushkin Enhanced Memory"),
    _T("Tundra Semiconductor"),
    _T("Adaptec Inc."),
    _T("LightSpeed Semi."),
    _T("ZSP Corp."),
    _T("AMIC Technology"),
    _T("Adobe Systems"),
    _T("Dynachip"),
    _T("PNY Technologies Inc"),
    _T("Newport Digital"),
    _T("MMC Networks"),
    _T("T Square"),
    _T("Seiko Epson"),
    _T("Broadcom"),
    _T("Viking Components"),
    _T("V3 Semiconductor"),
    _T("Flextronics (Orbit Semiconductor)"),
    _T("Suwa Electronics"),
    _T("Transmeta"),
    _T("Micron CMS"),
    _T("American Computer & Digital Components Inc"),
    _T("Enhance"),
    _T("Tower Semiconductor"),
    _T("CPU Design"),
    _T("Price Point"),
    _T("Maxim Integrated Product"),
    _T("Tellabs"),
    _T("Centaur Technology"),
    _T("Unigen Corporation"),
    _T("Transcend Information"),
    _T("Memory Card Technology"),
    _T("CKD Corporation Ltd"),
    _T("Capital Instruments Inc"),
    _T("Aica Kogyo, Ltd"),
    _T("Linvex Technology"),
    _T("MSC Vertriebs GmbH"),
    _T("AKM Company, Ltd"),
    _T("Dynamem Inc"),
    _T("NERA ASA"),
    _T("GSI Technology"),
    _T("Dane-Elec (C Memory)"),
    _T("Acorn Computers"),
    _T("Lara Technology"),
    _T("Oak Technology Inc"),
    _T("Itec Memory"),
    _T("Tanisys Technology"),
    _T("Truevision"),
    _T("Wintec Industries"),
    _T("Super PC Memory"),
    _T("MGV Memory"),
    _T("Galvantech"),
    _T("Gadzoox Networks"),
    _T("Multi Dimensional Cons."),
    _T("GateField"),
    _T("Integrated Memory System"),
    _T("Triscend"),
    _T("XaQti"),
    _T("Goldenram"),
    _T("Clear Logic"),
    _T("Cimaron Communications"),
    _T("Nippon Steel Semi. Corp."),
    _T("Advantage Memory"),
    _T("AMCC"),
    _T("LeCroy"),
    _T("Yamaha Corporation"),
    _T("Digital Microwave"),
    _T("NetLogic Microsystems"),
    _T("MIMOS Semiconductor"),
    _T("Advanced Fibre"),
    _T("BF Goodrich Data."),
    _T("Epigram"),
    _T("Acbel Polytech Inc."),
    _T("Apacer Technology"),
    _T("Admor Memory"),
    _T("FOXCONN"),
    _T("Quadratics Superconductor"),
    _T("3COM"),
};

static const wchar_t *BANK3_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("Camintonn Corporation"),
    _T("ISOA Incorporated"),
    _T("Agate Semiconductor"),
    _T("ADMtek Incorporated"),
    _T("HYPERTEC"),
    _T("Adhoc Technologies"),
    _T("MOSAID Technologies"),
    _T("Ardent Technologies"),
    _T("Switchcore"),
    _T("Cisco Systems Inc"),
    _T("Allayer Technologies"),
    _T("WorkX AG (Wichman)"),
    _T("Oasis Semiconductor"),
    _T("Novanet Semiconductor"),
    _T("E-M Solutions"),
    _T("Power General"),
    _T("Advanced Hardware Arch."),
    _T("Inova Semiconductors GmbH"),
    _T("Telocity"),
    _T("Delkin Devices"),
    _T("Symagery Microsystems"),
    _T("C-Port Corporation"),
    _T("SiberCore Technologies"),
    _T("Southland Microsystems"),
    _T("Malleable Technologies"),
    _T("Kendin Communications"),
    _T("Great Technology Microcomputer"),
    _T("Sanmina Corporation"),
    _T("HADCO Corporation"),
    _T("Corsair"),
    _T("Actrans System Inc."),
    _T("ALPHA Technologies"),
    _T("Silicon Laboratories Inc (Cygnal)"),
    _T("Artesyn Technologies"),
    _T("Align Manufacturing"),
    _T("Peregrine Semiconductor"),
    _T("Chameleon Systems"),
    _T("Aplus Flash Technology"),
    _T("MIPS Technologies"),
    _T("Chrysalis ITS"),
    _T("ADTEC Corporation"),
    _T("Kentron Technologies"),
    _T("Win Technologies"),
    _T("Tezzaron Semiconductor"),
    _T("Extreme Packet Devices"),
    _T("RF Micro Devices"),
    _T("Siemens AG"),
    _T("Sarnoff Corporation"),
    _T("Itautec SA"),
    _T("Radiata Inc."),
    _T("Benchmark Elect. (AVEX)"),
    _T("Legend"),
    _T("SpecTek Incorporated"),
    _T("Hi/fn"),
    _T("Enikia Incorporated"),
    _T("SwitchOn Networks"),
    _T("AANetcom Incorporated"),
    _T("Micro Memory Bank"),
    _T("ESS Technology"),
    _T("Virata Corporation"),
    _T("Excess Bandwidth"),
    _T("West Bay Semiconductor"),
    _T("DSP Group"),
    _T("Newport Communications"),
    _T("Chip2Chip Incorporated"),
    _T("Phobos Corporation"),
    _T("Intellitech Corporation"),
    _T("Nordic VLSI ASA"),
    _T("Ishoni Networks"),
    _T("Silicon Spice"),
    _T("Alchemy Semiconductor"),
    _T("Agilent Technologies"),
    _T("Centillium Communications"),
    _T("W.L. Gore"),
    _T("HanBit Electronics"),
    _T("GlobeSpan"),
    _T("Element"),
    _T("Pycon"),
    _T("Saifun Semiconductors"),
    _T("Sibyte, Incorporated"),
    _T("MetaLink Technologies"),
    _T("Feiya Technology"),
    _T("I & C Technology"),
    _T("Shikatronics"),
    _T("Elektrobit"),
    _T("Megic"),
    _T("Com-Tier"),
    _T("Malaysia Micro Solutions"),
    _T("Hyperchip"),
    _T("Gemstone Communications"),
    _T("Anadigm (Anadyne)"),
    _T("3ParData"),
    _T("Mellanox Technologies"),
    _T("Tenx Technologies"),
    _T("Helix AG"),
    _T("Domosys"),
    _T("Skyup Technology"),
    _T("HiNT Corporation"),
    _T("Chiaro"),
    _T("MDT Technologies GmbH"),
    _T("Exbit Technology A/S"),
    _T("Integrated Technology Express"),
    _T("AVED Memory"),
    _T("Legerity"),
    _T("Jasmine Networks"),
    _T("Caspian Networks"),
    _T("nCUBE"),
    _T("Silicon Access Networks"),
    _T("FDK Corporation"),
    _T("High Bandwidth Access"),
    _T("MultiLink Technology"),
    _T("BRECIS"),
    _T("World Wide Packets"),
    _T("APW"),
    _T("Chicory Systems"),
    _T("Xstream Logic"),
    _T("Fast-Chip"),
    _T("Zucotto Wireless"),
    _T("Realchip"),
    _T("Galaxy Power"),
    _T("eSilicon"),
    _T("Morphics Technology"),
    _T("Accelerant Networks"),
    _T("Silicon Wave"),
    _T("SandCraft"),
    _T("Elpida")};

static const wchar_t *BANK4_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("Solectron"),
    _T("Optosys Technologies"),
    _T("Buffalo (Formerly Melco)"),
    _T("TriMedia Technologies"),
    _T("Cyan Technologies"),
    _T("Global Locate"),
    _T("Optillion"),
    _T("Terago Communications"),
    _T("Ikanos Communications"),
    _T("Princeton Technology"),
    _T("Nanya Technology"),
    _T("Elite Flash Storage"),
    _T("Mysticom"),
    _T("LightSand Communications"),
    _T("ATI Technologies"),
    _T("Agere Systems"),
    _T("NeoMagic"),
    _T("AuroraNetics"),
    _T("Golden Empire"),
    _T("Mushkin"),
    _T("Tioga Technologies"),
    _T("Netlist"),
    _T("TeraLogic"),
    _T("Cicada Semiconductor"),
    _T("Centon Electronics"),
    _T("Tyco Electronics"),
    _T("Magis Works"),
    _T("Zettacom"),
    _T("Cogency Semiconductor"),
    _T("Chipcon AS"),
    _T("Aspex Technology"),
    _T("F5 Networks"),
    _T("Programmable Silicon Solutions"),
    _T("ChipWrights"),
    _T("Acorn Networks"),
    _T("Quicklogic"),
    _T("Kingmax Semiconductor"),
    _T("BOPS"),
    _T("Flasys"),
    _T("BitBlitz Communications"),
    _T("eMemory Technology"),
    _T("Procket Networks"),
    _T("Purple Ray"),
    _T("Trebia Networks"),
    _T("Delta Electronics"),
    _T("Onex Communications"),
    _T("Ample Communications"),
    _T("Memory Experts Intl"),
    _T("Astute Networks"),
    _T("Azanda Network Devices"),
    _T("Dibcom"),
    _T("Tekmos"),
    _T("API NetWorks"),
    _T("Bay Microsystems"),
    _T("Firecron Ltd"),
    _T("Resonext Communications"),
    _T("Tachys Technologies"),
    _T("Equator Technology"),
    _T("Concept Computer"),
    _T("SILCOM"),
    _T("3Dlabs"),
    _T("c’t Magazine"),
    _T("Sanera Systems"),
    _T("Silicon Packets"),
    _T("Viasystems Group"),
    _T("Simtek"),
    _T("Semicon Devices Singapore"),
    _T("Satron Handelsges"),
    _T("Improv Systems"),
    _T("INDUSYS GmbH"),
    _T("Corrent"),
    _T("Infrant Technologies"),
    _T("Ritek Corp"),
    _T("empowerTel Networks"),
    _T("Hypertec"),
    _T("Cavium Networks"),
    _T("PLX Technology"),
    _T("Massana Design"),
    _T("Intrinsity"),
    _T("Valence Semiconductor"),
    _T("Terawave Communications"),
    _T("IceFyre Semiconductor"),
    _T("Primarion"),
    _T("Picochip Designs Ltd"),
    _T("Silverback Systems"),
    _T("Jade Star Technologies"),
    _T("Pijnenburg Securealink"),
    _T("takeMS - Ultron AG"),
    _T("Cambridge Silicon Radio"),
    _T("Swissbit"),
    _T("Nazomi Communications"),
    _T("eWave System"),
    _T("Rockwell Collins"),
    _T("Picocel Co Ltd (Paion)"),
    _T("Alphamosaic Ltd"),
    _T("Sandburst"),
    _T("SiCon Video"),
    _T("NanoAmp Solutions"),
    _T("Ericsson Technology"),
    _T("PrairieComm"),
    _T("Mitac International"),
    _T("Layer N Networks"),
    _T("MtekVision (Atsana)"),
    _T("Allegro Networks"),
    _T("Marvell Semiconductors"),
    _T("Netergy Microelectronic"),
    _T("NVIDIA"),
    _T("Internet Machines"),
    _T("Memorysolution GmbH"),
    _T("Litchfield Communication"),
    _T("Accton Technology"),
    _T("Teradiant Networks"),
    _T("Scaleo Chip"),
    _T("Cortina Systems"),
    _T("RAM Components"),
    _T("Raqia Networks"),
    _T("ClearSpeed"),
    _T("Matsushita Battery"),
    _T("Xelerated"),
    _T("SimpleTech"),
    _T("Utron Technology"),
    _T("Astec International"),
    _T("AVM gmbH"),
    _T("Redux Communications"),
    _T("Dot Hill Systems"),
    _T("TeraChip")};

static const wchar_t *BANK5_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("T-RAM Incorporated"),
    _T("Innovics Wireless"),
    _T("Teknovus"),
    _T("KeyEye Communications"),
    _T("Runcom Technologies"),
    _T("RedSwitch"),
    _T("Dotcast"),
    _T("Silicon Mountain Memory"),
    _T("Signia Technologies"),
    _T("Pixim"),
    _T("Galazar Networks"),
    _T("White Electronic Designs"),
    _T("Patriot Scientific"),
    _T("Neoaxiom Corporation"),
    _T("3Y Power Technology"),
    _T("Scaleo Chip"),
    _T("Potentia Power Systems"),
    _T("C-guys Incorporated"),
    _T("Digital Communications Technology Incorporated"),
    _T("Silicon-Based Technology"),
    _T("Fulcrum Microsystems"),
    _T("Positivo Informatica Ltd"),
    _T("XIOtech Corporation"),
    _T("PortalPlayer"),
    _T("Zhiying Software"),
    _T("ParkerVision Inc"),
    _T("Phonex Broadband"),
    _T("Skyworks Solutions"),
    _T("Entropic Communications"),
    _T("I'M Intelligent Memory Ltd"),
    _T("Zensys A/S"),
    _T("Legend Silicon Corp."),
    _T("Sci-worx GmbH"),
    _T("SMSC (Standard Microsystems)"),
    _T("Renesas Electronics"),
    _T("Raza Microelectronics"),
    _T("Phyworks"),
    _T("MediaTek"),
    _T("Non-cents Productions"),
    _T("US Modular"),
    _T("Wintegra Ltd"),
    _T("Mathstar"),
    _T("StarCore"),
    _T("Oplus Technologies"),
    _T("Mindspeed"),
    _T("Just Young Computer"),
    _T("Radia Communications"),
    _T("OCZ"),
    _T("Emuzed"),
    _T("LOGIC Devices"),
    _T("Inphi Corporation"),
    _T("Quake Technologies"),
    _T("Vixel"),
    _T("SolusTek"),
    _T("Kongsberg Maritime"),
    _T("Faraday Technology"),
    _T("Altium Ltd"),
    _T("Insyte"),
    _T("ARM Ltd"),
    _T("DigiVision"),
    _T("Vativ Technologies"),
    _T("Endicott Interconnect Technologies"),
    _T("Pericom"),
    _T("Bandspeed"),
    _T("LeWiz Communications"),
    _T("CPU Technology"),
    _T("Ramaxel Technology"),
    _T("DSP Group"),
    _T("Axis Communications"),
    _T("Legacy Electronics"),
    _T("Chrontel"),
    _T("Powerchip Semiconductor"),
    _T("MobilEye Technologies"),
    _T("Excel Semiconductor"),
    _T("A-DATA Technology"),
    _T("VirtualDigm"),
    _T("G Skill Intl"),
    _T("Quanta Computer"),
    _T("Yield Microelectronics"),
    _T("Afa Technologies"),
    _T("KINGBOX Technology Co Ltd"),
    _T("Ceva"),
    _T("iStor Networks"),
    _T("Advance Modules"),
    _T("Microsoft"),
    _T("Open-Silicon"),
    _T("Goal Semiconductor"),
    _T("ARC International"),
    _T("Simmtec"),
    _T("Metanoia"),
    _T("Key Stream"),
    _T("Lowrance Electronics"),
    _T("Adimos"),
    _T("SiGe Semiconductor"),
    _T("Fodus Communications"),
    _T("Credence Systems Corp."),
    _T("Genesis Microchip Inc."),
    _T("Vihana Inc"),
    _T("WIS Technologies"),
    _T("GateChange Technologies"),
    _T("High Density Devices AS"),
    _T("Synopsys"),
    _T("Gigaram"),
    _T("Enigma Semiconductor Inc."),
    _T("Century Micro Inc."),
    _T("Icera Semiconductor"),
    _T("Mediaworks Integrated Systems"),
    _T("O’Neil Product Development"),
    _T("Supreme Top Technology Ltd"),
    _T("MicroDisplay Corporation"),
    _T("Team Group Inc."),
    _T("Sinett Corporation"),
    _T("Toshiba Corporation"),
    _T("Tensilica"),
    _T("SiRF Technology"),
    _T("Bacoc Inc."),
    _T("SMaL Camera Technologies"),
    _T("Thomson SC"),
    _T("Airgo Networks"),
    _T("Wisair Ltd"),
    _T("SigmaTel"),
    _T("Arkados"),
    _T("Compete IT gmbH Co KG"),
    _T("Eudar Technology Inc."),
    _T("Focus Enhancements"),
    _T("Xyratex")};

static const wchar_t *BANK6_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("Specular Networks"),
    _T("Patriot Memory (PDP Systems)"),
    _T("U-Chip Technology Corp."),
    _T("Silicon Optix"),
    _T("Greenfield Networks"),
    _T("CompuRAM GmbH"),
    _T("Stargen Inc"),
    _T("NetCell Corporation"),
    _T("Excalibrus Technologies Ltd"),
    _T("SCM Microsystems"),
    _T("Xsigo Systems Inc"),
    _T("CHIPS & Systems Inc"),
    _T("TierMultichip Solutions"),
    _T("CWRL Labs"),
    _T("Teradici"),
    _T("Gigaram Inc"),
    _T("gMicrosystems"),
    _T("PowerFlash Semiconductor"),
    _T("P.A. Semi Inc"),
    _T("NovaTech Solutions, S.A."),
    _T("cMicrosystems Inc"),
    _T("LevelNetworks"),
    _T("COS Memory AG"),
    _T("Innovasic Semiconductor"),
    _T("02IC Co Ltd"),
    _T("Tabula Inc"),
    _T("Crucial Technology"),
    _T("Chelsio Communications"),
    _T("Solarflare Communications"),
    _T("Xambala Inc."),
    _T("EADS Astrium"),
    _T("Terra Semiconductor Inc"),
    _T("Imaging Works Inc"),
    _T("Astute Networks Inc"),
    _T("Tzero"),
    _T("Emulex"),
    _T("Power-One"),
    _T("Pulse~LINK Inc."),
    _T("Hon Hai Precision Industry"),
    _T("White Rock Networks Inc."),
    _T("Telegent Systems USA Inc"),
    _T("Atrua Technologies Inc"),
    _T("Acbel Polytech Inc."),
    _T("eRide Inc."),
    _T("ULi Electronics Inc."),
    _T("Magnum Semiconductor Inc."),
    _T("neoOne Technology Inc"),
    _T("Connex Technology Inc"),
    _T("Stream Processors Inc"),
    _T("Focus Enhancements"),
    _T("Telecis Wireless Inc"),
    _T("uNav Microelectronics"),
    _T("Tarari Inc"),
    _T("Ambric Inc"),
    _T("Newport Media Inc"),
    _T("VMTS"),
    _T("Enuclia Semiconductor Inc"),
    _T("Virtium Technology Inc."),
    _T("Solid State System Co Ltd"),
    _T("Kian Tech LLC"),
    _T("Artimi"),
    _T("Power Quotient International"),
    _T("Avago Technologies"),
    _T("ADTechnology"),
    _T("Sigma Designs"),
    _T("SiCortex Inc"),
    _T("Ventura Technology Group"),
    _T("eASIC"),
    _T("M.H.S. SAS"),
    _T("Micro Star International"),
    _T("Rapport Inc."),
    _T("Makway International"),
    _T("Broad Reach Engineering Co"),
    _T("Semiconductor Mfg Intl Corp"),
    _T("SiConnect"),
    _T("FCI USA Inc."),
    _T("Validity Sensors"),
    _T("Coney Technology Co Ltd"),
    _T("Spans Logic"),
    _T("Neterion Inc."),
    _T("Qimonda"),
    _T("New Japan Radio Co Ltd"),
    _T("Velogix"),
    _T("Montalvo Systems"),
    _T("iVivity Inc."),
    _T("Walton Chaintech"),
    _T("AENEON"),
    _T("Lorom Industrial Co Ltd"),
    _T("Radiospire Networks"),
    _T("Sensio Technologies Inc"),
    _T("Nethra Imaging"),
    _T("Hexon Technology Pte Ltd"),
    _T("CompuStocx (CSX)"),
    _T("Methode Electronics Inc"),
    _T("Connect One Ltd"),
    _T("Opulan Technologies"),
    _T("Septentrio NV"),
    _T("Goldenmars Technology Inc."),
    _T("Kreton Corporation"),
    _T("Cochlear Ltd"),
    _T("Altair Semiconductor"),
    _T("NetEffect Inc"),
    _T("Spansion Inc"),
    _T("Taiwan Semiconductor Mfg"),
    _T("Emphany Systems Inc."),
    _T("ApaceWave Technologies"),
    _T("Mobilygen Corporation"),
    _T("Tego"),
    _T("Cswitch Corporation"),
    _T("Haier (Beijing) IC Design Co"),
    _T("MetaRAM"),
    _T("Axel Electronics Co Ltd"),
    _T("Tilera Corporation"),
    _T("Aquantia"),
    _T("Vivace Semiconductor"),
    _T("Redpine Signals"),
    _T("Octalica"),
    _T("InterDigital Communications"),
    _T("Avant Technology"),
    _T("Asrock Inc"),
    _T("Availink"),
    _T("Quartics Inc"),
    _T("Element CXI"),
    _T("Innovaciones Microelectronicas"),
    _T("VeriSilicon Microelectronics"),
    _T("W5 Networks")};

static const wchar_t *BANK7_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("MOVEKING"),
    _T("Mavrix Technology Inc"),
    _T("CellGuide Ltd"),
    _T("Faraday Technology"),
    _T("Diablo Technologies Inc"),
    _T("Jennic"),
    _T("Octasic"),
    _T("Molex Incorporated"),
    _T("3Leaf Networks"),
    _T("Bright Micron Technology"),
    _T("Netxen"),
    _T("NextWave Broadband Inc."),
    _T("DisplayLink"),
    _T("ZMOS Technology"),
    _T("Tec-Hill"),
    _T("Multigig Inc"),
    _T("Amimon"),
    _T("Euphonic Technologies Inc"),
    _T("BRN Phoenix"),
    _T("InSilica"),
    _T("Ember Corporation"),
    _T("Avexir Technologies Corporation"),
    _T("Echelon Corporation"),
    _T("Edgewater Computer Systems"),
    _T("XMOS Semiconductor Ltd"),
    _T("GENUSION Inc"),
    _T("Memory Corp NV"),
    _T("SiliconBlue Technologies"),
    _T("Rambus Inc."),
    _T("Andes Technology Corporation"),
    _T("Coronis Systems"),
    _T("Achronix Semiconductor"),
    _T("Siano Mobile Silicon Ltd"),
    _T("Semtech Corporation"),
    _T("Pixelworks Inc."),
    _T("Gaisler Research AB"),
    _T("Teranetics"),
    _T("Toppan Printing Co Ltd"),
    _T("Kingxcon"),
    _T("Silicon Integrated Systems"),
    _T("I-O Data Device Inc"),
    _T("NDS Americas Inc."),
    _T("Solomon Systech Limited"),
    _T("On Demand Microelectronics"),
    _T("Amicus Wireless Inc."),
    _T("SMARDTV SNC"),
    _T("Comsys Communication Ltd"),
    _T("Movidia Ltd"),
    _T("Javad GNSS Inc"),
    _T("Montage Technology Group"),
    _T("Trident Microsystems"),
    _T("Super Talent"),
    _T("Optichron Inc"),
    _T("Future Waves UK Ltd"),
    _T("SiBEAM Inc"),
    _T("Inicore,Inc."),
    _T("Virident Systems"),
    _T("M2000 Inc"),
    _T("ZeroG Wireless Inc"),
    _T("Gingle Technology Co Ltd"),
    _T("Space Micro Inc."),
    _T("Wilocity"),
    _T("Novafora, Ic."),
    _T("iKoa Corporation"),
    _T("ASint Technology"),
    _T("Ramtron"),
    _T("Plato Networks Inc."),
    _T("IPtronics AS"),
    _T("Infinite-Memories"),
    _T("Parade Technologies Inc."),
    _T("Dune Networks"),
    _T("GigaDevice Semiconductor"),
    _T("Modu Ltd"),
    _T("CEITEC"),
    _T("Northrop Grumman"),
    _T("XRONET Corporation"),
    _T("Sicon Semiconductor AB"),
    _T("Atla Electronics Co Ltd"),
    _T("TOPRAM Technology"),
    _T("Silego Technology Inc."),
    _T("Kinglife"),
    _T("Ability Industries Ltd"),
    _T("Silicon Power Computer & Communications"),
    _T("Augusta Technology Inc"),
    _T("Nantronics Semiconductors"),
    _T("Hilscher Gesellschaft"),
    _T("Quixant Ltd"),
    _T("Percello Ltd"),
    _T("NextIO Inc."),
    _T("Scanimetrics Inc."),
    _T("FS-Semi Company Ltd"),
    _T("Infinera Corporation"),
    _T("SandForce Inc."),
    _T("Lexar Media"),
    _T("Teradyne Inc."),
    _T("Memory Exchange Corp."),
    _T("Suzhou Smartek Electronics"),
    _T("Avantium Corporation"),
    _T("ATP Electronics Inc."),
    _T("Valens Semiconductor Ltd"),
    _T("Agate Logic Inc"),
    _T("Netronome"),
    _T("Zenverge Inc"),
    _T("N-trig Ltd"),
    _T("SanMax Technologies Inc."),
    _T("Contour Semiconductor Inc."),
    _T("TwinMOS"),
    _T("Silicon Systems Inc"),
    _T("V-Color Technology Inc."),
    _T("Certicom Corporation"),
    _T("JSC ICC Milandr"),
    _T("PhotoFast Global Inc."),
    _T("InnoDisk Corporation"),
    _T("Muscle Power"),
    _T("Energy Micro"),
    _T("Innofidei"),
    _T("CopperGate Communications"),
    _T("Holtek Semiconductor Inc."),
    _T("Myson Century Inc"),
    _T("FIDELIX"),
    _T("Red Digital Cinema"),
    _T("Densbits Technology"),
    _T("Zempro"),
    _T("MoSys"),
    _T("Provigent"),
    _T("Triad Semiconductor Inc")};

static const wchar_t *BANK8_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("Siklu Communication Ltd"),
    _T("A Force Manufacturing Ltd"),
    _T("Strontium"),
    _T("ALi Corp (Abilis Systems)"),
    _T("Siglead Inc"),
    _T("Ubicom Inc"),
    _T("Unifosa Corporation"),
    _T("Stretch Inc"),
    _T("Lantiq Deutschland GmbH"),
    _T("Visipro."),
    _T("EKMemory"),
    _T("Microelectronics Institute ZTE Corporation"),
    _T("u-blox AG"),
    _T("Carry Technology Co Ltd"),
    _T("Nokia"),
    _T("King Tiger Technology"),
    _T("Sierra Wireless"),
    _T("HT Micron"),
    _T("Albatron Technology Co Ltd"),
    _T("Leica Geosystems AG"),
    _T("BroadLight"),
    _T("AEXEA"),
    _T("ClariPhy Communications Inc"),
    _T("Green Plug"),
    _T("Design Art Networks"),
    _T("Mach Xtreme Technology Ltd"),
    _T("ATO Solutions Co Ltd"),
    _T("Ramsta"),
    _T("Greenliant Systems, Ltd"),
    _T("Teikon"),
    _T("Antec Hadron"),
    _T("NavCom Technology Inc"),
    _T("Shanghai Fudan Microelectronics"),
    _T("Calxeda Inc"),
    _T("JSC EDC Electronics"),
    _T("Kandit Technology Co Ltd"),
    _T("Ramos Technology"),
    _T("Goldenmars Technology"),
    _T("XeL Technology Inc."),
    _T("Newzone Corporation"),
    _T("ShenZhen MercyPower Tech"),
    _T("Nanjing Yihuo Technology"),
    _T("Nethra Imaging Inc."),
    _T("SiTel Semiconductor BV"),
    _T("SolidGear Corporation"),
    _T("Topower Computer Ind Co Ltd"),
    _T("Wilocity"),
    _T("Profichip GmbH"),
    _T("Gerad Technologies"),
    _T("Ritek Corporation"),
    _T("Gomos Technology Limited"),
    _T("Memoright Corporation"),
    _T("D-Broad Inc"),
    _T("HiSilicon Technologies"),
    _T("Syndiant Inc."),
    _T("Enverv Inc."),
    _T("Cognex"),
    _T("Xinnova Technology Inc."),
    _T("Ultron AG"),
    _T("Concord Idea Corporation"),
    _T("AIM Corporation"),
    _T("Lifetime Memory Products"),
    _T("Ramsway"),
    _T("Recore Systems B.V."),
    _T("Haotian Jinshibo Science Tech"),
    _T("Being Advanced Memory"),
    _T("Adesto Technologies"),
    _T("Giantec Semiconductor Inc"),
    _T("HMD Electronics AG"),
    _T("Gloway International (HK)"),
    _T("Kingcore"),
    _T("Anucell Technology Holding"),
    _T("Accord Software & Systems Pvt. Ltd"),
    _T("Active-Semi Inc."),
    _T("Denso Corporation"),
    _T("TLSI Inc."),
    _T("Qidan"),
    _T("Mustang"),
    _T("Orca Systems"),
    _T("Passif Semiconductor"),
    _T("GigaDevice Semiconductor (Beijing"),
    _T("Memphis Electronic"),
    _T("Beckhoff Automation GmbH"),
    _T("Harmony Semiconductor Corp"),
    _T("Air Computers SRL"),
    _T("TMT Memory"),
    _T("Eorex Corporation"),
    _T("Xingtera"),
    _T("Netsol"),
    _T("Bestdon Technology Co Ltd"),
    _T("Baysand Inc."),
    _T("Uroad Technology Co Ltd"),
    _T("Wilk Elektronik S.A."),
    _T("AAI"),
    _T("Harman"),
    _T("Berg Microelectronics Inc."),
    _T("ASSIA Inc"),
    _T("Visiontek Products LLC"),
    _T("OCMEMORY"),
    _T("Welink Solution Inc."),
    _T("Shark Gaming"),
    _T("Avalanche Technology"),
    _T("R&D Center ELVEES OJSC"),
    _T("KingboMars Technology Co Ltd"),
    _T("High Bridge Solutions Industri"),
    _T("Transcend Technology Co Ltd"),
    _T("Everspin Technologies"),
    _T("Hon-Hai Precision"),
    _T("Smart Storage Systems"),
    _T("Toumaz Group"),
    _T("Zentel Electronics Corporation"),
    _T("Panram International Corporation"),
    _T("Silicon Space Technology"),
    _T("LITE-ON IT Corporation"),
    _T("Inuitive"),
    _T("HMicro"),
    _T("BittWare Inc"),
    _T("GLOBALFOUNDRIES"),
    _T("ACPI Digital Co Ltd"),
    _T("Annapurna Labs"),
    _T("AcSiP Technology Corporation"),
    _T("Idea! Electronic Systems"),
    _T("Gowe Technology Co Ltd"),
    _T("Hermes Testing Solutions Inc"),
    _T("Positivo BGH"),
    _T("Intelligence Silicon Technology")};

static const wchar_t *BANK9_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("3D PLUS"),
    _T("Diehl Aerospace"),
    _T("Fairchild"),
    _T("Mercury Systems"),
    _T("Sonics Inc"),
    _T("Emerson Automation Solutions"),
    _T("Shenzhen Jinge Information Co Ltd"),
    _T("SCWW"),
    _T("Silicon Motion Inc."),
    _T("Anurag"),
    _T("King Kong"),
    _T("FROM30 Co Ltd"),
    _T("Gowin Semiconductor Corp"),
    _T("Fremont Micro Devices Ltd"),
    _T("Ericsson Modems"),
    _T("Exelis"),
    _T("Satixfy Ltd"),
    _T("Galaxy Microsystems Ltd"),
    _T("Gloway International Co Ltd"),
    _T("Lab"),
    _T("Smart Energy Instruments"),
    _T("Approved Memory Corporation"),
    _T("Axell Corporation"),
    _T("Essencore Limited"),
    _T("Phytium"),
    _T("UniIC Semiconductors Co Ltd"),
    _T("Ambiq Micro"),
    _T("eveRAM Technology Inc"),
    _T("Infomax"),
    _T("Butterfly Network Inc"),
    _T("Shenzhen City Gcai Electronics"),
    _T("Stack Devices Corporation"),
    _T("ADK Media Group"),
    _T("TSP Global Co Ltd"),
    _T("HighX"),
    _T("Shenzhen Elicks Technology"),
    _T("XinKai/Silicon Kaiser"),
    _T("Google Inc"),
    _T("Dasima International Development"),
    _T("Leahkinn Technology Limited"),
    _T("HIMA Paul Hildebrandt GmbH Co KG"),
    _T("Keysight Technologies"),
    _T("Techcomp International (Fastable)"),
    _T("Ancore Technology Corporation"),
    _T("Nuvoton"),
    _T("Korea Uhbele International Group Ltd"),
    _T("Ikegami Tsushinki Co Ltd"),
    _T("RelChip Inc"),
    _T("Baikal Electronics"),
    _T("Nemostech Inc."),
    _T("Memorysolution GmbH"),
    _T("Silicon Integrated Systems Corporation"),
    _T("Xiede"),
    _T("BRC"),
    _T("Flash Chi"),
    _T("Jone"),
    _T("GCT Semiconductor Inc."),
    _T("Hong Kong Zetta Device Technology"),
    _T("Unimemory Technology(s) Pte Ltd"),
    _T("Cuso"),
    _T("Kuso"),
    _T("Uniquify Inc."),
    _T("Skymedi Corporation"),
    _T("Core Chance Co Ltd"),
    _T("Tekism Co Ltd"),
    _T("Seagate Technology PLC"),
    _T("Hong Kong Gaia Group Co Limited"),
    _T("Gigacom Semiconductor LLC"),
    _T("V2 Technologies"),
    _T("TLi"),
    _T("Neotion"),
    _T("Lenovo"),
    _T("Shenzhen Zhongteng Electronic Corp. Ltd"),
    _T("Compound Photonics"),
    _T("In2H2 inc"),
    _T("Shenzhen Pango Microsystems Co Ltd"),
    _T("Vasekey"),
    _T("Cal-Comp Industria de Semicondutores"),
    _T("Eyenix Co Ltd"),
    _T("Heoriady"),
    _T("Accelerated Memory Production Inc."),
    _T("INVECAS Inc"),
    _T("AP Memory"),
    _T("Douqi Technology"),
    _T("Etron Technology Inc"),
    _T("Indie Semiconductor"),
    _T("Socionext Inc."),
    _T("HGST"),
    _T("EVGA"),
    _T("Audience Inc."),
    _T("EpicGear"),
    _T("Vitesse Enterprise Co"),
    _T("Foxtronn International Corporation"),
    _T("Bretelon Inc."),
    _T("Zbit Semiconductor Inc"),
    _T("Eoplex Inc"),
    _T("MaxLinear Inc"),
    _T("ETA Devices"),
    _T("LOKI"),
    _T("IMS Electronics Co Ltd"),
    _T("Dosilicon Co Ltd"),
    _T("Dolphin Integration"),
    _T("Shenzhen Mic Electronics Technology"),
    _T("Boya Microelectronics Inc."),
    _T("Geniachip (Roche)"),
    _T("Axign"),
    _T("Kingred Electronic Technology Ltd"),
    _T("Chao Yue Zhuo Computer Business Dept."),
    _T("Guangzhou Si Nuo Electronic Technology."),
    _T("Crocus Technology Inc."),
    _T("Creative Chips GmbH"),
    _T("GE Aviation Systems LLC."),
    _T("Asgard"),
    _T("Good Wealth Technology Ltd"),
    _T("TriCor Technologies"),
    _T("Nova-Systems GmbH"),
    _T("JUHOR"),
    _T("Zhuhai Douke Commerce Co Ltd"),
    _T("DSL Memory"),
    _T("Anvo-Systems Dresden GmbH"),
    _T("Realtek"),
    _T("AltoBeam"),
    _T("Wave Computing"),
    _T("Beijing TrustNet Technology Co Ltd"),
    _T("Innovium Inc."),
    _T("Starsway Technology Limited")};

static const wchar_t *BANK10_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("Weltronics Co LTD"),
    _T("VMware Inc"),
    _T("Hewlett Packard Enterprise"),
    _T("INTENSO"),
    _T("Puya Semiconductor"),
    _T("MEMORFI"),
    _T("MSC Technologies GmbH"),
    _T("Txrui"),
    _T("SiFive Inc"),
    _T("Spreadtrum Communications"),
    _T("XTX Technology Limited"),
    _T("UMAX Technology"),
    _T("Shenzhen Yong Sheng Technology"),
    _T("SNOAMOO (Shenzhen Kai Zhuo Yue)"),
    _T("Daten Tecnologia LTDA"),
    _T("Shenzhen XinRuiYan Electronics"),
    _T("Eta Compute"),
    _T("Energous"),
    _T("Raspberry Pi Trading Ltd"),
    _T("Shenzhen Chixingzhe Tech Co Ltd"),
    _T("Silicon Mobility"),
    _T("IQ-Analog Corporation"),
    _T("Uhnder Inc"),
    _T("Impinj"),
    _T("DEPO Computers"),
    _T("Nespeed Sysems"),
    _T("Yangtze Memory Technologies Co Ltd"),
    _T("MemxPro Inc."),
    _T("Tammuz Co Ltd"),
    _T("Allwinner Technology"),
    _T("Shenzhen City Futian District Qing Xuan Tong Computer Trading Firm"),
    _T("XMC"),
    _T("Teclast"),
    _T("Maxsun"),
    _T("Haiguang Integrated Circuit Design"),
    _T("RamCENTER Technology"),
    _T("Phison Electronics Corporation"),
    _T("Guizhou Huaxintong Semi-Conductor"),
    _T("Network Intelligence"),
    _T("Continental Technology (Holdings)"),
    _T("Guangzhou Huayan Suning Electronic"),
    _T("Guangzhou Zhouji Electronic Co Ltd"),
    _T("Shenzhen Giant Hui Kang Tech Co Ltd"),
    _T("Shenzhen Yilong Innovative Co Ltd"),
    _T("Neo Forza"),
    _T("Lyontek Inc"),
    _T("Shanghai Kuxin Microelectronics Ltd"),
    _T("Shenzhen Larix Technology Co Ltd"),
    _T("Qbit Semiconductor Ltd"),
    _T("Insignis Technology Corporation"),
    _T("Lanson Memory Co Ltd"),
    _T("Shenzhen Superway Electronics Co Ltd"),
    _T("Canaan - Creative Co Ltd"),
    _T("Black Diamond Memory"),
    _T("Shenzhen City Parker Baking Electronics"),
    _T("Shenzhen Baihong Technology Co Ltd"),
    _T("GEO Semiconductors"),
    _T("OCPC"),
    _T("Artery Technology Co Ltd"),
    _T("Jinyu"),
    _T("ShenzhenYing Chi Technology Development"),
    _T("Shenzhen Pengcheng Xin Technology"),
    _T("Pegasus Semiconductor (Shanghai) Co"),
    _T("Mythic Inc"),
    _T("Elmos Semiconductor AG"),
    _T("Kllisre"),
    _T("Shenzhen Winconway Technology"),
    _T("Shenzhen Xingmem Technology Corp"),
    _T("Gold Key Technology Co Ltd"),
    _T("Habana Labs Ltd"),
    _T("Hoodisk Electronics Co Ltd"),
    _T("SemsoTai (SZ) Technology Co Ltd"),
    _T("OM Nanotech Pvt.Ltd"),
    _T("Shenzhen Zhifeng Weiye Technology"),
    _T("Xinshirui (Shenzhen) Electronics Co"),
    _T("Guangzhou Zhong Hao Tian Electronic"),
    _T("Shenzhen Longsys Electronics Co Ltd"),
    _T("Deciso B.V."),
    _T("Puya Semiconductor (Shenzhen)"),
    _T("Shenzhen Veineda Technology Co Ltd"),
    _T("Antec Memory"),
    _T("Cortus SAS"),
    _T("Dust Leopard"),
    _T("MyWo AS"),
    _T("J&A Information Inc"),
    _T("Shenzhen JIEPEI Technology Co Ltd"),
    _T("Heidelberg University"),
    _T("Flexxon PTE Ltd"),
    _T("Wiliot"),
    _T("Raysun Electronics International Ltd"),
    _T("Aquarius Production Company LLC"),
    _T("MACNICA DHW LTDA"),
    _T("Intelimem"),
    _T("Zbit Semiconductor Inc"),
    _T("Shenzhen Technology Co Ltd"),
    _T("Signalchip"),
    _T("Shenzen Recadata Storage Technology"),
    _T("Hyundai Technology"),
    _T("Shanghai Fudi Investment Development"),
    _T("Aixi Technology"),
    _T("Tecon MT"),
    _T("Onda Electric Co Ltd"),
    _T("Jinshen"),
    _T("Kimtigo Semiconductor (HK) Limited"),
    _T("IIT Madras"),
    _T("Shenshan (Shenzhen) Electronic"),
    _T("Hefei Core Storage Electronic Limited"),
    _T("Colorful Technology Ltd"),
    _T("Visenta (Xiamen) Technology Co Ltd"),
    _T("Roa Logic BV"),
    _T("NSITEXE Inc"),
    _T("Hong Kong Hyunion Electronics"),
    _T("ASK Technology Group Limited"),
    _T("GIGA - BYTE Technology Co Ltd"),
    _T("Terabyte Co Ltd"),
    _T("Hyundai Inc"),
    _T("EXCELERAM"),
    _T("PsiKick"),
    _T("Netac Technology Co Ltd"),
    _T("PCCOOLER"),
    _T("Jiangsu Huacun Electronic Technology"),
    _T("Shenzhen Micro Innovation Industry"),
    _T("Beijing Tongfang Microelectronics Co"),
    _T("XZN Storage Technology"),
    _T("ChipCraft Sp.z.o.o,"),
    _T("ALLFLASH Technology Limited"),
};

static const wchar_t *BANK11_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("Foerd Technology Co Ltd"),
    _T("KingSpec"),
    _T("Codasip GmbH"),
    _T("SL Link Co Ltd"),
    _T("Shenzhen Kefu Technology Co Limited"),
    _T("Shenzhen ZST Electronics Technology"),
    _T("Kyokuto Electronic Inc"),
    _T("Warrior Technology"),
    _T("TRINAMIC Motion Control GmbH & Co"),
    _T("PixelDisplay Inc"),
    _T("Shenzhen Futian District Bo Yueda Elec"),
    _T("Richtek Power"),
    _T("Shenzhen LianTeng Electronics Co Ltd"),
    _T("AITC Memory"),
    _T("UNIC Memory Technology Co Ltd"),
    _T("Shenzhen Huafeng Science Technology"),
    _T("CXMT"),
    _T("Guangzhou Xinyi Heng Computer Trading Firm"),
    _T("SambaNova Systems"),
    _T("V-GEN"),
    _T("Jump Trading"),
    _T("Ampere Computing"),
    _T("Shenzhen Zhongshi Technology Co Ltd"),
    _T("Shenzhen Zhongtian Bozhong Technology"),
    _T("Tri - Tech International"),
    _T("Silicon Intergrated Systems Corporation"),
    _T("Shenzhen HongDingChen Information"),
    _T("Plexton Holdings Limited"),
    _T("AMS (Jiangsu Advanced Memory Semi)"),
    _T("Wuhan Jing Tian Interconnected Tech Co"),
    _T("Axia Memory Technology"),
    _T("Chipset Technology Holding Limited"),
    _T("Shenzhen Xinshida Technology Co Ltd"),
    _T("Shenzhen Chuangshifeida Technology"),
    _T("Guangzhou MiaoYuanJi Technology"),
    _T("ADVAN Inc"),
    _T("Shenzhen Qianhai Weishengda Electronic Commerce Company Ltd"),
    _T("Guangzhou Guang Xie Cheng Trading"),
    _T("StarRam International Co Ltd"),
    _T("Shen Zhen XinShenHua Tech Co Ltd"),
    _T("UltraMemory Inc"),
    _T("New Coastline Global Tech Industry Co"),
    _T("Sinker"),
    _T("Diamond"),
    _T("PUSKILL"),
    _T("Guangzhou Hao Jia Ye Technology Co"),
    _T("Ming Xin Limited"),
    _T("Barefoot Networks"),
    _T("Biwin Semiconductor (HK) Co Ltd"),
    _T("UD INFO Corporation"),
    _T("Trek Technology (S) PTE Ltd"),
    _T("Xiamen Kingblaze Technology Co Ltd"),
    _T("Shenzhen Lomica Technology Co Ltd"),
    _T("Nuclei System Technology Co Ltd"),
    _T("Wuhan Xun Zhan Electronic Technology"),
    _T("Shenzhen Ingacom Semiconductor Ltd"),
    _T("Zotac Technology Ltd"),
    _T("Foxline"),
    _T("Shenzhen Farasia Science Technology"),
    _T("Efinix Inc"),
    _T("Hua Nan San Xian Technology Co Ltd"),
    _T("Goldtech Electronics Co Ltd"),
    _T("Shanghai Han Rong Microelectronics Co"),
    _T("Shenzhen Zhongguang Yunhe Trading"),
    _T("Smart Shine(QingDao) Microelectronics"),
    _T("Thermaltake Technology Co Ltd"),
    _T("Shenzhen O’Yang Maile Technology Ltd"),
    _T("UPMEM"),
    _T("Chun Well Technology Holding Limited"),
    _T("Astera Labs Inc"),
    _T("Winconway"),
    _T("Advantech Co Ltd"),
    _T("Chengdu Fengcai Electronic Technology"),
    _T("The Boeing Company"),
    _T("Blaize Inc"),
    _T("Ramonster Technology Co Ltd"),
    _T("Wuhan Naonongmai Technology Co Ltd"),
    _T("Shenzhen Hui ShingTong Technology"),
    _T("Yourlyon"),
    _T("Fabu Technology"),
    _T("Shenzhen Yikesheng Technology Co Ltd"),
    _T("NOR-MEM"),
    _T("Cervoz Co Ltd"),
    _T("Bitmain Technologies Inc."),
    _T("Facebook Inc"),
    _T("Shenzhen Longsys Electronics Co Ltd"),
    _T("Guangzhou Siye Electronic Technology"),
    _T("Silergy"),
    _T("Adamway"),
    _T("PZG"),
    _T("Shenzhen King Power Electronics"),
    _T("Guangzhou ZiaoFu Tranding Co Ltd"),
    _T("Shenzhen SKIHOTAR Semiconductor"),
    _T("PulseRain Technology"),
    _T("Seeker Technology Limited"),
    _T("Shenzhen OSCOO Tech Co Ltd"),
    _T("Shenzhen Yze Technology Co Ltd"),
    _T("Shenzhen Jieshuo Electronic Commerce"),
    _T("Gazda"),
    _T("Hua Wei Technology Co Ltd"),
    _T("Esperanto Technologies"),
    _T("JinSheng Electronic(Shenzhen) Co Ltd"),
    _T("Shenzhen Shi Bolunshuai Technology"),
    _T("Shanghai Rei Zuan Information Tech"),
    _T("Fraunhofer IIS"),
    _T("Kandou Bus SA"),
    _T("Acer"),
    _T("Artmem Technology Co Ltd"),
    _T("Gstar Semiconductor Co Ltd"),
    _T("ShineDisk"),
    _T("Shenzhen CHN Technology Co Ltd"),
    _T("UnionChip Semiconductor Co Ltd"),
    _T("Tanbassh"),
    _T("Shenzhen Tianyu Jieyun Intl Logistics"),
    _T("MCLogic Inc"),
    _T("Eorex Corporation"),
    _T("Arm Technology(China) Co Ltd"),
    _T("Lexar Co Limited"),
    _T("QinetiQ Group plc"),
    _T("Exascend"),
    _T("Hong Kong Hyunion Electronics Co Ltd"),
    _T("Shenzhen Banghong Electronics Co Ltd"),
    _T("MBit Wireless Inc"),
    _T("Hex Five Security Inc"),
    _T("ShenZhen Juhor Precision Tech Co Ltd"),
    _T("Shenzhen Reeinno Technology Co Ltd"),
};

static const wchar_t *BANK12_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("ABIT Electronics(Shenzhen) Co Ltd"),
    _T("Semidrive"),
    _T("MyTek Electronics Corp"),
    _T("Wxilicon Technology Co Ltd"),
    _T("Shenzhen Meixin Electronics Ltd"),
    _T("Ghost Wolf"),
    _T("LiSion Technologies Inc"),
    _T("Power Active Co Ltd"),
    _T("Pioneer High Fidelity Taiwan Co. Ltd"),
    _T("LuoSilk"),
    _T("Shenzhen Chuangshifeida Technology"),
    _T("Black Sesame Technologies Inc"),
    _T("Jiangsu Xinsheng Intelligent Technology"),
    _T("MLOONG"),
    _T("Quadratica LLC"),
    _T("Anpec Electronics"),
    _T("Xi’an Morebeck Semiconductor Tech Co"),
    _T("Kingbank Technology Co Ltd"),
    _T("ITRenew Inc"),
    _T("Shenzhen Eaget Innovation Tech Ltd"),
    _T("Jazer"),
    _T("Xiamen Semiconductor Investment Group"),
    _T("Guangzhou Longdao Network Tech Co"),
    _T("Shenzhen Futian SEC Electronic Market"),
    _T("Allegro Microsystems LLC"),
    _T("Hunan RunCore Innovation Technology"),
    _T("C-Corsa Technology"),
    _T("Zhuhai Chuangfeixin Technology Co Ltd"),
    _T("Beijing InnoMem Technologies Co Ltd"),
    _T("YooTin"),
    _T("Shenzhen Pengxiong Technology Co Ltd"),
    _T("Dongguan Yingbang Commercial Trading Co"),
    _T("Shenzhen Ronisys Electronics Co Ltd"),
    _T("Hongkong Xinlan Guangke Co Ltd"),
    _T("Apex Microelectronics Co Ltd"),
    _T("Beijing Hongda Jinming Technology Co Ltd"),
    _T("Ling Rui Technology (Shenzhen) Co Ltd"),
    _T("Hongkong Hyunion Electronics Co Ltd"),
    _T("Starsystems Inc"),
    _T("Shenzhen Yingjiaxun Industrial Co Ltd"),
    _T("Dongguan Crown Code Electronic Commerce"),
    _T("Monolithic Power Systems Inc"),
    _T("WuHan SenNaiBo E-Commerce Co Ltd"),
    _T("Hangzhou Hikstorage Technology Co"),
    _T("Shenzhen Goodix Technology Co Ltd"),
    _T("Aigo Electronic Technology Co Ltd"),
    _T("Hefei Konsemi Storage Technology Co Ltd"),
    _T("Cactus Technologies Limited"),
    _T("DSIN"),
    _T("Blu Wireless Technology"),
    _T("Nanjing UCUN Technology Inc"),
    _T("Acacia Communications"),
    _T("Beijinjinshengyihe Technology Co Ltd"),
    _T("Zyzyx"),
    _T("C-SKY Microsystems Co Ltd"),
    _T("Shenzhen Hystou Technology Co Ltd"),
    _T("Syzexion"),
    _T("Kembona"),
    _T("Qingdao Thunderobot Technology Co Ltd"),
    _T("Morse Micro"),
    _T("Shenzhen Envida Technology Co Ltd"),
    _T("UDStore Solution Limited"),
    _T("Shunlie"),
    _T("Shenzhen Xin Hong Rui Tech Ltd"),
    _T("Shenzhen Yze Technology Co Ltd"),
    _T("Shenzhen Huang Pu He Xin Technology"),
    _T("Xiamen Pengpai Microelectronics Co Ltd"),
    _T("JISHUN"),
    _T("Shenzhen WODPOSIT Technology Co"),
    _T("Unistar"),
    _T("UNICORE Electronic (Suzhou) Co Ltd"),
    _T("Axonne Inc"),
    _T("Shenzhen SOVERECA Technology Co"),
    _T("Dire Wolf"),
    _T("Whampoa Core Technology Co Ltd"),
    _T("CSI Halbleiter GmbH"),
    _T("ONE Semiconductor"),
    _T("SimpleMachines Inc"),
    _T("Shenzhen Chengyi Qingdian Electronic"),
    _T("Shenzhen Xinlianxin Network Technology"),
    _T("Vayyar Imaging Ltd"),
    _T("Paisen Network Technology Co Ltd"),
    _T("Shenzhen Fengwensi Technology Co Ltd"),
    _T("Caplink Technology Limited"),
    _T("JJT Solution Co Ltd"),
    _T("HOSIN Global Electronics Co Ltd"),
    _T("Shenzhen KingDisk Century Technology"),
    _T("SOYO"),
    _T("DIT Technology Co Ltd"),
    _T("iFound"),
    _T("Aril Computer Company"),
    _T("ASUS"),
    _T("Shenzhen Ruiyingtong Technology Co"),
    _T("HANA Micron"),
    _T("RANSOR"),
    _T("Axiado Corporation"),
    _T("Tesla Corporation"),
    _T("Pingtouge (Shanghai) Semiconductor Co"),
    _T("S3Plus Technologies SA"),
    _T("Integrated Silicon Solution Israel Ltd"),
    _T("GreenWaves Technologies"),
    _T("NUVIA Inc"),
    _T("Guangzhou Shuvrwine Technology Co"),
    _T("Shenzhen Hangshun Chip Technology"),
    _T("Chengboliwei Electronic Business"),
    _T("Kowin Technology HK Limited"),
    _T("Euronet Technology Inc"),
    _T("SCY"),
    _T("Shenzhen Xinhongyusheng Electrical"),
    _T("PICOCOM"),
    _T("Shenzhen Toooogo Memory Technology"),
    _T("VLSI Solution"),
    _T("Costar Electronics Inc"),
    _T("Shenzhen Huatop Technology Co Ltd"),
    _T("Inspur Electronic Information Industry"),
    _T("Shenzhen Boyuan Computer Technology"),
    _T("Beijing Welldisk Electronics Co Ltd"),
    _T("Suzhou EP Semicon Co Ltd"),
    _T("Zhejiang Dahua Memory Technology"),
    _T("Virtu Financial"),
    _T("Datotek International Co Ltd"),
    _T("Telecom and Microelectronics Industries"),
    _T("Echow Technology Ltd"),
    _T("APEX-INFO"),
    _T("Yingpark"),
    _T("Shenzhen Bigway Tech Co Ltd"),
};

static const wchar_t *BANK13_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("Beijing Haawking Technology Co Ltd"),
    _T("Open HW Group"),
    _T("JHICC"),
    _T("ncoder AG"),
    _T("ThinkTech Information Technology Co"),
    _T("Shenzhen Chixingzhe Technology Co Ltd"),
    _T("Biao Ram Technology Co Ltd"),
    _T("Shenzhen Kaizhuoyue Electronics Co Ltd"),
    _T("Shenzhen YC Storage Technology Co Ltd"),
    _T("Shenzhen Chixingzhe Technology Co"),
    _T("Wink Semiconductor (Shenzhen) Co Ltd"),
    _T("AISTOR"),
    _T("Palma Ceia SemiDesign"),
    _T("EM Microelectronic-Marin SA"),
    _T("Shenzhen Monarch Memory Technology"),
    _T("Reliance Memory Inc"),
    _T("Jesis"),
    _T("Espressif Systems (Shanghai) Co Ltd"),
    _T("Shenzhen Sati Smart Technology Co Ltd"),
    _T("NeuMem Co Ltd"),
    _T("Lifelong"),
    _T("Beijing Oitech Technology Co Ltd"),
    _T("Groupe LDLC"),
    _T("Semidynamics Technology Services SLU"),
    _T("swordbill"),
    _T("YIREN"),
    _T("Shenzhen Yinxiang Technology Co Ltd"),
    _T("PoweV Electronic Technology Co Ltd"),
    _T("LEORICE"),
    _T("Waymo LLC"),
    _T("Ventana Micro Systems"),
    _T("Hefei Guangxin Microelectronics Co Ltd"),
    _T("Shenzhen Sooner Industrial Co Ltd"),
    _T("Horizon Robotics"),
    _T("Tangem AG"),
    _T("FuturePath Technology (Shenzhen) Co"),
    _T("RC Module"),
    _T("Timetec International Inc"),
    _T("ICMAX Technologies Co Limited"),
    _T("Lynxi Technologies Ltd Co"),
    _T("Guangzhou Taisupanke Computer Equipment"),
    _T("Ceremorphic Inc"),
    _T("Biwin Storage Technology Co Ltd"),
    _T("Beijing ESWIN Computing Technology"),
    _T("WeForce Co Ltd"),
    _T("Shenzhen Fanxiang Information Technology"),
    _T("Unisoc"),
    _T("YingChu"),
    _T("GUANCUN"),
    _T("IPASON"),
    _T("Ayar Labs"),
    _T("Amazon"),
    _T("Shenzhen Xinxinshun Technology Co"),
    _T("Galois Inc"),
    _T("Ubilite Inc"),
    _T("Shenzhen Quanxing Technology Co Ltd"),
    _T("Group RZX Technology LTDA"),
    _T("Yottac Technology (XI’AN) Cooperation"),
    _T("Shenzhen RuiRen Technology Co Ltd"),
    _T("Group Star Technology Co Ltd"),
    _T("RWA (Hong Kong) Ltd"),
    _T("Genesys Logic Inc"),
    _T("T3 Robotics Inc."),
    _T("Biostar Microtech International Corp"),
    _T("Shenzhen SXmicro Technology Co Ltd"),
    _T("Shanghai Yili Computer Technology Co"),
    _T("Zhixin Semicoducotor Co Ltd"),
    _T("uFound"),
    _T("Aigo Data Security Technology Co. Ltd"),
    _T(".GXore Technologies"),
    _T("Shenzhen Pradeon Intelligent Technology"),
    _T("Power LSI"),
    _T("PRIME"),
    _T("Shenzhen Juyang Innovative Technology"),
    _T("CERVO"),
    _T("SiEngine Technology Co., Ltd."),
    _T("Beijing Unigroup Tsingteng MicroSystem"),
    _T("Brainsao GmbH"),
    _T("Credo Technology Group Ltd"),
    _T("Shanghai Biren Technology Co Ltd"),
    _T("Nucleu Semiconductor"),
    _T("Shenzhen Guangshuo Electronics Co Ltd"),
    _T("ZhongsihangTechnology Co Ltd"),
    _T("Suzhou Mainshine Electronic Co Ltd."),
    _T("Guangzhou Riss Electronic Technology"),
    _T("Shenzhen Cloud Security Storage Co"),
    _T("ROG"),
    _T("Perceive"),
    _T("e-peas"),
    _T("Fraunhofer IPMS"),
    _T("Shenzhen Daxinlang Electronic Tech Co"),
    _T("Abacus Peripherals Private Limited"),
    _T("OLOy Technology"),
    _T("Wuhan P&S Semiconductor Co Ltd"),
    _T("Sitrus Technology"),
    _T("AnHui Conner Storage Co Ltd"),
    _T("Rochester Electronics"),
    _T("Wuxi Smart Memories Technologies Co"),
    _T("Star Memory"),
    _T("Agile Memory Technology Co Ltd"),
    _T("MEJEC"),
    _T("Rockchip Electronics Co Ltd"),
    _T("Dongguan Guanma e-commerce Co Ltd"),
    _T("Rayson Hi-Tech (SZ) Limited"),
    _T("MINRES Technologies GmbH"),
    _T("Himax Technologies Inc"),
    _T("Shenzhen Cwinner Technology Co Ltd"),
    _T("Tecmiyo"),
    _T("Shenzhen Suhuicun Technology Co Ltd"),
    _T("Vickter Electronics Co. Ltd."),
    _T("lowRISC"),
    _T("EXEGate FZE"),
    _T("Shenzhen 9 Chapter Technologies Co"),
    _T("Addlink"),
    _T("Starsway"),
    _T("Pensando Systems Inc."),
    _T("AirDisk"),
    _T("Shenzhen Speedmobile Technology Co"),
    _T("PEZY Computing"),
    _T("Extreme Engineering Solutions Inc"),
    _T("Shangxin Technology Co Ltd"),
    _T("Shanghai Zhaoxin Semiconductor Co"),
    _T("Xsight Labs Ltd"),
    _T("Hangzhou Hikstorage Technology Co"),
    _T("Dell Technologies"),
    _T("Guangdong StarFive Technology Co"),
};

static const wchar_t *BANK14_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("TECOTON"),
    _T("Abko Co Ltd"),
    _T("Shenzhen Feisrike Technology Co Ltd"),
    _T("Shenzhen Sunhome Electronics Co Ltd"),
    _T("Global Mixed-mode Technology Inc"),
    _T("Shenzhen Weien Electronics Co. Ltd."),
    _T("Shenzhen Cooyes Technology Co Ltd"),
    _T("ShenZhen ChaoYing ZhiNeng Technology"),
    _T("E-Rockic Technology Company Limited"),
    _T("Aerospace Science Memory Shenzhen"),
    _T("Shenzhen Quanji Technology Co Ltd"),
    _T("Dukosi"),
    _T("Maxell Corporation of America"),
    _T("Shenshen Xinxintao Electronics Co Ltd"),
    _T("Zhuhai Sanxia Semiconductor Co Ltd"),
    _T("Groq Inc"),
    _T("AstraTek"),
    _T("Shenzhen Xinyuze Technology Co Ltd"),
    _T("All Bit Semiconductor"),
    _T("ACFlow"),
    _T("Shenzhen Sipeed Technology Co Ltd"),
    _T("Linzhi Hong Kong Co Limited"),
    _T("Supreme Wise Limited"),
    _T("Blue Cheetah Analog Design Inc"),
    _T("Hefei Laiku Technology Co Ltd"),
    _T("Zord"),
    _T("SBO Hearing A/S"),
    _T("Regent Sharp International Limited"),
    _T("Permanent Potential Limited"),
    _T("Creative World International Limited"),
    _T("Base Creation International Limited"),
    _T("Shenzhen Zhixin Chuanglian Technology"),
    _T("Protected Logic Corporation"),
    _T("Sabrent"),
    _T("Union Memory"),
    _T("NEUCHIPS Corporation"),
    _T("Ingenic Semiconductor Co Ltd"),
    _T("SiPearl"),
    _T("Shenzhen Actseno Information Technology"),
    _T("RIVAI Technologies (Shenzhen) Co Ltd"),
    _T("Shenzhen Sunny Technology Co Ltd"),
    _T("Cott Electronics Ltd"),
    _T("Shanghai Synsense Technologies Co Ltd"),
    _T("Shenzhen Jintang Fuming Optoelectronics"),
    _T("CloudBEAR LLC"),
    _T("Emzior, LLC"),
    _T("Ehiway Microelectronic Science Tech Co"),
    _T("UNIM Innovation Technology (Wu XI)"),
    _T("GDRAMARS"),
    _T("Meminsights Technology"),
    _T("Zhuzhou Hongda Electronics Corp Ltd"),
    _T("Luminous Computing Inc"),
    _T("PROXMEM"),
    _T("Draper Labs"),
    _T("ORICO Technologies Co. Ltd."),
    _T("Space Exploration Technologies Corp"),
    _T("AONDEVICES Inc"),
    _T("Shenzhen Netforward Micro Electronic"),
    _T("Syntacore Ltd"),
    _T("Shenzhen Secmem Microelectronics Co"),
    _T("ONiO As"),
    _T("Shenzhen Peladn Technology Co Ltd"),
    _T("O-Cubes Shanghai Microelectronics"),
    _T("ASTC"),
    _T("UMIS"),
    _T("Paradromics"),
    _T("Sinh Micro Co Ltd"),
    _T("Metorage Semiconductor Technology Co"),
    _T("Aeva Inc"),
    _T("HongKong Hyunion Electronics Co Ltd"),
    _T("China Flash Co Ltd"),
    _T("Sunplus Technology Co Ltd"),
    _T("Idaho Scientific"),
    _T("Suzhou SF Micro Electronics Co Ltd"),
    _T("IMEX Cap AG"),
    _T("Fitipower Integrated Technology Co Ltd"),
    _T("ShenzhenWooacme Technology Co Ltd"),
    _T("KeepData Original Chips"),
    _T("Rivos Inc"),
    _T("Big Innovation Company Limited"),
    _T("Wuhan YuXin Semiconductor Co Ltd"),
    _T("United Memory Technology (Jiangsu)"),
    _T("PQShield Ltd"),
    _T("ArchiTek Corporation"),
    _T("ShenZhen AZW Technology Co Ltd"),
    _T("Hengchi Zhixin (Dongguan) Technology"),
    _T("Eggtronic Engineering Spa"),
    _T("Fusontai Technology"),
    _T("PULP Platform"),
    _T("Koitek Electronic Technology (Shenzhen) Co"),
    _T("Shenzhen Jiteng Network Technology Co"),
    _T("Aviva Links Inc"),
    _T("Trilinear Technologies Inc"),
    _T("Shenzhen Developer Microelectronics Co"),
    _T("Guangdong OPPO Mobile Telecommunication"),
    _T("Akeana"),
    _T("Lyczar"),
    _T("QJTEK"),
    _T("Shenzhen Shangzhaoyuan Technology"),
    _T("Han Stor"),
    _T("China Micro Semicon Co., Ltd."),
    _T("Shenzhen Zhuqin Technology Co Ltd"),
    _T("Shanghai Ningyuan Electronic Technology"),
    _T("Auradine"),
    _T("Suzhou Yishuo Electronics Co Ltd"),
    _T("Faurecia Clarion Electronics"),
    _T("SiMa Technologies"),
    _T("CFD Sales Inc"),
    _T("Suzhou Comay Information Co Ltd"),
    _T("Yentek"),
    _T("Qorvo Inc"),
    _T("Shenzhen Youzhi Computer Technology"),
    _T("Sychw Technology (Shenzhen) Co Ltd"),
    _T("MK Founder Technology Co Ltd"),
    _T("Siliconwaves Technologies Co Ltd"),
    _T("Hongkong Hyunion Electronics Co Ltd"),
    _T("Shenzhen Xinxinzhitao Electronics Business"),
    _T("Shenzhen HenQi Electronic Commerce Co"),
    _T("Shenzhen Jingyi Technology Co Ltd"),
    _T("Xiaohua Semiconductor Co. Ltd."),
    _T("Shenzhen Dalu Semiconductor Technology"),
    _T("Shenzhen Ninespeed Electronics Co Ltd"),
    _T("ICYC Semiconductor Co Ltd"),
    _T("Shenzhen Jaguar Microsystems Co Ltd"),
    _T("Beijing EC-Founder Co Ltd"),
    _T("Shenzhen Taike Industrial Automation Co"),
};

static const wchar_t *BANK15_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("Kalray SA"),
    _T("Shanghai Iluvatar CoreX Semiconductor Co"),
    _T("Fungible Inc"),
    _T("Song Industria E Comercio de Eletronicos"),
    _T("DreamBig Semiconductor Inc"),
    _T("ChampTek Electronics Corp"),
    _T("Fusontai Technology"),
    _T("Endress Hauser AG"),
    _T("altec ComputerSysteme GmbH"),
    _T("UltraRISC Technology (Shanghai) Co Ltd"),
    _T("Shenzhen Jing Da Kang Technology Co Ltd"),
    _T("Hangzhou Hongjun Microelectronics Co Ltd"),
    _T("Pliops Ltd"),
    _T("Cix Technology (Shanghai) Co Ltd"),
    _T("TeraDevices Inc"),
    _T("SpacemiT (Hangzhou)Technology Co Ltd"),
    _T("InnoPhase loT Inc"),
    _T("InnoPhase loT Inc"),
    _T("Yunhight Microelectronics"),
    _T("Samnix"),
    _T("HKC Storage Co Ltd"),
    _T("Chiplego Technology (Shanghai) Co Ltd"),
    _T("StoreSkill"),
    _T("Shenzhen Astou Technology Company"),
    _T("Guangdong LeafFive Technology Limited"),
    _T("Jin JuQuan"),
    _T("Huaxuan Technology (Shenzhen) Co Ltd"),
    _T("Gigastone Corporation"),
    _T("Kinsotin"),
    _T("PengYing"),
    _T("Shenzhen Xunhi Technology Co Ltd"),
    _T("FOXX Storage Inc"),
    _T("Shanghai Belling Corporation Ltd"),
    _T("Glenfy Tech Co Ltd"),
    _T("Sahasra Semiconductors Pvt Ltd"),
    _T("Chongqing SeekWave Technology Co Ltd"),
    _T("Shenzhen Zhixing Intelligent Manufacturing"),
    _T("Ethernovia"),
    _T("Shenzhen Xinrongda Technology Co Ltd"),
    _T("Hangzhou Clounix Technology Limited"),
    _T("JGINYUE"),
    _T("Shenzhen Xinwei Semiconductor Co Ltd"),
    _T("COLORFIRE Technology Co Ltd"),
    _T("B LKE"),
    _T("ZHUDIAN"),
    _T("REECHO"),
    _T("Enphase Energy Inc"),
    _T("Shenzhen Yingrui Storage Technology Co Ltd"),
    _T("Shenzhen Sinomos Semiconductor Technology"),
    _T("O2micro International Limited"),
    _T("Axelera AI BV"),
    _T("Silicon Legend Technology (Suzhou) Co Ltd"),
    _T("Suzhou Novosense Microelectronics Co Ltd"),
    _T("Pirateman"),
    _T("Yangtze MasonSemi"),
    _T("Shanghai Yunsilicon Technology Co Ltd"),
    _T("Rayson"),
    _T("Alphawave IP"),
    _T("Shenzhen Visions Chip Electronic Technology"),
    _T("KYO Group"),
    _T("Shenzhen Aboison Technology Co Ltd"),
    _T("Shenzhen JingSheng Semiconducto Co Ltd"),
    _T("Shenzhen Dingsheng Technology Co Ltd"),
    _T("EVAS Intelligence Co Ltd"),
    _T("Kaibright Electronic Technologies"),
    _T("Fraunhofer IMS"),
    _T("Shenzhen Xinrui Renhe Technology"),
    _T("Beijing Vcore Technology Co Ltd"),
    _T("Silicon Innovation Technologies Co Ltd"),
    _T("Shenzhen Zhengxinda Technology Co Ltd"),
    _T("Shenzhen Remai Electronics Co Lttd"),
    _T("Shenzhen Xinruiyan Electronics Co Ltd"),
    _T("CEC Huada Electronic Design Co Ltd"),
    _T("Westberry Technology Inc"),
    _T("Tongxin Microelectronics Co Ltd"),
    _T("UNIM Semiconductor (Shang Hai) Co Ltd"),
    _T("Shenzhen Qiaowenxingyu Industrial Co Ltd"),
    _T("ICC"),
    _T("Enfabrica Corporation"),
    _T("Niobium Microsystems Inc"),
    _T("Xiaoli AI Electronics (Shenzhen) Co Ltd"),
    _T("Silicon Mitus"),
    _T("Ajiatek Inc"),
    _T("HomeNet"),
    _T("Shenzhen Shubang Technology Co Ltd"),
    _T("Exacta Technologies Ltd"),
    _T("Synology"),
    _T("Trium Elektronik Bilgi Islem San Ve Dis"),
    _T("Wuxi HippStor Technology Co Ltd"),
    _T("SSCT"),
    _T("Sichuan Heentai Semiconductor Co Ltd"),
    _T("Zhejiang University"),
    _T("www.shingroup.cn"),
    _T("Suzhou Nano Mchip Technology Company"),
    _T("Feature Integration Technology Inc"),
    _T("d-Matrix"),
    _T("Golden Memory"),
    _T("Qingdao Thunderobot Technology Co Ltd"),
    _T("Shenzhen Tianxiang Chuangxin Technology"),
    _T("HYPHY USA"),
    _T("Valkyrie"),
    _T("Suzhou Hesetc Electronic Technology Co"),
    _T("Hainan Zhongyuncun Technology Co Ltd"),
    _T("Shenzhen Yousheng Bona Technology Co"),
    _T("Shenzhen Xinle Chuang Technology Co"),
    _T("DEEPX"),
    _T("iStarChip CA LLC"),
    _T("Shenzhen Vinreada Technology Co Ltd"),
    _T("Novatek Microelectronics Corp"),
    _T("Chemgdu EG Technology Co Ltd"),
    _T("AGI Technology"),
    _T("Syntiant"),
    _T("AOC"),
    _T("GamePP"),
    _T("Yibai Electronic Technologies"),
    _T("Hangzhou Rencheng Trading Co Ltd"),
    _T("HOGE Technology Co Ltd"),
    _T("United Micro Technology (Shenzhen) Co"),
    _T("Fabric of Truth Inc"),
    _T("Epitech"),
    _T("Elitestek"),
    _T("Cornelis Networks Inc"),
    _T("WingSemi Technologies Co Ltd"),
    _T("ForwardEdge ASIC"),
    _T("Beijing Future Imprint Technology Co Ltd"),
    _T("Fine Made Microelectronics Group Co Ltd"),
};

static const wchar_t *BANK16_JEDEC_ID_MANUF[] = {
    _T(""),
    _T("Changxin Memory Technology (Shanghai)"),
    _T("Synconv"),
    _T("MULTIUNIT"),
    _T("Zero ASIC Corporation"),
    _T("NTT Innovative Devices Corporation"),
    _T("Xbstor"),
    _T("Shenzhen South Electron Co Ltd"),
    _T("Iontra Inc"),
    _T("SIEFFI Inc"),
    _T("HK Winston Electronics Co Limited"),
    _T("Anhui SunChip Semiconductor Technology"),
    _T("HaiLa Technologies Inc"),
    _T("AUTOTALKS"),
    _T("Shenzhen Ranshuo Technology Co Limited"),
    _T("ScaleFlux"),
    _T("XC Memory"),
    _T("Guangzhou Beimu Technology Co., Ltd"),
    _T("Rays Semiconductor Nanjing Co Ltd"),
    _T("Milli-Centi Intelligence Technology Jiangsu"),
    _T("Zilia Technologioes"),
    _T("Incore Semiconductors"),
    _T("Kinetic Technologies"),
    _T("Nanjing Houmo Technology Co Ltd"),
    _T("Suzhou Yige Technology Co Ltd"),
    _T("Shenzhen Techwinsemi Technology Co Ltd"),
    _T("Pure Array Technology (Shanghai) Co. Ltd"),
    _T("Shenzhen Techwinsemi Technology Udstore"),
    _T("RISE MODE"),
    _T("NEWREESTAR"),
    _T("Hangzhou Hualan Microeletronique Co Ltd"),
    _T("Senscomm Semiconductor Co Ltd"),
    _T("Holt Integrated Circuits"),
    _T("Tenstorrent Inc"),
    _T("SkyeChip"),
    _T("Guangzhou Kaishile Trading Co Ltd"),
    _T("Jing Pai Digital Technology (Shenzhen) Co"),
    _T("Memoritek"),
    _T("Zhejiang Hikstor Technology Co Ltd"),
    _T("Memoritek PTE Ltd"),
    _T("Longsailing Semiconductor Co Ltd"),
    _T("LX Semicon"),
    _T("Shenzhen Techwinsemi Technology Co Ltd"),
    _T("AOC"),
    _T("GOEPEL Electronic GmbH"),
    _T("Shenzhen G-Bong Technology Co Ltd"),
    _T("Openedges Technology Inc"),
    _T("EA Semi Shangahi Limited"),
    _T("EMBCORF"),
    _T("Shenzhen MicroBT Electronics Technology"),
    _T("Shanghai Simor Chip Semiconductor Co"),
    _T("Xllbyte"),
    _T("Guangzhou Maidite Electronics Co Ltd."),
    _T("Zhejiang Changchun Technology Co Ltd"),
    _T("Beijing Cloud Security Technology Co Ltd"),
    _T("SSTC Technology and Distribution Inc"),
    _T("Shenzhen Panmin Technology Co Ltd"),
    _T("ITE Tech Inc"),
    _T("Beijing Zettastone Technology Co Ltd"),
    _T("Powerchip Micro Device"),
    _T("Shenzhen Ysemi Computing Co Ltd"),
    _T("Shenzhen Titan Micro Electronics Co Ltd"),
    _T("Shenzhen Macroflash Technology Co Ltd"),
    _T("Advantech Group"),
    _T("Shenzhen Xingjiachen Electronics Co Ltd"),
    _T("CHUQI"),
    _T("Dongguan Liesun Trading Co Ltd"),
    _T("Shenzhen Miuman Technology Co Ltd"),
    _T("Shenzhen Techwinsemi Technology Twsc"),
    _T("Encharge AI Inc"),
    _T("Shenzhen Zhenchuang Electronics Co Ltd"),
    _T("Giant Chip Co. Ltd"),
    _T("Shenzhen Runner Semiconductor Co Ltd"),
    _T("Scalinx"),
    _T("Shenzhen Lanqi Electronics Co Ltd"),
    _T("CoreComm Technology Co Ltd"),
    _T("DLI Memory"),
    _T("Shenzhen Fidat Technology Co Ltd"),
    _T("Hubei Yangtze Mason Semiconductor Tech"),
    _T("Flastor"),
    _T("PIRATEMAN"),
    _T("Barrie Technologies Co Ltd"),
    _T("Dynacard Co Ltd"),
    _T("Rivian Automotive"),
    _T("Shenzhen Fidat Technology Co Ltd"),
    _T("Zhejang Weiming Semiconductor Co Ltd"),
    _T("Shenzhen Xinhua Micro Technology Co Ltd"),
    _T("Duvonn Electronic Technology Co Ltd"),
    _T("Shenzhen Xinchang Technology Co Ltd"),
    _T("Leidos"),
    _T("Keepixo"),
    _T("Applied Brain Research Inc"),
    _T("Maxio Technology (Hangzhou) Co Ltd"),
    _T("HK DCHIP Technology Limited"),
    _T("Hitachi-LG Data Storage"),
    _T("Shenzhen Huadian Communication Co Ltd"),
    _T("Achieve Memory Technology (Suzhou) Co"),
    _T("Shenzhen Think Future Semiconductor Co"),
    _T("Innosilicon"),
    _T("Shenzhen Weilida Technology Co Ltd"),
    _T("Agrade Storage (Shenzhen) Co Ltd"),
    _T("Shenzhen Worldshine Data Technology Co"),
    _T("Mindgrove Technologies"),
    _T("BYD Semiconductor Co Ltd"),
    _T("Chipsine Semiconductor (Suzhou) Co Ltd"),
    _T("Shen Zhen Shi Xun He Shi Ji Dian Zi You"),
    _T("Shenzhen Jindacheng Computer Co Ltd"),
    _T("Shenzhen Baina Haichuan Technology Co"),
    _T("Shanghai Hengshi Electronic Technology"),
    _T("Beijing Boyu Tuxian Technology Co Ltd"),
    _T("China Chips Star Semiconductor Co Ltd"),
    _T("Shenzhen Shenghuacan Technology Co"),
    _T("Kinara Inc"),
    _T("TRASNA Semiconductor"),
    _T("KEYSOM"),
    _T("Shenzhen YYF Info Tech Co Ltd"),
    _T("Sharetronics Data Technology Co Ltd"),
    _T("AptCore Limited"),
    _T("Uchampion Semiconductor Co Ltd"),
    _T("YCT Semiconductor"),
    _T("FADU Inc"),
    _T("Hefei CLT Microelectronics Co LTD"),
    _T("Smart Technologies (BD) Ltd"),
    _T("Zhangdian District Qunyuan Computer Firm"),
    _T("Silicon Xpandas Electronics Co Ltd"),
    _T("PC Components Y Multimedia S"),
    _T("Shenzhen Tanlr Technology Group Co Ltd"),
};

static const wchar_t *BANK17_JEDEC_ID_MANUF[] = {
    _T("Shenzhen JIEQING Technology Co Ltd"),
    _T("Orionix"),
    _T("JoulWatt Technology Co Ltd"),
    _T("Tenstorrent"),
    _T("Unis Flash Memory Technology (Chengdu)"),
    _T("Huatu Stars"),
    _T("Ardor Gaming"),
    _T("QuanZhou KunFang Semiconductor Co Ltd"),
    _T("EIAI PLANET"),
    _T("Ningbo Lingkai Semiconductor Technology Inc"),
    _T("Shenzhen Hancun Technology Co Ltd"),
    _T("Hongkong Manyi Technology Co Limited"),
    _T("Shenzhen Storgon Technology Co Ltd"),
    _T("YUNTU Microelectronics"),
    _T("Essencore"),
    _T("Shenzhen Xingyun Lianchuang Computer Tech"),
    _T("ShenZhen Aoscar Digital Tech Co Ltd"),
    _T("XOC Technologies Inc"),
    _T("BOS Semiconductors"),
    _T("Eliyan Corp"),
    _T("Hangzhou Lishu Technology Co Ltd"),
    _T("Tier IV Inc"),
    _T("Wuhan Xuanluzhe Network Technology Co"),
    _T("Semi(Shanghai) Limited"),
    _T("Tech Vision Information Technology Co"),
    _T("Zhihe Computing Technology"),
    _T("Beijing Apexichips Tech"),
    _T("Yemas Holdingsl Limited"),
    _T("Eluktronics"),
    _T("Walton Digi - Tech Industries Ltd"),
    _T("Beijing Qixin Gongli Technology Co Ltd"),
    _T("M.RED"),
    _T("Shenzhen Damay Semiconductor Co Ltd"),
    _T("Corelab Tech Singapore Holding PTE LTD"),
    _T("EmBestor Technology Inc"),
    _T("XConn Technologies"),
    _T("Flagchip"),
    _T("CUNNUC"),
    _T("SGMicro"),
    _T("Lanxin Computing(Shenzhen) Technology"),
    _T("FuturePlus Systems LLC"),
    _T("Shenzhen Jielong Storage Technology Co"),
    _T("Precision Planting LLC"),
    _T("Sichuan ZeroneStor Microelectronics Tech"),
    _T("The University of Tokyo"),
    _T("Aodu(Fujian) Information Technology Co"),
    _T("Bytera Memory Inc"),
    _T("XSemitron Technology Inc"),
    _T("Cloud Ridge Ltd"),
    _T("Shenzhen XinChiTai Technology Co Ltd"),
    _T("Shenzhen Xinxin Semiconductor Co Ltd"),
    _T("Shenzhen ShineKing Electronics Co Ltd."),
    _T("Shenzhen Shande Semiconductor Co.Ltd."),
    _T("AheadComputing"),
    _T("Beijing Ronghua Kangweiye Technology"),
    _T("Shanghai Yunsilicon Technology Co Ltd"),
    _T("Shenzhen Wolongtai Technology Co Ltd."),
    _T(""),
    _T(""),
    _T(""),
    _T("GOFATOO"),
};

#define JEDEC_BANK_NUM_ID(bank) sizeof(BANK##bank##_JEDEC_ID_MANUF) / sizeof(BANK##bank##_JEDEC_ID_MANUF[0])

static const int JEDEC_ID_MANUF_BANK_NUM_ID[] = {
    JEDEC_BANK_NUM_ID(1),
    JEDEC_BANK_NUM_ID(2),
    JEDEC_BANK_NUM_ID(3),
    JEDEC_BANK_NUM_ID(4),
    JEDEC_BANK_NUM_ID(5),
    JEDEC_BANK_NUM_ID(6),
    JEDEC_BANK_NUM_ID(7),
    JEDEC_BANK_NUM_ID(8),
    JEDEC_BANK_NUM_ID(9),
    JEDEC_BANK_NUM_ID(10),
    JEDEC_BANK_NUM_ID(11),
    JEDEC_BANK_NUM_ID(12),
    JEDEC_BANK_NUM_ID(13),
    JEDEC_BANK_NUM_ID(14),
    JEDEC_BANK_NUM_ID(15),
    JEDEC_BANK_NUM_ID(16),
    JEDEC_BANK_NUM_ID(17),
};

static const wchar_t **JEDEC_ID_MANUF_BANK_MAP[] = {
    BANK1_JEDEC_ID_MANUF,
    BANK2_JEDEC_ID_MANUF,
    BANK3_JEDEC_ID_MANUF,
    BANK4_JEDEC_ID_MANUF,
    BANK5_JEDEC_ID_MANUF,
    BANK6_JEDEC_ID_MANUF,
    BANK7_JEDEC_ID_MANUF,
    BANK8_JEDEC_ID_MANUF,
    BANK9_JEDEC_ID_MANUF,
    BANK10_JEDEC_ID_MANUF,
    BANK11_JEDEC_ID_MANUF,
    BANK12_JEDEC_ID_MANUF,
    BANK13_JEDEC_ID_MANUF,
    BANK14_JEDEC_ID_MANUF,
    BANK15_JEDEC_ID_MANUF,
    BANK16_JEDEC_ID_MANUF,
    BANK17_JEDEC_ID_MANUF,
};

#define MAX_JEDEC_BANK_INDEX (sizeof(JEDEC_ID_MANUF_BANK_MAP) / sizeof(JEDEC_ID_MANUF_BANK_MAP[0]))

bool GetManNameFromJedecID(ULONGLONG jedecID, wchar_t *buffer, int maxBufSize);
bool GetManNameFromJedecIDContCode(unsigned char jedecID, int numContCode, wchar_t *buffer, int maxBufSize);

#ifdef WIN32

static wchar_t *MEMORY_MANUFACTURERS[] =
    {
        _T("3COM"),
        _T("3ParData"),
        _T("AANetcom Incorporated"),
        _T("Acbel Polytech Inc"),
        _T("Accelerant Networks"),
        _T("Acorn Computers"),
        _T("ACTEL"),
        _T("Actrans System Inc"),
        _T("Adaptec Inc"),
        _T("Adhoc Technologies"),
        _T("Admor Memory"),
        _T("ADMtek Incorporated"),
        _T("Adobe Systems"),
        _T("ADTEC Corporation"),
        _T("Advanced Fibre"),
        _T("Advanced Hardware Arch."),
        _T("Advantage Memory"),
        _T("AENEON"),
        _T("Agate Semiconductor"),
        _T("Agilent Technologies"),
        _T("Aica Kogyo"),
        _T("AKM Company"),
        _T("Alcatel Mietec"),
        _T("Alchemy Semiconductor"),
        _T("Allayer Technologies"),
        _T("Alliance Semiconductor"),
        _T("Allied-Signal"),
        _T("ALPHA Technologies"),
        _T("Altera"),
        _T("AMCC"),
        _T("AMD"),
        _T("American Computer & Digital Components Inc"),
        _T("AMI"),
        _T("AMIC Technology"),
        _T("AMS(Austria Micro)"),
        _T("Anadigm"),
        _T("Analog Devices"),
        _T("Apacer Technology"),
        _T("Aplus Flash Technology"),
        _T("Apple Computer"),
        _T("APW"),
        _T("Ardent Technologies"),
        _T("Array Microsystems"),
        _T("Artesyn Technologies"),
        _T("Asparix"),
        _T("Aster Electronics"),
        _T("Atmel"),
        _T("AVED Memory"),
        _T("Bay Networks"),
        _T("Benchmark Elect."),
        _T("Bestlink Systems"),
        _T("BF Goodrich Data."),
        _T("BRECIS"),
        _T("Bright Micro"),
        _T("Broadcom"),
        _T("Brooktree"),
        _T("Cabletron"),
        _T("Camintonn Corporation"),
        _T("Cannon"),
        _T("Capital Instruments Inc"),
        _T("Caspian Networks"),
        _T("Catalyst"),
        _T("Celestica"),
        _T("Centaur Technology"),
        _T("Centillium Communications"),
        _T("Century"),
        _T("Chameleon Systems"),
        _T("Chiaro"),
        _T("Chicory Systems"),
        _T("Chip Express"),
        _T("Chip2Chip Incorporated"),
        _T("Chrysalis ITS"),
        _T("Cimaron Communications"),
        _T("Cirrus Logic"),
        _T("Cisco Systems Inc"),
        _T("CKD Corporation Ltd."),
        _T("Clear Logic"),
        _T("Clearpoint"),
        _T("Compaq"),
        _T("Com-Tier"),
        _T("Conexant"),
        _T("Convex Computer"),
        _T("Corollary Inc"),
        _T("Corsair"),
        _T("C-Port Corporation"),
        _T("CPU Design"),
        _T("Cray Research"),
        _T("Crosspoint Solutions"),
        _T("Crystal Semiconductor"),
        _T("Cypress"),
        _T("Dallas Semiconductor"),
        _T("Dane-Elec"),
        _T("DATARAM"),
        _T("DEC"),
        _T("Delkin Devices"),
        _T("Dialog"),
        _T("Digital Microwave"),
        _T("DoD"),
        _T("Domosys"),
        _T("DSP Group"),
        _T("Dynachip"),
        _T("Dynamem"),
        _T("EIV(Switzerland)"),
        _T("Elan Circuit Tech"),
        _T("Elektrobit"),
        _T("Element 14"),
        _T("Elpida"),
        _T("Enhance 3000 Inc"),
        _T("Enikia Incorporated"),
        _T("Eon Silicon Devices"),
        _T("Epigram"),
        _T("Ericsson Components"),
        _T("eSilicon"),
        _T("ESS Technology"),
        _T("European Silicon Str."),
        _T("Eurotechnique"),
        _T("Exar"),
        _T("Exbit Technology A/S"),
        _T("Excess Bandwidth"),
        _T("Exel"),
        _T("Extreme Packet Devices"),
        _T("Fairchild"),
        _T("Fast-Chip"),
        _T("FDK Corporation"),
        _T("Feiya Technology"),
        _T("Flextronics (formerly Orbit)"),
        _T("FOXCONN"),
        _T("Freescale"),
        _T("Fujitsu"),
        _T("Gadzoox Nteworks"),
        _T("Galaxy Power"),
        _T("Galileo Tech"),
        _T("Galvantech"),
        _T("GateField"),
        _T("Gemstone Communications"),
        _T("GENNUM"),
        _T("GlobeSpan"),
        _T("Goldenram"),
        _T("Graychip"),
        _T("Great Technology Microcomputer"),
        _T("GSI Technology"),
        _T("GTE"),
        _T("HADCO Corporation"),
        _T("Hagiwara Solutions Co Ltd"),
        _T("Hal Computers"),
        _T("HanBit Electronics"),
        _T("Harris"),
        _T("Helix AG"),
        _T("Hewlett-Packard"),
        _T("Hi/fn"),
        _T("High Bandwidth Access"),
        _T("HiNT Corporation"),
        _T("Hitachi"),
        _T("Honeywell"),
        _T("Hughes Aircraft"),
        _T("Hyperchip"),
        _T("HYPERTEC"),
        _T("Hyundai Electronics"),
        _T("I & C Technology"),
        _T("I.T.T."),
        _T("I3 Design System"),
        _T("IBM"),
        _T("IDT"),
        _T("ILC Data Device"),
        _T("Infineon"),
        _T("Inmos"),
        _T("Inova Semiconductors GmbH"),
        _T("Integ. Memories Tech."),
        _T("Integrated CMOS (Vertex)"),
        _T("Integrated Memory System"),
        _T("Integrated Technology Express"),
        _T("Intel"),
        _T("Intellitech Corporation"),
        _T("Intersil"),
        _T("Intg. Silicon Solutions"),
        _T("Intl. CMOS Technology"),
        _T("Ishoni Networks"),
        _T("ISOA Incorporated"),
        _T("Itautec Philco SA"),
        _T("Itec Memory"),
        _T("Jasmine Networks"),
        _T("JTAG Technologies"),
        _T("Juniper Networks"),
        _T("Kawasaki Steel"),
        _T("Kendin Communications"),
        _T("Kentron Technologies"),
        _T("Kingston"),
        _T("Klic"),
        _T("Lanstar Semiconductor"),
        _T("Lara Technology"),
        _T("Lattice Semi."),
        _T("LeCroy"),
        _T("Legend"),
        _T("Legerity"),
        _T("Level One"),
        _T("LG Semi"),
        _T("Libit Signal Processing"),
        _T("LightSpeed Semi."),
        _T("Linvex Technology"),
        _T("Loral"),
        _T("LSI Logic"),
        _T("Lucent"),
        _T("Macronix"),
        _T("Malaysia Micro Solutions"),
        _T("mAlign Manufacturing"),
        _T("Malleable Technologies"),
        _T("Maxim Integrated Product"),
        _T("MCI Computer GMBH"),
        _T("Media Vision"),
        _T("Megic"),
        _T("Mellanox Technologies"),
        _T("Memory Card Technology"),
        _T("MetaLink Technologies"),
        _T("MGV Memory"),
        _T("MHS Electronic"),
        _T("Micro Linear"),
        _T("Micro Memory Bank"),
        _T("MicrochipTechnology"),
        _T("Micron CMS"),
        _T("Micron Technology"),
        _T("Micronas"),
        _T("MIMOS Semiconductor"),
        _T("MIPS Technologies"),
        _T("Mitsubishi"),
        _T("MMC Networks"),
        _T("Monolithic Memories"),
        _T("Morphics Technology"),
        _T("MOSAID Technologies"),
        _T("Mostek"),
        _T("mRadiata"),
        _T("MSC Vertriebs"),
        _T("Multi Dimensional Cons."),
        _T("MultiLink Technology"),
        _T("Music Semi"),
        _T("National"),
        _T("National Instruments"),
        _T("Nchip"),
        _T("NCR"),
        _T("nCUBE"),
        _T("NEC"),
        _T("NERA ASA"),
        _T("NetLogic Microsystems"),
        _T("New Media"),
        _T("Newport Communications"),
        _T("Newport Digital"),
        _T("NEXCOM"),
        _T("Nimbus Technology"),
        _T("Nippon Steel Semi. Corp."),
        _T("Nordic VLSI ASA"),
        _T("Northern Telecom"),
        _T("Novanet Semiconductor"),
        _T("Novatel Wireless"),
        _T("Oasis Semiconductor"),
        _T("OCZ"),
        _T("OKI Semiconductor"),
        _T("Omnivision"),
        _T("Panasonic"),
        _T("PCMCIA"),
        _T("Peregrine Semiconductor"),
        _T("Performance Semi."),
        _T("Philips Semi."),
        _T("Phobos Corporation"),
        _T("Plus Logic"),
        _T("PMC-Sierra"),
        _T("PNY Technologies Inc"),
        _T("Power General"),
        _T("PQuality Semiconductor"),
        _T("Price Point"),
        _T("Programmable Micro Corp"),
        _T("ProMos/Mosel Vitelic"),
        _T("Protocol Engines"),
        _T("Pycon"),
        _T("Qlogic"),
        _T("Quadratics Superconductor"),
        _T("QUALCOMM"),
        _T("Ramtron"),
        _T("Raytheon"),
        _T("RCA"),
        _T("Realchip"),
        _T("RE-M Solutions"),
        _T("RF Micro Devices"),
        _T("Ricoh Ltd."),
        _T("Robert Bosch"),
        _T("Rohm Company Ltd."),
        _T("Saifun Semiconductors"),
        _T("Samsung"),
        _T("SandCraft"),
        _T("Sanmina Corporation"),
        _T("Sanyo"),
        _T("Sarnoff Corporation"),
        _T("SCI"),
        _T("Seeq"),
        _T("Seiko Epson"),
        _T("Seiko Instruments"),
        _T("STMicroelectronics"),
        _T("Sharp"),
        _T("Shikatronics"),
        _T("SiberCore Technologies"),
        _T("Sibyte, Incorporated"),
        _T("Siemens AG"),
        _T("Silicon Access Networks"),
        _T("Silicon Laboratories"),
        _T("Silicon Spice"),
        _T("Silicon Wave"),
        _T("Skyup Technology"),
        _T("Smart Modular"),
        _T("Solbourne Computer"),
        _T("Sony"),
        _T("Southland Microsystems"),
        _T("SpaSE"),
        _T("SpecTek Incorporated"),
        _T("SSSI"),
        _T("SST"),
        _T("Sun Microsystems"),
        _T("SunDisk"),
        _T("Super PC Memory"),
        _T("Suwa Electronics"),
        _T("Switchcore"),
        _T("SwitchOn Networks"),
        _T("Symagery Microsystems"),
        _T("Synertek"),
        _T("T Square"),
        _T("Tezzaron Semiconductor"),
        _T("Tandem"),
        _T("Tanisys Technology"),
        _T("TCSI"),
        _T("TECMAR"),
        _T("Tektronix"),
        _T("Tellabs"),
        _T("Telocity"),
        _T("Tenx Technologies"),
        _T("Texas Instruments"),
        _T("Thesys"),
        _T("Thinking Machine"),
        _T("Thomson CSF"),
        _T("Toshiba Memory Corporation"),
        _T("Tower Semiconductor"),
        _T("Transcend Information"),
        _T("Transmeta"),
        _T("Transwitch"),
        _T("Triscend"),
        _T("Tristar"),
        _T("Truevision"),
        _T("TRW"),
        _T("Tundra Semiconductor"),
        _T("Unigen Corporation"),
        _T("United Microelec Corp"),
        _T("Univ. of NC"),
        _T("UTMC"),
        _T("V3 Semiconductor"),
        _T("Vanguard"),
        _T("Vantis"),
        _T("VideoLogic"),
        _T("Viking Components"),
        _T("Virata Corporation"),
        _T("Visic"),
        _T("Vitesse"),
        _T("VLSI"),
        _T("W.L. Gore"),
        _T("Wafer Scale Integration"),
        _T("West Bay Semiconductor"),
        _T("Win Technologies"),
        _T("Winbond Electronic"),
        _T("Wintec Industries"),
        _T("WorkX AG"),
        _T("World Wide Packets"),
        _T("XaQti"),
        _T("Xerox"),
        _T("Xicor"),
        _T("Xilinx"),
        _T("Xstream Logic"),
        _T("Yamaha Corporation"),
        _T("Zarlink"),
        _T("Zentrum"),
        _T("ZMD"),
        _T("Zilog"),
        _T("ZSP Corp."),
        _T("Zucotto Wireless"),
        _T(""), // Terminator
};

bool ConvertSMBIOSRAMManufacturer_Method1(wchar_t *szInRaw, BYTE *pbJedecID, int *piNumContCode);
bool ConvertSMBIOSRAMManufacturer_Method2(wchar_t *szInRaw, BYTE *pbJedecID, int *piNumContCode);

// GetRAMManufacturer
//
void SystemInfo_GetRAMManufacturer_SMBIOS(int iType, wchar_t *szOut, int iLenOut, wchar_t *szInRaw)
{
    BYTE byJedecID = 0;
    int iNumContCode = 0;

    wchar_t szRawTemp[SMB_STRINGLEN] = {_T('\0')};
    wchar_t szMemMan[SMB_STRINGLEN] = {_T('\0')};

    if (_tcslen(szInRaw) < (SMB_STRINGLEN - 1))
        _tcscpy(szRawTemp, szInRaw);

    _tcscpy(szOut, _T(""));

    /////////////////////////////////////////////////////////////////////////////////
    // DEBUGGING
    //
    // iType = RAMTYPE_DDR2;
    //_tcscpy(szRawTemp, "JEDEC ID:C1 00 00 00 00 00 00 00");
    //_tcscpy(szRawTemp, "JEDEC ID:AD 00 00 00 00 00 00 00");
    //_tcscpy(szRawTemp, "JEDEC ID:7F 7F 7F 0B 00 00 00 00");
    //_tcscpy(szRawTemp, "JEDEC ID:7F 7F 7F 0B FF FF FF FF");

    // iType = RAMTYPE_DDR2_FB_DIMM;
    //_tcscpy(szRawTemp, "JEDEC ID:02 9E 01 00 00 00 00 00");
    /////////////////////////////////////////////////////////////////////////////////

    switch (iType) // BIT601000.0013 - different handling for DDR2_FB as specification SPD different
    {
    case RAMTYPE_SDRAM:
    case RAMTYPE_DDR:
    case RAMTYPE_DDR2:
        if (ConvertSMBIOSRAMManufacturer_Method1(szRawTemp, &byJedecID, &iNumContCode))
            GetManNameFromJedecIDContCode(byJedecID, iNumContCode, szOut, iLenOut);
        break;

    case RAMTYPE_DDR2_FB_DIMM:
    case RAMTYPE_DDR3:
        if (ConvertSMBIOSRAMManufacturer_Method2(szRawTemp, &byJedecID, &iNumContCode))
            GetManNameFromJedecIDContCode(byJedecID, iNumContCode, szOut, iLenOut);

        break;
    }

    // It seems that some systems have the name of the manufacturer in the raw data, lets try and use that
    if (_tcslen(szOut) == 0) // BIT6010030002
    {
        for (int i = 0; i < MAX_JEDEC_BANK_INDEX; i++)
        {
            for (int j = 0; j < JEDEC_ID_MANUF_BANK_NUM_ID[i]; j++)
            {
                _tcscpy(szMemMan, JEDEC_ID_MANUF_BANK_MAP[i][j]);
                if (_tcsstr(_tcsupr(szInRaw), _tcsupr(szMemMan)))
                {
                    _tcscpy(szOut, JEDEC_ID_MANUF_BANK_MAP[i][j]);
                    return;
                }
            }
        }
    }
}

// ConvertSMBIOSRAMManufacturer_Method1
//
//  Try to get the RAm manufacturer from the raw SMBIOS string
//  e.g. convert "JEDEC ID:7F 7F 7F 0B FF FF FF FF" t0 the number 7F7F7F0BFFFFFFFF
/*
METHOD 1:
========
Memory type DDR-SDRAM
Byte 64-71 (40h-47h) Manufactuer JEDEC code
Manufacturer (ID) Samsung (CE00000000000000)
40: CE 00 00 00 00 00 00 00 01 4D 34 20 37 30 4C 36

Memory type DDR
Byte 64-71 (40h-47h) Manufactuer JEDEC code
Manufacturer (ID) Kingston (7F98000000000000)
40: 7F 98 00 00 00 00 00 00 04 4B 00 00 00 00 00 00
Manufacturer (ID) Micron Technology (2CFFFFFFFFFFFFFF)
40: 2C FF FF FF FF FF FF FF 08 38 56 44 44 54 33 32

Memory type		DDR2
Byte 64-71 (40h-47h) Manufactuer JEDEC code
Manufacturer (ID)	Corsair (7F7F9E0000000000)
40: 7F 7F 9E 00 00 00 00 00 01 43 4D 32 58 31 30 32

Memory type  FB-DDR2
Byte 117-118 (75h-76h) Manufactuer JEDEC code, byte 117 bits 0-6 specify the number of continutation bytes "7F"
Manufacturer (ID) Kingston (7F98000000000000)
70: 60 60 60 7F B3 01 98 01 08 26 12 08 9F F2 14 32
*/
bool ConvertSMBIOSRAMManufacturer_Method1(wchar_t *szInRaw, BYTE *pbJedecID, int *piNumContCode)
{
    int i = 0;
    wchar_t szJ1[LONG_STRING_LEN] = {_T('\0')};
    wchar_t *pszJ1Tail = NULL;
    wchar_t *pszJedecID = _tcsstr(szInRaw, _T("JEDEC ID:"));
    ULONGLONG ullJedecID = 0, ullByte = 0;
    BYTE byJedecID = 0;
    bool bValid = true;
    bool bFound_Code = false;
    int iNumContinuations = 0; // 7F's

    if (pszJedecID != NULL)
    {
        pszJedecID = _tcschr(szInRaw, _T(':'));
        if (pszJedecID != NULL)
            pszJedecID = &pszJedecID[1];

        // Get 8 bytes or fail
        for (i = 0; i < 8 && bValid; i++)
        {
            _tcscpy(szJ1, pszJedecID);
            pszJ1Tail = _tcschr(szJ1, ' ');
            if (pszJ1Tail == NULL && i < 7)
            {
                bValid = false;
                break;
            }
            if (i < 7)
            {
                //_tcscpy(pszJedecID, pszJ1Tail+sizeof(char));
                _tcscpy(pszJedecID, &pszJ1Tail[1]);
                *pszJ1Tail = _T('\0');
            }
#ifdef _UNICODE
            ullByte = _tcstoul(szJ1, _T('\0'), 16);
#else
            ullByte = strtoul(szJ1, _T('\0'), 16);
#endif
            ullJedecID = (ullJedecID << 8) + ullByte;

            // Validation, only allow
            //	[7F].. [7F] MM [00|FF] .. [00|FF]
            //		where MM is the manufacture byte
            if (!bFound_Code)
            {
                if (ullByte == 0x7F) // Continutation code
                    iNumContinuations++;
                else
                {
                    byJedecID = (BYTE)ullByte;
                    bFound_Code = true;
                }
            }
            else
            {
                if (ullByte != 0x00 && ullByte != 0xFF)
                    bValid = false;
            }
        }
    }
    if (bValid)
    {
        *pbJedecID = byJedecID;
        *piNumContCode = iNumContinuations;
    }

    return bValid;
}

// ConvertSMBIOSRAMManufacturer_Method2
//
//  Try to get the RAm manufacturer from the raw SMBIOS string
//  e.g. convert "JEDEC ID:7F 7F 7F 0B FF FF FF FF" t0 the number 7F7F7F0BFFFFFFFF
/*
METHOD 2:
========
Memory type		FB-DDR2
Byte 117-118 (75h-76h) Manufactuer JEDEC code
Manufacturer (ID)	Micron Technology (2C00000000000000)
70: 4C 00 05 80 89 80 2C 01 06 16 28 05 E1 5F 83 83		(80 2C => bits 0:6[80] = 0 continuation => 2C0000000000000000)
Manufacturer (ID)	Kingston (7F98000000000000)
70: 60 60 60 7F B3 01 98 01 08 26 12 08 9F F2 14 32		(01 98 => bits 0:6[01] = 1 continuation => 7F9800000000000000)

Memory type		DDR3
Byte 117-118 (75h-76h) Manufactuer JEDEC code
Manufacturer (ID)	Corsair (7F7F9E0000000000)
70: 00 00 00 00 00 02 9E 01 00 00 00 00 00 00 6D 61

Memory type		DDR3
0x029E
*/
bool ConvertSMBIOSRAMManufacturer_Method2(wchar_t *szInRaw, BYTE *pbJedecID, int *piNumContCode)
{
    int i = 0;
    wchar_t szJ1[LONG_STRING_LEN] = {_T('\0')};
    wchar_t *pszJ1Tail = NULL;
    ULONGLONG ullJedecID = 0, ullByte = 0;
    BYTE byJedecID = 0;
    bool bValid = true;
    int iNumContinuationBytes = 0, iCont = 0; // 7F's

    // Chop off the "JEDEC ID:" if it exists
    wchar_t *pszJedecID = _tcsstr(szInRaw, _T("JEDEC ID:"));
    if (pszJedecID != NULL)
    {
        pszJedecID = _tcschr(szInRaw, _T(':'));
        if (pszJedecID != NULL)
            pszJedecID = &pszJedecID[1];
    }
    else // Chop of the 0x as in "0x029E"
    {
        //"0x029E" -> 7F 7F 9E -> Corsair
        pszJedecID = _tcsstr(szInRaw, _T("0x")); // BIT601000.0015
        if (pszJedecID != NULL)
        {
            pszJedecID = _tcschr(szInRaw, _T('x'));
            if (pszJedecID != NULL)
                pszJedecID = &pszJedecID[1];
        }
    }
    if (pszJedecID != NULL)
    {
        if (_tcslen(pszJedecID) == 4) // No spaces
        {
            // Get 2 bytes or fail
            for (i = 0; i < 2 && bValid; i++)
            {
                _tcscpy(szJ1, pszJedecID);
                // Check if there is a space between the bytes
                // pszJ1Tail = szJ1 + 2 * sizeof(char);
                pszJ1Tail = &szJ1[2];
                if (i < 7)
                {
                    _tcscpy(pszJedecID, pszJ1Tail);
                    *pszJ1Tail = _T('\0');
                }
                if (i == 0)
                {
#ifdef _UNICODE
                    iNumContinuationBytes = _tcstoul(szJ1, _T('\0'), 16);
#else
                    iNumContinuationBytes = strtoul(szJ1, _T('\0'), 16);
#endif
                    iNumContinuationBytes = (iNumContinuationBytes & 0x3F); // Extract the number of continuation bytes from bits [6:0]
#if 0                                                                       // <km> Can be greater than 7 continuation codes
                    if (iNumContinuationBytes > 7)
                        bValid = false;
#endif
                }
                else if (i == 1)
                {
#ifdef _UNICODE
                    ullByte = _tcstoul(szJ1, _T('\0'), 16);
#else
                    ullByte = strtoul(szJ1, _T('\0'), 16);
#endif
                    for (iCont = 0; iCont < iNumContinuationBytes; iCont++) // Insert continutation bytes
                        ullJedecID = (ullJedecID << 8) + 0x7F;

                    byJedecID = (BYTE)ullByte;
                    ullJedecID = (ullJedecID << 8) + ullByte; // Insert manufacturer code

                    for (iCont = 0; iCont < 7 - iNumContinuationBytes; iCont++) // Insert remaining padding
                        ullJedecID = (ullJedecID << 8) + 0x00;
                }
            }
        }
        else // spaces
        {
            // Get 2 bytes (space between e.g. "02 9F" or fail
            for (i = 0; i < 2 && bValid; i++)
            {
                _tcscpy(szJ1, pszJedecID);
                pszJ1Tail = _tcschr(szJ1, _T(' '));
                if (pszJ1Tail == NULL)
                {
                    bValid = false;
                    break;
                }
                if (i < 7)
                {
                    //_tcscpy(pszJedecID, pszJ1Tail+sizeof(char));
                    _tcscpy(pszJedecID, &pszJ1Tail[1]);
                    *pszJ1Tail = _T('\0');
                }
                if (i == 0)
                {
#ifdef _UNICODE
                    iNumContinuationBytes = _tcstoul(szJ1, _T('\0'), 16);
#else
                    iNumContinuationBytes = strtoul(szJ1, _T('\0'), 16);
#endif
                    iNumContinuationBytes = (iNumContinuationBytes & 0x3F); // Extract the number of continuation bytes from bits [6:0]
#if 0                                                                       // <km> Can be greater than 7 continuation codes
                    if (iNumContinuationBytes > 7)
                        bValid = false;
#endif
                }
                else if (i == 1)
                {
#ifdef _UNICODE
                    ullByte = _tcstoul(szJ1, _T('\0'), 16);
#else
                    ullByte = strtoul(szJ1, _T('\0'), 16);
#endif
                    for (iCont = 0; iCont < iNumContinuationBytes; iCont++) // Insert continutation bytes
                        ullJedecID = (ullJedecID << 8) + 0x7F;

                    byJedecID = (BYTE)ullByte;
                    ullJedecID = (ullJedecID << 8) + ullByte; // Insert manufacturer code

                    for (iCont = 0; iCont < 7 - iNumContinuationBytes; iCont++) // Insert remaining padding
                        ullJedecID = (ullJedecID << 8) + 0x00;
                }
            }
        }
    }

    if (bValid)
    {
        *pbJedecID = byJedecID;
        *piNumContCode = iNumContinuationBytes;
    }

    return bValid;
}
#endif

// #include "JedecIDList.h"

// Returns a manufacturer name that corresponds to the Jedec ID
// http://www.idhw.com/textual/chip/jedec_spd_man.html
//
//  SPD for EEPROM
//
//
//  SPD for DDR2
//
/*
Hynix				80 AD -> AD00...
Kingston			01 98 -> 7F98...
Micron Tecnology	80 2C -> 2C00...
70   00 00 00 85 51 85 51 45 07 33 0A 04 92 17 BF 60
Manufacturer (ID)	Qimonda (7F7F7F7F7F510000)
70   1B 60 1B 80 B3 80 CE 01 07 29 10 10 CE 26 82 CA
Manufacturer (ID)	Samsung (CE00000000000000)
*/
bool GetManNameFromJedecID(ULONGLONG jedecID, wchar_t *buffer, int maxBufSize)
{
    for (int numContCode = 0; numContCode < 8; numContCode++)
    {
        int bitshift = 56 - numContCode * 8;
        if ((jedecID >> bitshift) != 0x7f)
        {
            // mask off the ID code;
            unsigned char jedecIDIdx = (jedecID >> bitshift) & 0xFF;
            // mask off the parity bit;
            jedecIDIdx = jedecIDIdx & 0x7F;

            return GetManNameFromJedecIDContCode(jedecIDIdx, numContCode, buffer, maxBufSize);
        }
    }

    return false;
}

bool GetManNameFromJedecIDContCode(unsigned char jedecID, int numContCode, wchar_t *buffer, int maxBufSize)
{
    wchar_t tmpBuffer[1024]; // SI101005
    memset(tmpBuffer, 0, sizeof(tmpBuffer));

    if (numContCode < 0 || numContCode >= MAX_JEDEC_BANK_INDEX)
        return false;

    if (jedecID >= JEDEC_ID_MANUF_BANK_NUM_ID[numContCode])
        return false;

    _tcscpy(tmpBuffer, JEDEC_ID_MANUF_BANK_MAP[numContCode][jedecID]);

    if ((int)_tcslen(tmpBuffer) > maxBufSize)
    {
        _tcsncpy_s(buffer, maxBufSize, tmpBuffer, maxBufSize);
        buffer[maxBufSize - 1] = _T('\0');
    }
    else
    {
        _tcscpy_s(buffer, maxBufSize, tmpBuffer);
    }

    return true;
}

/*


Memory SPD
------------------------------------------------------------------------------

DIMM #1

General
Memory type DDR
Manufacturer (ID) Kingston (7F98000000000000)
Size 256 MBytes
Max bandwidth PC3200 (200 MHz)
Part number K
Serial number 660A7D25
Manufacturing date Week 01/Year 04

Attributes
Number of banks 1
Data width 64 bits
Correction None
Registered no
Buffered no
Nominal Voltage 2.50 Volts
EPP no
XMP no

Timings table
Frequency (MHz) 166 200
CAS# 2.0 2.5
RAS# to CAS# delay 3 3
RAS# Precharge 3 3
TRAS 7 8


DIMM #2

General
Memory type DDR
Manufacturer (ID) Nanya Technology (7F7F7F0B00000000)
Size 256 MBytes
Max bandwidth PC3200 (200 MHz)
Part number M2U25664DS88B3G-5T
Manufacturing date Week 52/Year 03

Attributes
Number of banks 1
Data width 64 bits
Correction None
Registered no
Buffered no
Nominal Voltage 2.50 Volts
EPP no
XMP no

Timings table
Frequency (MHz) 133 166 200
CAS# 2.0 2.5 3.0
RAS# to CAS# delay 2 3 3
RAS# Precharge 2 3 3
TRAS 6 7 8


DIMM #3

General
Memory type DDR
Manufacturer (ID) Nanya Technology (7F7F7F0B00000000)
Size 256 MBytes
Max bandwidth PC3200 (200 MHz)
Part number M2U25664DS88B3G-5T
Manufacturing date Week 52/Year 03

Attributes
Number of banks 1
Data width 64 bits
Correction None
Registered no
Buffered no
Nominal Voltage 2.50 Volts
EPP no
XMP no

Timings table
Frequency (MHz) 133 166 200
CAS# 2.0 2.5 3.0
RAS# to CAS# delay 2 3 3
RAS# Precharge 2 3 3
TRAS 6 7 8


Dump Module #1
0 1 2 3 4 5 6 7 8 9 A B C D E F
00 80 08 07 0D 0A 01 40 00 04 50 60 00 82 08 00 01
10 0E 04 0C 01 02 20 00 60 70 00 00 3C 28 3C 28 40
20 60 60 40 40 00 00 00 00 00 37 41 28 23 55 00 00
30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 97
40 7F 98 00 00 00 00 00 00 04 4B 00 00 00 00 00 00
50 00 00 00 00 00 00 00 00 00 00 00 00 00 04 01 66
60 0A 7D 25 00 00 00 00 00 00 00 00 00 00 00 00 00
70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
90 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
A0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
B0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
C0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
D0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
E0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
F0 39 39 30 35 31 39 32 2D 30 34 35 2E 41 30 30 00


Dump Module #2
0 1 2 3 4 5 6 7 8 9 A B C D E F
00 80 08 07 0D 0A 01 40 00 04 50 60 00 82 08 00 01
10 0E 04 1C 01 02 20 00 60 70 75 75 3C 28 3C 28 40
20 60 60 40 40 00 00 00 00 00 37 46 20 28 50 00 00
30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 8E
40 7F 7F 7F 0B 00 00 00 00 09 4D 32 55 32 35 36 36
50 34 44 53 38 38 42 33 47 2D 35 54 00 00 03 34 00
60 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
90 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
A0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
B0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
C0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
D0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
E0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
F0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00


Dump Module #3
0 1 2 3 4 5 6 7 8 9 A B C D E F
00 80 08 07 0D 0A 01 40 00 04 50 60 00 82 08 00 01
10 0E 04 1C 01 02 20 00 60 70 75 75 3C 28 3C 28 40
20 60 60 40 40 00 00 00 00 00 37 46 20 28 50 00 00
30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 8E
40 7F 7F 7F 0B 00 00 00 00 09 4D 32 55 32 35 36 36
50 34 44 53 38 38 42 33 47 2D 35 54 00 00 03 34 00
60 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
90 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
A0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
B0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
C0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
D0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
E0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
F0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

















Memory SPD
------------------------------------------------------------------------------

DIMM #1

General
Memory type DDR
Manufacturer (ID) Micron Technology (2CFFFFFFFFFFFFFF)
Size 256 MBytes
Max bandwidth PC3200 (200 MHz)
Part number 8VDDT3264AG-40BG5
Serial number 74759A0A
Manufacturing date Week 41/Year 04

Attributes
Number of banks 1
Data width 64 bits
Correction None
Registered no
Buffered no
Nominal Voltage 2.50 Volts
EPP no
XMP no

Timings table
Frequency (MHz) 133 166 200
CAS# 2.0 2.5 3.0
RAS# to CAS# delay 2 3 3
RAS# Precharge 2 3 3
TRAS 6 7 8


DIMM #2

General
Memory type DDR
Manufacturer (ID) Micron Technology (2CFFFFFFFFFFFFFF)
Size 256 MBytes
Max bandwidth PC3200 (200 MHz)
Part number 8VDDT3264AG-40BG5
Serial number 74759A6D
Manufacturing date Week 41/Year 04

Attributes
Number of banks 1
Data width 64 bits
Correction None
Registered no
Buffered no
Nominal Voltage 2.50 Volts
EPP no
XMP no

Timings table
Frequency (MHz) 133 166 200
CAS# 2.0 2.5 3.0
RAS# to CAS# delay 2 3 3
RAS# Precharge 2 3 3
TRAS 6 7 8


Dump Module #1
0 1 2 3 4 5 6 7 8 9 A B C D E F
00 80 08 07 0D 0A 01 40 00 04 50 70 00 82 08 00 01
10 0E 04 1C 01 02 20 C0 60 70 75 75 3C 28 3C 28 40
20 60 60 40 40 00 00 00 00 00 37 46 30 28 50 00 01
30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 11 80
40 2C FF FF FF FF FF FF FF 08 38 56 44 44 54 33 32
50 36 34 41 47 2D 34 30 42 47 35 20 05 00 04 29 74
60 75 9A 0A 00 00 00 00 00 00 00 00 00 00 00 00 00
70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
80 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
90 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
A0 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
B0 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
C0 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
D0 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
E0 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
F0 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF


Dump Module #2
0 1 2 3 4 5 6 7 8 9 A B C D E F
00 80 08 07 0D 0A 01 40 00 04 50 70 00 82 08 00 01
10 0E 04 1C 01 02 20 C0 60 70 75 75 3C 28 3C 28 40
20 60 60 40 40 00 00 00 00 00 37 46 30 28 50 00 01
30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 11 80
40 2C FF FF FF FF FF FF FF 08 38 56 44 44 54 33 32
50 36 34 41 47 2D 34 30 42 47 35 20 05 00 04 29 74
60 75 9A 6D 00 00 00 00 00 00 00 00 00 00 00 00 00
70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
80 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
90 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
A0 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
B0 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
C0 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
D0 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
E0 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
F0 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF

















------------------------------------------------------------------------

DIMM #1

General
Memory type		DDR2
Module format		Regular UDIMM
Manufacturer (ID)	Corsair (7F7F9E0000000000)
Size			1024 MBytes
Max bandwidth		PC2-6400 (400 MHz)
Part number		CM2X1024-6400C4
Manufacturing date	Week 19/Year 06

Attributes
Number of banks		2
Data width		64 bits
Correction		None
Nominal Voltage		1.80 Volts
EPP			yes (2 profiles)
XMP			no

Timings table
Frequency (MHz)		270	400
CAS#			4.0	5.0
RAS# to CAS# delay	4	5
RAS# Precharge		4	5
TRAS			13	18
TRC			15	22

EPP profile 1 (full)
Voltage level		2.000 Volts
Address Command Rate	2T
Cycle time		2.500 ns (400.0 MHz)
tCL			-65.7 clocks
tRCD			4 clocks (10.00 ns)
tRP			4 clocks (10.00 ns)
tRAS			12 clocks (30.00 ns)
tRC			22 clocks (55.00 ns)
tWR			6 clocks (15.00 ns)

EPP profile 2 (full)
Voltage level		2.200 Volts
Address Command Rate	2T
Cycle time		1.875 ns (533.3 MHz)
tCL			-1200662773760.0 clocks
tRCD			6 clocks (9.50 ns)
tRP			6 clocks (9.50 ns)
tRAS			15 clocks (28.00 ns)
tRC			30 clocks (55.00 ns)
tWR			8 clocks (15.00 ns)


DIMM #2

General
Memory type		DDR2
Module format		Regular UDIMM
Manufacturer (ID)	Corsair (7F7F9E0000000000)
Size			1024 MBytes
Max bandwidth		PC2-6400 (400 MHz)
Part number		CM2X1024-6400C4
Manufacturing date	Week 19/Year 06

Attributes
Number of banks		2
Data width		64 bits
Correction		None
Nominal Voltage		1.80 Volts
EPP			yes (2 profiles)
XMP			no

Timings table
Frequency (MHz)		270	400
CAS#			4.0	5.0
RAS# to CAS# delay	4	5
RAS# Precharge		4	5
TRAS			13	18
TRC			15	22

EPP profile 1 (full)
Voltage level		2.000 Volts
Address Command Rate	2T
Cycle time		2.500 ns (400.0 MHz)
tCL			0.0 clocks
tRCD			4 clocks (10.00 ns)
tRP			4 clocks (10.00 ns)
tRAS			12 clocks (30.00 ns)
tRC			22 clocks (55.00 ns)
tWR			6 clocks (15.00 ns)

EPP profile 2 (full)
Voltage level		2.200 Volts
Address Command Rate	2T
Cycle time		1.875 ns (533.3 MHz)
tCL			-0.0 clocks
tRCD			6 clocks (9.50 ns)
tRP			6 clocks (9.50 ns)
tRAS			15 clocks (28.00 ns)
tRC			30 clocks (55.00 ns)
tWR			8 clocks (15.00 ns)















Memory SPD
------------------------------------------------------------------------------

DIMM #1

General
Memory type		DDR2
Module format		Regular UDIMM
Manufacturer (ID)	Corsair (7F7F9E0000000000)
Size			1024 MBytes
Max bandwidth		PC2-6400 (400 MHz)
Part number		CM2X1024-6400C4
Manufacturing date	Week 19/Year 06

Attributes
Number of banks		2
Data width		64 bits
Correction		None
Nominal Voltage		1.80 Volts
EPP			yes (2 profiles)
XMP			no

Timings table
Frequency (MHz)		270	400
CAS#			4.0	5.0
RAS# to CAS# delay	4	5
RAS# Precharge		4	5
TRAS			13	18
TRC			15	22

EPP profile 1 (full)
Voltage level		2.000 Volts
Address Command Rate	2T
Cycle time		2.500 ns (400.0 MHz)
tCL			-65.7 clocks
tRCD			4 clocks (10.00 ns)
tRP			4 clocks (10.00 ns)
tRAS			12 clocks (30.00 ns)
tRC			22 clocks (55.00 ns)
tWR			6 clocks (15.00 ns)

EPP profile 2 (full)
Voltage level		2.200 Volts
Address Command Rate	2T
Cycle time		1.875 ns (533.3 MHz)
tCL			-1200662773760.0 clocks
tRCD			6 clocks (9.50 ns)
tRP			6 clocks (9.50 ns)
tRAS			15 clocks (28.00 ns)
tRC			30 clocks (55.00 ns)
tWR			8 clocks (15.00 ns)


DIMM #2

General
Memory type		DDR2
Module format		Regular UDIMM
Manufacturer (ID)	Corsair (7F7F9E0000000000)
Size			1024 MBytes
Max bandwidth		PC2-6400 (400 MHz)
Part number		CM2X1024-6400C4
Manufacturing date	Week 19/Year 06

Attributes
Number of banks		2
Data width		64 bits
Correction		None
Nominal Voltage		1.80 Volts
EPP			yes (2 profiles)
XMP			no

Timings table
Frequency (MHz)		270	400
CAS#			4.0	5.0
RAS# to CAS# delay	4	5
RAS# Precharge		4	5
TRAS			13	18
TRC			15	22

EPP profile 1 (full)
Voltage level		2.000 Volts
Address Command Rate	2T
Cycle time		2.500 ns (400.0 MHz)
tCL			0.0 clocks
tRCD			4 clocks (10.00 ns)
tRP			4 clocks (10.00 ns)
tRAS			12 clocks (30.00 ns)
tRC			22 clocks (55.00 ns)
tWR			6 clocks (15.00 ns)

EPP profile 2 (full)
Voltage level		2.200 Volts
Address Command Rate	2T
Cycle time		1.875 ns (533.3 MHz)
tCL			-0.0 clocks
tRCD			6 clocks (9.50 ns)
tRP			6 clocks (9.50 ns)
tRAS			15 clocks (28.00 ns)
tRC			30 clocks (55.00 ns)
tWR			8 clocks (15.00 ns)


Dump Module #1
0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
00   80 08 08 0E 0A 61 40 00 05 25 45 00 82 08 00 00
10   0C 04 30 03 02 00 03 37 50 00 00 32 1E 32 2D 80
20   20 27 10 17 3C 1E 1E 00 00 37 4B 80 18 22 00 56
30   78 6F 3F 26 31 50 2E 65 30 3E 00 00 00 00 12 98
40   7F 7F 9E 00 00 00 00 00 01 43 4D 32 58 31 30 32
50   34 2D 36 34 30 30 43 34 20 20 20 20 20 06 19 00
60   00 00 00 6D 56 4E B1 30 88 50 01 20 25 25 8C 28
70   28 1E 3C 37 90 50 01 20 25 1E 8C 26 26 1C 3C 37
80   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
90   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
A0   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
B0   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
C0   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
D0   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
E0   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
F0   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF


Dump Module #2
0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
00   80 08 08 0E 0A 61 40 00 05 25 45 00 82 08 00 00
10   0C 04 30 03 02 00 03 37 50 00 00 32 1E 32 2D 80
20   20 27 10 17 3C 1E 1E 00 00 37 4B 80 18 22 00 56
30   78 6F 3F 26 31 50 2E 65 30 3E 00 00 00 00 12 98
40   7F 7F 9E 00 00 00 00 00 01 43 4D 32 58 31 30 32
50   34 2D 36 34 30 30 43 34 20 20 20 20 20 06 19 00
60   00 00 00 6D 56 4E B1 30 88 50 01 20 25 25 8C 28
70   28 1E 3C 37 90 50 01 20 25 1E 8C 26 26 1C 3C 37
80   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
90   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
A0   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
B0   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
C0   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
D0   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
E0   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
F0   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF










DIMM #1

General
Memory type SDRAM
Manufacturer (ID) (0000000000000000)
Size 128 MBytes
Max bandwidth PC133 (133 MHz)
Part number
Manufacturing date Week 00/Year 00

Attributes
Number of banks 1
Data width 64 bits
Correction None
Registered no
Buffered no

Timings table
Frequency (MHz) 100 133
CAS# 2.0 3.0
RAS# to CAS# delay 2 3
RAS# Precharge 2 3
TRAS# 5 6


DIMM #2

General
Memory type SDRAM
Manufacturer (ID) (0000000000000000)
Size 256 MBytes
Max bandwidth PC133 (133 MHz)
Part number
Manufacturing date Week 00/Year 00

Attributes
Number of banks 1
Data width 64 bits
Correction None
Registered no
Buffered no

Timings table
Frequency (MHz) 133
CAS# 3.0
RAS# to CAS# delay 3
RAS# Precharge 3
TRAS# 6


Dump Module #1
0 1 2 3 4 5 6 7 8 9 A B C D E F
00 80 08 04 0C 0A 01 40 00 01 75 54 00 80 08 00 01
10 8F 04 06 01 01 00 0E A0 60 00 00 14 0F 14 2D 20
20 15 08 15 08 00 00 00 00 00 00 00 00 00 00 00 00
30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 12 AF
40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
50 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
60 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 64 F6


Dump Module #2
0 1 2 3 4 5 6 7 8 9 A B C D E F
00 80 08 04 0D 0A 01 40 00 01 75 54 00 82 08 00 01
10 8F 04 04 01 01 00 0E A0 60 00 00 14 0F 14 2C 40
20 15 08 15 08 00 00 00 00 00 42 00 00 00 00 00 00
30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 10 0F
40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
50 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
60 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 64 FD












Memory SPD
------------------------------------------------------------------------------

DIMM #1

General
Memory type DDR-SDRAM
Manufacturer (ID) Samsung (CE00000000000000)
Size 512 MBytes
Max bandwidth PC2700 (166 MHz)
Part number M4 70L6524CU0-CB3
Serial number 410B2361
Manufacturing date Week 37/Year 05

Attributes
Number of banks 2
Data width 64 bits
Correction None
Registered no
Buffered no

Timings table
Frequency (MHz) 133 166
CAS# 2.0 2.5
RAS# to CAS# delay 3 3
RAS# Precharge 3 3
TRAS# 6 7


DIMM #2

General
Memory type DDR-SDRAM
Manufacturer (ID) (127F000000000000)
Size 512 MBytes
Max bandwidth PC2700 (166 MHz)
Part number
Manufacturing date Week 00/Year 00

Attributes
Number of banks 2
Data width 64 bits
Correction None
Registered no
Buffered no

Timings table
Frequency (MHz) 133 166
CAS# 2.0 2.5
RAS# to CAS# delay 3 3
RAS# Precharge 3 3
TRAS# 6 7


Dump Module #1
0 1 2 3 4 5 6 7 8 9 A B C D E F
00 80 08 07 0D 0A 02 40 00 04 60 70 00 82 10 00 01
10 0E 04 0C 01 02 20 C0 75 70 00 00 48 30 48 2A 40
20 80 80 45 45 00 00 00 00 00 3C 48 30 2D 55 00 00
30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 10 2F
40 CE 00 00 00 00 00 00 00 01 4D 34 20 37 30 4C 36
50 35 32 34 43 55 30 2D 43 42 33 20 30 43 05 25 41
60 0B 23 61 00 58 43 44 31 30 30 36 00 00 00 00 00
70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00


Dump Module #2
0 1 2 3 4 5 6 7 8 9 A B C D E F
00 80 08 07 0D 0A 02 40 00 04 60 70 00 82 10 00 01
10 0E 04 0C 01 02 20 C0 75 70 00 00 48 30 48 2A 40
20 80 80 45 45 00 00 00 00 00 3C 48 30 2D 60 00 00
30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 02 2C
40 12 7F 00 00 00 00 00 00 00 00 00 4B 36 34 36 34
50 55 35 31 43 35 33 33 33 4B 00 00 00 00 00 00 00
60 00 00 00 54 46 36 32 34 41 00 00 00 00 00 00 00
70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FF FF













Memory SPD
------------------------------------------------------------------------------

DIMM #1

General
Memory type DDR-SDRAM
Manufacturer (ID) Xerox (4341542500000000)
Size 512 MBytes
Max bandwidth PC2700 (166 MHz)
Part number
Serial number FFFFFFFF
Manufacturing date Week 37/Year 02

Attributes
Number of banks 2
Data width 64 bits
Correction None
Registered no
Buffered no

Timings table
Frequency (MHz) 166
CAS# 2.5
RAS# to CAS# delay 3
RAS# Precharge 3
TRAS# 7


Dump Module #1
0 1 2 3 4 5 6 7 8 9 A B C D E F
00 80 08 07 0D 0A 02 40 00 04 60 70 00 80 08 00 01
10 0E 04 08 01 02 20 00 75 70 00 00 48 30 48 2A 40
20 75 75 45 45 00 00 00 00 00 00 00 00 00 00 00 00
30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 05
40 43 41 54 25 00 00 00 00 02 00 00 00 00 00 00 00
50 00 00 00 00 00 00 00 00 00 00 00 00 00 02 25 FF
60 FF FF FF 00 00 00 00 00 00 00 00 00 00 00 00 00
70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00




















Memory SPD
------------------------------------------------------------------------------

DIMM #1

General
Memory type SDRAM
Manufacturer (ID) Xerox (4341550000000000)
Size 128 MBytes
Max bandwidth PC133 (133 MHz)
Part number

Attributes
Number of banks 2
Data width 64 bits
Correction None
Registered no
Buffered no
EPP no

Timings table
Frequency (MHz) 100 133
CAS# 2.0 3.0
RAS# to CAS# delay 2 3
RAS# Precharge 2 3
TRAS 5 6



DIMM #2

General
Memory type SDRAM
Manufacturer (ID) (0000000000000000)
Size 128 MBytes
Max bandwidth PC100 (100 MHz)
Part number

Attributes
Number of banks 2
Data width 64 bits
Correction None
Registered no
Buffered no
EPP no

Timings table
Frequency (MHz) 100
CAS# 3.0
RAS# to CAS# delay 2
RAS# Precharge 3
TRAS 5



Dump Module #1
0 1 2 3 4 5 6 7 8 9 A B C D E F
00 80 08 04 0C 09 02 40 00 01 75 54 00 80 08 00 01
10 8F 04 06 01 01 00 0E A0 60 00 00 14 0F 14 2D 10
20 15 08 15 08 00 00 00 00 00 00 00 00 00 00 00 00
30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 12 9F
40 43 41 55 00 00 00 00 00 01 00 00 00 00 00 00 00
50 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
60 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 64 F5


Dump Module #2
0 1 2 3 4 5 6 7 8 9 A B C D E F
00 80 08 04 0C 09 02 40 00 01 A0 60 00 80 08 00 01
10 8F 04 04 01 01 00 0E A0 60 00 00 18 14 14 32 10
20 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 12 A8
40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
50 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
60 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 64 F4























Memory SPD
------------------------------------------------------------------------------

DIMM #1

General
Memory type		DDR3
Module format		UDIMM
Manufacturer (ID)	Corsair (7F7F9E0000000000)
Size			2048 MBytes
Max bandwidth		PC3-8500F (533 MHz)
Part number		CM3X2G1600C9DHX

Attributes
Number of banks		8
Nominal Voltage		1.50 Volts
EPP			no
XMP			no

Timings table
Frequency (MHz)		457	533	610	686
CAS#			6.0	7.0	8.0	9.0
RAS# to CAS# delay	6	7	9	10
RAS# Precharge		6	7	9	10
TRAS			18	20	23	26
TRC			24	27	31	35

DIMM #2

General
Memory type		DDR3
Module format		UDIMM
Manufacturer (ID)	Corsair (7F7F9E0000000000)
Size			2048 MBytes
Max bandwidth		PC3-8500F (533 MHz)
Part number		CM3X2G1600C9DHX

Attributes
Number of banks		8
Nominal Voltage		1.50 Volts
EPP			no
XMP			no

Timings table
Frequency (MHz)		457	533	610	686
CAS#			6.0	7.0	8.0	9.0
RAS# to CAS# delay	6	7	9	10
RAS# Precharge		6	7	9	10
TRAS			18	20	23	26
TRC			24	27	31	35

Dump Module #1
0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
00   92 10 0B 02 02 11 00 09 03 51 01 08 0F 00 3C 00
10   69 78 69 3C 69 11 2C 95 70 03 3C 3C 00 F0 83 05
20   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
30   00 00 00 00 00 00 00 00 00 00 00 00 1F 11 01 01
40   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
50   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
60   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70   00 00 00 00 00 02 9E 01 00 00 00 00 00 00 6D 61
80   43 4D 33 58 32 47 31 36 30 30 43 39 44 48 58 20
90   20 20 00 00 00 00 00 00 00 00 00 00 00 00 00 00
A0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
B0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
C0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
D0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
E0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
F0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00


Dump Module #2
0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
00   92 10 0B 02 02 11 00 09 03 51 01 08 0F 00 3C 00
10   69 78 69 3C 69 11 2C 95 70 03 3C 3C 00 F0 83 05
20   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
30   00 00 00 00 00 00 00 00 00 00 00 00 1F 11 01 01
40   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
50   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
60   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70   00 00 00 00 00 02 9E 01 00 00 00 00 00 00 6D 61
80   43 4D 33 58 32 47 31 36 30 30 43 39 44 48 58 20
90   20 20 00 00 00 00 00 00 00 00 00 00 00 00 00 00
A0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
B0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
C0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
D0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
E0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
F0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

























Memory SPD
------------------------------------------------------------------------------

DIMM #1

General
Memory type             DDR
Manufacturer (ID)       Corsair (7F7F9E0000000000)
Size                    512 MBytes
Max bandwidth           PC3200 (200 MHz)
Part number             VS512MB400

Attributes
Number of banks         2
Data width              64 bits
Correction              None
Registered              no
Buffered                no
Nominal Voltage         2.50 Volts
EPP                     no
XMP                     no

Timings table
Frequency (MHz)         200
CAS#                    2.5
RAS# to CAS# delay      3
RAS# Precharge          3
TRAS                    8


DIMM #2

General
Memory type             DDR
Manufacturer (ID)       Kingston (7F98000000000000)
Size                    512 MBytes
Max bandwidth           PC2700 (166 MHz)
Part number             K
Serial number           501006AF
Manufacturing date      Week 14/Year 05

Attributes
Number of banks         1
Data width              64 bits
Correction              None
Registered              no
Buffered                no
Nominal Voltage         2.50 Volts
EPP                     no
XMP                     no

Timings table
Frequency (MHz)         133     166
CAS#                    2.0     2.5
RAS# to CAS# delay      3       3
RAS# Precharge          3       3
TRAS                    6       7


DIMM #3

General
Memory type             DDR
Manufacturer (ID)       Corsair (7F7F9E0000000000)
Size                    512 MBytes
Max bandwidth           PC3200 (200 MHz)
Part number             VS512MB400

Attributes
Number of banks         2
Data width              64 bits
Correction              None
Registered              no
Buffered                no
Nominal Voltage         2.50 Volts
EPP                     no
XMP                     no

Timings table
Frequency (MHz)         200
CAS#                    2.5
RAS# to CAS# delay      3
RAS# Precharge          3
TRAS                    8


DIMM #4

General
Memory type             DDR
Manufacturer (ID)       Corsair (7F7F9E0000000000)
Size                    512 MBytes
Max bandwidth           PC3200 (200 MHz)
Part number             VS512MB400

Attributes
Number of banks         2
Data width              64 bits
Correction              None
Registered              no
Buffered                no
Nominal Voltage         2.50 Volts
EPP                     no
XMP                     no

Timings table
Frequency (MHz)         200
CAS#                    2.5
RAS# to CAS# delay      3
RAS# Precharge          3
TRAS                    8


Dump Module #1
0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
00   80 08 07 0D 0A 02 40 00 04 50 60 00 82 08 00 01
10   0E 04 08 01 02 20 00 60 70 75 75 3C 28 3C 28 40
20   60 60 40 40 00 00 00 00 00 37 46 28 28 55 00 00
30   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 88
40   7F 7F 9E 00 00 00 00 00 01 56 53 35 31 32 4D 42
50   34 30 30 20 20 20 20 20 20 20 20 00 00 00 00 00
60   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
80   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
90   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
A0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
B0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
C0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
D0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
E0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
F0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00


Dump Module #2
0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
00   80 08 07 0D 0B 01 40 00 04 60 70 00 82 08 00 01
10   0E 04 0C 01 02 20 00 75 70 00 00 48 30 48 2A 80
20   75 75 45 45 00 00 00 00 00 3C 48 30 2D 60 00 00
30   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 8C
40   7F 98 00 00 00 00 00 00 00 4B 00 00 00 00 00 00
50   00 00 00 00 00 00 00 00 00 00 00 00 00 05 0E 50
60   10 06 AF 00 00 00 00 00 00 00 00 00 00 00 00 00
70   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
80   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
90   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
A0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
B0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
C0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
D0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
E0   4B 54 32 31 30 30 44 00 00 00 00 00 00 00 00 02
F0   39 39 30 35 31 39 32 2D 30 34 38 2E 41 30 31 00


Dump Module #3
0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
00   80 08 07 0D 0A 02 40 00 04 50 60 00 82 08 00 01
10   0E 04 08 01 02 20 00 60 70 75 75 3C 28 3C 28 40
20   60 60 40 40 00 00 00 00 00 37 46 28 28 55 00 00
30   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 88
40   7F 7F 9E 00 00 00 00 00 01 56 53 35 31 32 4D 42
50   34 30 30 20 20 20 20 20 20 20 20 00 00 00 00 00
60   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
80   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
90   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
A0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
B0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
C0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
D0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
E0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
F0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00


Dump Module #4
0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
00   80 08 07 0D 0A 02 40 00 04 50 60 00 82 08 00 01
10   0E 04 08 01 02 20 00 60 70 75 75 3C 28 3C 28 40
20   60 60 40 40 00 00 00 00 00 37 46 28 28 55 00 00
30   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 88
40   7F 7F 9E 00 00 00 00 00 01 56 53 35 31 32 4D 42
50   34 30 30 20 20 20 20 20 20 20 20 00 00 00 00 00
60   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
80   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
90   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
A0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
B0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
C0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
D0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
E0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
F0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00



















Memory SPD
------------------------------------------------------------------------------

DIMM #1

General
Memory type DDR2
Module format Regular UDIMM
Manufacturer (ID) OCZ (7F7F7F7FB0000000)
Size 2048 MBytes
Max bandwidth PC2-6400 (400 MHz)
Part number OCZ2G8002G

Attributes
Number of banks 2
Data width 64 bits
Correction None
Nominal Voltage 1.80 Volts
EPP no
XMP no

Timings table
Frequency (MHz) 333 400
CAS# 4.0 5.0
RAS# to CAS# delay 5 6
RAS# Precharge 5 6
TRAS 15 18
TRC 20 24

DIMM #2

General
Memory type DDR2
Module format Regular UDIMM
Manufacturer (ID) OCZ (7F7F7F7FB0000000)
Size 2048 MBytes
Max bandwidth PC2-6400 (400 MHz)
Part number OCZ2G8002G

Attributes
Number of banks 2
Data width 64 bits
Correction None
Nominal Voltage 1.80 Volts
EPP no
XMP no

Timings table
Frequency (MHz) 333 400
CAS# 4.0 5.0
RAS# to CAS# delay 5 6
RAS# Precharge 5 6
TRAS 15 18
TRC 20 24

Dump Module #1
0 1 2 3 4 5 6 7 8 9 A B C D E F
00 80 08 08 0E 0A 61 40 00 05 25 40 00 82 08 00 00
10 0C 08 70 01 02 00 07 30 50 3D 60 3C 1E 3C 2D 01
20 17 25 05 12 3C 1E 1E 00 06 3C 70 80 14 1E 00 00
30 00 03 00 00 00 00 00 00 00 00 00 00 00 00 10 EE
40 7F 7F 7F 7F B0 00 00 00 02 4F 43 5A 32 47 38 30
50 30 32 47 20 20 20 20 20 20 20 20 02 AC 00 00 00
60 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
80 07 0C 19 00 00 00 00 00 00 00 00 00 00 00 00 00
90 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
A0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
B0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
C0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
D0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
E0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
F0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00


Dump Module #2
0 1 2 3 4 5 6 7 8 9 A B C D E F
00 80 08 08 0E 0A 61 40 00 05 25 40 00 82 08 00 00
10 0C 08 70 01 02 00 07 30 50 3D 60 3C 1E 3C 2D 01
20 17 25 05 12 3C 1E 1E 00 06 3C 70 80 14 1E 00 00
30 00 03 00 00 00 00 00 00 00 00 00 00 00 00 10 EE
40 7F 7F 7F 7F B0 00 00 00 02 4F 43 5A 32 47 38 30
50 30 32 47 20 20 20 20 20 20 20 20 02 AC 00 00 00
60 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
80 07 0C 19 00 00 00 00 00 00 00 00 00 00 00 00 00
90 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
A0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
B0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
C0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
D0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
E0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
F0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00












Memory SPD
------------------------------------------------------------------------------

DIMM #2

General
Memory type		DDR
Manufacturer (ID)	Micron Technology (2CFFFFFFFFFFFFFF)
Size			512 MBytes
Max bandwidth		PC3200 (200 MHz)
Part number		16VDDT6464AG-40BG5
Serial number		290D60A4
Manufacturing date	Week 21/Year 05

Attributes
Number of banks		2
Data width		64 bits
Correction		None
Registered		no
Buffered		no
EPP			no

Timings table
Frequency (MHz)		133	166	200
CAS#			2.0	2.5	3.0
RAS# to CAS# delay	2	3	3
RAS# Precharge		2	3	3
TRAS			6	7	8



DIMM #3

General
Memory type		DDR
Manufacturer (ID)	Micron Technology (2CFFFFFFFFFFFFFF)
Size			256 MBytes
Max bandwidth		PC2700 (166 MHz)
Part number		8VDDT3264AG-335GB
Serial number		231788B4
Manufacturing date	Week 26/Year 04

Attributes
Number of banks		1
Data width		64 bits
Correction		None
Registered		no
Buffered		no
EPP			no

Timings table
Frequency (MHz)		133	166
CAS#			2.0	2.5
RAS# to CAS# delay	3	3
RAS# Precharge		3	3
TRAS			6	7



Dump Module #1
0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
00   80 08 07 0D 0A 02 40 00 04 50 70 00 82 08 00 01
10   0E 04 1C 01 02 20 C0 60 70 75 75 3C 28 3C 28 40
20   60 60 40 40 00 00 00 00 00 37 46 30 28 50 00 01
30   00 00 00 00 00 00 00 00 00 00 00 00 00 00 11 81
40   2C FF FF FF FF FF FF FF 09 31 36 56 44 44 54 36
50   34 36 34 41 47 2D 34 30 42 47 35 05 00 05 15 29
60   0D 60 A4 00 00 00 00 00 00 00 00 00 00 00 00 00
70   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00


Dump Module #2
0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
00   80 08 07 0D 0A 01 40 00 04 60 70 00 82 08 00 01
10   0E 04 0C 01 02 20 C0 75 70 00 00 48 30 48 2A 40
20   80 80 45 45 00 00 00 00 00 3C 48 30 2D 55 00 11
30   00 00 00 00 00 00 00 00 00 00 00 00 00 00 10 37
40   2C FF FF FF FF FF FF FF 09 38 56 44 44 54 33 32
50   36 34 41 47 2D 33 33 35 47 42 20 0B 00 04 1A 23
60   17 88 B4 00 00 00 00 00 00 00 00 00 00 00 00 00
70   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00













== Memory Device (type 17, handle 0023, length 27) ==
Array Handle = 0021
Error Info Handle = FFFE
Total Width (bits) = 72
Data Width (bits) = 64
Size = 128 MB
Form Factor = DIMM
Device Set = 0
Device Locator = XMM1
Bank Locator =
Memory Type = SDRAM
Type Detail = |Synchronous|
Speed (MHz) = 100
Manufacturer = JEDEC ID:C1 49 4E 46 49 4E 45 4F
Serial Number = FE830607
Asset Tag =
Part Number = HYS72V1600GR-8....

== Memory Device (type 17, handle 0024, length 27) ==
Array Handle = 0021
Error Info Handle = FFFE
Total Width (bits) = 64
Data Width (bits) = 64
Size = 256 MB
Form Factor = DIMM
Device Set = 0
Device Locator = XMM2
Bank Locator =
Memory Type = SDRAM
Type Detail = |Synchronous|
Speed (MHz) = 100
Manufacturer = JEDEC ID:2C FF FF FF FF FF FF FF
Serial Number = AE2D1021
Asset Tag =
Part Number = 16LSDT3264AG-10EE1

== Memory Device (type 17, handle 0025, length 27) ==
Array Handle = 0021
Error Info Handle = FFFE
Total Width (bits) = 72
Data Width (bits) = 64
Size = 128 MB
Form Factor = DIMM
Device Set = 0
Device Locator = XMM3
Bank Locator =
Memory Type = SDRAM
Type Detail = |Synchronous|
Speed (MHz) = 100
Manufacturer = JEDEC ID:C1 49 4E 46 49 4E 45 4F
Serial Number = 9B520D01
Asset Tag =
Part Number = HYS72V1600GR-8....

== Memory Device (type 17, handle 0028, length 27) ==
Array Handle = 0022
Error Info Handle = FFFE
Total Width (bits) = 4
Data Width (bits) = 4
Size = 512 KB
Form Factor = Chip
Device Set = 0
Device Locator = XU15
Bank Locator =
Memory Type = FLASH
Type Detail = |Non-volatile|
Speed (MHz) = 0
Manufacturer =
Serial Number =
Asset Tag =
Part Number =
















MemoryDeviceManufacturer[1]="JEDEC ID:C1 00 00 00 00 00 00 00"
MemoryDeviceSerialNumber[1]="13720101"
MemoryDevicePartNumber[1]="64T32000HU3.7A"
MemoryDeviceManufacturer[2]="JEDEC ID:"
MemoryDeviceSerialNumber[2]=" "
MemoryDevicePartNumber[2]=" "
MemoryDeviceManufacturer[3]="JEDEC ID:C1 00 00 00 00 00 00 00"
MemoryDeviceSerialNumber[3]="13E60101"
MemoryDevicePartNumber[3]="64T32000HU3.7A"
MemoryDeviceManufacturer[4]="JEDEC ID:"
MemoryDeviceSerialNumber[4]=" "
MemoryDevicePartNumber[4]=" "



















DIMM #3

General
Memory type		DDR2
Module format		Regular UDIMM
Manufacturer (ID)	Corsair (7F7F9E0000000000)
Size			2048 MBytes
Max bandwidth		PC2-6400 (400 MHz)
Part number		CM2X2048-8500C5D

Attributes
Number of banks		2
Data width		64 bits
Correction		None
Nominal Voltage		1.80 Volts
EPP			yes (1 profiles)
XMP			no

Timings table
Frequency (MHz)		270	400
CAS#			4.0	5.0
RAS# to CAS# delay	4	5
RAS# Precharge		4	5
TRAS			13	18
TRC			16	23

EPP profile 1 (full)
Voltage level		2.100 Volts
Address Command Rate	2T
Cycle time		1.875 ns (533.3 MHz)
tCL			5.0 clocks
tRCD			5 clocks (9.25 ns)
tRP			5 clocks (9.25 ns)
tRAS			15 clocks (28.00 ns)
tRC			22 clocks (41.00 ns)
tWR			8 clocks (15.00 ns)


DIMM #4

General
Memory type		DDR2
Module format		Regular UDIMM
Manufacturer (ID)	Corsair (7F7F9E0000000000)
Size			2048 MBytes
Max bandwidth		PC2-6400 (400 MHz)
Part number		CM2X2048-8500C5D

Attributes
Number of banks		2
Data width		64 bits
Correction		None
Nominal Voltage		1.80 Volts
EPP			yes (1 profiles)
XMP			no

Timings table
Frequency (MHz)		76	400
CAS#			4.0	5.0
RAS# to CAS# delay	1	5
RAS# Precharge		1	5
TRAS			4	18
TRC			5	23

EPP profile 1 (full)
Voltage level		2.100 Volts
Address Command Rate	2T
Cycle time		1.875 ns (533.3 MHz)
tCL			5.0 clocks
tRCD			5 clocks (9.25 ns)
tRP			5 clocks (9.25 ns)
tRAS			15 clocks (28.00 ns)
tRC			22 clocks (41.00 ns)
tWR			8 clocks (15.00 ns)

*/

////////////////////////////////////////////////////////////////////////////////////////
// SMBUS
////////////////////////////////////////////////////////////////////////////////////////
/*
1 #!/usr/bin/perl -w
2 #
3 # Copyright 1998, 1999 Philip Edelbrock <phil@netroedge.com>
4 # modified by Christian Zuckschwerdt <zany@triq.net>
5 # modified by Burkart Lingner <burkart@bollchen.de>
6 #
7 #    This program is free software; you can redistribute it and/or modify
8 #    it under the terms of the GNU General Public License as published by
9 #    the Free Software Foundation; either version 2 of the License, or
10 #    (at your option) any later version.
11 #
12 #    This program is distributed in the hope that it will be useful,
13 #    but WITHOUT ANY WARRANTY; without even the implied warranty of
14 #    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
15 #    GNU General Public License for more details.
16 #
17 #    You should have received a copy of the GNU General Public License
18 #    along with this program; if not, write to the Free Software
19 #    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
20 #
21 # Version 0.4  1999  Philip Edelbrock <phil@netroedge.com>
22 # Version 0.5  2000-03-30  Christian Zuckschwerdt <zany@triq.net>
23 #  html output (selectable by commandline switches)
24 # Version 0.6  2000-09-16  Christian Zuckschwerdt <zany@triq.net>
25 #  updated according to SPD Spec Rev 1.2B
26 #  see http://developer.intel.com/technology/memory/pc133sdram/spec/Spdsd12b.htm
27 # Version 0.7  2002-11-08  Jean Delvare <khali@linux-fr.org>
28 #  pass -w and use strict
29 #  valid HTML 3.2 output (--format mode)
30 #  miscellaneous formatting enhancements and bug fixes
31 #  clearer HTML output (original patch by Nick Kurshev <nickols_k@mail.ru>)
32 #  stop decoding on checksum error by default (--checksum option forces)
33 # Version 0.8  2005-06-20  Burkart Lingner <burkart@bollchen.de>
34 #  adapted to Kernel 2.6's /sys filesystem
35 # Version 0.9  2005-07-15  Jean Delvare <khali@linux-fr.org>
36 #  fix perl warning
37 #  fix typo
38 #  refactor some code
39 # Version 1.0  2005-09-18  Jean Delvare <khali@linux-fr.org>
40 #  add large lookup tables for manufacturer names, based on data
41 #  provided by Rudolf Marek, taken from:
42 #  http://www.jedec.org/download/search/JEP106r.pdf
43 # Version 1.1  2006-01-22  Jean Delvare <khali@linux-fr.org>
44 #  improve the text output, making it hopefully clearer
45 #  read eeprom by 64-byte blocks, this allows some code cleanups
46 #  use sysopen/sysread instead of open/read for better performance
47 #  verify checksum before decoding anything
48 # Version 1.2  2006-05-15  Jean Delvare <khali@linux-fr.org>
49 #  implement per-memory-type decoding
50 #  don't decode revision code, manufacturing date and assembly serial
51 #  number where not set
52 #  decode the manufacturing date to an ISO8601 date
53 # Version 1.3  2006-05-21  Jean Delvare <khali@linux-fr.org>
54 #  detect undefined manufacturer code and handle it properly
55 #  round up timing data
56 #  minor display adjustments
57 #  group cycle and access times, display the CAS value for each (SDRAM)
58 #  refactor some bitfield tests into loops (SDRAM)
59 #  display latencies and burst length on a single line (SDRAM)
60 #  don't display manufacturing location when undefined
61 #  check that the manufacturing date is proper BCD, else fall back to
62 #  hexadecimal display
63 # Version 1.4  2006-05-26  Jean Delvare <khali@linux-fr.org>
64 #  fix latencies decoding (SDRAM)
65 #  fix CAS latency decoding (DDR SDRAM)
66 #  decode latencies, timings and module height (DDR SDRAM)
67 #  decode size (Direct Rambus, Rambus)
68 #  decode latencies and timings (DDR2 SDRAM)
69 #  SPD revision decoding depends on memory type
70 #  use more user-friendly labels
71 #  fix HTML formatted output on checksum error
72 #
73 #
74 # EEPROM data decoding for SDRAM DIMM modules.
75 #
76 # Two assumptions: lm_sensors-2.x installed,
77 # and Perl is at /usr/bin/perl
78 #
79 # use the following command line switches
80 #  -f, --format            print nice html output
81 #  -b, --bodyonly          don't print html header
82 #                          (useful for postprocessing the output)
83 #  -c, --checksum          decode completely even if checksum fails
84 #  -h, --help              display this usage summary
85 #
86 # References:
87 # PC SDRAM Serial Presence
88 # Detect (SPD) Specification, Intel,
89 # 1997,1999, Rev 1.2B
90 #
91 # Jedec Standards 4.1.x & 4.5.x
92 # http://www.jedec.org
93 #
94
95 require 5.004;
96
97 use strict;
98 use POSIX;
99 use Fcntl qw(:DEFAULT :seek);
100 use vars qw($opt_html $opt_body $opt_bodyonly $opt_igncheck $use_sysfs
101             @vendors %decode_callback);
102
103 @vendors = (
104 ["AMD", "AMI", "Fairchild", "Fujitsu",
105  "GTE", "Harris", "Hitachi", "Inmos",
106  "Intel", "I.T.T.", "Intersil", "Monolithic Memories",
107  "Mostek", "Freescale (formerly Motorola)", "National", "NEC",
108  "RCA", "Raytheon", "Conexant (Rockwell)", "Seeq",
109  "Philips Semi. (Signetics)", "Synertek", "Texas Instruments", "Toshiba",
110  "Xicor", "Zilog", "Eurotechnique", "Mitsubishi",
111  "Lucent (AT&T)", "Exel", "Atmel", "SGS/Thomson",
112  "Lattice Semi.", "NCR", "Wafer Scale Integration", "IBM",
113  "Tristar", "Visic", "Intl. CMOS Technology", "SSSI",
114  "MicrochipTechnology", "Ricoh Ltd.", "VLSI", "Micron Technology",
115  "Hyundai Electronics", "OKI Semiconductor", "ACTEL", "Sharp",
116  "Catalyst", "Panasonic", "IDT", "Cypress",
117  "DEC", "LSI Logic", "Zarlink (formerly Plessey)", "UTMC",
118  "Thinking Machine", "Thomson CSF", "Integrated CMOS (Vertex)", "Honeywell",
119  "Tektronix", "Sun Microsystems", "SST", "ProMos/Mosel Vitelic",
120  "Infineon (formerly Siemens)", "Macronix", "Xerox", "Plus Logic",
121  "SunDisk", "Elan Circuit Tech.", "European Silicon Str.", "Apple Computer",
122  "Xilinx", "Compaq", "Protocol Engines", "SCI",
123  "Seiko Instruments", "Samsung", "I3 Design System", "Klic",
124  "Crosspoint Solutions", "Alliance Semiconductor", "Tandem", "Hewlett-Packard",
125  "Intg. Silicon Solutions", "Brooktree", "New Media", "MHS Electronic",
126  "Performance Semi.", "Winbond Electronic", "Kawasaki Steel", "Bright Micro",
127  "TECMAR", "Exar", "PCMCIA", "LG Semi (formerly Goldstar)",
128  "Northern Telecom", "Sanyo", "Array Microsystems", "Crystal Semiconductor",
129  "Analog Devices", "PMC-Sierra", "Asparix", "Convex Computer",
130  "Quality Semiconductor", "Nimbus Technology", "Transwitch", "Micronas (ITT Intermetall)",
131  "Cannon", "Altera", "NEXCOM", "QUALCOMM",
132  "Sony", "Cray Research", "AMS(Austria Micro)", "Vitesse",
133  "Aster Electronics", "Bay Networks (Synoptic)", "Zentrum or ZMD", "TRW",
134  "Thesys", "Solbourne Computer", "Allied-Signal", "Dialog",
135  "Media Vision", "Level One Communication"],
136 ["Cirrus Logic", "National Instruments", "ILC Data Device", "Alcatel Mietec",
137  "Micro Linear", "Univ. of NC", "JTAG Technologies", "Loral",
138  "Nchip", "Galileo Tech", "Bestlink Systems", "Graychip",
139  "GENNUM", "VideoLogic", "Robert Bosch", "Chip Express",
140  "DATARAM", "United Microelec Corp.", "TCSI", "Smart Modular",
141  "Hughes Aircraft", "Lanstar Semiconductor", "Qlogic", "Kingston",
142  "Music Semi", "Ericsson Components", "SpaSE", "Eon Silicon Devices",
143  "Programmable Micro Corp", "DoD", "Integ. Memories Tech.", "Corollary Inc.",
144  "Dallas Semiconductor", "Omnivision", "EIV(Switzerland)", "Novatel Wireless",
145  "Zarlink (formerly Mitel)", "Clearpoint", "Cabletron", "Silicon Technology",
146  "Vanguard", "Hagiwara Sys-Com", "Vantis", "Celestica",
147  "Century", "Hal Computers", "Rohm Company Ltd.", "Juniper Networks",
148  "Libit Signal Processing", "Mushkin Enhanced Memory", "Tundra Semiconductor", "Adaptec Inc.",
149  "LightSpeed Semi.", "ZSP Corp.", "AMIC Technology", "Adobe Systems",
150  "Dynachip", "PNY Electronics", "Newport Digital", "MMC Networks",
151  "T Square", "Seiko Epson", "Broadcom", "Viking Components",
152  "V3 Semiconductor", "Flextronics (formerly Orbit)", "Suwa Electronics", "Transmeta",
153  "Micron CMS", "American Computer & Digital Components Inc", "Enhance 3000 Inc", "Tower Semiconductor",
154  "CPU Design", "Price Point", "Maxim Integrated Product", "Tellabs",
155  "Centaur Technology", "Unigen Corporation", "Transcend Information", "Memory Card Technology",
156  "CKD Corporation Ltd.", "Capital Instruments, Inc.", "Aica Kogyo, Ltd.", "Linvex Technology",
157  "MSC Vertriebs GmbH", "AKM Company, Ltd.", "Dynamem, Inc.", "NERA ASA",
158  "GSI Technology", "Dane-Elec (C Memory)", "Acorn Computers", "Lara Technology",
159  "Oak Technology, Inc.", "Itec Memory", "Tanisys Technology", "Truevision",
160  "Wintec Industries", "Super PC Memory", "MGV Memory", "Galvantech",
161  "Gadzoox Nteworks", "Multi Dimensional Cons.", "GateField", "Integrated Memory System",
162  "Triscend", "XaQti", "Goldenram", "Clear Logic",
163  "Cimaron Communications", "Nippon Steel Semi. Corp.", "Advantage Memory", "AMCC",
164  "LeCroy", "Yamaha Corporation", "Digital Microwave", "NetLogic Microsystems",
165  "MIMOS Semiconductor", "Advanced Fibre", "BF Goodrich Data.", "Epigram",
166  "Acbel Polytech Inc.", "Apacer Technology", "Admor Memory", "FOXCONN",
167  "Quadratics Superconductor", "3COM"],
168 ["Camintonn Corporation", "ISOA Incorporated", "Agate Semiconductor", "ADMtek Incorporated",
169  "HYPERTEC", "Adhoc Technologies", "MOSAID Technologies", "Ardent Technologies",
170  "Switchcore", "Cisco Systems, Inc.", "Allayer Technologies", "WorkX AG",
171  "Oasis Semiconductor", "Novanet Semiconductor", "E-M Solutions", "Power General",
172  "Advanced Hardware Arch.", "Inova Semiconductors GmbH", "Telocity", "Delkin Devices",
173  "Symagery Microsystems", "C-Port Corporation", "SiberCore Technologies", "Southland Microsystems",
174  "Malleable Technologies", "Kendin Communications", "Great Technology Microcomputer", "Sanmina Corporation",
175  "HADCO Corporation", "Corsair", "Actrans System Inc.", "ALPHA Technologies",
176  "Silicon Laboratories, Inc. (Cygnal)", "Artesyn Technologies", "Align Manufacturing", "Peregrine Semiconductor",
177  "Chameleon Systems", "Aplus Flash Technology", "MIPS Technologies", "Chrysalis ITS",
178  "ADTEC Corporation", "Kentron Technologies", "Win Technologies", "Tachyon Semiconductor (formerly ASIC Designs Inc.)",
179  "Extreme Packet Devices", "RF Micro Devices", "Siemens AG", "Sarnoff Corporation",
180  "Itautec Philco SA", "Radiata Inc.", "Benchmark Elect. (AVEX)", "Legend",
181  "SpecTek Incorporated", "Hi/fn", "Enikia Incorporated", "SwitchOn Networks",
182  "AANetcom Incorporated", "Micro Memory Bank", "ESS Technology", "Virata Corporation",
183  "Excess Bandwidth", "West Bay Semiconductor", "DSP Group", "Newport Communications",
184  "Chip2Chip Incorporated", "Phobos Corporation", "Intellitech Corporation", "Nordic VLSI ASA",
185  "Ishoni Networks", "Silicon Spice", "Alchemy Semiconductor", "Agilent Technologies",
186  "Centillium Communications", "W.L. Gore", "HanBit Electronics", "GlobeSpan",
187  "Element 14", "Pycon", "Saifun Semiconductors", "Sibyte, Incorporated",
188  "MetaLink Technologies", "Feiya Technology", "I & C Technology", "Shikatronics",
189  "Elektrobit", "Megic", "Com-Tier", "Malaysia Micro Solutions",
190  "Hyperchip", "Gemstone Communications", "Anadigm (formerly Anadyne)", "3ParData",
191  "Mellanox Technologies", "Tenx Technologies", "Helix AG", "Domosys",
192  "Skyup Technology", "HiNT Corporation", "Chiaro", "MCI Computer GMBH",
193  "Exbit Technology A/S", "Integrated Technology Express", "AVED Memory", "Legerity",
194  "Jasmine Networks", "Caspian Networks", "nCUBE", "Silicon Access Networks",
195  "FDK Corporation", "High Bandwidth Access", "MultiLink Technology", "BRECIS",
196  "World Wide Packets", "APW", "Chicory Systems", "Xstream Logic",
197  "Fast-Chip", "Zucotto Wireless", "Realchip", "Galaxy Power",
198  "eSilicon", "Morphics Technology", "Accelerant Networks", "Silicon Wave",
199  "SandCraft", "Elpida"],
200 ["Solectron", "Optosys Technologies", "Buffalo (Formerly Melco)", "TriMedia Technologies",
201  "Cyan Technologies", "Global Locate", "Optillion", "Terago Communications",
202  "Ikanos Communications", "Princeton Technology", "Nanya Technology", "Elite Flash Storage",
203  "Mysticom", "LightSand Communications", "ATI Technologies", "Agere Systems",
204  "NeoMagic", "AuroraNetics", "Golden Empire", "Mushkin",
205  "Tioga Technologies", "Netlist", "TeraLogic", "Cicada Semiconductor",
206  "Centon Electronics", "Tyco Electronics", "Magis Works", "Zettacom",
207  "Cogency Semiconductor", "Chipcon AS", "Aspex Technology", "F5 Networks",
208  "Programmable Silicon Solutions", "ChipWrights", "Acorn Networks", "Quicklogic",
209  "Kingmax Semiconductor", "BOPS", "Flasys", "BitBlitz Communications",
210  "eMemory Technology", "Procket Networks", "Purple Ray", "Trebia Networks",
211  "Delta Electronics", "Onex Communications", "Ample Communications", "Memory Experts Intl",
212  "Astute Networks", "Azanda Network Devices", "Dibcom", "Tekmos",
213  "API NetWorks", "Bay Microsystems", "Firecron Ltd", "Resonext Communications",
214  "Tachys Technologies", "Equator Technology", "Concept Computer", "SILCOM",
215  "3Dlabs", "c't Magazine", "Sanera Systems", "Silicon Packets",
216  "Viasystems Group", "Simtek", "Semicon Devices Singapore", "Satron Handelsges",
217  "Improv Systems", "INDUSYS GmbH", "Corrent", "Infrant Technologies",
218  "Ritek Corp", "empowerTel Networks", "Hypertec", "Cavium Networks",
219  "PLX Technology", "Massana Design", "Intrinsity", "Valence Semiconductor",
220  "Terawave Communications", "IceFyre Semiconductor", "Primarion", "Picochip Designs Ltd",
221  "Silverback Systems", "Jade Star Technologies", "Pijnenburg Securealink", "MemorySolutioN",
222  "Cambridge Silicon Radio", "Swissbit", "Nazomi Communications", "eWave System",
223  "Rockwell Collins", "Picocel Co., Ltd.", "Alphamosaic Ltd", "Sandburst",
224  "SiCon Video", "NanoAmp Solutions", "Ericsson Technology", "PrairieComm",
225  "Mitac International", "Layer N Networks", "MtekVision", "Allegro Networks",
226  "Marvell Semiconductors", "Netergy Microelectronic", "NVIDIA", "Internet Machines",
227  "Peak Electronics", "Litchfield Communication", "Accton Technology", "Teradiant Networks",
228  "Europe Technologies", "Cortina Systems", "RAM Components", "Raqia Networks",
229  "ClearSpeed", "Matsushita Battery", "Xelerated", "SimpleTech",
230  "Utron Technology", "Astec International", "AVM gmbH", "Redux Communications",
231  "Dot Hill Systems", "TeraChip"],
232 ["T-RAM Incorporated", "Innovics Wireless", "Teknovus", "KeyEye Communications",
233  "Runcom Technologies", "RedSwitch", "Dotcast", "Silicon Mountain Memory",
234  "Signia Technologies", "Pixim", "Galazar Networks", "White Electronic Designs",
235  "Patriot Scientific", "Neoaxiom Corporation", "3Y Power Technology", "Europe Technologies",
236  "Potentia Power Systems", "C-guys Incorporated", "Digital Communications Technology Incorporated", "Silicon-Based Technology",
237  "Fulcrum Microsystems", "Positivo Informatica Ltd", "XIOtech Corporation", "PortalPlayer",
238  "Zhiying Software", "Direct2Data", "Phonex Broadband", "Skyworks Solutions",
239  "Entropic Communications", "Pacific Force Technology", "Zensys A/S", "Legend Silicon Corp.",
240  "sci-worx GmbH", "Oasis Silicon Systems", "Renesas Technology", "Raza Microelectronics",
241  "Phyworks", "MediaTek", "Non-cents Productions", "US Modular",
242  "Wintegra Ltd", "Mathstar", "StarCore", "Oplus Technologies",
243  "Mindspeed", "Just Young Computer", "Radia Communications", "OCZ",
244  "Emuzed", "LOGIC Devices", "Inphi Corporation", "Quake Technologies",
245  "Vixel", "SolusTek", "Kongsberg Maritime", "Faraday Technology",
246  "Altium Ltd.", "Insyte", "ARM Ltd.", "DigiVision",
247  "Vativ Technologies", "Endicott Interconnect Technologies", "Pericom", "Bandspeed",
248  "LeWiz Communications", "CPU Technology", "Ramaxel Technology", "DSP Group",
249  "Axis Communications", "Legacy Electronics", "Chrontel", "Powerchip Semiconductor",
250  "MobilEye Technologies", "Excel Semiconductor", "A-DATA Technology", "VirtualDigm",
251  "G Skill Intl", "Quanta Computer", "Yield Microelectronics", "Afa Technologies",
252  "KINGBOX Technology Co. Ltd.", "Ceva", "iStor Networks", "Advance Modules",
253  "Microsoft", "Open-Silicon", "Goal Semiconductor", "ARC International",
254  "Simmtec", "Metanoia", "Key Stream", "Lowrance Electronics",
255  "Adimos", "SiGe Semiconductor", "Fodus Communications", "Credence Systems Corp.",
256  "Genesis Microchip Inc.", "Vihana, Inc.", "WIS Technologies", "GateChange Technologies",
257  "High Density Devices AS", "Synopsys", "Gigaram", "Enigma Semiconductor Inc.",
258  "Century Micro Inc.", "Icera Semiconductor", "Mediaworks Integrated Systems", "O'Neil Product Development",
259  "Supreme Top Technology Ltd.", "MicroDisplay Corporation", "Team Group Inc.", "Sinett Corporation",
260  "Toshiba Corporation", "Tensilica", "SiRF Technology", "Bacoc Inc.",
261  "SMaL Camera Technologies", "Thomson SC", "Airgo Networks", "Wisair Ltd.",
262  "SigmaTel", "Arkados", "Compete IT gmbH Co. KG", "Eudar Technology Inc.",
263  "Focus Enhancements", "Xyratex"],
264 ["Specular Networks", "Patriot Memory", "U-Chip Technology Corp.", "Silicon Optix",
265  "Greenfield Networks", "CompuRAM GmbH", "Stargen, Inc.", "NetCell Corporation",
266  "Excalibrus Technologies Ltd", "SCM Microsystems", "Xsigo Systems, Inc.", "CHIPS & Systems Inc",
267  "Tier 1 Multichip Solutions", "CWRL Labs", "Teradici", "Gigaram, Inc.",
268  "g2 Microsystems", "PowerFlash Semiconductor", "P.A. Semi, Inc.", "NovaTech Solutions, S.A.",
269  "c2 Microsystems, Inc.", "Level5 Networks", "COS Memory AG", "Innovasic Semiconductor",
270  "02IC Co. Ltd", "Tabula, Inc.", "Crucial Technology", "Chelsio Communications",
271  "Solarflare Communications", "Xambala Inc.", "EADS Astrium", "ATO Semicon Co. Ltd.",
272  "Imaging Works, Inc.", "Astute Networks, Inc.", "Tzero", "Emulex",
273  "Power-One", "Pulse~LINK Inc.", "Hon Hai Precision Industry", "White Rock Networks Inc.",
274  "Telegent Systems USA, Inc.", "Atrua Technologies, Inc.", "Acbel Polytech Inc.",
275  "eRide Inc.","ULi Electronics Inc.", "Magnum Semiconductor Inc.", "neoOne Technology, Inc.",
276  "Connex Technology, Inc.", "Stream Processors, Inc.", "Focus Enhancements", "Telecis Wireless, Inc.",
277  "uNav Microelectronics", "Tarari, Inc.", "Ambric, Inc.", "Newport Media, Inc.", "VMTS",
278  "Enuclia Semiconductor, Inc.", "Virtium Technology Inc.", "Solid State System Co., Ltd.", "Kian Tech LLC",
279  "Artimi", "Power Quotient International", "Avago Technologies", "ADTechnology", "Sigma Designs",
280  "SiCortex, Inc.", "Ventura Technology Group", "eASIC", "M.H.S. SAS", "Micro Star International",
281  "Rapport Inc.", "Makway International", "Broad Reach Engineering Co.",
282  "Semiconductor Mfg Intl Corp", "SiConnect", "FCI USA Inc.", "Validity Sensors",
283  "Coney Technology Co. Ltd.", "Spans Logic", "Neterion Inc."]);
284
285 $use_sysfs = -d '/sys/bus';
286
287 # We consider that no data was written to this area of the SPD EEPROM if
288 # all bytes read 0x00 or all bytes read 0xff
289 sub spd_written(@)
290 {
291         my $all_00 = 1;
292         my $all_ff = 1;
293
294         foreach my $b (@_) {
295                 $all_00 = 0 unless $b == 0x00;
296                 $all_ff = 0 unless $b == 0xff;
297                 return 1 unless $all_00 or $all_ff;
298         }
299
300         return 0;
301 }
302
303 sub parity($)
304 {
305         my $n = shift;
306         my $parity = 0;
307
308         while ($n) {
309                 $parity++ if ($n & 1);
310                 $n >>= 1;
311         }
312
313         return ($parity & 1);
314 }
315
316 sub manufacturer(@)
317 {
318         my @bytes = @_;
319         my $ai = 0;
320         my $first;
321
322         return ("Undefined", []) unless spd_written(@bytes);
323
324         while (defined($first = shift(@bytes)) && $first == 0x7F) {
325                 $ai++;
326         }
327
328         return ("Invalid", []) unless defined $first;
329         return ("Invalid", [$first, @bytes]) if parity($first) != 1;
330         return ("Unknown", \@bytes) unless (($first & 0x7F) - 1 <= $vendors[$ai]);
331
332         return ($vendors[$ai][($first & 0x7F) - 1], \@bytes);
333 }
334
335 sub manufacturer_data(@)
336 {
337         my $hex = "";
338         my $asc = "";
339
340         return unless spd_written(@_);
341
342         foreach my $byte (@_) {
343                 $hex .= _stprintf("\%02X ", $byte);
344                 $asc .= ($byte >= 32 && $byte < 127) ? chr($byte) : '?';
345         }
346
347         return "$hex(\"$asc\")";
348 }
349
350 sub part_number(@)
351 {
352         my $asc = "";
353         my $byte;
354
355         while (defined ($byte = shift) && $byte >= 32 && $byte < 127) {
356                 $asc .= chr($byte);
357         }
358
359         return ($asc eq "") ? "Undefined" : $asc;
360 }
361
362 sub printl ($$) # print a line w/ label and value
363 {
364         my ($label, $value) = @_;
365         if ($opt_html) {
366                 $label =~ s/</\&lt;/sg;
367                 $label =~ s/>/\&gt;/sg;
368                 $label =~ s/\n/<br>\n/sg;
369                 $value =~ s/</\&lt;/sg;
370                 $value =~ s/>/\&gt;/sg;
371                 $value =~ s/\n/<br>\n/sg;
372                 print "<tr><td valign=top>$label</td><td>$value</td></tr>\n";
373         } else {
374                 my @values = split /\n/, $value;
375                 printf "%-47s %-32s\n", $label, shift @values;
376                 printf "%-47s %-32s\n", "", $_ foreach (@values);
377         }
378 }
379
380 sub printl2 ($$) # print a line w/ label and value (outside a table)
381 {
382         my ($label, $value) = @_;
383         if ($opt_html) {
384                 $label =~ s/</\&lt;/sg;
385                 $label =~ s/>/\&gt;/sg;
386                 $label =~ s/\n/<br>\n/sg;
387                 $value =~ s/</\&lt;/sg;
388                 $value =~ s/>/\&gt;/sg;
389                 $value =~ s/\n/<br>\n/sg;
390         }
391         print "$label: $value\n";
392 }
393
394 sub prints ($) # print seperator w/ given text
395 {
396         my ($label) = @_;
397         if ($opt_html) {
398                 $label =~ s/</\&lt;/sg;
399                 $label =~ s/>/\&gt;/sg;
400                 $label =~ s/\n/<br>\n/sg;
401                 print "<tr><td align=center colspan=2><b>$label</b></td></tr>\n";
402         } else {
403                 print "\n---=== $label ===---\n";
404         }
405 }
406
407 sub printh ($$) # print header w/ given text
408 {
409         my ($header, $sub) = @_;
410         if ($opt_html) {
411                 $header =~ s/</\&lt;/sg;
412                 $header =~ s/>/\&gt;/sg;
413                 $header =~ s/\n/<br>\n/sg;
414                 $sub =~ s/</\&lt;/sg;
415                 $sub =~ s/>/\&gt;/sg;
416                 $sub =~ s/\n/<br>\n/sg;
417                 print "<h1>$header</h1>\n";
418                 print "<p>$sub</p>\n";
419         } else {
420                 print "\n$header\n$sub\n";
421         }
422 }
423
424 # Parameter: bytes 0-63
425 sub decode_sdr_sdram($)
426 {
427         my $bytes = shift;
428         my ($l, $temp);
429
430 # SPD revision
431         printl "SPD Revision", $bytes->[62];
432
433 #size computation
434
435         prints "Memory Characteristics";
436
437         my $k=0;
438         my $ii=0;
439
440         $ii = ($bytes->[3] & 0x0f) + ($bytes->[4] & 0x0f) - 17;
441         if (($bytes->[5] <= 8) && ($bytes->[17] <= 8)) {
442                  $k = $bytes->[5] * $bytes->[17];
443         }
444
445         if($ii > 0 && $ii <= 12 && $k > 0) {
446                 printl "Size", ((1 << $ii) * $k) . " MB"; }
447         else {
448                 printl "INVALID SIZE", $bytes->[3] . "," . $bytes->[4] . "," .
449                                        $bytes->[5] . "," . $bytes->[17];
450         }
451
452         my @cas;
453         for ($ii = 0; $ii < 7; $ii++) {
454                 push(@cas, $ii + 1) if ($bytes->[18] & (1 << $ii));
455         }
456
457         my $trcd;
458         my $trp;
459         my $tras;
460         my $ctime = ($bytes->[9] >> 4) + ($bytes->[9] & 0xf) * 0.1;
461
462         $trcd =$bytes->[29];
463         $trp =$bytes->[27];;
464         $tras =$bytes->[30];
465
466         printl "tCL-tRCD-tRP-tRAS",
467                 $cas[$#cas] . "-" .
468                 ceil($trcd/$ctime) . "-" .
469                 ceil($trp/$ctime) . "-" .
470                 ceil($tras/$ctime);
471
472         $l = "Number of Row Address Bits";
473         if ($bytes->[3] == 0) { printl $l, "Undefined!"; }
474         elsif ($bytes->[3] == 1) { printl $l, "1/16"; }
475         elsif ($bytes->[3] == 2) { printl $l, "2/17"; }
476         elsif ($bytes->[3] == 3) { printl $l, "3/18"; }
477         else { printl $l, $bytes->[3]; }
478
479         $l = "Number of Col Address Bits";
480         if ($bytes->[4] == 0) { printl $l, "Undefined!"; }
481         elsif ($bytes->[4] == 1) { printl $l, "1/16"; }
482         elsif ($bytes->[4] == 2) { printl $l, "2/17"; }
483         elsif ($bytes->[4] == 3) { printl $l, "3/18"; }
484         else { printl $l, $bytes->[4]; }
485
486         $l = "Number of Module Rows";
487         if ($bytes->[5] == 0 ) { printl $l, "Undefined!"; }
488         else { printl $l, $bytes->[5]; }
489
490         $l = "Data Width";
491         if ($bytes->[7] > 1) {
492                 printl $l, "Undefined!"
493         } else {
494                 $temp = ($bytes->[7] * 256) + $bytes->[6];
495                 printl $l, $temp;
496         }
497
498         $l = "Module Interface Signal Levels";
499         if ($bytes->[8] == 0) { printl $l, "5.0 Volt/TTL"; }
500         elsif ($bytes->[8] == 1) { printl $l, "LVTTL"; }
501         elsif ($bytes->[8] == 2) { printl $l, "HSTL 1.5"; }
502         elsif ($bytes->[8] == 3) { printl $l, "SSTL 3.3"; }
503         elsif ($bytes->[8] == 4) { printl $l, "SSTL 2.5"; }
504         elsif ($bytes->[8] == 255) { printl $l, "New Table"; }
505         else { printl $l, "Undefined!"; }
506
507         $l = "Module Configuration Type";
508         if ($bytes->[11] == 0) { printl $l, "No Parity"; }
509         elsif ($bytes->[11] == 1) { printl $l, "Parity"; }
510         elsif ($bytes->[11] == 2) { printl $l, "ECC"; }
511         else { printl $l, "Undefined!"; }
512
513         $l = "Refresh Type";
514         if ($bytes->[12] > 126) { printl $l, "Self Refreshing"; }
515         else { printl $l, "Not Self Refreshing"; }
516
517         $l = "Refresh Rate";
518         $temp = $bytes->[12] & 0x7f;
519         if ($temp == 0) { printl $l, "Normal (15.625 us)"; }
520         elsif ($temp == 1) { printl $l, "Reduced (3.9 us)"; }
521         elsif ($temp == 2) { printl $l, "Reduced (7.8 us)"; }
522         elsif ($temp == 3) { printl $l, "Extended (31.3 us)"; }
523         elsif ($temp == 4) { printl $l, "Extended (62.5 us)"; }
524         elsif ($temp == 5) { printl $l, "Extended (125 us)"; }
525         else { printl $l, "Undefined!"; }
526
527         $l = "Primary SDRAM Component Bank Config";
528         if ($bytes->[13] > 126) { printl $l, "Bank2 = 2 x Bank1"; }
529         else { printl $l, "No Bank2 OR Bank2 = Bank1 width"; }
530
531         $l = "Primary SDRAM Component Widths";
532         $temp = $bytes->[13] & 0x7f;
533         if ($temp == 0) { printl $l, "Undefined!\n"; }
534         else { printl $l, $temp; }
535
536         $l = "Error Checking SDRAM Component Bank Config";
537         if ($bytes->[14] > 126) { printl $l, "Bank2 = 2 x Bank1"; }
538         else { printl $l, "No Bank2 OR Bank2 = Bank1 width"; }
539
540         $l = "Error Checking SDRAM Component Widths";
541         $temp = $bytes->[14] & 0x7f;
542         if ($temp == 0) { printl $l, "Undefined!"; }
543         else { printl $l, $temp; }
544
545         $l = "Min Clock Delay for Back to Back Random Access";
546         if ($bytes->[15] == 0) { printl $l, "Undefined!"; }
547         else { printl $l, $bytes->[15]; }
548
549         $l = "Burst lengths supported";
550         my @array;
551         for ($ii = 0; $ii < 4; $ii++) {
552                 push(@array, 1 << $ii) if ($bytes->[16] & (1 << $ii));
553         }
554         push(@array, "Page") if ($bytes->[16] & 128);
555         if (@array) { $temp = join ', ', @array; }
556         else { $temp = "None"; }
557         printl $l, $temp;
558
559         $l = "Number of Device Banks";
560         if ($bytes->[17] == 0) { printl $l, "Undefined/Reserved!"; }
561         else { printl $l, $bytes->[17]; }
562
563         $l = "Supported CAS Latencies";
564         if (@cas) { $temp = join ', ', @cas; }
565         else { $temp = "None"; }
566         printl $l, $temp;
567
568         $l = "Supported CS Latencies";
569         @array = ();
570         for ($ii = 0; $ii < 7; $ii++) {
571                 push(@array, $ii) if ($bytes->[19] & (1 << $ii));
572         }
573         if (@array) { $temp = join ', ', @array; }
574         else { $temp = "None"; }
575         printl $l, $temp;
576
577         $l = "Supported WE Latencies";
578         @array = ();
579         for ($ii = 0; $ii < 7; $ii++) {
580                 push(@array, $ii) if ($bytes->[20] & (1 << $ii));
581         }
582         if (@array) { $temp = join ', ', @array; }
583         else { $temp = "None"; }
584         printl $l, $temp;
585
586         if (@cas >= 1) {
587                 $l = "Cycle Time (CAS ".$cas[$#cas].")";
588                 printl $l, "$ctime ns";
589
590                 $l = "Access Time (CAS ".$cas[$#cas].")";
591                 $temp = ($bytes->[10] >> 4) + ($bytes->[10] & 0xf) * 0.1;
592                 printl $l, "$temp ns";
593         }
594
595         if (@cas >= 2 && spd_written(@$bytes[23..24])) {
596                 $l = "Cycle Time (CAS ".$cas[$#cas-1].")";
597                 $temp = $bytes->[23] >> 4;
598                 if ($temp == 0) { printl $l, "Undefined!"; }
599                 else {
600                         if ($temp < 4 ) { $temp=$temp + 15; }
601                         printl $l, $temp + (($bytes->[23] & 0xf) * 0.1) . " ns";
602                 }
603
604                 $l = "Access Time (CAS ".$cas[$#cas-1].")";
605                 $temp = $bytes->[24] >> 4;
606                 if ($temp == 0) { printl $l, "Undefined!"; }
607                 else {
608                         if ($temp < 4 ) { $temp=$temp + 15; }
609                         printl $l, $temp + (($bytes->[24] & 0xf) * 0.1) . " ns";
610                 }
611         }
612
613         if (@cas >= 3 && spd_written(@$bytes[25..26])) {
614                 $l = "Cycle Time (CAS ".$cas[$#cas-2].")";
615                 $temp = $bytes->[25] >> 2;
616                 if ($temp == 0) { printl $l, "Undefined!"; }
617                 else { printl $l, $temp + ($bytes->[25] & 0x3) * 0.25 . " ns"; }
618
619                 $l = "Access Time (CAS ".$cas[$#cas-2].")";
620                 $temp = $bytes->[26] >> 2;
621                 if ($temp == 0) { printl $l, "Undefined!"; }
622                 else { printl $l, $temp + ($bytes->[26] & 0x3) * 0.25 . " ns"; }
623         }
624
625         $l = "SDRAM Module Attributes";
626         $temp = "";
627         if ($bytes->[21] & 1) { $temp .= "Buffered Address/Control Inputs\n"; }
628         if ($bytes->[21] & 2) { $temp .= "Registered Address/Control Inputs\n"; }
629         if ($bytes->[21] & 4) { $temp .= "On card PLL (clock)\n"; }
630         if ($bytes->[21] & 8) { $temp .= "Buffered DQMB Inputs\n"; }
631         if ($bytes->[21] & 16) { $temp .= "Registered DQMB Inputs\n"; }
632         if ($bytes->[21] & 32) { $temp .= "Differential Clock Input\n"; }
633         if ($bytes->[21] & 64) { $temp .= "Redundant Row Address\n"; }
634         if ($bytes->[21] & 128) { $temp .= "Undefined (bit 7)\n"; }
635         if ($bytes->[21] == 0) { $temp .= "(None Reported)\n"; }
636         printl $l, $temp;
637
638         $l = "SDRAM Device Attributes (General)";
639         $temp = "";
640         if ($bytes->[22] & 1) { $temp .= "Supports Early RAS# Recharge\n"; }
641         if ($bytes->[22] & 2) { $temp .= "Supports Auto-Precharge\n"; }
642         if ($bytes->[22] & 4) { $temp .= "Supports Precharge All\n"; }
643         if ($bytes->[22] & 8) { $temp .= "Supports Write1/Read Burst\n"; }
644         if ($bytes->[22] & 16) { $temp .= "Lower VCC Tolerance: 5%\n"; }
645         else { $temp .= "Lower VCC Tolerance: 10%\n"; }
646         if ($bytes->[22] & 32) { $temp .= "Upper VCC Tolerance: 5%\n"; }
647         else { $temp .= "Upper VCC Tolerance: 10%\n"; }
648         if ($bytes->[22] & 64) { $temp .= "Undefined (bit 6)\n"; }
649         if ($bytes->[22] & 128) { $temp .= "Undefined (bit 7)\n"; }
650         printl $l, $temp;
651
652         $l = "Minimum Row Precharge Time";
653         if ($bytes->[27] == 0) { printl $l, "Undefined!"; }
654         else { printl $l, "$bytes->[27] ns"; }
655
656         $l = "Row Active to Row Active Min";
657         if ($bytes->[28] == 0) { printl $l, "Undefined!"; }
658         else { printl $l, "$bytes->[28] ns"; }
659
660         $l = "RAS to CAS Delay";
661         if ($bytes->[29] == 0) { printl $l, "Undefined!"; }
662         else { printl $l, "$bytes->[29] ns"; }
663
664         $l = "Min RAS Pulse Width";
665         if ($bytes->[30] == 0) { printl $l, "Undefined!"; }
666         else { printl $l, "$bytes->[30] ns"; }
667
668         $l = "Row Densities";
669         $temp = "";
670         if ($bytes->[31] & 1) { $temp .= "4 MByte\n"; }
671         if ($bytes->[31] & 2) { $temp .= "8 MByte\n"; }
672         if ($bytes->[31] & 4) { $temp .= "16 MByte\n"; }
673         if ($bytes->[31] & 8) { $temp .= "32 MByte\n"; }
674         if ($bytes->[31] & 16) { $temp .= "64 MByte\n"; }
675         if ($bytes->[31] & 32) { $temp .= "128 MByte\n"; }
676         if ($bytes->[31] & 64) { $temp .= "256 MByte\n"; }
677         if ($bytes->[31] & 128) { $temp .= "512 MByte\n"; }
678         if ($bytes->[31] == 0) { $temp .= "(Undefined! -- None Reported!)\n"; }
679         printl $l, $temp;
680
681         if (($bytes->[32] & 0xf) <= 9) {
682                 $l = "Command and Address Signal Setup Time";
683                 $temp = (($bytes->[32] & 0x7f) >> 4) + ($bytes->[32] & 0xf) * 0.1;
684                 printl $l, (($bytes->[32] >> 7) ? -$temp : $temp) . " ns";
685         }
686
687         if (($bytes->[33] & 0xf) <= 9) {
688                 $l = "Command and Address Signal Hold Time";
689                 $temp = (($bytes->[33] & 0x7f) >> 4) + ($bytes->[33] & 0xf) * 0.1;
690                 printl $l, (($bytes->[33] >> 7) ? -$temp : $temp) . " ns";
691         }
692
693         if (($bytes->[34] & 0xf) <= 9) {
694                 $l = "Data Signal Setup Time";
695                 $temp = (($bytes->[34] & 0x7f) >> 4) + ($bytes->[34] & 0xf) * 0.1;
696                 printl $l, (($bytes->[34] >> 7) ? -$temp : $temp) . " ns";
697         }
698
699         if (($bytes->[35] & 0xf) <= 9) {
700                 $l = "Data Signal Hold Time";
701                 $temp = (($bytes->[35] & 0x7f) >> 4) + ($bytes->[35] & 0xf) * 0.1;
702                 printl $l, (($bytes->[35] >> 7) ? -$temp : $temp) . " ns";
703         }
704 }
705
706 # Parameter: bytes 0-63
707 sub decode_ddr_sdram($)
708 {
709         my $bytes = shift;
710         my ($l, $temp);
711
712 # SPD revision
713         if ($bytes->[62] != 0xff) {
714                 printl "SPD Revision", ($bytes->[62] >> 4) . "." .
715                                        ($bytes->[62] & 0xf);
716         }
717
718 # speed
719         prints "Memory Characteristics";
720
721         $l = "Maximum module speed";
722         $temp = ($bytes->[9] >> 4) + ($bytes->[9] & 0xf) * 0.1;
723         my $ddrclk = 2 * (1000 / $temp);
724         my $tbits = ($bytes->[7] * 256) + $bytes->[6];
725         if (($bytes->[11] == 2) || ($bytes->[11] == 1)) { $tbits = $tbits - 8; }
726         my $pcclk = int ($ddrclk * $tbits / 8);
727         $pcclk += 100 if ($pcclk % 100) >= 50; # Round properly
728         $pcclk = $pcclk - ($pcclk % 100);
729         $ddrclk = int ($ddrclk);
730         printl $l, "${ddrclk}MHz (PC${pcclk})";
731
732 #size computation
733         my $k=0;
734         my $ii=0;
735
736         $ii = ($bytes->[3] & 0x0f) + ($bytes->[4] & 0x0f) - 17;
737         if (($bytes->[5] <= 8) && ($bytes->[17] <= 8)) {
738                  $k = $bytes->[5] * $bytes->[17];
739         }
740
741         if($ii > 0 && $ii <= 12 && $k > 0) {
742                 printl "Size", ((1 << $ii) * $k) . " MB"; }
743         else {
744                 printl "INVALID SIZE", $bytes->[3] . ", " . $bytes->[4] . ", " .
745                                        $bytes->[5] . ", " . $bytes->[17];
746         }
747
748         my $highestCAS = 0;
749         my %cas;
750         for ($ii = 0; $ii < 7; $ii++) {
751                 if ($bytes->[18] & (1 << $ii)) {
752                         $highestCAS = 1+$ii*0.5;
753                         $cas{$highestCAS}++;
754                 }
755         }
756
757         my $trcd;
758         my $trp;
759         my $tras;
760         my $ctime = ($bytes->[9] >> 4) + ($bytes->[9] & 0xf) * 0.1;
761
762         $trcd =($bytes->[29] >> 2)+(($bytes->[29] & 3)*0.25);
763         $trp =($bytes->[27] >> 2)+(($bytes->[27] & 3)*0.25);
764         $tras = $bytes->[30];
765
766         printl "tCL-tRCD-tRP-tRAS",
767                 $highestCAS . "-" .
768                 ceil($trcd/$ctime) . "-" .
769                 ceil($trp/$ctime) . "-" .
770                 ceil($tras/$ctime);
771
772 # latencies
773         if (keys %cas) { $temp = join ', ', sort { $b <=> $a } keys %cas; }
774         else { $temp = "None"; }
775         printl "Supported CAS Latencies", $temp;
776
777         my @array;
778         for ($ii = 0; $ii < 7; $ii++) {
779                 push(@array, $ii) if ($bytes->[19] & (1 << $ii));
780         }
781         if (@array) { $temp = join ', ', @array; }
782         else { $temp = "None"; }
783         printl "Supported CS Latencies", $temp;
784
785         @array = ();
786         for ($ii = 0; $ii < 7; $ii++) {
787                 push(@array, $ii) if ($bytes->[20] & (1 << $ii));
788         }
789         if (@array) { $temp = join ', ', @array; }
790         else { $temp = "None"; }
791         printl "Supported WE Latencies", $temp;
792
793 # timings
794         if (exists $cas{$highestCAS}) {
795                 printl "Minimum Cycle Time (CAS $highestCAS)",
796                        "$ctime ns";
797
798                 printl "Maximum Access Time (CAS $highestCAS)",
799                        (($bytes->[10] >> 4) * 0.1 + ($bytes->[10] & 0xf) * 0.01) . " ns";
800         }
801
802         if (exists $cas{$highestCAS-0.5} && spd_written(@$bytes[23..24])) {
803                 printl "Minimum Cycle Time (CAS ".($highestCAS-0.5).")",
804                        (($bytes->[23] >> 4) + ($bytes->[23] & 0xf) * 0.1) . " ns";
805
806                 printl "Maximum Access Time (CAS ".($highestCAS-0.5).")",
807                        (($bytes->[24] >> 4) * 0.1 + ($bytes->[24] & 0xf) * 0.01) . " ns";
808         }
809
810         if (exists $cas{$highestCAS-1} && spd_written(@$bytes[25..26])) {
811                 printl "Minimum Cycle Time (CAS ".($highestCAS-1).")",
812                        (($bytes->[25] >> 4) + ($bytes->[25] & 0xf) * 0.1) . " ns";
813
814                 printl "Maximum Access Time (CAS ".($highestCAS-1).")",
815                        (($bytes->[26] >> 4) * 0.1 + ($bytes->[26] & 0xf) * 0.01) . " ns";
816         }
817
818 # module attributes
819         if ($bytes->[47] & 0x03) {
820                 if (($bytes->[47] & 0x03) == 0x01) { $temp = "1.125\" to 1.25\""; }
821                 elsif (($bytes->[47] & 0x03) == 0x02) { $temp = "1.7\""; }
822                 elsif (($bytes->[47] & 0x03) == 0x03) { $temp = "Other"; }
823                 printl "Module Height", $temp;
824         }
825 }
826
827 sub ddr2_sdram_ctime($)
828 {
829         my $byte = shift;
830         my $ctime;
831
832         $ctime = $byte >> 4;
833         if (($byte & 0xf) <= 9) { $ctime += ($byte & 0xf) * 0.1; }
834         elsif (($byte & 0xf) == 10) { $ctime += 0.25; }
835         elsif (($byte & 0xf) == 11) { $ctime += 0.33; }
836         elsif (($byte & 0xf) == 12) { $ctime += 0.66; }
837         elsif (($byte & 0xf) == 13) { $ctime += 0.75; }
838
839         return $ctime;
840 }
841
842 sub ddr2_sdram_atime($)
843 {
844         my $byte = shift;
845         my $atime;
846
847         $atime = ($byte >> 4) * 0.1 + ($byte & 0xf) * 0.01;
848
849         return $atime;
850 }
851
852 # Parameter: bytes 0-63
853 sub decode_ddr2_sdram($)
854 {
855         my $bytes = shift;
856         my ($l, $temp);
857
858 # SPD revision
859         if ($bytes->[62] != 0xff) {
860                 printl "SPD Revision", ($bytes->[62] >> 4) . "." .
861                                        ($bytes->[62] & 0xf);
862         }
863
864 # speed
865         prints "Memory Characteristics";
866
867         $l = "Maximum module speed";
868         $temp = ($bytes->[9] >> 4) + ($bytes->[9] & 0xf) * 0.1;
869         my $ddrclk = 4 * (1000 / $temp);
870         my $tbits = ($bytes->[7] * 256) + $bytes->[6];
871         if (($bytes->[11] == 2) || ($bytes->[11] == 1)) { $tbits = $tbits - 8; }
872         my $pcclk = int ($ddrclk * $tbits / 8);
873         $pcclk += 100 if ($pcclk % 100) >= 50; # Round properly
874         $pcclk = $pcclk - ($pcclk % 100);
875         $ddrclk = int ($ddrclk);
876         printl $l, "${ddrclk}MHz (PC${pcclk})";
877
878 #size computation
879         my $k=0;
880         my $ii=0;
881
882         $ii = ($bytes->[3] & 0x0f) + ($bytes->[4] & 0x0f) - 17;
883         $k = (($bytes->[5] & 0x7) + 1) * $bytes->[17];
884
885         if($ii > 0 && $ii <= 12 && $k > 0) {
886                 printl "Size", ((1 << $ii) * $k) . " MB";
887         } else {
888                 printl "INVALID SIZE", $bytes->[3] . "," . $bytes->[4] . "," .
889                                        $bytes->[5] . "," . $bytes->[17];
890         }
891
892         my $highestCAS = 0;
893         my %cas;
894         for ($ii = 2; $ii < 7; $ii++) {
895                 if ($bytes->[18] & (1 << $ii)) {
896                         $highestCAS = $ii;
897                         $cas{$highestCAS}++;
898                 }
899         }
900
901         my $trcd;
902         my $trp;
903         my $tras;
904         my $ctime;
905
906         $ctime = ddr2_sdram_ctime($bytes->[9]);
907
908         $trcd =($bytes->[29] >> 2)+(($bytes->[29] & 3)*0.25);
909         $trp =($bytes->[27] >> 2)+(($bytes->[27] & 3)*0.25);
910         $tras =$bytes->[30];
911
912         printl "tCL-tRCD-tRP-tRAS",
913                 $highestCAS . "-" .
914                 ceil($trcd/$ctime) . "-" .
915                 ceil($trp/$ctime) . "-" .
916                 ceil($tras/$ctime);
917
918 # latencies
919         if (keys %cas) { $temp = join ', ', sort { $b <=> $a } keys %cas; }
920         else { $temp = "None"; }
921         printl "Supported CAS Latencies", $temp;
922
923 # timings
924         if (exists $cas{$highestCAS}) {
925                 printl "Minimum Cycle Time (CAS $highestCAS)",
926                        "$ctime ns";
927                 printl "Maximum Access Time (CAS $highestCAS)",
928                        ddr2_sdram_atime($bytes->[10]) . " ns";
929         }
930
931         if (exists $cas{$highestCAS-1} && spd_written(@$bytes[23..24])) {
932                 printl "Minimum Cycle Time (CAS ".($highestCAS-1).")",
933                        ddr2_sdram_ctime($bytes->[23]) . " ns";
934                 printl "Maximum Access Time (CAS ".($highestCAS-1).")",
935                        ddr2_sdram_atime($bytes->[24]) . " ns";
936         }
937
938         if (exists $cas{$highestCAS-2} && spd_written(@$bytes[25..26])) {
939                 printl "Minimum Cycle Time (CAS ".($highestCAS-2).")",
940                        ddr2_sdram_ctime($bytes->[25]) . " ns";
941                 printl "Maximum Access Time (CAS ".($highestCAS-2).")",
942                        ddr2_sdram_atime($bytes->[26]) . " ns";
943         }
944 }
945
946 # Parameter: bytes 0-63
947 sub decode_direct_rambus($)
948 {
949         my $bytes = shift;
950
951 #size computation
952         prints "Memory Characteristics";
953
954         my $ii;
955
956         $ii = ($bytes->[4] & 0x0f) + ($bytes->[4] >> 4) + ($bytes->[5] & 0x07) - 13;
957
958         if ($ii > 0 && $ii < 16) {
959                 printl "Size", (1 << $ii) . " MB";
960         } else {
961                 printl "INVALID SIZE", _stprintf("0x%02x, 0x%02x",
962                                                $bytes->[4], $bytes->[5]);
963         }
964 }
965
966 # Parameter: bytes 0-63
967 sub decode_rambus($)
968 {
969         my $bytes = shift;
970
971 #size computation
972         prints "Memory Characteristics";
973
974         my $ii;
975
976         $ii = ($bytes->[3] & 0x0f) + ($bytes->[3] >> 4) + ($bytes->[5] & 0x07) - 13;
977
978         if ($ii > 0 && $ii < 16) {
979                 printl "Size", (1 << $ii) . " MB";
980         } else {
981                 printl "INVALID SIZE", _stprintf("0x%02x, 0x%02x",
982                                                $bytes->[3], $bytes->[5]);
983         }
984 }
985
986 %decode_callback = (
987         "SDR SDRAM"     => \&decode_sdr_sdram,
988         "DDR SDRAM"     => \&decode_ddr_sdram,
989         "DDR2 SDRAM"    => \&decode_ddr2_sdram,
990         "Direct Rambus" => \&decode_direct_rambus,
991         "Rambus"        => \&decode_rambus,
992 );
993
994 # Parameter: bytes 64-127
995 sub decode_intel_spec_freq($)
996 {
997         my $bytes = shift;
998         my ($l, $temp);
999
1000         prints "Intel Specification";
1001
1002         $l = "Frequency";
1003         if ($bytes->[62] == 0x66) { $temp = "66MHz\n"; }
1004         elsif ($bytes->[62] == 100) { $temp = "100MHz or 133MHz\n"; }
1005         elsif ($bytes->[62] == 133) { $temp = "133MHz\n"; }
1006         else { $temp = "Undefined!\n"; }
1007         printl $l, $temp;
1008
1009         $l = "Details for 100MHz Support";
1010         $temp="";
1011         if ($bytes->[63] & 1) { $temp .= "Intel Concurrent Auto-precharge\n"; }
1012         if ($bytes->[63] & 2) { $temp .= "CAS Latency = 2\n"; }
1013         if ($bytes->[63] & 4) { $temp .= "CAS Latency = 3\n"; }
1014         if ($bytes->[63] & 8) { $temp .= "Junction Temp A (100 degrees C)\n"; }
1015         else { $temp .= "Junction Temp B (90 degrees C)\n"; }
1016         if ($bytes->[63] & 16) { $temp .= "CLK 3 Connected\n"; }
1017         if ($bytes->[63] & 32) { $temp .= "CLK 2 Connected\n"; }
1018         if ($bytes->[63] & 64) { $temp .= "CLK 1 Connected\n"; }
1019         if ($bytes->[63] & 128) { $temp .= "CLK 0 Connected\n"; }
1020         if (($bytes->[63] & 192) == 192) { $temp .= "Double-sided DIMM\n"; }
1021         elsif (($bytes->[63] & 192) != 0) { $temp .= "Single-sided DIMM\n"; }
1022         printl $l, $temp;
1023 }
1024
1025 sub readspd64 ($$) { # reads 64 bytes from SPD-EEPROM
1026         my ($offset, $dimm_i) = @_;
1027         my @bytes;
1028         if ($use_sysfs) {
1029                 # Kernel 2.6 with sysfs
1030                 sysopen(HANDLE, "/sys/bus/i2c/drivers/eeprom/$dimm_i/eeprom", O_RDONLY)
1031                         or die "Cannot open /sys/bus/i2c/drivers/eeprom/$dimm_i/eeprom";
1032                 binmode HANDLE;
1033                 sysseek(HANDLE, $offset, SEEK_SET);
1034                 sysread(HANDLE, my $eeprom, 64);
1035                 close HANDLE;
1036                 @bytes = unpack("C64", $eeprom);
1037         } else {
1038                 # Kernel 2.4 with procfs
1039                 for my $i (0 .. 3) {
1040                         my $hexoff = _stprintf('%02x', $offset + $i * 16);
1041                         push @bytes, split(" ", `cat /proc/sys/dev/sensors/$dimm_i/$hexoff`);
1042                 }
1043         }
1044         return @bytes;
1045 }
1046
1047 for (@ARGV) {
1048     if (/-h/) {
1049                 print "Usage: $0 [-c] [-f [-b]]\n",
1050                         "       $0 -h\n\n",
1051                         "  -f, --format            print nice html output\n",
1052                         "  -b, --bodyonly          don't print html header\n",
1053                         "                          (useful for postprocessing the output)\n",
1054                         "  -c, --checksum          decode completely even if checksum fails\n",
1055                         "  -h, --help              display this usage summary\n";
1056                 exit;
1057     }
1058     $opt_html = 1 if (/-f/);
1059     $opt_bodyonly = 1 if (/-b/);
1060     $opt_igncheck = 1 if (/-c/);
1061 }
1062 $opt_body = $opt_html && ! $opt_bodyonly;
1063
1064 if ($opt_body)
1065 {
1066         print "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n",
1067               "<html><head>\n",
1068                   "\t<meta HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n",
1069                   "\t<title>PC DIMM Serial Presence Detect Tester/Decoder Output</title>\n",
1070                   "</head><body>\n";
1071 }
1072
1073 printh 'Memory Serial Presence Detect Decoder',
1074 'By Philip Edelbrock, Christian Zuckschwerdt, Burkart Lingner,
1075 Jean Delvare and others
1076 Version 2.10.1';
1077
1078
1079 my $dimm_count=0;
1080 if ($use_sysfs) { $_=`ls /sys/bus/i2c/drivers/eeprom`; }
1081 else { $_=`ls /proc/sys/dev/sensors/`; }
1082 my @dimm_list=split();
1083
1084 for my $i ( 0 .. $#dimm_list ) {
1085         $_=$dimm_list[$i];
1086         if (($use_sysfs && /^\d+-\d+$/)
1087          || (!$use_sysfs && /^eeprom-/)) {
1088                 print "<b><u>" if $opt_html;
1089                 printl2 "\n\nDecoding EEPROM", ($use_sysfs ?
1090                         "/sys/bus/i2c/drivers/eeprom/$dimm_list[$i]" :
1091                         "/proc/sys/dev/sensors/$dimm_list[$i]");
1092                 print "</u></b>" if $opt_html;
1093                 print "<table border=1>\n" if $opt_html;
1094                 if (($use_sysfs && /^[^-]+-([^-]+)$/)
1095                  || (!$use_sysfs && /^[^-]+-[^-]+-[^-]+-([^-]+)$/)) {
1096                         my $dimm_num=$1 - 49;
1097                         printl "Guessing DIMM is in", "bank $dimm_num";
1098                 }
1099
1100 # Decode first 3 bytes (0-2)
1101                 prints "SPD EEPROM Information";
1102
1103                 my @bytes = readspd64(0, $dimm_list[$i]);
1104                 my $dimm_checksum = 0;
1105                 $dimm_checksum += $bytes[$_] foreach (0 .. 62);
1106                 $dimm_checksum &= 0xff;
1107
1108                 my $l = "EEPROM Checksum of bytes 0-62";
1109                 printl $l, ($bytes[63] == $dimm_checksum ?
1110                         _stprintf("OK (0x%.2X)", $bytes[63]):
1111                         _stprintf("Bad\n(found 0x%.2X, calculated 0x%.2X)\n",
1112                                 $bytes[63], $dimm_checksum));
1113
1114                 unless ($bytes[63] == $dimm_checksum or $opt_igncheck) {
1115                         print "</table>\n" if $opt_html;
1116                         next;
1117                 }
1118
1119                 $dimm_count++;
1120                 # Simple heuristic to detect Rambus
1121                 my $is_rambus = $bytes[0] < 4;
1122                 my $temp;
1123                 if ($is_rambus) {
1124                         if ($bytes[0] == 1) { $temp = "0.7"; }
1125                         elsif ($bytes[0] == 2) { $temp = "1.0"; }
1126                         elsif ($bytes[0] == 0 || $bytes[0] == 255) { $temp = "Invalid"; }
1127                         else { $temp = "Reserved"; }
1128                         printl "SPD Revision", $temp;
1129                 } else {
1130                         printl "# of bytes written to SDRAM EEPROM",
1131                                $bytes[0];
1132                 }
1133
1134                 $l = "Total number of bytes in EEPROM";
1135                 if ($bytes[1] <= 14) {
1136                         printl $l, 2**$bytes[1];
1137                 } elsif ($bytes[1] == 0) {
1138                         printl $l, "RFU";
1139                 } else { printl $l, "ERROR!"; }
1140
1141                 $l = "Fundamental Memory type";
1142                 my $type = "Unknown";
1143                 if ($is_rambus) {
1144                         if ($bytes[2] == 1) { $type = "Direct Rambus"; }
1145                         elsif ($bytes[2] == 17) { $type = "Rambus"; }
1146                 } else {
1147                         if ($bytes[2] == 1) { $type = "FPM DRAM"; }
1148                         elsif ($bytes[2] == 2) { $type = "EDO"; }
1149                         elsif ($bytes[2] == 3) { $type = "Pipelined Nibble"; }
1150                         elsif ($bytes[2] == 4) { $type = "SDR SDRAM"; }
1151                         elsif ($bytes[2] == 5) { $type = "Multiplexed ROM"; }
1152                         elsif ($bytes[2] == 6) { $type = "DDR SGRAM"; }
1153                         elsif ($bytes[2] == 7) { $type = "DDR SDRAM"; }
1154                         elsif ($bytes[2] == 8) { $type = "DDR2 SDRAM"; }
1155                 }
1156                 printl $l, $type;
1157
1158 # Decode next 61 bytes (3-63, depend on memory type)
1159                 $decode_callback{$type}->(\@bytes)
1160                         if exists $decode_callback{$type};
1161
1162 # Decode next 35 bytes (64-98, common to all memory types)
1163                 prints "Manufacturing Information";
1164
1165                 @bytes = readspd64(64, $dimm_list[$i]);
1166
1167                 $l = "Manufacturer";
1168                 # $extra is a reference to an array containing up to
1169                 # 7 extra bytes from the Manufacturer field. Sometimes
1170                 # these bytes are filled with interesting data.
1171                 ($temp, my $extra) = manufacturer(@bytes[0..7]);
1172                 printl $l, $temp;
1173                 $l = "Custom Manufacturer Data";
1174                 $temp = manufacturer_data(@{$extra});
1175                 printl $l, $temp if defined $temp;
1176
1177                 if (spd_written($bytes[8])) {
1178                         # Try the location code as ASCII first, as earlier specifications
1179                         # suggested this. As newer specifications don't mention it anymore,
1180                         # we still fall back to binary.
1181                         $l = "Manufacturing Location Code";
1182                         $temp = (chr($bytes[8]) =~ m/^[\w\d]$/) ? chr($bytes[8])
1183                               : _stprintf("0x%.2X", $bytes[8]);
1184                         printl $l, $temp;
1185                 }
1186
1187                 $l = "Part Number";
1188                 $temp = part_number(@bytes[9..26]);
1189                 printl $l, $temp;
1190
1191                 if (spd_written(@bytes[27..28])) {
1192                         $l = "Revision Code";
1193                         $temp = _stprintf("0x%02X%02X\n", @bytes[27..28]);
1194                         printl $l, $temp;
1195                 }
1196
1197                 if (spd_written(@bytes[29..30])) {
1198                         $l = "Manufacturing Date";
1199                         # In theory the year and week are in BCD format, but
1200                         # this is not always true in practice :(
1201                         if (($bytes[29] & 0xf0) <= 0x90
1202                          && ($bytes[29] & 0x0f) <= 0x09
1203                          && ($bytes[30] & 0xf0) <= 0x90
1204                          && ($bytes[30] & 0x0f) <= 0x09) {
1205                                 # Note that this heuristic will break in year 2080
1206                                 $temp = _stprintf("%d%02X-W%02X\n",
1207                                                 $bytes[29] >= 0x80 ? 19 : 20,
1208                                                 @bytes[29..30]);
1209                         } else {
1210                                 $temp = _stprintf("0x%02X%02X\n",
1211                                                 @bytes[29..30]);
1212                         }
1213                         printl $l, $temp;
1214                 }
1215
1216                 if (spd_written(@bytes[31..34])) {
1217                         $l = "Assembly Serial Number";
1218                         $temp = _stprintf("0x%02X%02X%02X%02X\n",
1219                                         @bytes[31..34]);
1220                         printl $l, $temp;
1221                 }
1222
1223 # Next 27 bytes (99-125) are manufacturer specific, can't decode
1224
1225 # Last 2 bytes (126-127) are reserved, Intel used them as an extension
1226                 if ($type eq "SDR SDRAM") {
1227                         decode_intel_spec_freq(\@bytes);
1228                 }
1229
1230                 print "</table>\n" if $opt_html;
1231         }
1232 }
1233 printl2 "\n\nNumber of SDRAM DIMMs detected and decoded", $dimm_count;
1234
1235 print "</body></html>\n" if $opt_body;
*/
