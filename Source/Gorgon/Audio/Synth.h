#pragma once

#include "Gorgon/Containers/Wave.h"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <string_view>
#include <iosfwd>

namespace Gorgon :: Audio {

    /**
     * A lightweight text-based synthesizer for the Gorgon engine.
     *
     * Synth implements a small custom Music Macro Language (GMM) that is optimized for
     * defining retro-style background music and simple sound effects as strings.
     *
     * The format is designed to keep sequences compact and readable by separating
     * instrument declarations (the header) from playback data (the body). A single
     * GMM string can express multiple simultaneous voices, tempo changes, octave shifts,
     * and basic slides.
     *
     * @see \ref gmm_syntax "GMM syntax"
     */
    class Synth {
    public:

        class ParseError : public std::runtime_error {
        public:
            enum Type {
                InvalidToken,
                MissingParameter,
                InvalidParameter
            };

            ParseError(Type type, const std::string& token)
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

        /// A parsed note duration
        struct Duration {
            /// The type of duration (fraction, units, or seconds).
            enum {
                Fraction,
                Units,
                Seconds
            } type;

            // The value of the duration, interpreted according to the type.
            union {
                struct { 
                    int numerator; 
                    int denominator; 
                } fraction;

                float units;
                float seconds;
            };

            /// Converts this duration to seconds based on the given tempo (BPM).
            float ToSeconds(float tempo) const;

            /// Converts this duration to number of samples based on the given tempo and sample rate.
            size_t ToSamples(float tempo, float sample_rate) const;

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

            static Duration Parse(const std::string_view& token);
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
                Rest
            } type = Type::NoOp;

            union {
                struct { 
                    Note note; 
                    Duration duration;
                    bool slide; 
                } note;
                float tempo;
                int octave;
                float volume;
            };

            static Node MakeNote(Note note, Duration duration, bool slide);

            static Node MakeRest(Duration duration);

            static Node MakeTempo(float tempo);

            static Node MakeOctaveAbsolute(int octave);

            static Node MakeOctaveRelative(int delta);

            static Node MakeVolume(float volume);
        };

        /// Parses a GMM string into a sequence of nodes. Throws ParseError on invalid input.
        void Parse(const std::string_view& gmm) {
            std::istringstream stream(std::string{gmm});
            Parse(stream);
        }

        /// Parses a GMM string from an input stream. Throws ParseError on invalid input.
        void Parse(std::istream &stream);

        float CalculateTotalDuration() const;

        size_t CalculateTotalSamples(float sample_rate = 44100.0f) const;

        Containers::Wave Render(float sample_rate = 44100.0f) const;

        /// Parses a single GMM token into a Node. Throws ParseError on invalid input. Cannot
        /// process comments.
        static Node ParseNode(const std::string_view& token);

        /// Converts a note and octave into frequency (Hz).
        static float NoteToFrequency(Note note, int octave) {
            return 440.0f * std::pow(2.0f, (static_cast<int>(note) + (octave - 4) * 12 - 9) / 12.0f);
        }

        /// Sequence of nodes that define a track.
        std::vector<Node> Nodes;

        std::vector<Audio::Channel> Channels = {Audio::Channel::Mono};

    private:
        struct TrackState {
            float Time = 0.0f;
            float Tempo = 120.0f;
            int Octave = 4;
            float Volume = 1.0f;
        };
    };

}