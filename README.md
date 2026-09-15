# C-Dogs for PicOS

C-Dogs SDL is a classic overhead run-and-gun game — squad-based shooting, destructible
scenery and dozens of campaigns — originally written by Ronny Wester and maintained today
by Cong Xu ([cxong/cdogs-sdl](https://github.com/cxong/cdogs-sdl), GPL-2.0). This repo is
that game ported to [PicOS](https://github.com/jeffory/PicOS) on the ClockworkPi PicoCalc:
the upstream engine is vendored under `src/` and builds against a small SDL shim
(`picos_sdl*.{h,c}`, `sdl_shim/`) that maps SDL video, input and mixer calls onto the PicOS
native API. Licence is GPL-2.0 (see `COPYING`); the port keeps that licence.

The app needs the `root-filesystem` and `audio` requirements and reads its game data from
`/apps/cdogs/data/` at runtime, so the whole `data/` tree ships inside the release ZIP.

## Install

C-Dogs is on the **PicOS App Store** — open the Store app on your PicoCalc and install it
from there. Nothing else to do.

## Build

Needs `arm-none-eabi-gcc` (tested with 15.2) and a newlib for ARM:

```sh
make
```

This produces `main.elf` (stripped) plus `main.elf.debug`. The build is large — expect a
few minutes at `-Os`. The PicOS native SDK headers and linker script are vendored in
`sdk/native/`.

`data/` is tracked in this repo and is what ships to the device. If you update the vendored
`src/` tree, regenerate it with:

```sh
./prepare_data.sh
```

## Release

1. Bump `version` in `app.json`.
2. Commit the change.
3. `git tag v<version> && git push && git push --tags`

GitHub Actions builds `main.elf`, packages `app.json`, `main.elf` and `data/` into a single
ZIP and publishes it as the Release for that tag. The PicOS App Store re-indexes within
about 30 minutes, after which the new version shows up on-device.
