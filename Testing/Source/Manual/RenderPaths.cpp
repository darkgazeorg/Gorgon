//Draws a scene that uses every drawing path of Gorgon: color modes, filling, tinting, tiling, clipping,
//masks, text and texture updates. The output should be the same with all graphics libraries, which
//allows screenshots of different builds to be compared.
//
//Usage: RenderPaths [static|sized|resize|multi|stress] [seconds]
//  static : only draws the scene
//  sized  : draws the scene after resizing the window once. Should also be the same in all libraries
//  resize : resizes, minimizes and restores the window while drawing
//  multi  : opens and closes a second window that uses the same textures
//  hidden : creates the window hidden, then shows and hides it while drawing
//  stress : creates, updates and destroys textures while they are being used by frames in flight

#include <Gorgon/Window.h>
#include <Gorgon/WindowManager.h>
#include <Gorgon/Main.h>
#include <Gorgon/GL.h>
#include <Gorgon/Graphics/Layer.h>
#include <Gorgon/Graphics/Bitmap.h>
#include <Gorgon/Graphics/Texture.h>
#include <Gorgon/Graphics/FreeType.h>

#ifdef GORGON_GL_VULKAN
#	include <Gorgon/GL/Vulkan.h>
#endif

#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

using namespace Gorgon;
using Graphics::Bitmap;
using Graphics::ColorMode;
using Graphics::RGBAf;
using Graphics::Tiling;

static const char *FontFile = "/usr/share/fonts/noto/NotoSans-Regular.ttf";

/// Every channel has a different gradient, so that swapped channels and flipped images can be noticed.
static Containers::Image MakeImage(int w, int h, ColorMode mode) {
	Containers::Image image({w, h}, mode);

	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			Byte r = Byte(255 * x / (w - 1));
			Byte g = Byte(255 * y / (h - 1));
			Byte b = (x / 8 + y / 8) % 2 ? 200 : 40;
			Byte a = Byte(255 * (x + y) / (w + h - 2));

			switch(mode) {
			case ColorMode::RGBA:
				image(x, y, 0) = r; image(x, y, 1) = g; image(x, y, 2) = b; image(x, y, 3) = a;
				break;
			case ColorMode::BGRA:
				image(x, y, 0) = b; image(x, y, 1) = g; image(x, y, 2) = r; image(x, y, 3) = a;
				break;
			case ColorMode::RGB:
				image(x, y, 0) = r; image(x, y, 1) = g; image(x, y, 2) = b;
				break;
			case ColorMode::BGR:
				image(x, y, 0) = b; image(x, y, 1) = g; image(x, y, 2) = r;
				break;
			case ColorMode::Grayscale:
				image(x, y, 0) = r;
				break;
			case ColorMode::Grayscale_Alpha:
				image(x, y, 0) = r; image(x, y, 1) = a;
				break;
			case ColorMode::Alpha:
				image(x, y, 0) = a;
				break;
			default:
				break;
			}
		}
	}

	return image;
}

static std::unique_ptr<Bitmap> MakeBitmap(int w, int h, ColorMode mode) {
	auto bitmap = std::make_unique<Bitmap>(MakeImage(w, h, mode));
	bitmap->Prepare();

	return bitmap;
}

/// Alpha only disc, used to test soft edged masks
static std::unique_ptr<Bitmap> MakeDisc(int size) {
	auto bitmap = std::make_unique<Bitmap>(Geometry::Size{size, size}, ColorMode::Alpha);
	float r = size / 2.f;

	for(int y = 0; y < size; y++) {
		for(int x = 0; x < size; x++) {
			float dx = x + .5f - r, dy = y + .5f - r;
			float d = r - std::sqrt(dx * dx + dy * dy);

			(*bitmap)(x, y, 0) = Byte(255 * std::min(std::max(d, 0.f), 1.f));
		}
	}

	bitmap->Prepare();

	return bitmap;
}

struct RenderScene {
	std::vector<std::unique_ptr<Bitmap>> bitmaps;

	Graphics::TextureImage updated, resized, partial, nosource;

	std::unique_ptr<Graphics::FreeType> font;

	Bitmap &Add(std::unique_ptr<Bitmap> bitmap) {
		bitmaps.push_back(std::move(bitmap));

		return *bitmaps.back();
	}

	void Load() {
		auto &rgba = Add(MakeBitmap(64, 64, ColorMode::RGBA));
		(void)rgba;

		//Each of these goes through a different texture format and channel mapping
		for(auto mode : {ColorMode::BGRA, ColorMode::RGB, ColorMode::BGR, ColorMode::Grayscale, ColorMode::Grayscale_Alpha, ColorMode::Alpha}) {
			Add(MakeBitmap(64, 64, mode));
		}

		Add(MakeDisc(100));

		//OpenGL makes a filter change on update, size and the color mode can also change
		updated.CreateEmpty({8, 8}, ColorMode::RGBA);
		GL::UpdateTexture(updated.GetID(), MakeImage(64, 64, ColorMode::RGBA));
		updated.Assume(updated.Release(), ColorMode::RGBA, {64, 64});

		//Resized textures have new images
		resized.CreateEmpty({8, 8}, ColorMode::RGBA);
		GL::ResizeTexture(resized.GetID(), {64, 64}, ColorMode::RGBA);
		GL::CopyToTexture(resized.GetID(), MakeImage(64, 64, ColorMode::RGBA), {0, 0});
		resized.Assume(resized.Release(), ColorMode::RGBA, {64, 64});

		//Partial updates, including conversion from RGB to a RGBA texture
		//Contents of an empty texture are undefined in OpenGL
		partial.CreateEmpty({64, 64}, ColorMode::RGBA);
		Containers::Image gray({64, 64}, ColorMode::RGBA);
		gray.Clear();
		GL::CopyToTexture(partial.GetID(), gray, {0, 0});
		GL::CopyToTexture(partial.GetID(), MakeImage(32, 32, ColorMode::RGBA), {0, 0});
		GL::CopyToTexture(partial.GetID(), MakeImage(32, 32, ColorMode::RGBA), {8, 8, 24, 24}, {40, 8});
		GL::CopyToTexture(partial.GetID(), MakeImage(32, 32, ColorMode::RGB), {16, 32});
		partial.Assume(partial.Release(), ColorMode::RGBA, {64, 64});

		font = std::make_unique<Graphics::FreeType>();
		if(!font->LoadFile(FontFile, 18)) {
			std::cout << "Cannot load font: " << FontFile << std::endl;
			font.reset();
		}
	}

	void DrawColorModes(Graphics::Layer &l) {
		for(int i = 0; i < 7; i++) {
			bitmaps[i]->Draw(l, 10 + i * 74, 10);
		}
	}

	void DrawFills(Graphics::Layer &l) {
		l.Draw(10, 84, 64, 64, RGBAf(1.f, 0.f, 0.f, 1.f));
		l.Draw(50, 104, 64, 44, RGBAf(0.f, 0.f, 1.f, 0.5f));

		bitmaps[0]->Draw(l, 124, 84, RGBAf(1.f, .5f, .25f, .75f));
		bitmaps[5]->Draw(l, 198, 84, RGBAf(.25f, 1.f, 1.f, 1.f));
		bitmaps[6]->Draw(l, 272, 84, RGBAf(1.f, 1.f, 0.f, .5f));

		bitmaps[0]->DrawIn(l, Tiling::Both, 346, 84, 96, 64);
		bitmaps[2]->DrawIn(l, Tiling::Horizontal, 452, 84, 96, 64);
		bitmaps[2]->DrawIn(l, Tiling::Vertical, 558, 84, 32, 64);

		//free form quad, rotated and stretched
		bitmaps[2]->Draw(l, 640.f, 84.f, 700.f, 100.f, 680.f, 150.f, 610.f, 130.f);

		//A texture that has no name should be black
		nosource.Draw(l, 710.f, 84.f, 780.f, 84.f, 780.f, 148.f, 710.f, 148.f);
	}

	void DrawTextureUpdates(Graphics::Layer &l) {
		updated.Draw(l, 10, 158);
		resized.Draw(l, 84, 158);
		partial.Draw(l, 158, 158);
	}

	void DrawClipped(Graphics::Layer &clipped, Graphics::Layer &nested, Graphics::Layer &tinted) {
		//Larger than the layer, all of these should be cut at the borders of the layer
		clipped.Draw(-20, -20, 300, 300, RGBAf(.2f, .6f, .2f, 1.f));
		bitmaps[0]->Draw(clipped, -16, -16);
		bitmaps[2]->Draw(clipped, 60, 20);

		nested.Draw(0, 0, 200, 200, RGBAf(.8f, .2f, .6f, 1.f));
		bitmaps[1]->Draw(nested, 4, 4);

		bitmaps[0]->Draw(tinted, 0, 0);
		tinted.Draw(30, 30, 40, 40, RGBAf(1.f));
	}

	void DrawMasked(Graphics::Layer &l, int x, int y) {
		auto &disc = *bitmaps[7];
		auto &alpha = *bitmaps[6];

		using TT = Graphics::TextureTarget;

		l.NewMask();

		l.SetDrawMode(TT::ToMask);
		disc.Draw(l, x, y);
		l.Draw(x, y + 40, 100, 20, RGBAf(1.f, 0.5f));

		l.SetDrawMode(TT::UseMask);
		l.Draw(x, y, 100, 50, RGBAf(1.f, .4f, 0.f, 1.f));
		bitmaps[0]->Draw(l, x + 18, y + 18);
		alpha.Draw(l, x, y + 50, RGBAf(0.f, 1.f, 1.f, 1.f));

		l.SetDrawMode(TT::Normal);

		//Target is switched to the mask and back for the second mask, screen contents should be retained
		l.NewMask();

		l.SetDrawMode(TT::ToMask);
		alpha.Draw(l, x + 120, y);
		bitmaps[2]->Draw(l, x + 120, y + 30);

		l.SetDrawMode(TT::UseMask);
		l.Draw(x + 110, y, 90, 100, RGBAf(.3f, .3f, 1.f, 1.f));

		l.SetDrawMode(TT::Normal);
		l.Draw(x + 210, y, 30, 100, RGBAf(1.f, 1.f, 1.f, 1.f));
	}

	void DrawText(Graphics::Layer &l) {
		if(!font) return;

		font->Print(l, "The quick brown fox jumps over the lazy dog 0123456789", 10, 520);
		font->Print(l, "Tinted text with clipping and blending", 10, 550);
	}
};

static void Background(Graphics::Layer &l) {
	//checker board makes alpha values easier to notice
	for(int y = 0; y < 600; y += 16) {
		for(int x = 0; x < 800; x += 16) {
			float v = (x / 16 + y / 16) % 2 ? .25f : .35f;

			l.Draw(x, y, 16, 16, RGBAf(v, v, v, 1.f));
		}
	}
}

/// Textures are destroyed while frames that use them could still be in flight.
static void Stress(Graphics::Layer &l, RenderScene &scene, int frame) {
	static std::vector<std::unique_ptr<Graphics::TextureImage>> old;
	static Graphics::TextureImage animated;

	l.Clear();

	auto modes = {ColorMode::RGBA, ColorMode::RGB, ColorMode::Alpha, ColorMode::Grayscale_Alpha, ColorMode::BGRA};
	auto mode = *(modes.begin() + frame % modes.size());

	//new texture every frame, the old ones are destroyed while they are in use
	auto image = MakeImage(32 + frame % 32, 32 + frame % 16, mode);
	auto tex = std::make_unique<Graphics::TextureImage>(image);
	tex->Draw(l, 300 + frame % 100, 158);
	old.push_back(std::move(tex));

	if(old.size() > 3) {
		old.erase(old.begin(), old.end() - 1);
	}

	//Replace the same texture name with varying sizes
	if(!animated.GetID()) {
		animated.CreateEmpty({16, 16}, ColorMode::RGBA);
	}
	auto animdata = MakeImage(16 + frame % 48, 16 + frame % 48, ColorMode::RGBA);
	GL::UpdateTexture(animated.GetID(), animdata);
	animated.Assume(animated.Release(), ColorMode::RGBA, animdata.GetSize());
	animated.Draw(l, 450, 158);

	//partial updates of a texture in use, which is also how font atlas is updated
	GL::CopyToTexture(scene.partial.GetID(), MakeImage(16, 16, mode), {frame % 48, frame % 48});
	scene.partial.Draw(l, 550, 158);

	scene.DrawMasked(l, 500 - frame % 50, 400);
}

static void Run(int argc, char **argv) {
	std::string mode = argc > 1 ? argv[1] : "static";
	double seconds = argc > 2 ? std::stod(argv[2]) : 3;

	Gorgon::Initialize("RenderPaths");
	GL::log.InitializeConsole();

	//Tiling window managers ignore resize requests for resizable windows, fixed size windows are floated
	Window wind({800, 600}, "RenderPaths", false, mode != "hidden");
	Graphics::Initialize();

	RenderScene scene;
	scene.Load();

	Graphics::Layer bg, main, clipped(Geometry::Bounds{10, 240, 130, 320}), nested(Geometry::Bounds{40, 10, 240, 50}), tinted(Geometry::Bounds{150, 240, 230, 320}), masked, maskedclip(Geometry::Bounds{250, 240, 400, 320});
	Graphics::Layer stress;

	wind.Add(bg);
	wind.Add(main);
	wind.Add(clipped);
	clipped.Add(nested);
	wind.Add(tinted);
	wind.Add(masked);
	wind.Add(maskedclip);
	wind.Add(stress);

	clipped.EnableClipping();
	nested.EnableClipping();
	maskedclip.EnableClipping();
	tinted.SetTintColor(RGBAf(.5f, 1.f, .5f, .75f));

	Background(bg);
	scene.DrawColorModes(main);
	scene.DrawFills(main);
	scene.DrawTextureUpdates(main);
	scene.DrawClipped(clipped, nested, tinted);
	scene.DrawMasked(masked, 10, 340);
	scene.DrawMasked(maskedclip, 0, 0);
	scene.DrawText(main);

	std::unique_ptr<Window> second;
	std::unique_ptr<Graphics::Layer> secondlayer;

	if(mode == "sized") {
		wind.Resize({640, 480});
	}

	auto start = std::chrono::steady_clock::now();
	int frame = 0, step = -1;

	while(!wind.IsClosed()) {
		double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

		if(elapsed > seconds) break;

		if(mode == "stress") {
			Stress(stress, scene, frame);
		}

		int newstep = int(elapsed * 2);

		if(newstep != step) {
			step = newstep;

			if(mode == "resize") {
				switch(step % 6) {
				case 0: wind.Resize({900, 700}); break;
				case 1: wind.Resize({400, 300}); break;
				case 2: wind.Minimize(); break;
				case 3: wind.Restore(); break;
				case 4: wind.Resize({1, 1}); break;
				case 5: wind.Resize({800, 600}); break;
				}
			}

			if(mode == "hidden") {
				switch(step % 4) {
				case 1: wind.Show(); break;
				case 3: wind.Hide(); break;
				}
			}

			if(mode == "multi") {
				if(step % 2 == 0) {
					second = std::make_unique<Window>(Geometry::Size{300, 200}, "RenderPaths2");
					secondlayer = std::make_unique<Graphics::Layer>();
					second->Add(*secondlayer);
					scene.bitmaps[0]->Draw(*secondlayer, 10, 10);
					secondlayer->Draw(100, 10, 64, 64, RGBAf(0.f, 1.f, 0.f, 1.f));
					scene.DrawMasked(*secondlayer, 0, 90);
				}
				else {
					secondlayer.reset();
					second.reset();
				}
			}
		}

		Gorgon::NextFrame();
		frame++;
	}

	std::cout << "Rendered " << frame << " frames in " << seconds << "s" << std::endl;
}

int main(int argc, char **argv) {
	Run(argc, argv);

	//Everything should be destroyed by now, validation layers will report if anything is leaked.
#ifdef GORGON_GL_VULKAN
	GL::Shutdown();
#endif

	return 0;
}
