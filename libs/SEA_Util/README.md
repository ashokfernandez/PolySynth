# SEA_Util

`SEA_Util` is a header-only C++17 utility library in the `sea::` namespace for
instrument and music-software building blocks that do **not** process audio
samples directly.

## Scope

Use `SEA_Util` for reusable non-DSP primitives such as:
- lock-free queues and data structures
- voice/resource allocation helpers
- music theory and MIDI utilities

If a type has a `Process(sample)`-style audio path, it belongs in `SEA_DSP`
instead.

## Conventions

- Header-only (`INTERFACE` CMake target)
- Public API namespace: `sea::`
- C++17 portable code
- Real-time safe design (no heap allocation after init)
- No framework dependencies

## Testing

`SEA_Util` components are validated from the project `tests/` target. Include
headers as:

```cpp
#include <sea_util/your_header.h>
```
