# Corne (crkbd) layout — ported from your Dygma Defy

A three-layer ZMK layout for a 42-key Corne, rebuilt from your `Defy.json`. Drop
`corne.keymap` into your `zmk-config/config/` folder (replacing the reference
repo's file), keep ZMK Studio enabled, and flash.

## Keymap

![Corne keymap](corne_keymap.svg)

Auto-generated from [`config/corne.keymap`](config/corne.keymap) by
[keymap-drawer](https://github.com/caksoylar/keymap-drawer) on every push (the
`draw` workflow, or `make svg` locally), so it always reflects the current layout.

## The three layers

```
╔══════════════════════════ 0 · BASE (QWERTY) ══════════════════════════╗
   ESC   Q   W   E   R   T   │   Y   U   I   O   P   BSPC
   TAB   A   S   D   F   G   │   H   J   K   L   ;   DEL
   SHFT  Z   X   C   V   B   │   N   M   ,   .   /   MO→SYM
                GUI  NAV  SPC │ SPC  CTRL  ALT
```
Left outer column = **Esc / Tab / Shift**, right outer column = **Bspc / Del /
hold-for-Symbols**, exactly as you asked.

```
╔══════════════════════════ 1 · SYM (numbers/symbols) ══════════════════╗
   ·    1   2   3   4   5   │   6   7   8   9   0    ·
   ·    !   @   #   $   %   │   ^   &   *   (   )    ·
   ·    =   -   \   `   '   │   [   ]   {   }   ~    ·
                 ·   ·   SPC │ SPC   ·   ·
```
Numbers on the top row, their shifted symbols directly below, and the remaining
punctuation on the bottom row. `_ + { } | ~ " :` come free via Shift, and
`< > ?` stay on Shift+`, . /` from the base layer. The outer columns are
transparent, so Esc/Tab/Shift and Bspc/Del keep working while you're here.

```
╔══════════════════════════ 2 · NAV (macros + arrows) ══════════════════╗
   ·   F1   F2    F3   F4    F5   │  HOME  PGDN  PGUP  END   PSCR   ·
   ·   :q   :w    C␣J  C␣%   C␣"  │  ←     ↓     ↑     →     C␣p    ·
   ·   G←   G→    G↑   G↓    G⇧Q  │  C-S   C-W   C-D   C-Ins Ins    ·
                  ·    ·    SPC   │ SPC   ·    ·
```
Held with the **left middle thumb**. Your macros live on the left hand and a few
on the right; the arrow cluster and page-nav keys are on the right hand.

## Thumb modifiers (no home-row mods)

| Thumb | Tap | Hold |
|-------|-----|------|
| Left outer | Ctrl | **GUI** |
| Left middle | — | **NAV layer** |
| Left inner | **Space** | — |
| Right inner | **Space** | — |
| Right middle | Enter | **Ctrl** |
| Right outer | Super (Linux key) | **Alt** |

Shift stays on the base-layer left outer column. Both innermost thumbs are Space,
as requested.

## Your macros — decoded from the Defy

Every macro below is compiled into the firmware. The ones marked **on NAV** are
placed on the nav layer; the rest are **defined and ready** — assign them to any
key live in ZMK Studio (Studio can't create macros, but it can bind ones that
already exist in the firmware).

| Behavior | Sends | Where |
|----------|-------|-------|
| `m_vim_q` | `:q` ⏎ | on NAV |
| `m_vim_w` | `:w` ⏎ | on NAV |
| `m_cs_j` | Ctrl+Space → J | on NAV |
| `m_cs_pct` | Ctrl+Space → % | on NAV |
| `m_cs_dq` | Ctrl+Space → " | on NAV |
| `m_cs_p` | Ctrl+Space → p | on NAV |
| GUI+←/→/↑/↓ | window snap (was macros 12–15) | on NAV |
| GUI+Shift+Q | (was macro 11) | on NAV |
| Ctrl+S / Ctrl+W / Ctrl+D / Ctrl+Ins | save / close / (macros 18,19,10,9) | on NAV |
| `m_vim_rla` | `:rla` ⏎ | defined |
| `m_sp_f` | Space → Shift+F | defined |
| `m_i3lock` | types `i3lock` ⏎ | defined |
| `m_checklist` | `/checklist` ⏎ | defined |
| `m_banner_green` | GUI+← `/banner green` ⏎ GUI+→ | defined |
| `m_banner_red` | GUI+← `/banner red` ⏎ GUI+→ | defined |
| `m_banner_blue` | GUI+← `/banner blue` ⏎ GUI+→ | defined |
| `m_giphy` | `/giphy` ⏎ | defined |
| `m_green` | `green` ⏎ | defined |

Simple one-shot chords from your Defy (Ctrl+R, Ctrl+Alt+T, etc.) are just
`&kp LC(R)`, `&kp LC(LA(T))` — no macro needed; add them to any key directly.

## Enabling ZMK Studio

In your `config/corne.conf` make sure you have:

```
CONFIG_ZMK_STUDIO=y
```

and that `build.yaml` builds with the `studio-rpc-usb-uart` snapshot for the
central (left) half, matching the reference repo. Then connect at
studio.zmk.dev over USB to move keys around without reflashing.

## Notes / things you may want to tweak

- The Defy had extra keys (70 vs 42), so a handful of symbols now live one layer
  deeper. Nothing was dropped — it's all reachable on SYM or via Shift.
- If your board isn't a stock Corne, only the physical key *order* in the
  `bindings` blocks would change; the layer logic and macros port as-is.
- Want a numpad-style SYM instead of the number-row style? Easy swap — say the
  word.
