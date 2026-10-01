param(
    [string]$Python = 'python',
    [string]$VenvPath = (Join-Path $PSScriptRoot '.toolchain'),
    [string]$CacheRoot = (Join-Path $env:SystemDrive 'codex-build/artnet5-esp32'),
    [string]$CoreDirectory = '',
    [string[]]$Environments = @('esp32-wifi-flex8'),
    [switch]$FileSystem,
    [switch]$NativeTests,
    [string]$Cxx = 'g++'
)
$ErrorActionPreference = 'Stop'
$savedEnvironment = @{}
foreach ($name in @('PLATFORMIO_CORE_DIR', 'PLATFORMIO_LIBDEPS_DIR', 'PLATFORMIO_BUILD_DIR')) {
    $savedEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}
Push-Location $PSScriptRoot
try {
    # No upload target: this entry point only builds files on the host.
    $env:PLATFORMIO_CORE_DIR = if ($CoreDirectory) { $CoreDirectory } else { Join-Path $CacheRoot 'core' }
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
    if ($LASTEXITCODE) { throw 'ESP firmware compilation failed' }
    if ($FileSystem) {
        foreach ($buildEnv in $Environments) {
            if ($buildEnv -notlike 'esp32-*' -or $buildEnv -eq 'esp32-power-diagnostic') {
                throw "Filesystem target does not apply to $buildEnv"
            }
            & $py -m platformio run -e $buildEnv -t buildfs
            if ($LASTEXITCODE) { throw "$buildEnv filesystem compilation failed" }
        }
    }
    if ($NativeTests) {
        & $py tools/test_host.py --cxx $Cxx
        if ($LASTEXITCODE) { throw 'ESP host tests failed; see test report' }
    }
} finally {
    Pop-Location
    foreach ($name in $savedEnvironment.Keys) {
        [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process')
    }
}
