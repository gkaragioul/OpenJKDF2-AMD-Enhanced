[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$evidenceDir = Join-Path $repoRoot 'build-evidence'
$outputPath = Join-Path $evidenceDir 'baseline.json'
$temporaryPath = "$outputPath.tmp"
New-Item -ItemType Directory -Force -Path $evidenceDir | Out-Null

function Get-ArtifactHash {
    param([string] $RelativePath)
    $path = Join-Path $repoRoot $RelativePath
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { return 'not_collected' }
    return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Invoke-UnitTests {
    param([string] $Configuration)
    $directory = Join-Path $repoRoot ("build/msvc-{0}" -f $Configuration.ToLowerInvariant())
    if (-not (Test-Path -LiteralPath (Join-Path $directory 'CTestTestfile.cmake'))) { return 'not_collected' }
    & ctest --test-dir $directory -L unit --output-on-failure | Out-Host
    return $(if ($LASTEXITCODE -eq 0) { 'passed' } else { 'failed' })
}

$os = Get-CimInstance Win32_OperatingSystem
$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
$gpu = Get-CimInstance Win32_VideoController | Where-Object CurrentHorizontalResolution | Select-Object -First 1
$gitCommit = (& git -C $repoRoot rev-parse HEAD).Trim()
$gitDescribe = (& git -C $repoRoot describe --always --dirty).Trim()
$debugTests = Invoke-UnitTests 'Debug'
$releaseTests = Invoke-UnitTests 'Release'
$smokeReportPath = Join-Path $repoRoot 'renderer-smoke-output/renderer-report.json'
$smokeReport = if (Test-Path -LiteralPath $smokeReportPath) {
    Get-Content -LiteralPath $smokeReportPath -Raw | ConvertFrom-Json
} else { $null }

$report = [ordered]@{
    schema = 1
    captured_utc = [DateTime]::UtcNow.ToString('o')
    source = [ordered]@{
        commit = $gitCommit
        describe = $gitDescribe
        branch = (& git -C $repoRoot branch --show-current).Trim()
    }
    operating_system = [ordered]@{
        caption = $os.Caption
        version = $os.Version
        build = $os.BuildNumber
        architecture = $os.OSArchitecture
    }
    processor = [ordered]@{
        name = $cpu.Name.Trim()
        logical_processors = $cpu.NumberOfLogicalProcessors
    }
    display_adapter = [ordered]@{
        name = $gpu.Name
        driver_version = $gpu.DriverVersion
        resolution = "{0}x{1}" -f $gpu.CurrentHorizontalResolution, $gpu.CurrentVerticalResolution
        refresh_hz = $gpu.CurrentRefreshRate
    }
    artifacts = [ordered]@{
        debug_sha256 = Get-ArtifactHash 'build/msvc-debug/openjkdf2-64.exe'
        release_sha256 = Get-ArtifactHash 'build/msvc-release/openjkdf2-64.exe'
        renderer_smoke_sha256 = Get-ArtifactHash 'build/msvc-release/openjkdf2-renderer-smoke.exe'
    }
    tests = [ordered]@{
        debug_unit = $debugTests
        release_unit = $releaseTests
        renderer_smoke = $(if ($smokeReport) { 'passed' } else { 'not_collected' })
    }
    renderer_smoke = $(if ($smokeReport) { $smokeReport } else { 'not_collected' })
    privacy = [ordered]@{
        username = 'not_collected'
        hostname = 'not_collected'
        serial_numbers = 'not_collected'
        absolute_paths = 'not_collected'
        command_line = 'not_collected'
    }
}

$report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $temporaryPath -Encoding utf8
Move-Item -LiteralPath $temporaryPath -Destination $outputPath -Force
Get-Content -LiteralPath $outputPath -Raw | ConvertFrom-Json | Out-Null
Write-Host "Baseline evidence written and validated."
