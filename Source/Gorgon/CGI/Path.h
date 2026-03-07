#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "../Geometry/Point.h"
#include "../Utils/Assert.h"
#include "Bezier.h"

namespace Gorgon ::CGI {

/// Path builder that records drawing commands and can flatten curves.
template <class Point_> class basic_Path {
public:
  using Point = Point_;
  using PointList = Geometry::PointList<Point>;

  /// Drawing commands supported by the path.
  enum Verb { MoveTo, LineTo, QuadraticTo, CubicTo, ArcTo, Close };

  /// Quadratic Bezier segment control point.
  struct Quadratic {
    Point C;
    Point To;
  };

  /// Cubic Bezier segment control points.
  struct Cubic {
    Point C1;
    Point C2;
    Point To;
  };

  /// SVG elliptical arc segment parameters.
  struct Arc {
    Float Rx;
    Float Ry;
    Float XAxisRotation;
    bool LargeArc;
    bool Sweep;
    Point To;
  };

  /// Single drawing command stored in the path.
  struct Command {
    Verb Verb = Verb::Close;

    union {
      Point To;
      Quadratic Quadratic;
      Cubic Cubic;
      Arc Arc;
    };

    Command() : To() {}

    /// Create a move command that starts or repositions the contour.
    static Command MoveTo(Point to) {
      Command cmd;
      cmd.Verb = Verb::MoveTo;
      cmd.To = to;
      return cmd;
    }

    /// Create a line command to the given point.
    static Command LineTo(Point to) {
      Command cmd;
      cmd.Verb = Verb::LineTo;
      cmd.To = to;
      return cmd;
    }

    /// Create a quadratic Bezier command with one control and an endpoint.
    static Command QuadraticTo(Point c, Point to) {
      Command cmd;
      cmd.Verb = Verb::QuadraticTo;
      cmd.Quadratic = {c, to};
      return cmd;
    }

    /// Create a cubic Bezier command with two controls and an endpoint.
    static Command CubicTo(Point c1, Point c2, Point to) {
      Command cmd;
      cmd.Verb = Verb::CubicTo;
      cmd.Cubic = {c1, c2, to};
      return cmd;
    }

    /// Create an SVG elliptical arc command.
    static Command ArcTo(Float rx, Float ry, Float xAxisRotation, bool largeArc,
                         bool sweep, Point to) {
      Command cmd;
      cmd.Verb = Verb::ArcTo;
      cmd.Arc = {rx, ry, xAxisRotation, largeArc, sweep, to};
      return cmd;
    }

    /// Create a close command that seals the current contour.
    static Command Close() {
      Command cmd;
      cmd.Verb = Verb::Close;
      return cmd;
    }
  };

  /// Metadata describing a contour inside the command buffer.
  struct Contour {
    std::uint32_t FirstCommandIndex = 0;
    std::uint32_t CommandCount = 0;
    std::uint8_t IsNegative = 0;

    bool Negative() const { return IsNegative != 0; }
  };

  /// Flattened contour representation produced by `FlattenContours`.
  struct FlattenedContour {
    PointList Points;
    bool IsClosed = false;
    bool IsNegative = false;
  };

  basic_Path() = default;
  basic_Path(const basic_Path &) = delete;
  basic_Path &operator=(const basic_Path &) = delete;
  basic_Path(basic_Path &&) = default;
  basic_Path &operator=(basic_Path &&) = default;

  /// Usage: Path p; p.AddMoveTo({0, 0}); p.AddLineTo({10, 0});
  /// p.CloseContour(); auto pts = p.Flatten();
  void AddMoveTo(Point to, bool isNegative = false) {
    if (ActiveContourIndex < 0 || !ExpectsMoveTo)
      BeginContour(isNegative);
    else
      Contours[ActiveContourIndex].IsNegative = isNegative ? 1 : 0;

    PushCommand(Command::MoveTo(to));
    ExpectsMoveTo = false;
  }

  /// Append a straight segment to the active contour.
  void AddLineTo(Point to) {
    ASSERT(ActiveContourIndex >= 0 && !ExpectsMoveTo,
           "LineTo requires an active contour started by MoveTo");
    PushCommand(Command::LineTo(to));
  }

  /// Append a quadratic Bezier segment to the active contour.
  void AddQuadraticTo(Point c, Point to) {
    ASSERT(ActiveContourIndex >= 0 && !ExpectsMoveTo,
           "QuadraticTo requires an active contour started by MoveTo");
    PushCommand(Command::QuadraticTo(c, to));
  }

  /// Append a cubic Bezier segment to the active contour.
  void AddCubicTo(Point c1, Point c2, Point to) {
    ASSERT(ActiveContourIndex >= 0 && !ExpectsMoveTo,
           "CubicTo requires an active contour started by MoveTo");
    PushCommand(Command::CubicTo(c1, c2, to));
  }

  /// Append an SVG elliptical arc segment to the active contour.
  void AddArcTo(Float rx, Float ry, Float xAxisRotation, bool largeArc,
                bool sweep, Point to) {
    ASSERT(ActiveContourIndex >= 0 && !ExpectsMoveTo,
           "ArcTo requires an active contour started by MoveTo");
    PushCommand(Command::ArcTo(rx, ry, xAxisRotation, largeArc, sweep, to));
  }

  /// Close the active contour, marking it as finished.
  void CloseContour() {
    ASSERT(ActiveContourIndex >= 0 && !ExpectsMoveTo,
           "Close requires an active contour started by MoveTo");
    PushCommand(Command::Close());
    ExpectsMoveTo = true;
    ActiveContourIndex = -1;
  }

  /// Flatten the path into polylines; optionally enforce winding.
  std::vector<PointList> Flatten(Float tolerance = 0.72f,
                                 bool enforceWinding = true) const {
    auto flattened = FlattenContours(tolerance, enforceWinding);
    std::vector<PointList> out;
    out.reserve(flattened.size());
    for (auto &fc : flattened) {
      out.push_back(std::move(fc.Points));
    }
    return out;
  }

  /// Flatten the path and return per-contour metadata.
  std::vector<FlattenedContour>
  FlattenContours(Float tolerance = 0.72f, bool enforceWinding = true) const {
    ASSERT(tolerance > 0, "Tolerance cannot be 0 or less");

    std::vector<FlattenedContour> out;
    out.reserve(Contours.size());

    for (const auto &contour : Contours) {
      if (contour.CommandCount == 0)
        continue;

      const std::size_t beginIndex = contour.FirstCommandIndex;
      const std::size_t endIndex = beginIndex + contour.CommandCount;

      ASSERT(endIndex <= Commands.size(),
             "Contour command range is out of bounds");

      FlattenedContour flattened;
      flattened.IsNegative = contour.Negative();

      auto &points = flattened.Points;
      bool haveCurrent = false;
      Point current(0, 0);
      Point start(0, 0);

      for (std::size_t i = beginIndex; i < endIndex; i++) {
        const auto &cmd = Commands[i];

        if (!haveCurrent)
          ASSERT(cmd.Verb == Verb::MoveTo, "Contours must start with MoveTo");

        switch (cmd.Verb) {
        case Verb::MoveTo: {
          current = cmd.To;
          start = current;
          haveCurrent = true;
          points.Clear();
          points.Push(current);
          break;
        }
        case Verb::LineTo: {
          ASSERT(haveCurrent, "LineTo requires MoveTo");
          current = cmd.To;
          if (points.IsEmpty() || points.Back() != current)
            points.Push(current);
          break;
        }
        case Verb::QuadraticTo: {
          ASSERT(haveCurrent, "QuadraticTo requires MoveTo");
          const Point c1 =
              current + (cmd.Quadratic.C - current) * (2.0f / 3.0f);
          const Point c2 = cmd.Quadratic.To +
                           (cmd.Quadratic.C - cmd.Quadratic.To) * (2.0f / 3.0f);

          basic_Bezier<Point> bez(current, c1, c2, cmd.Quadratic.To);
          auto seg = bez.Flatten(tolerance);
          auto sz = seg.GetSize();
          if (sz > 0) {
            for (std::size_t pi = 1; pi < sz; pi++) {
              const auto &p = seg[pi];
              if (points.IsEmpty() || points.Back() != p)
                points.Push(p);
            }
          }

          current = cmd.Quadratic.To;
          break;
        }
        case Verb::CubicTo: {
          ASSERT(haveCurrent, "CubicTo requires MoveTo");
          basic_Bezier<Point> bez(current, cmd.Cubic.C1, cmd.Cubic.C2,
                                  cmd.Cubic.To);
          auto seg = bez.Flatten(tolerance);
          auto sz = seg.GetSize();
          if (sz > 0) {
            // Append, skipping the first point (it equals current).
            for (std::size_t pi = 1; pi < sz; pi++) {
              const auto &p = seg[pi];
              if (points.IsEmpty() || points.Back() != p)
                points.Push(p);
            }
          }
          current = cmd.Cubic.To;
          break;
        }
        case Verb::ArcTo: {
          ASSERT(haveCurrent, "ArcTo requires MoveTo");
          AppendArcAsPolyline(points, current, cmd.Arc, tolerance);
          current = cmd.Arc.To;
          break;
        }
        case Verb::Close: {
          ASSERT(haveCurrent, "Close requires MoveTo");
          flattened.IsClosed = true;
          current = start;
          i = endIndex;
          break;
        }
        }
      }

      if (points.GetSize() < 2)
        continue;

      if (points.GetSize() >= 2 && points.Front() == points.Back())
        points.Pop();

      if (enforceWinding && flattened.IsClosed && points.GetSize() >= 3) {
        auto area2 = SignedArea2(points);
        bool isCCW = area2 > 0;

        // Convention: positive geometry -> CCW, negative geometry -> CW.
        bool wantCCW = !flattened.IsNegative;
        if (isCCW != wantCCW)
          std::reverse(points.begin(), points.end());
      }

      out.push_back(std::move(flattened));
    }
    return out;
  }

  /// Create a deep copy of the path.
  basic_Path Duplicate() const {
    basic_Path copy;
    copy.Commands = Commands;
    copy.Contours = Contours;
    copy.ActiveContourIndex = ActiveContourIndex;
    copy.ExpectsMoveTo = ExpectsMoveTo;
    return copy;
  }

  /// Apply an in-place transform to every point stored in commands.
  template <class F_> void TransformPoints(F_ fn) {
    for (auto &cmd : Commands) {
      switch (cmd.Verb) {
      case Verb::MoveTo:
      case Verb::LineTo:
        fn(cmd.To);
        break;
      case Verb::QuadraticTo:
        fn(cmd.Quadratic.C);
        fn(cmd.Quadratic.To);
        break;
      case Verb::CubicTo:
        fn(cmd.Cubic.C1);
        fn(cmd.Cubic.C2);
        fn(cmd.Cubic.To);
        break;
      case Verb::ArcTo:
        fn(cmd.Arc.To);
        break;
      case Verb::Close:
        break;
      }
    }
  }

  /// Get read-only access to the commands.
  const std::vector<Command> &GetCommands() const { return Commands; }

  /// Get read-only access to the contours.
  const std::vector<Contour> &GetContours() const { return Contours; }

private:
  std::vector<Command> Commands;
  std::vector<Contour> Contours;

  int ActiveContourIndex = -1;
  bool ExpectsMoveTo = false;

  void BeginContour(bool isNegative) {
    Contour contour;
    contour.FirstCommandIndex = Commands.size();
    contour.CommandCount = 0;
    contour.IsNegative = isNegative ? 1 : 0;

    Contours.push_back(contour);
    ActiveContourIndex = Contours.size() - 1;
    ExpectsMoveTo = true;
  }

  void PushCommand(const Command &cmd) {
    ASSERT(ActiveContourIndex >= 0,
           "Cannot add path commands without an active contour");
    Commands.push_back(cmd);

    auto &contour = Contours[(std::size_t)ActiveContourIndex];
    contour.CommandCount++;
  }

  static Float SignedArea2(const PointList &points) {
    // Returns twice the signed area. Positive means CCW.
    Float a = 0;
    auto n = points.GetSize();
    for (std::size_t i = 0; i < n; i++) {
      const auto &p0 = points[i];
      const auto &p1 = points[(i + 1) % n];
      a += p0.X * p1.Y - p1.X * p0.Y;
    }
    return a;
  }

  static void AppendArcAsPolyline(PointList &points, const Point &start,
                                  const Arc &arc, Float tolerance) {
    if (std::abs(arc.Rx) < 0.001f || std::abs(arc.Ry) < 0.001f) {
      if (points.IsEmpty() || points.Back() != arc.To)
        points.Push(arc.To);
      return;
    }

    Float rx = std::abs(arc.Rx);
    Float ry = std::abs(arc.Ry);
    const Float rotation = arc.XAxisRotation * (Float)PI / 180.0f;

    const Float dx = (start.X - arc.To.X) / 2.0f;
    const Float dy = (start.Y - arc.To.Y) / 2.0f;

    const Float cosRot = std::cos(rotation);
    const Float sinRot = std::sin(rotation);

    const Float x1p = cosRot * dx + sinRot * dy;
    const Float y1p = -sinRot * dx + cosRot * dy;

    const Float lambda = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
    if (lambda > 1.0f) {
      const Float scale = std::sqrt(lambda);
      rx *= scale;
      ry *= scale;
    }

    const Float denom = rx * rx * y1p * y1p + ry * ry * x1p * x1p;
    if (denom <= 0.0f) {
      if (points.IsEmpty() || points.Back() != arc.To)
        points.Push(arc.To);
      return;
    }

    const Float sq = std::max(
        0.0f, (rx * rx * ry * ry - rx * rx * y1p * y1p - ry * ry * x1p * x1p) /
                  denom);
    Float coef = std::sqrt(sq);
    if (arc.LargeArc == arc.Sweep)
      coef = -coef;

    const Float cxp = coef * rx * y1p / ry;
    const Float cyp = -coef * ry * x1p / rx;

    const Float cx = cosRot * cxp - sinRot * cyp + (start.X + arc.To.X) / 2.0f;
    const Float cy = sinRot * cxp + cosRot * cyp + (start.Y + arc.To.Y) / 2.0f;

    const Float theta1 = VectorAngle(1, 0, (x1p - cxp) / rx, (y1p - cyp) / ry);
    Float dtheta = VectorAngle((x1p - cxp) / rx, (y1p - cyp) / ry,
                               (-x1p - cxp) / rx, (-y1p - cyp) / ry);

    if (arc.Sweep && dtheta < 0)
      dtheta += (Float)2 * (Float)PI;
    else if (!arc.Sweep && dtheta > 0)
      dtheta -= (Float)2 * (Float)PI;

    const int numSegments =
        std::max(1, (int)std::ceil(std::abs(dtheta) / ((Float)PI / 2.0f)));
    const Float segmentAngle = dtheta / numSegments;

    Point segmentStart = start;

    for (int i = 0; i < numSegments; i++) {
      const Float startAngle = theta1 + i * segmentAngle;
      const Float endAngle = startAngle + segmentAngle;

      const Float tanHalf = std::tan(segmentAngle / 2.0f);
      const Float alpha = std::sin(segmentAngle) *
                          (std::sqrt(4.0f + 3.0f * tanHalf * tanHalf) - 1.0f) /
                          3.0f;

      const Float cos1 = std::cos(startAngle), sin1 = std::sin(startAngle);
      const Float cos2 = std::cos(endAngle), sin2 = std::sin(endAngle);

      const Float p0x = rx * cos1, p0y = ry * sin1;
      const Float p3x = rx * cos2, p3y = ry * sin2;

      const Float p1x = p0x - alpha * rx * sin1;
      const Float p1y = p0y + alpha * ry * cos1;
      const Float p2x = p3x + alpha * rx * sin2;
      const Float p2y = p3y - alpha * ry * cos2;

      auto transform = [&](Float x, Float y) -> Point {
        return Point(cosRot * x - sinRot * y + cx,
                     sinRot * x + cosRot * y + cy);
      };

      const Point c1 = transform(p1x, p1y);
      const Point c2 = transform(p2x, p2y);
      const Point to = transform(p3x, p3y);

      basic_Bezier<Point> bez(segmentStart, c1, c2, to);
      auto seg = bez.Flatten(tolerance);
      const auto sz = seg.GetSize();
      for (std::size_t pi = 1; pi < sz; pi++) {
        const auto &p = seg[pi];
        if (points.IsEmpty() || points.Back() != p)
          points.Push(p);
      }

      segmentStart = to;
    }
  }

  static Float VectorAngle(Float ux, Float uy, Float vx, Float vy) {
    const Float dot = ux * vx + uy * vy;
    const Float len =
        std::sqrt(ux * ux + uy * uy) * std::sqrt(vx * vx + vy * vy);
    if (len <= 0.0f)
      return 0.0f;

    Float cosValue = dot / len;
    if (cosValue < -1.0f)
      cosValue = -1.0f;
    else if (cosValue > 1.0f)
      cosValue = 1.0f;

    Float angle = std::acos(cosValue);
    if (ux * vy - uy * vx < 0)
      angle = -angle;
    return angle;
  }
};

using Path = basic_Path<Geometry::Pointf>;

} // namespace Gorgon::CGI
  // namespace Gorgon
