#pragma once

#include <wx/app.h>

class AppFrame;

class MiFiStatsApp : public wxApp
{
	public:
		~MiFiStatsApp();
		virtual bool OnInit();

	private:
		AppFrame* pFrame = nullptr;
};