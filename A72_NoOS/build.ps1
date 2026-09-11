[CmdletBinding()]
param(
    [ValidateSet('Wkup', 'Main')]
    [string]$Console = 'Wkup',

    [string]$PdkPackages = 'E:\work\Tools\ti-processor-sdk-rtos-j721e-evm-09_02_00_05\pdk_jacinto_09_02_00_30\packages',

    [string]$ToolchainRoot = '',

    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = $PSScriptRoot
$variant = $Console.ToLowerInvariant()
$outputDir = Join-Path $projectRoot "build\$variant"
$objectDir = Join-Path $outputDir 'obj'
$stem = "cphmc_a72_noos_$variant"

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

function Find-ToolchainBin {
    param([string]$RequestedRoot)

    if ($RequestedRoot) {
        $requested = (Resolve-Path -LiteralPath $RequestedRoot).Path
        if (Test-Path -LiteralPath (Join-Path $requested 'aarch64-none-elf-gcc.exe')) {
            return $requested
        }
        if (Test-Path -LiteralPath (Join-Path $requested 'bin\aarch64-none-elf-gcc.exe')) {
            return (Join-Path $requested 'bin')
        }
        throw "No aarch64-none-elf-gcc.exe under ToolchainRoot: $RequestedRoot"
    }

    $gccCommand = Get-Command 'aarch64-none-elf-gcc.exe' -ErrorAction SilentlyContinue
    if ($gccCommand) {
        return (Split-Path -Parent $gccCommand.Source)
    }

    $knownRoot = 'C:\ti\ti-processor-sdk-rtos-j721e-evm-09_02_00_05\gcc-arm-9.2-2019.12-mingw-w64-i686-aarch64-none-elf\bin'
    if (Test-Path -LiteralPath (Join-Path $knownRoot 'aarch64-none-elf-gcc.exe')) {
        return $knownRoot
    }

    throw 'AArch64 GCC was not found. Pass -ToolchainRoot <gcc-arm-... directory>.'
}

function Find-ImageTool {
    param([string]$Name)

    $candidates = @(
        (Join-Path $projectRoot "..\CPHMC_boot\CPHMC_1A\tools\$Name"),
        (Join-Path $PdkPackages "ti\boot\sbl\tools\$Name")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "Image tool not found: $Name"
}

if ($Clean) {
    if (Test-Path -LiteralPath $outputDir) {
        Remove-Item -LiteralPath $outputDir -Recurse -Force
    }
    Write-Host "Cleaned $outputDir"
    return
}

if (-not (Test-Path -LiteralPath $PdkPackages)) {
    throw "PDK packages path does not exist: $PdkPackages"
}

$platformMk = Join-Path $PdkPackages 'ti\build\makerules\platform.mk'
$platformText = Get-Content -LiteralPath $platformMk -Raw
if ($platformText -notmatch 'SBL_CORE_ID_mpu1_0\s*=\s*0(?:\s|$)') {
    throw "Unexpected mpu1_0 SBL core ID in $platformMk; refusing to create an incompatible appimage."
}

$toolBin = Find-ToolchainBin -RequestedRoot $ToolchainRoot
$gcc = Join-Path $toolBin 'aarch64-none-elf-gcc.exe'
$objcopy = Join-Path $toolBin 'aarch64-none-elf-objcopy.exe'
$readelf = Join-Path $toolBin 'aarch64-none-elf-readelf.exe'
$sizeTool = Join-Path $toolBin 'aarch64-none-elf-size.exe'
$out2rprc = Find-ImageTool -Name 'out2rprc.exe'
$imageGen = Find-ImageTool -Name 'MulticoreImageGen.exe'

$consoleBase = if ($Console -eq 'Main') { '0x02800000UL' } else { '0x42300000UL' }

New-Item -ItemType Directory -Force -Path $objectDir | Out-Null

$includeDir = Join-Path $projectRoot 'include'
$linkerScript = Join-Path $projectRoot 'linker\j721e_a72_0.ld'
$commonFlags = @(
    '-mcpu=cortex-a72',
    '-march=armv8-a',
    '-mstrict-align',
    '-ffreestanding',
    '-fno-builtin',
    '-fno-common',
    '-fno-pic',
    '-fno-pie',
    '-fno-stack-protector',
    '-fno-asynchronous-unwind-tables',
    '-fno-unwind-tables',
    '-ffunction-sections',
    '-fdata-sections',
    '-mgeneral-regs-only',
    '-Wall',
    '-Wextra',
    '-Werror',
    '-O2',
    '-g3',
    "-DA72_CONSOLE_UART_BASE=$consoleBase",
    "-I$includeDir"
)

$sources = @(
    @{ Path = (Join-Path $projectRoot 'src\startup.S'); Object = (Join-Path $objectDir 'startup.o'); IsC = $false },
    @{ Path = (Join-Path $projectRoot 'src\main.c');    Object = (Join-Path $objectDir 'main.o');    IsC = $true  },
    @{ Path = (Join-Path $projectRoot 'src\uart.c');    Object = (Join-Path $objectDir 'uart.o');    IsC = $true  },
    @{ Path = (Join-Path $projectRoot 'src\timer.c');   Object = (Join-Path $objectDir 'timer.o');   IsC = $true  }
)

Write-Host "Building $stem (console base $consoleBase)"
foreach ($source in $sources) {
    $compileArgs = @($commonFlags)
    if ($source.IsC) {
        $compileArgs += '-std=c11'
    }
    $compileArgs += @('-c', $source.Path, '-o', $source.Object)
    Invoke-NativeTool -Tool $gcc -ToolArguments $compileArgs
}

$elf = Join-Path $outputDir "$stem.elf"
$binary = Join-Path $outputDir "$stem.bin"
$rprc = Join-Path $outputDir "$stem.rprc"
$appimage = Join-Path $outputDir "$stem.appimage"
$map = Join-Path $outputDir "$stem.map"

$objects = $sources | ForEach-Object { $_.Object }
$linkArgs = @(
    '-mcpu=cortex-a72',
    '-nostdlib',
    '-static',
    '-no-pie',
    "-Wl,-T,$linkerScript",
    "-Wl,-Map,$map",
    '-Wl,--gc-sections',
    '-Wl,--build-id=none',
    '-o', $elf
) + $objects
Invoke-NativeTool -Tool $gcc -ToolArguments $linkArgs
Invoke-NativeTool -Tool $objcopy -ToolArguments @('-O', 'binary', $elf, $binary)

$elfHeader = (& $readelf -h $elf) -join "`n"
if ($LASTEXITCODE -ne 0) {
    throw 'readelf failed while checking the ELF header.'
}
if (($elfHeader -notmatch 'Machine:\s+AArch64') -or ($elfHeader -notmatch 'Entry point address:\s+0x70000000')) {
    throw "ELF verification failed. Expected AArch64 with entry 0x70000000.`n$elfHeader"
}

$programHeaders = (& $readelf -lW $elf) -join "`n"
if ($LASTEXITCODE -ne 0 -or $programHeaders -notmatch 'LOAD\s+0x[0-9a-fA-F]+\s+0x0*70000000') {
    throw "ELF LOAD segment verification failed.`n$programHeaders"
}

Invoke-NativeTool -Tool $out2rprc -ToolArguments @($elf, $rprc)
Invoke-NativeTool -Tool $imageGen -ToolArguments @('LE', '55', $appimage, '0', $rprc)

if ((Get-Item -LiteralPath $appimage).Length -ge 0x100000) {
    throw 'The appimage no longer fits in the existing 0x180000..0x27ffff OSPI slot.'
}

$appBytes = [System.IO.File]::ReadAllBytes($appimage)
if (($appBytes.Length -lt 32) -or
    ([BitConverter]::ToUInt32($appBytes, 0) -ne 0x5254534D) -or
    ([BitConverter]::ToUInt32($appBytes, 4) -ne 1) -or
    ([BitConverter]::ToUInt32($appBytes, 8) -ne 55) -or
    ([BitConverter]::ToUInt32($appBytes, 12) -ne 1) -or
    ([BitConverter]::ToUInt32($appBytes, 16) -ne 0) -or
    ([BitConverter]::ToUInt32($appBytes, 20) -ne 0x20) -or
    ([BitConverter]::ToUInt32($appBytes, 28) -ne 0x444E454D)) {
    throw 'Generated appimage header is not J721E(55), mpu1_0(0), version 1.'
}

$rprcBytes = [System.IO.File]::ReadAllBytes($rprc)
if (($rprcBytes.Length -lt 20) -or
    ([BitConverter]::ToUInt32($rprcBytes, 0) -ne 0x43525052) -or
    ([BitConverter]::ToUInt32($rprcBytes, 4) -ne 0x70000000)) {
    throw 'Generated RPRC magic or entry address is invalid.'
}

$sectionCount = [BitConverter]::ToUInt32($rprcBytes, 12)
$rprcOffset = 20
for ($sectionIndex = 0; $sectionIndex -lt $sectionCount; ++$sectionIndex) {
    if (($rprcOffset + 20) -gt $rprcBytes.Length) {
        throw "Truncated RPRC section header $sectionIndex."
    }
    $sectionAddress = [BitConverter]::ToUInt32($rprcBytes, $rprcOffset)
    $sectionAddressHigh = [BitConverter]::ToUInt32($rprcBytes, $rprcOffset + 4)
    $sectionSize = [BitConverter]::ToUInt32($rprcBytes, $rprcOffset + 8)
    $sectionEnd = [uint64]$sectionAddress + [uint64]$sectionSize
    if (($sectionAddressHigh -ne 0) -or
        ($sectionAddress -lt 0x70000000) -or
        ($sectionEnd -gt 0x70700000)) {
        throw "RPRC section $sectionIndex lies outside reserved MSMC SRAM."
    }
    $rprcOffset += 20 + $sectionSize
}
if ($rprcOffset -ne $rprcBytes.Length) {
    throw 'RPRC section table does not consume the complete file.'
}

# Use the .NET implementation instead of Get-FileHash.  Some CCS-launched
# PowerShell environments do not auto-load the module which owns that cmdlet.
$sha256Algorithm = [System.Security.Cryptography.SHA256]::Create()
try {
    $appImageSha256 = [BitConverter]::ToString(
        $sha256Algorithm.ComputeHash([System.IO.File]::ReadAllBytes($appimage))
    ).Replace('-', '')
}
finally {
    $sha256Algorithm.Dispose()
}

Write-Host ''
Invoke-NativeTool -Tool $sizeTool -ToolArguments @('-A', $elf)
Write-Host ''
Write-Host 'Build and static verification: PASS'
Write-Host "  ELF      : $elf"
Write-Host "  BIN      : $binary"
Write-Host "  RPRC     : $rprc"
Write-Host "  APPIMAGE : $appimage"
Write-Host "  SHA256   : $appImageSha256"
Write-Host '  SoC/core : J721E(55) / mpu1_0(0)'
Write-Host '  MSMC SRAM: 0x70000000..0x706fffff'
