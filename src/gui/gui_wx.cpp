#include "gui_wx.h"
#include "calc.h"
#include "guiopenfile.h"
#include "wabbiticon.xpm"

#define BIG_WINDOWS_ICON 0
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#define min(a, b)  (((a) < (b)) ? (a) : (b))


extern wxString CalcModelTxt[11];
bool gif_anim_advance;
bool silent_mode = false;
bool DEBUG = true;
enum
{
	ID_File_New,
	ID_File_Open,
	ID_File_Save,
	ID_File_Gif,
	ID_File_Close,
	ID_File_Quit,

	ID_Edit_Copy,
	ID_Edit_Paste,
	
	ID_Calc_Skin,
	ID_Calc_Sound,
	ID_Calc_Pause,
	ID_Calc_Connect,
	ID_Calc_Vars,
	ID_Calc_Options,
	
	ID_Speed_400,
	ID_Speed_500,
	ID_Speed_200,
	ID_Speed_100,
	ID_Speed_50,
	ID_Speed_25,
	ID_Speed_Custom,
	
	ID_Size_100,
	ID_Size_200,
	ID_Size_300,
	ID_Size_400,
	
	ID_Debug_Reset,
	ID_Debug_Open,
	
	ID_Help_About,
	ID_Help_Website
};

/*BEGIN_EVENT_TABLE(MyFrame, wxFrame)
	EVT_SHOW(MyFrame::OnShow)
	EVT_MENU(ID_Quit, MyFrame::OnQuit)
	EVT_MENU(ID_About, MyFrame::OnAbout)
END_EVENT_TABLE()*/

class MyApp: public wxApp
{
	virtual bool OnInit();
	//virtual int MainLoop();
	void OnTimer(wxTimerEvent& event);
	wxTimer *timer;
public:
	void getTimer(int slot);
};

IMPLEMENT_APP(MyApp)
char* wxStringToChar(wxString input)
{
#if (wxUSE_UNICODE)
		size_t size = input.size() + 1;
		char *buffer = new char[size];//No need to multiply by 4, converting to 1 byte char only.
		memset(buffer, 0, size); //Good Practice, Can use buffer[0] = '&#65533;' also.
		wxEncodingConverter wxec;
		wxec.Init(wxFONTENCODING_ISO8859_1, wxFONTENCODING_ISO8859_1, wxCONVERT_SUBSTITUTE);
		wxec.Convert(input.mb_str(), buffer);
		return buffer; //To free this buffer memory is user responsibility.
#else
		return (char *)(input.c_str());
#endif
}

char load_file_buffer[PATH_MAX];
bool LoadRomIntialDialog() {
	wxString strFilter 	= wxT("\
Known types ( *.sav; *.rom)|*.sav;*.rom|\
Save States  (*.sav)|*.sav|\
ROMs  (*.rom)|*.rom|\
All Files (*.*)|*.*");
	wxFileDialog dialog(NULL, wxT("Wabbitemu: Please select a ROM or save state"),
	wxT(""), wxT(""), strFilter, wxFD_OPEN | wxFD_FILE_MUST_EXIST);//, wxDefaultPosition,wxDefaultSize, "filedlg")
	if (dialog.ShowModal() == wxID_OK) {
		strcpy(load_file_buffer, wxStringToChar(dialog.GetPath()));
		return true;
	}
	else
		return false;
}

bool MyApp::OnInit()
{
	wxImage::AddHandler(new wxPNGHandler);

	memset(calcs, 0, sizeof(calcs));
	int slot = calc_slot_new();
	calcs[slot].SkinEnabled = false;
	slot = rom_load(slot, calcs[slot].rom_path);
	if (slot != -1) gui_frame(slot);
	else {
		bool loadFile = LoadRomIntialDialog();
		//wxMessageBox(string, wxT("OnInit"), wxOK, NULL);
		//printf_d("hello", string);
		if (loadFile) {
			slot = calc_slot_new();
			slot = rom_load(slot, load_file_buffer);
			if (slot != -1) gui_frame(slot);
			else return FALSE;
		} else return FALSE;
	}
	calcs[slot].wxFrame->SetFocus();
	timer = new wxTimer();
	timer->Connect(wxEVT_TIMER, (wxObjectEventFunction) &MyApp::OnTimer);
	timer->Start(FPS, false);
	return TRUE;
}

/*int MyApp::MainLoop() {
	
}*/

unsigned GetTickCount()
{
		struct timeval tv;
		if(gettimeofday(&tv, NULL) != 0)
				return 0;

		return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}


void MyApp::OnTimer(wxTimerEvent& event) {
	static int difference;
	static unsigned prevTimer;
	unsigned dwTimer = GetTickCount();
	
	// How different the timer is from where it should be
	// guard from erroneous timer calls with an upper bound
	// that's the limit of time it will take before the
	// calc gives up and claims it lost time
	difference += ((dwTimer - prevTimer) & 0x003F) - TPF;
	prevTimer = dwTimer;
	//printf_d("%d", difference);

	// Are we greater than Ticks Per Frame that would call for
	// a frame skip?
	if (difference > -TPF) {
		calc_run_all();
		while (difference >= TPF) {
			calc_run_all();
			difference -= TPF;
		}

		int i;
		for (i = 0; i < MAX_CALCS; i++) {
			if (calcs[i].active) {
				gui_draw(i);
			}
		}
	// Frame skip if we're too far ahead.
	} else
		difference += TPF;
}

int gui_draw(int slot) {
	gslot = slot;
	calcs[slot].wxLCD->Refresh();
	calcs[slot].wxLCD->Update();
	calcs[slot].wxFrame->Refresh();
	calcs[slot].wxFrame->Update();

	if (calcs[slot].gif_disp_state != GDS_IDLE) {
		static int skip = 0;
		if (skip == 0) {
			gif_anim_advance = true;
			calcs[slot].wxFrame->Update();
		}
		skip = (skip + 1) % 4;
	}
}

int gui_frame(int slot) {
	if (!calcs[slot].Scale)
    	calcs[slot].Scale = 2; //Set original scale
    	
	// Set gslot so the CreateWindow functions operate on the correct calc
	gslot = slot;
	calcs[slot].wxFrame = new MyFrame(slot);
	calcs[slot].wxFrame->Show(true);

	calcs[slot].wxLCD = new MyLCD(slot);
	calcs[slot].wxLCD->Show(true);
	if (calcs[slot].wxFrame == NULL /*|| calcs[slot].hwndLCD == NULL*/) return -1;
	calcs[slot].running = TRUE;
	calcs[slot].wxFrame->SetSpeed(100);
	
	calcs[slot].wxFrame->Centre(0);   //Centres the frame
	
	gui_frame_update(slot);
	return 0;
}

int gui_frame_update(int slot) {
	LPCALC lpCalc = &calcs[slot];
	wxMenuBar *wxMenu = lpCalc->wxFrame->GetMenuBar();
	lpCalc->calcSkin = wxGetBitmapFromMemory(TI_83p);
	lpCalc->keymap = wxGetBitmapFromMemory(TI_83p_Keymap).ConvertToImage();
	int skinWidth, skinHeight, keymapWidth, keymapHeight;
	
	if (lpCalc->calcSkin.IsOk()) {
		skinWidth = 350;//lpCalc->calcSkin.GetWidth();
		skinHeight = 725;//lpCalc->calcSkin.GetHeight();
	}
	if (lpCalc->keymap.IsOk()) {
		keymapWidth = 350;//lpCalc->keymap.GetWidth();
		keymapHeight = 725;//lpCalc->keymap.GetHeight();
	}
	int foundX = 0, foundY = 0;
	bool foundScreen = false;
	if (((skinWidth != keymapWidth) || (skinHeight != keymapHeight)) && skinHeight > 0 && skinWidth > 0) {
		lpCalc->SkinEnabled = false;
		wxMessageBox(wxT("Skin and Keymap are not the same size"), wxT("Error"),  wxOK, NULL);
	} else {
		lpCalc->SkinSize.SetWidth(skinWidth);
		lpCalc->SkinSize.SetHeight(skinHeight);		//find the screen size
		for(int y = 0; y < skinHeight && foundScreen == false; y++) {
			for (int x = 0; x < skinWidth && foundScreen == false; x++) {
				if (lpCalc->keymap.GetBlue(x, y) == 0 &&
						lpCalc->keymap.GetRed(x, y) == 255 &&
						lpCalc->keymap.GetGreen(x, y) == 0) {
					//81 92
					foundX = x;
					foundY = y;
					foundScreen = true;
				}
			}
		}
		lpCalc->LCDRect.SetLeft(foundX);
		lpCalc->LCDRect.SetTop(foundY);
		do {
			foundX++;
		} while (lpCalc->keymap.GetBlue(foundX, foundY) == 0 &&
				lpCalc->keymap.GetRed(foundX, foundY) == 255 &&
				lpCalc->keymap.GetGreen(foundX, foundY) == 0);
		lpCalc->LCDRect.SetRight(foundX--);
		do {
			foundY++;
		}while (lpCalc->keymap.GetBlue(foundX, foundY) == 0 &&
				lpCalc->keymap.GetRed(foundX, foundY) == 255 &&
				lpCalc->keymap.GetGreen(foundX, foundY) == 0);
		lpCalc->LCDRect.SetBottom(foundY);
	}
	if (!foundScreen) {
		wxMessageBox(wxT("Unable to find the screen box"), wxT("Error"), wxOK, NULL);
		lpCalc->SkinEnabled = false;
	}

	if (wxMenu != NULL) {
		if (!lpCalc->SkinEnabled) {
			wxMenu->Check(ID_Calc_Skin, false);
			// Create status bar
			wxStatusBar *wxStatus = lpCalc->wxFrame->GetStatusBar();
			if (wxStatus == NULL)
				wxStatus = calcs[slot].wxFrame->CreateStatusBar(1, wxST_SIZEGRIP, wxID_ANY );
			const int iStatusWidths[] = {100, -1};
			wxStatus->SetFieldsCount(2, iStatusWidths);
			wxStatus->SetStatusText(CalcModelTxt[lpCalc->model], 1);
			
			wxSize skinSize(128*lpCalc->Scale, 64*lpCalc->Scale+4); //The +4 is important to show all LCD
			
			if (wxMenu)
				skinSize.IncBy(0, wxSystemSettings::GetMetric(wxSYS_MENU_Y, calcs[slot].wxFrame));
			
			
			lpCalc->wxFrame->SetClientSize(skinSize);
			lpCalc->wxFrame->SetSize(128*lpCalc->Scale, 64*lpCalc->Scale+60);
		} else {
			wxMenu->Check(ID_Calc_Skin, true);
			wxStatusBar *wxStatus = lpCalc->wxFrame->GetStatusBar();
			if (wxStatus != NULL) {
				wxStatus->Destroy();
			}
			wxSize skinSize(350, 725);
			if (wxMenu)
				skinSize.IncBy(0, wxSystemSettings::GetMetric(wxSYS_MENU_Y, lpCalc->wxFrame));
			lpCalc->wxFrame->SetClientSize(skinSize);
		}
	}
	
	calcs[slot].wxFrame->SendSizeEvent();
}

extern wxRect db_rect;
int gui_debug(int slot) {
	/*MyDebugger hdebug;
	wxRect pos = {CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT+600, CW_USEDEFAULT+400};
	if (db_rect.left != -1) CopyRect(&pos, &db_rect);

	pos.right -= pos.left;
	pos.bottom -= pos.top;

	if ((hdebug = FindWindow(g_szDebugName, "Debugger"))) {
		SwitchToThisWindow(hdebug, TRUE);
		return -1;
	}
	calcs[slot].running = FALSE;
	hdebug = CreateWindowEx(
		WS_EX_APPWINDOW,
		g_szDebugName,
		"Debugger",
		WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		pos.left, pos.top, pos.right, pos.bottom,
		0, 0, g_hInst, NULL);

	calcs[slot].hwndDebug = hdebug;
	SendMessage(hdebug, WM_SIZE, 0, 0);*/
	return 0;
}

void printf_d( const char* format, ... ) {
	if (DEBUG == false) return;
	va_list args;
	//fprintf( stdout, "Error: " );
	va_start( args, format );
	vfprintf( stdout, format, args );
	va_end( args );
}

MyFrame::MyFrame(int curslot) : wxFrame(NULL, wxID_ANY, wxT("Wabbitemu"))
{
	this->SetWindowStyleFlag(wxBORDER_RAISED);
	slot = curslot;
	wxSize skinSize(350, 725);
	calcs[slot].SkinSize = skinSize;
	LCD_t *lcd = calcs[slot].cpu.pio.lcd;
	int scale = calcs[slot].Scale;
	int draw_width = lcd->width * scale;
	int draw_height = 64 * scale;
	wxRect lcdRect((128 * scale - draw_width) / 2, 0, draw_width, draw_height);
	calcs[slot].LCDRect = lcdRect;

	wxSize windowSize;
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxMenuBar *m_menubar = new wxMenuBar( 0 );
	wxMenu *m_fileMenu = new wxMenu();	
	wxMenuItem* m_newMenuItem;
	m_newMenuItem = new wxMenuItem( m_fileMenu, ID_File_New, wxString( wxT("New") ) + wxT('\t') + wxT("CTRL+N"), wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_newMenuItem );
	
	wxMenuItem* m_openMenuItem;
	m_openMenuItem = new wxMenuItem( m_fileMenu, ID_File_Open, wxString( wxT("Open...") ) + wxT('\t') + wxT("CTRL+O"), wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_openMenuItem );
	
	wxMenuItem* m_saveMenuItem;
	m_saveMenuItem = new wxMenuItem( m_fileMenu, ID_File_Save, wxString( wxT("Save State...") ) , wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_saveMenuItem );
	
	wxMenuItem* m_separator1;
	m_separator1 = m_fileMenu->AppendSeparator();
	
	wxMenuItem* m_gifMenuItem;
	m_gifMenuItem = new wxMenuItem( m_fileMenu, ID_File_Gif, wxString( wxT("Record GIF") ) + wxT('\t') + wxT("Backspace"), wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_gifMenuItem );
	
	wxMenuItem* m_separator2;
	m_separator2 = m_fileMenu->AppendSeparator();
	
	wxMenuItem* m_closeMenuItem;
	m_closeMenuItem = new wxMenuItem( m_fileMenu, ID_File_Close, wxString( wxT("Close") ) + wxT('\t') + wxT("CTRL+W"), wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_closeMenuItem );
	
	wxMenuItem* m_exitMenuItem;
	m_exitMenuItem = new wxMenuItem( m_fileMenu, ID_File_Quit, wxString( wxT("Exit") ) + wxT('\t') + wxT("ALT+F4"), wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_exitMenuItem );
	
	m_menubar->Append( m_fileMenu, wxT("File") );
	
	wxMenu *m_editMenu = new wxMenu();
	wxMenuItem* m_copyMenuItem;
	m_copyMenuItem = new wxMenuItem( m_editMenu, ID_Edit_Copy, wxString( wxT("Copy") ) + wxT('\t') + wxT("CTRL+C"), wxEmptyString, wxITEM_NORMAL );
	m_editMenu->Append( m_copyMenuItem );
	
	wxMenuItem* m_pasteMenuItem;
	m_pasteMenuItem = new wxMenuItem( m_editMenu, ID_Edit_Paste, wxString( wxT("Paste") ) + wxT('\t') + wxT("CTRL+V"), wxEmptyString, wxITEM_NORMAL );
	m_editMenu->Append( m_pasteMenuItem );this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxMenuItem* m_separator5;
	m_separator5 = m_editMenu->AppendSeparator();
	
	wxMenuItem* m_optionsMenuItem;
	m_optionsMenuItem = new wxMenuItem( m_editMenu, ID_Calc_Options, wxString( wxT("Preferences\tCtrl+Q") ) , wxEmptyString, wxITEM_NORMAL );
	m_editMenu->Append( m_optionsMenuItem );
	
	m_menubar->Append( m_editMenu, wxT("Edit") );
	
	wxMenu *m_calcMenu = new wxMenu();
	wxMenuItem* m_skinMenuItem;
	m_skinMenuItem = new wxMenuItem( m_calcMenu, ID_Calc_Skin, wxString( wxT("Enable Skin") ) , wxEmptyString, wxITEM_CHECK );
	m_calcMenu->Append( m_skinMenuItem );
	
	wxMenuItem* m_soundMenuItem;
	m_soundMenuItem = new wxMenuItem( m_calcMenu, ID_Calc_Sound, wxString( wxT("Enable Sound") ) , wxEmptyString, wxITEM_CHECK );
	m_calcMenu->Append( m_soundMenuItem );
	
	wxMenuItem* m_separator3;
	m_separator3 = m_calcMenu->AppendSeparator();
	
	wxMenuItem* m_pauseMenuItem;
	m_pauseMenuItem = new wxMenuItem( m_calcMenu, ID_Calc_Pause, wxString( wxT("Pause Emulation") ) , wxEmptyString, wxITEM_CHECK );
	m_calcMenu->Append( m_pauseMenuItem );
	
	wxMenu *m_speedMenu = new wxMenu();
	m_calcMenu->Append( -1, wxT("Speed"), m_speedMenu );
	
	wxMenuItem* m_setSpeed500;
	m_setSpeed500 = new wxMenuItem( m_speedMenu, ID_Speed_500, wxString( wxT("500%") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_setSpeed500 );
	
	wxMenuItem* m_setSpeed400;
	m_setSpeed400 = new wxMenuItem( m_speedMenu, ID_Speed_400, wxString( wxT("400%") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_setSpeed400 );
	
	wxMenuItem* m_setSpeed200;
	m_setSpeed200 = new wxMenuItem( m_speedMenu, ID_Speed_200, wxString( wxT("200%") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_setSpeed200 );
	
	wxMenuItem* m_setSpeed100;
	m_setSpeed100 = new wxMenuItem( m_speedMenu, ID_Speed_100, wxString( wxT("100%") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_setSpeed100 );
	
	wxMenuItem* m_setSpeed50;
	m_setSpeed50 = new wxMenuItem( m_speedMenu, ID_Speed_50, wxString( wxT("50%") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_setSpeed50 );
	
	wxMenuItem* m_setSpeed25;
	m_setSpeed25 = new wxMenuItem( m_speedMenu, ID_Speed_25, wxString( wxT("25%") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_setSpeed25 );
	
	wxMenuItem* m_separatorSpeedCustom;
	m_separatorSpeedCustom = m_speedMenu->AppendSeparator();
	
	wxMenuItem* m_setSpeedCustom;
	m_setSpeedCustom = new wxMenuItem( m_speedMenu, ID_Speed_Custom, wxString( wxT("Custom...") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_setSpeedCustom );
	
	wxMenu *m_sizeMenu = new wxMenu();
	m_calcMenu->Append(-1,wxT("Size"), m_sizeMenu);
	
	wxMenuItem* m_setSize100;
	m_setSize100 = new wxMenuItem( m_sizeMenu, ID_Size_100, wxString( wxT("100%") ) , wxEmptyString, wxITEM_CHECK );
	m_sizeMenu->Append( m_setSize100 );
	
	wxMenuItem* m_setSize200;
	m_setSize200 = new wxMenuItem( m_sizeMenu, ID_Size_200, wxString( wxT("200%") ) , wxEmptyString, wxITEM_CHECK );
	m_sizeMenu->Append( m_setSize200 );
	
	wxMenuItem* m_setSize300;
	m_setSize300 = new wxMenuItem( m_sizeMenu, ID_Size_300, wxString( wxT("300%") ) , wxEmptyString, wxITEM_CHECK );
	m_sizeMenu->Append( m_setSize300 );
	
	wxMenuItem* m_setSize400;
	m_setSize400 = new wxMenuItem( m_sizeMenu, ID_Size_400, wxString( wxT("400%") ) , wxEmptyString, wxITEM_CHECK );
	m_sizeMenu->Append( m_setSize400 );
	
	wxMenuItem* m_separator4;
	m_separator4 = m_calcMenu->AppendSeparator();
	
	wxMenuItem* m_connectMenuItem;
	m_connectMenuItem = new wxMenuItem( m_calcMenu, ID_Calc_Connect, wxString( wxT("Connect To...") ) , wxEmptyString, wxITEM_NORMAL );
	m_calcMenu->Append( m_connectMenuItem );
	
	wxMenuItem* m_separator6;
	m_separator6 = m_calcMenu->AppendSeparator();
	
	wxMenuItem* m_varsMenuItem;
	m_varsMenuItem = new wxMenuItem( m_calcMenu, ID_Calc_Vars, wxString( wxT("Variables...") ) , wxEmptyString, wxITEM_NORMAL );
	m_calcMenu->Append( m_varsMenuItem );
	
	m_menubar->Append( m_calcMenu, wxT("Calculator") );
	
	wxMenu *m_debugMenu = new wxMenu();
	wxMenuItem* m_resetMenuItem;
	m_resetMenuItem = new wxMenuItem( m_debugMenu, ID_Debug_Reset, wxString( wxT("Reset") ) , wxEmptyString, wxITEM_NORMAL );
	m_debugMenu->Append( m_resetMenuItem );
	
	wxMenuItem* m_debugMenuItem;
	m_debugMenuItem = new wxMenuItem( m_debugMenu, ID_Debug_Open, wxString( wxT("Open Debugger...") ) , wxEmptyString, wxITEM_NORMAL );
	m_debugMenu->Append( m_debugMenuItem );
	
	m_menubar->Append( m_debugMenu, wxT("Debug") );
	
	wxMenu *m_helpMenu = new wxMenu();
	wxMenuItem* m_websiteMenuItem;
	m_websiteMenuItem = new wxMenuItem( m_helpMenu, ID_Help_Website, wxString( wxT("Website") ) , wxEmptyString, wxITEM_NORMAL );
	m_helpMenu->Append( m_websiteMenuItem );
	
	wxMenuItem* m_aboutMenuItem;
	m_aboutMenuItem = new wxMenuItem( m_helpMenu, ID_Help_About, wxString( wxT("About") ) , wxEmptyString, wxITEM_NORMAL );
	m_helpMenu->Append( m_aboutMenuItem );
	
	m_menubar->Append( m_helpMenu, wxT("Help") );
	
	this->SetMenuBar( m_menubar );
	
	wxStatusBar *m_statusBar1 = new wxStatusBar(this);
	this->SetStatusBar(m_statusBar1);
	
	this->Connect(wxEVT_PAINT, wxPaintEventHandler(MyFrame::OnPaint));
	// Resize event connection
	this->Connect(wxEVT_SIZE, (wxObjectEventFunction) &MyFrame::OnResize);
	
	this->Connect(ID_File_New, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnFileNew);
	this->Connect(ID_File_Open, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnFileOpen);
	this->Connect(ID_File_Save, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnFileSave);
	this->Connect(ID_File_Close, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnFileClose);
	this->Connect(ID_File_Quit, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnFileQuit);
	this->Connect(ID_Calc_Pause, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnPauseEmulation);
	
	this->Connect(ID_Speed_Custom, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnSetSpeedCustom);
	this->Connect(ID_Speed_500, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnSetSpeed);
	this->Connect(ID_Speed_400, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnSetSpeed);
	this->Connect(ID_Speed_200, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnSetSpeed);
	this->Connect(ID_Speed_100, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnSetSpeed);
	this->Connect(ID_Speed_50, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnSetSpeed);
	this->Connect(ID_Speed_25, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnSetSpeed);
	
	this->Connect(ID_Size_100, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnSetSize);
	this->Connect(ID_Size_200, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnSetSize);
	this->Connect(ID_Size_300, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnSetSize);
	this->Connect(ID_Size_400, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnSetSize);
	
	this->Connect(ID_Help_About, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnHelpAbout);
	this->Connect(ID_Help_Website, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnHelpWebsite);
	this->Connect(ID_Calc_Skin, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction) &MyFrame::OnCalcSkin);
	this->Connect(wxID_ANY, wxEVT_KEY_DOWN, (wxObjectEventFunction) &MyFrame::OnKeyDown);
	this->Connect(wxID_ANY, wxEVT_KEY_UP, (wxObjectEventFunction) &MyFrame::OnKeyUp);
	/* OnQuit */
	this->Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(MyFrame::OnQuit));
	
	//Speed starts at 100%
	m_menubar->Check(ID_Speed_100, true);
	//Size starts at 200%
	m_menubar->Check(ID_Size_200, true);
	
	//int menuSize = wxSystemSettings::GetMetric(wxSYS_MENU_Y);
	if (calcs[slot].SkinEnabled)
		windowSize = calcs[slot].SkinSize;
	else
		windowSize.Set(128 * calcs[slot].Scale, 64 * calcs[slot].Scale+60);
	
	#if (defined(__WXMSW__) && BIG_WINDOWS_ICON == 1)
	wxBitmap bitmap(wxT(“wabbiticon.png”), wxBITMAP_TYPE_PNG);
	icon.CopyFromBitmap(bitmap);
	#else
	wxIcon icon(wxICON(wabbiticon));
	#endif
	
	SetIcon(icon);
	
	this->SetSize(windowSize);
}

// Resize function
void MyFrame::OnResize(wxSizeEvent& event) {
	event.Skip(true);
	LPCALC lpCalc = &calcs[slot];
	if (lpCalc->SkinEnabled)
		return;
	if (!calcs[slot].SkinEnabled)
	{
		int width_scale = event.GetSize().GetWidth() / 128;
		int height_scale = (event.GetSize().GetHeight()-60) / 64;
		int scale = max(2, max(width_scale, height_scale));

		/*int new_width = event.GetSize().GetWidth();
		int new_height = event.GetSize().GetHeight();
		if (new_width > this->GetSize().GetWidth() || new_height > this->GetSize().GetHeight())
			scale++;
		else
			scale--;*/
		calcs[slot].Scale = max(2, scale);
		this->SetSize(wxDefaultCoord, wxDefaultCoord, scale * 128, scale * 64 + 60, wxSIZE_USE_EXISTING);
	}
}

void MyFrame::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	LPCALC lpCalc = &calcs[slot];
	if (lpCalc->SkinEnabled) {
		lpCalc->wxLCD->Update();
		wxBitmap test = wxGetBitmapFromMemory(TI_83p);
		dc.DrawBitmap(test, 0, 0, true);
	}
}

void MyFrame::OnFileNew(wxCommandEvent &event) {
	char *newFilePath = (char *) malloc(PATH_MAX);
	strcpy(newFilePath, calcs[slot].rom_path);
	int slot = calc_slot_new();
	if (rom_load(slot, newFilePath) != -1) {
		calcs[slot].SkinEnabled = calcs[slot].SkinEnabled;
		calcs[slot].Scale = calcs[slot].Scale;
		gui_frame(slot);
	} else {
		wxMessageBox(wxT("Failed to create new calc"));
	}
}

void MyFrame::OnFileOpen(wxCommandEvent &event) {
	GetOpenSendFileName(slot, 0);
}

void MyFrame::OnFileSave(wxCommandEvent &event) {
	SaveStateDialog(slot);
}

void MyFrame::OnFileClose(wxCommandEvent &event) {
	Close(TRUE);
}

void MyFrame::OnSetSize(wxCommandEvent &event) {
	/* This function is called when user changes size of LCD in menu */
    wxMenuBar *wxMenu = calcs[this->slot].wxFrame->GetMenuBar();
    int eventID;
    wxMenu->Check(ID_Size_100,false);
    wxMenu->Check(ID_Size_200,false);
    wxMenu->Check(ID_Size_300,false);
    wxMenu->Check(ID_Size_400,false);
    
    eventID = event.GetId();
    
    switch (eventID) {
        case ID_Size_100:
            calcs[slot].Scale = 1;  //This is half of the Wabbit default, but equals real LCD
            wxMenu->Check(ID_Size_100,true);
            printf_d("[wxWabbitemu] [OnSetSize] Set Scale 100% \n");
            break;
        case ID_Size_200:
            calcs[slot].Scale = 2; //Wabbit default, twice the LCD
            wxMenu->Check(ID_Size_200,true);
            printf_d("[wxWabbitemu] [OnSetSize] Set Scale 200% \n");
            break;
        case ID_Size_300:
            calcs[slot].Scale = 3;
            wxMenu->Check(ID_Size_300,true);
            printf_d("[wxWabbitemu] [OnSetSize] Set Scale 300% \n");
            break;
        case ID_Size_400:
            calcs[slot].Scale = 4;
            wxMenu->Check(ID_Size_400,true);
            printf_d("[wxWabbitemu] [OnSetSize] Set Scale 400% \n");
            break;
        default:
	    printf_d("[wxWabbitemu] [W] [OnSetSize] Some strange, evil thing called this function. Disregarding. \n");
	    break;
    }
    if (!calcs[slot].SkinEnabled) {
		/* Update size of frame to match the new LCD Size */
        calcs[slot].wxFrame->SetSize(128*calcs[slot].Scale, 64*calcs[slot].Scale+60);
    }
}

void MyFrame::OnSetSpeed(wxCommandEvent &event) {
	printf_d("[wxWabbitemu] [OnSetSpeed] Function called! \n");
	wxMenuBar *wxMenu = calcs[this->slot].wxFrame->GetMenuBar();
	int eventID;
	wxMenu->Check(ID_Speed_500, false);
	wxMenu->Check(ID_Speed_400, false);
	wxMenu->Check(ID_Speed_200, false);
	wxMenu->Check(ID_Speed_100, false);
	wxMenu->Check(ID_Speed_50, false);
	wxMenu->Check(ID_Speed_25, false);
	wxMenu->Check(ID_Speed_Custom, false);
	wxMenu->SetLabel(ID_Speed_Custom, wxString(wxT("Custom...")));
	wxMenu->Check(ID_Speed_Custom, false);
	
	eventID = event.GetId();
	printf_d("[wxWabbitemu] [OnSetSpeed] Got widget ID that called this function: %d \n",eventID);
	switch (eventID) {
		case ID_Speed_100:
			calcs[slot].wxFrame->SetSpeed(100);
			wxMenu->Check(ID_Speed_100, true);
			printf_d("[wxWabbitemu] [OnSetSpeed] Setting emulated calc speed to 100%%. \n");
			break;
		case ID_Speed_200:
			calcs[slot].wxFrame->SetSpeed(200);
			wxMenu->Check(ID_Speed_200, true);
			printf_d("[wxWabbitemu] [OnSetSpeed] Setting emulated calc speed to 200%%. \n");
			break;
		case ID_Speed_25:
			calcs[slot].wxFrame->SetSpeed(25);
			wxMenu->Check(ID_Speed_25, true);
			printf_d("[wxWabbitemu] [OnSetSpeed] Setting emulated calc speed to 25%%. \n");
			break;
		case ID_Speed_400:
			calcs[slot].wxFrame->SetSpeed(400);
			wxMenu->Check(ID_Speed_400, true);
			printf_d("[wxWabbitemu] [OnSetSpeed] Setting emulated calc speed to 400%%. \n");
			break;
		case ID_Speed_50:
			calcs[slot].wxFrame->SetSpeed(50);
			wxMenu->Check(ID_Speed_50, true);
			printf_d("[wxWabbitemu] [OnSetSpeed] Setting emulated calc speed to 50%%. \n");
			break;
		case ID_Speed_500:
			calcs[slot].wxFrame->SetSpeed(500);
			wxMenu->Check(ID_Speed_500, true);
			printf_d("[wxWabbitemu] [OnSetSpeed] Setting emulated calc speed to 500%%. \n");
			break;
		default:
			printf_d("[wxWabbitemu] [W] [OnSetSpeed] Some strange, evil thing called this function. Disregarding. \n");
			break;
	}
}

void MyFrame::OnSetSpeedCustom(wxCommandEvent &event) {
	wxMenuBar *wxMenu = calcs[this->slot].wxFrame->GetMenuBar();
	long resp;
	printf_d("[wxWabbitemu] [OnSetSpeedCustom] Function called! Opening numerical input dialog...\n");
	resp = wxGetNumberFromUser(wxString(wxT("Enter the speed (in percentage) you wish to set:")), wxString(wxT("")), wxString(wxT("Wabbitemu - Custom Speed")), 100, 0, 10000);
	if (resp != -1) {
		printf_d("[wxWabbitemu] [OnSetSpeedCustom] User provided a valid number, so setting speed %%. \n");
		wxMenu->SetLabel(ID_Speed_Custom, wxString::Format(wxT("%i%%"),resp));
		wxMenu->Check(ID_Speed_500, false);
		wxMenu->Check(ID_Speed_400, false);
		wxMenu->Check(ID_Speed_200, false);
		wxMenu->Check(ID_Speed_100, false);
		wxMenu->Check(ID_Speed_50, false);
		wxMenu->Check(ID_Speed_25, false);
		wxMenu->Check(ID_Speed_Custom, true);
		calcs[slot].wxFrame->SetSpeed(resp);
		printf_d("[wxWabbitemu] [OnSetSpeedCustom] Speed set to %i%%. \n", resp);
	} else {
		// note to self: does the "entered something invalid" apply?? supposedly, OnSetSpeedCustom is checked...
		printf_d("[wxWabbitemu] [OnSetSpeedCustom] User canceled dialog or entered something invalid. \n", resp);
		/* Dirty, evil hack... but I'm too lazy to create another var to indicate
		 * custom speed status, soo... :P */
		if (wxMenu->GetLabel(ID_Speed_Custom) == wxString(wxT("Custom..."))) {
			// Do nothing
			printf_d("[wxWabbitemu] [OnSetSpeedCustom] Menu label is 'Custom...', so unchecking the menu. \n", resp);
			wxMenu->Check(ID_Speed_Custom, false);
		} else {
			printf_d("[wxWabbitemu] [OnSetSpeedCustom] Menu label is different, so checking the menu. \n");
			wxMenu->Check(ID_Speed_Custom, true);
		}
	}
}

void MyFrame::OnPauseEmulation(wxCommandEvent &event) {
	wxMenuBar *wxMenu = calcs[this->slot].wxFrame->GetMenuBar();
	if (calcs[this->slot].running) {
		//Tick is checked and emulation stops
		calcs[this->slot].running = FALSE;
		wxMenu->Check(ID_Calc_Pause, true);
	} else {
		//Tick is unchecked and emulation resumes
		calcs[this->slot].running = TRUE;
		wxMenu->Check(ID_Calc_Pause, false);
	}
}

void MyFrame::SetSpeed(int speed) {
	calcs[slot].speed = speed;
	wxMenuBar *wxMenu = calcs[slot].wxFrame->GetMenuBar();
}

void MyFrame::OnKeyDown(wxKeyEvent& event)
{
	int keycode = event.GetKeyCode();
	if (keycode == WXK_F8) {
		if (calcs[slot].speed == 100)
			SetSpeed(400);
		else
			SetSpeed(100);
	}
	if (keycode == WXK_SHIFT) {
		wxUint32 raw = event.GetRawKeyCode();
		if (raw == 65505) {
			keycode = WXK_LSHIFT;
		} else {
			keycode = WXK_RSHIFT;
		}
	}

	keyprog_t *kp = keypad_key_press(&calcs[slot].cpu, keycode);
	if (kp) {
		if ((calcs[slot].cpu.pio.keypad->keys[kp->group][kp->bit] & KEY_STATEDOWN) == 0) {
			calcs[slot].cpu.pio.keypad->keys[kp->group][kp->bit] |= KEY_STATEDOWN;
			this->Update();
			FinalizeButtons();
		}
	}
	return;
}

void MyFrame::OnKeyUp(wxKeyEvent& event)
{
	int key = event.GetKeyCode();
	if (key == WXK_SHIFT) {
		keypad_key_release(&calcs[slot].cpu, WXK_LSHIFT);
		keypad_key_release(&calcs[slot].cpu, WXK_RSHIFT);
	} else {
		keypad_key_release(&calcs[slot].cpu, key);
	}
	FinalizeButtons();
}

void MyFrame::OnFileQuit(wxCommandEvent& WXUNUSED(event))
{
	Close(TRUE);
}

void MyFrame::OnCalcSkin(wxCommandEvent& event)
{
	calcs[slot].SkinEnabled = !calcs[slot].SkinEnabled;
	gui_frame_update(slot);
	this->Refresh();
	this->Update();
}
 
void MyFrame::OnHelpAbout(wxCommandEvent& WXUNUSED(event))
{
	wxMessageBox(wxT("wxWabbitEmu is a port of Wabbitemu that is cross-platform."), wxT("About Wabbitemu"), wxOK | wxICON_INFORMATION, this);
}

void MyFrame::OnHelpWebsite(wxCommandEvent& WXUNUSED(event))
{
	//This function is currently linux only
	system("xdg-open http://code.google.com/p/wxwabbitemu/");
}

void MyFrame::OnQuit(wxCloseEvent& event)
{
	printf_d("[wxWabbitemu] OnQuit called! \n");
	calcs[this->slot].active = FALSE;
	/* Created event in preparation to fix crash bug - this should NOT
	 * affect normal operation. */
	//printf_d("[wxTextEditor] [OnQuit] Killing all timers in current window... \n");
	//wxTimer *thetimer = calcs[this->slot].wxFrame.timer;
	//wxTimer *thetimer = MyApp::timer;
	Destroy();
}

void MyFrame::FinalizeButtons() {
	int group, bit;
	keypad_t *kp = calcs[slot].cpu.pio.keypad;
	for(group=0;group<7;group++) {
		for(bit=0;bit<8;bit++) {
			if ((kp->keys[group][bit] & KEY_STATEDOWN) &&
				((kp->keys[group][bit] & KEY_MOUSEPRESS) == 0) &&
				((kp->keys[group][bit] & KEY_KEYBOARDPRESS) == 0)) {
					kp->keys[group][bit] &= (~KEY_STATEDOWN);
			}
		}
	}
}
