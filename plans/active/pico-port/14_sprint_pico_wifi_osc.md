# Sprint Pico-14: WiFi / OSC Remote Control

**Depends on:** Sprint Pico-9 (FreeRTOS), Sprint Pico-12 (Presets — OSC can trigger preset load)
**Approach:** CYW43 WiFi + minimal OSC parser over UDP.
**Estimated effort:** 4-5 days

> **Why this sprint exists:** WiFi/OSC provides a rich wireless control surface without
> hardware design. A single TouchOSC layout on a phone/tablet can expose every parameter
> with sliders, buttons, and XY pads. The Pico 2 W has a CYW43 WiFi chip on-board.

---

## Goal

Connect to WiFi and accept OSC (Open Sound Control) messages over UDP for wireless
parameter control from a laptop, phone, or tablet using apps like TouchOSC or Max/MSP.

---

## Task 14.1: CYW43 Library Swap

### Problem

Currently linked to `pico_cyw43_arch_none` (LED access only, no WiFi).

### Fix

In CMakeLists.txt, replace:
```cmake
pico_cyw43_arch_none → pico_cyw43_arch_lwip_sys_freertos
```

This links the full CYW43 WiFi driver, lwIP TCP/IP stack, and FreeRTOS threading
support. Remove `cyw43_arch_poll()` calls from serial task (FreeRTOS background task
handles driver servicing).

**Binary impact:** ~60-80KB flash increase. Current UF2 ~709KB of 2MB — plenty of room.

**Gotcha:** CYW43 WiFi + BLE simultaneously has known deadlock issues. This sprint uses
WiFi only, no BLE.

### Validation

- `just pico-build` succeeds
- Binary size documented
- LED heartbeat still works (CYW43 LED access API differs with new library)

---

## Task 14.2: WiFi Connection Manager

### Problem

Need to connect to WiFi and handle disconnection.

### Fix

New file: `src/platform/pico/pico_wifi.h/cpp`

- WiFi credentials: compile definitions in CMakeLists via a `.gitignore`d local config
  file (e.g., `src/platform/pico/wifi_config.cmake.local`)
- Connection flow:
  1. `cyw43_arch_enable_sta_mode()` — station mode
  2. `cyw43_arch_wifi_connect_timeout_ms(ssid, password, auth, 30000)`
  3. Print IP address on serial
- Reconnection: retry every 10 seconds if connection drops
- Serial commands: `WIFI_STATUS` (connection state, IP, RSSI), `WIFI_CONNECT <ssid> <pw>`

### Validation

- Synth connects to WiFi, IP visible via `WIFI_STATUS`
- `ping <ip>` from laptop succeeds

---

## Task 14.3: Radio Task

### Problem

WiFi and OSC need a dedicated FreeRTOS task.

### Fix

New file: `src/platform/pico/pico_radio_task.h/cpp`

FreeRTOS task at priority 2 (Low-Normal):
- Stack: 2048 words (8KB) — lwIP/CYW43 have deep call stacks
- Manages WiFi connection, then enters UDP OSC receive loop
- Calls `app` methods to dispatch OSC commands

### Validation

- Task runs, WiFi connects
- No stack overflow (check `uxTaskGetStackHighWaterMark`)

---

## Task 14.4: Minimal OSC Parser

### Problem

Need to parse OSC messages without a heavy library.

### Fix

New file: `src/platform/pico/pico_osc.h/cpp`

OSC is a simple binary protocol. Minimal parser (~100-150 lines):
- Read address pattern (null-terminated, padded to 4-byte boundary)
- Read type tag string (e.g., `",f"` for float, `",i"` for int)
- Read arguments (big-endian float/int)

OSC address-to-parameter mapping:

| OSC Address | Type | Maps To |
|---|---|---|
| `/polysynth/note/on` | int, int | NoteOn(note, velocity) |
| `/polysynth/note/off` | int | NoteOff(note) |
| `/polysynth/filter/cutoff` | float | filterCutoff (20-20000) |
| `/polysynth/filter/resonance` | float | filterResonance (0-1) |
| `/polysynth/amp/attack` | float | ampAttack |
| `/polysynth/amp/decay` | float | ampDecay |
| `/polysynth/amp/sustain` | float | ampSustain |
| `/polysynth/amp/release` | float | ampRelease |
| `/polysynth/preset/load` | int | Load preset slot |
| `/polysynth/panic` | — | Panic (all notes off) |

Plus all other SynthState parameters following `/polysynth/<section>/<param>`.

### Validation

- Send OSC from laptop (`oscsend` CLI or TouchOSC)
- Filter cutoff changes audibly

---

## Task 14.5: UDP Socket Listener

### Problem

Need to receive OSC packets.

### Fix

After WiFi connects, in the radio task:
```cpp
struct udp_pcb* pcb = udp_new();
udp_bind(pcb, IP_ADDR_ANY, 9000);  // Standard OSC port
udp_recv(pcb, osc_recv_callback, &app);
```

**Critical: Multi-producer safety.** With serial, MIDI, and OSC all sending commands,
three producers write to the audio engine. The `SPSCQueue` is single-producer.

**Required fix:** All command sources must be serialized through a single FreeRTOS queue
(multi-producer safe). A "command dispatcher" task drains the FreeRTOS queue and pushes
into the SPSC audio queue. This keeps the audio-side SPSC unchanged.

If Sprint 11 already added a mutex (the simpler approach), this sprint must upgrade to
the FreeRTOS command queue pattern — a mutex with three contending tasks is fragile.

### Validation

- OSC messages control synth parameters
- Serial + MIDI + OSC work simultaneously without crashes

---

## Task 14.6: mDNS Service Discovery (Optional)

### Problem

Users must know the IP address to send OSC.

### Fix

Register via mDNS as `polysynth.local`:
```cpp
mdns_resp_add_netif(netif, "polysynth");
```

lwIP includes an mDNS responder. Low effort, high convenience.

### Validation

- `ping polysynth.local` resolves from laptop on same network

---

## Risks

1. **WiFi latency** — UDP OSC over WiFi adds 1-10ms latency. Acceptable for parameter
   control, marginal for note triggering. Document expected latency.
2. **Multi-producer SPSC violation** — Critical. Must be addressed. See Task 14.5.
3. **lwIP memory** — Needs its own buffer pool configured via `lwipopts.h`. Default
   settings should work but may need tuning.
4. **WiFi credentials in source** — Use `.gitignore`d local config file.
5. **CYW43 + FreeRTOS stability** — Test with sustained WiFi traffic during audio.

---

## Definition of Done

- [ ] CYW43 WiFi connects to network
- [ ] IP address visible via serial `WIFI_STATUS`
- [ ] UDP OSC listener on port 9000
- [ ] At least 10 OSC-controllable parameters
- [ ] Note on/off via OSC works
- [ ] Multi-producer command flow safe (FreeRTOS queue or equivalent)
- [ ] Serial + MIDI + OSC work simultaneously
- [ ] No audio glitches during WiFi traffic
- [ ] mDNS `polysynth.local` resolves (optional)
- [ ] `just test` and `just test-embedded` pass
- [ ] `just pico-build` succeeds
- [ ] Binary size increase documented
