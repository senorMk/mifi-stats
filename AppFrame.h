#pragma once

#include <wx/wx.h>
#include "AppFrameBase.h"

const auto _1MB = 1048576;

const auto _1GB = 1073741824;

class AppFrame : public AppFrameBase
{
	public:
		AppFrame(wxWindow* parent);
		virtual ~AppFrame();

		void OnClose(wxCloseEvent& event);

	protected:
		void OnBackgroundTimer(wxTimerEvent& event);
		void OnExit(wxCommandEvent& event);
		void OnMinimize(wxCommandEvent& event);

	private:
		void CheckForUpdates();
		void Load();

	private:
		const wxString CookieFile = "cookies.txt";
};
