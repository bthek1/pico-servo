# Plan: Local Compile + Remote Flash

## Goal

- Compile runs **locally** (this machine) — no more SSH to Pi for builds
- Flash copies the `.uf2` to the Pi via `scp`, then runs `flash.sh` on the Pi via `ssh`
- Remove the git repo from the Pi — it is no longer needed
- Update `justfile` to match the new workflow

---

## Changes

### 1. `compile.sh` — no changes needed

`compile.sh` already runs cmake/make in the local `build/` dir with no Pi-specific paths. It works as-is when run locally, provided the ARM cross-compiler and Pico SDK are available on this machine.

Verify locally:
```bash
arm-none-eabi-gcc --version   # must exist
./compile.sh website
```

### 2. `flash.sh` — add remote flash mode

Add a `--remote` flag (or detect by default) that:
1. `scp`s the `.uf2` to `pi:~/pico-flash/<target>.uf2` (a scratch dir, no repo needed)
2. SSHes to the Pi and copies the file to the mount point

New logic in `flash.sh`:
```
if --remote (or no local mount found):
    scp build/targets/<target>/<target>.uf2  pi:~/pico-flash/<target>.uf2
    ssh pi "cp ~/pico-flash/<target>.uf2 /media/bthek1/RPI-RP2/"  (or via picotool reboot)
else:
    existing local cp behaviour
```

The Pi only needs the Pico plugged in — no repo, no SDK.

### 3. Remove git repo from the Pi

Once the plan is complete and local compile is confirmed working:
```bash
ssh pi "rm -rf ~/Documents/pico/pico-servo"
```

The Pi only needs:
- `picotool` (for BOOTSEL auto-reboot) — keep if installed
- USB access to the Pico (`/dev/ttyACM0`, `/media/bthek1/RPI-RP2/`)

### 4. `justfile` — rewrite for local workflow

Remove:
- `pull` (no remote repo)
- `push-secrets` (secrets stay local; no remote repo to sync to)
- `deploy` / `deploy-clean` (replace with simpler local equivalents)
- All `ssh pi "cd ~/Documents/pico/pico-servo && ..."` compile invocations

Add / keep:
- `compile target=''` → runs `./compile.sh {{target}}` locally
- `compile-clean target=''` → runs `./compile.sh --clean {{target}}` locally
- `flash target=''` → `./flash.sh --remote {{target}}` (scp + ssh)
- `deploy target=''` → `(compile target) (flash target) _wait-tty monitor`
- `monitor` → `ssh pi "picocom -b 115200 /dev/ttyACM0"` (keep, Pi still owns the serial port)

---

## Prerequisites (local machine)

Confirm before starting:
- [ ] `arm-none-eabi-gcc` installed locally
- [ ] `cmake` installed locally
- [ ] Pico SDK submodule initialised (`git submodule update --init --recursive`)
- [ ] `just` installed locally
- [ ] SSH key auth to Pi working (`ssh pi` no password prompt)

---

## Implementation Order

1. Verify local compile works (`./compile.sh website`)
2. Update `flash.sh` with remote scp+ssh logic
3. Test `./flash.sh --remote website` end-to-end
4. Rewrite `justfile`
5. Test `just deploy`
6. Remove git repo from Pi: `ssh pi "rm -rf ~/Documents/pico/pico-servo"`
7. Move this plan to `docs/plans/completed/`
