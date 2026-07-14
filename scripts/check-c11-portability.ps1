[CmdletBinding()]
param(
    [string]$SourceRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $SourceRoot) {
    $SourceRoot = Join-Path $PSScriptRoot '..\src'
}

$violations = Get-ChildItem -LiteralPath $SourceRoot -Recurse -File -Include '*.c', '*.h' |
    Select-String -Pattern '\bcase\s+[^:]+\s+\.\.\.\s+[^:]+' |
    ForEach-Object {
        '{0}:{1}: GNU case range is not valid C11: {2}' -f $_.Path, $_.LineNumber, $_.Line.Trim()
    }

if ($violations) {
    $violations | Write-Error
    exit 1
}

Write-Output 'C11 portability check passed.'
