// TTSDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SpeechApp.h"
#include "TTSDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTTSDlg dialog


CTTSDlg::CTTSDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTTSDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTTSDlg)
	m_sBlurb = _T("");
	m_sEdit = _T("");
	//}}AFX_DATA_INIT
}


void CTTSDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTTSDlg)
	DDX_Text(pDX, IDC_BLURB, m_sBlurb);
	DDX_Text(pDX, IDC_EDIT1, m_sEdit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTTSDlg, CDialog)
	//{{AFX_MSG_MAP(CTTSDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTTSDlg message handlers

BOOL CTTSDlg::OnInitDialog() 
{
    m_sBlurb.Format( "Current TTS buffer for channel %s", m_sChanName );
	CDialog::OnInitDialog();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
