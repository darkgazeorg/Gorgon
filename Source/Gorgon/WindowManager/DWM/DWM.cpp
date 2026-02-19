#include "DWM.h"

#include "../../WindowManager.h"




namespace Gorgon :: WindowManager {

		/// @cond INTERNAL
		HCURSOR defaultcursor;

		Geometry::Point GetMousePosition(Gorgon::internal::windowdata *wind) {
			POINT pnt;
			RECT winrect;
			Geometry::Point ret;
			GetWindowRect((HWND)wind->handle, &winrect);
			GetCursorPos(&pnt);

			ret.X=pnt.x-(wind->chrome.Left+winrect.left);
			ret.Y=pnt.y-(wind->chrome.Top+winrect.top);

			return ret;
		}

		namespace internal {

			void switchcontext(Gorgon::internal::windowdata &data) {
				if(context==reinterpret_cast<intptr_t>(&data)) return;
				wglMakeCurrent(data.device_context, data.context);
				context=reinterpret_cast<intptr_t>(&data);
			}

			void finalizerender(Gorgon::internal::windowdata &data) {
				SwapBuffers(data.device_context);
			}

		}
		/// @endcond

		void init() {
			defaultcursor=LoadCursor(NULL, IDC_ARROW);

			Monitor::Refresh(true);

			cf_png = RegisterClipboardFormat(L"PNG");
			cf_urilist = RegisterClipboardFormat(L"URIList");
			cf_g_bmp = RegisterClipboardFormat(L"[Gorgon]Bitmap");
			cf_html = RegisterClipboardFormat(L"HTML Format");
		}

		Icon::Icon(const Containers::Image &image) {
			data = new internal::icondata;

            data->icon = 0;
            FromImage(image);
		}
		
		Icon::Icon() {
			data = new internal::icondata;
            data->icon = 0;
        }
        
        Icon::Icon(Icon &&icon) {
            data = new internal::icondata;
            std::swap(data, icon.data);
        }
        
        Icon &Icon::operator =(Icon &&icon) {
            Destroy();
            
            std::swap(data, icon.data);
            return *this;
        }
        
        void Icon::FromImage(const Containers::Image &image) {
			LONG dwWidth, dwHeight;
			BITMAPV5HEADER bi;
			HBITMAP hBitmap;
			void *lpBits;
			HICON hAlphaIcon = NULL;

			dwWidth  = image.GetWidth(); 
			dwHeight = image.GetHeight();

			ZeroMemory(&bi, sizeof(BITMAPV5HEADER));
			bi.bV5Size           = sizeof(BITMAPV5HEADER);
			bi.bV5Width           = dwWidth;
			bi.bV5Height          = -dwHeight;
			bi.bV5Planes = 1;
			bi.bV5BitCount = 32;
			bi.bV5Compression = BI_BITFIELDS;
			// The following mask specification specifies a supported 32 BPP
			// alpha format for Windows XP.
			bi.bV5RedMask   =  0x00FF0000;
			bi.bV5GreenMask =  0x0000FF00;
			bi.bV5BlueMask  =  0x000000FF;
			bi.bV5AlphaMask =  0xFF000000;

			HDC hdc;
			hdc = GetDC(NULL);

			// Create the DIB section with an alpha channel.
			hBitmap = CreateDIBSection(hdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS,
				(void **)&lpBits, NULL, (DWORD)0);



			// Create an empty mask bitmap.
			HBITMAP hMonoBitmap = CreateBitmap(dwWidth, dwHeight, 1, 1, NULL);

			image.CopyToBGRABuffer((Byte*)lpBits);

			ICONINFO ii;
			ii.fIcon = TRUE;  // Change fIcon to TRUE to create an alpha icon
			ii.xHotspot = 0;
			ii.yHotspot = 0;
			ii.hbmMask = hMonoBitmap;
			ii.hbmColor = hBitmap;

			// Create the alpha cursor with the alpha DIB section.
			hAlphaIcon = CreateIconIndirect(&ii);

			DeleteObject(hBitmap);
			DeleteObject(hMonoBitmap);

			data->icon = hAlphaIcon;
        }
        
        void Icon::Destroy() {
            if(data->icon) {
                DeleteObject(data->icon);
                data->icon = 0;
            }
        }

		Icon::~Icon() {
            Destroy();
            
			delete data;
		}
	}
