#pragma once

#include "Gorgon/Containers/Collection.h"
#include "Gorgon/Containers/Wave.h"
#include "Gorgon/Enum.h"
#include "Gorgon/String.h"

#include <cmath>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>
#include <string_view>
#include <iosfwd>

namespace Gorgon :: Audio {

    /**
     * A lightweight text-based synthesizer.
     *
     * Synth implements a custom Gorgon Music Macro Language (GMM) that is optimized for
     * defining background music as strings.
     *
     * The format is designed to keep sequences compact and readable by separating
     * instrument declarations (the header) from playback data (the body). A single
     * GMM string can express multiple simultaneous voices, tempo changes, octave shifts,
     * and basic slides.
     * 
     * While Synth can be used by multiple threads, most functions are forced to acquire a lock
     * to protect the internal state of the synthesizer, so it is recommended to create separate
     * Synth instances for each thread if concurrent rendering is needed.
     *
     * @see \ref gmm_syntax "GMM syntax"
     */
    class Synth {
    public:
        /// A parsed note duration
        struct Duration {
            /// The type of duration (fraction, units, or seconds).
            enum {
                TempoFraction,
                TempoUnits,
                ClockSeconds,
                NoteFraction
            } type;

            // The value of the duration, interpreted according to the type.
            union {
                struct { 
                    int Numerator; 
                    int Denominator; 
                } Fraction;

                float Units;
                float Seconds;
            };

            /// Converts this duration to seconds based on the given tempo (BPM).
            float ToSeconds(float tempo) const;

            /// Converts this duration to number of samples based on the given tempo and sample rate.
            double ToSamples(float tempo, float sample_rate) const;

            /// Converts this duration to seconds based on the given tempo (BPM) and note length.
            float ToSeconds(float tempo, float notelength) const;

            /// Converts this duration to number of samples based on the given tempo, sample rate, and note length.
            double ToSamples(float tempo, float sample_rate, float notelength) const;

            /// Factory method for creating duration from a fraction (e.g., 1/4 for a quarter note).
            static Duration FromFraction(int numerator, int denominator);

            /// Factory method for creating duration from a simple denominator (e.g., 4 for a quarter note).
            static Duration FromFraction(int denominator) {
                return FromFraction(1, denominator);
            }

            /// Factory method for creating durations from a number of units 
            // (where 1 unit = a full note).
            static Duration FromUnits(float units);

            /// Factory method for creating durations from a number of seconds.
            static Duration FromSeconds(float seconds);

            /// Factory method for creating durations from a number of milliseconds.
            static Duration FromMillis(float milliseconds) {
                return FromSeconds(milliseconds / 1000.0f);
            }

            /// Factory method for creating durations from a number of fractions
            /// of the current note. This not supported for note durations, but is
            /// useful for ramp spans.
            static Duration FromNoteFraction(float fraction);

            static Duration Parse(const std::string_view& token);
        };

        /// Defines length of the rendered audio.
        struct AudioDuration {
            /// Total duration of the audio in samples including release overflow.
            size_t Total;

            /// Duration of the audio without release overflow (i.e., until the end of the last note).
            /// If the audio is to be looped or gaplessly followed by another track, this is the point 
            /// at which it should start overlaying the remaining samples with the beginning of the 
            /// track.
            size_t End;
        };

    private: 
        
        /// Internal state used during rendering. This keeps track of the current sample position,
        struct TrackState {
            double Sample = 0;
            float Tempo = 120.0f;
            int Octave = 4;
            std::vector<float> Volume;
            Duration Separation = Duration::FromFraction(64);
            size_t InstrumentIndex = 1;
        };

    public:

        /// Defines ramp types for volume or pitch changes.
        enum class RampType {
            None,
            Linear,
            Exponential,
            SquareRoot,
            Logarithmic,
            SCurve
        };

        /// Defines a ramp curve for a slide or volume change.
        struct Ramp {
            RampType Type = RampType::Linear;

            /// Duration of the ramp in seconds.
            Duration Span = Duration::FromFraction(16);

            /// Shape factor for the ramp curve. Controls the steepness or 
            /// curvature of the ramp. None and Linear ramps ignore this
            /// value.
            float ShapeFactor = 0.5f;

            /// Parses a ramp definition from a string. The format is defined as follows:
            /// arguments are separated by commas, and the first argument is the ramp type 
            /// (e.g., "Linear", "Exponential", etc.). The second argument is the duration of 
            /// the ramp (e.g. 16 for a sixteenth note, or (0.5) for half a second). The
            /// third argument is optional and defines the shape factor for the ramp curve 
            /// (default is 1.0 for a standard curve). For None, only the first argument is
            /// valid, others will throw. For linear, Duration is required but ShapeFactor
            /// should not be specified. For other types, Duration is required and ShapeFactor 
            /// is optional.
            static Ramp Parse(const std::string_view& token);

            /// Returns the multiplier for the ramp at a given time. Distance is the number
            /// of samples since the start of the ramp (or to the end if it's a decay).
            float GetMultiplier(size_t distance, unsigned sample_rate) const;
        };

        struct Node;

        /** Instrument is an abstract base class for different types of
         *  synthesizer voices. Each instrument defines how to render a note
         *  with specific settings (e.g., a sine wave with an attack and decay).
         */
        class Instrument {
        public:
            virtual ~Instrument() = default;

            /// Renders a note into the given wave buffer based on the current
            /// track state, sample rate, and note duration. The track state includes
            /// information about the current tempo, octave, and volume levels.
            /// Should return the number of samples rendered for this note (not including 
            /// release overflow). State can be modified, but the sample advance should not 
            /// be applied by the instrument itself, unless it needs to be modified beyond 
            /// the note duration. Node could be any node, Render is expected to return
            /// 0 if node is not a note, however, if necessary, it can modify the state.
            /// Octave, tempo, volume and separation changes will be handled by the caller before the
            /// call to Render, so the instrument can assume that the state is already updated
            /// but could override it if necessary.
            virtual double Render(
                Containers::Wave &wave, const Node &node, 
                TrackState &state, float sample_rate
            ) = 0;

            /// Loads instrument settings from a string. The format of the settings string
            /// is defined by each specific instrument type. This allows for flexible
            /// configuration of different instruments (e.g., setting the rise and fall times
            /// for a sine wave).
            virtual void LoadSettings(const std::string_view& settings) {
                if(!settings.empty()) {
                    throw Error(Error::InvalidParameter, "This instrument does not accept settings: " + std::string{settings});
                }
            }

            /// This function returns how much a given note's release phase would overflow
            /// into the next note based on the current track state and sample rate. This is used to
            /// determine unclipped length of the entire track.
            virtual double ReleaseOverflow(TrackState &state, float sample_rate, const Node &note) const = 0;

            /// Returns the name of the instrument.
            std::string GetInstrumentName() const {
                return Name;
            }

            std::string Name;
        };

        /** A simple sine wave instrument with configurable attack and release ramps.
         *  This is the default instrument for all notes unless changed by an @ command.
         */
        class Sine : public Instrument {
        public:
            Sine() {
                Name = "Sine";
                Attack.Span = Duration::FromFraction(32);
                Release.Span = Duration::FromFraction(16);
            }
            
            double Render(
                Containers::Wave &wave, const Node &node,
                TrackState &state, float sample_rate
            ) override;

            void LoadSettings(const std::string_view& settings) override;

            double ReleaseOverflow(TrackState &state, float sample_rate, const Node &note) const override;

            /// Attack and release falloff settings for the sine wave. Attack 
            /// controls how the note volume increases at the start, while release
            /// controls how it decreases at the end. Neither attack nor release
            /// can be more than 20% of the note duration to avoid silent notes.
            Ramp Attack, Release;

            /// This controls how fast the note fades regardless of the note
            /// duration. If set to None, it will not affect the note. If set, 
            /// the note will fall to Sustain level after this amount of time 
            /// has passed.
            Ramp Decay = {RampType::None};

            /// Sustain level controls the volume level that the note holds after
            /// the attack and decay phases. It is a multiplier from 0.0 (silent)
            /// to 1.0 (full volume).
            float Sustain = 1.0f;
        };

        /// If GMM encounters an error, it throws this exception with
        /// details about the error type and the offending token.
        class Error : public std::runtime_error {
        public:
            enum Type {
                InvalidToken,
                MissingParameter,
                InvalidParameter,
                InvalidDuration,
                UnknownInstrument
            };

            Error(Type type, const std::string& token)
                : std::runtime_error("GMM Parse Error: " + token), type(type) 
            { }
            
            Type type;
            size_t position = std::string::npos;
        };

        /// Musical notes (C, C#, D, etc.) mapped to their semitone offsets.
        enum class Note {
            C = 0,
            CS = 1,
            D = 2,
            DS = 3,
            E = 4,
            F = 5,
            FS = 6,
            G = 7,
            GS = 8,
            A = 9,
            AS = 10,
            B = 11
        };

        /// A single node in a track.
        struct Node {
            /// The type of node (note, tempo change, octave shift, etc.).
            enum class Type {
                NoOp,
                Note,
                Tempo,
                OctaveAbsolute,
                OctaveRelative, // increment/decrement from current octave
                Volume,
                Rest,
                Separation, // articulation/separation between notes
                InstrumentChange
            } type = Type::NoOp;

            union {
                struct { 
                    Note note; 
                    Duration duration;
                    bool slide; 
                } note;
                float tempo;
                int octave;
                size_t index;
                Duration duration;
                struct {
                    /// 0 -> all channels
                    int channel;
                    float volume;
                } volume;
            };

            static Node MakeNote(Note note, Duration duration, bool slide);

            static Node MakeRest(Duration duration);

            static Node MakeTempo(float tempo);

            static Node MakeOctaveAbsolute(int octave);

            static Node MakeOctaveRelative(int delta);

            static Node MakeVolume(float volume, int channel = 0);

            static Node MakeSeparation(Duration duration);

            static Node MakeInstrumentChange(size_t instrument_index);
        };

        /// Parses a GMM string into a sequence of nodes. Throws ParseError on invalid input.
        void Parse(const std::string_view& gmm) {
            std::istringstream stream(std::string{gmm});
            Parse(stream);
        }

        /// Parses a GMM string from an input stream. Throws ParseError on invalid input.
        void Parse(std::istream &stream);

        float CalculateDuration() const;

        AudioDuration CalculateSamples(float sample_rate = 44100.0f) const;

        Containers::Wave Render(float sample_rate = 44100.0f) const;

        /// Parses a single GMM token into a Node. Throws ParseError on invalid input. Cannot
        /// process comments.
        static Node ParseNode(const std::string_view& token, int channels);

        Node ParseNode(const std::string_view& token) const {
            return ParseNode(token, (int)Channels.size());
        }

        /// Registers a new instrument factory function with the given name. The factory
        /// should return a reference to a new instance of the instrument when called. 
        /// This will replace any existing factory with the same name. Instrument names
        /// are case-insensitive.
        void AddInstrumentFactory(std::function<Instrument &()> factory, const std::string &name);

        /// Removes the instrument factory with the given name. If no such factory exists, 
        /// this function does nothing.
        void RemoveInstrumentFactory(const std::string &name);

        /// Removes all registered instrument factories and resets to only the default sine wave.
        void ClearInstrumentFactories();

        /// Checks if an instrument factory with the given name exists.
        bool HasInstrumentFactory(const std::string& name) const {
            return instrumentfactories.find(String::ToLower(name)) != instrumentfactories.end();
        }

        /// Adds an instrument to the synthesizer, taking ownership.
        /// Returns the 1-based index of the newly added instrument.
        size_t AddInstrument(Instrument& instrument);

        /// Replaces the instrument at the given 1-based index, taking ownership
        /// and deleting the previous instrument.
        void SetInstrument(size_t index, Instrument& instrument);

        /// Returns a const reference to the instrument at the given 1-based index.
        const Instrument& GetInstrument(size_t index) const;

        /// Returns the number of instruments.
        size_t GetInstrumentCount() const;

        /// Removes and deletes the instrument at the given 1-based index.
        void RemoveInstrument(size_t index);

        /// Adds a channel to the output. Silently ignores if already present.
        /// Channel order is normalized after adding.
        void AddChannel(Audio::Channel channel);

        /// Returns true if the given channel is present in the output configuration.
        bool HasChannel(Audio::Channel channel) const;

        /// Removes the given channel from the output. Silently ignores if not present.
        void RemoveChannel(Audio::Channel channel);

        /// Returns the current channel configuration.
        const std::vector<Audio::Channel>& GetChannels() const {
            return Channels;
        }

        /// Appends a node to the end of the track.
        void AddNode(Node node);

        /// Returns a const reference to the node at the given index.
        const Node& GetNode(size_t index) const {
            return Nodes[index];
        }

        /// Returns the number of nodes in the track.
        size_t GetNodeCount() const {
            return Nodes.size();
        }

        /// Removes the node at the given index.
        void RemoveNode(size_t index);

        /// Clears all nodes from the track, resetting it to an empty state.
        void Clear() {
            auto guard = std::lock_guard(critical);

            Nodes.clear();
        }

        /// Converts a note and octave into frequency (Hz).
        static float NoteToFrequency(Note note, int octave) {
            return 440.0f * std::pow(2.0f, (static_cast<int>(note) + (octave - 4) * 12 - 9) / 12.0f);
        }
    private:
        std::pair<double, double> calculatesamples(float sample_rate) const;

        /// Sequence of nodes that define a track.
        std::vector<Node> Nodes;

        std::vector<Audio::Channel> Channels = {Audio::Channel::Mono};

        /// Factory functions for creating instruments by name. This allows for dynamic
        /// registration of new instrument types without modifying the Synth class.
        std::map<std::string, std::function<Instrument&()>> instrumentfactories = {
            {"sine", []() -> Instrument& { return *new Sine(); }}
        };

        /// Map of instrument indices to their definitions. The first instrument 
        /// @1 is used for the first note, @2 for the second, etc. @0 is always
        /// silent. Due to this, instruments are 1-indexed in GMM, but 0-indexed
        /// in the collection.
        Containers::Collection<Instrument> instruments = {
            new Sine()
        };

        mutable std::mutex critical;
    };

    DefineEnumStringsCM(Synth, RampType, {
        {Synth::RampType::None, "None"},
        {Synth::RampType::Linear, "Linear"},
        {Synth::RampType::Exponential, "Exponential"},
        {Synth::RampType::Exponential, "exp"},
        {Synth::RampType::SquareRoot, "Square root"},
        {Synth::RampType::SquareRoot, "SquareRoot"},
        {Synth::RampType::SquareRoot, "sqrt"},
        {Synth::RampType::Logarithmic, "Logarithmic"},
        {Synth::RampType::Logarithmic, "log"},
        {Synth::RampType::SCurve, "S-curve"},
        {Synth::RampType::SCurve, "SCurve"},
        {Synth::RampType::SCurve, "s"}
    })

}
