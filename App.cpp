#include "App.h"
#include "AppFrame.h"

IMPLEMENT_APP(MiFiStatsApp);

MiFiStatsApp::~MiFiStatsApp()
{
}

bool MiFiStatsApp::OnInit()
{
	wxApp::SetAppName("HuaStats");

	try
	{
		pFrame = new AppFrame(0L);
		pFrame->Show();
	}
	catch (...)
	{
		return false;
	}

	wxInitAllImageHandlers();

	return true;
}