/*! \page gmm_syntax Gorgon Music Macro (GMM) Syntax

Gorgon Music Macro (GMM) is a compact text format for describing polyphonic, retro-style music
and sound effects directly as strings. It is designed to be easy to read and write while
still supporting advanced features like multiple voices, multi-channel panning, tempo automation, 
instrument switching, and non-linear parameter ramps.

GMM separates the definition of global settings and instruments (the *header*) from the actual 
musical sequence (the *body*). This split keeps the playback code free of hard-coded “magic numbers” 
and makes it easy to reuse instrument settings across multiple tracks.

## Structure

### Header (Engine Config & Instrument Declarations)

The header consists of global engine configurations (starting with `%`) and instrument 
declarations (starting with `@`). Instrument declarations assign a numeric ID to a waveform 
type and its envelope.

Example:

```text
# Engine Config
%CHANNELS = 2

# Instrument Bank
@1 = sine, curve=sqrt, falloff=150
@2 = pulse, duty=50
@3 = noise, bitdepth=8
```

### Body (Track Data)

The body contains one or more tracks, each tagged with a track identifier (`1>`, `2>`, …).
The tracks are processed simultaneously, allowing polyphony.

Example:

```text
1> T120 @1 C4 D4 E3/4 R4 G2.
2> @2 C2 C2 C2 C2

# Example combining multi-channel panning, volume fades, and non-linear slides
1> V(0, 100) V(1, 0) C4 V(0, 0:4){exp} V(1, 100:4){exp} C4~F4{sqrt} V0 R4
```

## Core Commands

### `%KEY = value`
Global engine configuration. Must be defined in the header.
- `%CHANNELS = 2` sets the engine to render interleaved stereo audio.

### `@ID = type, param=value`
Defines an instrument in the header.
- `ID`: numeric instrument identifier
- `type`: waveform type (`sine`, `pulse`, `noise`, ...)
- `param=value`: optional envelope / waveform parameters (such as `curve`, `duty`, `falloff`, `taper`)

### `N>`
Track identifier used to select which voice is being written. Each track is mixed together.
Example: `1>`, `2>`.

### `@ID`
Inline instrument switch. Changes the current track to use the specified instrument (from the header).

### `T<Value>[:Duration]{Curve}`
Tempo change (beats per minute). 
Can be an immediate change (`T120`) or smoothly ramped over a duration (`T60:1{scurve}`).

### Notes: `A`–`G` (optionally `+` or `-` for sharps/flats)
A note can be followed by a duration token:
- `C2` — duration as a note fraction (quarter, half, etc.)
- `C3/4` — fractional duration (three quarters)
- `C(0.5)` — absolute time in seconds
- `C0.22` — absolute tempo units (relative to current BPM)

A dot (`.`) after a duration extends it by 50% (dotted notes).

### `R<Duration>`
Rest (silence) with the given duration.

### `V<Percent>[:Duration]{Curve}`
Global volume adjustment (0–100%). Modifies all channels for the current track.
Can be immediate (`V80`) or ramped over time (`V0:4{exp}`).

### `V(<Channel>, <Percent>[:Duration]{Curve})`
Channel-specific volume override. Used for panning audio in multi-channel setups.
Example: `V(0, 100)` sets Channel 0 (Left) to 100%. `V(1, 0:2)` fades Channel 1 (Right) to 0% over a half-note.

### `O<Octave>` / `<` / `>`
Octave control. `O5` sets octave 5. `<` and `>` shift the current octave down/up by one.

### `~[{Curve}]` (Slide/Portamento)
Placed between two notes to smoothly glide pitch from the first note to the next over the first
note’s duration. 
Example: `C2~B2`. Can accept a curve modifier for non-linear pitch sweeps (e.g., `C2~C6{sqrt}`).

---

## Ramps and Curves
Several commands (`T`, `V`, and `~`) support continuous interpolation over time. 
This is achieved using the `:Duration` and `{curve}` syntax.

* **`:Duration`** — Defines how long the transition takes. It accepts the same timing formats as notes (e.g., `:4` for a quarter note, `:(5.0)` for 5 seconds).
* **`{curve}`** — Defines the mathematical easing applied to the interpolation. If omitted, the transition is linear.
  * `{linear}`: Standard linear interpolation (Default).
  * `{exp}`: Exponential curve (Accelerating, standard for natural-sounding volume fades).
  * `{sqrt}`: Square root curve (Decelerating, fast initial change that slows down).
  * `{scurve}`: Smoothstep curve (Slow start, fast middle, gentle settle).
*/