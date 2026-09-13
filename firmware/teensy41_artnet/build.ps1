param(
    [string]$Python = 'python',
    [string]$VenvPath = (Join-Path $PSScriptRoot '.toolchain'),
    [string]$CacheRoot = (Join-Path $env:SystemDrive 'codex-build/artnet5-rx32'),
    [string[]]$Environments = @('teensy41_safe', 'teensy41_octo_web_rx32'),
    [switch]$NativeTests,
    [string]$Cxx = 'g++'
)
$ErrorActionPreference = 'Stop'
$savedEnvironment = @{}
foreach ($name in @('PLATFORMIO_CORE_DIR','PLATFORMIO_LIBDEPS_DIR','PLATFORMIO_BUILD_DIR','FASTLED_TEST_ENV')) {
    $savedEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}
Push-Location $PSScriptRoot
try {
    # Short, project-specific paths avoid Windows archive path limits. No ESP cache.
    $env:PLATFORMIO_CORE_DIR = Join-Path $CacheRoot 'core'
    $env:PLATFORMIO_LIBDEPS_DIR = Join-Path $CacheRoot 'libs'
    $env:PLATFORMIO_BUILD_DIR = Join-Path $CacheRoot 'build'
    $py = Join-Path $VenvPath 'Scripts/python.exe'
    if (-not (Test-Path -LiteralPath $py)) {
        & $Python -m venv $VenvPath
        if ($LASTEXITCODE) { throw 'venv failed' }
    }
    & $py -m pip install -r requirements-build.txt
    if ($LASTEXITCODE) { throw 'Toolchain installation failed' }
    $pioArgs = @('-m', 'platformio', 'run', '-j', '4')
    foreach ($buildEnv in $Environments) { $pioArgs += @('-e', $buildEnv) }
    & $py @pioArgs
    if ($LASTEXITCODE) { throw 'Firmware compilation failed' }
    if ($NativeTests) {
        & $Cxx -std=c++17 -O2 -Wall -Wextra -Werror -Iinclude test/native/receiver_tests.cpp -o test/native/receiver_tests.exe
        if ($LASTEXITCODE) { throw 'Native test compilation failed' }
        & './test/native/receiver_tests.exe'
        if ($LASTEXITCODE) { throw 'Native tests failed' }
        & $Cxx -std=c++17 -DOCTO_ESP_PROFILE -Wall -Wextra -Werror -pedantic test/native/octo_tests.cpp -o test/native/octo_tests.exe
        if ($LASTEXITCODE) { throw 'Octo test compilation failed' }
        & './test/native/octo_tests.exe'
        if ($LASTEXITCODE) { throw 'Octo tests failed' }
        foreach ($native in @('http_request_tests', 'runtime_receiver_tests', 'web_config_tests', 'artnet_run_policy_tests', 'esp_test_pattern_tests')) {
            & $Cxx -std=c++17 -Wall -Wextra -Werror -pedantic "test/native/$native.cpp" -o "test/native/$native.exe"
            if ($LASTEXITCODE) { throw "$native compilation failed" }
            & "./test/native/$native.exe"
            if ($LASTEXITCODE) { throw "$native failed" }
        }
        if ($Environments -contains 'teensy41_octo_web_rx32') {
            $jsonInclude = Join-Path $env:PLATFORMIO_LIBDEPS_DIR 'teensy41_octo_web_rx32/ArduinoJson/src'
            & $Cxx -std=c++17 -Wall -Wextra -Werror -pedantic "-I$jsonInclude" test/native/web_config_json_tests.cpp -o test/native/web_config_json_tests.exe
            if ($LASTEXITCODE) { throw 'JSON tests compilation failed' }
            & './test/native/web_config_json_tests.exe'
            if ($LASTEXITCODE) { throw 'JSON tests failed' }
        }
        & $py -m unittest discover -s test -p 'test_qnethernet_patch.py' -v
        if ($LASTEXITCODE) { throw 'RX32 patch tests failed' }
        if ($Environments -contains 'teensy41_octo_identify_rx32' -or $Environments -contains 'teensy41_octo_web_rx32') {
            $env:FASTLED_TEST_ENV = if ($Environments -contains 'teensy41_octo_web_rx32') { 'teensy41_octo_web_rx32' } else { 'teensy41_octo_identify_rx32' }
            & $py -m unittest discover -s test -p 'test_vendor_patch.py' -v
            if ($LASTEXITCODE) { throw 'FastLED vendor guard tests failed' }
        }
    }
} finally {
    Pop-Location
    foreach ($name in $savedEnvironment.Keys) {
        [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process')
    }
}
