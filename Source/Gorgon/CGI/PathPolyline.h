#pragma once

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "../Geometry/Point.h"
#include "Path.h"
#include "Polyline.h"

namespace Gorgon ::CGI {
namespace detail {
/// Evaluate a cubic Bezier at parameter t.
/// @param p0 Start point.
/// @param p1 First control point.
/// @param p2 Second control point.
/// @param p3 End point.
/// @param t Parameter in [0,1].
/// @return Point on the cubic curve.
inline Gorgon::Geometry::Pointf
bezierAt(Gorgon::Geometry::Pointf p0, Gorgon::Geometry::Pointf p1,
         Gorgon::Geometry::Pointf p2, Gorgon::Geometry::Pointf p3, double t) {
  double u = 1.0 - t;
  return p0 * (u * u * u) + p1 * (3.0 * u * u * t) + p2 * (3.0 * u * t * t) +
         p3 * (t * t * t);
}

/// Generic template-safe evaluation of a bezier curve
template <class Point>
inline Point bezierAtTmpl(const Point &p0, const Point &p1, const Point &p2,
                          const Point &p3, double t) {
  double u = 1.0 - t;
  double b0 = u * u * u;
  double b1 = 3.0 * u * u * t;
  double b2 = 3.0 * u * t * t;
  double b3 = t * t * t;
  return Point(p0.X * b0 + p1.X * b1 + p2.X * b2 + p3.X * b3,
               p0.Y * b0 + p1.Y * b1 + p2.Y * b2 + p3.Y * b3);
}

/// Center and radius of a circle passing through 3 points using Cramer's rule
template <class Point>
inline bool GetArcCenterAndRadius(const Point &p1, const Point &p2,
                                  const Point &p3, Point &center,
                                  double &radius) {
  double x1 = p1.X, y1 = p1.Y;
  double x2 = p2.X, y2 = p2.Y;
  double x3 = p3.X, y3 = p3.Y;

  double D = 2.0 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
  if (std::abs(D) < 1e-9)
    return false;

  double sq1 = x1 * x1 + y1 * y1;
  double sq2 = x2 * x2 + y2 * y2;
  double sq3 = x3 * x3 + y3 * y3;

  center.X = (sq1 * (y2 - y3) + sq2 * (y3 - y1) + sq3 * (y1 - y2)) / D;
  center.Y = (sq1 * (x3 - x2) + sq2 * (x1 - x3) + sq3 * (x2 - x1)) / D;

  double dx = x1 - center.X;
  double dy = y1 - center.Y;
  radius = std::sqrt(dx * dx + dy * dy);
  return true;
}

/// Calculates the exact bulge for the unique arc passing through p1, p2, and p3
/// using purely vector-based math (no angles, no wrap-around bugs)
template <class Point>
inline double ComputeBulge3P(const Point &p1, const Point &p2,
                             const Point &p3) {
  double v1x = p1.X - p2.X;
  double v1y = p1.Y - p2.Y;
  double v3x = p3.X - p2.X;
  double v3y = p3.Y - p2.Y;

  double cross = v1x * v3y - v1y * v3x;
  double dot = v1x * v3x + v1y * v3y;

  double len1 = std::sqrt(v1x * v1x + v1y * v1y);
  double len3 = std::sqrt(v3x * v3x + v3y * v3y);

  double denom = len1 * len3 - dot;

  if (denom < 1e-9)
    return 0.0;

  return -cross / denom;
}

/// Recursively subdivides a Cubic Bezier into perfectly fitting Circular Arcs
template <class Point>
inline void
BezierToArcsRecursive(Point p0, Point p1, Point p2, Point p3, double tolerance,
                      int depth,
                      std::vector<std::pair<Point, double>> &outArcs) {

  Point mid = bezierAtTmpl(p0, p1, p2, p3, 0.5);
  double bulge = ComputeBulge3P(p0, mid, p3);

  Point q1 = bezierAtTmpl(p0, p1, p2, p3, 0.25);
  Point q2 = bezierAtTmpl(p0, p1, p2, p3, 0.75);

  bool needsSplit = false;
  if (depth < 8) {
    Point center(0, 0);
    double radius = 0;

    if (GetArcCenterAndRadius(p0, mid, p3, center, radius)) {
      double d1 = std::abs(std::sqrt((q1.X - center.X) * (q1.X - center.X) +
                                     (q1.Y - center.Y) * (q1.Y - center.Y)) -
                           radius);
      double d2 = std::abs(std::sqrt((q2.X - center.X) * (q2.X - center.X) +
                                     (q2.Y - center.Y) * (q2.Y - center.Y)) -
                           radius);
      if (d1 > tolerance || d2 > tolerance) {
        needsSplit = true;
      }
    } else {
      double dx = p3.X - p0.X;
      double dy = p3.Y - p0.Y;
      double chordLen = std::sqrt(dx * dx + dy * dy) + 1e-9;
      double cross1 =
          std::abs(dx * (q1.Y - p0.Y) - dy * (q1.X - p0.X)) / chordLen;
      double cross2 =
          std::abs(dx * (q2.Y - p0.Y) - dy * (q2.X - p0.X)) / chordLen;
      if (cross1 > tolerance || cross2 > tolerance) {
        needsSplit = true;
      }
    }
  }

  if (needsSplit) {
    Point p01((p0.X + p1.X) / 2.0, (p0.Y + p1.Y) / 2.0);
    Point p12((p1.X + p2.X) / 2.0, (p1.Y + p2.Y) / 2.0);
    Point p23((p2.X + p3.X) / 2.0, (p2.Y + p3.Y) / 2.0);

    Point p012((p01.X + p12.X) / 2.0, (p01.Y + p12.Y) / 2.0);
    Point p123((p12.X + p23.X) / 2.0, (p12.Y + p23.Y) / 2.0);

    Point p0123((p012.X + p123.X) / 2.0, (p012.Y + p123.Y) / 2.0);

    BezierToArcsRecursive(p0, p01, p012, p0123, tolerance, depth + 1, outArcs);
    BezierToArcsRecursive(p0123, p123, p23, p3, tolerance, depth + 1, outArcs);
  } else {
    outArcs.push_back({p3, bulge});
  }
}

inline std::vector<double>
chordParam(const std::vector<Gorgon::Geometry::Pointf> &pts) {
  int n = (int)pts.size();
  std::vector<double> u(n, 0.0);
  for (int i = 1; i < n; i++)
    u[i] = u[i - 1] + (pts[i] - pts[i - 1]).Distance();

  double total = u.back();
  if (total > 1e-12) {
    for (auto &v : u)
      v /= total;
  } else if (n > 1) {
    for (int i = 0; i < n; i++)
      u[i] = double(i) / (n - 1);
  }
  return u;
}

/// Reparameterize sample parameters using one Newton-Raphson step per sample.
/// @param pts Input points to fit.
/// @param u Current parameter values in [0,1].
/// @param p0 Cubic start point.
/// @param p1 Cubic first control point.
/// @param p2 Cubic second control point.
/// @param p3 Cubic end point.
/// @return Updated parameter values clamped to [0,1].
inline std::vector<double>
reparameterize(const std::vector<Gorgon::Geometry::Pointf> &pts,
               const std::vector<double> &u, Gorgon::Geometry::Pointf p0,
               Gorgon::Geometry::Pointf p1, Gorgon::Geometry::Pointf p2,
               Gorgon::Geometry::Pointf p3) {
  std::vector<double> uNew(u.size());

  for (int i = 0; i < (int)u.size(); i++) {
    double t = u[i];
    double mt = 1.0 - t;

    Gorgon::Geometry::Pointf q = bezierAt(p0, p1, p2, p3, t);
    Gorgon::Geometry::Pointf q1 = (p1 - p0) * (3.0 * mt * mt) +
                                  (p2 - p1) * (6.0 * mt * t) +
                                  (p3 - p2) * (3.0 * t * t);
    Gorgon::Geometry::Pointf t0 = {p2.X - 2 * p1.X + p0.X,
                                   p2.Y - 2 * p1.Y + p0.Y};
    Gorgon::Geometry::Pointf t1 = {p3.X - 2 * p2.X + p1.X,
                                   p3.Y - 2 * p2.Y + p1.Y};
    Gorgon::Geometry::Pointf q2 = t0 * (6.0 * mt) + t1 * (6.0 * t);

    Gorgon::Geometry::Pointf diff = q - pts[i];
    double num = diff.DotProduct(q1);
    double den = q1.DotProduct(q1) + diff.DotProduct(q2);

    uNew[i] = (std::abs(den) < 1e-12)
                  ? t
                  : std::max(0.0, std::min(1.0, t - num / den));
  }
  return uNew;
}

/// Fit a cubic Bezier segment to a point run with fixed endpoint tangents.
/// @param pts Sample points for the run.
/// @param u Parameter values associated with @p pts.
/// @param t1 Unit tangent at the start.
/// @param t2 Unit tangent at the end.
/// @return Pair of computed control points (c1, c2).
inline std::pair<Gorgon::Geometry::Pointf, Gorgon::Geometry::Pointf>
fitCubic(const std::vector<Gorgon::Geometry::Pointf> &pts,
         const std::vector<double> &u, Gorgon::Geometry::Pointf t1,
         Gorgon::Geometry::Pointf t2) {
  int n = (int)pts.size();
  Gorgon::Geometry::Pointf p0 = pts[0], p3 = pts[n - 1];

  double c[2][2] = {}, x[2] = {};
  for (int i = 0; i < n; i++) {
    double t = u[i], mt = 1.0 - t;
    double b0 = mt * mt * mt, b1 = 3.0 * mt * mt * t, b2 = 3.0 * mt * t * t,
           b3 = t * t * t;
    Gorgon::Geometry::Pointf a1 = t1 * b1, a2 = t2 * b2;

    c[0][0] += a1.DotProduct(a1);
    c[0][1] += a1.DotProduct(a2);
    c[1][0] += a2.DotProduct(a1);
    c[1][1] += a2.DotProduct(a2);

    Gorgon::Geometry::Pointf rhs = pts[i] - (p0 * (b0 + b1) + p3 * (b2 + b3));
    x[0] += rhs.DotProduct(a1);
    x[1] += rhs.DotProduct(a2);
  }

  double det = c[0][0] * c[1][1] - c[0][1] * c[1][0];
  double alpha1, alpha2;
  if (std::abs(det) < 1e-12) {
    double len = (p3 - p0).Distance() / 3.0;
    alpha1 = alpha2 = len;
  } else {
    alpha1 = (x[0] * c[1][1] - x[1] * c[0][1]) / det;
    alpha2 = (c[0][0] * x[1] - c[1][0] * x[0]) / det;
  }

  double segLen = (p3 - p0).Distance() / 3.0;
  if (alpha1 < 1e-9)
    alpha1 = segLen;
  if (alpha2 < 1e-9)
    alpha2 = segLen;

  return {p0 + t1 * alpha1, p3 + t2 * alpha2};
}

/// Compute maximum squared fitting error for a candidate cubic.
/// @param pts Sample points for the run.
/// @param u Parameter values associated with @p pts.
/// @param p0 Cubic start point.
/// @param p1 Cubic first control point.
/// @param p2 Cubic second control point.
/// @param p3 Cubic end point.
/// @param splitIdx Output index of worst-error sample.
/// @return Maximum squared distance between curve and samples.
inline double maxSqError(const std::vector<Gorgon::Geometry::Pointf> &pts,
                         const std::vector<double> &u,
                         Gorgon::Geometry::Pointf p0,
                         Gorgon::Geometry::Pointf p1,
                         Gorgon::Geometry::Pointf p2,
                         Gorgon::Geometry::Pointf p3, int &splitIdx) {
  double best = 0.0;
  splitIdx = (int)pts.size() / 2;

  for (int i = 1; i < (int)pts.size() - 1; i++) {
    Gorgon::Geometry::Pointf d = bezierAt(p0, p1, p2, p3, u[i]) - pts[i];
    double e = d.DotProduct(d);
    if (e > best) {
      best = e;
      splitIdx = i;
    }
  }
  return best;
}

/// Cubic Bezier command payload (control1, control2, endpoint).
struct CubicCmd {
  Gorgon::Geometry::Pointf c1, c2, to;
};

/// Estimate an endpoint tangent by probing nearby samples.
/// @param pts Input point run.
/// @param index Anchor point index.
/// @param dir Direction (+1 forward, -1 backward).
/// @param reach Maximum probe distance in samples.
/// @return Normalized tangent or {0,0} if unavailable.
inline Gorgon::Geometry::Pointf
edgeTangent(const std::vector<Gorgon::Geometry::Pointf> &pts, int index,
            int dir, int reach = 3) {
  int n = (int)pts.size();
  for (int k = 1; k <= reach; k++) {
    int j = index + dir * k;
    if (j < 0 || j >= n)
      break;
    Gorgon::Geometry::Pointf d = (pts[j] - pts[index]).Normalize();
    if (d.Distance() > 1e-12)
      return d;
  }
  return {0, 0};
}

/// Recursive Schneider-style curve fitting for one point interval.
/// @param pts Full point sequence.
/// @param first First sample index (inclusive).
/// @param last Last sample index (inclusive).
/// @param t1 Start tangent.
/// @param t2 End tangent.
/// @param tolerance Maximum distance error.
/// @param maxIter Reparameterization iterations per recursion level.
/// @param out Output cubic command list.
inline void schneiderFit(const std::vector<Gorgon::Geometry::Pointf> &pts,
                         int first, int last, Gorgon::Geometry::Pointf t1,
                         Gorgon::Geometry::Pointf t2, double tolerance,
                         int maxIter, std::vector<CubicCmd> &out) {
  int n = last - first + 1;
  if (n < 2)
    return;

  if (n == 2) {
    double len = (pts[last] - pts[first]).Distance() / 3.0;
    Gorgon::Geometry::Pointf p0 = pts[first], p3 = pts[last];
    out.push_back({p0 + t1 * len, p3 + t2 * len, p3});
    return;
  }

  std::vector<Gorgon::Geometry::Pointf> sub(pts.begin() + first,
                                            pts.begin() + last + 1);
  auto u = chordParam(sub);
  auto [p1, p2] = fitCubic(sub, u, t1, t2);
  Gorgon::Geometry::Pointf p0 = sub.front(), p3 = sub.back();

  int splitIdx;
  double thresh = tolerance * tolerance;
  double err = maxSqError(sub, u, p0, p1, p2, p3, splitIdx);

  if (err < thresh) {
    out.push_back({p1, p2, p3});
    return;
  }

  for (int iter = 0; iter < maxIter && err >= thresh; iter++) {
    u = reparameterize(sub, u, p0, p1, p2, p3);
    auto [np1, np2] = fitCubic(sub, u, t1, t2);
    p1 = np1;
    p2 = np2;
    err = maxSqError(sub, u, p0, p1, p2, p3, splitIdx);
  }

  if (err < thresh) {
    out.push_back({p1, p2, p3});
    return;
  }

  int si = first + splitIdx;
  Gorgon::Geometry::Pointf tSplit;
  if (si > first && si < last) {
    tSplit = ((pts[si] - pts[si - 1]).Normalize() +
              (pts[si + 1] - pts[si]).Normalize())
                 .Normalize();
  } else {
    tSplit = (pts[si < last ? si + 1 : si] - pts[si]).Normalize();
  }

  if (tSplit.Distance() < 1e-12)
    tSplit =
        (pts[si + 1 < (int)pts.size() ? si + 1 : si] - pts[si]).Normalize();

  schneiderFit(pts, first, si, t1, tSplit * -1.0, tolerance, maxIter, out);
  schneiderFit(pts, si, last, tSplit, t2, tolerance, maxIter, out);
}

/// Fit one polyline run with one or more cubic segments.
/// @param pts Ordered points to approximate.
/// @param tolerance Distance tolerance used for splitting.
/// @param maxIter Newton refinement iterations per segment.
/// @return Cubic command sequence describing the fitted run.
inline std::vector<CubicCmd>
fitCurvesToRun(const std::vector<Gorgon::Geometry::Pointf> &pts,
               double tolerance, int maxIter = 4) {
  std::vector<CubicCmd> out;
  int n = (int)pts.size();
  if (n < 2)
    return out;

  Gorgon::Geometry::Pointf t1 = edgeTangent(pts, 0, +1);
  Gorgon::Geometry::Pointf t2 = edgeTangent(pts, n - 1, -1);
  schneiderFit(pts, 0, n - 1, t1, t2, tolerance, maxIter, out);
  return out;
}

/// Convert an SVG-like circular arc command to a polyline bulge value.
/// @param from Arc start point.
/// @param arc Arc payload.
/// @param bulge Output bulge value (tan(sweep/4)) on success.
/// @return True if the arc can be represented as a circular bulge segment.
template <class Point>
inline bool TryArcToBulge(const Point &from,
                          const typename basic_Path<Point>::Arc &arc,
                          double &bulge) {
  if (std::abs(arc.Rx - arc.Ry) > Float(1e-4))
    return false;
  if (arc.Rx < Float(1e-6))
    return false;

  Float rx = arc.Rx, ry = arc.Ry;
  const Float dx = (from.X - arc.To.X) / Float(2);
  const Float dy = (from.Y - arc.To.Y) / Float(2);
  const Float x1p = dx, y1p = dy;

  const Float lam = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
  if (lam > Float(1)) {
    Float s = std::sqrt(lam);
    rx *= s;
    ry *= s;
  }

  const Float denom = rx * rx * y1p * y1p + ry * ry * x1p * x1p;
  if (denom <= Float(0))
    return false;

  const Float sq = std::max(
      Float(0),
      (rx * rx * ry * ry - rx * rx * y1p * y1p - ry * ry * x1p * x1p) / denom);
  Float coef = std::sqrt(sq);
  if (arc.LargeArc == arc.Sweep)
    coef = -coef;

  const Float cxp = coef * rx * y1p / ry;
  const Float cyp = -coef * ry * x1p / rx;

  auto vecAngle = [](double ux, double uy, double vx, double vy) {
    double dot = ux * vx + uy * vy;
    double len = std::sqrt((ux * ux + uy * uy) * (vx * vx + vy * vy));
    if (len <= 0)
      return 0.0;
    double c = std::max(-1.0, std::min(1.0, dot / len));
    double a = std::acos(c);
    if (ux * vy - uy * vx < 0)
      a = -a;
    return a;
  };

  double dtheta =
      vecAngle(double((x1p - cxp) / rx), double((y1p - cyp) / ry),
               double((-x1p - cxp) / rx), double((-y1p - cyp) / ry));

  if (arc.Sweep && dtheta < 0)
    dtheta += 2 * Gorgon::PI;
  if (!arc.Sweep && dtheta > 0)
    dtheta -= 2 * Gorgon::PI;

  bulge = std::tan(dtheta / 4.0);
  return true;
}

} // namespace detail

/// Convert a path into one or more bulge-aware polylines.
/// @param path Source path made of line, curve, and arc commands.
/// @param tolerance Flattening/fitting tolerance for curve approximation.
/// @return Equivalent polylines preserving closure and orientation.
template <class Point>
inline std::vector<Polyline> PathToPolylines(const basic_Path<Point> &path,
                                             Float tolerance = 0.72f) {
  using VerbT = typename basic_Path<Point>::Verb;

  const auto &commands = path.GetCommands();
  const auto &contours = path.GetContours();

  std::vector<Polyline> result;
  result.reserve(contours.size());

  for (const auto &contour : contours) {
    if (contour.CommandCount == 0)
      continue;

    const std::size_t begin = contour.FirstCommandIndex;
    const std::size_t end = begin + contour.CommandCount;

    Polyline poly;
    poly.isNegative = contour.Negative();

    Point current(0, 0), start(0, 0);

    auto pushVertex = [&](const Point &p, bool isCurve) {
      poly.vertexes.push_back({double(p.X), double(p.Y), 0.0, isCurve});
    };

    auto flushFlat = [&](const Geometry::PointList<Point> &pts) {
      for (std::size_t pi = 1; pi < pts.GetSize(); pi++)
        pushVertex(pts[pi], true);
    };

    for (std::size_t i = begin; i < end; i++) {
      const auto &cmd = commands[i];

      switch (cmd.Verb) {
      case VerbT::MoveTo:
        current = cmd.To;
        start = current;
        poly.vertexes.clear();
        pushVertex(current, false);
        break;

      case VerbT::LineTo:
        current = cmd.To;
        pushVertex(current, false);
        break;

      case VerbT::QuadraticTo: {
        const Point c1(
            (current.X + (cmd.Quadratic.C.X - current.X) * (2.0 / 3.0)),
            (current.Y + (cmd.Quadratic.C.Y - current.Y) * (2.0 / 3.0)));
        const Point c2(
            (cmd.Quadratic.To.X +
             (cmd.Quadratic.C.X - cmd.Quadratic.To.X) * (2.0 / 3.0)),
            (cmd.Quadratic.To.Y +
             (cmd.Quadratic.C.Y - cmd.Quadratic.To.Y) * (2.0 / 3.0)));

        std::vector<std::pair<Point, double>> arcSegments;
        detail::BezierToArcsRecursive<Point>(current, c1, c2, cmd.Quadratic.To,
                                             tolerance, 0, arcSegments);

        for (const auto &arc : arcSegments) {
          poly.vertexes.back().bulge = arc.second;
          pushVertex(arc.first, true);
        }
        current = cmd.Quadratic.To;
        break;
      }

      case VerbT::CubicTo: {
        std::vector<std::pair<Point, double>> arcSegments;
        detail::BezierToArcsRecursive<Point>(current, cmd.Cubic.C1,
                                             cmd.Cubic.C2, cmd.Cubic.To,
                                             tolerance, 0, arcSegments);

        for (const auto &arc : arcSegments) {
          poly.vertexes.back().bulge = arc.second;
          pushVertex(arc.first, true);
        }
        current = cmd.Cubic.To;
        break;
      }

      case VerbT::ArcTo: {
        double bulge = 0.0;
        if (detail::TryArcToBulge<Point>(current, cmd.Arc, bulge)) {
          poly.vertexes.back().bulge = bulge;
          poly.vertexes.push_back(
              {double(cmd.Arc.To.X), double(cmd.Arc.To.Y), 0.0, false});
        } else {
          basic_Path<Point> tmp;
          tmp.AddMoveTo(current);
          tmp.AddArcTo(cmd.Arc.Rx, cmd.Arc.Ry, cmd.Arc.XAxisRotation,
                       cmd.Arc.LargeArc, cmd.Arc.Sweep, cmd.Arc.To);
          auto flat = tmp.Flatten(tolerance, false);
          if (!flat.empty())
            flushFlat(flat[0]);
        }
        current = cmd.Arc.To;
        break;
      }

      case VerbT::Close: {
        poly.isClosed = true;
        if (poly.vertexes.size() >= 2) {
          const auto &f = poly.vertexes.front();
          const auto &b = poly.vertexes.back();
          if (std::abs(b.x - f.x) < 1e-9 && std::abs(b.y - f.y) < 1e-9) {
            poly.closingCurve = b.isCurve;
            poly.vertexes.pop_back();
          }
        }
        current = start;
        i = end;
        break;
      }
      }
    }

    if (poly.vertexes.size() >= 2)
      result.push_back(std::move(poly));
  }

  return result;
}

/// Convert bulge-aware polylines to a path.
/// @param polylines Input polylines.
/// @param fitTolerance Tolerance used when fitting curve runs back to cubics.
/// @return Path reconstructed from line, arc, and cubic commands.
template <class Point>
inline basic_Path<Point> PolylinesToPath(const std::vector<Polyline> &polylines,
                                         double fitTolerance = 0.5) {
  basic_Path<Point> path;

  for (const auto &poly : polylines) {
    const int n = (int)poly.vertexes.size();
    if (n == 0)
      continue;

    const int numSegments = poly.isClosed ? n : n - 1;
    if (numSegments <= 0)
      continue;

    path.AddMoveTo(Point(Float(poly.vertexes[0].x), Float(poly.vertexes[0].y)),
                   poly.isNegative);

    std::vector<Gorgon::Geometry::Pointf> curveRun;
    curveRun.push_back({Float(poly.vertexes[0].x), Float(poly.vertexes[0].y)});

    auto flushCurveRun = [&]() {
      if (curveRun.size() < 2) {
        curveRun.clear();
        return;
      }
      for (const auto &c : detail::fitCurvesToRun(curveRun, fitTolerance)) {
        path.AddCubicTo(Point(Float(c.c1.X), Float(c.c1.Y)),
                        Point(Float(c.c2.X), Float(c.c2.Y)),
                        Point(Float(c.to.X), Float(c.to.Y)));
      }
      Gorgon::Geometry::Pointf last = curveRun.back();
      curveRun.clear();
      curveRun.push_back(last);
    };

    for (int i = 0; i < numSegments; i++) {
      const auto &v0 = poly.vertexes[i];
      const auto &v1 = poly.vertexes[(i + 1) % n];
      const Point to(Float(v1.x), Float(v1.y));

      if (v0.bulge != 0.0) {
        flushCurveRun();

        const double sweep = 4.0 * std::atan(v0.bulge);
        const double dx = v1.x - v0.x;
        const double dy = v1.y - v0.y;
        const double chord = std::sqrt(dx * dx + dy * dy);
        const double sinHalf = std::sin(sweep / 2.0);

        if (chord < 1e-9 || std::abs(sinHalf) < 1e-9) {
          path.AddLineTo(to);
        } else {
          const double r = std::abs(chord / (2.0 * sinHalf));
          path.AddArcTo(Float(r), Float(r), Float(0),
                        std::abs(sweep) > Gorgon::PI, v0.bulge > 0.0, to);
        }

        curveRun.clear();
        curveRun.push_back({Float(v1.x), Float(v1.y)});

      } else if (v1.isCurve ||
                 (poly.isClosed && i == numSegments - 1 && poly.closingCurve)) {
        curveRun.push_back({Float(v1.x), Float(v1.y)});

      } else {
        flushCurveRun();
        path.AddLineTo(to);
        curveRun.clear();
        curveRun.push_back({Float(v1.x), Float(v1.y)});
      }
    }

    flushCurveRun();
    if (poly.isClosed)
      path.CloseContour();
  }

  return path;
}

/// Convert a single polyline into a path.
/// @param poly Input polyline.
/// @param fitTolerance Tolerance used when fitting curve runs back to cubics.
/// @return Path representing the input polyline.
template <class Point>
inline basic_Path<Point> PolylineToPath(const Polyline &poly,
                                        double fitTolerance = 0.5) {
  return PolylinesToPath<Point>({poly}, fitTolerance);
}

/// Convert a point list to a polyline with zero bulges.
/// @param points Input points.
/// @param isClosed Whether the resulting polyline is closed.
/// @param isNegative Orientation flag propagated to the polyline.
/// @return Polyline representation of the point list.
template <class Point>
inline Polyline PointListToPolyline(const Geometry::PointList<Point> &points,
                                    bool isClosed = false,
                                    bool isNegative = false) {
  Polyline poly;
  poly.isClosed = isClosed;
  poly.isNegative = isNegative;
  poly.vertexes.reserve(points.GetSize());

  for (const auto &p : points)
    poly.vertexes.push_back({double(p.X), double(p.Y), 0.0, false});

  if (poly.isClosed && poly.vertexes.size() >= 2) {
    const auto &f = poly.vertexes.front();
    const auto &b = poly.vertexes.back();
    if (std::abs(b.x - f.x) < 1e-9 && std::abs(b.y - f.y) < 1e-9)
      poly.vertexes.pop_back();
  }

  return poly;
}

/// Convert a polyline to a point list, optionally appending closure point.
/// @param poly Input polyline.
/// @param includeClosure If true, append first point again for closed
/// polylines.
/// @return Point list extracted from polyline vertices.
template <class Point>
inline Geometry::PointList<Point>
PolylineToPointList(const Polyline &poly, bool includeClosure = false) {
  Geometry::PointList<Point> points;
  for (const auto &v : poly.vertexes)
    points.Push(Point(Float(v.x), Float(v.y)));

  if (includeClosure && poly.isClosed && !poly.vertexes.empty())
    points.Push(Point(Float(poly.vertexes[0].x), Float(poly.vertexes[0].y)));

  return points;
}

/// Convert a path directly into point lists.
/// @param path Source path.
/// @param tolerance Flattening/fitting tolerance.
/// @param includeClosure If true, closed contours repeat their first point.
/// @return One point list per contour.
template <class Point>
inline std::vector<Geometry::PointList<Point>>
PathToPointLists(const basic_Path<Point> &path, Float tolerance = 0.72f,
                 bool includeClosure = false) {
  std::vector<Geometry::PointList<Point>> out;
  auto polylines = PathToPolylines(path, tolerance);
  out.reserve(polylines.size());
  for (const auto &poly : polylines)
    out.push_back(PolylineToPointList<Point>(poly, includeClosure));
  return out;
}

/// Convert point lists to a path using intermediate polylines.
/// @param pointLists Input contour point lists.
/// @param isClosed Whether each generated polyline contour is closed.
/// @param isNegative Orientation flag for generated contours.
/// @param fitTolerance Tolerance used when fitting curve runs back to cubics.
/// @return Path assembled from the provided point lists.
template <class Point>
inline basic_Path<Point>
PointListsToPath(const std::vector<Geometry::PointList<Point>> &pointLists,
                 bool isClosed = true, bool isNegative = false,
                 double fitTolerance = 0.5) {
  std::vector<Polyline> polylines;
  polylines.reserve(pointLists.size());
  for (const auto &points : pointLists)
    polylines.push_back(PointListToPolyline(points, isClosed, isNegative));
  return PolylinesToPath<Point>(polylines, fitTolerance);
}

} // namespace Gorgon::CGI