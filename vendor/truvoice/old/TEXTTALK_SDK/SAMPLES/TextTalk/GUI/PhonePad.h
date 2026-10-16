// PhonePad.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPhonePad dialog

extern int g_wmPhonePadMsg;

class CPhonePad : public CDialog
{
// Construction
public:
	void ShowPad( CWnd* pParent );
    CPhonePad();
	//CPhonePad(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPhonePad)
	enum { IDD = IDD_PHONEPAD };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPhonePad)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPhonePad)
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CWnd* m_pParent;
};
