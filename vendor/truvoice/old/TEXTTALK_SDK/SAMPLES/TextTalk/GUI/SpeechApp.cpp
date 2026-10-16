////////////////////////////////////////////////////////////////////////////////////
// DIALOGIC Corp.
//

// SpeechApp.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "SpeechApp.h"

#include "MainFrm.h"
#include "SpeechAppDoc.h"
#include "SpeechAppView.h"
#include "SAPChannel.h"
#include "BlurbDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// stuff to prevent multiple instances of this app
static LPCTSTR s_sSemaphore = "MLR_SEMAPHORE";
static HANDLE s_hSemaphore = NULL;
static LPCTSTR s_sAppName = "SpeechApp";
//



/////////////////////////////////////////////////////////////////////////////
// CSpeechAppApp

BEGIN_MESSAGE_MAP(CSpeechAppApp, CWinApp)
	//{{AFX_MSG_MAP(CSpeechAppApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpeechAppApp construction

CSpeechAppApp::CSpeechAppApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSpeechAppApp object

CSpeechAppApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSpeechAppApp initialization

BOOL CSpeechAppApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.


    // see if an instance is already running
    s_hSemaphore = CreateSemaphore( NULL, 0, 1, s_sSemaphore );
    if( ERROR_ALREADY_EXISTS == GetLastError() )
    {
        // if window can be found then set focus to it
        HWND hWnd = FindWindow( NULL, s_sAppName );
        if( NULL != hWnd && IsWindow( hWnd ) )
        {
            HWND hChild = GetLastActivePopup( hWnd );

            if( IsIconic( hWnd ) )
            {
                ShowWindow( hWnd, SW_RESTORE );
            }
            VERIFY( SetForegroundWindow( hChild ) );
        }

        // die now
        return FALSE;
    }

    CBlurbDlg oDlg;

	try
	{

		CChannel::Initialize( RUNTIME_CLASS( CSAPChannel ), m_saChanNames ); 
	}
	catch( CChannelError E )
	{
	    oDlg.EndDialog( IDOK );
		AfxMessageBox( E.m_sMsg );
		return FALSE;
	}


    if( 0 == m_saChanNames.GetSize() )
    {
        ASSERT( FALSE );
        AfxMessageBox( IDS_NOCHANNELS );
        return FALSE;
    }

    oDlg.EndDialog( IDOK );

#ifdef _DEBUG
    // check if all channels are responding
    for( int i = 0; i < m_saChanNames.GetSize(); i++ )
    {
        const CString& sName = m_saChanNames.GetAt( i );
        CSAPChannel* pCh = (CSAPChannel*) CChannel::GetChannelFromName( sName );
        TRACE( "Channel %s\n", sName );
        ASSERT_VALID( pCh );
        ASSERT( pCh->IsOpen() );
        if( 0 != pCh->SetHook( TRUE ) )
        {
            CString sTxt;
            sTxt.Format( "%s is not responding", (LPCTSTR) sName  );
            AfxMessageBox( sTxt );
        }
    }
#endif


#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CSpeechAppDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CSpeechAppView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

            
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CSpeechAppApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CSpeechAppApp commands

int CSpeechAppApp::ExitInstance() 
{
    CChannel::Destroy();


    if( NULL != s_hSemaphore )
    {
        CloseHandle( s_hSemaphore );
    }
	
	return CWinApp::ExitInstance();
}
