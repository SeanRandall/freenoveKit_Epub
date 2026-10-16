// BlurbDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SpeechApp.h"
#include "BlurbDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBlurbDlg dialog


CBlurbDlg::CBlurbDlg(CWnd* pParent /*=NULL*/)
{
	//{{AFX_DATA_INIT(CBlurbDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    VERIFY( Create( IDD, pParent ) );
    ShowWindow( SW_SHOW );
}


void CBlurbDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBlurbDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBlurbDlg, CDialog)
	//{{AFX_MSG_MAP(CBlurbDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBlurbDlg message handlers
