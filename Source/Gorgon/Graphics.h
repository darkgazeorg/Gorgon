#pragma once

#include <cmath>
#include <stdint.h>
#include <string.h>

#include "GL.h"
#include "Geometry/Point.h"
#include "Geometry/Size.h"
#include "Geometry/Rectangle.h"

#include "Graphics/Color.h"

namespace Gorgon {


    /// Contains generic 2D graphics related data structures and functions. These functions are tied to
    /// underlying GL system through textures.
    namespace Graphics {

        /// Initializes Graphics module, should be performed after an OpenGL context is
        /// created. There is a mechanism to ensure initialization is performed once.
        void Initialize();

        /// Details which directions a texture should tile. If its not tiled for that direction, it will
        /// be stretched. If the target size is smaller, tiling causes partial draw instead of shrinking.
        /// @todo Should be fitted to String::Enum
        enum class Tiling {
            None		= 0,
            Horizontal	= 1,
            Vertical	= 2,
            Both		= 3,
        };

        /// 2D orientation constants
        enum class Orientation {
            Horizontal = 1,
            Vertical   = 2
        };

        /// Creates a Tiling class from the given horizontal, vertical tiling info.
        inline Tiling Tile(bool horizontal, bool vertical) {
            return (horizontal ?
                (vertical ? Tiling::Both     : Tiling::Horizontal) :
                (vertical ? Tiling::Vertical : Tiling::None)
            );
        }

        /// Defines how an object is aligned
        enum class Alignment {
            /// Placed at the start of the axis
            Start		= 1,

            /// Centered along the axis
            Center		= 4,

            /// Placed at the end of the axis
            End			= 2,
        };


        /// Defines how a text is aligned. Justification should be used as an independent
        /// flag as a text could both be justified and centered (for partial lines).
        enum class TextAlignment {
            /// Text is aligned to left
            Left		= 8,

            /// Text is aligned to center
            Center		= 32,

            /// Text is aligned to right
            Right		= 16,
        };

        DefineEnumStrings(TextAlignment, {
            {TextAlignment::Left,   "Left"},
            {TextAlignment::Center, "Center"},
            {TextAlignment::Right, "Right"},
        });

        /// Defines how an object is placed in a 2D axis system
        enum class Placement {
            /// Placed at top left
            TopLeft			=  9,

            /// Placed at top center
            TopCenter		= 33,

            /// Placed at top right
            TopRight		= 17,


            /// Placed at middle left
            MiddleLeft		= 12,

            /// Placed at the center
            MiddleCenter	= 36,

            /// Placed at middle right
            MiddleRight		= 20,


            /// Placed at bottom
            BottomLeft		= 10,

            /// Placed at bottom center
            BottomCenter	= 34,

            /// Placed at bottom right
            BottomRight		= 18,
        };

        DefineEnumStrings(Placement, {
            {Placement::TopLeft,        "Top left"},
            {Placement::TopCenter,      "Top center"},
            {Placement::TopRight,       "Top right"},
            {Placement::MiddleLeft,     "Middle left"},
            {Placement::MiddleCenter,   "Middle center"},
            {Placement::MiddleRight,    "Middle right"},
            {Placement::BottomLeft,     "Bottom left"},
            {Placement::BottomCenter,   "Bottom center"},
            {Placement::BottomRight,    "Bottom right"},
        });

        /// Returns horizontal alignment from a placement
        inline Alignment GetHorizontal(Placement placement) {
            return Alignment(((int)placement & 56)>>3);
        }

        /// Returns vertical alignment from a placement
        inline Alignment GetVertical(Placement placement) {
            return Alignment((int)placement & 7);
        }

        /// Returns the offset of the object according to the given placement rule when there is the given
        /// remainder between object size and the area its being drawn on. Typical usage:
        /// `CalculateOffset(Placement::MiddleCenter, areasize - objectsize)`
        template<class T_>
        Geometry::basic_Point<T_> CalculateOffset(Placement place, Geometry::basic_Size<T_> remainder) {
            switch(GetHorizontal(place)) {
            case Alignment::Start:
                remainder.Width=0;
                break;
            case Alignment::Center:
                remainder.Width/=2;
                break;
            case Alignment::End:
                break;
#ifndef NDEBUG
            default:
                throw std::runtime_error("Unknown mode");
                break;
#endif
            }

            switch(GetVertical(place)) {
            case Alignment::Start:
                remainder.Height=0;
                break;
            case Alignment::Center:
                remainder.Height/=2;
                break;
            case Alignment::End:
                break;
#ifndef NDEBUG
            default:
                throw std::runtime_error("Unknown mode");
                break;
#endif
            }

            return (Geometry::basic_Point<T_>)remainder;
        }

        inline Geometry::Rectangle ShrinkFit(Placement place, const Geometry::Size &obj, const Geometry::Size &target) {
            auto factor = Geometry::Sizef(target) / obj;
            auto aspect = (float)obj.Width / obj.Height;
            if(target.Width < obj.Width || target.Height < obj.Height) {
                auto nsize = (factor.Width > factor.Height) ?
                    Geometry::Sizef{target.Height * aspect, (float)target.Height} :
                    Geometry::Sizef{(float)target.Width, target.Width / aspect};

                auto offset = CalculateOffset<float>(place, Geometry::Sizef{target} - nsize);
                return {offset, nsize};
            }
            else {
                auto offset = CalculateOffset<float>(place, target - obj);
                return {offset, obj};
            }
        }

        /// This class allows control over a sizable object
        class SizeController {
        public:
            /// Controls how a direction is tiled
            enum Tiling {
                /// The drawing is drawn only once with its original size. The placement of this single
                /// drawing will be determined by the alignment.
                Single				= 0,

                /// The drawing is stretched along this axis to cover the given size. If the given drawing
                /// object is truly scalable, this value acts same as Tile.
                Stretch				= 2,

                /// The drawing is tiled along this axis to cover the given size. If the area is smaller,
                /// drawing will be cut from the given size. If the area is larger, drawing will be repeated
                /// as necessary. While repeating, it is possible for drawing not to cover the area exactly,
                /// In this case, alignment determines which side will be incomplete. If center alignment
                /// is selected, both sides will have same amount of incomplete drawing.
                Tile				= 1,

                /// The drawing is tiled along this axis to cover the given size. In this mode, the drawing will
                /// never placed incomplete. Instead, it will be repeated to cover as much area as possible without
                /// exceeding the given size. Any area that is left will be distributed to the edges according to
                /// the selected alignment.
                Integral_Smaller	= 13,

                /// The drawing is tiled along this axis to cover the given size. In this mode, the drawing will
                /// never placed incomplete. Instead, it will be repeated to cover entire area. Excess drawing
                /// will be aligned depending on the selected alignment.
                Integral_Fill		= 21,

                /// The drawing is tiled along this axis to cover the given size. In this mode, the drawing will
                /// never placed incomplete. Instead, it will be repeated to cover entire area. If the last drawing
                /// that will be partial is more than 50% of the size of the original drawing, it will be drawn. Excess
                /// drawing or the area left empty is distributed according to the selected alignment.
                Integral_Best		= 45,
            };

            /// Creates a default size controller which tiles the object from top-left
            SizeController() : Horizontal(Tile), Vertical(Tile), Place(Placement::TopLeft) { }

            /// Creates a size controller that places a single object to the given placement. This is an implicit conversion
            /// constructor.
            SizeController(Placement p) : Horizontal(Single), Vertical(Single), Place(p) { }

            /// Creates a new size controller with the given tiling options and placement
            SizeController(Tiling h, Tiling v, Placement p=Placement::TopLeft) : Horizontal(h), Vertical(v), Place(p) { }

            /// Creates a new size controller with the given tiling options and placement
            SizeController(Graphics::Tiling tile, Placement p=Placement::TopLeft) :
                Horizontal(int(tile)&int(Graphics::Tiling::Horizontal) ? Tile : Stretch),
                Vertical  (int(tile)&int(Graphics::Tiling::Vertical)   ? Tile : Stretch),
                Place(p)
            { }

            /// Calculates the size of the object according to the tiling rules
            Geometry::Size CalculateSize(Geometry::Size objectsize, const Geometry::Size &area) const {
                switch(Horizontal) {
                case Integral_Smaller:
                    objectsize.Width=(area.Width/objectsize.Width)*objectsize.Width;
                    break;
                case Integral_Fill:
                    objectsize.Width=int(std::ceil(float(area.Width)/objectsize.Width))*objectsize.Width;
                    break;
                case Integral_Best:
                    objectsize.Width=int(std::round(float(area.Width)/objectsize.Width))*objectsize.Width;
                    break;
                case Stretch:
                case Tile:
                    objectsize.Width=area.Width;
                    break;
                case Single:
                    break;
#ifndef NDEBUG
                default:
                    throw std::runtime_error("Unknown mode");
                    break;
#endif
                }

                switch(Vertical) {
                case Integral_Smaller:
                    objectsize.Width=(area.Width/objectsize.Width)*objectsize.Width;
                    break;
                case Integral_Fill:
                    objectsize.Width=int(std::ceil(float(area.Width)/objectsize.Width))*objectsize.Width;
                    break;
                case Integral_Best:
                    objectsize.Width=int(std::round(float(area.Width)/objectsize.Width))*objectsize.Width;
                    break;
                case Stretch:
                case Tile:
                    objectsize.Height=area.Height;
                    break;
                case Single:
                    break;
#ifndef NDEBUG
                default:
                    throw std::runtime_error("Unknown mode");
                    break;
#endif
                }

                return objectsize;
            }

            /// Calculates the size of the object according to the tiling rules
            Geometry::Sizef CalculateSize(Geometry::Sizef objectsize, const Geometry::Sizef &area) const {
                switch(Horizontal) {
                    case Integral_Smaller:
                        objectsize.Width=floor(area.Width/objectsize.Width)*objectsize.Width;
                        break;
                    case Integral_Fill:
                        objectsize.Width=ceil(area.Width/objectsize.Width)*objectsize.Width;
                        break;
                    case Integral_Best:
                        objectsize.Width=round(area.Width/objectsize.Width)*objectsize.Width;
                        break;
                    case Stretch:
                    case Tile:
                        objectsize.Width=area.Width;
                        break;
                    case Single:
                        break;
#ifndef NDEBUG
                    default:
                        throw std::runtime_error("Unknown mode");
                        break;
#endif
                }

                switch(Vertical) {
                    case Integral_Smaller:
                        objectsize.Height=floor(area.Height/objectsize.Height)*objectsize.Height;
                        break;
                    case Integral_Fill:
                        objectsize.Height=ceil(area.Height/objectsize.Height)*objectsize.Height;
                        break;
                    case Integral_Best:
                        objectsize.Height=round(area.Height/objectsize.Height)*objectsize.Height;
                        break;
                    case Stretch:
                    case Tile:
                        objectsize.Height=area.Height;
                        break;
                    case Single:
                        break;
#ifndef NDEBUG
                    default:
                        throw std::runtime_error("Unknown mode");
                        break;
#endif
                }

                return objectsize;
            }


            /// Calculates the size of the object according to the tiling and placement rules
            Geometry::Point CalculateOffset(const Geometry::Size &objectsize, const Geometry::Size &area) const {
                return Graphics::CalculateOffset(Place, area-CalculateSize(objectsize, area));
            }


            /// Calculates the size of the object according to the tiling and placement rules
            Geometry::Pointf CalculateOffset(const Geometry::Sizef &objectsize, const Geometry::Sizef &area) const {
                return Graphics::CalculateOffset(Place, area-CalculateSize(objectsize, area));
            }

            /// Calculates the drawing area of the object according to the tiling and placement rules
            Geometry::Rectangle CalculateArea(const Geometry::Size &objectsize, const Geometry::Size &area) const {
                auto size=CalculateSize(objectsize, area);
                return{Graphics::CalculateOffset(Place, area-size), size};
            }

            /// Calculates the drawing area of the object according to the tiling and placement rules
            Geometry::Rectanglef CalculateArea(const Geometry::Sizef &objectsize, const Geometry::Sizef &area) const {
                auto size=CalculateSize(objectsize, area);
                return{Graphics::CalculateOffset(Place, area-size), size};
            }

            /// Calculates the size of the object according to the tiling rules
            Geometry::Size CalculateSize(Geometry::Size repeatingsize, const Geometry::Size &fixedsize, const Geometry::Size &area) const {
                switch(Horizontal) {
                case Integral_Smaller:
                    repeatingsize.Width=((area.Width-fixedsize.Width)/repeatingsize.Width)*repeatingsize.Width+fixedsize.Width;
                    break;
                case Integral_Fill:
                    repeatingsize.Width=int(std::ceil(float(area.Width-fixedsize.Width)/repeatingsize.Width))*repeatingsize.Width+fixedsize.Width;
                    break;
                case Integral_Best:
                    repeatingsize.Width=int(std::round(float(area.Width-fixedsize.Width)/repeatingsize.Width))*repeatingsize.Width+fixedsize.Width;
                    break;
                case Stretch:
                case Tile:
                    repeatingsize.Width=area.Width;
                    break;
                case Single:
                    break;
#ifndef NDEBUG
                default:
                    throw std::runtime_error("Unknown mode");
                    break;
#endif
                }

                switch(Vertical) {
                case Integral_Smaller:
                    repeatingsize.Height=((area.Height-fixedsize.Height)/repeatingsize.Height)*repeatingsize.Height+fixedsize.Height;
                    break;
                case Integral_Fill:
                    repeatingsize.Height=int(std::ceil(float(area.Height-fixedsize.Height)/repeatingsize.Height))*repeatingsize.Height+fixedsize.Height;
                    break;
                case Integral_Best:
                    repeatingsize.Height=int(std::round(float(area.Height-fixedsize.Height)/repeatingsize.Height))*repeatingsize.Height+fixedsize.Height;
                    break;
                case Stretch:
                case Tile:
                    repeatingsize.Height=area.Height;
                    break;
                case Single:
                    break;
#ifndef NDEBUG
                default:
                    throw std::runtime_error("Unknown mode");
                    break;
#endif
                }

                return repeatingsize;
            }

            /// Calculates the size of the object according to the tiling rules
            Geometry::Sizef CalculateSize(Geometry::Sizef repeatingsize, const Geometry::Sizef &fixedsize, const Geometry::Sizef &area) const {
                switch(Horizontal) {
                    case Integral_Smaller:
                        repeatingsize.Width=((area.Width-fixedsize.Width)/repeatingsize.Width)*repeatingsize.Width+fixedsize.Width;
                        break;
                    case Integral_Fill:
                        repeatingsize.Width=area.Width-fixedsize.Width/repeatingsize.Width*repeatingsize.Width+fixedsize.Width;
                        break;
                    case Integral_Best:
                        repeatingsize.Width=area.Width-fixedsize.Width/repeatingsize.Width*repeatingsize.Width+fixedsize.Width;
                        break;
                    case Stretch:
                    case Tile:
                        repeatingsize.Width=area.Width;
                        break;
                    case Single:
                        break;
#ifndef NDEBUG
                    default:
                        throw std::runtime_error("Unknown mode");
                        break;
#endif
                }

                switch(Vertical) {
                    case Integral_Smaller:
                        repeatingsize.Width=((area.Width-fixedsize.Width)/repeatingsize.Width)*repeatingsize.Width+fixedsize.Width;
                        break;
                    case Integral_Fill:
                        repeatingsize.Width=area.Width-fixedsize.Width/repeatingsize.Width*repeatingsize.Width+fixedsize.Width;
                        break;
                    case Integral_Best:
                        repeatingsize.Width=area.Width-fixedsize.Width/repeatingsize.Width*repeatingsize.Width+fixedsize.Width;
                        break;
                    case Stretch:
                    case Tile:
                        repeatingsize.Height=area.Height;
                        break;
                    case Single:
                        break;
#ifndef NDEBUG
                    default:
                        throw std::runtime_error("Unknown mode");
                        break;
#endif
                }

                return repeatingsize;
            }

            /// Calculates the size of the object according to the tiling and placement rules
            Geometry::Point CalculateOffset(const Geometry::Size &repeatingsize, const Geometry::Size &fixedsize, const Geometry::Size &area) const {
                return Graphics::CalculateOffset(Place, area-CalculateSize(repeatingsize, fixedsize, area));
            }

            /// Calculates the size of the object according to the tiling and placement rules
            Geometry::Pointf CalculateOffset(const Geometry::Sizef &repeatingsize, const Geometry::Sizef &fixedsize, const Geometry::Sizef &area) const {
                return Graphics::CalculateOffset(Place, area-CalculateSize(repeatingsize, fixedsize, area));
            }

            /// Calculates the drawing area of the object according to the tiling and placement rules
            Geometry::Rectangle CalculateArea(const Geometry::Size &repeatingsize, const Geometry::Size &fixedsize, const Geometry::Size &area) const {
                auto size=CalculateSize(repeatingsize, fixedsize, area);
                return{Graphics::CalculateOffset(Place, area-size), size};
            }

            /// Calculates the drawing area of the object according to the tiling and placement rules
            Geometry::Rectangle CalculateArea(const Geometry::Sizef &repeatingsize, const Geometry::Sizef &fixedsize, const Geometry::Sizef &area) const {
                auto size=CalculateSize(repeatingsize, fixedsize, area);
                return{Graphics::CalculateOffset(Place, area-size), size};
            }

            Graphics::Tiling GetTiling() const {
                return Graphics::Tile(Horizontal!=Single && Horizontal!=Stretch, Vertical!=Single && Vertical!=Stretch);
            }

            /// Horizontal tiling mode
            Tiling				Horizontal;

            /// Vertical tiling mode
            Tiling				Vertical;

            /// Placement method
            Placement	Place;
        };

        /// This interface represents a GL texture source.
        class TextureSource {
        public:
            /// Should return GL::Texture to be drawn. This could be 0 to denote no texture is to be used.
            virtual GL::Texture GetID() const = 0;
            
            virtual ColorMode GetMode() const = 0;

            /// Should return the size of the image stored in texture. Not the whole texture size.
            virtual Geometry::Size GetImageSize() const = 0;

            /// Should return the coordinates of the texture to be used
            virtual const Geometry::Pointf *GetCoordinates() const = 0;

            /// Returns whether this texture uses only a part of the GL::Texture. This indicates that the tiling
            /// operations should be performed without texture repeating method.
            bool IsPartial() const {
                return memcmp(GetCoordinates(), fullcoordinates, sizeof(fullcoordinates))!=0;
            }

        protected:
            /// Coordinates that selects the entire texture to be used
            static const Geometry::Pointf fullcoordinates[4];
        };

        namespace internal {


            extern GL::Texture LastTexture;

            void ActivateQuadVertices();
            void DrawQuadVertices();

        }
    }
}
