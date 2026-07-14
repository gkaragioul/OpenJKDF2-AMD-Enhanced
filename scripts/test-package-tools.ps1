$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$modulePath = Join-Path $repoRoot "packaging\windows\PackageTools.psm1"
Import-Module $modulePath -Force

$root = Join-Path ([IO.Path]::GetTempPath()) ("openjkdf2-package-tools-" + [Guid]::NewGuid().ToString("N"))
try {
    $valid = Join-Path $root "valid"
    [void](New-Item -ItemType Directory -Path (Join-Path $valid "Episode") -Force)
    [void](New-Item -ItemType Directory -Path (Join-Path $valid "Resource") -Force)
    [IO.File]::WriteAllBytes((Join-Path $valid "Episode\JK1.GOB"), [byte[]](1, 2, 3))
    [IO.File]::WriteAllBytes((Join-Path $valid "Resource\Res1hi.gob"), [byte[]](1, 2, 3))
    [IO.File]::WriteAllBytes((Join-Path $valid "Resource\Res2.gob"), [byte[]](1, 2, 3))
    [IO.File]::WriteAllBytes((Join-Path $valid "JK.EXE"), [byte[]](1, 2, 3))

    $validation = Test-JKDataDirectory -Path $valid
    if (-not $validation.Valid -or $validation.Missing.Count -ne 0) { throw "Valid data directory was rejected" }

    Remove-Item -LiteralPath (Join-Path $valid "Resource\Res2.gob")
    $invalid = Test-JKDataDirectory -Path $valid
    if ($invalid.Valid -or $invalid.Missing -notcontains "Resource\Res2.gob") { throw "Missing asset was not reported" }
    [IO.File]::WriteAllBytes((Join-Path $valid "Resource\Res2.gob"), [byte[]](1, 2, 3))

    $found = Find-JKDataDirectory -CandidatePaths @((Join-Path $root "missing"), $valid)
    if ($found -ne [IO.Path]::GetFullPath($valid)) { throw "Candidate discovery did not return the valid root" }

    $package = Join-Path $root "package"
    [void](New-Item -ItemType Directory -Path $package)
    [IO.File]::WriteAllBytes((Join-Path $package "OpenJKDF2.exe"), [byte[]](1))
    [IO.File]::WriteAllBytes((Join-Path $package "OpenAL32.dll"), [byte[]](1))
    if ((Get-PackageProprietaryFindings -Path $package).Count -ne 0) { throw "Open-source package files were falsely flagged" }
    [IO.File]::WriteAllBytes((Join-Path $package "JK1.GOB"), [byte[]](1))
    if ((Get-PackageProprietaryFindings -Path $package).Count -ne 1) { throw "Proprietary GOB was not flagged" }

    "package tools tests passed"
} finally {
    Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
}
