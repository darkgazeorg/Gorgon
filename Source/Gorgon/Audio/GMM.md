/*! \page gmm_syntax Gorgon Music Macro (GMM) Syntax

Gorgon Music Macro (GMM) is a compact text format for describing polyphonic, retro-style music
and sound effects directly as strings. It is designed to be easy to read and write while
still supporting key features like multiple voices, tempo automation, instrument switching,
and basic pitch slides.

GMM separates the definition of instruments (the *header*) from the actual musical sequence
(the *body*). This split keeps the playback code free of hard-coded “magic numbers” and makes
it easy to reuse instrument settings across multiple tracks.

## Structure

### Header (instrument declarations)

The header consists of instrument declarations that assign a numeric ID to a waveform type and
its envelope/configuration. Each declaration starts with `@ID`.

Example:

```text
@1 = sine, curve=sqrt, falloff=150
@2 = pulse, duty=50
@3 = noise, bitdepth=8
```

### Body (track data)

The body contains one or more tracks, each tagged with a track identifier (`1>`, `2>`, …).
The tracks are processed simultaneously, allowing polyphony.

Example:

```text
1> T120 @1 C4 D4 E3/4 R4 G2.
2> @2 C2 C2 C2 C2

# Example combining volume automation and slide
1> V100 C4 V50 C4 V25 C4~F4 V0 R4
```

## Core Commands

### `@ID = type, param=value`
Defines an instrument in the header.

- `ID`: numeric instrument identifier
- `type`: waveform type (`sine`, `pulse`, `noise`, ...)
- `param=value`: optional envelope / waveform parameters (such as `curve`, `duty`, `falloff`)

### `T<value>`
Tempo change (beats per minute). Applies to the current track.

Example: `T120` sets tempo to 120 BPM.

### `N>`
Track identifier used to select which voice is being written. Each track is mixed together.

Example: `1>`, `2>`.

### `@ID`
Inline instrument switch. Changes the current track to use the specified instrument
(from the header).

### Notes: `A`–`G` (optionally `+` or `-` for sharps/flats)
A note can be followed by a duration token:

- `C2` — duration as a note fraction (quarter, half, etc.)
- `C3/4` — fractional duration (three quarters)
- `C(0.5)` — absolute time in seconds
- `C0.22` — absolute tempo units (relative to current BPM)

A dot (`.`) after a duration extends it by 50% (dotted notes).

### `R<Number>`
Rest (silence) with the given duration.

### `V<Percent>`
Volume multiplier (0–100%).

### `O<Octave>` / `<` / `>`
Octave control. `O5` sets octave 5. `<` and `>` shift the current octave down/up by one.

### `~` (slide/portamento)
Placed between two notes to smoothly glide pitch from the first note to the next over the first
note’s duration.

Example: `C2~B2`.

*/
