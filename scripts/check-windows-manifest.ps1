[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$manifestPath = Join-Path $repoRoot 'packaging/win32/openjkdf2.manifest.in'
[xml] $manifest = Get-Content -Raw -LiteralPath $manifestPath

$manager = [System.Xml.XmlNamespaceManager]::new($manifest.NameTable)
$manager.AddNamespace('asmv1', 'urn:schemas-microsoft-com:asm.v1')
$identity = $manifest.SelectSingleNode(
    '/asmv1:assembly/asmv1:dependency/asmv1:dependentAssembly/asmv1:assemblyIdentity[@name="Microsoft.Windows.Common-Controls"]',
    $manager
)

if (-not $identity) {
    throw 'Windows manifest must request Microsoft.Windows.Common-Controls.'
}
if ($identity.version -ne '6.0.0.0') {
    throw "Common Controls manifest version must be 6.0.0.0, found '$($identity.version)'."
}
if ($identity.publicKeyToken -ne '6595b64144ccf1df') {
    throw 'Common Controls manifest public key token is incorrect.'
}

Write-Output 'Windows manifest requests Microsoft.Windows.Common-Controls 6.0.0.0.'
