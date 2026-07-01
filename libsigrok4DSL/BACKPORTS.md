# libsigrok4DSL selective backport notes

This fork diverged from upstream libsigrok around commit `26aec7fd` (April
2013, ~libsigrok 0.2.0 era) and has not been rebased since. A full upstream
sync was evaluated and rejected (see the project's `update-sigrok` plan) since
DSL-specific config keys, struct fields, and the `SR_DF_DSO` packet type are
fused into what would otherwise be "core" code, making a wholesale swap a
multi-week architectural rewrite rather than a version bump.

Instead, this file tracks a small number of narrowly-scoped fixes applied
directly to the fork's existing code, found via direct review of the
in-scope generic/shared files (`strutil.c`, `std.c`, `hardware/common/*.c`)
rather than by porting specific upstream commits — 12+ years of drift meant
upstream's own fixes to these files target code structures (or functions
added post-fork) that no longer exist in this fork in a form a patch could
apply to. Where checked, upstream itself still has the same defect (noted
below), so these aren't "backports" in the strict sense of an existing
upstream fix — they're locally-identified and locally-fixed.

## Fixes applied

### 1. `strutil.c`: `sr_parse_period()` / `sr_parse_voltage()` — uninitialized out-parameter

**File/function:** `libsigrok4DSL/strutil.c:407` (`sr_parse_period`) and
`libsigrok4DSL/strutil.c:440` (`sr_parse_voltage`).

**Bug:** Both functions take `uint64_t *q` as an out-parameter (the unit
divisor: 1000 for "ms", 1 for "s", etc.). If the input string has digits but
**no recognized unit suffix at all** (the `if (s && *s)` block is skipped
because `*s` is `'\0'`), the function returns `SR_OK` without ever writing to
`*q` — the caller reads uninitialized stack memory.

**Reachable from:** `libsigrok4DSL/input/in_vcd.c:233` calls
`sr_parse_period()` with the VCD file's `$timescale` string — user-supplied
file content, not just internal/trusted callers.

**Fix:** Initialize `*q = 1` before the conditional block (matches the
semantic the "s"/"v" suffix branches already use for a bare base unit).

**Upstream status:** checked upstream's current `src/strutil.c` — the same
defect exists there unchanged as of this writing. Not a backport of an
existing fix; a fix found and applied locally.

### 2. `hardware/common/ezusb.c`: `ezusb_install_firmware()` — dead error-handling branch

**File/function:** `libsigrok4DSL/hardware/common/ezusb.c:71` inside
`ezusb_install_firmware()`.

**Bug:** `fread()` returns a `size_t` byte count (0 on EOF or error) and
never returns `EOF` (`-1`) — that's only a valid return value for
`fgetc()`/`getc()`-family functions. The existing `if (chunksize == EOF)`
check can never be true, silently swallowing `fread()` I/O errors (e.g. a
firmware file that becomes unreadable partway through a USB firmware
upload) as if the file had simply ended.

**Fix:** Replaced with `if (chunksize == 0 && ferror(fw))`, which correctly
distinguishes a genuine read error from a clean EOF, and reports it via
`strerror(errno)`.

**Upstream status:** not checked commit-by-commit (this is a straightforward
API-misuse fix identifiable from the code alone, not upstream-specific).

## Explicitly out of scope (per the original plan)

- `hwdriver.c`'s config-key table: DSL-extended (78+ custom `SR_CONF_*`
  keys), high regression risk for low reward.
- `session.c` / `session_driver.c` beyond the two fixes above: carry
  DSL-specific `SR_DF_DSO` packet-type branches; any upstream fix touching
  code adjacent to those branches needs re-verification before porting,
  and none of the fixes found in this pass came close to that code.
- `hardware/DSL/*.c`, `dsdevice.c`, `lib_main.c`: untouched by design (DSL
  hardware drivers, confirmed cleanly separated from generic code during
  initial scoping).
- `log.c`: entirely DSL's own `xlog` integration, no upstream equivalent
  exists to diff against.

## Verification performed

- `make -j12`: clean build, 0 warnings/errors.
- App launch smoke test: DSView starts, activates the demo device, and
  shuts down cleanly (exercises `backend.c`/`session.c`/`log.c`/`error.c`
  init/uninit paths, which is as much as a hardware-independent test can
  cover — no DSLogic/DSCope hardware was available to test against, but
  none of `hardware/DSL/*.c` was touched).
