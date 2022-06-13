#include "AppFrame.h"
#include <wx/sstream.h>
#include <wx/xml/xml.h>
#include <wx/richmsgdlg.h>
#include <curl/curl.h>

size_t WriteData(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	size_t Written = fwrite(ptr, size, nmemb, stream);

	return Written;
}

unsigned long long DigitFromString(const wxString& String)
{
	if (String.IsEmpty())
		return 0;

	unsigned long long Result = 0;

	try
	{
		Result = std::stoull(String.ToStdString());
	}
	catch (std::exception& except)
	{
		wxRichMessageDialog  Dlg(nullptr, wxString::Format(_("Caught an exception:\n\n %s"), wxString(except.what())),
			_("Something Went Wrong:"), wxOK | wxICON_INFORMATION | wxCENTER);
		Dlg.ShowModal();
	}

	return Result;
}

wxString SizeToString(unsigned long long Size)
{
	char buffer[256] = { 0 };

	if (Size < _1MB)
	{
		sprintf(buffer, "%.02f KB", (double)Size / 1024);
		return wxString(buffer);
	}
	else if (Size >= _1MB && Size < _1GB)
	{
		sprintf(buffer, "%.02f MB", (double)Size / _1MB);
		return wxString(buffer);
	}

	sprintf(buffer, "%.02f GB", (double)Size / _1GB);

	return wxString(buffer);
}

size_t SendEventCurlWriteCallback(void* contents, size_t size, size_t nmemb, std::string* response)
{
	size_t NewLength = size * nmemb;

	try
	{
		response->append((char*)contents, NewLength);
	}
	catch (std::bad_alloc& except)
	{
		wxRichMessageDialog  Dlg(nullptr, wxString::Format(_("Caught an exception:\n\n %s"), wxString(except.what())),
			_("Something Went Wrong:"), wxOK | wxICON_INFORMATION | wxCENTER);
		Dlg.ShowModal();

		return 0;
	}

	return NewLength;
}

wxString SecondsToTime(long Seconds)
{
	char buffer[50] = { 0 };
	sprintf(buffer, "%02ld:%02ld:%02ld", Seconds / 3600, (Seconds % 3600) / 60, Seconds % 60);

	return wxString(buffer);
}

AppFrame::AppFrame(wxWindow* parent)
	: AppFrameBase(parent)
{
	Load();
}

AppFrame::~AppFrame()
{
}

void AppFrame::OnClose(wxCloseEvent& event)
{
	Destroy();
}

void AppFrame::OnExit(wxCommandEvent& event)
{
	Destroy();
}

void AppFrame::OnMinimize(wxCommandEvent& event)
{
}

void AppFrame::OnBackgroundTimer(wxTimerEvent& event)
{
	CheckForUpdates();
}

void AppFrame::Load()
{
	if (wxFileExists(CookieFile))
	{
		wxRemoveFile(CookieFile);
	}

	CURLcode Response;
	CURL* Curl = curl_easy_init();

	if (!Curl)
	{
		return;
	}

	const wxString HomeURL = "http://192.168.8.1/html/home.html";
	std::string OutputString;

	curl_easy_setopt(Curl, CURLOPT_URL, HomeURL.mb_str(wxConvUTF8).data());
	curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, SendEventCurlWriteCallback);
	curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &OutputString);
	curl_easy_setopt(Curl, CURLOPT_VERBOSE, true);
	curl_easy_setopt(Curl, CURLOPT_COOKIEJAR, CookieFile.mb_str(wxConvUTF8).data());
	curl_easy_setopt(Curl, CURLOPT_TIMEOUT, 5L);

	Response = curl_easy_perform(Curl);

	curl_easy_cleanup(Curl);

	if (Response == CURLE_OK)
	{
		m_pBackgroundTimer->Start(1000);
	}
	else
	{
		wxRichMessageDialog  Dlg(this, "Couldn't start the timer.",
			_("Something Went Wrong:"), wxOK | wxICON_INFORMATION | wxCENTER);
		Dlg.ShowModal();

		const wxString empty = "";
		CurrentConnectTime->SetValue(empty);
		CurrentUpload->SetValue(empty);
		CurrentDownload->SetValue(empty);
		TotalUpload->SetValue(empty);
		TotalDownload->SetValue(empty);
		TotalUploadDownload->SetValue(empty);
		TotalConnectTime->SetValue(empty);
	}
}

void AppFrame::CheckForUpdates()
{
	CURLcode Response;
	CURL* Curl = curl_easy_init();

	if (!Curl)
	{
		return;
	}
	
	const wxString StatsURL = "http://192.168.8.1/api/monitoring/traffic-statistics";
	std::string OutputString;
	
	curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, SendEventCurlWriteCallback);
	curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &OutputString);
	curl_easy_setopt(Curl, CURLOPT_URL, StatsURL.mb_str(wxConvUTF8).data());
	curl_easy_setopt(Curl, CURLOPT_VERBOSE, true);
	curl_easy_setopt(Curl, CURLOPT_COOKIEFILE, CookieFile.mb_str(wxConvUTF8).data());

	Response = curl_easy_perform(Curl);

	if (Response != CURLE_OK)
	{
		const wxString empty = "";
		CurrentConnectTime->SetValue(empty);
		CurrentUpload->SetValue(empty);
		CurrentDownload->SetValue(empty);
		TotalUpload->SetValue(empty);
		TotalDownload->SetValue(empty);
		TotalUploadDownload->SetValue(empty);
		TotalConnectTime->SetValue(empty);

		return;
	}

	wxStringInputStream InputStream(OutputString);

	wxXmlDocument doc;
	{
		wxLogNull LogNoMore;
		if (!doc.Load(InputStream))
		{
			return;
		}
	}

	wxXmlNode* child = doc.GetRoot()->GetChildren();

	unsigned long long TotalUploadBytes = 0, TotalDownloadBytes = 0;

	while (child)
	{
		wxString ChildName = child->GetName();

		if (ChildName == "code")
		{
			wxString ChildContent = child->GetNodeContent();

			unsigned long long Code = DigitFromString(ChildContent);

			// Error
			if (Code == 125002)
			{
				curl_easy_cleanup(Curl);
				m_pBackgroundTimer->Stop();
				Sleep(2000);
				Load();

				return;
			}
		}
		else if (ChildName == "error")
		{
			curl_easy_cleanup(Curl);
			return;
		}


		wxXmlNode* pSubChild = child->GetChildren();
		if (!pSubChild)
		{
			curl_easy_cleanup(Curl);
			return;
		}

		wxString ChildText = pSubChild->GetContent();

		unsigned long long Digit = DigitFromString(ChildText);

		if (ChildName == "CurrentConnectTime")
		{
			wxString t = SecondsToTime(Digit);
			CurrentConnectTime->SetValue(t);
		}
		else if (ChildName == "CurrentUpload")
		{
			wxString d = SizeToString(Digit);
			CurrentUpload->SetValue(d);
		}
		else if (ChildName == "CurrentDownload")
		{
			wxString d = SizeToString(Digit);
			CurrentDownload->SetValue(d);
		}
		else if (ChildName == "TotalUpload")
		{
			wxString d = SizeToString(Digit);
			TotalUpload->SetValue(d);
			TotalUploadBytes += Digit;
		}
		else if (ChildName == "TotalDownload")
		{
			wxString d = SizeToString(Digit);
			TotalDownload->SetValue(d);
			TotalDownloadBytes += Digit;

			TotalUploadDownload->SetValue(SizeToString(TotalUploadBytes + TotalDownloadBytes));
		}
		else if (ChildName == "TotalConnectTime")
		{
			wxString t = SecondsToTime(Digit);
			TotalConnectTime->SetValue(t);
		}

		child = child->GetNext();
	}
	
	curl_easy_cleanup(Curl);
}