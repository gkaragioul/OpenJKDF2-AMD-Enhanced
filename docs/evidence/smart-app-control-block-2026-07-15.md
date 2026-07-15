# Smart App Control Release-test block -- historical and resolved

## Historical reproduction

On 2026-07-15, the exact Release gate built every target but Windows Smart App
Control refused to start one newly linked unsigned test process:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Release -Test
```

CTest reported 38/39 because
`build/msvc-release/test_startup_options.exe` never reached `main`. Direct
execution reported `An Application Control policy has blocked this file`.

Historical evidence:

- blocked SHA-256: `BAB83F1C5889088481D5776643A2E0967CC70696078992D3C7D4DF8C60C4678A`
- file size: 17,408 bytes
- Authenticode state: unsigned
- alternate data streams: only the normal `:$DATA` stream
- Smart App Control state: `On`
- user-mode Code Integrity state: enforced
- Code Integrity event IDs: 3033 and 3077
- policy ID: `{0283ac0f-fff1-49ae-ada1-8a933130cad6}`
- the Debug counterpart passed

This was an external launch-policy block, not a failed assertion.

## Current resolution

After rebuilding the current timing-observer source, the same exact Debug and
Release commands both pass 40/40 tests. The Release
`test_startup_options.exe` now starts and returns zero, and the Release
production executable completes the seven-process 60/120 FPS timing capture.

Windows does not expose a definitive reason why cloud reputation allowed the new
binary. The evidence supports only that the historical binary was blocked and
the current rebuilt binary is allowed; it does not attribute the change to a
specific reputation decision.

## Safety decision

Smart App Control, Defender, Code Integrity, certificate stores, and all other
Windows security controls remained enabled and unchanged. No exception,
self-signed certificate, registry modification, or policy bypass was used.

Microsoft documents that Smart App Control has no per-application exception and
that an unknown unsigned application may require a trusted signature when cloud
reputation does not allow it. See the official
[Smart App Control FAQ](https://support.microsoft.com/en-US/Windows/Security/Threat-Malware-Protection/smart-app-control-frequently-asked-questions)
and
[code-signing guidance](https://learn.microsoft.com/en-us/windows/apps/develop/smart-app-control/code-signing-for-smart-app-control).
