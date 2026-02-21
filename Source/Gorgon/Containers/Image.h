#pragma once

#include <fstream>
#include <vector>

#include "../Types.h"
#include "../Geometry/Point.h"
#include "../Geometry/Size.h"
#include "../Geometry/Bounds.h"
#include "../Graphics/Color.h"
#include "../IO/Stream.h"

namespace Gorgon :: Containers {
        
        enum class InterpolationMethod {
            None,
            NearestNeighbor = None,
            Linear,
            Bilinear = Linear,
            Cubic,
            Bicubic = Cubic,
        };

        template<class T_, class V_ = void>
        struct FixImageValue;

        template<class T_>
        struct FixImageValue<T_, typename std::enable_if<std::is_integral<T_>::value>::type> {
            static T_ Fix(float val) {
                return T_(Clamp<float>(std::round(val), std::numeric_limits<T_>::lowest(), std::numeric_limits<T_>::max()));
            }
        };

        template<>
        struct FixImageValue<float, void> {
            static float Fix(float val) {
                return val;
            }
        };

        /// This class is a container for image data. It supports different color modes and access to the
        /// underlying data through () operator. This object implements move semantics. Since copy constructor is
        /// expensive, it is deleted against accidental use. If a copy of the object is required, use Duplicate function.
        /// TODO Separate non-rgba related images, export/import with rgbaf
        template<class T_>
        class basic_Image {
        public:

            /// Constructs an empty image data
            basic_Image() {
            }

            /// Constructs a new image data with the given width, height and color mode. This constructor 
            /// does not initialize data inside the image
            basic_Image(const Geometry::Size &size, Graphics::ColorMode mode) : size(size), mode(mode) {
                cpp=Graphics::GetChannelsPerPixel(mode);
                data=(Byte*)malloc(size.Area()*cpp*sizeof(T_));

                alphaloc = Graphics::HasAlpha(mode) ? Graphics::GetAlphaIndex(mode) : -1;
            }

            /// Copy constructor is disabled
            basic_Image(const basic_Image &) = delete;

            /// Move constructor
            basic_Image(basic_Image &&data) : basic_Image() {
                Swap(data);
            }

            /// Copy assignment is disabled
            basic_Image &operator=(const basic_Image &) = delete;

            /// Move assignment
            basic_Image &operator=(basic_Image &&other) { 
                if(this == &other) return *this;
                
                Destroy();
                Swap(other);
                
                return *this;
            }

            /// Destructor
            ~basic_Image() {
                Destroy();
            }

            /// Duplicates this image, essentially performing the work of copy constructor
            basic_Image Duplicate() const {
                basic_Image n;
                n.Assign(data, size, mode);

                return n;
            }

            /// Resizes the image to the given size and color mode. This function discards the contents
            /// of the image and does not perform any initialization.
            void Resize(const Geometry::Size &size, Graphics::ColorMode mode) {
#ifndef NDEBUG
                if(!size.IsValid())
                    throw std::runtime_error("basic_Image size cannot be negative");
#endif

                // Check if resize is really necessary
                if(this->size==size && this->cpp==Graphics::GetChannelsPerPixel(mode))
                    return;

                this->size     = size;
                this->mode     = mode;
                this->cpp      = Graphics::GetChannelsPerPixel(mode);
                this->alphaloc = Graphics::HasAlpha(mode) ? Graphics::GetAlphaIndex(mode) : -1;

                if(data) {
                    free(data);
                }
                
                data=(Byte*)malloc(size.Area()*cpp*sizeof(T_));
            }

            /// Assigns the image to the copy of the given data. Ownership of the given data
            /// is not transferred. If the given data is not required elsewhere, consider using
            /// Assume function. This variant performs resize and copy at the same time. The given 
            /// data should have the size of width*height*Graphics::GetBytesPerPixel(mode)*sizeof(T_). 
            /// This function does not perform any checks for the data size while copying it.
            /// If width or height is 0, the newdata is not accessed and this method effectively
            /// Destroys the current image. In this case, both width and height should be specified as 0.
            void Assign(Byte *newdata, const Geometry::Size &size, Graphics::ColorMode mode) {
#ifndef NDEBUG
                if(!size.IsValid())
                    throw std::runtime_error("basic_Image size cannot be negative");
#endif
                this->size     = size;
                this->mode     = mode;
                this->cpp      = Graphics::GetChannelsPerPixel(mode);
                this->alphaloc = Graphics::HasAlpha(mode) ? Graphics::GetAlphaIndex(mode) : -1;

                if(data && data!=newdata) {
                    free(data);
                }
                
                if(size.Area()*cpp>0) {
                    data=(Byte*)malloc(size.Area()*cpp*sizeof(T_));
                    memcpy(data, newdata, size.Area()*cpp*sizeof(T_));
                }
                else {
                    data=nullptr;
                }
            }

            /// Assigns the image to the copy of the given data. Ownership of the given data
            /// is not transferred. If the given data is not required elsewhere, consider using
            /// Assume function. The size and color mode of the image stays the same. The given 
            /// data should have the size of width*height*Graphics::GetBytesPerPixel(mode)*sizeof(T_). 
            /// This function does not perform any checks for the data size while copying it.t
            void Assign(Byte *newdata) {
                memcpy(data, newdata, size.Area()*cpp*sizeof(T_));
            }

            /// Assumes the ownership of the given data. This variant changes the size and
            /// color mode of the image. The given data should have the size of 
            /// width*height*Graphics::GetBytesPerPixel(mode)*sizeof(T_). This function
            /// does not perform any checks for the data size while assuming it.
            /// newdata could be nullptr however, in this case
            /// width, height should be 0. mode is not assumed to be ColorMode::Invalid while
            /// the image is empty, therefore it could be specified as any value.
            void Assume(Byte *newdata, const Geometry::Size &size, Graphics::ColorMode mode) {
#ifndef NDEBUG
                if(!size.IsValid())
                    throw std::runtime_error("basic_Image size cannot be negative");
#endif
                this->size   = size;
                this->mode   = mode;
                this->cpp    = Graphics::GetChannelsPerPixel(mode);
                this->alphaloc = Graphics::HasAlpha(mode) ? Graphics::GetAlphaIndex(mode) : -1;

                if(data && data!=newdata) {
                    free(data);
                }
                
                data=newdata;
            }

            /// Assumes the ownership of the given data. The size and color mode of the image stays the same.
            /// The given data should have the size of width*height*Graphics::GetBytesPerPixel(mode)*sizeof(T_).
            /// This function does not perform any checks for the data size while assuming it.
            void Assume(Byte *newdata) {
                if(data && data!=newdata) {
                    free(data);
                }
                
                data=newdata;
            }

            /// Returns and disowns the current data buffer. If image is empty, this method will return a nullptr.
            Byte *Release() {
                auto temp=data;
                data=nullptr;
                Destroy();

                return temp;
            }

            /// Cleans the contents of the buffer by setting every byte it contains to 0.
            void Clear() {
#ifndef NDEBUG
                if(!data) {
                    throw std::runtime_error("basic_Image data is empty");
                }
#endif

                memset(data, 0, size.Area()*cpp*sizeof(T_));
            }

            /// Destroys this image by setting width and height to 0 and freeing the memory
            /// used by its data. Also color mode is set to ColorMode::Invalid
            void Destroy() {
                if(data) {
                    free(data);
                    data=nullptr;
                }
                size   = {0, 0};
                cpp    = 0;
                mode   = Graphics::ColorMode::Invalid;
            }

            /// Swaps this image with another. This function is used to implement move semantics.
            void Swap(basic_Image &other) {
                using std::swap;

                swap(data,     other.data);
                swap(size,     other.size);
                swap(mode,     other.mode);
                swap(cpp,      other.cpp);
                swap(alphaloc, other.alphaloc);
            }

            /// Returns the raw data pointer
            Byte *RawData() {
                return data;
            }

            /// Returns the raw data pointer
            const Byte *RawData() const {
                return data;
            }

            /// Converts this image data to RGBA buffer
            void ConvertToRGBA() {
                if(!data) return;

                switch(mode) {
                case Graphics::ColorMode::BGRA:
                    for(int i=0; i<size.Area(); i++) {
                        std::swap(data[i*4+2], data[i*4+0]);
                    }
                    break;

                case Graphics::ColorMode::Grayscale_Alpha: {
                    auto pdata = data;
                    data = (Byte*)malloc(size.Area()*4*sizeof(T_));

                    for(int i=0; i<size.Area(); i++) {
                        data[i*4+0] = pdata[i*2+0];
                        data[i*4+1] = pdata[i*2+0];
                        data[i*4+2] = pdata[i*2+0];
                        data[i*4+3] = pdata[i*2+1];
                    }
                    delete pdata;
                }
                break;

                case Graphics::ColorMode::Grayscale: {
                    auto pdata = data;
                    data = (Byte*)malloc(size.Area()*4*sizeof(T_));

                    for(int i=0; i<size.Area(); i++) {
                        data[i*4+0] = pdata[i+0];
                        data[i*4+1] = pdata[i+0];
                        data[i*4+2] = pdata[i+0];
                        data[i*4+3] = 255;
                    }
                    delete pdata;
                }
                break;

                case Graphics::ColorMode::Alpha: {
                    auto pdata = data;
                    data = (Byte*)malloc(size.Area()*4*sizeof(T_));

                    for(int i=0; i<size.Area(); i++) {
                        data[i*4+0] = 255;
                        data[i*4+1] = 255;
                        data[i*4+2] = 255;
                        data[i*4+3] = pdata[i+0];
                    }
                    delete pdata;
                }
                break;

                case Graphics::ColorMode::RGB: {
                    auto pdata = data;
                    data = (Byte*)malloc(size.Area()*4*sizeof(T_));

                    for(int i=0; i<size.Area(); i++) {
                        data[i*4+0] = pdata[i*3+0];
                        data[i*4+1] = pdata[i*3+1];
                        data[i*4+2] = pdata[i*3+2];
                        data[i*4+3] = 255;
                    }
                    delete pdata;
                }
                break;

                case Graphics::ColorMode::BGR: {
                    auto pdata = data;
                    data = (Byte*)malloc(size.Area()*4*sizeof(T_));

                    for(int i=0; i<size.Area(); i++) {
                        data[i*4+0] = pdata[i*3+2];
                        data[i*4+1] = pdata[i*3+1];
                        data[i*4+2] = pdata[i*3+0];
                        data[i*4+3] = 255;
                    }
                    delete pdata;
                }
                break;
                
                case Graphics::ColorMode::RGBA:
                    //do nothing
                    break;
                    
                default:
                    throw std::runtime_error("Invalid color mode");

                }

                mode = Graphics::ColorMode::RGBA;
            }
            
            // cppcheck-suppress constParameter
            /// Copies data from one image to another. This operation does not perform
            /// blending. Additionally, color modes should be the same. However, this
            /// function will do clipping for overflows. Do not use negative values for
            /// target. Will return false if nothing is copied.
            bool CopyTo(basic_Image &dest, Geometry::Point target = {0, 0}) const {
                if(dest.GetMode() != mode || size.Area() == 0 || dest.GetSize().Area() == 0) 
                    return false;
                
                if(target.X > dest.GetWidth()) return false;
                
                if(target.Y > dest.GetHeight()) return false;
                
                int dw = dest.GetWidth(), dh = dest.GetHeight();
                Byte *dd = dest.RawData();
                const Byte *sd = RawData();
                
                for(int y=0; y<GetHeight(); y++) {
                    //out of pixels to copy
                    if(y+target.Y >= dh)
                        break;
                    
                    int si = y * cpp * size.Width;
                    int di = (y+target.Y) * cpp * dw + target.X * cpp;
                    
                    int cs = std::min(size.Width, dw - target.X) * cpp;
                    
                    memcpy(dd + di, sd + si, cs);
                }
                
                return true;
            }
            
            // cppcheck-suppress constParameter
            /// Copies data from one image to another. This operation does not perform
            /// blending. Additionally, color modes should be the same. However, this
            /// function will do clipping. Source bounds should be within the image.
            /// Will return false if nothing is copied.
            bool CopyTo(basic_Image &dest, Geometry::Bounds source, Geometry::Point target = {0, 0}) const {
                if(dest.GetMode() != mode || size.Area() == 0 || dest.GetSize().Area() == 0) 
                    return false;
                
                if(target.X < 0) {
                    source.Left -= target.X;
                    target.X = 0;
                }
                
                if(target.Y < 0) {
                    source.Top -= target.Y;
                    target.Y = 0;
                }
                
                if(target.X + source.Width() > dest.GetWidth()) {
                    source.Right -= target.X + source.Width() - dest.GetWidth();
                }
                if(target.Y + source.Height() > dest.GetHeight()) {
                    source.Bottom -= target.Y + source.Height() - dest.GetHeight();
                }
                
                if(source.Left >= source.Right) return false;
                
                if(source.Top  >= source.Bottom) return false;
                
                int dw = dest.GetWidth();
                int sw = source.Width();
                Byte *dd = dest.RawData();
                const Byte *sd = RawData();
                
                for(int y=source.Top; y<source.Bottom; y++) {
                    int si = (y * size.Width + source.Left) * cpp;
                    int di = ((y - source.Top + target.Y) * dw + target.X) * cpp;
                    
                    int cs = sw * cpp;
                    
                    memcpy(dd + di, sd + si, cs);
                }
                
                return true;
            }

            /// Copies this image to a RGBA buffer, buffer should be resize before calling this function
            void CopyToRGBABuffer(Byte *buffer) const {
                if(!data) return;

                int i;

                switch(mode) {
                    case Graphics::ColorMode::RGBA:
                        memcpy(buffer, data, size.Area()*4);
                        break;

                    case Graphics::ColorMode::BGRA:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i*4+0] = data[i*4+2];
                            buffer[i*4+1] = data[i*4+1];
                            buffer[i*4+2] = data[i*4+0];
                            buffer[i*4+3] = data[i*4+3];
                        }
                        break;

                    case Graphics::ColorMode::Grayscale_Alpha:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i*4+0] = data[i*2+0];
                            buffer[i*4+1] = data[i*2+0];
                            buffer[i*4+2] = data[i*2+0];
                            buffer[i*4+3] = data[i*2+1];
                        }
                        break;

                    case Graphics::ColorMode::Grayscale:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i*4+0] = data[i+0];
                            buffer[i*4+1] = data[i+0];
                            buffer[i*4+2] = data[i+0];
                            buffer[i*4+3] = 255;
                        }
                        break;

                    case Graphics::ColorMode::Alpha:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i*4+0] = 255;
                            buffer[i*4+1] = 255;
                            buffer[i*4+2] = 255;
                            buffer[i*4+3] = data[i+0];
                        }
                        break;

                    case Graphics::ColorMode::RGB: 
                        for(i=0; i<size.Area(); i++) {
                            buffer[i*4+0] = data[i*3+0];
                            buffer[i*4+1] = data[i*3+1];
                            buffer[i*4+2] = data[i*3+2];
                            buffer[i*4+3] = 255;
                        }
                        break;

                    case Graphics::ColorMode::BGR:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i*4+0] = data[i*3+2];
                            buffer[i*4+1] = data[i*3+1];
                            buffer[i*4+2] = data[i*3+0];
                            buffer[i*4+3] = 255;
                        }
                        break;
                    
                    default:
                        throw std::runtime_error("Invalid mode");
                }
            }

            /// Copies this image to a RGBA buffer, buffer should be resize before calling this function.
            /// This function is here mostly to create icon for Win32
            void CopyToBGRABuffer(Byte *buffer) const {
                if(!data) return;

                int i;

                switch(mode) {
                    case Graphics::ColorMode::BGRA:
                        memcpy(buffer, data, size.Area()*4);
                        break;

                    case Graphics::ColorMode::RGBA:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i*4+0] = data[i*4+2];
                            buffer[i*4+1] = data[i*4+1];
                            buffer[i*4+2] = data[i*4+0];
                            buffer[i*4+3] = data[i*4+3];
                        }
                        break;

                    case Graphics::ColorMode::Grayscale_Alpha:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i*4+0] = data[i*2+0];
                            buffer[i*4+1] = data[i*2+0];
                            buffer[i*4+2] = data[i*2+0];
                            buffer[i*4+3] = data[i*2+1];
                        }
                        break;

                    case Graphics::ColorMode::Grayscale:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i*4+0] = data[i+0];
                            buffer[i*4+1] = data[i+0];
                            buffer[i*4+2] = data[i+0];
                            buffer[i*4+3] = 255;
                        }
                        break;

                    case Graphics::ColorMode::Alpha:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i*4+0] = 255;
                            buffer[i*4+1] = 255;
                            buffer[i*4+2] = 255;
                            buffer[i*4+3] = data[i+0];
                        }
                        break;

                    case Graphics::ColorMode::BGR: 
                        for(i=0; i<size.Area(); i++) {
                            buffer[i*4+0] = data[i*3+0];
                            buffer[i*4+1] = data[i*3+1];
                            buffer[i*4+2] = data[i*3+2];
                            buffer[i*4+3] = 255;
                        }
                        break;

                    case Graphics::ColorMode::RGB:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i*4+0] = data[i*3+2];
                            buffer[i*4+1] = data[i*3+1];
                            buffer[i*4+2] = data[i*3+0];
                            buffer[i*4+3] = 255;
                        }
                        break;
                    
                    default:
                        throw std::runtime_error("Invalid mode");

                }
            }
            /// Copies this image to a RGBA buffer, buffer should be resize before calling this function.
            /// This function is here mostly to create icon for X11
            void CopyToBGRABufferLong(unsigned long *buffer) const {
                if(!data) return;

                int i;

                switch(mode) {
                    case Graphics::ColorMode::BGRA:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i] = (data[i*4+0]<<0) | (data[i*4+1]<<8) | (data[i*4+2]<<16) | (data[i*4+3]<<24);
                        }
                        break;

                    case Graphics::ColorMode::RGBA:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i] = (data[i*4+2]<<0) | (data[i*4+1]<<8) | (data[i*4+0]<<16) | (data[i*4+3]<<24);
                        }
                        break;

                    case Graphics::ColorMode::Grayscale_Alpha:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i] = (data[i*2+0]<<0) | (data[i*2+0]<<8) | (data[i*2+0]<<16) | (data[i*2+1]<<24);
                        }
                        break;

                    case Graphics::ColorMode::Grayscale:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i] = (data[i*1+0]<<0) | (data[i*1+0]<<8) | (data[i*1+0]<<16) | (255<<24);
                        }
                        break;

                    case Graphics::ColorMode::Alpha: 
                        for(i=0; i<size.Area(); i++) {
                            buffer[i] = (255<<0) | (255<<8) | (255<<16) | (data[i*1+0]<<24);
                        }
                        break;

                    case Graphics::ColorMode::BGR: 
                        for(i=0; i<size.Area(); i++) {
                            buffer[i] = (data[i*3+0]<<0) | (data[i*3+1]<<8) | (data[i*3+2]<<16) | (255<<24);
                        }
                        break;

                    case Graphics::ColorMode::RGB:
                        for(i=0; i<size.Area(); i++) {
                            buffer[i] = (data[i*3+2]<<0) | (data[i*3+1]<<8) | (data[i*3+0]<<16) | (255<<24);
                        }
                        break;
                    
                    default:
                        throw std::runtime_error("Invalid mode");

                }
            }

            /// Shrinks the size of the image using integer area interpolation.
            basic_Image ShrinkMultiple(const Geometry::Size& factor) const {
                Geometry::Size newsize = {(int)std::ceil((float)size.Width/factor.Width), (int)std::ceil((float)size.Height/factor.Height)};
                basic_Image target(newsize, GetMode());

                for(int y=0; y<newsize.Height; y++) {
                    auto th = std::min(y*factor.Height+factor.Height, size.Height);
                    auto ys = y * factor.Height;

                    for(int x=0; x<newsize.Width; x++) {
                        auto tw = std::min(x*factor.Width+factor.Width, size.Width);
                        auto xs = x * factor.Width;

                        int   count = 0;
                        float sum[4]= {};

                        for(int yy=ys; yy<th; yy++) {
                            for(int xx=xs; xx<tw; xx++) {
                                count++;

                                for(unsigned c=0; c<cpp; c++) {
                                    sum[c] += operator()(xx, yy, c);
                                }
                            }
                        }

                        for(unsigned c=0; c<cpp; c++) {
                            target(x, y, c) = FixImageValue<T_>::Fix(sum[c] / count);
                        }
                    }
                }

                return target;
            }

            /// Scales this image to the given size. In the image is shrunk more than 2x its original size
            /// Area interpolation is used along with the specified interpolation method.
            basic_Image Scale(const Geometry::Size &newsize, InterpolationMethod method = InterpolationMethod::Cubic) const {
                basic_Image target(newsize, GetMode());

                float fx = (float)size.Width / newsize.Width;
                float fy = (float)size.Height / newsize.Height;

                if(fx > 2 || fy > 2) {
                    if(fx < 2)
                        fx = 1;
                    if(fy < 2)
                        fy = 1;

                    auto img = ShrinkMultiple({int(fx), int(fy)});

                    if(img.GetSize() == newsize)
                        return img;
                    else
                        return img.Scale(newsize, method);
                }

                if(method == InterpolationMethod::NearestNeighbor) {
                    float yy = fy/2;
                    for(int y=0; y<newsize.Height; y++) {
                        float xx = fx/2;
                        for(int x=0; x<newsize.Width; x++) {
                            for(unsigned c=0; c<cpp; c++) {
                                target(x, y, c) = operator()((int)xx, (int)yy, c);
                            }

                            xx += fx;
                        }

                        yy += fy;
                    }
                }
                else if(method == InterpolationMethod::Linear) {
                    float yy = 0.5f;
                    for(int y=0; y<newsize.Height; y++) {
                        float ly  = yy - (int)yy;
                        float ly1 = 1 - ly;
                        int   y1 = (int)yy;
                        int   y2 = y1 + 1;

                        y1 = y1 < 0 ? 0 : y1;
                        y2 = y2 >= size.Height ? size.Height - 1 : y2;

                        float xx = 0.5f;
                        for(int x=0; x<newsize.Width; x++) {
                            float lx  = xx - (int)xx;
                            float lx1 = 1 - lx;
                            int   x1 = (int)xx;
                            int   x2 = x1 + 1;

                            x1 = x1 < 0 ? 0 : x1;
                            x2 = x2 >= size.Width ? size.Width - 1 : x2;

                            for(unsigned c=0; c<cpp; c++) {
                                target(x, y, c) = FixImageValue<T_>::Fix(
                                    lx1 * ly1 * operator()(x1, y1, c) +
                                    lx  * ly1 * operator()(x2, y1, c) +
                                    lx  * ly  * operator()(x2, y2, c) +
                                    lx1 * ly  * operator()(x1, y2, c)
                                );
                            }

                            xx += fx;
                        }

                        yy += fy;
                    }
                }
                else if(method == InterpolationMethod::Cubic) {
                    float yy = 0.f;
                    int ys[4];
                    float wy[4], yf[4];
                    int xs[4];
                    float wx[4], xf[4];
                    const float a = -0.5;
                    for(int y=0; y<newsize.Height; y++) {
                        yf[1]  = yy - (int)yy;
                        yf[2] = 1 - yf[1];
                        yf[3] = 2 - yf[1];
                        yf[0] = 1 + yf[1];

                        for(int i=0; i<4; i++) {
                            if(i == 1 || i == 2) {
                                wy[i] = (a+2) * yf[i]*yf[i]*yf[i] - (a+3) * yf[i]*yf[i] + 1;
                            }
                            else {
                                wy[i] = a * yf[i]*yf[i]*yf[i] - 5*a * yf[i]*yf[i] + 8*a * yf[i] - 4*a;
                            }
                            ys[i]  = Clamp((int)yy-1 + i, 0, size.Height-1);
                        }

                        float xx = 0.f;
                        for(int x=0; x<newsize.Width; x++) {
                            xf[1]  = xx - (int)xx;
                            xf[2] = 1 - xf[1];
                            xf[3] = 2 - xf[1];
                            xf[0] = 1 + xf[1];

                            for(int i=0; i<4; i++) {
                                if(i == 1 || i == 2) {
                                    wx[i] = (a+2) * xf[i]*xf[i]*xf[i] - (a+3) * xf[i]*xf[i] + 1;
                                }
                                else {
                                    wx[i] = a * xf[i]*xf[i]*xf[i] - 5*a * xf[i]*xf[i] + 8*a * xf[i] - 4*a;
                                }
                                xs[i]  = Clamp((int)xx-1 + i, 0, size.Width-1);
                            }

                            for(unsigned c=0; c<cpp; c++) {
                                float v = 0;
                                for(int j=0; j<4; j++) {
                                    for(int i=0; i<4; i++) {
                                        v += wx[i] * wy[j] * operator()(xs[i], ys[j], c);
                                    }
                                }

                                target(x, y, c) = FixImageValue<T_>::Fix(v);
                            }

                            xx += fx;
                        }

                        yy += fy;
                    }
                }
                else {
                    throw std::runtime_error("Unknown interpolation method");
                }

                return target;
            }
            
            /// Rotates this image with the given angle.
            basic_Image Rotate(Float angle, InterpolationMethod method = InterpolationMethod::Cubic) const {
                return Rotate(angle, {size.Width/2.f, size.Height/2.f}, method);
            }

            /// Rotates this image with the given angle.
            basic_Image Rotate(Float angle, const Geometry::Pointf origin, InterpolationMethod method = InterpolationMethod::Cubic) const {
                Geometry::Boundsf bnds = {0,0, Geometry::Sizef(size)};
                Geometry::Rotate(bnds, angle, origin);
                Geometry::Bounds b{
                    (int)std::floor(bnds.Left)-1, (int)std::floor(bnds.Top)-1, 
                    (int)std::ceil(bnds.Right)+1, (int)std::ceil(bnds.Bottom)+1, 
                };

                auto newsize = b.GetSize();

                basic_Image target(newsize, GetMode());

                Float cosa = std::cos(-angle); //inverse transform
                Float sina = std::sin(-angle);

                if(method == InterpolationMethod::NearestNeighbor) {
                    for(int y=0; y<newsize.Height; y++) {
                        float yn = y - origin.Y + b.Top;

                        for(int x=0; x<newsize.Width; x++) {
                            for(unsigned c=0; c<cpp; c++) {
                                float xn = x - origin.X + b.Left;

                                target(x, y, c) = Get((int)std::round(xn * cosa - yn * sina + origin.X), (int)std::round(xn * sina + yn * cosa + origin.Y), c);
                            }
                        }
                    }
                }
                else if(method == InterpolationMethod::Linear) {
                    for(int y=0; y<newsize.Height; y++) {
                        float yn = y - origin.Y + b.Top;

                        for(int x=0; x<newsize.Width; x++) {
                            float xn = x - origin.X + b.Left;

                            float xx = xn * cosa - yn * sina + origin.X;
                            float yy = xn * sina + yn * cosa + origin.Y;

                            int   y1 = (int)std::floor(yy);
                            float ly  = yy - y1;
                            float ly1 = 1 - ly;
                            int   y2 = y1 + 1;

                            int   x1 = (int)std::floor(xx);
                            float lx  = xx - x1;
                            float lx1 = 1 - lx;
                            int   x2 = x1 + 1;

                            for(unsigned c=0; c<cpp; c++) {
                                target(x, y, c) = FixImageValue<T_>::Fix(
                                    lx1 * ly1 * Get(x1, y1, c) +
                                    lx  * ly1 * Get(x2, y1, c) +
                                    lx  * ly  * Get(x2, y2, c) +
                                    lx1 * ly  * Get(x1, y2, c)
                                );
                            }
                        }
                    }
                }
                else if(method == InterpolationMethod::Cubic) {
                    int ys[4];
                    float wy[4], yf[4];
                    int xs[4];
                    float wx[4], xf[4];
                    const float a = -0.5;
                    for(int y=0; y<newsize.Height; y++) {
                        float yn = y - origin.Y + b.Top;

                        for(int x=0; x<newsize.Width; x++) {
                            float xn = x - origin.X + b.Left;

                            float xx = xn * cosa - yn * sina + origin.X;
                            float yy = xn * sina + yn * cosa + origin.Y;

                            xf[1]  = xx - std::floor(xx);
                            xf[2] = 1 - xf[1];
                            xf[3] = 2 - xf[1];
                            xf[0] = 1 + xf[1];

                            for(int i=0; i<4; i++) {
                                if(i == 1 || i == 2) {
                                    wx[i] = (a+2) * xf[i]*xf[i]*xf[i] - (a+3) * xf[i]*xf[i] + 1;
                                }
                                else {
                                    wx[i] = a * xf[i]*xf[i]*xf[i] - 5*a * xf[i]*xf[i] + 8*a * xf[i] - 4*a;
                                }
                                xs[i]  = (int)floor(xx)-1 + i;
                            }

                            yf[1]  = yy - std::floor(yy);
                            yf[2] = 1 - yf[1];
                            yf[3] = 2 - yf[1];
                            yf[0] = 1 + yf[1];

                            for(int i=0; i<4; i++) {
                                if(i == 1 || i == 2) {
                                    wy[i] = (a+2) * yf[i]*yf[i]*yf[i] - (a+3) * yf[i]*yf[i] + 1;
                                }
                                else {
                                    wy[i] = a * yf[i]*yf[i]*yf[i] - 5*a * yf[i]*yf[i] + 8*a * yf[i] - 4*a;
                                }
                                ys[i]  = (int)floor(yy)-1 + i;
                            }

                            for(unsigned c=0; c<cpp; c++) {
                                float v = 0;
                                for(int j=0; j<4; j++) {
                                    for(int i=0; i<4; i++) {
                                        v += wx[i] * wy[j] * Get(xs[i], ys[j], c);
                                    }
                                }

                                target(x, y, c) = FixImageValue<T_>::Fix(v);
                            }
                        }
                    }
                }
                else {
                    throw std::runtime_error("Unknown interpolation method");
                }

                return target;
            }

            /// Rotates this image with the given angle.
            basic_Image SkewX(Float perpixel, InterpolationMethod method = InterpolationMethod::Cubic) const {
                return SkewX(perpixel, {0.f, 0.f}, method);
            }

            /// Rotates this image with the given angle.
            basic_Image SkewX(Float perpixel, const Geometry::Pointf origin, InterpolationMethod method = InterpolationMethod::Cubic) const {
                Geometry::Boundsf bnds = {0,0, Geometry::Sizef(size)};
                Geometry::SkewX(bnds, perpixel, origin);
                Geometry::Bounds b{
                    (int)std::floor(bnds.Left)-1, (int)std::floor(bnds.Top)-1, 
                    (int)std::ceil(bnds.Right)+1, (int)std::ceil(bnds.Bottom)+1, 
                };

                auto newsize = b.GetSize();

                basic_Image target(newsize, GetMode());

                if(method == InterpolationMethod::NearestNeighbor) {
                    for(int y=0; y<newsize.Height; y++) {
                        float yn = y - origin.Y + b.Top;

                        for(int x=0; x<newsize.Width; x++) {
                            for(unsigned c=0; c<cpp; c++) {
                                float xn = float(x + b.Left);

                                target(x, y, c) = Get((int)std::round(xn - yn * perpixel), y, c);
                            }
                        }
                    }
                }
                else if(method == InterpolationMethod::Linear) {
                    for(int y=0; y<newsize.Height; y++) {
                        float yn = y - origin.Y + b.Top;

                        for(int x=0; x<newsize.Width; x++) {
                            float xn = float(x + b.Left);

                            float xx = xn - yn * perpixel;

                            int   x1 = (int)std::floor(xx);
                            float lx  = xx - x1;
                            float lx1 = 1 - lx;
                            int   x2 = x1 + 1;

                            for(unsigned c=0; c<cpp; c++) {
                                target(x, y, c) = FixImageValue<T_>::Fix(
                                    lx1 * Get(x1, y, c) +
                                    lx  * Get(x2, y, c)
                                );
                            }
                        }
                    }
                }
                else if(method == InterpolationMethod::Cubic) {
                    int xs[4];
                    float wx[4], xf[4];
                    const float a = -0.5;
                    for(int y=0; y<newsize.Height; y++) {
                        float yn = y - origin.Y + b.Top;

                        for(int x=0; x<newsize.Width; x++) {
                            float xn = float(x + b.Left);

                            float xx = xn - yn * perpixel;

                            xf[1]  = xx - std::floor(xx);
                            xf[2] = 1 - xf[1];
                            xf[3] = 2 - xf[1];
                            xf[0] = 1 + xf[1];

                            for(int i=0; i<4; i++) {
                                if(i == 1 || i == 2) {
                                    wx[i] = (a+2) * xf[i]*xf[i]*xf[i] - (a+3) * xf[i]*xf[i] + 1;
                                }
                                else {
                                    wx[i] = a * xf[i]*xf[i]*xf[i] - 5*a * xf[i]*xf[i] + 8*a * xf[i] - 4*a;
                                }
                                xs[i]  = (int)floor(xx)-1 + i;
                            }

                            for(unsigned c=0; c<cpp; c++) {
                                float v = 0;
                                
                                for(int i=0; i<4; i++) {
                                    v += wx[i] * Get(xs[i], y, c);
                                }
                                target(x, y, c) = FixImageValue<T_>::Fix(v);
                            }
                        }
                    }
                }
                else {
                    throw std::runtime_error("Unknown interpolation method");
                }

                return target;
            }

            /// Rotates this image with the given angle.
            basic_Image SkewY(Float perpixel, InterpolationMethod method = InterpolationMethod::Cubic) const {
                return SkewY(perpixel, {0.f, 0.f}, method);
            }

            /// Rotates this image with the given angle.
            basic_Image SkewY(Float perpixel, const Geometry::Pointf origin, InterpolationMethod method = InterpolationMethod::Cubic) const {
                Geometry::Boundsf bnds = {0,0, Geometry::Sizef(size)};
                Geometry::SkewY(bnds, perpixel, origin);
                Geometry::Bounds b{
                    (int)std::floor(bnds.Left)-1, (int)std::floor(bnds.Top)-1, 
                    (int)std::ceil(bnds.Right)+1, (int)std::ceil(bnds.Bottom)+1, 
                };

                auto newsize = b.GetSize();

                basic_Image target(newsize, GetMode());

                if(method == InterpolationMethod::NearestNeighbor) {
                    for(int x=0; x<newsize.Width; x++) {
                        float xn = x - origin.X + b.Left;

                        for(int y=0; y<newsize.Height; y++) {
                            for(unsigned c=0; c<cpp; c++) {
                                float yn = float(y + b.Top);

                                target(x, y, c) = Get(x, (int)std::round(yn - xn * perpixel), c);
                            }
                        }
                    }
                }
                else if(method == InterpolationMethod::Linear) {
                    for(int x=0; x<newsize.Width; x++) {
                        float xn = x - origin.X + b.Left;

                        for(int y=0; y<newsize.Height; y++) {
                            float yn = float(y + b.Top);

                            float yy = yn - xn * perpixel;

                            int   y1 = (int)std::floor(yy);
                            float ly  = yy - y1;
                            float ly1 = 1 - ly;
                            int   y2 = y1 + 1;

                            for(unsigned c=0; c<cpp; c++) {
                                target(x, y, c) = FixImageValue<T_>::Fix(
                                    ly1 * Get(x, y1, c) +
                                    ly  * Get(x, y2, c)
                                );
                            }
                        }
                    }
                }
                else if(method == InterpolationMethod::Cubic) {
                    int ys[4];
                    float wy[4], yf[4];
                    const float a = -0.5;
                    for(int x=0; x<newsize.Width; x++) {
                        float xn = x - origin.X + b.Left;

                        for(int y=0; y<newsize.Height; y++) {
                            float yn = float(y + b.Top);

                            float yy = yn - xn * perpixel;

                            yf[1]  = yy - std::floor(yy);
                            yf[2] = 1 - yf[1];
                            yf[3] = 2 - yf[1];
                            yf[0] = 1 + yf[1];

                            for(int i=0; i<4; i++) {
                                if(i == 1 || i == 2) {
                                    wy[i] = (a+2) * yf[i]*yf[i]*yf[i] - (a+3) * yf[i]*yf[i] + 1;
                                }
                                else {
                                    wy[i] = a * yf[i]*yf[i]*yf[i] - 5*a * yf[i]*yf[i] + 8*a * yf[i] - 4*a;
                                }
                                ys[i]  = (int)floor(yy)-1 + i;
                            }

                            for(unsigned c=0; c<cpp; c++) {
                                float v = 0;
                                
                                for(int i=0; i<4; i++) {
                                    v += wy[i] * Get(x, ys[i], c);
                                }
                                target(x, y, c) = FixImageValue<T_>::Fix(v);
                            }
                        }
                    }
                }
                else {
                    throw std::runtime_error("Unknown interpolation method");
                }

                return target;
            }
            
            /// Mirrors this bitmap along X axis as a new one.
            basic_Image MirrorX() const {
                basic_Image target(size, GetMode());
                int yy = size.Height - 1;
                for(int y=0; y<size.Height; y++) {
                    for(int x=0; x<size.Width; x++) {
                        for(unsigned c=0; c<cpp; c++) {
                            target(x, y, c) = operator()(x, yy, c);
                        }
                    }
                    
                    yy--;
                }
                
                return target;
            }
            
            /// Mirrors this bitmap along Y axis as a new one.
            basic_Image MirrorY() const {
                basic_Image target(size, GetMode());
                int xx = size.Width- 1;
                for(int x=0; x<size.Width; x++) {
                    for(int y=0; y<size.Height; y++) {
                        for(unsigned c=0; c<cpp; c++) {
                            target(x, y, c) = operator()(xx, y, c);
                        }
                    }
                    
                    xx--;
                }
                
                return target;
            }

            /// Flips this bitmap along X axis as a new one.
            basic_Image FlipX() const {
                return MirrorY();
            }
            
            /// Flips this bitmap along Y axis as a new one.
            basic_Image FlipY() const {
                return MirrorX();
            }

            /// Imports a given bitmap file. BMP RLE compression and colorspaces are not supported.
            bool ImportBMP(const std::string &filename, bool dib = false) {
                std::ifstream file(filename, std::ios::binary);

                if(!file.is_open()) return false;

                return ImportBMP(file, dib);
            }

            /// Imports a given bitmap file. BMP RLE compression and colorspaces are not supported.
            bool ImportBMP(std::istream &file, bool dib = false) {
                using namespace IO;

                unsigned long off = 0;

                if(!dib) {
                    if(ReadString(file, 2) != "BM") return false;

                    IO::ReadUInt32(file);

                    ReadUInt16(file); //reserved 1
                    ReadUInt16(file); //reserved 2

                    off = ReadUInt32(file);
                }

                auto headersize = IO::ReadUInt32(file);

                int width = 0, height = 0;
                int bpp = 0;
                bool upsidedown = false;
                bool grayscalepalette = true;
                int colorsused = 0;
                bool alpha = false;
                int redshift, greenshift, blueshift, alphashift;
                float redmult = 1, greenmult = 1, bluemult = 1, alphamult = 1;
                uint32_t redmask = 0, greenmask = 0, bluemask = 0, alphamask = 0;

                std::vector<Graphics::RGBA> palette;

                auto shiftcalc=[](uint32_t v) {
                    int pos = 0;
                    while(v) {
                        if(v&1) return pos;
                        v=v>>1;
                        pos++;
                    }

                    return pos;
                };

                if(headersize == 12) { //2x bmp
                    width = ReadInt16(file);
                    height = ReadInt16(file);
                    ReadUInt16(file); //planes
                    bpp = ReadUInt16(file);
                }

                if(headersize >= 40) { //3x
                    width = ReadInt32(file);
                    height = ReadInt32(file);
                    ReadUInt16(file); //planes
                    bpp = ReadUInt16(file);

                    auto compress = ReadUInt32(file);

                    if(compress != 0 && compress != 3) return false;

                    bool bitcompress = compress != 0;

                    ReadUInt32(file); //size of bitmap

                    ReadInt32(file); //horz resolution
                    ReadInt32(file); //vert resolution

                    colorsused = ReadUInt32(file);
                    ReadUInt32(file); //colors important

                    if(bitcompress && headersize == 40) {
                        redmask = (uint32_t)ReadUInt32(file);
                        greenmask = (uint32_t)ReadUInt32(file);
                        bluemask = (uint32_t)ReadUInt32(file);
                    }
                    else if(bpp == 16) {
                        redmask   = 0x7c00;
                        greenmask = 0x03e0;
                        bluemask  = 0x001f;
                    }
                    else if(bpp == 32) {
                        redmask   = 0x00ff0000;
                        greenmask = 0x0000ff00;
                        bluemask  = 0x000000ff;
                    }
                }

                if(headersize >= 108) {
                    redmask = (uint32_t)ReadUInt32(file);
                    greenmask = (uint32_t)ReadUInt32(file);
                    bluemask = (uint32_t)ReadUInt32(file);
                    alphamask = (uint32_t)ReadUInt32(file);

                    file.seekg(13*4, std::ios::cur); //colorspace information
                }

                redshift = shiftcalc(redmask);
                greenshift = shiftcalc(greenmask);
                blueshift = shiftcalc(bluemask);
                alphashift = shiftcalc(alphamask);

                if(redmask) {
                    redmult = 255.f / (redmask>>redshift);
                }
                if(greenmask) {
                    greenmult = 255.f / (greenmask>>greenshift);
                }
                if(bluemask) {
                    bluemult = 255.f / (bluemask>>blueshift);
                }
                if(alphamask) {
                    alphamult = 255.f / (alphamask>>alphashift);
                }

                if(headersize > 108) {
                    file.seekg(headersize - 108, std::ios::cur);
                }

                if(height > 0)
                    upsidedown = true;
                else
                    height *= -1;

                if(bpp <= 8) { //paletted
                    if(colorsused == 0)
                        colorsused = 1 << bpp;

                    palette.reserve(colorsused);

                    for(int i=0; i<colorsused; i++) {
                        Byte r, g, b, a = 255;
                        
                        b = ReadUInt8(file);
                        g = ReadUInt8(file);
                        r = ReadUInt8(file);

                        if(r!=b || b!=g) grayscalepalette = false;

                        if(headersize > 12) {
                            a = ReadUInt8(file); //reserved
                        }

                        palette.emplace_back(r, g, b, a);
                    }

                    for(int i=0; i<colorsused; i++) {
                        if(palette[i].A) {
                            alpha = true;
                            break;
                        }
                    }
                }

                if(!dib)
                    file.seekg(off, std::ios::beg);

                if((bpp == 32 || bpp == 16) && alphamask != 0) {
                    Resize({width, height}, Graphics::ColorMode::RGBA);
                    alpha = true;
                }
                else if(bpp <= 8 && grayscalepalette) {
                    if(alpha)
                        Resize({width, height}, Graphics::ColorMode::Grayscale_Alpha);
                    else 
                        Resize({width, height}, Graphics::ColorMode::Grayscale);
                }
                else if(alpha) {
                    if(redmask == 0 && greenmask == 0 && bluemask == 0) {
                        Resize({width, height}, Graphics::ColorMode::Alpha);
                    }
                    else {
                        Resize({width, height}, Graphics::ColorMode::RGBA);
                    }
                }
                else {
                    Resize({width, height}, Graphics::ColorMode::RGB);
                }

                int ys, ye, yc;

                if(upsidedown) {
                    ys = height-1;
                    ye = -1;
                    yc = -1;
                }
                else {
                    ys = 0;
                    ye = height;
                    yc = 1;
                }

                if(bpp == 24) {
                    for(int y = ys; y!=ye; y += yc) {
                        int bytes = 0;
                        for(int x=0; x<width; x++) {
                            this->operator ()({x, y}, 0) = ReadUInt8(file);
                            this->operator ()({x, y}, 1) = ReadUInt8(file);
                            this->operator ()({x, y}, 2) = ReadUInt8(file);

                            bytes += 3;
                        }

                        if(bytes%4) {
                            file.seekg(4-bytes%4, std::ios::cur);
                        }
                    }
                }
                else if(bpp == 16 || bpp == 32) {
                    for(int y = ys; y!=ye; y += yc) {
                        int bytes = 0;
                        for(int x=0; x<width; x++) {
                            uint32_t pix;
                            
                            if(bpp == 16)
                                pix = ReadUInt16(file);
                            else
                                pix = ReadUInt32(file);

                            if(redmask != 0 || greenmask != 0 || bluemask != 0) {
                                this->operator ()({x, y}, 0) = (Byte)std::round(((pix&redmask)>>redshift) * redmult);
                                this->operator ()({x, y}, 1) = (Byte)std::round(((pix&greenmask)>>greenshift) * greenmult);
                                this->operator ()({x, y}, 2) = (Byte)std::round(((pix&bluemask)>>blueshift) * bluemult);
                            }

                            if(alpha) {
                                if(redmask != 0 || greenmask != 0 || bluemask != 0) {
                                    this->operator ()({x, y}, 3) = (Byte)std::round(((pix&alphamask)>>alphashift) * alphamult);
                                }
                                else {
                                    this->operator ()({x, y}, 0) = (Byte)std::round(((pix&alphamask)>>alphashift) * alphamult);
                                }
                            }
                            
                            bytes += bpp/8;
                        }

                        if(bytes%4) {
                            file.seekg(4-bytes%4, std::ios::cur);
                        }
                    }
                }
                else if(bpp < 8) {
                    Byte bitmask = (1 << bpp) - 1;
                    bitmask = bitmask << (8-bpp);
                    for(int y = ys; y!=ye; y += yc) {
                        int bytes = 0;
                        int bits  = 0;
                        Byte v = 0;
                        for(int x=0; x<width; x++) {
                            int ind;
                            if(bits == 0) {
                                v = ReadUInt8(file);
                                bits = 8;
                                bytes++;
                            }

                            ind = (v&bitmask)>>(8-bpp);
                            if(ind >= colorsused)
                                continue;

                            auto col = palette[ind];
                            
                            if(grayscalepalette) {
                                this->operator ()({x, y}, 0) = col.R;

                                if(alpha)
                                    this->operator ()({x, y}, 1) = col.A;
                            }
                            else {
                                this->operator ()({x, y}, 0) = col.R;
                                this->operator ()({x, y}, 1) = col.G;
                                this->operator ()({x, y}, 2) = col.B;
                                

                                if(alpha) {
                                    this->operator ()({x, y}, 3) = col.A;
                                }
                            }

                            v = v<<bpp;
                            bits -= bpp;
                        }

                        if(bytes%4) {
                            file.seekg(4-bytes%4, std::ios::cur);
                        }
                    }
                }

                return true;
            }

            /// Exports the image as a bitmap. RGB is exported as 24-bit, RGBA, BGR, BGRA is exported
            /// as 32-bit, Grayscale exported as 8-bit, Grayscale alpha, alpha only is exported as
            /// 16-bit
            bool ExportBMP(const std::string &filename, bool usev4 = false, bool dib = false) {
                std::ofstream file(filename, std::ios::binary);

                if(!file.is_open()) return false;

                return ExportBMP(file, usev4, dib);
            }

            /// Exports the image as a bitmap. RGB is exported as 24-bit, RGBA, BGR, BGRA is exported
            /// as 32-bit, Grayscale exported as 8-bit, Grayscale alpha, alpha only is exported as
            /// 16-bit
            bool ExportBMP(std::ostream &file, bool usev4 = false, bool dib = false) {
                using namespace IO;
                using Graphics::ColorMode;

                long datasize = 0;
                long headersize = 0;
                long extraspace = 0;
                int bpp = 0;
                int compression = 0;
                int stride = 0;

                switch(mode) {
                case ColorMode::RGB:
                case ColorMode::BGR:
                    stride = 3 * size.Width;
                    if(stride%4) stride += (4-(stride%4));

                    datasize = stride * size.Height;

                    headersize = 40;
                    bpp = 24;
                    break;

                case ColorMode::RGBA:
                case ColorMode::BGRA:
                case ColorMode::Grayscale_Alpha:
                    stride = 4 * size.Width;
                    if(stride%4) stride += (4-(stride%4));

                    datasize = stride * size.Height;

                    headersize = 108;
                    bpp = 32;
                    compression = 3;
                    break;

                case ColorMode::Grayscale:
                    stride = size.Width;
                    if(stride%4) stride += (4-(stride%4));

                    datasize = stride * size.Height;

                    headersize = 40;
                    extraspace = 256 * 4; //palette
                    bpp = 8;
                    break;

                case ColorMode::Alpha:
                    stride = 2 * size.Width;
                    if(stride%4) stride += (4-(stride%4));

                    datasize = stride * size.Height;

                    headersize = 108;

                    compression = 3;
                    bpp = 16;
                    break;

                default:
                    throw std::runtime_error("Unsupported color mode");
                }

                
                //header
                if(!dib) {
                    WriteString(file, "BM");
                    WriteUInt32(file, 14 + headersize + extraspace + datasize);
                    WriteUInt16(file, 0); //reserved 1
                    WriteUInt16(file, 0); //reserved 2
                    WriteUInt32(file, 14 + headersize + extraspace);
                }

                WriteUInt32(file, headersize);
                WriteInt32 (file, size.Width);
                WriteInt32 (file, size.Height);
                WriteUInt16(file, 1);
                WriteUInt16(file, bpp);
                WriteUInt32(file, compression);
                WriteUInt32(file, datasize);
                WriteInt32 (file, 2834);
                WriteInt32 (file, 2834);
                WriteInt32(file, 0); //colors used
                WriteInt32(file, 0); //colors important

                if(compression == 3) {
                    if(mode == ColorMode::Alpha) {
                        WriteUInt32(file, 0x00000001);
                        WriteUInt32(file, 0x00000002);
                        WriteUInt32(file, 0x00000004);
                    }
                    else if(mode == ColorMode::BGRA) {
                        WriteUInt32(file, 0x00ff0000);
                        WriteUInt32(file, 0x0000ff00);
                        WriteUInt32(file, 0x000000ff);
                    }
                    else {	
                        WriteUInt32(file, 0x000000ff);
                        WriteUInt32(file, 0x0000ff00);
                        WriteUInt32(file, 0x00ff0000);
                    }
                }

                if(headersize == 108 || usev4) {
                    if(compression != 3) {
                        WriteUInt32(file, 0x00000001);
                        WriteUInt32(file, 0x00000002);
                        WriteUInt32(file, 0x00000004);
                    }
                    if(mode == ColorMode::Alpha) {
                        WriteUInt32(file, 0x0000ff00);
                    }
                    else {
                        WriteUInt32(file, 0xff000000);
                    }
                    WriteUInt32(file, 1); //device dependent RGB
                    for(int i=0; i<12; i++)  //color profile settings, all 0
                        WriteUInt32(file, 0);
                }

                int ostride;
                switch(mode) {
                    case ColorMode::RGB:
                        ostride = size.Width*3;
                        for(int y=size.Height-1; y>=0; y--) {
                            WriteArray(file, data+y*ostride, ostride);

                            for(int j=0; j<stride-ostride; j++)
                                WriteUInt8(file, 0);
                        }
                        break;

                    case ColorMode::BGR:
                        ostride = size.Width*3;

                        for(int y=size.Height-1; y>=0; y--) {
                            WriteArray(file, data+y*ostride, ostride);

                            for(int j=0; j<stride-ostride; j++)
                                WriteUInt8(file, 0);
                        }
                        break;


                    case ColorMode::RGBA:
                    case ColorMode::BGRA:
                        ostride = size.Width*4;
                        for(int y=size.Height-1; y>=0; y--) {
                            WriteArray(file, data+y*ostride, ostride);
                        }
                        break;

                    case ColorMode::Grayscale:
                        //write palette
                        for(int i=0; i<256; i++) {
                            WriteUInt8(file, (Byte)i);
                            WriteUInt8(file, (Byte)i);
                            WriteUInt8(file, (Byte)i);
                            WriteUInt8(file, 0);
                        }

                        //write data
                        ostride = size.Width;
                        for(int y=size.Height-1; y>=0; y--) {
                            WriteArray(file, data+y*ostride, ostride);

                            for(int j=0; j<stride-ostride; j++)
                                WriteUInt8(file, 0);
                        }


                        break;
                        
                    
                    case ColorMode::Grayscale_Alpha:
                        ostride = size.Width*2;

                        for(int y=size.Height-1; y>=0; y--) {
                            for(int x=0; x<size.Width; x++) {
                                WriteUInt8(file, data[y*ostride+x*2]);
                                WriteUInt8(file, data[y*ostride+x*2]);
                                WriteUInt8(file, data[y*ostride+x*2]);
                                WriteUInt8(file, data[y*ostride+x*2+1]);
                            }
                        }
                        break;


                    case ColorMode::Alpha:
                        ostride = size.Width;

                        for(int y=size.Height-1; y>=0; y--) {
                            for(int x=0; x<size.Width; x++) {
                                WriteUInt8(file, 0xff);
                                WriteUInt8(file, data[y*ostride+x]);
                            }

                            for(int j=0; j<stride-ostride*2; j++)
                                WriteUInt8(file, 0);
                        }
                        break;
                    
                    default:
                        throw std::runtime_error("Invalid mode");
                }

                return true;
            }

            /// Provides access to the given component in x and y coordinates. This
            /// function performs bounds checking only on debug mode.
            Byte &operator()(const Geometry::Point &p, unsigned component=0) {
#ifndef NDEBUG
                if(p.X<0 || p.Y<0 || p.X>=size.Width || p.Y>=size.Height || component>=cpp) {
                    throw std::runtime_error("Index out of bounds");
                }
#endif
                return data[cpp*(size.Width*p.Y+p.X)+component];
            }

            /// Provides access to the given component in x and y coordinates. This
            /// function performs bounds checking only on debug mode.
            Byte operator()(const Geometry::Point &p, unsigned component=0) const {
#ifndef NDEBUG
                if(p.X<0 || p.Y<0 || p.X>=size.Width || p.Y>=size.Height || component>=cpp) {
                    throw std::runtime_error("Index out of bounds");
                }
#endif
                return data[cpp*(size.Width*p.Y+p.X)+component];
            }

            /// Provides access to the given component in x and y coordinates. This
            /// function returns 0 if the given coordinates are out of bounds. This
            /// function works slower than the () operator.
            Byte Get(const Geometry::Point &p, unsigned component = 0) const {
                if (p.X < 0 || p.Y < 0 || p.X >= size.Width || p.Y >= size.Height || component >= cpp) {
                    return 0;
                }

                return data[cpp*(size.Width*p.Y + p.X) + component];
            }

            /// Provides access to the given component in x and y coordinates. This
            /// function returns 0 if the given coordinates are out of bounds. This
            /// function works slower than the () operator.
            Byte Get(const Geometry::Point &p, Byte def, unsigned component = 0) const {
                if (p.X < 0 || p.Y < 0 || p.X >= size.Width || p.Y >= size.Height || component >= cpp) {
                    return def;
                }

                return data[cpp*(size.Width*p.Y + p.X) + component];
            }

            /// Provides access to the given component in x and y coordinates. This
            /// function performs bounds checking only on debug mode.
            Byte &operator()(int x, int y, unsigned component=0) {
#ifndef NDEBUG
                if(x<0 || y<0 || x>=size.Width || y>=size.Height || component>=cpp) {
                    throw std::runtime_error("Index out of bounds");
                }
#endif
                return data[cpp*(size.Width*y+x)+component];
            }

            /// Provides access to the given component in x and y coordinates. This
            /// function performs bounds checking only on debug mode.
            Byte operator()(int x, int y, unsigned component=0) const {
#ifndef NDEBUG
                if(x<0 || y<0 || x>=size.Width || y>=size.Height || component>=cpp) {
                    throw std::runtime_error("Index out of bounds");
                }
#endif
                return data[cpp*(size.Width*y+x)+component];
            }

            /// Provides access to the given component in x and y coordinates. This
            /// function returns 0 if the given coordinates are out of bounds. This
            /// function works slower than the () operator.
            Byte Get(int x, int y, unsigned component=0) const {
                if(x<0 || y<0 || x>=size.Width || y>=size.Height || component>=cpp) {
                    return 0;
                }

                return data[cpp*(size.Width*y+x)+component];
            }

            /// Returns the alpha at the given location. If the given location does not exits
            /// this function will return 0. If there is no alpha channel, image is assumed
            /// to be opaque.
            Byte GetAlphaAt(int x, int y) const {
                if(x<0 || y<0 || x>=size.Width || y>=size.Height) {
                    return 0;
                }

                if(alphaloc == -1)
                    return 255;

                return data[cpp*(size.Width*y+x)+alphaloc];
            }

            /// Returns the alpha at the given location. If the given location does not exits
            /// this function will return 0. If there is no alpha channel, image is assumed
            /// to be opaque.
            Graphics::RGBA GetRGBAAt(int x, int y) const {
                if(x<0 || y<0 || x>=size.Width || y>=size.Height) {
                    return 0;
                }
                
                switch(mode) {
                    case Graphics::ColorMode::Alpha:
                        return {255, 255, 255, Byte((*this)(x, y, 0))};
                    case Graphics::ColorMode::Grayscale_Alpha:
                        return {Byte((*this)(x, y, 0)), Byte((*this)(x, y, 0)), Byte((*this)(x, y, 0)), Byte((*this)(x, y, 1))};
                    case Graphics::ColorMode::Grayscale:
                        return {Byte((*this)(x, y, 0)), Byte((*this)(x, y, 0)), Byte((*this)(x, y, 0)), 255};
                    case Graphics::ColorMode::RGB:
                        return {Byte((*this)(x, y, 0)), Byte((*this)(x, y, 1)), Byte((*this)(x, y, 2)), 255};
                    case Graphics::ColorMode::BGR:
                        return {Byte((*this)(x, y, 2)), Byte((*this)(x, y, 1)), Byte((*this)(x, y, 0)), 255};
                    case Graphics::ColorMode::RGBA:
                        return {Byte((*this)(x, y, 0)), Byte((*this)(x, y, 1)), Byte((*this)(x, y, 2)), Byte((*this)(x, y, 3))};
                    case Graphics::ColorMode::BGRA:
                        return {Byte((*this)(x, y, 2)), Byte((*this)(x, y, 1)), Byte((*this)(x, y, 0)), Byte((*this)(x, y, 3))};
                    default:
                        return 0;
                }
            }

            /// Returns the alpha at the given location. If the given location does not exits
            /// this function will return 0. If there is no alpha channel, image is assumed
            /// to be opaque.
            Graphics::RGBA GetRGBAAt(Geometry::Point p) const {
                return GetRGBAAt(p.X, p.Y);
            }
            
            ///Sets the color at the given location to the specified RGBA value. If pixel does not
            ///exists, the call will be ignored.
            void SetRGBAAt(int x, int y, Graphics::RGBA color) {
                if(x<0 || y<0 || x>=size.Width || y>=size.Height) {
                    return;
                }
                
                switch(mode) {
                    case Graphics::ColorMode::Alpha:
                        (*this)(x, y, 0) = color.A;
                        break;
                    case Graphics::ColorMode::Grayscale_Alpha:
                        (*this)(x, y, 0) = color.Luminance();
                        (*this)(x, y, 1) = color.A;
                        break;
                    case Graphics::ColorMode::Grayscale:
                        (*this)(x, y, 0) = color.Luminance();
                        break;
                    case Graphics::ColorMode::RGB:
                        (*this)(x, y, 0) = color.R;
                        (*this)(x, y, 1) = color.G;
                        (*this)(x, y, 2) = color.B;
                        break;
                    case Graphics::ColorMode::BGR:
                        (*this)(x, y, 2) = color.R;
                        (*this)(x, y, 1) = color.G;
                        (*this)(x, y, 0) = color.B;
                        break;
                    case Graphics::ColorMode::RGBA:
                        (*this)(x, y, 0) = color.R;
                        (*this)(x, y, 1) = color.G;
                        (*this)(x, y, 2) = color.B;
                        (*this)(x, y, 3) = color.A;
                        break;
                    case Graphics::ColorMode::BGRA:
                        (*this)(x, y, 2) = color.R;
                        (*this)(x, y, 1) = color.G;
                        (*this)(x, y, 0) = color.B;
                        (*this)(x, y, 3) = color.A;
                        break;
                    default:
                        ;
                }
            }
            
            ///Sets the color at the given location to the specified RGBA value. If pixel does not
            ///exists, the call will be ignored.
            void SetRGBAAt(Geometry::Point p, Graphics::RGBA color) {
                SetRGBAAt(p.X, p.Y, color);
            }
        
            /// Returns the size of the image
            Geometry::Size GetSize() const {
                return size;
            }

            /// Returns the width of the image
            int GetWidth() const {
                return size.Width;
            }

            /// Returns the height of the image
            int GetHeight() const {
                return size.Height;
            }

            /// Total size of this image in number units
            unsigned long GetTotalSize() const {
                return size.Area()*cpp;
            }

            /// Returns the color mode of the image
            Graphics::ColorMode GetMode() const {
                return mode;
            }
            
            /// Changes the color mode of the image. Only works if the bits/pixel 
            /// of the target mode is the same as the original
            void ChangeMode(Graphics::ColorMode value) {
                if(Graphics::GetChannelsPerPixel(mode) != Graphics::GetChannelsPerPixel(value))
                    throw std::runtime_error("Modes differ in number of bits/pixel");
                
                mode = value;
                this->alphaloc = Graphics::HasAlpha(mode) ? Graphics::GetAlphaIndex(mode) : -1;
            }

            /// Returns the number units occupied by a single pixel of this image
            unsigned GetChannelsPerPixel() const {
                return cpp;
            }

            /// Returns if this image has alpha channel
            bool HasAlpha() const {
                return alphaloc != -1;
            }

            /// Returns the index of alpha channel. Value of -1 denotes no alpha channel
            int GetAlphaIndex() const {
                return alphaloc;
            }

        protected:
            /// Data that stores pixels of the image
            T_ *data = nullptr;

            /// Width of the image
            Geometry::Size size = {0, 0};

            /// Color mode of the image
            Graphics::ColorMode mode = Graphics::ColorMode::Invalid;

            /// Channels per pixel information
            unsigned cpp = 0;

            /// Location of the alpha channel, -1 means it does not exits
            int alphaloc = -1;
        };
        
        
        /// Compares two images. Images are equal if their size, mode and 
        /// contents are the same
        template <class T_>
        bool operator ==(const basic_Image<T_> &l, const basic_Image<T_> &r) {
            if(l.GetSize() != r.GetSize()) return false;
            if(l.GetMode() != r.GetMode()) return false;
            
            const auto S = l.GetSize();
            const auto C = l.GetChannelsPerPixel();
            for(int y=0; y<S.Height; y++) for(int x=0; x<S.Width; x++) for(int c=0; c<C; c++)
                if(l(x, y, c) != r(x, y, c)) return false;
            
            return true;
        }
        
        /// Compares two images. Images are equal if their size, mode and 
        /// contents are not the same
        template <class T_>
        bool operator !=(const basic_Image<T_> &l, const basic_Image<T_> &r) {
            return !(l == r);
        }
        
        /// Swaps two images. Should be used unqualified for ADL.
        template <class T_>
        inline void swap(basic_Image<T_> &l, basic_Image<T_> &r) {
            l.Swap(r);
        }
        

        using Image = basic_Image<Byte>;

    }
