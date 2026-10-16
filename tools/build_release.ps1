param(
    [string]$Manifest = "$PSScriptRoot\..\docs\firmware-manifest.json",
    [string]$StartAt = "",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot\..").Path
$firmware = Join-Path $root "firmware"
$webOutput = Join-Path $root "docs\firmware"
$releaseOutput = Join-Path $root "release"
$work = Join-Path ([System.IO.Path]::GetTempPath()) "retroreader-release-build"

if (-not $env:IDF_PATH) {
    throw "Activate ESP-IDF before running this script."
}
if (-not ([System.IO.Path]::GetFullPath($work)).StartsWith(
        [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath()),
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to use a build directory outside the temporary folder."
}

$catalogue = Get-Content -Raw -LiteralPath $Manifest | ConvertFrom-Json
$readerBuild = Join-Path $work "reader"
$networkBuild = Join-Path $work "network"
New-Item -ItemType Directory -Force -Path $work,$webOutput,$releaseOutput |
    Out-Null
if ($Clean) {
    Remove-Item -LiteralPath $readerBuild,$networkBuild -Recurse -Force -ErrorAction SilentlyContinue
}

$variants = @($catalogue.variants)
if ($StartAt) {
    $startIndex = [Array]::FindIndex(
        $variants,
        [Predicate[object]] { param($item) $item.id -eq $StartAt })
    if ($startIndex -lt 0) { throw "Unknown release variant: $StartAt" }
    $variants = $variants[$startIndex..($variants.Count - 1)]
}

foreach ($variant in $variants) {
    $common = @()
    foreach ($definition in $variant.cmake) { $common += "-D$definition" }

    & idf.py -C $firmware -B $readerBuild @common "-DRR_APP_SOURCE=paragraph_reader_stable_main.c" build
    if ($LASTEXITCODE) { throw "Reader build failed: $($variant.id)" }

    & idf.py -C $firmware -B $networkBuild @common "-DRR_APP_SOURCE=network_services_main.c" build
    if ($LASTEXITCODE) { throw "Network build failed: $($variant.id)" }

    $merged = Join-Path $work $variant.image
    $merge = @("-m", "esptool", "--chip", "esp32s3", "merge_bin", "--flash_size", "16MB", "--output", $merged,
        "0x0", (Join-Path $readerBuild "bootloader\bootloader.bin"),
        "0x8000", (Join-Path $readerBuild "partition_table\partition-table.bin"),
        "0xd000", (Join-Path $readerBuild "ota_data_initial.bin"),
        "0x10000", (Join-Path $readerBuild "retroreader.bin"),
        "0x610000", (Join-Path $networkBuild "retroreader.bin"))
    & python @merge
    if ($LASTEXITCODE) { throw "Image merge failed: $($variant.id)" }

    Copy-Item -LiteralPath $merged -Destination (Join-Path $webOutput $variant.image) -Force
    Copy-Item -LiteralPath $merged -Destination (Join-Path $releaseOutput $variant.image) -Force
}

Copy-Item -LiteralPath $Manifest -Destination (Join-Path $releaseOutput "firmware-manifest.json") -Force
$checksums = foreach ($variant in $catalogue.variants) {
    $path = Join-Path $releaseOutput $variant.image
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLower()
    "$hash  $($variant.image)"
}
$checksums | Set-Content -LiteralPath (Join-Path $releaseOutput "SHA256SUMS.txt") -Encoding ascii
Write-Host "Built $($variants.Count) RetroReader $($catalogue.version) variants from '$($variants[0].id)'."
