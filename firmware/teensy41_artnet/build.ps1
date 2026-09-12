param([string]$Python = 'python', [switch]$NativeTests)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    # The project path is too long for some GNU toolchain archive members on Windows.
    # Dedicated, short cache; never share this directory with the ESP build.
    $env:PLATFORMIO_CORE_DIR = 'C:/codex-tools/teensy41-pio'
    $env:PLATFORMIO_LIBDEPS_DIR = 'C:/codex-tools/teensy41-libs'
    $env:PLATFORMIO_BUILD_DIR = 'C:/codex-tools/teensy41-build'
    if (-not (Test-Path -LiteralPath '.toolchain/Scripts/python.exe')) {
        & $Python -m venv .toolchain
        if ($LASTEXITCODE) { throw 'venv failed' }
    }
    $py = Join-Path $PSScriptRoot '.toolchain/Scripts/python.exe'
    & $py -m pip install -r requirements-build.txt
    if ($LASTEXITCODE) { throw 'Toolchain installation failed' }
    & $py -m platformio run -j 1 -e teensy41_safe -e teensy41_driver_compile
    if ($LASTEXITCODE) { throw 'Firmware compilation failed' }
    if ($NativeTests) {
        & $py -m ziglang c++ -nostdlib++ -std=c++17 -O2 -Iinclude test/native/receiver_tests.cpp -o test/native/receiver_tests.exe
        if ($LASTEXITCODE) { throw 'Native test compilation failed' }
        & './test/native/receiver_tests.exe'
        if ($LASTEXITCODE) { throw 'Native tests failed' }
    }
} finally { Pop-Location }
