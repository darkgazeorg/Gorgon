#pragma once

#include "Kernel.h"

#include "../Containers/Image.h"


namespace Gorgon :: ImageProcessing {
    
    /// These are the policies that governs values outside the boundaries of the image. Not all
    /// values are available in all functions
    enum class OutOfBoundsPolicy {
        NearestNeighbor,
        FixedColor,
        Cyclic,
        Mirror,
        PartialSum,
    };

    /// These are the policies that governs values that falls outside the allowed range of values.
    /// For performance reasons, these policies will not be applied to float or double images. If 
    /// necessary, you may use Clamp, Abs and Scale functions separately.
    enum class OutOfRangePolicy {
        NoChange,
        RoundAndClamp,
        Clamp,
        Abs,
        RoundAndAbs,
        Scale,
        ScaleAndRound,
        ScaleAndClamp,
        ScaleRoundAndClamp
    };

    /**
     * Performs convolution operation to the given image with the supplied kernel, there are 
     * warnings that apply to the use of parameters, read the whole documentation. For bitmaps
     * you may use Convolution member function. It will call this function with the contained image.
     * outsidecolor could be converted from RGBA if an 8-bit image is used. outofrange is only used
     * when this function is applied to integral typed images including Image class. scale is used
     * only if outofrange is set to Scale or ScaleThenClamp. If noalpha is set, the operation will
     * not be performed on alpha channel and the original alpha channel is copied to the output.
     */
    template <class CT_ = Byte>
    Containers::basic_Image<CT_> Convolution(
        const Containers::basic_Image<CT_> &input,
        Kernel kernel, 
        OutOfBoundsPolicy outofbounds = OutOfBoundsPolicy::NearestNeighbor,
        OutOfRangePolicy outofrange = OutOfRangePolicy::Clamp,
        std::array<CT_, 4> outsidecolor = {}, 
        bool noalpha = false, bool normalize = false,
        std::array<Float, 4> scale = {1.f, 1.f, 1.f, 1.f}
    ) {
        if(input.GetSize().Area() == 0)
            return {};
        
        if(normalize)
            kernel.Normalize();
        
        //collect info
        auto mode = input.GetMode();
        
        int C = GetChannelsPerPixel(mode);
        int A = GetAlphaIndex(mode);
        
        int W = input.GetWidth();
        int H = input.GetHeight();
        
        int w = kernel.GetWidth();
        int h = kernel.GetHeight();
        
        Containers::basic_Image<CT_> output({W, H}, mode);
        
        auto forpixelsmultiplier = [&]() {
            for(int y=0; y<H; y++) {
                for(int x=0; x<W; x++) {
                    std::array<Float, 4> values = {};
                    
                    Float weightsum = 0;
                    
                    for(int i=0; i<w; i++) {
                        for(int j=0; j<h; j++) {
                            auto cx = x - i + w/2;
                            auto cy = y - j + h/2;
                            
                            if(cx >= 0 && cx < W && cy >= 0 && cy < H) {
                                for(int c=0; c<C; c++) {
                                    values[c] += input(cx, cy, c) * kernel(i, j);
                                }
                                
                                weightsum += kernel(i, j);
                            }
                        }
                    }
                    
                    if(std::is_integral<CT_>::value) {
                        switch(outofrange) {
                        case OutOfRangePolicy::Clamp:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)Clamp<Float>(
                                        values[c] / weightsum, 
                                        std::numeric_limits<CT_>::min(), std::numeric_limits<CT_>::max()
                                    );
                            }
                            break;
                            
                        case OutOfRangePolicy::RoundAndClamp:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)Clamp<Float>(
                                        std::round(values[c] / weightsum),
                                        std::numeric_limits<CT_>::min(), std::numeric_limits<CT_>::max()
                                    );
                            }
                            break;
                            
                        case OutOfRangePolicy::Abs:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)std::fabs(values[c] / weightsum);
                            }
                            break;
                            
                        case OutOfRangePolicy::RoundAndAbs:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)std::fabs(std::round(values[c] / weightsum));
                            }
                            break;
                        
                        case OutOfRangePolicy::Scale:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)(values[c] / weightsum * scale[c]);
                            }
                            break;
                        
                        case OutOfRangePolicy::ScaleAndClamp:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)Clamp<Float>(
                                        values[c] / weightsum * scale[c],
                                        std::numeric_limits<CT_>::min(), std::numeric_limits<CT_>::max()
                                    );
                            }
                            break;
                        
                        case OutOfRangePolicy::ScaleAndRound:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)std::round(values[c] / weightsum * scale[c]);
                            }
                            break;
                        
                        case OutOfRangePolicy::ScaleRoundAndClamp:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)Clamp<Float>(
                                        std::round(values[c] / weightsum * scale[c]),
                                        std::numeric_limits<CT_>::min(), std::numeric_limits<CT_>::max()
                                    );
                            }
                            break;
                        
                        default:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = Byte(values[c] / weightsum);
                            }
                            break;
                        }
                    }
                    else {
                        for(int c=0; c<C; c++) {
                            if(!noalpha || c != A)
                                output(x, y, c) = Byte(values[c] / weightsum);
                        }
                    }
                }
            }
        };
        
        auto forpixels = [&](auto oob) {
            for(int y=0; y<H; y++) {
                for(int x=0; x<W; x++) {
                    std::array<Float, 4> values = {};
                    
                    for(int i=0; i<w; i++) {
                        for(int j=0; j<h; j++) {
                            auto cx = x - i + w/2;
                            auto cy = y - j + h/2;
                            
                            if(cx < 0 || cx >= W || cy < 0 || cy >= H) {
                                oob(cx, cy, values, kernel(i, j));
                            }
                            else {
                                for(int c=0; c<C; c++) {
                                    values[c] += input(cx, cy, c) * kernel(i, j);
                                }
                            }
                        }
                    }
                    
                    
                    if(std::is_integral<CT_>::value) {
                        switch(outofrange) {
                        case OutOfRangePolicy::Clamp:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)Clamp<Float>(
                                        values[c], 
                                        std::numeric_limits<CT_>::min(), std::numeric_limits<CT_>::max()
                                    );
                            }
                            break;
                            
                        case OutOfRangePolicy::RoundAndClamp:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)Clamp<Float>(
                                        std::round(values[c]),
                                        std::numeric_limits<CT_>::min(), std::numeric_limits<CT_>::max()
                                    );
                            }
                            break;
                        case OutOfRangePolicy::Abs:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)std::fabs(values[c]);
                            }
                            break;
                            
                        case OutOfRangePolicy::RoundAndAbs:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)std::fabs(std::round(values[c]));
                            }
                            break;
                        
                        case OutOfRangePolicy::Scale:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)(values[c] * scale[c]);
                            }
                            break;
                        
                        case OutOfRangePolicy::ScaleAndClamp:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)Clamp<Float>(
                                        values[c] * scale[c],
                                        std::numeric_limits<CT_>::min(), std::numeric_limits<CT_>::max()
                                    );
                            }
                            break;
                        
                        case OutOfRangePolicy::ScaleAndRound:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)std::round(values[c] * scale[c]);
                            }
                            break;
                        
                        case OutOfRangePolicy::ScaleRoundAndClamp:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = (CT_)Clamp<Float>(
                                        std::round(values[c] * scale[c]),
                                        std::numeric_limits<CT_>::min(), std::numeric_limits<CT_>::max()
                                    );
                            }
                            break;
                        
                        default:
                            for(int c=0; c<C; c++) {
                                if(!noalpha || c != A)
                                    output(x, y, c) = Byte(values[c]);
                            }
                            break;
                        }
                    }
                    else {
                        for(int c=0; c<C; c++) {
                            if(!noalpha || c != A)
                                output(x, y, c) = Byte(values[c]);
                        }
                    }
                }
            }
        };
        
        switch(outofbounds) {
        default:
        case OutOfBoundsPolicy::FixedColor:
            forpixels([&](auto , auto , auto &values, float kernel) {
                for(int c=0; c<C; c++) {
                    values[c] += outsidecolor[c] * kernel;
                }
            });
            break;
            
        case OutOfBoundsPolicy::PartialSum:
            forpixelsmultiplier();
            break;
            
        case OutOfBoundsPolicy::NearestNeighbor:
            forpixels([&](auto x, auto y, auto &values, float kernel) {
                auto cx = Clamp(x, 0, W - 1);
                auto cy = Clamp(y, 0, H - 1);
                
                for(int c=0; c<C; c++) {
                    values[c] += input(cx, cy, c) * kernel;
                }
            });
            break;
            
        case OutOfBoundsPolicy::Cyclic:
            forpixels([&](auto x, auto y, auto &values, float kernel) {
                auto cx = PositiveMod(x, W - 1);
                auto cy = PositiveMod(y, H - 1);
                
                for(int c=0; c<C; c++) {
                    values[c] += input(cx, cy, c) * kernel;
                }
            });
            break;
            
        case OutOfBoundsPolicy::Mirror:
            forpixels([&](auto x, auto y, auto &values, float kernel) {
                auto cx = Mirror(x, W - 1);
                auto cy = Mirror(y, H - 1);
                
                for(int c=0; c<C; c++) {
                    values[c] += input(cx, cy, c) * kernel;
                }
            });
            break;
        }

        if(noalpha && A != -1) {
            for(int y=0; y<H; y++) {
                for(int x=0; x<W; x++) {
                    output(x, y, A) = input(x, y, A);
                }
            }
        }

        return output;
    }

}
