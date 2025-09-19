#include <wx/wx.h>
#include <wx/stdpaths.h>
#include <wx/filepicker.h>
#include <wx/gauge.h>
#include <wx/datetime.h>
#include <wx/statbmp.h>
#include <wx/filesys.h>
#include <wx/fs_mem.h>
#include <wx/bitmap.h>
#include <wx/image.h>

#include <fstream>

#include "custom.h"

class AppFrame : public wxFrame {
public:
	AppFrame( const wxString &title );

private:
	void CreateControls();
	void BindEvents();

	wxFilePickerCtrl *_pathPk;
	wxButton *_btnOpen;
	wxCheckBox *_cbxDelArc;
	wxCheckBox *_cbxChgPfix;
	wxCheckBox *_cbxLogFil;
	wxCheckBox *_cbxKeysExcl;
	TxtBox *_tbKeys;
	wxCheckBox *_cbxConvJpg;
	wxGauge *_bar;
	TxtBox *_tbW;
	TxtBox *_tbH;
	TxtBox *_tbMxSize;
	wxButton *_btnResize;
	wxButton *_btnRename;
	wxButton *_btnCheck;
	TxtBox *_tbLog;
	wxStaticText *_txPercent;
	wxStaticText *_txStatus;
	wxStaticBitmap *_bmpIcon;

	void OnOpenButtonClick( wxCommandEvent &event );
	void OnPathChanged( wxFileDirPickerEvent &event );
    void OnKeyDown(wxKeyEvent &event);  // 新添加的方法
};

AppFrame::AppFrame( const wxString &title ): wxFrame( nullptr, wxID_ANY, title ) {
	CreateControls();
	BindEvents();
}


void AppFrame::CreateControls() {
	auto pl = new wxPanel( this );
	auto szMain = new wxBoxSizer( wxVERTICAL );

	pl->SetBackgroundColour( wxColour( 40, 40, 40 ) );
	auto szPad = new wxBoxSizer( wxVERTICAL );
	szPad->Add( szMain, 1, wxEXPAND | wxALL, 10 );
	pl->SetSizer( szPad );

	wxBitmap bmp;

	wxFileSystem fs;

	wxString pathRss = wxStandardPaths::Get().GetResourcesDir();
	wxString pathIco = pathRss + "/ico64.png";
	wxFSFile *file = fs.OpenFile( pathIco );

	if ( file ) {
		bmp = wxBitmap( wxImage( *file->GetStream(), wxBITMAP_TYPE_PNG ) );
		delete file;
	}
	else { wxLogError( "開啟File失敗: %s", pathIco ); }

	_bmpIcon = new wxStaticBitmap( pl, wxID_ANY, bmp );
	szMain->Add( _bmpIcon, 0, wxALIGN_CENTER | wxBOTTOM, 10 );

	auto szPath = new wxBoxSizer( wxHORIZONTAL );
	_pathPk = new wxFilePickerCtrl( pl, wxID_ANY, wxEmptyString, wxEmptyString, wxFileSelectorDefaultWildcardStr, wxDefaultPosition, wxSize( 300, -1 ) );
	_btnOpen = new wxButton( pl, wxID_ANY, "Open", wxDefaultPosition, wxSize( 60, -1 ) );
	szPath->Add( _pathPk, 1, wxEXPAND | wxRIGHT, 5 );
	szPath->Add( _btnOpen, 0 );

	szMain->Add( szPath, 0, wxEXPAND | wxBOTTOM, 10 );

	auto szOpts = new wxStaticBoxSizer( wxHORIZONTAL, pl, "Options" );
	_cbxDelArc = new wxCheckBox( szOpts->GetStaticBox(), wxID_ANY, "Delete Archive" );
	_cbxChgPfix = new wxCheckBox( szOpts->GetStaticBox(), wxID_ANY, "Change (C##) prefix last" );
	_cbxLogFil = new wxCheckBox( szOpts->GetStaticBox(), wxID_ANY, "Log To File" );
	szOpts->Add( _cbxDelArc, 0, wxRIGHT, 10 );
	szOpts->Add( _cbxChgPfix, 0, wxRIGHT, 10 );
	szOpts->Add( _cbxLogFil, 0 );

	szMain->Add( szOpts, 0, wxEXPAND | wxBOTTOM, 10 );

	auto szKeys = new wxStaticBoxSizer( wxVERTICAL, pl, "FileName Keywords" );
	auto szKeysC = new wxBoxSizer( wxHORIZONTAL );
	_cbxKeysExcl = new wxCheckBox( szKeys->GetStaticBox(), wxID_ANY, "Delete" );
	_tbKeys = new TxtBox( szKeys->GetStaticBox(), wxID_ANY );
	szKeysC->Add( _cbxKeysExcl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );
	szKeysC->Add( _tbKeys, 1, wxALIGN_CENTER_VERTICAL );
	szKeys->Add( szKeysC, 1, wxEXPAND | wxALL, 5 );

	szMain->Add( szKeys, 0, wxEXPAND | wxBOTTOM, 10 );

	auto szImgSet = new wxStaticBoxSizer( wxVERTICAL, pl, "Convert Image to JPG Settings" );
	auto szImgSetC = new wxBoxSizer( wxHORIZONTAL );
	_cbxConvJpg = new wxCheckBox( szImgSet->GetStaticBox(), wxID_ANY, "Convert to JPG" );
	_bar = new wxGauge( szImgSet->GetStaticBox(), wxID_ANY, 100, wxDefaultPosition, wxSize( -1, 20 ) );
	szImgSetC->Add( _cbxConvJpg, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10 );
	szImgSetC->Add( _bar, 1, wxALIGN_CENTER_VERTICAL );

	auto szDim = new wxBoxSizer( wxHORIZONTAL );
	_tbW = new TxtBox( szImgSet->GetStaticBox(), wxID_ANY, "1680", wxDefaultPosition, wxSize( 50, -1 ) );
	_tbH = new TxtBox( szImgSet->GetStaticBox(), wxID_ANY, "1280", wxDefaultPosition, wxSize( 50, -1 ) );
	_tbMxSize = new TxtBox( szImgSet->GetStaticBox(), wxID_ANY, "450", wxDefaultPosition, wxSize( 50, -1 ) );
	szDim->Add( new wxStaticText( szImgSet->GetStaticBox(), wxID_ANY, "Width:" ), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );
	szDim->Add( _tbW, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10 );
	szDim->Add( new wxStaticText( szImgSet->GetStaticBox(), wxID_ANY, "Height:" ), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );
	szDim->Add( _tbH, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10 );
	szDim->Add( new wxStaticText( szImgSet->GetStaticBox(), wxID_ANY, "MaxSize:" ), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );
	szDim->Add( _tbMxSize, 0, wxALIGN_CENTER_VERTICAL );

	szImgSet->Add( szImgSetC, 0, wxEXPAND | wxALL, 5 );
	szImgSet->Add( szDim, 0, wxEXPAND | wxALL, 5 );

	szMain->Add( szImgSet, 0, wxEXPAND | wxBOTTOM, 10 );

	auto szBtns = new wxBoxSizer( wxHORIZONTAL );
	_btnResize = new wxButton( pl, wxID_ANY, "Resize" );
	_btnRename = new wxButton( pl, wxID_ANY, "Rename" );
	_btnCheck = new wxButton( pl, wxID_ANY, "Check" );
	szBtns->Add( _btnResize, 0, wxRIGHT, 5 );
	szBtns->Add( _btnRename, 0, wxRIGHT, 5 );
	szBtns->Add( _btnCheck );

	szMain->Add( szBtns, 0, wxALIGN_RIGHT | wxBOTTOM, 10 );

	_tbLog = new TxtBox( pl, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1, 150 ), wxTE_MULTILINE   );
	_tbLog->SetBackgroundColour( wxColour( 50, 50, 50 ) );
	//_tbLog->SetForegroundColour( wxColour( 200, 200, 200 ) );
	szMain->Add( _tbLog, 1, wxEXPAND | wxBOTTOM, 10 );

	auto szStatus = new wxBoxSizer( wxHORIZONTAL );
	_txPercent= new wxStaticText( pl, wxID_ANY, "0/0", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER );
	_txStatus = new wxStaticText( pl, wxID_ANY, "Ready", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER );
	_txStatus->SetBackgroundColour( wxColour( 60, 60, 60 ) );
	_txStatus->SetForegroundColour( wxColour( 200, 200, 200 ) );
	szStatus->Add( _txPercent, 2, wxLEFT | wxEXPAND );
	szStatus->Add( _txStatus, 8, wxEXPAND );
	szMain->Add( szStatus, 0, wxEXPAND| wxBOTTOM, 10 );

	SetMinSize( wxSize( 500, 600 ) );
	pl->SetSizerAndFit( szPad );
	SetClientSize( pl->GetSize() );
}


void AppFrame::BindEvents() {
	_btnOpen->Bind( wxEVT_BUTTON, &AppFrame::OnOpenButtonClick, this );
	_pathPk->Bind( wxEVT_FILEPICKER_CHANGED, &AppFrame::OnPathChanged, this );

	this->Bind(wxEVT_CHAR_HOOK, &AppFrame::OnKeyDown, this);
}

void AppFrame::OnOpenButtonClick( wxCommandEvent &event ) {
	wxString path = _pathPk->GetPath();
	if ( !path.IsEmpty() ) wxLaunchDefaultApplication( path );

}

void AppFrame::OnPathChanged( wxFileDirPickerEvent &event ) {
	_tbLog->AppendText( "Path changed to: " + event.GetPath() + "\n" );
}

void AppFrame::OnKeyDown(wxKeyEvent &event) {
	if (event.GetModifiers() == wxMOD_CMD && event.GetKeyCode() == 'Q') {
		Close(true);
	} else {
		event.Skip();
	}
}

class ComicHelperApp : public wxApp {
public:
	virtual bool OnInit() override {
		if ( !wxApp::OnInit() ) return false;

		wxImage::AddHandler( new wxPNGHandler() );

		auto frame = new AppFrame( "ComicHelper" );
		frame->Show( true );
		return true;
	}

	virtual void OnUnhandledException() override {
		LogError( "Unhandled exception occurred." );
	}

	virtual bool OnExceptionInMainLoop() override {
		LogError( "Exception in main loop occurred." );
		return false;
	}

	virtual void OnAssertFailure( const wxChar *file, int line, const wxChar *func,
		const wxChar *cond, const wxChar *msg ) override {
		wxString assertMessage = wxString::Format(
			"Assert failure in file '%s', line %d, in function '%s'.\n"
			"Condition: %s\n"
			"Message: %s",
			file, line, func, cond, msg
		);
		LogError( assertMessage );
	}

private:
	void LogError( const wxString &message ) {
		auto now = wxDateTime::Now();
		auto timestamp = now.Format( "%Y%m%d%H%M%S", wxDateTime::Local );
		auto logMessage = wxString::Format( "%s| %s\n", timestamp, message );

		std::ofstream logFile( "ui.error.log", std::ios_base::app );
		if ( logFile.is_open() ) {
			logFile << logMessage;
			logFile.close();
		}

#ifdef _DEBUG
        wxMessageBox(logMessage, "Error", wxOK | wxICON_ERROR);
#endif
	}
};

wxIMPLEMENT_APP( ComicHelperApp );
