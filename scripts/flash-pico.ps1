param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('pico', 'pico2')]
    [string]$Board
)
$ErrorActionPreference = 'Stop'
$uf2 = Join-Path (Split-Path $PSScriptRoot -Parent) "build-$Board/nova_os.uf2"
if (-not (Test-Path $uf2)) { throw "Firmware not found: $uf2. Run scripts/build-all.ps1 first." }
Write-Host "Put the $Board board into BOOTSEL mode, then copy this firmware to its RPI-RP2 drive:" -ForegroundColor Cyan
Write-Host $uf2 -ForegroundColor Green
Write-Host 'This uses the board ROM UF2 recovery/install path; it is not USB-thumb-drive OS boot.'
