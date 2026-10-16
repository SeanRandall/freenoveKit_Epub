// SendDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SpeechApp.h"
#include "SendDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSendDlg dialog


CSendDlg::CSendDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSendDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSendDlg)
	m_sText = _T("");
	//}}AFX_DATA_INIT
}


void CSendDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSendDlg)
	DDX_Text(pDX, IDC_EDIT1, m_sText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSendDlg, CDialog)
	//{{AFX_MSG_MAP(CSendDlg)
	ON_BN_CLICKED(IDC_LOAD, OnLoad)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSendDlg message handlers

void CSendDlg::OnLoad() 
{
	CFileDialog dlg( TRUE, "TXT", NULL, 
        0, "Text Files (*.TXT)|*.TXT|All Files (*.*)|*.* ||" );
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
        else
        {
            AfxMessageBox( IDS_CANTLOADFILE );
        }
    }
}
