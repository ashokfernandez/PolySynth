# Sprint Pico-12: Preset System (Flash Storage)

**Depends on:** Sprint Pico-11 (USB MIDI — for testing presets with real musical input)
**Blocks:** Sprint Pico-13 (Effects — effect params included in presets)
**Approach:** Binary preset format in onboard flash, serial commands for management.
**Estimated effort:** 2-3 days

> **Why this sprint exists:** Without presets, every sound must be dialled in from
> scratch on each power cycle. Flash-based presets make the synth usable as a real
> instrument — save your sounds, recall them instantly, and share them via serial.

---

## Goal

Save and load synthesizer patches to the Pico's onboard 2MB flash. Presets persist
across power cycles. 16 user slots + factory presets. Serial commands for management.
Slot 0 auto-loads on boot.

---

## Task 12.1: Binary Preset Format

### Problem

The desktop `PresetManager` uses nlohmann/json (25K+ header, heap-heavy). Unsuitable
for embedded.

### Fix

New file: `src/platform/pico/pico_preset.h`

```cpp
struct PicoPreset {
    uint32_t magic;      // 'PSYN' (0x5053594E)
    uint16_t version;    // Format version (1)
    uint16_t size;       // sizeof(SynthState) at time of save
    uint32_t checksum;   // CRC32 of SynthState bytes
    char name[16];       // Null-terminated preset name
    SynthState state;    // Raw struct copy (aggregate, no pointers)
    // Padded to fit within one 4KB flash sector
};
```

`SynthState` is an aggregate with all float/int fields and no pointers — raw `memcpy`
is safe as long as `version` matches. CRC32 detects flash corruption.

Include a minimal CRC32 implementation (~50 lines, no library needed).

### Validation

- `static_assert(sizeof(PicoPreset) <= 4096)` — fits in one flash sector
- `static_assert(std::is_aggregate_v<SynthState>)` — safe to memcpy

---

## Task 12.2: Flash Storage Driver

### Problem

Need to read/write presets to flash without corrupting running code.

### Fix

New file: `src/platform/pico/pico_flash_store.h/cpp`

Use Pico SDK `hardware/flash.h`:
- `flash_range_erase(offset, size)` — erases 4KB sectors
- `flash_range_program(offset, data, size)` — writes 256-byte pages
- Read via XIP direct memory access: `(const PicoPreset*)(XIP_BASE + offset)`

Reserve last 64KB of flash (16 sectors) for 16 preset slots:
- Flash offset: `PICO_FLASH_SIZE_BYTES - (16 * 4096)`

**Critical safety:** Flash write/erase temporarily disables XIP. Use
`flash_safe_execute()` which handles multicore safety (disables interrupts on both
cores). Audio ISR on Core 1 runs from SRAM (`__time_critical_func`) so DMA continues,
but verify no non-SRAM code is called from the audio path.

### Validation

- Write a test preset, read it back, verify CRC matches
- Power cycle — preset persists
- No audio glitch during save (test with sustained note)

---

## Task 12.3: Preset Serial Commands

### Problem

Need user-facing commands for preset management.

### Fix

Extend `pico_command_dispatch.cpp` and `command_parser.h`:

| Command | Action |
|---|---|
| `SAVE <slot> [name]` | Save current state to slot 0-15 with optional name |
| `LOAD <slot>` | Load preset from slot, apply to engine |
| `LIST` | List all occupied slots with names |
| `DELETE <slot>` | Erase preset slot |
| `DUMP <slot>` | Dump preset as hex (for serial backup/transfer) |
| `UPLOAD` | Receive hex-encoded preset via serial |
| `FACTORY <n>` | Load factory preset n |

On `LOAD`: copy `preset.state` into `app.GetPendingState()`, call
`app.PushStateUpdate()` to apply.

### Validation

- `SAVE 0 "Bright Pad"` then `LOAD 0` restores identical sound
- `LIST` shows slot 0 occupied with name

---

## Task 12.4: Factory Presets

### Problem

The synth should ship with usable sounds out of the box.

### Fix

Define 4-8 factory presets as `constexpr` arrays in a separate flash region (read-only,
outside the 16 user slots):

| Preset | Description |
|---|---|
| Init Patch | Default SynthState |
| Fat Bass | Saw, low cutoff, high resonance, short attack |
| Bright Pad | Two detuned saws, high cutoff, slow attack/release |
| Lead | Square wave, medium cutoff, legato glide |
| Filter Sweep | Saw, filter envelope modulation, medium resonance |

### Validation

- Each factory preset produces a distinct, musical sound
- `FACTORY 0` through `FACTORY 4` all work

---

## Task 12.5: Boot Preset Load

### Problem

Synth should restore the last-used sound on power-up.

### Fix

On startup (after `PicoSynthApp::Init()`):
1. Check if slot 0 contains a valid preset (magic + CRC check)
2. If valid, load it (last-used patch restoration)
3. If invalid/empty, use default `SynthState`
4. Print loaded preset name on serial

`SAVE 0` becomes "save as default boot patch".

### Validation

- Save a preset to slot 0, power cycle, synth boots with saved sound
- With empty slot 0, synth boots with default sound

---

## Risks

1. **SynthState struct changes** — Adding/removing fields breaks saved presets. The
   `version` and `size` fields detect this. For now, version mismatch = preset ignored
   with serial warning. Migration code can be added later.
2. **Flash write during audio** — `flash_safe_execute` disables interrupts briefly
   (~50ms for sector erase + program). Audio DMA ISR runs from SRAM and should be
   unaffected, but verify with sustained note during save.
3. **Flash wear** — 100K erase cycles per sector. At 10 saves/day = 27 years. Not a
   concern.

---

## Definition of Done

- [ ] Binary preset format with magic, version, CRC32
- [ ] 16 user preset slots in flash (last 64KB)
- [ ] SAVE/LOAD/LIST/DELETE serial commands work
- [ ] DUMP/UPLOAD enable serial transfer
- [ ] 4+ factory presets with distinct sounds
- [ ] Slot 0 auto-loads on boot
- [ ] Power cycle preserves presets
- [ ] No audio glitch during save operation
- [ ] `just test` and `just test-embedded` pass
- [ ] `just pico-build` succeeds
