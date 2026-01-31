/**
 * @file
 *  DDR4 SPD 解析库头文件
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#ifndef _OKN_DDR4_SPD_PARSER_LIB_H_
#define _OKN_DDR4_SPD_PARSER_LIB_H_

#include <Uefi.h>  // EFI_STATUS, UINT8/UINT16/UINTN, BOOLEAN, EFI_SUCCESS...

#define DDR4_SPD_LEN 512u

//
// SPD offsets (DDR4 SPD base block)
//
#define DDR4_SPD_BYTE_PRIMARY_SDRAM_PACKAGE_TYPE   6   // 0x006
#define DDR4_SPD_BYTE_SECONDARY_SDRAM_PACKAGE_TYPE 10  // 0x00A (用于非对称场景)
#define DDR4_SPD_BYTE_MODULE_ORGANIZATION          12  // 0x00C
#define DDR4_SPD_BYTE_MODULE_BUS_WIDTH             13  // 0x00D

#define DDR4_SPD_MIN_LEN_FOR_BASIC_FIELDS 14  // 至少能读到 Byte13

//
// 基础字段解析
//
EFI_STATUS EFIAPI OknDdr4SpdIsRankMixAsymmetrical(IN CONST UINT8 *Spd, IN UINTN SpdLen, OUT BOOLEAN *Asymmetrical);
EFI_STATUS EFIAPI OknDdr4SpdGetSdramDeviceWidthBits(IN CONST UINT8 *Spd, IN UINTN SpdLen, OUT UINT16 *DeviceWidthBits);
EFI_STATUS EFIAPI OknDdr4SpdGetPackageRanksPerDimm(IN CONST UINT8 *Spd, IN UINTN SpdLen, OUT UINT8 *PackageRanks);
EFI_STATUS EFIAPI OknDdr4SpdGetPrimaryBusWidthBits(IN CONST UINT8 *Spd,
                                                   IN UINTN        SpdLen,
                                                   OUT UINT16     *PrimaryBusWidthBits);
EFI_STATUS EFIAPI OknDdr4SpdGetBusWidthExtensionBits(IN CONST UINT8 *Spd,
                                                     IN UINTN        SpdLen,
                                                     OUT UINT16     *ExtensionWidthBits);
EFI_STATUS EFIAPI OknDdr4SpdGetTotalBusWidthBits(IN CONST UINT8 *Spd,
                                                 IN UINTN        SpdLen,
                                                 IN BOOLEAN      IncludeExtension,
                                                 OUT UINT16     *TotalBusWidthBits);

EFI_STATUS EFIAPI OknDdr4SpdGetPrimaryDieCount(IN CONST UINT8 *Spd, IN UINTN SpdLen, OUT UINT8 *DieCount);
EFI_STATUS EFIAPI OknDdr4SpdIsPrimary3ds(IN CONST UINT8 *Spd, IN UINTN SpdLen, OUT BOOLEAN *Is3ds);
EFI_STATUS EFIAPI OknDdr4SpdEstimatePackagesPerRank(IN CONST UINT8 *Spd,
                                                    IN UINTN        SpdLen,
                                                    IN BOOLEAN      IncludeExtension,
                                                    OUT UINT16     *PackagesPerRank);
EFI_STATUS EFIAPI OknDdr4SpdEstimateTotalPackages(IN CONST UINT8 *Spd,
                                                  IN UINTN        SpdLen,
                                                  IN BOOLEAN      IncludeExtension,
                                                  OUT UINT16     *TotalPackages);
EFI_STATUS EFIAPI OknDdr4SpdGetLogicalRanksPerDimm_SymmetricOnly(IN CONST UINT8 *Spd,
                                                                 IN UINTN        SpdLen,
                                                                 OUT UINT16     *LogicalRanks);
EFI_STATUS EFIAPI OknDdr4SpdEstimateTotalDies_3dsSymmetricOnly(IN CONST UINT8 *Spd,
                                                               IN UINTN        SpdLen,
                                                               IN BOOLEAN      IncludeExtension,
                                                               OUT UINT32     *TotalDies);
EFI_STATUS EFIAPI OknDdr4SpdEstimateDataPackagesPerRank(IN CONST UINT8 *Spd,
                                                        IN UINTN        SpdLen,
                                                        OUT UINT16     *DataPackagesPerRank);
EFI_STATUS EFIAPI OknDdr4SpdEstimateTotalDataPackages(IN CONST UINT8 *Spd,
                                                      IN UINTN        SpdLen,
                                                      OUT UINT16     *TotalDataPackages);
EFI_STATUS EFIAPI OknDdr4SpdEstimateEccPackagesPerRank(IN CONST UINT8 *Spd,
                                                       IN UINTN        SpdLen,
                                                       OUT UINT16     *EccPackagesPerRank);
EFI_STATUS EFIAPI OknDdr4SpdEstimateTotalEccPackages(IN CONST UINT8 *Spd,
                                                     IN UINTN        SpdLen,
                                                     OUT UINT16     *TotalEccPackages);

#endif  // _OKN_DDR4_SPD_PARSER_LIB_H_
