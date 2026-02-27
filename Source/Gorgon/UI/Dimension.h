#pragma once

#include "../Geometry/Margin.h"
#include "../Geometry/Size.h"

namespace Gorgon :: UI {

    /// Dimension data for components. Allows relative position and sizing.
    class Dimension {
    public:

        /// Unit for dimensions. Dimensions in UI system does not allow floating
        /// point numbers as floating point numbers are not precise and may cause
        /// issues. Additionally, final values always rounded, so that the symbols
        /// are always on full pixels.
        enum Unit {
            /// Fixed pixel based dimensions
            Pixel,

            /// Dimension will be relative to the parent and given in percent.
            /// If higher resolution is necessary use BasisPoint.
            Percent,

            /// 1/1000th of a pixel, there are only few places that this will be used.
            /// Currently only rotation center use non-integer pixels
            MilliPixel,

            /// Dimension will be relative to the parent and given in 1/10000.
            BasisPoint,

            /// Dimension will be relative to the text size, given value is the
            /// percent of the width of an EM dash. If no font information is 
            /// found, 10px will be used for EM dash. Thus, 1 unit will be 0.1
            /// pixels.
            EM,

            /// Size in units defined by UI scale. Uses UnitSize and spacing
            UnitSize,

            /// Size in units defined by UI scale. Uses UnitSize and spacing.
            MilliUnitSize,

            /// Spaces defined by UI scale.
            Spaces,

            /// Fractions of the container. Defaults to 6 if not set.
            Fractions

        };
    private:

        constexpr int dimension_calc(double value, Unit unit) {
            switch(unit) {
            case Percent:
                return int(std::round(value * 100));
            case MilliPixel:
                return int(std::round(value * 1000));
            case BasisPoint:
                return int(std::round(value * 10000));
            case EM:
                return int(std::round(value * 100));
            case MilliUnitSize:
                return int(std::round(value * 1000));
            default:
                return int(std::round(value));
            }
        }

    public:

        /// Constructs a new dimension or type casts integer to dimension
        constexpr Dimension(int value = 0, Unit unit = Pixel) : value(value), unit(unit) {/* implicit */
        }

        /// Constructs a new dimension or type casts real number to dimension
        constexpr Dimension(double value, Unit unit = Percent) : value(dimension_calc(value, unit)), unit(unit) {

        }

        /// Constructs a new dimension or type casts real number to dimension
        constexpr Dimension(float value, Unit unit = Percent) : Dimension((double)value, unit) {

        }

        /// Returns the calculated dimension in pixels
        constexpr int operator ()(int parentwidth, int unitsize, int spacing, int emwidth = 10, int fractions = 6, bool issize = false) const {
            return Calculate(parentwidth, unitsize, spacing, emwidth, fractions, issize);
        }

        /// Returns the calculated dimension in pixels
        constexpr int Calculate(int parentwidth, int unitsize, int spacing, int emwidth = 10, int fractions = 6, bool issize = false) const {
            if(issize) {
                switch(unit) {
                case Percent:
                    return int(std::floor((double)value * parentwidth / 100));
                case BasisPoint:
                    return int(std::floor((double)value * parentwidth / 10000));
                case Fractions:
                    return int(std::floor((double)value * parentwidth / fractions));
                case MilliPixel:
                    return int(std::floor((double)value / 1000));
                case EM:
                    return int(std::round(value * emwidth / 100));
                case UnitSize:
                    if(value >= -1 && value <= 1)
                        return value * unitsize;
                    return value * unitsize + (value - 1) * spacing;
                case MilliUnitSize:
                    if(value >= -1000 && value <= 1000)
                        return int(std::floor((double)value * unitsize / 1000));
                    return int(std::floor(((double)value * unitsize + (double)(value - 1000) * spacing) / 1000));
                case Spaces:
                    return value * spacing;
                case Pixel:
                default:
                    return value;
                }
            }
            else {
                switch(unit) {
                case Percent:
                    return int(std::round((double)value * parentwidth / 100));
                case BasisPoint:
                    return int(std::round((double)value * parentwidth / 10000));
                case Fractions:
                    return int(std::floor((double)value * parentwidth / fractions));
                case MilliPixel:
                    return int(std::round((double)value / 1000));
                case EM:
                    return int(std::round(value * emwidth / 100));
                case UnitSize:
                    return value * (unitsize + spacing);
                case MilliUnitSize:
                    return int(std::round((double)value * (unitsize + spacing) / 1000));
                case Spaces:
                    return value * spacing;
                case Pixel:
                default:
                    return value;
                }
            }
        }

        /// Returns the calculated dimension in pixels
        constexpr float CalculateFloat(float parentwidth, int unitsize, int spacing, float emwidth = 10, int fractions = 6, bool issize = false) const {
            if(issize) {
                switch(unit) {
                    case Percent:
                        return (float)value * parentwidth / 100.f;
                    case BasisPoint:
                        return (float)value * parentwidth / 10000.f;
                    case Fractions:
                        return (float)value * parentwidth / fractions;
                    case MilliPixel:
                        return (float)value / 1000;
                    case EM:
                        return (float)value * emwidth / 100.f;
                    case UnitSize:
                        if(!issize || (value >= -1 && value <= 1))
                            return float(value * unitsize);
                        return float(value * unitsize + (value - 1) * spacing);
                    case MilliUnitSize:
                        if(!issize || (value >= -1000 && value <= 1000))
                            return (float)value * unitsize / 1000;
                        return ((float)value * unitsize + (float)(value - 1000) * spacing) / 1000;
                    case Spaces:
                        return float(value * spacing);
                    case Pixel:
                    default:
                        return (float)value;
                }
            }
            else {
                switch(unit) {
                    case Percent:
                        return (float)value * parentwidth / 100.f;
                    case BasisPoint:
                        return (float)value * parentwidth / 10000.f;
                    case Fractions:
                        return (float)value * parentwidth / fractions;
                    case MilliPixel:
                        return (float)value / 1000;
                    case EM:
                        return (float)value * emwidth / 100.f;
                    case UnitSize:
                        return float(value * (unitsize + spacing));
                    case MilliUnitSize:
                        return (float)value * (unitsize + spacing) / 1000;
                    case Spaces:
                        return float(value * spacing);
                    case Pixel:
                    default:
                        return (float)value;
                }
            }
        }
        
        /// Returns if the dimension is relative to the parentwidth
        constexpr bool IsRelative() const {
            return unit == Percent || unit == BasisPoint || unit == Fractions;
        }

        /// Returns the value of the dimension, should not be considered as
        /// pixels
        constexpr int GetValue() const {
            return value;
        }

        /// Returns the unit of the dimension
        constexpr Unit GetUnit() const {
            return unit;
        }

        /// Changes the value of the dimension without modifying the units
        constexpr void Set(int value) {
            this->value = value;
        }

        /// Changes the value and unit of the dimension.
        constexpr void Set(int value, Unit unit) {
            this->value = value;
            this->unit = unit;
        }

        bool operator ==(const Dimension &other) const {
            return other.unit == unit && other.value == value;
        }

        bool operator !=(const Dimension &other) const {
            return !(*this == other);
        }

    protected:
        int value;
        Unit unit;
    };

    /// Similar to Dimension except default unit is UnitSize and supports floats and
    /// doubles for automatic conversion
    class UnitDimension : public Dimension {
    public:
        constexpr UnitDimension(int value = 0, Unit unit = UnitSize) : Dimension(value, unit) {
        }

        constexpr UnitDimension(double value, Unit unit = MilliUnitSize) : Dimension(value, unit) {
        }

        constexpr UnitDimension(float value, Unit unit = MilliUnitSize) : Dimension((double)value, unit) {
        }

        constexpr UnitDimension(const Dimension &d) : Dimension(d) { }

        bool operator ==(const UnitDimension &other) const {
            return other.unit == unit && other.value == value;
        }

        bool operator !=(const UnitDimension &other) const {
            return !(*this == other);
        }
    };

    /// This class stores the location information for a box object
    using Point = Geometry::basic_Point<Dimension>;

    /// This class stores the size information for a box object
    using Size = Geometry::basic_Size<Dimension>;

    /// This class stores the location information for a box object
    using UnitPoint = Geometry::basic_Point<UnitDimension>;

    /// This class stores the size information for a box object
    using UnitSize = Geometry::basic_Size<UnitDimension>;

    /// This class stores the margin information for a box object
    using Margin = Geometry::basic_Margin<Dimension>;
    
    /// Converts a dimension based point to pixel based point
    inline Geometry::Point Convert(const Point &p, const Geometry::Size &parent, int unitsize, int spacing, int emwidth = 10, int fractions = 6) {
        return {p.X(parent.Width, unitsize, spacing, emwidth, fractions), p.Y(parent.Height, unitsize, spacing, emwidth, fractions)};
    }

    /// Converts a dimension based size to pixel based size
    inline Geometry::Size Convert(const Size &s, const Geometry::Size &parent, int unitsize, int spacing, int emwidth = 10, int fractions = 6) {
        return {s.Width(parent.Width, unitsize, spacing, emwidth, fractions, true), s.Height(parent.Height, unitsize, spacing, emwidth, fractions, true)};
    }

    /// Converts a dimension based point to pixel based point
    inline Geometry::Point Convert(const UnitPoint &p, const Geometry::Size &parent, int unitsize, int spacing, int emwidth = 10, int fractions = 6) {
        return {p.X(parent.Width, unitsize, spacing, emwidth, fractions), p.Y(parent.Height, unitsize, spacing, emwidth, fractions)};
    }

    /// Converts a dimension based size to pixel based size
    inline Geometry::Size Convert(const UnitSize &s, const Geometry::Size &parent, int unitsize, int spacing, int emwidth = 10, int fractions = 6) {
        return {s.Width(parent.Width, unitsize, spacing, emwidth, fractions, true), s.Height(parent.Height, unitsize, spacing, emwidth, fractions, true)};
    }

    /// Converts a dimension based margin to pixel based margin
    inline Geometry::Margin Convert(const Margin &m, const Geometry::Size &parent, int unitsize, int spacing, int emwidth = 10, int fractions = 6) {
        return {m.Left(parent.Width, unitsize, spacing, emwidth, fractions), m.Top(parent.Height, unitsize, spacing, emwidth, fractions), m.Right(parent.Width, unitsize, spacing, emwidth, fractions), m.Bottom(parent.Height, unitsize, spacing, emwidth, fractions), };
    }

    /// Converts the given value to dimension with pixel units
    inline constexpr Dimension Pixels(int val) {
        return {val, Dimension::Pixel};
    }

    /// Converts the given value to dimension with percentage units
    inline constexpr Dimension Percentage(int val) {
        return {val, Dimension::Percent};
    }

    /// Converts the given value to dimension with percentage units
    inline constexpr Dimension Percentage(double val) {
        return {val, Dimension::Percent};
    }

    /// Converts the given value to dimension with percentage units
    inline constexpr Dimension Percentage(float val) {
        return {val, Dimension::Percent};
    }

    /// Converts the given value to dimension with fractions units
    inline constexpr Dimension Fractions(int val) {
        return {val, Dimension::Fractions};
    }

    /// Converts the given value to dimension with unit size
    inline constexpr Dimension Units(int val) {
        return {val, Dimension::UnitSize};
    }

    /// Converts the given value to dimension with unit size
    inline constexpr Dimension Units(float val) {
        return {val, Dimension::MilliUnitSize};
    }

    /// Converts the given value to dimension with unit size
    inline constexpr Dimension Units(double val) {
        return {val, Dimension::MilliUnitSize};
    }

    class DualDimension {
    public:
        Dimension one, two;

        operator Point() const {
            return {one, two};
        }

        operator UnitPoint() const {
            return {one, two};
        }

        operator Size() const {
            return {one, two};
        }

        operator UnitSize() const {
            return {one, two};
        }
    };

    /// Converts the given value to dimension with pixel units
    inline constexpr DualDimension Pixels(int one, int two) {
        return {{one, Dimension::Pixel}, {two, Dimension::Pixel}};
    }

    /// Converts the given value to dimension with percentage units
    inline constexpr DualDimension Percent(int one, int two) {
        return {{one, Dimension::Percent}, {two, Dimension::Percent}};
    }

    /// Converts the given one, given twoue to dimension with percentage units
    inline constexpr DualDimension Percent(double one, double two) {
        return {{one, Dimension::Percent}, {two, Dimension::Percent}};
    }

    /// Converts the given one, given twoue to dimension with percentage units
    inline constexpr DualDimension Percent(float one, float two) {
        return {{one, Dimension::Percent}, {two, Dimension::Percent}};
    }

    /// Converts the given value to dimension with percentage units
    inline constexpr DualDimension Fractions(int one, int two) {
        return {{one, Dimension::Fractions}, {two, Dimension::Fractions}};
    }

    /// Converts the given one, given twoue to dimension with unit size
    inline constexpr DualDimension Units(int one, int two) {
        return {{one, Dimension::UnitSize}, {two, Dimension::UnitSize}};
    }

    /// Converts the given one, given twoue to dimension with unit size
    inline constexpr DualDimension Units(float one, float two) {
        return {{one, Dimension::MilliUnitSize}, {two, Dimension::MilliUnitSize}};
    }

    /// Converts the given one, given twoue to dimension with unit size
    inline constexpr DualDimension Units(double one, double two) {
        return {{one, Dimension::MilliUnitSize}, {two, Dimension::MilliUnitSize}};
    }

    /// Converts the given value to dimension with pixel units
    inline Point Pixels(const Geometry::Point &val) {
        return {{val.X, Dimension::Pixel}, {val.Y, Dimension::Pixel}};
    }

    /// Converts the given value to dimension with percentage units
    inline Point Percentage(const Geometry::Point &val) {
        return {{val.X, Dimension::Percent}, {val.Y, Dimension::Percent}};
    }

    /// Converts the given value to dimension with percentage units
    inline Point Fractions(const Geometry::Point &val) {
        return {{val.X, Dimension::Fractions}, {val.Y, Dimension::Fractions}};
    }

    /// Converts the given value to dimension with unit size
    inline Point Units(const Geometry::Point &val) {
        return {{val.X, Dimension::UnitSize}, {val.Y, Dimension::UnitSize}};
    }

    /// Converts the given value to dimension with pixel units
    inline Size Pixels(const Geometry::Size &val) {
        return {{val.Width, Dimension::Pixel}, {val.Height, Dimension::Pixel}};
    }

    /// Converts the given value to dimension with percentage units
    inline Size Percentage(const Geometry::Size &val) {
        return {{val.Width, Dimension::Percent}, {val.Height, Dimension::Percent}};
    }

    /// Converts the given value to dimension with percentage units
    inline Size Fractions(const Geometry::Size &val) {
        return {{val.Width, Dimension::Fractions}, {val.Height, Dimension::Fractions}};
    }

    /// Converts the given value to dimension with unit size
    inline Size Units(const Geometry::Size &val) {
        return {{val.Width, Dimension::UnitSize}, {val.Height, Dimension::UnitSize}};
    }

    namespace literals {
        inline Dimension operator""_px(unsigned long long val) {
            return Pixels(int(val));
        }

        inline Dimension operator""_px(long double val) {
            return {(double)val, Dimension::MilliPixel};
        }

        inline Dimension operator""_perc(unsigned long long val) {
            return Percentage(int(val));
        }

        inline Dimension operator""_perc(long double val) {
            return Percentage(double(val));
        }

        inline Dimension operator""_u(unsigned long long val) {
            return Units(int(val));
        }

        inline Dimension operator""_u(long double val) {
            return Units(double(val));
        }

        inline Dimension operator""_em(unsigned long long val) {
            return {int(val)*100, Dimension::EM};
        }

        inline Dimension operator""_em(long double val) {
            return {double(val), Dimension::EM};
        }

        inline Dimension operator""_spcs(unsigned long long val) {
            return {int(val), Dimension::Spaces};
        }

        inline Dimension operator""_fr(unsigned long long val) {
            return {int(val), Dimension::Fractions};
        }

    }

}
