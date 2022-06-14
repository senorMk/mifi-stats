#include "App.h"
#include "AppFrame.h"

IMPLEMENT_APP(MiFiStatsApp);

MiFiStatsApp::~MiFiStatsApp()
{
}

bool MiFiStatsApp::OnInit()
{
	wxApp::SetAppName("MiFiStats");

	try
	{
		pFrame = new AppFrame(0L);
		pFrame->SetIcon(wxICON(MainIcon));
		pFrame->Show();
	}
	catch (...)
	{
		return false;
	}

	wxInitAllImageHandlers();

	return true;
}