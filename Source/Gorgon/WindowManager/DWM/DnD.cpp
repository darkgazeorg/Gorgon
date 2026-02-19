#include "DWM.h"

#include "../../Window.h"
#include "../../Input/DnD.h"


#include <ShlObj.h>

#undef max

namespace Gorgon :: WindowManager {

	HRESULT __stdcall GGEDropTarget::DragEnter(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
		// does the data object contain data we want?
		FORMATETC fileformat = {CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		FORMATETC textformat = {CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

		if(pDataObject->QueryGetData(&fileformat)==S_OK) {
			islocal = parent->IsLocalPointer();
			if(islocal)
				parent->SwitchToWMPointers();

			auto data = new FileData();

			STGMEDIUM stgmed;
			std::string name;
			pDataObject->GetData(&fileformat, &stgmed);
			DROPFILES *fdata = (DROPFILES *)GlobalLock(stgmed.hGlobal);


			wchar_t *widetext = (wchar_t *)((char *)(fdata)+fdata->pFiles);

			while(widetext[0]) {
				name.resize(wcslen(widetext));

				wcstombs(&name[0], widetext, wcslen(widetext)+1);
				OS::winslashtonormal(name);
				data->AddFile(name);

				widetext += wcslen(widetext)+1;
			}

			GlobalUnlock(stgmed.hGlobal);
			ReleaseStgMedium(&stgmed);


			auto &drag = Input::PrepareDrag();
			drag.AssumeData(*data);
			Input::GetDragOperation().MarkAsOS();
			Input::GetDragOperation().DataReady();
			Input::StartDrag();

			*pdwEffect = DROPEFFECT_MOVE;
		}

		if(pDataObject->QueryGetData(&textformat) == S_OK) {
			islocal = parent->IsLocalPointer();
			if(islocal)
				parent->SwitchToWMPointers();

			STGMEDIUM stgmed;
			std::string textdata;
			pDataObject->GetData(&textformat, &stgmed);
			char *text = (char *)GlobalLock(stgmed.hGlobal);

			textdata = text;

			if(Input::IsDragging()) {
				auto &drag = Input::GetDragOperation();
				drag.AddTextData(textdata);
			}
			else {
				Input::BeginDrag(textdata);
				Input::GetDragOperation().MarkAsOS();
				Input::GetDragOperation().DataReady();
			}

			GlobalUnlock(stgmed.hGlobal);
			ReleaseStgMedium(&stgmed);

			*pdwEffect = DROPEFFECT_COPY;
		}
		else {
			*pdwEffect = DROPEFFECT_NONE;
		}

		return S_OK;
	}
	
	HRESULT __stdcall GGEDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
		if(!Input::IsDragging() || !Input::GetDragOperation().HasTarget()) {
			*pdwEffect = DROPEFFECT_NONE;
		}
		else {
			auto &drag = Input::GetDragOperation();

			if(drag.HasData(Resource::GID::File)) {
				auto &data = dynamic_cast<FileData &>(drag.GetData(Resource::GID::File));

				if(data.Action==data.Move)
					*pdwEffect = DROPEFFECT_MOVE;
				else
					*pdwEffect = DROPEFFECT_COPY;
			}
			else if(drag.HasData(Resource::GID::Text)) {
				*pdwEffect = DROPEFFECT_COPY;
			}
			else {
				*pdwEffect = DROPEFFECT_NONE;
			}
		}

		return S_OK;
	}
	
	HRESULT __stdcall GGEDropTarget::DragLeave(void) {
		Input::CancelDrag();

		if(islocal)
			parent->SwitchToLocalPointers();

		return S_OK;
	}
	
	HRESULT __stdcall GGEDropTarget::Drop(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
		Input::Drop(parent->GetMouseLocation());

		if(islocal)
			parent->SwitchToLocalPointers();

		return S_OK;
	}

}