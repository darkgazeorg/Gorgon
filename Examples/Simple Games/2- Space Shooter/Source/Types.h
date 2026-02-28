#pragma once

// =============================================================================
// Types.h
// -----------------------------------------------------------------------------
// This file is a shared "toolkit" for the whole project. Instead of repeating
// common type names and small helper functions in every file, we define them
// once here and include this header wherever needed.
//
// Think of it like a shared library inside our own game: anything that several
// different files need can live here.
// =============================================================================

#include <Gorgon/Geometry/Point.h>

// The engine uses its own Point types inside a namespace.
// Writing "Gorgon::Geometry::Point" every time would be very verbose, so we
// bring them into the global scope with "using".
// Point  -> integer coordinates  (good for screen pixel positions)
// Pointf -> float coordinates    (good for physics / smooth movement)
using Gorgon::Geometry::Point;
using Gorgon::Geometry::Pointf;

// A small utility that returns a random floating-point number between min and
// max. We use it whenever we need some randomness, e.g. where a new asteroid
// should appear and how fast it should move.
//
// How it works:
//   rand() gives a random integer from 0 to RAND_MAX.
//   Dividing by RAND_MAX scales that to a 0..1 range.
//   Multiplying by (max - min) scales it to a 0..(max-min) range.
//   Adding min shifts it to the min..max range.
// We should probably use a better random generator for a real game, 
// but this is simple and good enough for our purposes.
inline float RandomFloat(float min, float max) {
    return (max - min) * rand() / RAND_MAX + min;
}
