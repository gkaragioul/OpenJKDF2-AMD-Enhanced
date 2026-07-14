# Diagnostics

AMD Enhanced diagnostics are designed to be useful without collecting personal
data. A diagnostics directory contains:

- `openjkdf2.log`: human-readable events.
- `openjkdf2.jsonl`: one JSON object per event for tooling.
- `run-state.json`: `unclean` from startup until an orderly shutdown changes it
  to `clean`.

Every structured event has a schema version, UTC timestamp, severity,
subsystem, event, and fields object. Event text is JSON-escaped and occurrences
of the configured game-data root are replaced with `<data-dir>`. Callers must
use `diag_redact_path` before adding any path rooted outside that explicitly
configured data directory. Usernames, hostnames, full command lines, saves, and
device serial numbers must never be submitted as diagnostic event content.

Run-state replacement is written through a temporary file so an interrupted
write cannot leave a partially written marker.

When `--data-dir <path>` is present, it is a read-only base-asset root and is
not translated to the legacy mod-oriented `-path` option. User-owned files use
`--user-dir <path>`, `%LOCALAPPDATA%\OpenJKDF2 AMD Enhanced` by default, or a
`UserData` directory beside the executable when `--portable` is explicit.
Relative reads prefer writable overrides before base assets; writes outside
the configured writable root are rejected by the overlay resolver.
