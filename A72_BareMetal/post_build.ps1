[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Elf,
    [string]$PdkPackages = 'E:\work\Tools\ti-processor-sdk-rtos-j721e-evm-09_02_00_05\pdk_jacinto_09_02_00_30\packages'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Invoke-NativeTool {
    param(
        [Parameter(Mandatory = $true)][string]$Tool,
        [Parameter(Mandatory = $true)][string[]]$ToolArguments
    )
    & $Tool @ToolArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed ($LASTEXITCODE): $Tool $($ToolArguments -join ' ')"
    }
}

function Find-GccTool {
    param([Parameter(Mandatory = $true)][string]$Name)
    $fromPath = Get-Command $Name -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }
    $knownBin = 'C:\ti\ti-processor-sdk-rtos-j721e-evm-09_02_00_05\gcc-arm-9.2-2019.12-mingw-w64-i686-aarch64-none-elf\bin'
    $candidate = Join-Path $knownBin $Name
    if (Test-Path -LiteralPath $candidate) {
        return $candidate
    }
    throw "AArch64 GCC tool was not found: $Name"
}

function Find-ImageTool {
    param([Parameter(Mandatory = $true)][string]$Name)
    $candidates = @(
        (Join-Path $PSScriptRoot "..\CPHMC_boot\CPHMC_1A\tools\$Name"),
        (Join-Path $PdkPackages "ti\boot\sbl\tools\$Name")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    throw "TI SBL image tool was not found: $Name"
}

$elfPath = (Resolve-Path -LiteralPath $Elf).Path
$outputDir = Split-Path -Parent $elfPath
$stem = [System.IO.Path]::GetFileNameWithoutExtension($elfPath)
$legacyMsmcBinary = Join-Path $outputDir "$stem.msmc.bin"
$ddrBinary = Join-Path $outputDir "$stem.ddr.bin"
$rprc = Join-Path $outputDir "$stem.rprc"
$atfElf = Join-Path $outputDir 'bl31.elf'
$atfRprc = Join-Path $outputDir 'bl31.rprc'
$opteeElf = Join-Path $outputDir 'bl32.elf'
$opteeRprc = Join-Path $outputDir 'bl32.rprc'
$appimage = Join-Path $outputDir "$stem.appimage"
$flashAppimage = Join-Path $outputDir 'atf_optee_spl.appimage.hs_fs'

# Older revisions emitted an A72 MSMC payload. The official ATF layout leaves
# MSMC ownership to BL31 and the existing R5F allocations, so remove that
# obsolete generated artifact if it is still present in the CCS output folder.
if (Test-Path -LiteralPath $legacyMsmcBinary) {
    [System.IO.File]::Delete($legacyMsmcBinary)
}

$objcopy = Find-GccTool -Name 'aarch64-none-elf-objcopy.exe'
$linker = Find-GccTool -Name 'aarch64-none-elf-ld.exe'
$readelf = Find-GccTool -Name 'aarch64-none-elf-readelf.exe'
$sizeTool = Find-GccTool -Name 'aarch64-none-elf-size.exe'
$out2rprc = Find-ImageTool -Name 'out2rprc.exe'
$imageGen = Find-ImageTool -Name 'MulticoreImageGen.exe'
$combinedBinDir = Join-Path $PdkPackages 'ti\boot\sbl\tools\combined_appimage\bin\j721e_evm'
$atfBin = Join-Path $combinedBinDir 'bl31.bin'
$opteeBin = Join-Path $combinedBinDir 'bl32.bin'
foreach ($officialImage in @($atfBin, $opteeBin)) {
    if (-not (Test-Path -LiteralPath $officialImage)) {
        throw "Required TI J721E GP firmware image does not exist: $officialImage"
    }
}

$platformMk = Join-Path $PdkPackages 'ti\build\makerules\platform.mk'
if (-not (Test-Path -LiteralPath $platformMk)) {
    throw "PDK platform.mk does not exist: $platformMk"
}
$platformText = Get-Content -LiteralPath $platformMk -Raw
if ($platformText -notmatch 'SBL_CORE_ID_mpu1_0\s*=\s*0(?:\s|$)') {
    throw "Unexpected mpu1_0 SBL core ID in $platformMk"
}

$elfHeader = (& $readelf -h $elfPath) -join "`n"
if ($LASTEXITCODE -ne 0 -or
    $elfHeader -notmatch 'Machine:\s+AArch64' -or
    $elfHeader -notmatch 'Entry point address:\s+0x80080000') {
    throw "ELF verification failed. Expected AArch64 BL33 entry 0x80080000.`n$elfHeader"
}

$programHeaders = (& $readelf -lW $elfPath) -join "`n"
if ($LASTEXITCODE -ne 0 -or
    $programHeaders -notmatch 'LOAD\s+0x[0-9a-fA-F]+\s+0x0*80080000' -or
    $programHeaders -match 'LOAD\s+0x[0-9a-fA-F]+\s+0x0*700[0-9a-fA-F]{5}') {
    throw "ELF LOAD segment verification failed.`n$programHeaders"
}

# Compact diagnostic binary; AppImage/RPRC remains the flash image.
Invoke-NativeTool -Tool $objcopy -ToolArguments @('-O', 'binary', '--only-section=.boot', '--only-section=.vectors', '--only-section=.text', '--only-section=.rodata', '--only-section=.data', $elfPath, $ddrBinary)
Invoke-NativeTool -Tool $out2rprc -ToolArguments @($elfPath, $rprc)

# TI's prebuilt BL31 expects OP-TEE at 0x9e800000 and BL33 at 0x80080000.
# Convert the official raw firmware blobs to RPRC exactly as TI's
# combined_appimage makefile does. Only BL31 owns mpu1_0; OP-TEE and BL33 are
# load-only records so TBL starts the A72 at the ATF entry point.
Invoke-NativeTool -Tool $linker -ToolArguments @('-b', 'binary', '-A', 'aarch64', '--oformat', 'elf64-littleaarch64', '--section-start=.data=0x70000000', '-e', '0x70000000', $atfBin, '-o', $atfElf)
Invoke-NativeTool -Tool $out2rprc -ToolArguments @($atfElf, $atfRprc)
Invoke-NativeTool -Tool $linker -ToolArguments @('-b', 'binary', '-A', 'aarch64', '--oformat', 'elf64-littleaarch64', '--section-start=.data=0x9e800000', '-e', '0x9e800000', $opteeBin, '-o', $opteeElf)
Invoke-NativeTool -Tool $out2rprc -ToolArguments @($opteeElf, $opteeRprc)
Invoke-NativeTool -Tool $imageGen -ToolArguments @('LE', '55', $appimage, '31', $opteeRprc, '31', $rprc, '0', $atfRprc)

if ((Get-Item -LiteralPath $appimage).Length -ge 0x100000) {
    throw 'The appimage does not fit in the existing OSPI 0x180000..0x27ffff slot.'
}

$appBytes = [System.IO.File]::ReadAllBytes($appimage)
$imageCount = 3
$headerSize = 16 + (8 * $imageCount) + 8
if (($appBytes.Length -lt $headerSize) -or
    ([BitConverter]::ToUInt32($appBytes, 0) -ne 0x5254534D) -or
    ([BitConverter]::ToUInt32($appBytes, 4) -ne $imageCount) -or
    ([BitConverter]::ToUInt32($appBytes, 8) -ne 55) -or
    ([BitConverter]::ToUInt32($appBytes, 12) -ne 1) -or
    ([BitConverter]::ToUInt32($appBytes, $headerSize - 4) -ne 0x444E454D)) {
    throw 'Generated appimage has an invalid TI J721E multicore header.'
}

# MulticoreImageGen stores an eight-byte core-ID/file-offset record per image.
# Verify both ownership and each embedded RPRC entry point, including the
# official ordering used by TI: OP-TEE, BL33, then ATF.
$expectedCoreIds = @(31, 31, 0)
$expectedEntries = @(0x9e800000L, 0x80080000L, 0x70000000L)
$previousOffset = $headerSize
for ($imageIndex = 0; $imageIndex -lt $imageCount; ++$imageIndex) {
    $recordOffset = 16 + (8 * $imageIndex)
    $coreId = [BitConverter]::ToUInt32($appBytes, $recordOffset)
    $embeddedOffset = [BitConverter]::ToUInt32($appBytes, $recordOffset + 4)
    if (($coreId -ne $expectedCoreIds[$imageIndex]) -or
        ($embeddedOffset -lt $headerSize) -or
        ($embeddedOffset -ge $appBytes.Length) -or
        ($embeddedOffset -lt $previousOffset) -or
        (($embeddedOffset + 8) -gt $appBytes.Length) -or
        ([BitConverter]::ToUInt32($appBytes, $embeddedOffset) -ne 0x43525052) -or
        ([BitConverter]::ToUInt32($appBytes, $embeddedOffset + 4) -ne $expectedEntries[$imageIndex])) {
        throw "Invalid embedded image $imageIndex in the TI combined appimage."
    }
    $previousOffset = $embeddedOffset
}

$rprcBytes = [System.IO.File]::ReadAllBytes($rprc)
if (($rprcBytes.Length -lt 20) -or
    ([BitConverter]::ToUInt32($rprcBytes, 0) -ne 0x43525052) -or
    ([BitConverter]::ToUInt32($rprcBytes, 4) -ne 0x80080000L)) {
    throw 'Generated RPRC magic or entry address is invalid.'
}

$sectionCount = [BitConverter]::ToUInt32($rprcBytes, 12)
$rprcOffset = 20
$hasDdrSection = $false
$ddrStart = [Convert]::ToUInt64('80080000', 16)
$ddrLimit = [Convert]::ToUInt64('82000000', 16)
for ($sectionIndex = 0; $sectionIndex -lt $sectionCount; ++$sectionIndex) {
    if (($rprcOffset + 20) -gt $rprcBytes.Length) {
        throw "Truncated RPRC section header $sectionIndex."
    }
    $sectionAddress = [BitConverter]::ToUInt32($rprcBytes, $rprcOffset)
    $sectionAddressHigh = [BitConverter]::ToUInt32($rprcBytes, $rprcOffset + 4)
    $sectionSize = [BitConverter]::ToUInt32($rprcBytes, $rprcOffset + 8)
    $sectionEnd = [uint64]$sectionAddress + [uint64]$sectionSize
    $sectionStart = [uint64]$sectionAddress
    $inDdr = ($sectionStart -ge $ddrStart) -and ($sectionEnd -le $ddrLimit)
    if (($sectionAddressHigh -ne 0) -or (-not $inDdr)) {
        throw "RPRC section $sectionIndex is outside the A72 DDR reservation."
    }
    $hasDdrSection = $hasDdrSection -or $inDdr
    $rprcOffset += 20 + $sectionSize
}
if ($rprcOffset -ne $rprcBytes.Length) {
    throw 'RPRC section table does not consume the complete file.'
}
if (-not $hasDdrSection) {
    throw 'Generated RPRC does not contain the A72 DDR image.'
}

# Deployment alias: preserve the existing updater filename. The content is now
# TI ATF + TI OP-TEE + the A72 bare-metal BL33; it does not contain U-Boot SPL.
Copy-Item -LiteralPath $appimage -Destination $flashAppimage -Force

Write-Host ''
Invoke-NativeTool -Tool $sizeTool -ToolArguments @('-A', $elfPath)
Write-Host ''
Write-Host 'A72 No-OS post-build verification: PASS'
Write-Host "  ELF      : $elfPath"
Write-Host "  DDR BIN  : $ddrBinary"
Write-Host "  RPRC     : $rprc"
Write-Host "  APPIMAGE : $appimage"
Write-Host "  FLASH    : $flashAppimage"
Write-Host '  Images   : TI OP-TEE(load-only) + A72 BL33(load-only) + TI ATF(mpu1_0)'
Write-Host '  A72 entry: 0x70000000 (TI ATF BL31, EL3)'
Write-Host '  BL33     : 0x80080000 (A72 bare-metal, EL1 after hand-off)'
Write-Host '  ATF      : 0x70000000..0x7001ffff'
Write-Host '  BL33 DDR : 0x80080000..0x81ffffff (vectors/MMU tables/stacks included)'
