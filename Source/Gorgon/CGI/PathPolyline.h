#pragma once

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "../Geometry/Point.h"
#include "Path.h"
#include "Polyline.h"

namespace Gorgon::CGI {
namespace detail {
inline Gorgon::Geometry::Pointf
bezierAt(Gorgon::Geometry::Pointf p0, Gorgon::Geometry::Pointf p1,
         Gorgon::Geometry::Pointf p2, Gorgon::Geometry::Pointf p3, double t) {
  double u = 1.0 - t;
  return p0 * (u * u * u) + p1 * (3.0 * u * u * t) + p2 * (3.0 * u * t * t) +
         p3 * (t * t * t);
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

struct CubicCmd {
  Gorgon::Geometry::Pointf c1, c2, to;
};

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

/// Convert a circular SVG arc command to a bulge value.
/// Returns false when the arc is non-circular (Rx != Ry) and must be flattened.
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

/// Convert every contour of @p path to a Polyline.
/// @param tolerance  Flattening tolerance for non-circular segments.
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
        const Point c1 =
            current + (cmd.Quadratic.C - current) * (Float(2) / Float(3));
        const Point c2 =
            cmd.Quadratic.To +
            (cmd.Quadratic.C - cmd.Quadratic.To) * (Float(2) / Float(3));
        basic_Bezier<Point> bez(current, c1, c2, cmd.Quadratic.To);
        flushFlat(bez.Flatten(tolerance));
        current = cmd.Quadratic.To;
        break;
      }

      case VerbT::CubicTo: {
        basic_Bezier<Point> bez(current, cmd.Cubic.C1, cmd.Cubic.C2,
                                cmd.Cubic.To);
        flushFlat(bez.Flatten(tolerance));
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

/// Reconstruct a basic_Path from a collection of Polylines.
/// @param fitTolerance  Max deviation for Schneider cubic fitting.
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

template <class Point>
inline basic_Path<Point> PolylineToPath(const Polyline &poly,
                                        double fitTolerance = 0.5) {
  return PolylinesToPath<Point>({poly}, fitTolerance);
}

// PointList ↔ Polyline ↔ Path helpers
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