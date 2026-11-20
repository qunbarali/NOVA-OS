$ErrorActionPreference = 'Stop'

function Require-Command([string]$Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command '$Name' was not found on PATH. Install the Pico development tools first."
    }
}

Require-Command 'cmake'
Require-Command 'ninja'
Require-Command 'arm-none-eabi-gcc'

if (-not $env:PICO_SDK_PATH -or -not (Test-Path (Join-Path $env:PICO_SDK_PATH 'external/pico_sdk_import.cmake'))) {
    throw 'PICO_SDK_PATH is missing or does not point to a valid Pico SDK directory.'
}

$targets = @(
    @{ Name = 'pico'; Board = 'pico' },
    @{ Name = 'pico2'; Board = 'pico2' }
)

foreach ($target in $targets) {
    $buildDir = "build-$($target.Name)"
    Write-Host "`n=== Configuring NOVA for $($target.Board) ===" -ForegroundColor Cyan
    cmake -S . -B $buildDir -G Ninja `
        "-DPICO_BOARD=$($target.Board)" `
        "-DPICO_SDK_PATH=$env:PICO_SDK_PATH" `
        -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed for $($target.Board)." }

    Write-Host "=== Building NOVA for $($target.Board) ===" -ForegroundColor Cyan
    cmake --build $buildDir --parallel
    if ($LASTEXITCODE -ne 0) { throw "Build failed for $($target.Board)." }

    $uf2 = Join-Path $buildDir 'nova_os.uf2'
    if (-not (Test-Path $uf2)) { throw "Expected UF2 output was not created: $uf2" }
    Write-Host "Built: $uf2" -ForegroundColor Green
}

Write-Host "`nBoth NOVA firmware targets built successfully." -ForegroundColor Green
