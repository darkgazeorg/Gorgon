#include "Kernel.h"

#include "../Utils/Assert.h"

#include <iomanip>
#include <numeric>

namespace Gorgon :: ImageProcessing {
    
    Kernel::Kernel(const std::initializer_list<std::initializer_list<Float>> &values) {
        *this = values;
    }

    Kernel &Kernel::operator = (const std::initializer_list<std::initializer_list<Float>> &values) {
        auto maxlistsize = values.begin()->size();
        
        for(auto &list : values) {            
            if(list.size()  > maxlistsize)
                maxlistsize = (int)list.size();
        }
        
        for(auto &list : values) {
            ASSERT(list.size() == maxlistsize, "Kernel size is not consistent");
            
            kernel.insert(kernel.end(), list.begin(), list.end());
            
            if(list.size() < maxlistsize)
                kernel.insert(kernel.end(), maxlistsize - list.size(), 0);
        }        
            
        size.Height = int(values.size());
        size.Width = int(maxlistsize);
        
        return *this;
    }

    void Kernel::Resize(const Geometry::Size &value) {
        size = value;
        
        kernel.resize(size.Area());
    }

    Float Kernel::GetTotal() const {
        return std::accumulate(kernel.begin(), kernel.end(), 0.0f);
    }
    
    void Kernel::Normalize() {
        auto f = 1.0f / GetTotal();
        
        for(auto &val : kernel) {
            val *= f;
        }
    }

    Kernel Kernel::SobelFilter(Axis axis) {
        Kernel nkernel;
        
        switch (axis){
            case Axis::X:
                nkernel = {{1.f, 0.f, -1.f}, {2.f, 0.f, -2.f}, {1.f, 0.f, -1.f}};
                break;
            case Axis::Y:
                nkernel = {{1.f, 2.f, 1.f}, {0.f, 0.f, 0.f}, {-1.f, -2.f, -1.f}};
                break;
        }
        
        return nkernel;
    }


    Kernel Kernel::Sharpen() {
        return {{-0.025f, -0.1f, -0.025f}, {-0.1f, 1.5f, -0.1f}, {-0.025f, -0.1f, -0.025f}};
    }  

    Kernel Kernel::BoxFilter(int kernelsize) {
        Kernel nkernel;
        
        nkernel.Resize({kernelsize, kernelsize});
        nkernel.createboxfilter(1.0f /(kernelsize * kernelsize), 1.0f / (kernelsize * kernelsize));
        
        return nkernel;
    }
    
    Kernel Kernel::CircularFilter(float kernelsize) {
        Kernel nkernel;
        int size = (int)std::ceil(kernelsize);
        nkernel.Resize({size, size});
        nkernel.createcircularfilter(0, 255);
        return nkernel;
    }

    Kernel Kernel::EdgeDetection(int kernelsize) {
        Kernel nkernel;
        
        nkernel.Resize({kernelsize, kernelsize});
        nkernel.createboxfilter(float(kernelsize * kernelsize - 1), -1.f);
        
        return nkernel;
    }

    Kernel Kernel::GaussianFilter(float sigma, Axis axis, float quality) {
        Kernel nkernel;
        int center = (int)std::ceil(sigma * quality);
        int size = center * 2 + 1;
        
        if(axis == Axis::Y)
            nkernel.size = {1, size};
        else
            nkernel.size = {size, 1};

        float base1 = -1.f / (2 * sigma * sigma);

        for(int x = 0; x < size; x++) {
            auto value =  std::exp((x-center) * (x-center) * base1);
            
            nkernel.kernel.push_back(value);
        }
        
        nkernel.Normalize();
        
        return nkernel;
    }

    void Kernel::createboxfilter(float centervalue, float others) {
        int centerindex = size.Height / 2;
        
        for(int y = 0; y < size.Height; y++) {
            for(int x= 0; x < size.Width; x++) {
                if(x == centerindex && y == centerindex)
                    Get(x, y) = centervalue;
                else
                    Get(x, y) = others;
            } 
        }
    }
    
    void Kernel::createcircularfilter(float centervalue, float others) {
        auto rs = size.Height * size.Width ;

        for(int y = -size.Height/2; y < size.Height/2; y++) {
            for(int x= -size.Width/2; x < size.Width/2; x++) {
                Gorgon::Geometry::Point current = {x,y};
                auto distance = current.EuclideanSquare();
                if(distance <= rs) 
                        Get(x, y) = centervalue;
                else
                    Get(x, y) = others;
            }
        }
    }

    std::ostream &operator <<(std::ostream &out, const Kernel &kernel) {
        std::ios oldState(nullptr);
        oldState.copyfmt(std::cout);

        for(int y=0; y<kernel.GetHeight(); y++) {
            for(int x=0; x<kernel.GetWidth(); x++) {
                std::cout << std::setw(6) << kernel(x, y) << " ";
            }
            std::cout << '\n';
        }

        std::cout.copyfmt(oldState);

        return out;
    }

}
