#define CATCH_CONFIG_MAIN


#include <catch2/catch_test_macros.hpp>

#include <Gorgon/Containers/Image.h>
#include <Gorgon/Encoding/PNG.h>
#include "Gorgon/Filesystem.h"
#include "Gorgon/Encoding/JPEG.h"

using namespace Gorgon;

bool operator ==(const Containers::Image &im1, const Containers::Image &im2) {
	if(im2.GetSize() != im1.GetSize() || im2.GetMode() != im1.GetMode())
		return false;

	for(int y=0; y<im1.GetSize().Height; y++) {
		for(int x=0; x<im1.GetSize().Width; x++) {
			for(unsigned c=0; c<im1.GetChannelsPerPixel(); c++)
				if(im1({x,y}, c) != im2({x,y}, c))
					return false;
		}
	}

	return true;
}

bool similar(const Containers::Image &im1, const Containers::Image &im2, int maxdif=20) {
	if(im2.GetSize() != im1.GetSize() || im2.GetMode() != im1.GetMode())
		return false;

	for(int y=0; y<im1.GetSize().Height; y++) {
		for(int x=0; x<im1.GetSize().Width; x++) {
			for(unsigned c=0; c<im1.GetChannelsPerPixel(); c++) {
				auto v1 = im1({x,y}, c);
				auto v2 = im2({x,y}, c);
				if(abs(v1-v2)>maxdif)
					return false;
			}
		}
	}

	return true;
}

TEST_CASE("PNG compression/decompression") {
	Containers::Image im1({36, 36}, Graphics::ColorMode::RGBA);

	int diff = 51;

	for(int r=0; r<6; r++) {
		for(int g=0; g<6; g++) {
			for(int b=0; b<6; b++) {
				for(int a=0; a<6; a++) {
					im1({r + g*6, b + a*6}, 0) = r*diff;
					im1({r + g*6, b + a*6}, 1) = g*diff;
					im1({r + g*6, b + a*6}, 2) = b*diff;
					im1({r + g*6, b + a*6}, 3) = a*diff;
				}
			}
		}
	}

	Encoding::Png.Encode(im1, "out.png");

	Containers::Image im2;

	REQUIRE(Filesystem::IsFile("out.png"));
	REQUIRE(Filesystem::Size("out.png") > 8);

	Encoding::Png.Decode("out.png", im2);

	REQUIRE(im2.GetSize() == im1.GetSize());
	REQUIRE(im2.GetMode() == im1.GetMode());

	REQUIRE(im1 == im2);

	im1.Resize({36, 6}, Graphics::ColorMode::RGB);

	for(int r=0; r<6; r++) {
		for(int g=0; g<6; g++) {
			for(int b=0; b<6; b++) {
				im1({r + g*6, b}, 0) = r*diff;
				im1({r + g*6, b}, 1) = g*diff;
				im1({r + g*6, b}, 2) = b*diff;
			}
		}
	}

	std::ofstream outfile("out.png", std::ios::binary);
	Encoding::Png.Encode(im1, outfile);
	outfile.close();

	std::ifstream infile("out.png", std::ios::binary);
	Encoding::Png.Decode(infile, im2);

	REQUIRE(im2.GetSize() == im1.GetSize());
	REQUIRE(im2.GetMode() == im1.GetMode());

	REQUIRE(im1 == im2);


	im1.Resize({6, 6}, Graphics::ColorMode::Grayscale_Alpha);

	for(int g=0; g<6; g++) {
		for(int a=0; a<6; a++) {
			im1({g, a}, 0) = g*diff;
			im1({g, a}, 1) = a*diff;
		}
	}

	Encoding::Png.Encode(im1, "outfile.png");

	Encoding::Png.Decode("outfile.png", im2);

	REQUIRE(im2.GetSize() == im1.GetSize());
	REQUIRE(im2.GetMode() == im1.GetMode());

	REQUIRE(im1 == im2);


	im1.Resize({6, 1}, Graphics::ColorMode::Grayscale);

	for(int g=0; g<6; g++) {
		im1({g, 0}, 0) = g*diff;
	}

	std::vector<Byte> vec;
	Encoding::Png.Encode(im1, vec);

	Encoding::Png.Decode(vec, im2);

	REQUIRE(im2.GetSize() == im1.GetSize());
	REQUIRE(im2.GetMode() == im1.GetMode());

	REQUIRE(im1 == im2);
}

TEST_CASE("JPEG compression/decompression") {
	int diff = 5;
	int mw   = 16;

	Containers::Image im1({mw*mw, mw}, Graphics::ColorMode::RGB);

	for(int r=0; r<mw; r++) {
		for(int g=0; g<mw; g++) {
			for(int b=0; b<mw; b++) {
				im1({r + g*mw, b}, 0) = r*diff;
				im1({r + g*mw, b}, 1) = g*diff;
				im1({r + g*mw, b}, 2) = b*diff;
			}
		}
	}

	Encoding::Jpg.Encode(im1, "out.jpg");

	Containers::Image im2;

	REQUIRE(Filesystem::IsFile("out.jpg"));
	REQUIRE(Filesystem::Size("out.jpg") > 8);

	auto fsize = Filesystem::Size("out.jpg");

	Encoding::Jpg.Decode("out.jpg", im2);

	REQUIRE(im2.GetSize() == im1.GetSize());
	REQUIRE(im2.GetMode() == im1.GetMode());

	REQUIRE(similar(im1, im2));

	im1.Resize({mw*mw, mw}, Graphics::ColorMode::RGB);

	for(int r=0; r<mw; r++) {
		for(int g=0; g<mw; g++) {
			for(int b=0; b<mw; b++) {
				im1({r + g*mw, b}, 0) = r*diff;
				im1({r + g*mw, b}, 1) = g*diff;
				im1({r + g*mw, b}, 2) = b*diff;
			}
		}
	}

	std::ofstream outfile("out.jpg", std::ios::binary);
	Encoding::Jpg.Encode(im1, outfile, 40);
	outfile.close();

	REQUIRE(Filesystem::Size("out.jpg") < fsize);

	std::ifstream infile("out.jpg", std::ios::binary);
	Encoding::Jpg.Decode(infile, im2);

	REQUIRE(im2.GetSize() == im1.GetSize());
	REQUIRE(im2.GetMode() == im1.GetMode());

	REQUIRE(similar(im1, im2, 30));


	im1.Resize({mw, mw}, Graphics::ColorMode::Grayscale);

	for(int y=0; y<mw; y++) {
		for(int g=0; g<mw; g++) {
			im1({g, y}, 0) = g*diff;
		}
	}

	std::vector<Byte> vec;
	Encoding::Jpg.Encode(im1, vec);

	Encoding::Jpg.Decode(vec, im2);

	REQUIRE(im2.GetSize() == im1.GetSize());
	REQUIRE(im2.GetMode() == im1.GetMode());

	REQUIRE(similar(im1, im2, 2));
}
