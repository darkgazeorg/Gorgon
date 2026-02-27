#include "DWM.h"

#include "../../WindowManager.h"
#include <ShellScalingApi.h>

namespace Gorgon :: WindowManager {
namespace internal {
	BOOL CALLBACK monitordata::MonitorEnumProc(
		_In_ HMONITOR hMonitor,
		_In_ HDC      hdcMonitor,
		_In_ LPRECT   lprcMonitor,
		_In_ LPARAM   dwData
	) {
		MONITORINFOEX mi;
		mi.cbSize = sizeof(MONITORINFOEX);
		GetMonitorInfo(hMonitor, &mi);
		auto mon = new Monitor();
		mon->data->handle = hMonitor;
		mon->name = UnicodeToMByte(mi.szDevice);
		mon->area = {mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right-mi.rcMonitor.left, mi.rcMonitor.bottom-mi.rcMonitor.top};
		mon->usable = {mi.rcWork.left, mi.rcWork.top, mi.rcWork.right, mi.rcWork.bottom};
		mon->isprimary = (mi.dwFlags&MONITORINFOF_PRIMARY)!=0;
		DEVICE_SCALE_FACTOR factor = DEVICE_SCALE_FACTOR_INVALID;
		GetScaleFactorForMonitor(hMonitor, &factor);
		if(factor > 0) mon->scalefactor = int(factor) / 100.f;

		DEVMODE devmode = {};
		devmode.dmSize = sizeof(DEVMODE);
		EnumDisplaySettings(mi.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
		
		mon->scalefactor = float(mon->area.Width) / devmode.dmPelsWidth;

		if(mon->isprimary) {
			Monitor::primary = mon;
			Monitor::monitors.Insert(mon, 0);
		}
		else {
			Monitor::monitors.Add(mon);
		}
		return true;
	}

}

	Monitor::Monitor() {
		data = new internal::monitordata;
	}

	Monitor::~Monitor() {
		delete data;
	}

	void Monitor::Refresh(bool force) {
		monitors.Destroy();
		primary = nullptr;
		EnumDisplayMonitors(nullptr, nullptr, &internal::monitordata::MonitorEnumProc, 0);
	}

	bool Monitor::IsChangeEventSupported() {
		return false;
	}


	Event<> Monitor::ChangedEvent;
	Containers::Collection<Monitor> Monitor::monitors;
	Monitor *Monitor::primary = nullptr;

}
