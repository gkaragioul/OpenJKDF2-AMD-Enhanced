# Upstream-ready change groups

The fork keeps changes in focused commits so generally useful fixes can be
reviewed independently. No pull request is claimed until it exists in the
upstream repository.

Candidate groups for upstream submission are:

- Standards-compliant shader diagnostics and renderer resource telemetry.
- Capability-based renderer fallback selection without vendor-name branching.
- Display selection, transaction, and desktop-safe Borderless behavior.
- High-resolution pacing telemetry and fixed-step timing helpers.
- Per-user data overlay and privacy-safe structured diagnostics.
- Test contracts for resolution, aspect, frame rates, display restoration,
  controls, recovery, and package legality.

Fork-specific branding, package scripts, RX 7900 XTX evidence, and validation
hooks should remain in this branch or be separated from generally useful engine
patches before any upstream proposal. Submission requires the repository owner's
normal review and credentials and is intentionally not represented as completed
by this document.
