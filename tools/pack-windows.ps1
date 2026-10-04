# Portable Windows release for DokitLab Sunrise.
#
# Customer unpacks the folder and runs sunrise.exe.
# Layout next to exe:
#   sunrise.exe, Qt/OpenSSL DLLs, assets/, data/, key/, source.txt
#
# Usage (PowerShell):
#   cd D:\projects\DokitLab\sunrise
#   .\tools\pack-windows.ps1
#   .\tools\pack-windows.ps1 -BuildDir ".\build\Desktop_Qt_5_15_2_MinGW_64_bit-Release" -QtBin "D:\Qt\5.15.2\mingw81_64\bin"
#
# Output: dist\Sunrise-Windows-YYYYMMDD-HHMMSS\  (+ .zip)
#
# Update an existing dist folder in place (data\ and key\ are never touched):
#   .\tools\pack-windows.ps1 -OutDir ".\dist\Sunrise-Windows-20260930-162818"

param(
    [string]$BuildDir = "",
    [string]$QtBin = "",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root

function Find-BuildExe {
    param([string]$Preferred)
    $candidates = @()
    if ($Preferred) { $candidates += $Preferred }
    $candidates += @(
        (Join-Path $Root "build\Desktop_Qt_5_15_2_MinGW_64_bit-Release"),
        (Join-Path $Root "build\Desktop_Qt_5_15_2_MinGW_64_bit-Debug"),
        (Join-Path $Root "build-release"),
        (Join-Path $Root "build")
    )
    foreach ($dir in $candidates) {
        $exe = Join-Path $dir "sunrise.exe"
        if (Test-Path $exe) { return (Resolve-Path $dir).Path }
    }
    return $null
}

function Find-Windeployqt {
    param([string]$PreferredQtBin)
    if ($PreferredQtBin) {
        $w = Join-Path $PreferredQtBin "windeployqt.exe"
        if (Test-Path $w) { return $w }
    }
    $qmake = Get-Command qmake -ErrorAction SilentlyContinue
    if ($qmake) {
        $bin = Split-Path -Parent $qmake.Source
        $w = Join-Path $bin "windeployqt.exe"
        if (Test-Path $w) { return $w }
    }
    foreach ($g in @(
        "D:\Qt\5.15.2\mingw81_64\bin\windeployqt.exe",
        "C:\Qt\5.15.2\mingw81_64\bin\windeployqt.exe",
        "D:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe"
    )) {
        if (Test-Path $g) { return $g }
    }
    return $null
}

function Deploy-ByMapping {
    param(
        [string]$WindeployPath,
        [string]$ExePath,
        [string]$OutDir
    )
    $mapping = & $WindeployPath --list mapping --no-translations $ExePath 2>&1
    if ($LASTEXITCODE -ne 0 -and -not $mapping) {
        throw "windeployqt --list mapping failed"
    }
    $copied = 0
    foreach ($line in $mapping) {
        $text = "$line"
        if ($text -notmatch '^"(.+?)"\s+"(.+?)"$') { continue }
        $src = $Matches[1]
        $rel = $Matches[2]
        if ($rel -like "translations\*") { continue }
        if (-not (Test-Path -LiteralPath $src)) {
            Write-Warning "Missing source: $src"
            continue
        }
        $dest = Join-Path $OutDir $rel
        $destDir = Split-Path -Parent $dest
        if (-not (Test-Path $destDir)) {
            New-Item -ItemType Directory -Force -Path $destDir | Out-Null
        }
        Copy-Item -LiteralPath $src -Destination $dest -Force
        $copied++
    }
    if ($copied -lt 5) {
        throw "Deploy-ByMapping copied too few files ($copied)"
    }
    Write-Host "Deployed $copied files via windeployqt mapping"
}

$BuildPath = Find-BuildExe -Preferred $BuildDir
if (-not $BuildPath) {
    Write-Error "sunrise.exe not found. Build the project first (Release recommended)."
}
$ExeSrc = Join-Path $BuildPath "sunrise.exe"

$AssetsSrc = Join-Path $Root "assets"
if (-not (Test-Path $AssetsSrc)) {
    $AssetsSrc = Join-Path $BuildPath "assets"
}
if (-not (Test-Path $AssetsSrc)) {
    Write-Error "assets folder not found in project root or near build."
}

$Windeploy = Find-Windeployqt -PreferredQtBin $QtBin
if (-not $Windeploy) {
    Write-Error "windeployqt.exe not found. Pass -QtBin path to Qt bin (e.g. D:\Qt\5.15.2\mingw81_64\bin)."
}

$qtBinDir = Split-Path -Parent $Windeploy
$qtRoot = Split-Path -Parent $qtBinDir
$env:QTDIR = $qtRoot
$env:PATH = ($qtBinDir + ";" + (Join-Path $qtRoot "..\..\Tools\mingw810_64\bin") + ";" + $env:PATH)

$DistRoot = Join-Path $Root "dist"
$InPlace = [bool]$OutDir
if ($InPlace) {
    if (-not (Test-Path $OutDir)) {
        Write-Error "OutDir not found: $OutDir"
    }
    $OutDir = (Resolve-Path $OutDir).Path
    $DistRoot = Split-Path -Parent $OutDir
    $stamp = (Split-Path -Leaf $OutDir) -replace '^Sunrise-Windows-', ''
    $running = @(Get-Process sunrise -ErrorAction SilentlyContinue |
        Where-Object { $_.Path -and $_.Path.StartsWith($OutDir, [StringComparison]::OrdinalIgnoreCase) })
    if ($running.Count -gt 0) {
        Write-Error "sunrise.exe from $OutDir is running. Close it and run the script again."
    }
} else {
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $OutDir = Join-Path $DistRoot "Sunrise-Windows-$stamp"
    New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
}

Write-Host "Copying sunrise.exe ..."
Copy-Item $ExeSrc (Join-Path $OutDir "sunrise.exe") -Force

Write-Host "Copying assets ..."
$assetsDest = Join-Path $OutDir "assets"
# User-edited clinical templates (htmls\interface\*.txt) survive a repack.
$templatesDir = Join-Path $assetsDest "htmls\interface"
$templatesBackup = $null
if (Test-Path $templatesDir) {
    $userTemplates = @(Get-ChildItem -LiteralPath $templatesDir -Filter "*.txt" -File -ErrorAction SilentlyContinue)
    if ($userTemplates.Count -gt 0) {
        $templatesBackup = Join-Path ([IO.Path]::GetTempPath()) ("sunrise-templates-" + [Guid]::NewGuid().ToString("N"))
        New-Item -ItemType Directory -Force -Path $templatesBackup | Out-Null
        $userTemplates | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $templatesBackup -Force }
    }
}
if (Test-Path $assetsDest) {
    Remove-Item -Recurse -Force $assetsDest
}
Copy-Item $AssetsSrc $assetsDest -Recurse -Force
if ($templatesBackup) {
    New-Item -ItemType Directory -Force -Path $templatesDir | Out-Null
    Get-ChildItem -LiteralPath $templatesBackup -File |
        ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $templatesDir -Force }
    Remove-Item -Recurse -Force $templatesBackup -ErrorAction SilentlyContinue
}
$nested = Join-Path $OutDir "assets\assets"
if (Test-Path $nested) {
    Remove-Item -Recurse -Force $nested
}
Get-ChildItem (Join-Path $OutDir "assets") -Recurse -Directory -Filter "_backup_*" -ErrorAction SilentlyContinue |
    ForEach-Object { Remove-Item -Recurse -Force $_.FullName }

foreach ($f in @("source.txt", "updates.txt")) {
    $p = Join-Path $AssetsSrc $f
    if (Test-Path $p) { Copy-Item $p (Join-Path $OutDir $f) -Force }
}
$journalSrc = Join-Path $AssetsSrc "aJournal.rtf"
$journalDest = Join-Path $OutDir "aJournal.rtf"
if ((Test-Path $journalSrc) -and -not (Test-Path $journalDest)) {
    Copy-Item $journalSrc $journalDest
}

Write-Host "Creating data/ and key/ ..."
New-Item -ItemType Directory -Force -Path (Join-Path $OutDir "data") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $OutDir "key") | Out-Null

$sslNames = @("libcrypto-1_1-x64.dll", "libssl-1_1-x64.dll")
foreach ($dll in $sslNames) {
    $fromBuild = Join-Path $BuildPath $dll
    if (Test-Path $fromBuild) {
        Copy-Item $fromBuild $OutDir -Force
    }
}
$missingSsl = @($sslNames | Where-Object { -not (Test-Path (Join-Path $OutDir $_)) })
if ($missingSsl.Count -gt 0) {
    foreach ($root in @(
        "D:\Qt\Tools\mingw1120_64\opt\bin",
        "D:\Qt\Tools\mingw1310_64\opt\bin",
        "D:\Qt\Tools\mingw810_64\opt\bin",
        "C:\Qt\Tools\mingw1120_64\opt\bin"
    )) {
        foreach ($dll in $missingSsl) {
            $p = Join-Path $root $dll
            if (Test-Path $p) { Copy-Item $p $OutDir -Force }
        }
        $missingSsl = @($sslNames | Where-Object { -not (Test-Path (Join-Path $OutDir $_)) })
        if ($missingSsl.Count -eq 0) { break }
    }
}
if ($missingSsl.Count -gt 0) {
    Write-Warning ("OpenSSL DLLs missing: {0}. HTTPS/license activation will fail." -f ($missingSsl -join ", "))
}

Write-Host "Deploying Qt runtime ..."
$exePath = Join-Path $OutDir "sunrise.exe"
$deployOk = $false
try {
    $deployArgs = @("--no-translations", "--no-system-d3d-compiler", "--compiler-runtime", $exePath)
    if ($BuildPath -match "Release") { $deployArgs = @("--release") + $deployArgs }
    elseif ($BuildPath -match "Debug") { $deployArgs = @("--debug") + $deployArgs }
    & $Windeploy @deployArgs
    if ($LASTEXITCODE -eq 0 -and (Test-Path (Join-Path $OutDir "platforms\qwindows.dll"))) {
        $deployOk = $true
        Write-Host "windeployqt OK"
    }
} catch {
    Write-Warning $_.Exception.Message
}

if (-not $deployOk) {
    Write-Warning "windeployqt copy failed — falling back to --list mapping"
    Deploy-ByMapping -WindeployPath $Windeploy -ExePath $exePath -OutDir $OutDir
}

$mingwBins = @(
    "D:\Qt\Tools\mingw810_64\bin",
    "D:\Qt\Tools\mingw1120_64\bin",
    "D:\Qt\Tools\mingw1310_64\bin",
    "C:\Qt\Tools\mingw810_64\bin"
) | Where-Object { Test-Path $_ }
foreach ($dll in @("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")) {
    if (Test-Path (Join-Path $OutDir $dll)) { continue }
    foreach ($bin in $mingwBins) {
        $p = Join-Path $bin $dll
        if (Test-Path $p) {
            Copy-Item $p $OutDir -Force
            break
        }
    }
}

@"
[Paths]
Prefix=.
Plugins=.
Libraries=.
"@ | Set-Content -Path (Join-Path $OutDir "qt.conf") -Encoding ASCII

$Readme = @"
DokitLab Санрайс — Windows

Запуск: sunrise.exe

Рядом с программой:
  assets/      — ресурсы программы (изображения, html, справка)
  data/        — база SQLite (base.db)
  key/         — файл лицензии (license.json)
  source.txt   — выбор сервера: 1 = https://dokitlab.ru/sun
  aJournal.rtf — журнал

Не устанавливайте в Program Files без прав записи —
папки data/ и key/ должны быть доступны пользователю на запись.
"@
Set-Content -Path (Join-Path $OutDir "README.txt") -Value $Readme -Encoding UTF8

$required = @(
    "sunrise.exe",
    "Qt5Core.dll",
    "Qt5Widgets.dll",
    "Qt5Network.dll",
    "Qt5Sql.dll",
    "platforms\qwindows.dll",
    "sqldrivers\qsqlite.dll",
    "assets\sysImages",
    "assets\htmls",
    "source.txt",
    "data",
    "key",
    "qt.conf"
)
$bad = @()
foreach ($r in $required) {
    if (-not (Test-Path (Join-Path $OutDir $r))) { $bad += $r }
}
if ($bad.Count -gt 0) {
    Write-Error ("Dist incomplete, missing: {0}" -f ($bad -join ", "))
}

$zipPath = Join-Path $DistRoot ("Sunrise-Windows-{0}.zip" -f $stamp)
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Write-Host "Creating zip ..."
$zipStage = Join-Path ([IO.Path]::GetTempPath()) ("sunrise-zip-" + [Guid]::NewGuid().ToString("N"))
$zipStageDir = Join-Path $zipStage (Split-Path -Leaf $OutDir)
New-Item -ItemType Directory -Force -Path $zipStageDir | Out-Null
try {
    Get-ChildItem -LiteralPath $OutDir -Force | Where-Object { $_.Name -notin @("data", "key") } |
        ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $zipStageDir -Recurse -Force }
    New-Item -ItemType Directory -Force -Path (Join-Path $zipStageDir "data") | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $zipStageDir "key") | Out-Null
    Compress-Archive -Path $zipStageDir -DestinationPath $zipPath -Force
} finally {
    Remove-Item -Recurse -Force $zipStage -ErrorAction SilentlyContinue
}

$sizeMb = [math]::Round(((Get-ChildItem $OutDir -Recurse -File | Measure-Object Length -Sum).Sum) / 1MB, 1)
Write-Host ""
Write-Host "OK: $OutDir  ($sizeMb MB)"
Write-Host "ZIP: $zipPath"
