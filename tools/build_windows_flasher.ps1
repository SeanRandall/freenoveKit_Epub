$ErrorActionPreference = 'Stop'
$Repo = Split-Path -Parent $PSScriptRoot
$Release = Join-Path $Repo 'release'
$Scratch = Join-Path $env:TEMP 'evv-reader-installer-build'
$Dist = Join-Path $Scratch 'dist'
$Work = Join-Path $Scratch 'work'
$Spec = Join-Path $Scratch 'spec'
$Venv = Join-Path $Scratch 'venv'
$Python = Join-Path $Venv 'Scripts\python.exe'

New-Item -ItemType Directory -Force $Dist, $Work, $Spec | Out-Null
if (-not (Test-Path $Python)) { python -m venv $Venv }

& $Python -m pip install --disable-pip-version-check esptool pyinstaller
if ($LASTEXITCODE -ne 0) { throw "Installing build dependencies failed with exit code $LASTEXITCODE" }
& $Python -m PyInstaller --noconfirm --clean --onefile `
    --name EVV-Reader-Installer `
    --collect-data esptool `
    --distpath $Dist `
    --workpath $Work `
    --specpath $Spec `
    (Join-Path $PSScriptRoot 'windows_flasher.py')
if ($LASTEXITCODE -ne 0) { throw "PyInstaller failed with exit code $LASTEXITCODE" }

Copy-Item (Join-Path $Dist 'EVV-Reader-Installer.exe') $Release -Force
Write-Host "Built $Release\EVV-Reader-Installer.exe"
