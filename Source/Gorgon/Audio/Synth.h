#pragma once

#include "../Containers/Collection.h"
#include "../Containers/Wave.h"
#include "../Enum.h"
#include "../String.h"
#include "../String.h"
#include "../TMP.h"

#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
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
     * and multiple tracks.
     * 
     * While Synth can be used by multiple threads, most functions are forced to acquire a lock
     * to protect the internal state of the synthesizer, so it is recommended to create separate
     * Synth instances for each thread if concurrent rendering is needed.
     *
     * For multi-track rendering, using Normalize on the rendered wave is recommended to avoid
     * clipping, as the current implementation simply sums the samples of each track without any
     * dynamic range management. However, single track rendering should not require normalization
     * as default instruments are designed to avoid clipping when played at full volume.
     *
     * @see \ref gmm_syntax "GMM syntax"
     */
    class Synth {
    public:
        /// A parsed note duration
        struct Duration {
            /// The type of duration (fraction, units, or seconds).
            enum {
                // Only used in state to indicate default should be used
                None,
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
            double ToSamples(float tempo, unsigned int sample_rate) const;

            /// Converts this duration to seconds based on the given tempo (BPM) and note length.
            float ToSeconds(float tempo, float notelength) const;

            /// Converts this duration to number of samples based on the given tempo, sample rate, and note length.
            double ToSamples(float tempo, unsigned int sample_rate, float notelength) const;

            /// Returns the first non-empty duration between this and another duration. If both are empty, returns an empty duration.
            Duration Or(const Duration& other) const {
                if(type != None) return *this;
                return other;
            }

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

            static Duration Empty() {
                Duration d;
                d.type = None;
                return d;
            }

            static Duration Parse(const std::string_view& token, bool allow_empty = false);
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
            Duration Separation = Duration::Empty();
            size_t InstrumentIndex = 1;
        };

    public:
        /// Defines type of a tag. This allows for categorization and filtering
        /// of songs based on their tags, which can be used for music selection 
        /// in games or other applications. Tags are case insensitive and should
        /// be used consistently for effective categorization. Prefer shorter
        /// tags (e.g., rock instead of rock and roll) for better readability and 
        /// easier filtering.
        enum TagType {
            /// Do not use this tag for categorization, it is used in search
            /// to match any tag type.
            Any,
            /// Musical genre, such as rock, jazz, classical, etc. 
            /// Subgenres can be specified along with the main genre.
            Genre,
            /// Mood or emotional tone of the music, such as happy, sad, energetic, calm, etc.
            Mood,
            /// Theme or subject matter of the music, such as love, adventure, nature, etc.
            Theme,
            /// Geographical or cultural origin of the music, such as African, Asian, European, etc.
            Region,
            /// Historical period or era of the music, such as Baroque, Classical, Romantic, etc.
            Era,
            /// Custom tag for user-defined categorization.
            Custom
        };

        /// A tag is a simple string label that can be attached to a song for categorization
        /// and filtering purposes. Each tag has a type that defines the category of the tag
        /// (e.g., genre, mood, etc.) and a value that is the actual label (e.g., "rock", "happy", etc.).
        struct Tag {
            TagType Type;
            std::string Value;
        };

        /// Metadata about the song, such as title, artist, album, etc. This is not used for
        /// rendering but can be useful for organizing and displaying information about the 
        /// song in a music player or game menu. Tags can be used for automatic categorization
        /// and filtering of songs based on their attributes. You can check multiple tags at
        /// once using the HasTags function. If tag type is not specified, it defaults to Any.
        /// Specified tag type only affects the immediate value following it. Examples:
        /// ```
        ///  HasTags(Synth::Genre, "rock", "energetic"); // genre is rock, any tag could be energetic
        ///  HasTags(Synth::Genre, "rock", Synth::Mood, "energetic"); // genre is rock and mood is energetic
        ///  HasTags(Synth::Genre, "folk", "irish", "calm");
        ///  HasTags(Synth::MetaData::Or("classic", "orchestral", "piano"), "happy");
        ///  HasTags("happy", Synth::Genre, Synth::MetaData::Not("rock"));
        ///  HasTags(Synth::MetaData::Or("happy", std::make_pair(Synth::Genre, Synth::MetaData::Not("blues"))));
        /// ```
        struct MetaData {

            std::string Title;
            std::string Artist;
            std::string Arrangement;
            std::string Album;
            std::string Comment;

            /// Tags contained in this metadata. Use HasTag and HasTags functions to check for specific tags.
            std::vector<Tag> Tags;

            bool HasTag(TagType type, const std::string &value) const;

            bool HasTag(const std::string &value) const {
                return HasTag(TagType::Any, value);
            }

            /// Allows using Or operator between tags. For example, MetaData::Or("happy", "joyful") creates
            /// a tuple that can be used in HasTags to check if either "happy" or "joyful" tag is present.
            /// The tag type before a tuple will apply to all values in the tuple. However, you can use
            /// a pair to override the type for specific values within the tuple. For example, 
            /// MetaData::Or(std::make_pair(Synth::Genre, "rock"), std::make_pair(Synth::Mood, "happy")) 
            /// creates a tuple that checks for genre rock or mood happy, regardless of the tag type specified 
            /// before the tuple.
            template<typename ... R_>
            static auto Or(R_ && ... args) {
                return std::tuple<R_...>(std::forward<R_>(args)...);
            }

            /// This can be used to invert the logic of a check. 
            struct Not {
                std::string Value;

                explicit Not(std::string value) : Value(std::move(value)) {}
            };

            /// Checks tags supplied in a vector. All these tags should exist for this function
            /// to return true. This will consider all tags, regardless of their type.
            bool HasTags(const std::vector<std::string> &values) const;

            /// Checks tags supplied in a vector of pairs, where the first element of the pair
            /// is the tag type and the second element is the tag value. All these tags should exist
            /// for this function to return true.
            bool HasTags(const std::vector<std::pair<TagType, std::string>> &values) const;
        
            /// Checks tags supplied in a variadic list of arguments. Arguments can be strings, 
            /// pairs, tuples, or Not structures. See MetaData documentation for examples.
            bool HasTags() const { return true; }

            /// Checks tags supplied in a variadic list of arguments. Arguments can be strings, 
            /// pairs, tuples, or Not structures. See MetaData documentation for examples.
            template<typename First, typename... Rest>
            bool HasTags(First&& first, Rest&&... rest) const {
                using T = std::decay_t<First>;

                if constexpr (std::is_same_v<T, TagType>) {
                    return ApplyToNext(first, std::forward<Rest>(rest)...);
                }
                else {
                    return CheckItem(TagType::Any, std::forward<First>(first)) 
                        && HasTags(std::forward<Rest>(rest)...);
                }
            }
        private:
            template<typename T>
            bool CheckItem(TagType type, const T& item) const {
                if constexpr (TMP::IsTupleV<std::decay_t<T>>) {
                    return std::apply([&](auto&&... args) {
                        return (CheckItem(type, args) || ...);
                    }, item);
                } 
                else if constexpr (TMP::IsPairV<std::decay_t<T>>) {
                    return HasTag(item.first, item.second);
                }
                else if constexpr (std::is_same_v<T, Not>) {
                    return !HasTag(type, item.Value);
                }
                else {
                    return HasTag(type, item);
                }
            }

            template<typename Next, typename... Remaining>
            bool ApplyToNext(TagType type, Next&& next, Remaining&&... remaining) const {
                return CheckItem(type, std::forward<Next>(next)) 
                    && HasTags(std::forward<Remaining>(remaining)...);
            }
        };

        /// Defines ramp types for volume or pitch changes.
        enum class RampType {
            /// No ramp, instant change
            None,
            /// Linear ramp, constant rate of change, ShapeFactor is ignored.
            Linear,
            /// Exponential ramp, rate of change increases or decreases exponentially based on ShapeFactor.
            /// ShapeFactor of 0 results in a linear ramp, while higher values create a more pronounced curve.
            Exponential,
            /// Square root ramp, rate of change follows a square root curve
            SquareRoot,
            Logarithmic,
            SCurve
        };

        //TODO: Add support for center point for S curve.

        /// Defines a ramp curve for a slide or volume change.
        struct Ramp {
            RampType Type = RampType::Linear;

            /// Duration of the ramp in seconds.
            Duration Span = Duration::FromFraction(16);

            /// Shape factor for the ramp curve. Controls the steepness or 
            /// curvature of the ramp. None and Linear ramps ignore this
            /// value. ShapeFactor of 0 creates a linear ramp while a value
            /// close to 1 creates a more pronounced curve. The value must
            /// be in the range [0, 1), avoid getting too close to 1.
            float ShapeFactor = 0.5f;

            /// Parses a ramp definition from a string. The format is defined as follows:
            /// arguments are separated by commas, and the first argument is the ramp type 
            /// (e.g., "Linear", "Exponential", etc.). The second argument is the duration of 
            /// the ramp (e.g. 16 for a sixteenth note, or (0.5) for half a second). The
            /// third argument is optional and defines the shape factor for the ramp curve 
            /// (default is 0.5 for a standard curve). For None, only the first argument is
            /// valid, others will throw. For linear, Duration is required but ShapeFactor
            /// should not be specified. For other types, Duration is required and ShapeFactor 
            /// is optional.
            static Ramp Parse(const std::string_view& token);

            /// Returns the multiplier for the ramp at a given time. Distance is the number
            /// of samples since the start of the ramp (or to the end if it's a decay).
            float GetMultiplier(size_t distance, float tempo, unsigned int sample_rate, float note_length) const;

            /// Returns the multiplier for the ramp at a given fraction. Distance is the number
            /// of samples since the start of the ramp (or to the end if it's a decay).
            float GetMultiplier(float t) const;

            /// Converts the ramp span to number of samples based on the given tempo and sample rate.
            double ToSamples(float tempo, unsigned int sample_rate, float note_length) const;
        };

        
        struct Node;

        /** Instrument is an abstract base class for different types of
         *  synthesizer voices. Each instrument defines how to render a note
         *  with specific settings (e.g., a sine wave with an attack and decay).
         */
        class Instrument {
        public:

            using Factory = std::function<Instrument&()>;

            virtual ~Instrument() = default;

            /// Creates a new instance of the instrument with the same settings.
            virtual Instrument &Clone() const = 0;

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
                TrackState &state, unsigned int sample_rate
            ) = 0;

            /// Called after the last note of the track is rendered, allowing the instrument to render any additional
            /// release tail if necessary. 
            virtual void RenderTheEnd(Containers::Wave &wave, TrackState &state, unsigned int sample_rate) {
                // By default, instruments have no special handling for the end of the track,
                // but this can be overridden by specific instruments if necessary (e.g., to render
                // release tails that extend beyond the last note).
            }

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
            virtual double ReleaseOverflow(TrackState &state, unsigned int sample_rate, const Node &note) const = 0;

            /// Resets any internal state of the instrument that should not carry over between renders.
            virtual void Reset() {
                // By default, instruments have no internal state that needs resetting between notes,
                // but this can be overridden by specific instruments if necessary (e.g., for a noise generator).
            }

            /// Returns the name of the instrument.
            std::string GetInstrumentName() const {
                return Name;
            }

            /// A unique name for the instrument, used for referencing in GMM.
            std::string Name;

            /// Optional description of the instrument, can be used for documentation.
            std::string Description;
        };

        /** A simple sine wave instrument with configurable attack and release ramps.
         *  This is the default instrument for all notes unless changed by an @ command.
         */
        class Sine : public Instrument {
        public:
            Sine() {
                Name = "Sine";
                Attack.Span = Duration::FromFraction(128);
                Release.Span = Duration::FromFraction(16);
                Decay.Span = Duration::FromFraction(16);
            }

            Instrument &Clone() const override {
                return *new Sine(*this);
            }

            double Render(
                Containers::Wave &wave, const Node &node,
                TrackState &state, unsigned int sample_rate
            ) override;

            void LoadSettings(const std::string_view& settings) override;

            double ReleaseOverflow(TrackState &state, unsigned int sample_rate, const Node &note) const override;

            /// Attack and release falloff settings for the sine wave. Attack 
            /// controls how the note volume increases at the start, while release
            /// controls how it decreases at the end. Neither attack nor release
            /// can be more than 20% of the note duration to avoid silent notes.
            Ramp Attack, Release;

            /// This controls how fast the note fades regardless of the note
            /// duration. If set to None, it will not affect the note. If set, 
            /// the note will fall to Sustain level after this amount of time 
            /// has passed.
            Ramp Decay;

            /// Sustain level controls the volume level that the note holds after
            /// the attack and decay phases. It is a multiplier from 0.0 (silent)
            /// to 1.0 (full volume).
            float Sustain = 0.8f;

            /// Scales the track volume. Can be used to avoid clipping when release
            /// overflow is expected.
            float Volume = 0.7f;

            /// Default separation between notes.
            Duration Separation = Duration::FromFraction(32);

            /// Modifies the pitch of the sine wave. A full number represents a semitone.
            /// A fraction can be used for microtonal adjustments, +/-12 can be used for
            /// full octave shifts.
            float PitchOffset = 0.0f;
        };

        class PWM : public Instrument {
        public:
            PWM() {
                Name = "PWM";
            }

            Instrument &Clone() const override {
                return *new PWM(*this);
            }

            void LoadSettings(const std::string_view& settings) override;

            double ReleaseOverflow(TrackState &state, unsigned int sample_rate, const Node &note) const override {
                return Trise * 2.5 * sample_rate;
            }

            double Render(
                Containers::Wave &wave, const Node &node,
                TrackState &state, unsigned int sample_rate
            ) override;

            void RenderTheEnd(Containers::Wave &wave, TrackState &state, unsigned int sample_rate) override;

            void Reset() override {
                current_level = 0.0f;
                phase = 0.0f;
            }

            /// Duty cycle of the pulse wave, from 0.0 (silent) to 1.0 (full square wave). 
            /// Default is 0.5 for a standard square wave.
            float DutyCycle = 0.5f;

            /// Time it takes for the pulse wave to rise to full volume at the start of the note, in seconds.
            /// Larger values create a softer sound.
            float Trise = 0.0002f;

            /// Scales the track volume. Can be used to avoid clipping when multiple tracks
            /// are expected to overlap.
            float Volume = 1.0f;

            /// Default separation between notes.
            Duration Separation = Duration::FromFraction(32);

            /// Modifies the pitch of the sine wave. A full number represents a semitone.
            /// A fraction can be used for microtonal adjustments, +/-12 can be used for
            /// full octave shifts.
            float PitchOffset = 0.0f;

            /// If true, the phase of the waveform will reset to 0 at the start of each note. 
            /// This can be used to create a more percussive sound, while false will create a more legato sound.
            bool ResetPhase = false;

        private:
            float current_level = 0.0f; // for simple one-pole low-pass filter to create a softer sound
            double phase = 0.0f; // current phase of the waveform, from 0.0 to 1.0
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
                InstrumentChange,
                TrackChange
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

            static Node MakeTrackChange(size_t track_index);
        };

        /// A track is a sequence of nodes that are played in order. Multiple tracks can be 
        /// played simultaneously to create polyphony. Each track has its own state 
        /// (e.g., current octave, tempo, etc.) that is modified by the nodes in the track 
        /// and used during rendering.
        struct Track {
            explicit Track(size_t index = 0) : Index(index) { }

            std::vector<Node> Nodes;

            size_t Index = 0;

            auto begin() { return Nodes.begin(); }
            auto end() { return Nodes.end(); }
            auto begin() const { return Nodes.begin(); }
            auto end() const { return Nodes.end(); }
            auto size() const { return Nodes.size(); }
            auto &operator[](size_t index) { return Nodes[index]; }
            const auto &operator[](size_t index) const { return Nodes[index]; }
        };

        /// Parses a GMM string into a sequence of nodes. Throws ParseError on invalid input.
        void Parse(const std::string_view& gmm) {
            std::istringstream stream(std::string{gmm});
            Parse(stream);
        }

        /// Parses a GMM string from an input stream. Throws ParseError on invalid input.
        void Parse(std::istream &stream);

        float CalculateDuration() const;

        AudioDuration CalculateSamples(unsigned int sample_rate = 44100u) const;

        Containers::Wave Render(unsigned int sample_rate = 44100u) const;

        /// Parses a single GMM token into a Node. Throws ParseError on invalid input. Cannot
        /// process comments.
        static Node ParseNode(const std::string_view& token, int channels);

        Node ParseNode(const std::string_view& token) const {
            return ParseNode(token, (int)channels.size());
        }

        /// Registers a new instrument factory function with the given name. The factory
        /// should return a reference to a new instance of the instrument when called. 
        /// This will replace any existing factory with the same name. Instrument names
        /// are case-insensitive. Factory generated objects will be owned and managed by the
        /// Synth class, and will be deleted when replaced or when the Synth is destroyed.
        void AddInstrumentFactory(Instrument::Factory factory, const std::string &name);

        /// Convenience overload that takes an instrument instance instead of a factory function. 
        /// The instrument will be cloned using its Clone method to create new instances when needed. 
        /// This is useful for derived instruments that have specific settings.
        void AddInstrumentFactory(const Instrument &instrument, const std::string &name) {
            AddInstrumentFactory([&instrument]() -> Instrument& { return instrument.Clone(); }, name);
        }

        /// Removes the instrument factory with the given name. If no such factory exists, 
        /// this function does nothing.
        void RemoveInstrumentFactory(const std::string &name);

        /// Removes all registered instrument factories and resets to only the default sine wave.
        void ClearInstrumentFactories();

        /// Checks if an instrument factory with the given name exists.
        bool HasInstrumentFactory(const std::string& name) const {
            return 
                findinstrument(name, instrumentfactories).has_value() ||
                findinstrument(name, baseinstrumentfactories).has_value()
            ;
        }

        /// Returns a list of all system instrument names.
        static std::vector<std::string> GetInstrumentRegistry();

        /// Returns a new instance of the instrument with the given name. If no such instrument exists, returns nullptr.
        static std::unique_ptr<Instrument> CreateRegistryInstrument(const std::string& name) {
            auto type = String::ToLower(name);

            if(auto it = findinstrument(name, baseinstrumentfactories); it.has_value()) {
                return std::unique_ptr<Instrument>(&it.value()());
            }
            else {
                return {};
            }
        }

        /// Returns a new instance of the instrument with the given name. If no such instrument exists, returns nullptr.
        std::unique_ptr<Instrument> CreateInstrument(const std::string& name) {
            auto type = String::ToLower(name);

            if(auto it = findinstrument(name, instrumentfactories); it.has_value()) {
                return std::unique_ptr<Instrument>(&it.value()());
            }
            else if(auto it = findinstrument(name, baseinstrumentfactories); it.has_value()) {
                return std::unique_ptr<Instrument>(&it.value()());
            }
            else {
                return {};
            }
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
            return channels;
        }

        /// Appends a node to the end of a track.
        void AddNode(Node node, size_t track = 1);

        /// Returns a const reference to the node at the given index.
        const Node& GetNode(size_t index, size_t track = 1) const {
            return tracks[track][index];
        }

        /// Returns the number of nodes in the track.
        size_t GetNodeCount(size_t track = 1) const {
            return tracks[track].size();
        }

        /// Returns the number of actual tracks in the synthesizer.
        /// Track 0 is reserved for global settings and is not counted
        /// in this total. Therefore, actual track indices start from 1.
        size_t GetTrackCount() const {
            return tracks.size() - 1;
        }

        /// Removes the node at the given index.
        void RemoveNode(size_t index, size_t track = 1);

        /// Clears all nodes from the track, resetting it to an empty state.
        void Clear(size_t track = 1) {
            auto guard = std::lock_guard(critical);

            tracks[track].Nodes.clear();
        }

        /// Clears all nodes from all tracks.
        void ClearAll() {
            auto guard = std::lock_guard(critical);

            for(auto& track : tracks) {
                track.Nodes.clear();
            }
        }

        /// Resets the synthesizer to its initial state, ready to parse a new GMM string. 
        void Reset() {
            auto guard = std::lock_guard(critical);

            tracks = {Track()};
            channels = {Audio::Channel::Mono};

            instruments.DeleteAll();
            instruments.Add(new Sine());
        }

        /// Converts a note and octave into frequency (Hz).
        static float NoteToFrequency(Note note, int octave, float pitch_offset = 0.0f) {
            return 440.0f * std::pow(2.0f, (static_cast<int>(note) + (octave - 4) * 12 - 9 + pitch_offset) / 12.0f);
        }

    private:
        using FactoryEntry = std::pair<std::string, Instrument::Factory>;
        
        /// Internal function to calculate total data samples and music end samples.
        /// Total data includes release overflow, while music end is the point at which
        /// the last note ends without overflow. This is used to determine the actual length 
        /// of the track to have gapless looping or chaining.
        std::pair<double, double> calculatesamples(unsigned int sample_rate) const;

        /// List of tracks
        std::vector<Track> tracks = {Track()};

        std::vector<Audio::Channel> channels = {Audio::Channel::Mono};

        /// Factory functions for creating instruments by name. This allows for dynamic
        /// registration of new instrument types without modifying the Synth class.
        std::vector<FactoryEntry> instrumentfactories = {};

        /// Base factory functions for creating instruments by name. This is used as
        /// fallback when the requested instrument factory is not found in the current
        /// instrumentfactories map.
        static const std::vector<FactoryEntry> baseinstrumentfactories;

        static std::optional<Instrument::Factory> findinstrument(const std::string &name, const std::vector<FactoryEntry> &factories) {
            auto type = String::ToLower(name);

            for(const auto& [key, factory] : factories) {
                if(key == type) {
                    return factory;
                }
                else if(String::Replace(key, " ", "") == type) {
                    return factory;
                }
            }

            return std::nullopt;
        }

        static std::optional<size_t> findinstrumentindex(const std::string &name, const std::vector<FactoryEntry> &factories) {
            auto type = String::ToLower(name);

            size_t index = 0;
            for(const auto& [key, factory] : factories) {
                if(key == type) {
                    return index;
                }
                else if(String::Replace(key, " ", "") == type) {
                    return index;
                }

                index++;
            }

            return std::nullopt;
        }

        /// Map of instrument indices to their definitions. The first instrument 
        /// @1 is used for the first note, @2 for the second, etc. @0 is always
        /// silent. Due to this, instruments are 1-indexed in GMM, but 0-indexed
        /// in the collection.
        Containers::Collection<Instrument> instruments = {
            new Sine()
        };

        mutable std::recursive_mutex critical;
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
