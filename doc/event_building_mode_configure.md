# EventBuildingMode Configure — Phase Map

## Overview

The `configureEventBuildingMode()` sequence for CFO and DTC front-ends uses a multi-iteration,
multi-sub-step protocol to bring up the timing chain in a controlled order. Each iteration
corresponds to a phase; sub-iterations handle steps within a phase that must each complete in <5s.

Key design constraints:

- **Dual DTC instances**: The same physical DTC can be controlled by two FE instances — one in
  the CFO subsystem (no real ROCs / emulated-only) and one in the detector subsystem (with real
  ROCs). Iteration ordering avoids duplicating work (e.g. JA lock at iteration 0 is visible to
  the instance at iteration 1).
- **DCS ownership**: Only one DTC instance can control DCS (ROC communication via DMA). This is
  the instance with real ROCs. The CFO-subsystem DTC must never call `EnableDCSReception()`.
- **Sub-step budget**: Each sub-step must complete in <5s to avoid SOAP/xoap timeouts.

## Phase Map

| Iteration | Phase | CFO | DTC (no real ROCs) | DTC (real ROCs) |
|---|---|---|---|---|
| 0 (sub-iters) | **1a — Establish Clocks** | halt, disable beam modes, SoftReset, disable all outputs, JA setup | SoftReset, selective disable, passthrough, JA setup | idle |
| 1 (sub-iters) | **1b — Establish Clocks** | idle | idle | SoftReset, selective disable, passthrough, JA setup (skip JA reset if already locked) |
| 2 (sub-iters) | **2 — Establish CFO Timing Chain** | Enable all links + clock markers, check DTC CFO CDR lock via FE Macro, SERDES reset + retry if needed | Self-check CFO CDR lock, throw if not locked | Self-check CFO CDR lock, throw if not locked |
| 3 (sub-iters) | **3 — Establish Timing Chain Sync** | *TBD* | *TBD* | *TBD* |
| 4 (sub-iters) | **4 — Establish Local ROC Config** | idle | idle | ROC emulator mask, enable ROC links, DCS setup |
| 5 | **Final SoftReset** | SoftReset | SoftReset | SoftReset |
| 6 | **5 — Enable CFO Idle Operation** | ReleaseAllBuffers, EnableEmbeddedClockMarker, EnableAcceleratorRF0, SetPunchEnable, EnableLink(ALL) | configureCommon, EnableLink(EVB), set CFOEventModeRequiredMask | configureCommon, EnableLink(EVB), set CFOEventModeRequiredMask |

Constants in `CFOandDTCCoreVInterface.h`:

- `CONFIG_PHASE_ESTABLISH_CLOCKS_A` = 0
- `CONFIG_PHASE_ESTABLISH_CLOCKS_B` = 1
- `CONFIG_PHASE_ESTABLISH_TIMING_CHAIN` = 2
- `CONFIG_PHASE_ESTABLISH_TIMING_SYNC` = 3
- `CONFIG_PHASE_ESTABLISH_ROC_CONFIG` = 4
- `CONFIG_PHASE_FINAL_SOFT_RESET` = 5
- `CONFIG_CFO_EVENT_SENDING_START_ITERATION` = 6

`RUN_START_READY_FOR_TRIGGERS_ITERATION` = 7 (in `RunControlIterationConstants.h`).

## Phase 1a/1b — Establish Clocks

A DTC with no real ROCs (`(roc_mask_ & ~roc_emulated_mask_) == 0`) runs at iteration 0 (Phase 1a).
A DTC with at least one real ROC runs at iteration 1 (Phase 1b). The CFO runs at iteration 0.

### CFO sub-steps (iteration 0)

0. `halt()`, `DisableBeamOnMode(ALL)`, `DisableBeamOffMode(ALL)`, `SoftReset()`, `ClearControlRegister()`, `DisableAllOutputs()`
1. JA setup: if JA unlocked → full reset (`alsoResetJA=true`); if locked → mux-only select
2+. JA lock polling (1s intervals, ~10 attempts)

### DTC sub-steps (iteration 0 or 1)

0. `SoftReset()`, `ClearControlRegister(keepMask)`, `DisableLink(EVB)`, disable configured ROC links, `DisableCFOLoopback()` — CFO link is never disabled
1. JA setup (same lock-aware pattern)
2+. JA lock polling

## Phase 2 — Establish CFO Timing Chain

### CFO sub-steps (iteration 2)

0. `EnableLink(CFO_Link_ALL)`, `EnableEmbeddedClockMarker()`
1. Enumerate DTCs via config tree, call "Get Link Lock Status" FE Macro on each. Parse output for `"CFO CDR Lock"` line with `"[x]"`. If any DTC unlocked → `ResetSERDES()` (upstream + per-link), retry.
2. Retry CDR lock check. If still unlocked → throw exception.

### DTC (iteration 2)

Each DTC independently verifies `ReadSERDESRXCDRLock(DTC_Link_CFO)`. If not locked, throws exception.

## Phase 3 — Establish Timing Chain Sync

TBD.

## Phase 4 — Establish Local ROC Config

Only DTCs with real ROCs act. CFO and no-ROC DTCs idle.

## Final SoftReset (iteration 5)

Both CFO and all DTCs: `SoftReset()` to clear accumulated errors.

## Phase 5 — Enable CFO Idle Operation (iteration 6)

CFO enables clock markers, RF0, punch, and all links. DTCs enable EVB link and set event mode mask.
