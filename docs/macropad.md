# Corne macropad

Layers (switch: hold the **left middle thumb** on HOME / OBS / MACROS to open PICK, then tap a target).
Esc is the top-left key on every layer. No layer has a transparent key (`scripts/lint-keymap.sh`).

| # | Layer  | Purpose |
|---|--------|---------|
| 0 | HOME   | default. Win+L lock, screenshots (Win / Mac / Linux-flameshot) |
| 1 | GAME   | left half only; leave with a 2.2 s hold on the top-left Esc key |
| 2 | OBS    | scenes, record, stream, audio (chords below) |
| 3 | MACROS | GUI+Space, tmux prefix chords |
| 4 | PICK   | S = HOME, D = GAME, F = OBS, G = MACROS |

Reboot, deep sleep (battery only) and leaving GAME land on HOME.

## OBS chords (Ctrl+Shift+Alt + key)

| Key (OBS layer)        | Chord                 | Action                    |
|------------------------|-----------------------|---------------------------|
| row 1, col 2           | Ctrl+Shift+Alt+F13    | switch to scene 1         |
| row 1, col 3           | Ctrl+Shift+Alt+F14    | switch to scene 2         |
| row 1, col 4           | Ctrl+Shift+Alt+F15    | switch to scene 3         |
| row 2, col 2           | Ctrl+Shift+Alt+F16    | record start/stop         |
| row 2, col 3           | Ctrl+Shift+Alt+F17    | record pause              |
| row 2, col 4 (hold 0.7 s) | Ctrl+Shift+Alt+F18 | stream start/stop         |
| row 3, col 2           | Ctrl+Shift+Alt+F19    | mic mute toggle           |
| row 3, col 3           | Ctrl+Shift+Alt+F20    | desktop audio mute toggle |
| row 3, col 4           | Ctrl+Shift+Alt+F21    | save replay buffer        |

Bind these in OBS (Settings > Hotkeys) or map them in a host script that calls obs-websocket.

## tmux macros (prefix = C-Space, from the user's tmux.conf)

| Key        | Sends                    | tmux action                      |
|------------|--------------------------|----------------------------------|
| T-PFX      | C-Space                  | prefix                           |
| T-SESS     | C-Space, s               | fzf session switch               |
| T-SCRATCH  | C-Space, C-n             | tpad scratchpad                  |
| T-LAZYGIT  | C-Space, C-g             | tpad lazygit (per directory)     |
| T-TPM-I    | C-Space, Shift+I         | tpm: install plugins             |

## Tools

- `scripts/lint-keymap.sh` : CI guard (no `&trans`, 42 bindings per layer, no layer keys on GAME).
- `scripts/rune.rb` : generates the OLED layer glyphs in `screen_art.h` (`ruby scripts/rune.rb check`).
- The old typing layout is the git tag `typing-v1`.
