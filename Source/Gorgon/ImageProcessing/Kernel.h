#pragma once

#include "../Types.h"
#include "../Geometry/Size.h"

#include <iosfwd>
#include <vector>
#include <initializer_list>


namespace Gorgon :: ImageProcessing {
    
    /**
     * This class is used in conjunction with Convolution function to apply convolution filter to
     * an image.
     */
    class Kernel {
    public:
        
        /// Default constructor creates an empty kernel
        Kernel() {}
        
        /// Creates a new kernel using the supplied values
        Kernel(const std::initializer_list<std::initializer_list<Float>> &values);
        
        /// Move constructor is supported
        Kernel(Kernel &&) = default;

        /// Copy constructor is supported
        Kernel(Kernel &) = default;
        
        /// Assigns new values
        Kernel &operator =(const std::initializer_list<std::initializer_list<Float>> &values);
        
        /// Move assignment is supported
        Kernel &operator =(Kernel &&) = default;
        
        /// Copy assignment is supported
        Kernel &operator =(Kernel &) = default;
        
        /// Changes the size of the kernel. Values will not be preserved in a meaningful way.
        void Resize(const Geometry::Size &size);
        
        /// Changes the size of the kernel. Values will not be preserved in a meaningful way.
        void Resize(int width, int height) {
            Resize({width, height});
        }
        
        /// Changes the size of the kernel. Values will not be preserved in a meaningful way. This
        /// variant sets both dimensions to the value value.
        void Resize(int size) {
            Resize({size, size});
        }
        
        /// Returns the width of the filter
        int GetWidth() const {
            return size.Width;
        }
        
        /// Returns the height of the filter
        int GetHeight() const {
            return size.Height;
        }

        /// Returns the size of the filter
        Geometry::Size GetSize() const {
            return size;
        }
        
        /// Returns sum of all elements in the filter
        Float GetTotal() const;
        
        /// Returns a specific element. This value can be changed
        Float &Get(int x, int y) {
            return kernel[x + y * size.Width];
        }
        
        /// Returns a specific element.
        Float Get(int x, int y) const {
            return kernel[x + y * size.Width];
        }
        
        /// Returns a specific element.
        Float Get(const Geometry::Point &index) const {
            return kernel[index.X + index.Y * size.Width];
        }
        
        /// Returns a specific element. This value can be changed
        Float &Get(const Geometry::Point &index) {
            return kernel[index.X + index.Y * size.Width];
        }
        
        /// Returns a specific element. This value can be changed
        Float &operator ()(int x, int y) {
            return kernel[x + y * size.Width];
        }
        
        Float operator ()(int x, int y) const {
            return kernel[x + y * size.Width];
        }
        
        /// Returns a specific element. 
        Float operator ()(const Geometry::Point &index) const {
            return kernel[index.X + index.Y * size.Width];
        }
        
        /// Returns a specific element. This value can be changed
        Float &operator ()(const Geometry::Point &index) {
            return kernel[index.X + index.Y * size.Width];
        }
        
        /// Returns the internal data that is stored
        std::vector<Float> &GetData() {
            return kernel;
        }
        
        /// Normalizes the kernel so the sum becomes 1
        void Normalize();
        
        /// Create a sobel filter for gradient calculation
        static Kernel SobelFilter(Axis axis);
        
        /// Creates a simple sharpen filter.
        static Kernel Sharpen();
        
        /// Creates a box kernel for blur filter. It is better to use gaussian filter for this
        /// purpose.
        static Kernel BoxFilter(int kernelsize);
        
        ///Creates a Circular filter, it can be used by the openning/closing
        static Kernel CircularFilter(float kernelsize);
        
        /// A kernel that can be used for edge detection.
        static Kernel EdgeDetection(int kernelsize);
        
        /// Creates kernel for Gaussian blur filter. This function creates filter for only one
        /// axis. Gaussian blur can be achived by applying two separate filters for axis instead of 
        /// single two dimensional filter. By splitting the kernel filtering works at least sigma
        /// times faster. Larger quality values improve the result while reducing the processing
        /// speed. Qualities above 3 will not yield an improvement for 8-bit images.
        static Kernel GaussianFilter(float sigma, Axis axis, float quality = 2.0f);
        
    private :
        void createboxfilter(float centervalue, float others);
        void createcircularfilter(float centervlaue, float others);
        
        
        Geometry::Size size = {0, 0};
        
        std::vector<Float> kernel;
    };

    std::ostream &operator <<(std::ostream &out, const Kernel &kernel);
    
}
