[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Debug',

    [switch] $Test
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot ("build/msvc-{0}" -f $Configuration.ToLowerInvariant())
$pythonPackages = Join-Path $repoRoot 'build/python-packages'
$vsDevCmd = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/2022/BuildTools/Common7/Tools/VsDevCmd.bat'

if (-not (Test-Path -LiteralPath $vsDevCmd -PathType Leaf)) {
    throw "Visual Studio 2022 Build Tools were not found at the expected path: $vsDevCmd"
}

function Invoke-DeveloperCommand {
    param([Parameter(Mandatory)][string] $Command)

    $pythonPrefix = if (Test-Path -LiteralPath $pythonPackages -PathType Container) {
        "set `"PYTHONPATH=$pythonPackages`" && "
    } else {
        ''
    }
    $commandLine = "`"$vsDevCmd`" -arch=x64 -host_arch=x64 && $pythonPrefix$Command"
    & cmd.exe /d /s /c $commandLine
    if ($LASTEXITCODE -ne 0) {
        throw "Developer command failed with exit code $LASTEXITCODE."
    }
}

$testOption = if ($Test) { 'ON' } else { 'OFF' }
$configure = 'cmake -S "{0}" -B "{1}" -G Ninja -DCMAKE_BUILD_TYPE={2} -DOPENJKDF2_BUILD_TESTS={3}' -f $repoRoot, $buildDir, $Configuration, $testOption
Invoke-DeveloperCommand $configure
Invoke-DeveloperCommand ('cmake --build "{0}" --target openjkdf2-64' -f $buildDir)

if ($Test) {
    Invoke-DeveloperCommand ('cmake --build "{0}" --target openjkdf2-unit-tests' -f $buildDir)
    Invoke-DeveloperCommand ('ctest --test-dir "{0}" -L unit --output-on-failure' -f $buildDir)
}
