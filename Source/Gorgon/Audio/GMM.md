/*! \page gmm_syntax Gorgon Music Macro (GMM) Syntax

Gorgon Music Macro (GMM) is a compact text format for describing polyphonic, retro-style music and sound effects directly as strings. It is designed to be easy to read and write while still supporting advanced features like multiple voices, multi-channel panning, tempo automation, instrument switching, and non-linear parameter ramps.

GMM separates the definition of global settings and instruments (the *header*) from the actual musical sequence (the *body*). This split keeps the playback code free of hard-coded “magic numbers” and makes it easy to reuse instrument settings across multiple tracks.

GMM supports comments through the `#` symbol. Anything following `#` on a line is ignored, allowing you to annotate your music or temporarily disable parts of the sequence. 

**Note: The following features are currently incomplete:**
- Slide
- Tempo and volume ramps
- Tracks

---

## 1. File Structure

### Header (Engine Config & Instrument Declarations)
The header consists of global engine configurations (starting with `%`) and instrument declarations (starting with `@`). Instrument declarations assign a numeric ID to a waveform type and its envelope.

### Body (Track Data)
The body contains one or more tracks, each tagged with a track identifier (`1>`, `2>`, …). The tracks are processed simultaneously, allowing polyphony.

**Example File:**
```text
# --- Engine Config ---
%CHANNELS = 2

# --- Instrument Bank ---
@1 = sine(Flute), attack={s, 64}, decay={linear, 2/1}, sustain=0, release={exp, 4}
@2 = pulse(Lead), duty=50
@3 = noise(Snare), bitdepth=8

# --- Sequence Data ---
1> T120 @1 C4 D4 E3/4 R4 G2.
2> @2 C2 C2 C2 C2

# Multi-channel panning, volume fades, and non-linear slides
1> V[0]100 V[1]0 C4 V[0]0:4{exp} V[1]100:4{exp} C4~F4{sqrt} V0 R4
```

---

## 2. Core Commands

### Engine Configuration
* **`%KEY = value`**: Global engine configuration. Must be defined in the header.
    * `%CHANNELS = 2` sets the engine to render interleaved stereo audio.
    * `%CHANNELS = [FL, FR]` explicitly specifies channels (See Gorgon::Audio::Channel for details).

### Track & Playback Control
* **`N>`**: Track identifier. Maps the following sequence to a specific voice (e.g., `1>`, `2>`). Each track is mixed together. Tracks can be split across multiple lines for readability. If track identified is not specified, it defaults to `1>`. Tracks have their own tempo and volume settings, but share the same global engine configuration (e.g., channels) and instrument bank.
* **`T<Value>[:Duration]{Curve}`**: Tempo change in beats per minute. Can be immediate (`T120`) or ramped (`T60:1{s}`). Affects only the current track.
* **`V<Percent>[:Duration]{Curve}`**: Global track volume (0–100%). Modifies all channels for the current track.
* **`V[<Channel>]<Percent>[:Duration[{<Curve>}]]`**: Channel-specific volume override for panning. Example: `V[0]100` sets Channel 0 to 100%. `V[1]0:2` fades Channel 1 to 0% over a half-note.

### Musical Notation
* **`A`–`G`**: Musical notes. Can be followed by `+` (sharp) or `-` (flat). If no duration is specified, a quarter note is used.
* **`R<Duration>`**: Rest (silence) for the given duration.
* **`O<Octave>` / `<` / `>`**: Octave control. `O5` sets octave 5. `<` and `>` shift the current octave down or up by one.
* **`~[{Curve}]` (Slide/Portamento)**: Smoothly glides pitch from the first note to the next over the total duration. Example: `C2~B2`. Accepts an optional curve modifier (e.g., `C2~C6{sqrt}`).
* **`S<Duration>`**: Sets note separation (articulation). Controls how long before the end of the note the release phase is triggered. Example: `S8` triggers the release an eighth note before the note ends. Release will never trigger before the attack phase finishes; if S is longer, it clamps to the attack duration. If attack is longer than the note duration, attack is shortened to fit the note duration, and S is ignored.

---

## 3. Durations & Ramps

### Durations
Duration tokens can be applied to notes, rests, and transitions.
* `2` — Fraction of a whole note (e.g., half note).
* `.` — Appended to a fraction to extend it by 50% (dotted note).
* `3/4` — Explicit fractional duration.
* `(0.5)` — Absolute time in seconds.
* `0.22` — Absolute tempo units (relative to current BPM).
* `[0.5]` — Fraction of the current note's length (used only in instrument settings).

### Ramps & Curves
Ramps govern how values transition over time (tempo, volume, and envelopes). They are defined by a Type, a Duration, and an optional Shape Factor. Parameters are separated by commas. Ramps are mirrored in the temporal domain if used as a falloff.

**Ramp Types:**
* `none`: Direct transition.
* `linear`: Standard linear transition.
* `exp` (Exponential): Starts slowly but aggressively ramps to the target (can cause clicks).
* `sqrt` (Square Root): Starts aggressively but slows down near the target.
* `log` (Logarithmic): Like square root, but more aggressive.
* `s` (SCurve): Soft start and stop for the smoothest transition.

If the type is not `none`, a duration must be specified. If the type is not `linear`, a shape factor (0 = near linear, 1 = extreme curve) can be specified.

---

## 4. Instruments

Instruments are defined in the header and can be assigned inline using `@ID`. Each instrument has a waveform type and optional envelope parameters. A custom name can be specified in `()` immediately after the type for readability. All ramps default to S curve with a shape factor of 0.5, if only the duration is specified. None can also be used without `{}` for attack, decay, and release to set the ramp to be none. Example:
```text
@1 = sine (Flute) attack=none, decay=none, sustain=0

### Sine
A pure sine wave generator. With a smooth release, it mimics a flute.
* **`attack`**: Controls how aggressively the note's volume increases (`none` can cause clicks). Specified as a ramp.
* **`decay`**: Controls how quickly the sound fades after the attack phase (`none` sustains the note at full volume; other types cause it to fade before the note ends). Specified as a ramp.
* **`sustain`**: The volume multiplier (0.0 to 1.0) held after the attack and decay phases.
* **`release`**: Controls how quickly the sound fades after the sustain phase. Triggered by the `Q` quantization value (`none` abruptly ends the note). Specified as a ramp.

**Example Definition:**
```text
@1 = sine(Guitar), attack=64, decay={linear, 2/1}, sustain=0, release={exp, 4}
```
*/