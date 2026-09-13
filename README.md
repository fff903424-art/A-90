# A-90 Geode Mod

Uses the supplied A-90 assets:

- `A-90.webp`
- `A-90_Stop_Sign.webp`
- `A-90_Jumpscare.webp`
- `A-90_Warning.mp3`
- `A-90_Jumpscare.mp3`

The encounter randomly appears after a configurable delay. During STOP, Jump/Left/Right press events kill the active player. Inputs are not globally disabled.

Configuration is in `src/A90State.hpp`.

Build with:

```bash
geode build
```

The current Geode documentation specifies `mod.json` resources and supports sprite/file resources; the current Geode loader also exposes each mod's runtime resources directory through `Mod::getResourcesDir()`.
