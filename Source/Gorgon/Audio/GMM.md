/*! \page gmm_syntax Gorgon Music Macro (GMM) Syntax

Gorgon Music Macro (GMM) is a compact text format for describing polyphonic, retro-style music and sound effects directly as strings. It is designed to be easy to read and write while still supporting advanced features like multiple voices, multi-channel panning, tempo automation, instrument switching, and non-linear parameter ramps.

GMM separates the definition of global settings and instruments (the *header*) from the actual musical sequence (the *body*). This split keeps the playback code free of hard-coded “magic numbers” and makes it easy to reuse instrument settings across multiple tracks.

GMM supports comments through the `#` symbol. Anything following `#` on a line is ignored, allowing you to annotate your music or temporarily disable parts of the sequence. 

**Note: The following features are currently incomplete:**
- Slide
- Tempo and volume ramps
- Tracks
- Vibrato

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
%Title = Adventure Theme
%Artist = Jane Doe

# --- Instrument Bank ---
@1 = sine(Flute), attack={s, 64}, decay={linear, 2/1}, sustain=0, release={exp, 4}
@2 = pulse(Lead), duty=50
@3 = noise(Snare), bitdepth=8

# --- Sequence Data ---
1> T120 @1 C4 D4 E3/4 R4 G2.
2> @2 C2 C2 C2 C2

# Multi-channel panning, volume fades, and non-linear slides
1> V(0)100 V(1)0 C4 V(0)0:4{exp} V(1)100:4{exp} C4^F4{sqrt} V0 R4
```

---

## 2. Core Commands

### Engine Configuration
* **`%KEY = value`**: Global engine configuration. Must be defined in the header.
    * `%CHANNELS = 2` sets the engine to render interleaved stereo audio.
    * `%CHANNELS = [FL, FR]` explicitly specifies channels (See Gorgon::Audio::Channel for details).

### Metadata
Metadata fields are set using `%KEY = value` in the header. They are stored with the audio and can be queried via `Synth::MetaData`. If muliple title, artist, arranger, album, and copyright fields are supplied, the last one takes precedence. Multiple comment fields are concatenated with a newline separator. The rest of the fields are tags and multiple tags of the same type can be specified. Note, many music players cannot handle multiple genres and often use the last one. Add your tags accordingly.

* **`%title`**: The title of the piece.
* **`%artist`** (or **`%author`**): The composer or performer.
* **`%arranger`**: The arranger, if different from the artist.
* **`%album`**: The album or collection name.
* **`%copyright`**: Copyright notice.
* **`%comment`**: Free-form text comment. Multiple `%comment` lines are appended with a newline separator.
* **`%genre`**: Musical genre tag (e.g., `rock`, `jazz`, `ambient`).
* **`%mood`**: Emotional tone tag (e.g., `happy`, `tense`, `relaxed`).
* **`%theme`**: Subject matter tag (e.g., `battle`, `nature`, `love`).
* **`%style`**: Arrangement style tag (e.g., `orchestral`, `chiptune`, `acoustic`).
* **`%region`**: Cultural or geographical origin tag (e.g., `celtic`, `japanese`).
* **`%era`**: Historical period tag (e.g., `baroque`, `80s`, `medieval`).
* **`%custom-tag`**: A user-defined tag.

**Example:**
```gmm
%title = Main Theme
%artist = John Doe
%genre = ambient
%mood = calm
%custom-tag = looping
```

### Track & Playback Control
* **`N>`**: Track identifier. Maps the following sequence to a specific voice (e.g., `1>`, `2>`). Each track is mixed together. Tracks can be split across multiple lines for readability. If track identified is not specified, it defaults to `1>`. Tracks have their own tempo and volume settings, but share the same global engine configuration (e.g., channels) and instrument bank.
* **`T<Value>[:Duration|:{Ramp}]`**: Tempo change in beats per minute. Can be immediate (`T120`) or ramped (`T60:1{s}`). Affects only the current track. If duration is supplied, it is considered a linear ramp.
* **`V[(channel)]<Percent>[:Duration|:{Ramp}]`**: Global track volume (0–100%). Modifies all channels for the current track. If duration is supplied, it is considered a linear ramp. Channel is 1 based, 0 is used for all channels. **Examples:**
```gmm
V(1)50 #sets the volume of the first channel to 50% while leaving the rest unchanged.
V50:1 #sets the volume of all channels to 50% over a 2 second linear ramp.
V(2)0:{exp, 2.5} #fades out the second channel over an exponential ramp with a span of 2.5 full notes.
```

### Musical Notation
* **`A`–`G`**: Musical notes. Can be followed by `+` (sharp) or `-` (flat). If no duration is specified, a quarter note is used.
* **`R<Duration>`**: Rest (silence) for the given duration.
* **`O<Octave>` / `<` / `>`**: Octave control. `O5` sets octave 5. `<` and `>` shift the current octave down or up by one.
* **`~[{Rate, Depth, Delay, End, Curve}]` (Vibrato)**: Applies pitch modulation to the preceding note to simulate a vibrating string or breath.
    * If used alone (`C4~`), it applies the instrument's default vibrato settings.
    * Can be overridden inline with tuple parameters: `C4~{5.0, 0.5, 0}` (5Hz rate, half-semitone depth, no delay).
* **`^` (Slide/Portamento)**: A single command that smoothly glides pitch from a starting note to a target note over a specified duration.
    * **Syntax:** `<StartNote>^<TargetNote><Duration>[{Curve}]`
    * **Example:** `C4^G4:2` slides from C4 to G4 over a half note.
    * **Example:** `C4^C5:4{s}` slides an entire octave over a quarter note using an S-Curve for a smooth takeoff and landing.
* **`S<Duration>`**: Sets note separation (articulation). Controls how long before the end of the note the release phase is triggered. Example: `S8` triggers the release an eighth note before the note ends. This allows for more staccato or legato phrasing by controlling how much of the release ramp is audible. `S` without a duration returns to the default separation defined in the instrument.

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

If the type is not `none`, a duration must be specified. If the type is not `linear`, a shape factor (0 = linear, close to 1 = extreme curve) can be specified. Shape must be in the range [0, 1). Do not use values too close to 1 as they can cause extreme curves that may cause audio artifacts. Where ramps are supported, first the type should be specified, followed by the duration (except for none), and then the shape factor if applicable. These values should be separated by commas. 

---

## 4. Instruments

Instruments are defined in the header and can be assigned inline using `@ID`. Each instrument has a waveform type and optional envelope parameters. A custom name can be specified in `()` immediately after the type for readability. All ramps default to S curve with a shape factor of 0.5, if only the duration is specified. None can also be used without `{}` for attack, decay, and release to set the ramp to be none. Example:
```text
@1 = sine (Flute) attack=none, decay=none, sustain=0
```

### Sine
A pure sine wave generator. Default is a classic synth sound. However, with modifications to the envelope and vibrato settings, it can be used for a wide range of sounds from flute, to bell, to bass.
* **`attack`**: Controls how aggressively the note's volume increases (`none` can cause clicks). Specified as a ramp.
* **`decay`**: Controls how quickly the sound fades after the attack phase (`none` sustains the note at full volume; other types cause it to fade before the note ends). Specified as a ramp.
* **`sustain`**: The volume multiplier (0.0 to 1.0) held after the attack and decay phases.
* **`release`**: Controls how quickly the sound fades after the sustain phase. Triggered by the `S` separation value (`none` abruptly ends the note). Specified as a ramp.
* **`volume`**: Peak output volume as a percentage (0–100). Defaults to 70. You may use this to avoid clipping when using aggressive envelope settings.
* **`separation`**: Default note separation duration for this instrument. Overrides the global `S` command default for notes played with this instrument.
* **`pitch`** (or **`pitchoffset`**): Semitone offset applied to all frequencies. Positive values shift up, negative shift down (e.g., `-12` drops by one octave).

**Example Definition:**
```text
@1 = sine(Guitar), attack=64, decay={linear, 2/1}, sustain=0, release={exp, 4}
@2 = sine(Bass), pitch=-12, volume=80, separation=16
```

**Vibrato Settings**
Instruments can define a default vibrato profile. This allows notes to organically modulate without needing track-level modifiers. It requires three parameters defined as a tuple: `{Rate, Depth, Delay}`.
* **`Rate`**: The speed of the wobble in Hz (e.g., `5.0` to `7.0` is typical for acoustic instruments).
* **`Depth`**: The maximum pitch deviation in semitones (e.g., `0.5` is a quarter-step bend).
* **`Delay`**: Absolute time in seconds (e.g., `(0.2)`) or duration fraction (e.g., `8`) before the vibrato effect begins, mimicking a human player settling on a note before applying vibrato.

**Example Definition:**
```text
@1 = sine(Violin), attack={s, 32}, vibrato={6.0, 0.25, 16}
```

---

### PWM
A pulse-width modulation oscillator. Produces a rectangular wave whose harmonic content is shaped by the duty cycle. It is well suited for chiptune leads, basses, and pseudo-triangle sounds.

* **`duty`** (or **`dutycycle`**): The fraction of each cycle spent high (0.0–1.0). `0.5` is a perfect square wave; smaller values create a thinner, brighter buzz. Defaults to `0.5`.
* **`rise`** (or **`slew`**): Rise/fall time in seconds applied to each edge transition. Reduces aliasing and harsh harmonics. Set to `0` for a hard digital edge. Defaults to `0.0002` (0.2 ms).
* **`volume`**: Peak output volume as a percentage (0–100). Defaults to 100.
* **`separation`**: Default note separation duration. Controls the gap between notes, similar to the `S` command.
* **`pitch`** (or **`pitchoffset`**): Semitone offset applied to all frequencies.
* **`resetphase`**: If `true`, forces the oscillator phase to reset to zero at the start of every note. This guarantees a consistent transient on each note, which is essential for punchy bass sounds. Defaults to `false` (smooth legato transitions). Accepts `true`/`false` or `1`/`0`.

**Example Definitions:**
```text
@1 = pwm(Lead),   duty=0.5,   rise=0.0002
@2 = pwm(Bass),   duty=0.25,  pitch=-12,  resetphase=true
@3 = pwm(Buzz),   duty=0.125, rise=0.0001, volume=60
```

---

### Built-in Named Instruments

These instruments can be referenced by name in the header without specifying individual parameters. Settings can still be overridden after the name. All names are case-insensitive and underscores may be used in place of spaces.

**Sine-based:**

* **synth** — Classic lead/pad synth with a quick attack and moderate decay.
* **flute** — Soft and airy with a gentle S-curve attack and short logarithmic release.
* **chiptune** — Instant attack, short linear decay, zero sustain — retro game music staple.
* **ambient pad** — Very slow attack and long release at low volume for atmospheric textures.
* **electric piano** — S-curve attack with a warm decay and moderate sustain.
* **deep sub bass** — Pitched 2 octaves down for powerful low-end support.
* **xylophone** — Pitched 1 octave up with a crisp attack and long resonant decay.
* **bell** — Pitched 1 octave up with an instant attack and very long logarithmic decay.

**PWM-based:**

* **square lead** — 50% duty cycle square wave, the definitive 8-bit lead.
* **thin pulse** — 12.5% duty cycle — bright and buzzy, ideal for arpeggios and harpsichord textures.
* **punchy bass** — 25% duty cycle dropped one octave with phase reset for a tight, consistent transient.
* **soft triangle** — 50% duty cycle with heavy slew limiting (8 ms) that rounds the wave into a warm triangle-like tone.

**Example using a named instrument:**
```text
@1 = flute
@2 = punchy bass
@3 = bell, volume=50
```
*/