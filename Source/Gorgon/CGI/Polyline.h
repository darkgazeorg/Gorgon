#pragma once

#include "../Geometry/Bounds.h"
#include "../Types.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Gorgon ::CGI {

/// A single vertex in a bulge-encoded polyline.
///   - bulge != 0  -> the segment is a circular arc; bulge = tan(sweep/4),
///                    positive = CCW, negative = CW.
///   - isCurve     -> the segment is the destination of a flattened curve run
struct PolyVertex {
  Float x = 0.0;
  Float y = 0.0;
  Float bulge = 0.0;
  bool isCurve = false;
};

/// A closed or open polyline made of PolyVertex entries.
struct Polyline {
  bool isClosed = false;
  bool isNegative = false;
  bool closingCurve = false;
  std::vector<PolyVertex> vertexes;
};

/// Full geometric description of a single circular arc recovered from a
/// bulge-encoded segment.
struct ArcGeometry {
  Float cx = 0.0, cy = 0.0;
  Float radius = 0.0, startAngle = 0.0, endAngle = 0.0;
  Float sweep = 0.0, midX = 0.0, midY = 0.0;
};

/// A typed, standalone segment extracted from a Polyline.
/// Arc geometry can be retrieved via ComputeArcGeometry().
struct Segment {
  /// Segment primitive type.
  enum class Type { Line, Arc };

  Type type = Type::Line;
  Float x0 = 0.0, y0 = 0.0;
  Float x1 = 0.0, y1 = 0.0;
  Float bulge = 0.0;
  bool collapsedArc = false;
  bool isCurve = false;

  int id = -1;
  int polyId = -1;
};

/// Recover full arc geometry from a bulge-encoded segment.
/// @param x0 Start X coordinate.
/// @param y0 Start Y coordinate.
/// @param x1 End X coordinate.
/// @param y1 End Y coordinate.
/// @param bulge Bulge value where bulge = tan(sweep/4).
/// @param[out] out Output arc geometry on success.
/// @return True when a valid non-degenerate arc can be recovered.
inline bool ComputeArcGeometry(Float x0, Float y0, Float x1, Float y1,
                               Float bulge, ArcGeometry &out) {
  if (std::abs(bulge) < static_cast<Float>(1e-15))
    return false;
  const Float dx = x1 - x0, dy = y1 - y0;
  const Float chord = std::sqrt(dx * dx + dy * dy);
  if (chord < static_cast<Float>(1e-12))
    return false;

  out.sweep = static_cast<Float>(4.0) * std::atan(bulge);
  const Float halfSweep = out.sweep / static_cast<Float>(2.0);
  const Float sinHalf = std::sin(halfSweep);
  if (std::abs(sinHalf) < static_cast<Float>(1e-12))
    return false;

  // Signed radius: positive = CCW, negative = CW
  const Float rSigned = (chord / static_cast<Float>(2.0)) / sinHalf;
  out.radius = std::abs(rSigned);

  // Centre: midpoint of chord displaced along left unit-perpendicular
  const Float midX = (x0 + x1) / static_cast<Float>(2.0);
  const Float midY = (y0 + y1) / static_cast<Float>(2.0);
  const Float perpX = -dy / chord;
  const Float perpY = dx / chord;
  const Float dist = rSigned * std::cos(halfSweep);

  out.cx = midX + perpX * dist;
  out.cy = midY + perpY * dist;

  out.startAngle = std::atan2(y0 - out.cy, x0 - out.cx);
  out.endAngle = std::atan2(y1 - out.cy, x1 - out.cx);

  const Float midAngle = out.startAngle + halfSweep;
  out.midX = out.cx + out.radius * std::cos(midAngle);
  out.midY = out.cy + out.radius * std::sin(midAngle);
  return true;
}

/// Compute signed area for a polyline, including bulge arc contributions.
/// @param poly Polyline to evaluate.
/// @return Positive for CCW winding, negative for CW winding, zero for
/// degenerate shapes.
inline Float SignedArea(const Polyline &poly) {
  const int n = (int)poly.vertexes.size();
  const int segments = poly.isClosed ? n : n - 1;
  if (n < 2 || segments <= 0)
    return 0.0;

  Float area = 0.0;
  for (int i = 0; i < segments; ++i) {
    const auto &v0 = poly.vertexes[i];
    const auto &v1 = poly.vertexes[(i + 1) % n];
    area += v0.x * v1.y - v1.x * v0.y;
    if (v0.bulge != 0.0) {
      const Float dx = v1.x - v0.x, dy = v1.y - v0.y;
      const Float chord2 = dx * dx + dy * dy;
      const Float sweep = static_cast<Float>(4.0) * std::atan(v0.bulge);
      const Float sinHalf = std::sin(sweep / static_cast<Float>(2.0));
      if (std::abs(sinHalf) > static_cast<Float>(1e-12)) {
        const Float r2 = chord2 / (static_cast<Float>(4.0) * sinHalf * sinHalf);
        area += r2 * (sweep - std::sin(sweep));
      }
    }
  }
  return area / static_cast<Float>(2.0);
}

/// Test whether a polyline has counter-clockwise winding.
/// @param poly Polyline to evaluate.
/// @return True when signed area is positive.
inline bool IsCounterClockwise(const Polyline &poly) {
  return SignedArea(poly) > 0.0;
}
/// Test whether a polyline has clockwise winding.
/// @param poly Polyline to evaluate.
/// @return True when signed area is negative.
inline bool IsClockwise(const Polyline &poly) { return SignedArea(poly) < 0.0; }

/// Reverse polyline winding and remap segment attributes.
/// @param poly Polyline to reverse in place.
inline void ReversePolyline(Polyline &poly) {
  const int n = (int)poly.vertexes.size();
  const int segments = poly.isClosed ? n : n - 1;
  if (n < 2)
    return;

  std::vector<Float> segBulge(segments);
  std::vector<bool> segCurve(segments);
  for (int i = 0; i < segments; ++i) {
    segBulge[i] = poly.vertexes[i].bulge;
    segCurve[i] = poly.vertexes[(i + 1) % n].isCurve;
  }

  std::reverse(poly.vertexes.begin(), poly.vertexes.end());
  for (auto &v : poly.vertexes) {
    v.bulge = 0.0;
    v.isCurve = false;
  }

  for (int j = 0; j < segments; ++j) {
    const int orig = (segments - 2 - j + segments) % segments;
    poly.vertexes[j].bulge = -segBulge[orig];
    poly.vertexes[(j + 1) % n].isCurve = segCurve[orig];
  }
  poly.isNegative = !poly.isNegative;
}

namespace detail {

/// Test whether an angle lies on the directed arc sweep.
/// @param a Angle to test, in radians.
/// @param start Arc start angle, in radians.
/// @param sweep Signed arc sweep, in radians.
/// @return True when @p a is inside the swept interval.
inline bool AngleInArcSweep(Float a, Float start, Float sweep) {
  Float rel = a - start;
  if (sweep > 0.0) {
    while (rel <= 0.0)
      rel += static_cast<Float>(2.0 * Gorgon::PI);
    while (rel > static_cast<Float>(2.0 * Gorgon::PI))
      rel -= static_cast<Float>(2.0 * Gorgon::PI);
    return rel < sweep;
  } else {
  } else {
    while (rel >= 0.0)
      rel -= static_cast<Float>(2.0 * Gorgon::PI);
    while (rel < static_cast<Float>(-2.0 * Gorgon::PI))
      rel += static_cast<Float>(2.0 * Gorgon::PI);
    return rel > sweep;
  }
  }
}

} // namespace detail

/// Compute a tight axis-aligned bounding box for a polyline.
/// @param poly Polyline to bound.
/// @return Bounding box that includes line and arc extrema.
inline Geometry::Boundsf PolylineBounds(const Polyline &poly) {
  Geometry::Boundsf bb(0.0, 0.0, 0.0, 0.0);
  bool hasBounds = false;
  auto expand = [&](Float x, Float y) {
    if (!hasBounds) {
      bb.Left = bb.Right = x;
      bb.Top = bb.Bottom = y;
      hasBounds = true;
      return;
    }
    if (x < bb.Left)
      bb.Left = x;
    if (y < bb.Top)
      bb.Top = y;
    if (x > bb.Right)
      bb.Right = x;
    if (y > bb.Bottom)
      bb.Bottom = y;
  };

  const int n = (int)poly.vertexes.size();
  const int segments = poly.isClosed ? n : n - 1;
  if (n == 0)
    return bb;

  for (const auto &v : poly.vertexes)
    expand(v.x, v.y);

  static const Float cardinals[4] = {static_cast<Float>(0.0),
                                     static_cast<Float>(Gorgon::PI / 2.0),
                                     static_cast<Float>(Gorgon::PI),
                                     static_cast<Float>(3.0 * Gorgon::PI / 2.0)};
  for (int i = 0; i < segments; ++i) {
    const auto &v0 = poly.vertexes[i];
    if (v0.bulge == 0.0)
      continue;
    const auto &v1 = poly.vertexes[(i + 1) % n];
    ArcGeometry ag;
    if (!ComputeArcGeometry(v0.x, v0.y, v1.x, v1.y, v0.bulge, ag))
      continue;
    for (Float ang : cardinals)
      if (detail::AngleInArcSweep(ang, ag.startAngle, ag.sweep))
        expand(ag.cx + ag.radius * std::cos(ang),
               ag.cy + ag.radius * std::sin(ang));
  }
  return bb;
}

/// Extract explicit typed segments from a polyline.
/// @param poly Source polyline.
/// @return Segment list preserving line/arc type and segment metadata.
inline std::vector<Segment> ExtractSegments(const Polyline &poly) {
  const int n = (int)poly.vertexes.size();
  const int segments = poly.isClosed ? n : n - 1;

  std::vector<Segment> out;
  out.reserve(segments);
  for (int i = 0; i < segments; ++i) {
    const auto &v0 = poly.vertexes[i];
    const auto &v1 = poly.vertexes[(i + 1) % n];
    Segment s;
    s.x0 = v0.x;
    s.y0 = v0.y;
    s.x1 = v1.x;
    s.y1 = v1.y;
    s.bulge = v0.bulge;
    s.type = (v0.bulge != 0.0) ? Segment::Type::Arc : Segment::Type::Line;
    s.isCurve = v1.isCurve;
    out.push_back(s);
  }
  return out;
}

} // namespace Gorgon::CGI