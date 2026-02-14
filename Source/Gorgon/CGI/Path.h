#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "../Geometry/Point.h"
#include "../Utils/Assert.h"
#include "Bezier.h"

namespace Gorgon {
namespace CGI {

/// Path builder that records drawing commands and can flatten curves.
template <class Point_> class basic_Path {
public:
  using Point = Point_;
  using PointList = Geometry::PointList<Point>;

  /// Drawing commands supported by the path.
  enum Verb { MoveTo, LineTo, CubicTo, Close };

  /// Cubic Bezier segment control points.
  struct Cubic {
    Point C1;
    Point C2;
    Point To;
  };

  /// Single drawing command stored in the path.
  struct Command {
    Verb Verb = Close;

    union {
      Point To;
      Cubic Cubic;
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

    /// Create a cubic Bezier command with two controls and an endpoint.
    static Command CubicTo(Point c1, Point c2, Point to) {
      Command cmd;
      cmd.Verb = Verb::CubicTo;
      cmd.Cubic = {c1, c2, to};
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

  /// Append a cubic Bezier segment to the active contour.
  void AddCubicTo(Point c1, Point c2, Point to) {
    ASSERT(ActiveContourIndex >= 0 && !ExpectsMoveTo,
           "CubicTo requires an active contour started by MoveTo");
    PushCommand(Command::CubicTo(c1, c2, to));
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
      case Verb::CubicTo:
        fn(cmd.Cubic.C1);
        fn(cmd.Cubic.C2);
        fn(cmd.Cubic.To);
        break;
      case Verb::Close:
        break;
      }
    }
  }

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
};

using Path = basic_Path<Geometry::Pointf>;

} // namespace CGI
} // namespace Gorgon
