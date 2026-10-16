// TTSDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTTSDlg dialog

class CTTSDlg : public CDialog
{
// Construction
public:
	CString m_sChanName;
	CTTSDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTTSDlg)
	enum { IDD = IDD_TTSDLG };
	CString	m_sBlurb;
	CString	m_sEdit;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTTSDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTTSDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
