# EventBuildingMode Configure — Phase Map

## Overview

The `configureEventBuildingMode()` sequence for CFO and DTC front-ends uses a multi-iteration,
multi-sub-step protocol to bring up the timing chain in a controlled order. Each iteration
corresponds to a phase; sub-iterations handle steps within a phase that must each complete in <5s.

Two operating modes use this sequence:

- **EventBuildingMode** — skips sync phases (iterations 4–7 are idle for all devices).
- **EventBuildingAndSyncMode** — assumes the RTF 40 MHz sync clock is present and runs the
  full sync sequence (edge fix, RTF Marker Offset, error verification).

Both modes use the same iteration count (12 iterations, 0–11).

Key design constraints:

- **Dual DTC instances**: The same physical DTC can be controlled by two FE instances — one in
  the CFO subsystem (no real ROCs / emulated-only) and one in the detector subsystem (with real
  ROCs). Iteration ordering avoids duplicating work (e.g. JA lock at iteration 0 is visible to
  the instance at iteration 1).
- **DCS ownership**: Only one DTC instance can control DCS (ROC communication via DMA). This is
  the instance with real ROCs. The CFO-subsystem DTC must never call `EnableDCSReception()`.
- **No-ROC DTCs** skip ROC/EVB setup entirely — they only participate in clock/sync phases
  (iterations 0–7) and Final SoftReset (iteration 10).
- **Sub-step budget**: Each sub-step must complete in <5s to avoid SOAP/xoap timeouts.

## Phase Map

| Iter | Phase | CFO | DTC (no real ROCs) | DTC (real ROCs) |
|---|---|---|---|---|
| 0 | **1a — Establish Clocks** | `halt()`, `DisableBeamOnMode(ALL)`, `DisableBeamOffMode(ALL)`, `SoftReset()`, `ClearControlRegister()`, `DisableAllOutputs()`, JA setup + lock poll | `SoftReset()`, `ClearControlRegister(keepMask)`, `DisableCFOLoopback()`, JA setup + lock poll | idle |
| 1 | **1b — Establish Clocks** | idle | idle | `SoftReset()`, `ClearControlRegister(keepMask)`, `DisableLink(EVB)`, disable ROC links, `DisableCFOLoopback()`, JA setup + lock poll |
| 2 | **2a — Timing Chain: Enable** | `EnableLink(CFO_Link_ALL)`, `EnableEmbeddedClockMarker()` | idle | idle |
| 3 | **2b — Timing Chain: CDR Check** | Enumerate DTCs, call "Get Link Lock Status" FE Macro on each; if unlocked -> `ResetSERDES()` + retry; throw on 2nd failure | `ReadSERDESRXCDRLock(DTC_Link_CFO)` — throw if not locked | `ReadSERDESRXCDRLock(DTC_Link_CFO)` — throw if not locked |
| 4 | **3a — Sync: CFO check + edge fix (no-ROC)** | Call "Get RTF Interface Status" FE Macro on all DTCs; validate CFO Emulation Mode OFF, JA Source RJ45, Saturated YES, CFO CDR Lock LOCKED | Clear JA counters + SoftReset, wait markers > 1000, check edge errors (txMarkers/rxToTx/parity/batchSlip) — toggle edge + SoftReset if needed, retry once; then verify JA counters stayed 0 | idle |
| 5 | **3b — Sync: edge fix (ROC DTCs)** | idle | idle | Same edge fix sub-steps as 3a |
| 6 | **3c — Sync: RTF offset + verify (no-ROC)** | idle | Clear JA counters, apply RTF offset + `SoftReset()`, wait markers > 1000, verify error flags + RTF counters; if RTFPhase-only, toggle RTF punched clock edge (bit 7) + `SoftReset()` and re-verify once; then verify JA counters stayed 0 | idle |
| 7 | **3d — Sync: RTF offset + verify (ROC)** | idle | idle | Same offset + verify sub-steps as 3c |
| 8 | **4 — ROC and DCS Setup** | idle | idle | `SetupROCs()` per link, `EnableDCSReception()`, CRV `SetPunchEnable()`, `SoftReset()`, ROC DCS-based configure |
| 9 | **5 — ROC Data Path Setup** | idle | idle | `DisableLink(EVB)`, `SetEVBInfo()`, DRP mode, `EnableLink(EVB)`, `SetCFOEventModeRequiredMask()` |
| 10 | **Final SoftReset** | `SoftReset()` | `SoftReset()` | `SoftReset()` |
| 11 | **6 — Enable CFO Operation** | `EnableAcceleratorRF0()`, `SetPunchEnable()` | idle | idle |

In **EventBuildingMode** (no sync), iterations 4–7 are idle for all devices (CFO and all DTCs
log "Sync phase idle" and advance).

Constants in `CFOandDTCCoreVInterface.h`:

- `CONFIG_PHASE_ESTABLISH_CLOCKS_A` = 0
- `CONFIG_PHASE_ESTABLISH_CLOCKS_B` = 1
- `CONFIG_PHASE_ESTABLISH_TIMING_CHAIN_ENABLE` = 2
- `CONFIG_PHASE_ESTABLISH_TIMING_CHAIN_CHECK` = 3
- `CONFIG_PHASE_ESTABLISH_SYNC_A` = 4
- `CONFIG_PHASE_ESTABLISH_SYNC_B` = 5
- `CONFIG_PHASE_ESTABLISH_SYNC_C` = 6
- `CONFIG_PHASE_ESTABLISH_SYNC_D` = 7
- `CONFIG_PHASE_ESTABLISH_ROC_CONFIG` = 8
- `CONFIG_PHASE_ROC_DATA_PATH` = 9
- `CONFIG_PHASE_FINAL_SOFT_RESET` = 10
- `CONFIG_CFO_EVENT_SENDING_START_ITERATION` = 11

`RUN_START_READY_FOR_TRIGGERS_ITERATION` = 12 (in `RunControlIterationConstants.h`).

Operating mode constants in `CFOandDTCCoreVInterface.h`:

- `CONFIG_MODE_EVENT_BUILDING` = `"EventBuildingMode"`
- `CONFIG_MODE_EVENT_BUILDING_AND_SYNC` = `"EventBuildingAndSyncMode"`

## How a DTC knows its type

A DTC is classified as "with real ROCs" (`has_real_roc_flow_ = true`) if **any** of these
conditions is met (checked in order during DTC instantiation):

1. At least one enabled, non-emulated ROC: `(roc_mask_ & ~roc_emulated_mask_) != 0`
2. Any enabled ROC has `ROCTypeLinkTable` connected (not `NO_LINK`)
3. Any enabled ROC has `LinkToSlowControlsChannelTable` connected (not `NO_LINK`)
4. The DTC's `EnableROCConfigureStep` configuration parameter is `true`

Rules 2–4 allow exercising the "real ROC" configuration flow (Phases 1b, 3b, 3d, 4, 5) with
emulated-only ROCs — useful for testing subsystem configure sequences without physical hardware.

The classification reason is stored in `real_roc_flow_reason_` and included in error messages
so operators can see why a DTC took a particular phase path.

## Phase 1a/1b — Establish Clocks

### CFO sub-steps (iteration 0)

0. `next_starting_event_window_tag_ = 0`, `halt()`, `DisableBeamOnMode(ALL)`,
   `DisableBeamOffMode(ALL)`, `SoftReset()`, `ClearControlRegister()`, `DisableAllOutputs()`
1. JA setup: if JA unlocked -> full reset; if locked -> mux-only select
2+. JA lock polling (1s intervals, ~10 attempts)

### DTC sub-steps (iteration 0 for no-ROC, iteration 1 for ROC DTCs)

0. `SoftReset()`, `ClearControlRegister(keepMask)`, `DisableCFOLoopback()`.
   ROC DTCs also `DisableLink(EVB)` and disable configured ROC links. No-ROC DTCs skip these.
1. JA setup (same lock-aware pattern as CFO)
2+. JA lock polling

## Phase 2a — Timing Chain: Enable (iteration 2)

### CFO

`EnableLink(CFO_Link_ALL)`, `EnableEmbeddedClockMarker()`.

### DTC

Idle.

## Phase 2b — Timing Chain: CDR Check (iteration 3)

### CFO — sub-steps

0. Enumerate all DTC FE interfaces via config tree, call "Get Link Lock Status" FE Macro on
   each. If any DTC unlocked -> `ResetSERDES()` (upstream + per-link), advance to retry.
1. Retry CDR lock check. If still unlocked -> throw exception.

### DTC

Self-check `ReadSERDESRXCDRLock(DTC_Link_CFO)`. If not locked, throws exception.

## Phase 3a — Sync: CFO check + edge fix (no-ROC DTCs) (iteration 4)

Only active in **EventBuildingAndSyncMode**. In **EventBuildingMode**, all devices idle.

### CFO

Enumerates all DTCs, calls "Get RTF Interface Status" FE Macro on each. Validates: CFO Emulation
Mode OFF, JA Source RJ45, Saturated YES, CFO CDR Lock LOCKED. Throws if any check fails.

### DTC w/o ROCs — sub-steps

0. Clear JA counters (CDR Unlock, JA Unlock, JA Recovered Clock LOS, JA External Clock
   LOS — SoftReset does not clear these), then `SoftReset()`.
   Wait for CFO Rx Clock Markers > 1000 (3s timeout, throw if not reached).
1. Check edge errors (txMarkers, rxToTx, parity, batchSlip).
   If any non-zero -> `ToggleExternalCFOSampleEdge()`, 100ms settle, `SoftReset()`,
   continue to sub-step 2.
   If all zero -> verify JA counters stayed 0 (throw if not), done.
2. `SoftReset()` to clear transient counters from edge flip, wait for markers > 1000 again
   (3s timeout).
3. Re-check edge errors. If still present -> throw. Otherwise verify JA counters stayed 0.

### DTC w/ ROCs: idle

## Phase 3b — Sync: edge fix (ROC DTCs) (iteration 5)

Only active in **EventBuildingAndSyncMode**.

### DTC w/ ROCs

Same sub-step sequence as Phase 3a for no-ROC DTCs.

### CFO, DTC w/o ROCs: idle

## Phase 3c — Sync: RTF offset + verify (no-ROC DTCs) (iteration 6)

Only active in **EventBuildingAndSyncMode**.

### DTC w/o ROCs — sub-steps

0. Clear JA counters (CDR Unlock, JA Unlock, JA Recovered Clock LOS, JA External Clock LOS).
   Read `ReadRTFHistIdelay()`, verify saturated and bin != 7. Compute
   `impliedPos = 2 - satBin`, call `SetCFOSamplePermanentOffset(impliedPos)`, `SoftReset()`.
1+. Wait for markers > 1000 (1s polls, up to ~7s timeout).
Verify. Read error flags (RTF 40MHz Phase Shift, Illegal Marker Timing, Event Start Marker
Tx, Clock Marker Tx, Rx-to-Tx Data Corruption) and RTF counters (Event Start Character Error,
40MHz Character Error, CDC Diagnostic parity + batch slip).
  - If **RTFPhase is the only error** (all other flags clear, all RTF counters zero): toggle
    RTF punched clock edge via `ToggleRTFPunchedClockEdge()` (Control Register bit 7),
    100ms settle, `SoftReset()`, then re-wait for markers > 1000 and re-verify.
    This retry happens at most once.
  - If any other error flag or RTF counter is non-zero (or RTFPhase persists after retry):
    throw with `getCFORTFSettingsStatusAndErrors()` output.
  - Otherwise: verify JA counters (CDR, JA, JA-Rec, JA-Ext) stayed 0 since the clear at
    sub-step 0 (throw if not). Pass.

### CFO, DTC w/ ROCs: idle

## Phase 3d — Sync: RTF offset + verify (ROC DTCs) (iteration 7)

Only active in **EventBuildingAndSyncMode**.

### DTC w/ ROCs

Same sub-step sequence as Phase 3c.

### CFO, DTC w/o ROCs: idle

## Phase 4 — ROC and DCS Setup (iteration 8)

Only DTCs with real ROCs act. CFO and no-ROC DTCs idle.

Sub-step 0:
1. `SetupROCs()` per link — enables/disables links, configures emulation per `roc_mask_` and
   `roc_emulated_mask_`. CRV DTCs force `clockMakersEnabled = false`.
2. `EnableDCSReception()`
3. CRV: `SetPunchEnable()`
4. `SoftReset()` to clear lock counters
5. If `EnableROCConfigureStep` config is true: begin ROC DCS-based configure (continue to
   sub-step 1+)

Sub-steps 1+: ROC DCS-based configure. The DTC acts as FESupervisor for its ROCs — sets each
ROC's sub-iteration index, calls `roc->configure()`, checks `getSubIterationWork()`. First
sub-step also calls `WaitForLinkReady()` per ROC. Repeats until all ROCs are done.

## Phase 5 — ROC Data Path Setup (iteration 9)

Only DTCs with real ROCs act. CFO and no-ROC DTCs idle.

1. `DisableLink(EVB)`, read `EventBuilderDTCID`/`EventBuilderMode`/`EventBuilderPartitionID`/
   `EventBuilderMACIndex` from config, `SetEVBInfo()`
2. Software DRP mode: `EnableSoftwareDRP()` or `DisableSoftwareDRP()` based on config
3. `EnableLink(EVB)`
4. Read `EventModeRequiredMask` from config (default 0), `SetCFOEventModeRequiredMask(mask)`

## Final SoftReset (iteration 10)

Both CFO and all DTCs: `SoftReset()` to clear accumulated errors from ROC/DCS and data path
setup.

## Phase 6 — Enable CFO Operation (iteration 11)

CFO only. All DTCs idle.

- `EnableAcceleratorRF0()`
- `SetPunchEnable()`

Links and clock markers are already enabled from Phase 2a. `ReleaseAllBuffers` belongs in the
`start()` transition by the artdaq readout subsystem, not here.
