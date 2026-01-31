#include <Library/OKN/OknDdr4SpdLib.h>

STATIC EFI_STATUS OknDdr4SpdReadByte(IN CONST UINT8 *Spd, IN UINTN SpdLen, IN UINTN Offset, OUT UINT8 *Value)
{
  if (Spd == NULL || Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (SpdLen <= Offset) {
    return EFI_BUFFER_TOO_SMALL;
  }
  *Value = Spd[Offset];

  return EFI_SUCCESS;
}

/**
 * Check whether the module uses an asymmetrical rank mix.
 *
 * 目的/口径:
 *   - 判断该 DIMM 的 RankMix 是否为"非对称(Asymmetrical)"
 *   - RankMix=1 表示不同 rank 可能使用不同的 SDRAM package 组织/容量/堆叠等,
 *     这会影响后续"推导类"计算(例如 total dies、logical ranks、某些容量推导).
 *   - 本函数只做"字段读取 + 布尔判断", 不做任何推导.
 *
 * SPD 字段来源(协议位置):
 *   - Byte 12 (0x00C) Module Organization, Table 28
 *       * Bit[6] = Rank Mix
 *           0 = Symmetrical
 *           1 = Asymmetrical
 *
 * 参数:
 *   @param[in]  Spd           SPD 原始字节数组(至少覆盖到 Byte 12)
 *   @param[in]  SpdLen        Spd 长度
 *   @param[out] Asymmetrical  输出: TRUE=非对称, FALSE=对称
 *
 * 返回值:
 *   EFI_SUCCESS           成功
 *   EFI_INVALID_PARAMETER 参数为空
 *   EFI_BUFFER_TOO_SMALL  SpdLen 不够读取 Byte 12
 */
EFI_STATUS EFIAPI OknDdr4SpdIsRankMixAsymmetrical(IN CONST UINT8 *Spd, IN UINTN SpdLen, OUT BOOLEAN *Asymmetrical)
{
  UINT8      B;
  EFI_STATUS Status;

  if (Asymmetrical == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Status = OknDdr4SpdReadByte(Spd, SpdLen, DDR4_SPD_BYTE_MODULE_ORGANIZATION, &B);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  *Asymmetrical = ((B >> 6) & 0x1u) ? TRUE : FALSE;
  return EFI_SUCCESS;
}

/**
 * Get SDRAM device I/O width in bits (ChipWidth: x4/x8/x16/x32).
 *
 * 目的/口径:
 *   - 解析 DDR4 DRAM 器件的"设备位宽/颗粒位宽"(通常称为 ChipWidth)
 *   - 返回值为"位数", 即 4/8/16/32 bits(对应 x4/x8/x16/x32)
 *   - 该字段用于推导封装颗数(bus width / device width).
 *
 * SPD 字段来源(协议位置):
 *   - Byte 12 (0x00C) Module Organization, Table 28
 *       * Bits[2:0] = SDRAM Device Width
 *           000b = 4 bits (x4)
 *           001b = 8 bits (x8)
 *           010b = 16 bits (x16)
 *           011b = 32 bits (x32)
 *           others = reserved
 *
 * 参数:
 *   @param[in]  Spd              SPD 原始字节数组(至少覆盖到 Byte 12)
 *   @param[in]  SpdLen           Spd 长度
 *   @param[out] DeviceWidthBits  输出: 4/8/16/32(单位 bit)
 *
 * 返回值:
 *   EFI_SUCCESS           成功
 *   EFI_INVALID_PARAMETER 参数为空
 *   EFI_BUFFER_TOO_SMALL  SpdLen 不够读取 Byte 12
 *   EFI_UNSUPPORTED       遇到 reserved 编码
 */
EFI_STATUS EFIAPI OknDdr4SpdGetSdramDeviceWidthBits(IN CONST UINT8 *Spd, IN UINTN SpdLen, OUT UINT16 *DeviceWidthBits)
{
  UINT8      B, Code;
  EFI_STATUS Status;

  if (NULL == DeviceWidthBits) {
    return EFI_INVALID_PARAMETER;
  }
  Status = OknDdr4SpdReadByte(Spd, SpdLen, DDR4_SPD_BYTE_MODULE_ORGANIZATION, &B);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Code = (UINT8)(B & 0x7u);
  switch (Code) {
    case 0: *DeviceWidthBits = 4; break;
    case 1: *DeviceWidthBits = 8; break;
    case 2: *DeviceWidthBits = 16; break;
    case 3: *DeviceWidthBits = 32; break;
    default: return EFI_UNSUPPORTED;
  }
  return EFI_SUCCESS;
}

/**
 * Get the number of package ranks per DIMM (Package Rank count).
 *
 * 目的/口径:
 *   - 解析该 DIMM 的"Package Ranks per DIMM"(常说的 1R/2R/4R... 的 R)
 *   - 该字段代表"每个 rank 由一组共享 CS 的 DRAM 设备集合组成"的 rank 数量
 *   - 返回范围: 1..8
 *
 * SPD 字段来源(协议位置):
 *   - Byte 12 (0x00C) Module Organization, Table 28
 *       * Bits[5:3] = Number of Package Ranks per DIMM
 *           000b = 1
 *           ...
 *           111b = 8
 *
 * 参数:
 *   @param[in]  Spd           SPD 原始字节数组(至少覆盖到 Byte 12)
 *   @param[in]  SpdLen        Spd 长度
 *   @param[out] PackageRanks  输出: package rank 数(1..8)
 *
 * 返回值:
 *   EFI_SUCCESS           成功
 *   EFI_INVALID_PARAMETER 参数为空
 *   EFI_BUFFER_TOO_SMALL  SpdLen 不够读取 Byte 12
 */
EFI_STATUS EFIAPI OknDdr4SpdGetPackageRanksPerDimm(IN CONST UINT8 *Spd, IN UINTN SpdLen, OUT UINT8 *PackageRanks)
{
  UINT8      B;
  EFI_STATUS Status;

  if (PackageRanks == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Status = OknDdr4SpdReadByte(Spd, SpdLen, DDR4_SPD_BYTE_MODULE_ORGANIZATION, &B);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  *PackageRanks = (UINT8)(((B >> 3) & 0x7u) + 1u);
  return EFI_SUCCESS;
}

/**
 * Get the primary memory bus width in bits.
 *
 * 目的/口径:
 *   - 解析 DIMM 的"主数据总线宽度(Primary bus width)"
 *   - 典型: x64(非 ECC)对应 64 bits
 *   - 注意: 本函数只返回 primary bus, 不包含 ECC 扩展位宽.
 *
 * SPD 字段来源(协议位置):
 *   - Byte 13 (0x00D) Module Memory Bus Width, Table 32
 *       * Bits[2:0] = Primary bus width
 *           000b = 8
 *           001b = 16
 *           010b = 32
 *           011b = 64
 *           others = reserved
 *
 * 参数:
 *   @param[in]  Spd                 SPD 原始字节数组(至少覆盖到 Byte 13)
 *   @param[in]  SpdLen              Spd 长度
 *   @param[out] PrimaryBusWidthBits 输出: 8/16/32/64(单位 bit)
 *
 * 返回值:
 *   EFI_SUCCESS           成功
 *   EFI_INVALID_PARAMETER 参数为空
 *   EFI_BUFFER_TOO_SMALL  SpdLen 不够读取 Byte 13
 *   EFI_UNSUPPORTED       遇到 reserved 编码
 */
EFI_STATUS EFIAPI OknDdr4SpdGetPrimaryBusWidthBits(IN CONST UINT8 *Spd,
                                                   IN UINTN        SpdLen,
                                                   OUT UINT16     *PrimaryBusWidthBits)
{
  UINT8      B, Code;
  EFI_STATUS Status;

  if (PrimaryBusWidthBits == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Status = OknDdr4SpdReadByte(Spd, SpdLen, DDR4_SPD_BYTE_MODULE_BUS_WIDTH, &B);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Code = (UINT8)(B & 0x7u);
  switch (Code) {
    case 0: *PrimaryBusWidthBits = 8; break;
    case 1: *PrimaryBusWidthBits = 16; break;
    case 2: *PrimaryBusWidthBits = 32; break;
    case 3: *PrimaryBusWidthBits = 64; break;
    default: return EFI_UNSUPPORTED;
  }
  return EFI_SUCCESS;
}

/**
 * Get the bus width extension in bits (ECC extension width).
 *
 * 目的/口径:
 *   - 解析 DIMM 的"总线扩展位宽(Bus width extension)"
 *   - 典型 ECC DIMM: extension = 8 bits(形成 72-bit 总线: 64+8)
 *   - 若非 ECC DIMM: extension = 0
 *
 * SPD 字段来源(协议位置):
 *   - Byte 13 (0x00D) Module Memory Bus Width, Table 32
 *       * Bits[4:3] = Bus width extension
 *           000b = 0
 *           001b = 8
 *           others = reserved
 *
 * 参数:
 *   @param[in]  Spd                 SPD 原始字节数组(至少覆盖到 Byte 13)
 *   @param[in]  SpdLen              Spd 长度
 *   @param[out] ExtensionWidthBits  输出: 0 或 8(单位 bit)
 *
 * 返回值:
 *   EFI_SUCCESS           成功
 *   EFI_INVALID_PARAMETER 参数为空
 *   EFI_BUFFER_TOO_SMALL  SpdLen 不够读取 Byte 13
 *   EFI_UNSUPPORTED       遇到 reserved 编码
 */
EFI_STATUS EFIAPI OknDdr4SpdGetBusWidthExtensionBits(IN CONST UINT8 *Spd,
                                                     IN UINTN        SpdLen,
                                                     OUT UINT16     *ExtensionWidthBits)
{
  UINT8      B, Code;
  EFI_STATUS Status;

  if (ExtensionWidthBits == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Status = OknDdr4SpdReadByte(Spd, SpdLen, DDR4_SPD_BYTE_MODULE_BUS_WIDTH, &B);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Code = (UINT8)((B >> 3) & 0x3u);
  switch (Code) {
    case 0: *ExtensionWidthBits = 0; break;
    case 1: *ExtensionWidthBits = 8; break;
    default: return EFI_UNSUPPORTED;
  }
  return EFI_SUCCESS;
}

/**
 * Get total bus width in bits (primary + optional extension).
 *
 * 目的/口径:
 *   - 获取 DIMM 的"总线总位宽"
 *   - 可选择是否把 ECC 扩展位宽算进去:
 *       IncludeExtension = FALSE -> 返回 primary bus width(例如 64)
 *       IncludeExtension = TRUE  -> 返回 primary + extension(例如 72)
 *
 * SPD 字段来源(协议位置):
 *   - Byte 13 (0x00D) Module Memory Bus Width, Table 32
 *       * Bits[2:0] = Primary bus width
 *       * Bits[4:3] = Bus width extension
 *
 * 计算逻辑:
 *   TotalBusWidthBits = PrimaryBusWidthBits + (IncludeExtension ? ExtensionWidthBits : 0)
 *
 * 参数:
 *   @param[in]  Spd               SPD 原始字节数组
 *   @param[in]  SpdLen            Spd 长度
 *   @param[in]  IncludeExtension  是否包含 ECC 扩展位宽
 *   @param[out] TotalBusWidthBits 输出: 总线位宽(单位 bit)
 *
 * 返回值:
 *   EFI_SUCCESS           成功
 *   EFI_INVALID_PARAMETER 参数为空
 *   EFI_BUFFER_TOO_SMALL  SpdLen 不够
 *   EFI_UNSUPPORTED       primary 或 extension 遇到 reserved 编码
 */
EFI_STATUS EFIAPI OknDdr4SpdGetTotalBusWidthBits(IN CONST UINT8 *Spd,
                                                 IN UINTN        SpdLen,
                                                 IN BOOLEAN      IncludeExtension,
                                                 OUT UINT16     *TotalBusWidthBits)
{
  EFI_STATUS Status;
  UINT16     Primary;
  UINT16     Ext;

  if (TotalBusWidthBits == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = OknDdr4SpdGetPrimaryBusWidthBits(Spd, SpdLen, &Primary);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Ext = 0;
  if (IncludeExtension) {
    Status = OknDdr4SpdGetBusWidthExtensionBits(Spd, SpdLen, &Ext);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  *TotalBusWidthBits = (UINT16)(Primary + Ext);
  return EFI_SUCCESS;
}

/**
 * Get primary SDRAM die count per package (1..8).
 *
 * 目的/口径:
 *   - 解析"每个 DRAM 封装内 die 的数量(Die Count)"
 *   - 对 SDP: 通常为 1
 *   - 对 DDP: 可能为 2
 *   - 对 3DS: 可能为 2/4/8 等(结合 Signal Loading 判断 3DS)
 *
 * SPD 字段来源(协议位置):
 *   - Byte 6 (0x006) Primary SDRAM Package Type, Table 20
 *       * Bits[6:4] = Die Count
 *           000b = 1
 *           ...
 *           111b = 8
 *
 * 参数:
 *   @param[in]  Spd       SPD 原始字节数组(至少覆盖到 Byte 6)
 *   @param[in]  SpdLen    Spd 长度
 *   @param[out] DieCount  输出: die 数(1..8)
 *
 * 返回值:
 *   EFI_SUCCESS           成功
 *   EFI_INVALID_PARAMETER 参数为空
 *   EFI_BUFFER_TOO_SMALL  SpdLen 不够读取 Byte 6
 */
EFI_STATUS EFIAPI OknDdr4SpdGetPrimaryDieCount(IN CONST UINT8 *Spd, IN UINTN SpdLen, OUT UINT8 *DieCount)
{
  UINT8      B;
  EFI_STATUS Status;

  if (DieCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Status = OknDdr4SpdReadByte(Spd, SpdLen, DDR4_SPD_BYTE_PRIMARY_SDRAM_PACKAGE_TYPE, &B);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  *DieCount = (UINT8)(((B >> 4) & 0x7u) + 1u); /* Bits[6:4] */
  return EFI_SUCCESS;
}

/**
 *  Check whether the primary SDRAM package type indicates 3DS.
 *
 *  目的/口径:
 *    - 判断该 DIMM 的 Primary SDRAM Package 是否为 3DS(Single load stack)
 *    - 3DS 的核心特征: Signal Loading 指示 Single load stack
 *
 *  SPD 字段来源(协议位置):
 *    - Byte 6 (0x006) Primary SDRAM Package Type, Table 20
 *        * Bits[1:0] = Signal Loading
 *            10b = Single load stack (3DS)
 *            11b = Reserved
 *
 *  参数:
 *    @param[in]  Spd    SPD 原始字节数组(至少覆盖到 Byte 6)
 *    @param[in]  SpdLen Spd 长度
 *    @param[out] Is3ds  输出: TRUE=3DS, FALSE=非 3DS
 *
 *  返回值:
 *    EFI_SUCCESS           成功
 *    EFI_INVALID_PARAMETER 参数为空
 *    EFI_BUFFER_TOO_SMALL  SpdLen 不够读取 Byte 6
 *    EFI_UNSUPPORTED       Signal Loading 为 reserved 编码
 */
EFI_STATUS EFIAPI OknDdr4SpdIsPrimary3ds(IN CONST UINT8 *Spd, IN UINTN SpdLen, OUT BOOLEAN *Is3ds)
{
  UINT8      B, Loading;
  EFI_STATUS Status;

  if (Is3ds == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Status = OknDdr4SpdReadByte(Spd, SpdLen, DDR4_SPD_BYTE_PRIMARY_SDRAM_PACKAGE_TYPE, &B);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Loading = (UINT8)(B & 0x3u); /* Bits[1:0] */
  if (Loading == 3u) {
    return EFI_UNSUPPORTED; /* 11b reserved */
  }

  *Is3ds = (Loading == 2u) ? TRUE : FALSE; /* 10b => 3DS */
  return EFI_SUCCESS;
}

/**
 * Estimate the number of DRAM packages per rank.
 *
 * 目的/口径:
 *   - 推导"每个 Rank 需要多少颗 DRAM 封装来覆盖总线位宽"
 *   - 可选: 是否包含 ECC(IncludeExtension)
 *       FALSE -> 仅 primary(数据颗粒数/每rank)
 *       TRUE  -> primary + extension(数据+ECC 总颗粒数/每rank)
 *
 * SPD 字段来源(协议位置):
 *   - Byte 12 (0x00C) Module Organization, Table 28
 *       * Bits[2:0] = SDRAM device width
 *       * Bit[6]    = Rank Mix
 *   - Byte 13 (0x00D) Module Memory Bus Width, Table 32
 *       * Bits[2:0] = Primary bus width
 *       * Bits[4:3] = Bus width extension
 *
 * 计算公式(对称模块):
 *   PackagesPerRank = TotalBusWidthBits(IncludeExtension) / SdramDeviceWidthBits
 *
 * 备注:
 *   - SPD 本身不直接提供"封装颗数", 此处为工程推导量.
 *   - 对 RankMix=1(非对称)模块, 推导可能需要结合 Byte10 等更复杂规则；
 *     为避免误判, 本函数默认返回 EFI_UNSUPPORTED.
 *
 * 参数:
 *   @param[in]  Spd              SPD 原始字节数组(至少覆盖 Byte 13)
 *   @param[in]  SpdLen           Spd 长度
 *   @param[in]  IncludeExtension 是否把 ECC 扩展位宽算进总线位宽
 *   @param[out] PackagesPerRank  输出: 每 rank 封装颗数
 *
 * 返回值:
 *   EFI_SUCCESS           成功
 *   EFI_INVALID_PARAMETER 参数为空
 *   EFI_BUFFER_TOO_SMALL  SpdLen 不够
 *   EFI_UNSUPPORTED       RankMix=1 或 reserved 编码
 *   EFI_COMPROMISED_DATA  总线位宽无法被 device width 整除(数据异常/非典型设计)
 */
EFI_STATUS EFIAPI OknDdr4SpdEstimatePackagesPerRank(IN CONST UINT8 *Spd,
                                                    IN UINTN        SpdLen,
                                                    IN BOOLEAN      IncludeExtension,
                                                    OUT UINT16     *PackagesPerRank)
{
  EFI_STATUS Status;
  BOOLEAN    Asym;
  UINT16     Bus;
  UINT16     Dev;

  if (PackagesPerRank == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = OknDdr4SpdIsRankMixAsymmetrical(Spd, SpdLen, &Asym);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  if (Asym) {
    return EFI_UNSUPPORTED;
  }

  Status = OknDdr4SpdGetTotalBusWidthBits(Spd, SpdLen, IncludeExtension, &Bus);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  Status = OknDdr4SpdGetSdramDeviceWidthBits(Spd, SpdLen, &Dev);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  if (Dev == 0 || (Bus % Dev) != 0) {
    return EFI_COMPROMISED_DATA;
  }

  *PackagesPerRank = (UINT16)(Bus / Dev);
  return EFI_SUCCESS;
}

/**
 *  Estimate total number of DRAM packages on the DIMM.
 *
 *  目的/口径:
 *    - 推导整条 DIMM 的"DRAM 封装总颗数"
 *    - 可选: 是否包含 ECC(IncludeExtension)
 *        FALSE -> 只算数据封装(不含 ECC)
 *        TRUE  -> 数据封装 + ECC 封装
 *
 *  SPD 字段来源(协议位置):
 *    - PackagesPerRank 的字段见 Ddr4SpdEstimatePackagesPerRank()
 *    - Byte 12 (0x00C) Module Organization, Table 28
 *        * Bits[5:3] = Package Ranks per DIMM
 *
 *  计算公式(对称模块):
 *    TotalPackages = PackagesPerRank * PackageRanksPerDimm
 *
 *  参数:
 *    @param[in]  Spd              SPD 原始字节数组
 *    @param[in]  SpdLen           Spd 长度
 *    @param[in]  IncludeExtension 是否把 ECC 扩展位宽算进去
 *    @param[out] TotalPackages    输出: 整条 DIMM 的封装颗数
 *
 *  返回值:
 *    EFI_SUCCESS / EFI_INVALID_PARAMETER / EFI_BUFFER_TOO_SMALL / EFI_UNSUPPORTED / EFI_COMPROMISED_DATA
 *    (内部依赖 Ddr4SpdEstimatePackagesPerRank)
 */
EFI_STATUS EFIAPI OknDdr4SpdEstimateTotalPackages(IN CONST UINT8 *Spd,
                                                  IN UINTN        SpdLen,
                                                  IN BOOLEAN      IncludeExtension,
                                                  OUT UINT16     *TotalPackages)
{
  EFI_STATUS Status;
  UINT16     Ppr;
  UINT8      Ranks;

  if (TotalPackages == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = OknDdr4SpdEstimatePackagesPerRank(Spd, SpdLen, IncludeExtension, &Ppr);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  Status = OknDdr4SpdGetPackageRanksPerDimm(Spd, SpdLen, &Ranks);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  *TotalPackages = (UINT16)(Ppr * (UINT16)Ranks);
  return EFI_SUCCESS;
}

/**
 * Get logical ranks per DIMM for symmetrical modules only.
 *
 * 目的/口径:
 *   - 计算"Logical Ranks per DIMM"(主要用于 3DS 堆叠器件的寻址/容量推导)
 *   - 仅支持对称模块(RankMix=0), 避免在非对称模块上算错.
 *   - 对非 3DS: logical ranks 通常等于 package ranks
 *   - 对 3DS: logical ranks = package ranks * die count(每个 package rank 内的 die 贡献逻辑 rank)
 *
 * SPD 字段来源(协议位置):
 *   - Byte 12 (0x00C) Module Organization, Table 28
 *       * Bits[5:3] = Package ranks per DIMM
 *       * Bit[6]    = Rank Mix
 *   - Byte 6 (0x006) Primary SDRAM Package Type, Table 20
 *       * Bits[6:4] = Die count
 *       * Bits[1:0] = Signal Loading (3DS)
 *   - 文档的 logical ranks 计算规则: 3DS 时与 die count 相乘
 *
 * 计算逻辑(对称模块):
 *   if (!Is3ds) LogicalRanks = PackageRanks
 *   else        LogicalRanks = PackageRanks * DieCount
 *
 * 参数:
 *   @param[in]  Spd          SPD 原始字节数组
 *   @param[in]  SpdLen       Spd 长度
 *   @param[out] LogicalRanks 输出: logical ranks per DIMM
 *
 * 返回值:
 *   EFI_SUCCESS           成功
 *   EFI_INVALID_PARAMETER 参数为空
 *   EFI_BUFFER_TOO_SMALL  SpdLen 不够
 *   EFI_UNSUPPORTED       RankMix=1 或 3DS 字段为 reserved
 */
EFI_STATUS EFIAPI OknDdr4SpdGetLogicalRanksPerDimm_SymmetricOnly(IN CONST UINT8 *Spd,
                                                                 IN UINTN        SpdLen,
                                                                 OUT UINT16     *LogicalRanks)
{
  EFI_STATUS Status;
  BOOLEAN    Asym;
  BOOLEAN    Is3ds;
  UINT8      PkgRanks;
  UINT8      Die;

  if (LogicalRanks == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = OknDdr4SpdIsRankMixAsymmetrical(Spd, SpdLen, &Asym);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  if (Asym) {
    return EFI_UNSUPPORTED;
  }

  Status = OknDdr4SpdGetPackageRanksPerDimm(Spd, SpdLen, &PkgRanks);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Status = OknDdr4SpdIsPrimary3ds(Spd, SpdLen, &Is3ds);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  if (!Is3ds) {
    *LogicalRanks = (UINT16)PkgRanks;
    return EFI_SUCCESS;
  }

  Status = OknDdr4SpdGetPrimaryDieCount(Spd, SpdLen, &Die);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  *LogicalRanks = (UINT16)((UINT16)PkgRanks * (UINT16)Die);
  return EFI_SUCCESS;
}

/**
 * Estimate total die count on the DIMM for 3DS symmetrical modules only.
 *
 * 目的/口径:
 *   - 对 3DS(Single load stack)模块, 推导整条 DIMM 的"总 die 数"
 *   - 仅对"对称模块 + 3DS"有明确意义:
 *       TotalDies = TotalPackages(可选含ECC) * DieCount
 *   - 若非 3DS: 该推导意义不大, 本函数通常返回 EFI_UNSUPPORTED.
 *
 * SPD 字段来源(协议位置):
 *   - TotalPackages 依赖:
 *       Byte12 (Table 28) + Byte13 (Table 32)(见 OknDdr4SpdEstimateTotalPackages)
 *   - DieCount / 3DS 依赖:
 *       Byte6 (0x006) Primary SDRAM Package Type, Table 20
 *   - RankMix 依赖:
 *       Byte12 bit6 (Table 28)
 *
 * 计算公式(对称且 3DS):
 *   TotalDies = TotalPackages(IncludeExtension) * DieCount
 *
 * 参数:
 *   @param[in]  Spd              SPD 原始字节数组
 *   @param[in]  SpdLen           Spd 长度
 *   @param[in]  IncludeExtension 是否把 ECC 扩展位宽算进 TotalPackages
 *   @param[out] TotalDies        输出: 整条 DIMM 的总 die 数
 *
 * 返回值:
 *   EFI_SUCCESS           成功
 *   EFI_INVALID_PARAMETER 参数为空
 *   EFI_BUFFER_TOO_SMALL  SpdLen 不够
 *   EFI_UNSUPPORTED       非 3DS 或 RankMix=1(非对称)或 reserved 编码
 *   EFI_COMPROMISED_DATA  推导过程中出现不可整除等异常
 */
EFI_STATUS EFIAPI OknDdr4SpdEstimateTotalDies_3dsSymmetricOnly(IN CONST UINT8 *Spd,
                                                               IN UINTN        SpdLen,
                                                               IN BOOLEAN      IncludeExtension,
                                                               OUT UINT32     *TotalDies)
{
  EFI_STATUS Status;
  BOOLEAN    Is3ds;
  UINT16     TotalPkg;
  UINT8      Die;

  if (TotalDies == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = OknDdr4SpdIsPrimary3ds(Spd, SpdLen, &Is3ds);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  if (!Is3ds) {
    return EFI_UNSUPPORTED; /* 非 3DS: 这个推导没意义, 直接报错 */
  }

  Status = OknDdr4SpdEstimateTotalPackages(Spd, SpdLen, IncludeExtension, &TotalPkg);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Status = OknDdr4SpdGetPrimaryDieCount(Spd, SpdLen, &Die);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  *TotalDies = (UINT32)TotalPkg * (UINT32)Die;
  return EFI_SUCCESS;
}

/**************************************************************************************************************** */
/**
 * Estimate the number of DATA DRAM packages per rank (excluding ECC).
 *
 * 目的/口径:
 *   - 计算"每个 Rank 用于承载数据位(Primary Bus)所需的 DRAM 封装颗数"
 *   - 明确: 不包含 ECC(不使用 bus width extension)
 *   - 这就是工程里常说的"每 rank 多少颗数据颗粒"
 *
 * SPD 字段来源(协议位置):
 *   - Byte 13 (0x00D) Module Memory Bus Width, Table 32
 *       * Bits[2:0] = Primary bus width (8/16/32/64 bits)
 *   - Byte 12 (0x00C) Module Organization, Table 28
 *       * Bits[2:0] = SDRAM device width (x4/x8/x16/x32)
 *       * Bit[6]    = Rank Mix (0=Symmetrical, 1=Asymmetrical)
 *
 * 计算公式(对称模块):
 *   DataPackagesPerRank = PrimaryBusWidthBits / SdramDeviceWidthBits
 *
 * 参数:
 *   @param[in]  Spd                  SPD 原始字节数组(至少覆盖到 Byte 13)
 *   @param[in]  SpdLen               Spd 长度
 *   @param[out] DataPackagesPerRank  输出: 每 rank 的"数据封装颗数"(不含 ECC)
 *
 * 返回值:
 *   EFI_SUCCESS           成功
 *   EFI_INVALID_PARAMETER 参数为空
 *   EFI_BUFFER_TOO_SMALL  SpdLen 不够读取相关字节
 *   EFI_UNSUPPORTED       RankMix=1(非对称混搭模块本函数不计算, 避免误判)
 *   EFI_COMPROMISED_DATA  出现不可整除等异常组合(SPD 数据不一致/非典型设计)
 */
EFI_STATUS EFIAPI OknDdr4SpdEstimateDataPackagesPerRank(IN CONST UINT8 *Spd,
                                                        IN UINTN        SpdLen,
                                                        OUT UINT16     *DataPackagesPerRank)
{
  EFI_STATUS Status;
  BOOLEAN    Asym;
  UINT16     PrimaryBus;
  UINT16     DevWidth;

  if (DataPackagesPerRank == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Byte12 bit6: Rank Mix (Table 28)
  Status = OknDdr4SpdIsRankMixAsymmetrical(Spd, SpdLen, &Asym);
  if (EFI_ERROR(Status))
    return Status;
  if (Asym)
    return EFI_UNSUPPORTED;

  // Byte13 bits[2:0]: Primary bus width (Table 32)
  Status = OknDdr4SpdGetPrimaryBusWidthBits(Spd, SpdLen, &PrimaryBus);
  if (EFI_ERROR(Status))
    return Status;

  // Byte12 bits[2:0]: SDRAM device width (Table 28)
  Status = OknDdr4SpdGetSdramDeviceWidthBits(Spd, SpdLen, &DevWidth);
  if (EFI_ERROR(Status))
    return Status;

  if (DevWidth == 0 || (PrimaryBus % DevWidth) != 0) {
    return EFI_COMPROMISED_DATA;
  }

  *DataPackagesPerRank = (UINT16)(PrimaryBus / DevWidth);
  return EFI_SUCCESS;
}

/**
 * Estimate total number of DATA DRAM packages on the DIMM (excluding ECC).
 *
 * 目的/口径:
 *   - 计算整条 DIMM 上"数据用 DRAM 封装颗数(不含 ECC)"
 *   - 等价于: 每 rank 数据封装颗数 × package ranks 数
 *
 * SPD 字段来源(协议位置):
 *   - DataPackagesPerRank 的字段见 OknDdr4SpdEstimateDataPackagesPerRank()
 *   - Byte 12 (0x00C) Module Organization, Table 28
 *       * Bits[5:3] = Number of Package Ranks per DIMM (000=1 ... 111=8)
 *
 * 计算公式(对称模块):
 *   TotalDataPackages = DataPackagesPerRank * PackageRanksPerDimm
 *
 * 参数:
 *   @param[in]  Spd               SPD 原始字节数组
 *   @param[in]  SpdLen            Spd 长度
 *   @param[out] TotalDataPackages 输出: 整条 DIMM 的数据封装颗数(不含 ECC)
 *
 * 返回值:
 *   EFI_SUCCESS / EFI_INVALID_PARAMETER / EFI_BUFFER_TOO_SMALL / EFI_UNSUPPORTED / EFI_COMPROMISED_DATA
 *   (含义同上；内部依赖 OknDdr4SpdEstimateDataPackagesPerRank)
 */
EFI_STATUS EFIAPI OknDdr4SpdEstimateTotalDataPackages(IN CONST UINT8 *Spd,
                                                      IN UINTN        SpdLen,
                                                      OUT UINT16     *TotalDataPackages)
{
  EFI_STATUS Status;
  UINT16     Ppr;
  UINT8      Ranks;

  if (TotalDataPackages == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = OknDdr4SpdEstimateDataPackagesPerRank(Spd, SpdLen, &Ppr);
  if (EFI_ERROR(Status))
    return Status;

  // Byte12 bits[5:3]: Package Ranks per DIMM (Table 28)
  Status = OknDdr4SpdGetPackageRanksPerDimm(Spd, SpdLen, &Ranks);
  if (EFI_ERROR(Status))
    return Status;

  *TotalDataPackages = (UINT16)(Ppr * (UINT16)Ranks);
  return EFI_SUCCESS;
}

/**
 * Estimate the number of ECC DRAM packages per rank (ECC-only).
 *
 * 目的/口径:
 *   - 计算"每个 Rank 为 ECC 扩展位宽(bw extension)所需的 DRAM 封装颗数"
 *   - 只计算 ECC 扩展部分, 不包含 primary 数据位宽
 *   - 若 extension=0 表示无 ECC, 则返回 0
 *
 * SPD 字段来源(协议位置):
 *   - Byte 13 (0x00D) Module Memory Bus Width, Table 32
 *       * Bits[4:3] = Bus width extension (000=0, 001=8 bits; others reserved)
 *   - Byte 12 (0x00C) Module Organization, Table 28
 *       * Bits[2:0] = SDRAM device width (x4/x8/x16/x32)
 *       * Bit[6]    = Rank Mix (0/1)
 *
 * 计算公式(对称模块):
 *   EccPackagesPerRank = ExtensionWidthBits / SdramDeviceWidthBits
 *   - 若 ExtensionWidthBits == 0, 则 EccPackagesPerRank = 0
 *
 * 参数:
 *   @param[in]  Spd               SPD 原始字节数组
 *   @param[in]  SpdLen            Spd 长度
 *   @param[out] EccPackagesPerRank 输出: 每 rank 的 ECC 封装颗数(ECC-only)
 *
 * 返回值:
 *   EFI_SUCCESS           成功(包括无 ECC 返回 0)
 *   EFI_INVALID_PARAMETER 参数为空
 *   EFI_BUFFER_TOO_SMALL  SpdLen 不够
 *   EFI_UNSUPPORTED       RankMix=1(非对称混搭不算)
 *   EFI_UNSUPPORTED       extension 编码为 reserved
 *   EFI_COMPROMISED_DATA  不可整除等异常组合
 */
EFI_STATUS EFIAPI OknDdr4SpdEstimateEccPackagesPerRank(IN CONST UINT8 *Spd,
                                                       IN UINTN        SpdLen,
                                                       OUT UINT16     *EccPackagesPerRank)
{
  EFI_STATUS Status;
  BOOLEAN    Asym;
  UINT16     ExtBus;
  UINT16     DevWidth;

  if (EccPackagesPerRank == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = OknDdr4SpdIsRankMixAsymmetrical(Spd, SpdLen, &Asym);
  if (EFI_ERROR(Status))
    return Status;
  if (Asym)
    return EFI_UNSUPPORTED;

  // Byte13 bits[4:3]: Bus width extension (Table 32) : 0 or 8 bits typical
  Status = OknDdr4SpdGetBusWidthExtensionBits(Spd, SpdLen, &ExtBus);
  if (EFI_ERROR(Status))
    return Status;

  // 没有 ECC 扩展 => 0 颗
  if (ExtBus == 0) {
    *EccPackagesPerRank = 0;
    return EFI_SUCCESS;
  }

  Status = OknDdr4SpdGetSdramDeviceWidthBits(Spd, SpdLen, &DevWidth);
  if (EFI_ERROR(Status))
    return Status;

  if (DevWidth == 0 || (ExtBus % DevWidth) != 0) {
    return EFI_COMPROMISED_DATA;
  }

  *EccPackagesPerRank = (UINT16)(ExtBus / DevWidth);
  return EFI_SUCCESS;
}

/**
 * Estimate total number of ECC DRAM packages on the DIMM (ECC-only).
 *
 * 目的/口径:
 *   - 计算整条 DIMM 上"ECC 用 DRAM 封装颗数"
 *   - 等价于: 每 rank ECC 封装颗数 × package ranks 数
 *   - 若无 ECC(extension=0), 则输出 0
 *
 * SPD 字段来源(协议位置):
 *   - EccPackagesPerRank 的字段见 Ddr4SpdEstimateEccPackagesPerRank()
 *   - Byte 12 (0x00C) Module Organization, Table 28
 *       * Bits[5:3] = Package Ranks per DIMM
 *
 * 计算公式(对称模块):
 *   TotalEccPackages = EccPackagesPerRank * PackageRanksPerDimm
 *
 * 参数:
 *   @param[in]  Spd              SPD 原始字节数组
 *   @param[in]  SpdLen           Spd 长度
 *   @param[out] TotalEccPackages 输出: 整条 DIMM 的 ECC 封装颗数
 *
 * 返回值:
 *   EFI_SUCCESS / EFI_INVALID_PARAMETER / EFI_BUFFER_TOO_SMALL / EFI_UNSUPPORTED / EFI_COMPROMISED_DATA
 *   (内部依赖 Ddr4SpdEstimateEccPackagesPerRank)
 */
EFI_STATUS EFIAPI OknDdr4SpdEstimateTotalEccPackages(IN CONST UINT8 *Spd, IN UINTN SpdLen, OUT UINT16 *TotalEccPackages)
{
  EFI_STATUS Status;
  UINT16     EccPpr;
  UINT8      Ranks;

  if (TotalEccPackages == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = OknDdr4SpdEstimateEccPackagesPerRank(Spd, SpdLen, &EccPpr);
  if (EFI_ERROR(Status))
    return Status;

  Status = OknDdr4SpdGetPackageRanksPerDimm(Spd, SpdLen, &Ranks);
  if (EFI_ERROR(Status))
    return Status;

  *TotalEccPackages = (UINT16)(EccPpr * (UINT16)Ranks);
  return EFI_SUCCESS;
}
