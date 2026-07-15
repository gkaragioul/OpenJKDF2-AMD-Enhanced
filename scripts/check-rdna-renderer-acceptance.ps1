[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$failures = [System.Collections.Generic.List[string]]::new()

function Require-RawPattern([string]$Path, [string]$Pattern, [string]$Description) {
    $full = Join-Path $root $Path
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
        $failures.Add("missing file: $Path")
        return
    }
    if ((Get-Content -Raw -LiteralPath $full) -notmatch $Pattern) {
        $failures.Add("$Path missing $Description")
    }
}

$expectedStages = @(
    'blur_f.glsl','blur_v.glsl','default_f.glsl','default_v.glsl',
    'menu_f.glsl','menu_v.glsl','ssao_f.glsl','ssao_v.glsl',
    'ssao_mix_f.glsl','ssao_mix_v.glsl','texfbo_f.glsl','texfbo_v.glsl',
    'ui_f.glsl','ui_v.glsl'
)
$shaderRoot = Join-Path $root 'resource/shaders'
$actualStages = @(Get-ChildItem -LiteralPath $shaderRoot -Filter '*.glsl' -File |
    Select-Object -ExpandProperty Name | Sort-Object)
if (@(Compare-Object $expectedStages $actualStages).Count -ne 0) {
    $failures.Add('resource/shaders does not contain the exact audited fourteen-stage manifest')
}

$std3DPath = Join-Path $root 'src/Platform/GL/std3D.c'
$std3D = Get-Content -Raw -LiteralPath $std3DPath
$programMatches = [regex]::Matches($std3D, 'std3D_(?:loadProgram|loadSimpleTexProgram)\("shaders/([^"\)]+)"')
$actualPrograms = @($programMatches | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
$expectedPrograms = @('blur','default','menu','ssao','ssao_mix','texfbo','ui')
if (@(Compare-Object $expectedPrograms $actualPrograms).Count -ne 0) {
    $failures.Add('std3D.c does not load the exact seven audited shader programs')
}

Require-RawPattern 'src/Platform/GL/ShaderCompile.c' 'glGetShaderiv\(result\.object,\s*GL_COMPILE_STATUS' 'shader compile-status check'
Require-RawPattern 'src/Platform/GL/ShaderCompile.c' 'glGetProgramiv\(result\.object,\s*GL_LINK_STATUS' 'program link-status check'
Require-RawPattern 'src/Platform/GL/ShaderCompile.c' 'glGetShaderInfoLog' 'shader compiler-log capture'
Require-RawPattern 'src/Platform/GL/ShaderCompile.c' 'glGetProgramInfoLog' 'program linker-log capture'
Require-RawPattern 'src/Platform/GL/ShaderCompile.c' '#version 330' 'GLSL 3.30 source preamble'
Require-RawPattern 'resource/shaders/default_f.glsl' '(?s)#ifdef GL_ARB_texture_gather.*?textureGather\(.*?#else.*?texelFetch\(' 'extension-gated texture-gather fallback'
Require-RawPattern 'src/Platform/GL/std3D.c' 'glCheckFramebufferStatus\(GL_FRAMEBUFFER\)' 'framebuffer completeness query'
Require-RawPattern 'src/Platform/GL/std3D.c' 'GL_FRAMEBUFFER_COMPLETE' 'framebuffer completeness decision'
Require-RawPattern 'src/Platform/GL/std3D.c' 'GL_(?:R8|RGB8|RGBA8|RGBA16F)' 'sized internal texture formats'
Require-RawPattern 'src/Platform/GL/std3D.c' '(?s)if \(GLEW_EXT_texture_filter_anisotropic\).*?GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT' 'extension-gated anisotropy capability query'
Require-RawPattern 'src/Win95/Window.c' '(?s)Window_contextFallbackTier\s*=\s*0.*?SDL_GL_CONTEXT_MAJOR_VERSION,\s*3.*?SDL_GL_CONTEXT_MINOR_VERSION,\s*3.*?Window_contextFallbackTier\s*=\s*1' 'successful-context fallback tier selection'
Require-RawPattern 'src/Platform/GL/std3D.c' 'resource_initialization_failed fallback=compatibility_required' 'resource failure compatibility-fallback telemetry'

$rendererSources = $std3D + "`n" +
    (Get-Content -Raw -LiteralPath (Join-Path $root 'src/Platform/GL/ShaderCompile.c')) + "`n" +
    (Get-Content -Raw -LiteralPath (Join-Path $root 'src/Win95/Window.c'))
if ($rendererSources -match '(?im)^\s*(?:if|else\s+if|switch)\b[^\r\n]*\b(?:AMD|ATI|Radeon|NVIDIA|Intel)\b|\b(?:AMD|ATI|Radeon|NVIDIA|Intel)\b[^\r\n]*\b(?:if|switch)\s*\(') {
    $failures.Add('renderer selection contains a prohibited GPU-vendor-name conditional')
}

if ($failures.Count) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Host 'PASS: renderer source matches the fourteen-stage, seven-program standards and capability contract.'
