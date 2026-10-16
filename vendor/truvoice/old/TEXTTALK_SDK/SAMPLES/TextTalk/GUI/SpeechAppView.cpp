// SpeechAppView.cpp : implementation of the CSpeechAppView class
//

#include "stdafx.h"
#include "SpeechApp.h"

#include "SpeechAppDoc.h"
#include "SpeechAppView.h"
#include "SAPChannel.h"
#include "TTSDlg.h"
#include "PhonePad.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// obligatory inits
BOOL CSpeechAppView::m_bFirstSystemCall = TRUE;

// statics
static const CString s_sSection = "Config";
static const CString s_sParamSection = "Parameters";
static CString s_sInitialTextFileName = "";
static CString s_sHelpFileName = "SpeechAp.hlp";

static CString s_sCentigramEXE = "Centigram English TruVoice Text-to-Speech Converter (Win32)";

#include "gettysburg.h"

/////////////////////////////////////////////////////////////////////////////
// CSpeechAppView

IMPLEMENT_DYNCREATE(CSpeechAppView, CFormView)

BEGIN_MESSAGE_MAP(CSpeechAppView, CFormView)
	//{{AFX_MSG_MAP(CSpeechAppView)
	ON_BN_CLICKED(IDC_SENDTEXTR, OnSendtextr)
	ON_BN_CLICKED(IDC_LOAD, OnLoad)
	ON_COMMAND(IDM_PHONEPAD, OnPhonepad)
	ON_UPDATE_COMMAND_UI(IDM_PHONEPAD, OnUpdatePhonepad)
	ON_WM_DESTROY()
	ON_COMMAND(IDC_USERDICT, OnUserdict)
	ON_BN_CLICKED(IDC_HELP_ME_RHONDA, OnHelpMeRhonda)
	ON_COMMAND(IDC_USINGME, OnUsingme)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CFormView::OnFilePrintPreview)

    ON_MESSAGE( WM_STATECHANGE, OnStateChange )
    //ON_MESSAGE( WM_LVROWSELCHANGE, OnRowSelChange )
    ON_REGISTERED_MESSAGE( g_wmLVRowSelChange, OnRowSelChange )
    ON_REGISTERED_MESSAGE( g_wmPhonePadMsg, OnPhonePadMsg )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpeechAppView construction/destruction
template <class T> 
static inline void DELETE_PTR( T*& pT )
{
    if( NULL != pT )
    {
        delete pT;
        pT = NULL;
    }
}


class CDataItem : public CObject
{
public:
	CString m_sText;
	BOOL m_bFirstCall;
	CDataItem()
	{
		m_bFirstCall = TRUE;
		m_sText.Empty();
	}
	~CDataItem()
	{
	}
};

CSpeechAppView::CSpeechAppView()
	: CFormView(CSpeechAppView::IDD)
{
	//{{AFX_DATA_INIT(CSpeechAppView)
	m_sText = _T("");
	//}}AFX_DATA_INIT
	// TODO: add construction code here

	m_sSaveText = _T("");
    m_pPhonePad = NULL;

}

CSpeechAppView::~CSpeechAppView()
{
    if( NULL != m_pPhonePad )
    {
        ASSERT_VALID( m_pPhonePad );
        m_pPhonePad->DestroyWindow();
    }
    DELETE_PTR( m_pPhonePad );
}

void CSpeechAppView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpeechAppView)
	DDX_Control(pDX, IDC_LOAD, m_Load);
	DDX_Control(pDX, IDC_EDIT1, m_Text);
	DDX_Control(pDX, IDC_SENDTEXTR, m_SendText);
	DDX_Control(pDX, IDC_LIST1, m_List);
	DDX_Text(pDX, IDC_EDIT1, m_sText);
	//}}AFX_DATA_MAP
}

BOOL CSpeechAppView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFormView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CSpeechAppView printing

BOOL CSpeechAppView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CSpeechAppView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CSpeechAppView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
} 

void CSpeechAppView::OnPrint(CDC* pDC, CPrintInfo*)
{
	// TODO: add code to print the controls
}

/////////////////////////////////////////////////////////////////////////////
// CSpeechAppView diagnostics

#ifdef _DEBUG
void CSpeechAppView::AssertValid() const
{
	CFormView::AssertValid();
}

void CSpeechAppView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CSpeechAppDoc* CSpeechAppView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSpeechAppDoc)));
	return (CSpeechAppDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSpeechAppView message handlers

void CSpeechAppView::OnInitialUpdate() 
{
    // fill the list
    CSpeechAppApp* pApp = dynamic_cast <CSpeechAppApp*> ( AfxGetApp() );
    ASSERT_VALID( pApp );

	CSAPChannel::m_sDefaultGreeting.LoadString( IDS_DEFAULTGREETING );
    s_sInitialTextFileName = pApp->GetProfileString( s_sSection, "InitialTextFileName", 
		"" );
    CSAPChannel::m_sGreetingFileName = pApp->GetProfileString( s_sSection, 
		"GreetingFileName" );
    CSAPChannel::m_nSpeedStep = pApp->GetProfileInt( s_sParamSection,
        "SpeedStepSize", CSAPChannel::m_nSpeedStep );
    CSAPChannel::m_nPitchStep = pApp->GetProfileInt( s_sParamSection,
        "PitchStepSize", CSAPChannel::m_nPitchStep );
    CSAPChannel::m_nVolumeStep = pApp->GetProfileInt( s_sParamSection,
        "VolumeStepSize", CSAPChannel::m_nVolumeStep );


	TRACE( "RUN COnfig: \n"
		"initial textfile name = %s\n"
		"Greeting file = %s\n"
		"speed step = %d\n"
		"pitch step = %d\n"
		"volume step = %d\n",
		(LPCTSTR) s_sInitialTextFileName,
		(LPCTSTR) CSAPChannel::m_sGreetingFileName,
		CSAPChannel::m_nSpeedStep,
		CSAPChannel::m_nPitchStep,
		CSAPChannel::m_nVolumeStep ); 


	CFormView::OnInitialUpdate();
    CStringArray sA;
    sA.Add( "Channel    " );
    sA.Add( "Status                   " );
    sA.Add( "Voice Name " );
    sA.Add( "Speech Rate  " );
    sA.Add( "Volume      " );
    sA.Add( "Pitch      " );
    m_List.InitColumns( sA );
    m_List.SetFullRowSel( TRUE );


    for( int i = 0; i < pApp->m_saChanNames.GetSize(); i++ )
    {
        const CString& sName = pApp->m_saChanNames.GetAt( i );
        CSAPChannel* pCh = dynamic_cast < CSAPChannel* > 
            ( CChannel::GetChannelFromName( sName ) );
        ASSERT_VALID( pCh );
        int nItem = m_List.InsertItem( i, sName );
		CDataItem* pItem = new CDataItem;
		m_List.SetItemData( nItem, (DWORD) pItem );

        m_List.SetItemText( nItem, 1, pCh->GetStateAsText() );
        pCh->SetWindowCallback( m_hWnd, WM_STATECHANGE );
    }


	m_List.SetCurSel( 0 );
    OnPhonepad();
}

LRESULT CSpeechAppView::OnStateChange( WPARAM, LPARAM lParam )
{
    ASSERT_VALID( this );
    ASSERT( 0 != lParam );
    CString sTxt = CString( (LPCTSTR) lParam );
    LV_FINDINFO find;
    int nItem;


    find.flags = LVFI_STRING;
    find.psz = sTxt;
    nItem = m_List.FindItem( &find );
    if( -1 == nItem )
    {
        ASSERT( FALSE );
    }
    else
    {
        CSAPChannel* pCh = dynamic_cast < CSAPChannel* > 
            ( CChannel::GetChannelFromName( sTxt ) );
        ASSERT_VALID( pCh );
        m_List.SetItemText( nItem, 1, pCh->GetStateAsText() );
        m_List.SetItemText( nItem, 2, pCh->GetVoice() );
        m_List.SetItemText( nItem, 3, pCh->GetSpeed() );
        m_List.SetItemText( nItem, 4, pCh->GetVolume() );
        m_List.SetItemText( nItem, 5, pCh->GetPitch() );
		if( m_bFirstSystemCall && pCh->IsWaitingForText() )
		{
			m_bFirstSystemCall = FALSE;
			m_List.SetCurSel( nItem );
			nItem = m_List.m_nCurSel;
		}
        if( nItem == m_List.m_nCurSel )
        {
			CDataItem* pItem =  reinterpret_cast<CDataItem*> ( m_List.GetItemData( nItem ) );
            m_SendText.EnableWindow( pCh->IsWaitingForText() );
            //m_Text.EnableWindow( pCh->IsWaitingForText() );
            m_Text.SetReadOnly( !pCh->IsWaitingForText() );			
            m_Load.EnableWindow( pCh->IsWaitingForText() );
			if( pCh->IsWaitingForText() && pItem->m_bFirstCall )
			{
				pItem->m_bFirstCall = FALSE;
				LoadInitialText();
				pItem->m_sText = m_sText;

			}
			if( pCh->IsIdle() )
			{
				m_sText = "";
				UpdateData( FALSE );
			}
        }
    }
    return 0;
}

LRESULT CSpeechAppView::OnRowSelChange( WPARAM wParam, LPARAM lParam )
{
	TRACE( "**** CSpeechAppView::OnRowSelChange\n" );
    CListCtrlExt* pL = (CListCtrlExt*) lParam;
    ASSERT_VALID( pL );
    ASSERT_KINDOF( CListCtrl, pL );
    ASSERT( &m_List == pL );

	if( -1 != pL->m_nOldSel )
	{
		// make sure text of old item is saved
		CDataItem* pItem =  reinterpret_cast<CDataItem*> ( pL->GetItemData( pL->m_nOldSel ) );
		ASSERT_VALID( pItem );
		pItem->m_sText = m_sText;
	}

    if( -1 != pL->m_nCurSel )
    {
        CString sTxt;
		CDataItem* pItem =  reinterpret_cast<CDataItem*> ( pL->GetItemData( pL->m_nCurSel ) );
        sTxt = m_List.GetItemText( m_List.m_nCurSel, 0 );
        CSAPChannel* pCh = dynamic_cast < CSAPChannel* >( CChannel::GetChannelFromName( sTxt ) );
        ASSERT_VALID( pCh );
        m_SendText.EnableWindow( pCh->IsWaitingForText() );
        //m_Text.EnableWindow( pCh->IsWaitingForText() );
        m_Text.SetReadOnly( !pCh->IsWaitingForText() );
        m_Load.EnableWindow( pCh->IsWaitingForText() );


		if( pCh->IsWaitingForText() && pItem->m_bFirstCall )
		{
			pItem->m_bFirstCall = FALSE;
			LoadInitialText();
		}
		else
		{
			ASSERT_VALID( pItem );
			m_sText = pItem->m_sText;
		}
    }
    else
    {
        m_SendText.EnableWindow( FALSE );
        m_Text.SetReadOnly( TRUE );
        //m_Text.EnableWindow( FALSE );
        m_Load.EnableWindow( FALSE );
        m_sText.Empty();
    }
    UpdateData( FALSE );
    return 0;
}



void CSpeechAppView::OnSendtextr() 
{
    if( -1 != m_List.m_nCurSel )
    {
        CString sTxt;
        sTxt = m_List.GetItemText( m_List.m_nCurSel, 0 );
        CSAPChannel* pCh = dynamic_cast < CSAPChannel* > 
            ( CChannel::GetChannelFromName( sTxt ) );
        ASSERT_VALID( pCh );

        // grab stuff from control
        if( pCh->IsWaitingForText() )
        {
            UpdateData();
            pCh->SendText( m_sText );
        }
        else
        {
            MessageBeep( (DWORD) -1 );
        }
    }
}



void CSpeechAppView::OnLoad() 
{	
	CFileDialog dlg( TRUE, NULL, NULL, 
        0, "Text Files (*.TXT)|*.TXT|All Files (*.*)|*.*||" );
    int nRc = dlg.DoModal();
    if( IDOK == nRc )
    {
        CFile oFile;
        if( oFile.Open( dlg.GetPathName(), CFile::modeRead ) )
        {
            // read the whole block
            DWORD dwSize;
            LPTSTR pStr;
            TRY
            {
                dwSize = oFile.GetLength();
            }
            CATCH( CFileException, pE )
            {
                TRACE( "file exception caught error = %d\n", pE->m_cause );
                AfxMessageBox( IDS_CANTLOADFILE );
                ASSERT( FALSE );
                return;
            }
            END_CATCH;

            TRY
            {
                pStr = m_sText.GetBuffer( dwSize + 1 );
            }
            CATCH( CMemoryException, pE )
            {
                TRACE( "Out of memory\n" );
                ASSERT( FALSE );
                AfxMessageBox( IDS_CANTLOADFILE );
                return;
            }
            END_CATCH;

            TRY
            {
                oFile.Read( pStr, dwSize );
            }
            CATCH( CFileException, pE )
            {
                TRACE( "file exception caught error = %d\n", pE->m_cause );
                ASSERT( FALSE );
                AfxMessageBox( IDS_CANTLOADFILE );
                return;
            }
            END_CATCH;

            pStr[ dwSize ] = '\0';
            m_sText.ReleaseBuffer();
            UpdateData( FALSE );
        }
    }	
}


void CSpeechAppView::OnPhonepad() 
{
    ASSERT_VALID( this );
    ASSERT( NULL == m_pPhonePad );
    m_pPhonePad = new CPhonePad;
    ASSERT_VALID( m_pPhonePad );
    (dynamic_cast< CPhonePad* >( m_pPhonePad) )->ShowPad( this );
}

LRESULT CSpeechAppView::OnPhonePadMsg( WPARAM wParam, LPARAM lParam )
{
    if( NULL == m_pPhonePad )
    {
        ASSERT( FALSE );
        return 0;
    }
    ASSERT_VALID( m_pPhonePad );
    if( WM_CLOSE == wParam )
    {
        m_pPhonePad->DestroyWindow();
        DELETE_PTR( m_pPhonePad );
    }
    else
    {
        ASSERT( WM_COMMAND == wParam );
        // the key is in lParam
        if( -1 != m_List.m_nCurSel )
        {
            CString sTxt = m_List.GetItemText( m_List.m_nCurSel, 0 );
            CSAPChannel* pCh = dynamic_cast < CSAPChannel* > 
                ( CChannel::GetChannelFromName( sTxt ) );
            ASSERT_VALID( pCh );
            pCh->StuffKey( (const char) lParam );
        }        
    }
    return 0L;
}

void CSpeechAppView::OnUpdatePhonepad(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( NULL == m_pPhonePad );
	
}

void CSpeechAppView::LoadInitialText()
{
	if( !s_sInitialTextFileName.IsEmpty() )
	{
		CFile oF;
		CFileException oFE;
		DWORD dwSize;
		char* pStr;
		if( oF.Open( s_sInitialTextFileName, CFile::modeRead, &oFE ) )
		{
			// load the whole thing
			TRY
			{
				dwSize = oF.GetLength();
				pStr = m_sText.GetBuffer( dwSize + 1 );
				oF.Read( pStr, dwSize );
				pStr[ dwSize ] = '\0';
				m_sText.ReleaseBuffer();
			}
			CATCH( CFileException, pE )
			{
				TRACE( "file exception caught error = %d\n", pE->m_cause );
				ASSERT( FALSE );
				m_sText = s_sInitialText;
			}
			AND_CATCH( CMemoryException, pE )
			{
				TRACE( "Out of memory\n" );
				ASSERT( FALSE );
				m_sText = s_sInitialText;
			}
			END_CATCH;
		}
		else
		{
			m_sText = s_sInitialText;
		}
	}
	else
	{
		m_sText = s_sInitialText;
	}
	UpdateData( FALSE );
}

void CSpeechAppView::OnDestroy() 
{
	ASSERT_VALID( this );
	ASSERT( IsWindow( m_List.m_hWnd ) );
	for( int i = 0; i < m_List.GetItemCount(); i++ )
	{
		CDataItem* pItem = reinterpret_cast< CDataItem* > ( m_List.GetItemData( i ) );
		ASSERT_VALID( pItem );
		delete pItem;
	}

	VERIFY( ::WinHelp( m_hWnd, NULL, HELP_QUIT, 0 ) );
	CFormView::OnDestroy();	
	
}

void CSpeechAppView::OnUserdict() 
{	
	static const CString sEntry = "UserDictExeFile";
	CWnd* pWnd = FindWindow( "#32770", s_sCentigramEXE );
	if( NULL == pWnd )
	{
		CWinApp* pApp = AfxGetApp();
		BOOL bBrowse = FALSE;
		ASSERT_VALID( pApp );
		CString sCommand = pApp->GetProfileString( s_sSection, sEntry, "" );
		if( "" == sCommand )
		{
			int nRc = AfxMessageBox( IDS_NOTSET, MB_OKCANCEL );
			if( IDCANCEL == nRc )
			{
				return;
			}
			bBrowse = TRUE;
		}
		while( 1 )
		{
			PROCESS_INFORMATION pi;
			STARTUPINFO si;
			if( bBrowse )
			{
				CFileDialog dlg( TRUE, NULL, NULL, 
					0, "Program Files (*.EXE)|*.EXE|All Files (*.*)|*.*||" );
				int nRc = dlg.DoModal();
				if( IDCANCEL == nRc )
				{
					return;
				}
				sCommand = dlg.GetPathName();
			}

			si.cb = sizeof( STARTUPINFO );
			si.lpReserved = NULL;
			si.lpDesktop = NULL;
			si.lpTitle = NULL;
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_SHOW;
			si.cbReserved2 = 0;
			si.lpReserved2 = NULL;
			bBrowse = !CreateProcess( sCommand, NULL, NULL, NULL, FALSE,
				0, NULL, NULL, &si, &pi );
			if( bBrowse )
			{
				int nRc = AfxMessageBox( IDS_CANTRUN, MB_OKCANCEL );
				if( IDCANCEL == nRc )
				{
					return;
				}
			}
			else
			{
				WaitForInputIdle( pi.hProcess, INFINITE );
				pApp->WriteProfileString( s_sSection, sEntry, sCommand );
				break;
			}
		}
	}
	else
	{
		ASSERT_VALID( pWnd );
		pWnd->SetWindowPos( &wndTop, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
	}
}

void CSpeechAppView::OnHelpMeRhonda() 
{
	VERIFY( ::WinHelp( m_hWnd, s_sHelpFileName, HELP_CONTEXT, 5000 ) );
}

void CSpeechAppView::OnUsingme() 
{
	OnHelpMeRhonda();
}
