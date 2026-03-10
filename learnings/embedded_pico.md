# Embedded / Pico Learnings

Hard-won lessons from working with the RP2350 Pico platform in this codebase.

---

## Type Discipline on Cortex-M33

**Problem:** `SynthState` fields were `double` and Pico platform code used `double`
for function signatures, causing implicit double→float narrowing throughout the
embedded pipeline.

**Action:** Changed all `SynthState` fields from `double` to `float`. Updated
`ParamMeta.h` (desktop `reinterpret_cast`), `PolySynth.cpp` (iPlug2 boundary casts),
and Pico `main.cpp` (`set_param`/`get_param` signatures). All tests pass — the
float→JSON double→float round-trip is lossless because every float is exactly
representable as a double.

**Lesson:** On Cortex-M33, `double` operations are software-emulated (~10x slower
than `float`). Use `float` for all control parameters (including `SynthState`),
`sample_t` for per-sample DSP signals, and `uint32_t` for timestamps. iPlug2's
`GetParam()->Value()` returns `double` — narrow at the boundary with
`static_cast<float>()`. 7 significant digits (float) is more than enough for
synth parameters like `filterCutoff = 20000.0`. Enable `-Wdouble-promotion` to
catch accidental double promotion in hot paths.

---

## `std::atomic<uint64_t>` Is NOT Lock-Free on 32-bit ARM

**Problem:** `Engine::UpdateVisualization()` uses `std::atomic<uint64_t>` which is
called from the DMA ISR. On Cortex-M33 (32-bit), 64-bit atomics use `__atomic_store_8`
from libatomic, which disables interrupts or uses a global spinlock — breaking
real-time safety.

**Action:** Sprint 7 plan removes `UpdateVisualization()` from the ISR entirely.
Added AGENTS.md rule: "No `std::atomic<uint64_t>` in ISR context" and pre-flight
check: "verify atomics are lock-free with `static_assert`."

**Lesson:** Always verify `std::atomic<T>::is_always_lock_free` for any type used
across the ISR boundary. On 32-bit ARM, only 8/16/32-bit atomics are lock-free.
Use two `std::atomic<uint32_t>` instead of one `std::atomic<uint64_t>`, or redesign
the data flow to avoid the 64-bit store.

---

## `s_peakLevel` Data Race (PR #65 Review)

**Problem:** `s_peakLevel` (a plain `float`) is written by the ISR and read by the
main loop without synchronization. Single-word float stores are atomic in practice on
Cortex-M33, but it's technically undefined behavior per the C++ memory model.

**Action:** Sprint 7 plan adds conversion to `std::atomic<float>` with relaxed memory
ordering. Main loop uses `.exchange(0.0f)` for atomic read-and-reset.

**Lesson:** Even when hardware guarantees atomic stores for aligned 32-bit values,
use `std::atomic<float>` to make the contract explicit and avoid UB. The relaxed
ordering has zero overhead on ARM (no barrier instructions needed).

---

## XIP Cache Misses in DMA ISR

**Problem:** All code runs from external QSPI flash via XIP (Execute-In-Place) cache.
When the DMA ISR fires and cache doesn't contain the next DSP function, execution
stalls for flash fetch. With branchy DSP code (waveform switch, ADSR stage, filter
model), cache misses are virtually guaranteed — causing variable-latency audio glitches.

**Action:** Sprint 7 plan adds `__time_critical_func` attribute to `dma_irq_handler`
and `audio_callback` to place them in SRAM (~4-8KB cost out of 520KB).

**Lesson:** On RP2350, always mark DMA ISR handlers and their callees with
`__time_critical_func`. Since SEA_DSP is header-only and inlined, the inlined DSP
code ends up in SRAM automatically. Verify placement with
`arm-none-eabi-objdump -h build/pico/polysynth_pico.elf | grep time_critical`.

---

## FreeRTOS: Use Raspberry Pi Fork, Not Mainline

**Problem:** Mainline FreeRTOS `ARM_CM33_NTZ/non_secure` port builds but crashes in
`crt0.S` on RP2350. The RP2350 runs in secure mode only (no TrustZone non-secure
mode available).

**Action:** Plans specify `https://github.com/raspberrypi/FreeRTOS-Kernel.git` which
includes a `RP2350_ARM_NTZ` portable layer.

**Lesson:** Always use the Raspberry Pi fork of FreeRTOS-Kernel for RP2350 targets.
The mainline port's `ARM_CM33_NTZ` assumes actual non-secure mode operation, which
doesn't match the RP2350's secure-only execution model.

---

## FreeRTOS Stack Sizes Are in Words, Not Bytes

**Problem:** `configMINIMAL_STACK_SIZE = 256` looks small but is actually 1024 bytes
(256 × 4 bytes per word on ARM). Confusing units lead to either wasted memory or
stack overflows.

**Lesson:** FreeRTOS on ARM measures stack sizes in 32-bit words. Always comment the
byte equivalent: `configMINIMAL_STACK_SIZE 256 // = 1024 bytes`. Enable
`configCHECK_FOR_STACK_OVERFLOW = 2` during development.

---

## Tanh Call Count: 6, Not 5

**Problem:** Architecture doc and sprint plans disagreed on the number of tanh calls
in the ladder filter (5 vs 6). The actual `ProcessTransistor()` code calls:
`Tanh(in)` + `Tanh(u)` + 4× `Tanh(v_out)` in loop = **6 total**.

**Action:** Standardized all plan documents on 6 tanh calls. Updated CPU budget math.

**Lesson:** Always verify DSP cycle counts against actual source code, not design
documents. The difference between 5 and 6 tanh calls changes voice-count projections.
