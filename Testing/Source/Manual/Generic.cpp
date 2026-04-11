#include "Gorgon/Encoding/LZMA.h"
#include "Gorgon/Graphics/Color.h"
#include "Gorgon/Types.h"
#include "GraphicsHelper.h"
#include <Gorgon/Graphics/Bitmap.h>

#include <Gorgon/ImageProcessing/Kernel.h>
#include <Gorgon/ImageProcessing/Filters.h>
#include <fstream>

using namespace Gorgon::ImageProcessing;
std::string helptext = 
    "Key list:\n"
    "esc\tClose\n"
;

using namespace Gorgon::Graphics;


int main() {
    Application app("generictest", "Test", helptext, 10);
    
    Graphics::Layer layer;
    app.wind.Add(layer);
    
    Bitmap bmp({7,7}, Gorgon::Graphics::ColorMode::Grayscale);
    bmp.Clear();
    bmp(3, 3, 0) = 255;

    auto k = Kernel::GaussianFilter(1, Gorgon::Axis::X, 2);
    std::cout << k;
    k.Normalize();

    bmp.Assume(
        Convolution(bmp.GetData(), k)
    );
    bmp = bmp.ZoomMultiple(8);
    bmp.Prepare();

    bmp.Draw(layer, 0,0);
    bmp.Export("test.png");


    while(true) {
        Gorgon::NextFrame();
    }

    return 0;
}
