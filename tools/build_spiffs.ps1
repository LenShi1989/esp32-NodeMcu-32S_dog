#requires -Version 5.1
<#
.SYNOPSIS
  建立 DogBot（ESP32 NodeMCU-32S）的 SPIFFS 映像檔（spiffs.bin），可直接由網頁 OTA 上傳。

.DESCRIPTION
  使用 ESP32 Arduino core 內附的 mkspiffs 工具，將 esp32_nodemcu_32s_dog/data
  打包成 SPIFFS 映像。

  映像大小必須「完全等於」裝置上的 SPIFFS 分割區大小，否則 SPIFFS.begin()
  會掛載失敗並自動格式化，網頁就會顯示「找不到 /index.html」。
  因此本指令稿會依下列順序自動偵測分割區大小：

    1. -Size 參數（手動指定，優先）
    2. -Scheme 指定的分割表（ESP32 core 的 tools/partitions/<Scheme>.csv）
    3. 最近一次編譯產生的 build/<fqbn>/partitions.csv
    4. ESP32 core 的 default.csv（NodeMCU-32S 在 core 3.x 固定使用 default）

.PARAMETER DataDir
  前端檔案資料夾，預設 esp32_nodemcu_32s_dog/data。

.PARAMETER Out
  輸出檔路徑，預設 <專案根目錄>/build/spiffs.bin。

.PARAMETER Size
  直接指定 SPIFFS 分割區大小（例如 0x160000）。指定後不再自動偵測。

.PARAMETER Scheme
  指定 ESP32 core 的分割表名稱（例如 default、min_spiffs、minimal），
  會讀取 tools/partitions/<Scheme>.csv 的 spiffs 資料列。

.PARAMETER Mkspiffs
  mkspiffs.exe 路徑；省略時會自動搜尋 Arduino15 安裝目錄。

.PARAMETER Force
  即使輸出檔已比 data/ 新、大小也正確，仍強制重新建立映像。

.EXAMPLE
  .\tools\build_spiffs.ps1

.EXAMPLE
  .\tools\build_spiffs.ps1 -Scheme default -Force

.EXAMPLE
  .\tools\build_spiffs.ps1 -Size 0x100000 -Out build\big_spiffs.bin
#>
[CmdletBinding()]
param(
  [string]$DataDir = '',
  [string]$Out = '',
  [string]$Size = '',
  [string]$Scheme = '',
  [string]$Mkspiffs = '',
  [switch]$Force
)

$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------------------
# 小工具
# ---------------------------------------------------------------------------
function ConvertTo-ByteSize {
  param([string]$Text)
  $t = $Text.Trim()
  if ($t -match '^0[xX]([0-9a-fA-F]+)$') {
    return [Convert]::ToInt64($Matches[1], 16)
  }
  if ($t -match '^[0-9]+$') {
    return [int64]$t
  }
  throw "無法解析大小：$Text（請用 0x160000 或十進位位元組數）"
}

function Format-ByteSize {
  param([int64]$Bytes)
  if ($Bytes -ge 1MB) { return ('{0:N2} MB' -f ($Bytes / 1MB)) }
  if ($Bytes -ge 1KB) { return ('{0:N1} KB' -f ($Bytes / 1KB)) }
  return "$Bytes bytes"
}

function Get-CorePartitionsDir {
  $base = Join-Path $env:LOCALAPPDATA 'Arduino15\packages\esp32\hardware\esp32'
  if (-not (Test-Path $base)) { return $null }

  $verDir = Get-ChildItem -Path $base -Directory -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' } |
    Sort-Object { [version]$_.Name } -Descending |
    Select-Object -First 1
  if (-not $verDir) { return $null }

  $dir = Join-Path $verDir.FullName 'tools\partitions'
  if (Test-Path $dir) { return $dir }
  return $null
}

function Read-PartitionCsv {
  param([string]$Path)
  $rows = @()
  foreach ($line in Get-Content -Path $Path -Encoding UTF8) {
    $t = $line.Trim()
    if (-not $t) { continue }
    if ($t.StartsWith('#')) { continue }

    $f = @($t.Split(',') | ForEach-Object { $_.Trim() })
    if ($f.Count -lt 5) { continue }

    $rows += [pscustomobject]@{
      Name    = $f[0]
      Type    = $f[1]
      SubType = $f[2]
      Offset  = $f[3]
      Size    = $f[4]
    }
  }
  return $rows
}

function Get-SpiffsPartition {
  param([string]$Path)
  if (-not (Test-Path $Path)) { return $null }

  $row = Read-PartitionCsv -Path $Path |
    Where-Object { $_.Type -eq 'data' -and $_.SubType -eq 'spiffs' } |
    Select-Object -First 1
  if (-not $row) { return $null }

  if (-not $row.Size) {
    throw "分割表 $Path 的 spiffs 沒有固定大小，請改用 -Size 指定。"
  }

  return [pscustomobject]@{
    Offset = $row.Offset
    Size   = (ConvertTo-ByteSize $row.Size)
    Source = $Path
  }
}

# ---------------------------------------------------------------------------
# 路徑
# ---------------------------------------------------------------------------
$scriptDir = $PSScriptRoot
if (-not $scriptDir) { $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path }
$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $scriptDir '..'))
$sketchDir = Join-Path $repoRoot 'esp32_nodemcu_32s_dog'

if (-not $DataDir) { $DataDir = Join-Path $sketchDir 'data' }
if (-not $Out) { $Out = Join-Path $repoRoot 'esp32_nodemcu_32s_dog\build\spiffs.bin' }

$DataDir = [System.IO.Path]::GetFullPath($DataDir)
$Out = [System.IO.Path]::GetFullPath($Out)

# ---------------------------------------------------------------------------
# 尋找 mkspiffs.exe
# ---------------------------------------------------------------------------
if (-not $Mkspiffs) {
  $roots = @(
    (Join-Path $env:LOCALAPPDATA 'Arduino15\packages\esp32\tools\mkspiffs'),
    (Join-Path $env:USERPROFILE 'AppData\Local\Arduino15\packages\esp32\tools\mkspiffs')
  ) | Select-Object -Unique

  foreach ($root in $roots) {
    if (-not (Test-Path $root)) { continue }
    $found = Get-ChildItem -Path $root -Filter 'mkspiffs.exe' -Recurse -ErrorAction SilentlyContinue |
      Sort-Object FullName -Descending | Select-Object -First 1
    if ($found) { $Mkspiffs = $found.FullName; break }
  }
}

if (-not $Mkspiffs -or -not (Test-Path $Mkspiffs)) {
  throw '找不到 mkspiffs.exe。請先安裝 ESP32 開發板支援，或以 -Mkspiffs 指定路徑。'
}

# ---------------------------------------------------------------------------
# 決定 SPIFFS 分割區大小
# ---------------------------------------------------------------------------
$corePartitions = Get-CorePartitionsDir
$partition = $null

if ($Size) {
  $partition = [pscustomobject]@{
    Offset = '(未指定)'
    Size   = (ConvertTo-ByteSize $Size)
    Source = '命令列 -Size'
  }
} elseif ($Scheme) {
  if (-not $corePartitions) {
    throw "找不到 ESP32 core 的分割表資料夾，無法使用 -Scheme $Scheme。"
  }
  $csv = Join-Path $corePartitions ($Scheme + '.csv')
  if (-not (Test-Path $csv)) { throw "找不到分割表：$csv" }
  $partition = Get-SpiffsPartition -Path $csv
  if (-not $partition) { throw "分割表 $csv 內沒有 spiffs 資料列。" }
} else {
  # 優先使用最近一次編譯實際套用的 partitions.csv
  $buildRoot = Join-Path $sketchDir 'build'
  if (Test-Path $buildRoot) {
    $compiled = Get-ChildItem -Path $buildRoot -Directory -ErrorAction SilentlyContinue |
      ForEach-Object {
        $csv = Join-Path $_.FullName 'partitions.csv'
        if (Test-Path $csv) { Get-Item $csv }
      } |
      Sort-Object LastWriteTime -Descending |
      Select-Object -First 1

    if ($compiled) {
      $partition = Get-SpiffsPartition -Path $compiled.FullName
    }
  }

  # 尚未編譯過時，退回 ESP32 core 的 default 分割表
  if (-not $partition -and $corePartitions) {
    $defaultCsv = Join-Path $corePartitions 'default.csv'
    if (Test-Path $defaultCsv) { $partition = Get-SpiffsPartition -Path $defaultCsv }
  }

  if (-not $partition) {
    throw '無法自動偵測 SPIFFS 分割區大小。請先編譯一次，或以 -Size 0x160000 指定。'
  }
}

$sizeBytes = [int64]$partition.Size
if ($sizeBytes -le 0) { throw "SPIFFS 分割區大小不正確：$sizeBytes" }
if (($sizeBytes % 4096) -ne 0) {
  Write-Warning "SPIFFS 分割區大小 $sizeBytes 不是 4096 的倍數，映像可能無法掛載。"
}

# ---------------------------------------------------------------------------
# 檢查 data/
# ---------------------------------------------------------------------------
if (-not (Test-Path $DataDir)) { throw "找不到 data 資料夾：$DataDir" }

$dataFiles = @(Get-ChildItem -Path $DataDir -Recurse -File)
if ($dataFiles.Count -eq 0) { throw "data 資料夾內沒有任何檔案：$DataDir" }

if (-not (Test-Path (Join-Path $DataDir 'index.html'))) {
  throw "data 資料夾根目錄缺少 index.html：$DataDir"
}

$dataBytes = ($dataFiles | Measure-Object -Property Length -Sum).Sum
if (-not $dataBytes) { $dataBytes = 0 }
if ($dataBytes -ge $sizeBytes) {
  throw ("data 內容 {0} bytes 超過 SPIFFS 分割區 {1} bytes，無法建立映像。" -f $dataBytes, $sizeBytes)
}

# ---------------------------------------------------------------------------
# 已是最新就不重建（-Force 可強制）
# ---------------------------------------------------------------------------
$newestData = ($dataFiles | Sort-Object LastWriteTime -Descending | Select-Object -First 1).LastWriteTime
if ((Test-Path $Out) -and -not $Force) {
  $outItem = Get-Item $Out
  if ($outItem.Length -eq $sizeBytes -and $outItem.LastWriteTime -ge $newestData) {
    Write-Host "映像檔已是最新，略過重建：$Out" -ForegroundColor Green
    Write-Host '如需強制重建，請加上 -Force。'
    exit 0
  }
}

$outDir = Split-Path -Parent $Out
if ($outDir -and -not (Test-Path $outDir)) {
  New-Item -ItemType Directory -Path $outDir | Out-Null
}

Write-Host '=== DogBot SPIFFS 映像 ===' -ForegroundColor Cyan
Write-Host ("mkspiffs : {0}" -f $Mkspiffs)
Write-Host ("分割表   : {0}" -f $partition.Source)
Write-Host ("SPIFFS   : offset {0}, size {1} bytes ({2})" -f $partition.Offset, $sizeBytes, (Format-ByteSize $sizeBytes))
Write-Host ("data     : {0} 個檔案, {1} bytes ({2})" -f $dataFiles.Count, $dataBytes, (Format-ByteSize $dataBytes))
Write-Host ("剩餘空間 : {0} bytes" -f ($sizeBytes - $dataBytes))
Write-Host ("輸出     : {0}" -f $Out)
Write-Host ''

# ---------------------------------------------------------------------------
# 建立映像
# ---------------------------------------------------------------------------
if (Test-Path $Out) { Remove-Item $Out -Force }

$sizeArg = '0x' + $sizeBytes.ToString('X')
& $Mkspiffs -c $DataDir -b 4096 -p 256 -s $sizeArg $Out
if ($LASTEXITCODE -ne 0) { throw "mkspiffs 失敗（exit code $LASTEXITCODE）" }

$length = (Get-Item $Out).Length
if ($length -ne $sizeBytes) {
  throw ("映像大小不符：產生 {0} bytes，分割區是 {1} bytes。這會導致裝置掛載失敗並格式化分割區。" -f $length, $sizeBytes)
}

# ---------------------------------------------------------------------------
# 驗證映像內容
# ---------------------------------------------------------------------------
Write-Host ''
$listing = @(& $Mkspiffs -l $Out 2>&1)
if ($LASTEXITCODE -ne 0) { throw "無法讀取映像檔（exit code $LASTEXITCODE）" }

$listing | ForEach-Object { Write-Host "  $_" }

$required = @('/index.html', '/style.css', '/app.js')
$missing = @()
foreach ($name in $required) {
  $found = $false
  foreach ($line in $listing) {
    if ([string]$line -match ('\s' + [regex]::Escape($name) + '$')) {
      $found = $true
      break
    }
  }
  if (-not $found) { $missing += $name }
}

if ($missing.Count -gt 0) {
  throw ("映像檔缺少必要檔案：{0}。請確認 data/ 內容。" -f ($missing -join ', '))
}

Write-Host ''
Write-Host ("已建立：{0}（{1:N0} bytes，符合分割區大小）" -f $Out, $length) -ForegroundColor Green
Write-Host '可直接在網頁「OTA 更新 → SPIFFS .bin」上傳。'
