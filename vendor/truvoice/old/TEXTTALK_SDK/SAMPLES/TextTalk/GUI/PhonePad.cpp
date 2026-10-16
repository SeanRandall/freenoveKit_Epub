// PhonePad.cpp : implementation file
//

#include "stdafx.h"
#include "SpeechApp.h"
#include "PhonePad.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPhonePad dialog


//CPhonePad::CPhonePad(CWnd* pParent /*=NULL*/)
//	: CDialog(CPhonePad::IDD, pParent)
//
//{
	//{{AFX_DATA_INIT(CPhonePad)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
//}
//

int g_wmPhonePadMsg = RegisterWindowMessage( "___PHONEPADMESSAGE___" );

CPhonePad::CPhonePad()
{
    m_pParent = NULL;
}

void CPhonePad::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPhonePad)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPhonePad, CDialog)
	//{{AFX_MSG_MAP(CPhonePad)
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPhonePad message handlers

void CPhonePad::ShowPad( CWnd* pParent )
{
    ASSERT_VALID( this );
    ASSERT_VALID( pParent );
    Create( CPhonePad::IDD, pParent );
    CenterWindow( pParent );
    ShowWindow( SW_SHOW );

    m_pParent = pParent;
}

void CPhonePad::OnDestroy() 
{
	CDialog::OnDestroy();	
}

void CPhonePad::OnClose() 
{
    ASSERT_VALID( this );
	ASSERT_VALID( m_pParent );
    m_pParent->PostMessage( g_wmPhonePadMsg, (WPARAM) WM_CLOSE, 0 );
	CDialog::OnDestroy();	
	CDialog::OnClose();
}


BOOL CPhonePad::OnCommand(WPARAM wParam, LPARAM lParam) 
{
    ASSERT_VALID( this );
	ASSERT_VALID( m_pParent );
    LPARAM lSend;
    switch( wParam )
    {
    case IDC_1: lSend = '1'; break;
    case IDC_2: lSend = '2'; break;
    case IDC_3: lSend = '3'; break;
    case IDC_4: lSend = '4'; break;
    case IDC_5: lSend = '5'; break;
    case IDC_6: lSend = '6'; break;
    case IDC_7: lSend = '7'; break;
    case IDC_8: lSend = '8'; break;
    case IDC_9: lSend = '9'; break;
    case IDC_10: lSend = '*'; break;
    case IDC_11: lSend = '0'; break;
    case IDC_12: lSend = '#'; break;
    }
    m_pParent->PostMessage( g_wmPhonePadMsg, (WPARAM) WM_COMMAND, lSend );
    
	return CDialog::OnCommand(wParam, lParam);
}

