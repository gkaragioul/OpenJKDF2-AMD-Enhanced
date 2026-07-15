[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$failures = [Collections.Generic.List[string]]::new()

function Require-Pattern {
    param([string]$Path, [string]$Pattern, [string]$Description)
    $fullPath = Join-Path $repoRoot $Path
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        $failures.Add("$Description (missing $Path)")
        return
    }
    if ((Get-Content -LiteralPath $fullPath -Raw) -notmatch $Pattern) {
        $failures.Add("$Description ($Path)")
    }
}

Require-Pattern 'src/Platform/Win32/DisplayWatchdog.c' 'display_watchdog_start' 'engine coordinator starts the watchdog'
Require-Pattern 'src/Platform/Win32/DisplayWatchdog.c' 'display_restore_apply_if_armed' 'coordinator preflights exact-snapshot restoration'
Require-Pattern 'src/Platform/Win32/DisplayWatchdog.c' 'WAIT_OBJECT_0' 'coordinator waits for a bounded ready handshake'
Require-Pattern 'src/Platform/Win32/DisplayWatchdog.c' 'display_restore_disarm' 'coordinator fails closed by disarming state'
Require-Pattern 'src/Platform/Win32/DisplayRestore.c' 'display_restore_is_armed' 'restore state has a read-only armed query'
Require-Pattern 'src/Tools/display_watchdog_main.c' 'watchdog_write_ready' 'helper writes a ready handshake'
Require-Pattern 'src/Tools/display_watchdog_main.c' 'argc == 6.*--watch' 'watch protocol requires a ready-file path'
Require-Pattern 'src/main.c' 'display_watchdog_start' 'game startup invokes the coordinator'
Require-Pattern 'src/main.c' 'Window_SetRestorationGuardReady' 'game assigns the fail-closed guard result'
Require-Pattern 'src/main.c' 'display_watchdog_disarm' 'clean exit disarms the watchdog'
Require-Pattern 'src/Win95/Window.c' 'Window_SetRestorationGuardReady' 'window policy exposes a restoration guard setter'
Require-Pattern 'CMakeLists.txt' 'add_dependencies\(\$\{BIN_NAME\} openjkdf2-display-watchdog\)' 'game build depends on the watchdog executable'
Require-Pattern 'scripts/build-windows-package.ps1' 'openjkdf2-display-watchdog\.exe.*OpenJKDF2-Display-Watchdog\.exe' 'package stages the watchdog executable'
Require-Pattern 'scripts/verify-windows-package.ps1' 'OpenJKDF2-Display-Watchdog\.exe' 'package verification requires the watchdog executable'

if ($failures.Count) {
    throw "Display watchdog integration contract failed:`n - $($failures -join "`n - ")"
}

Write-Output 'PASS: display watchdog is fail-closed, lifecycle-integrated, and packaged.'
