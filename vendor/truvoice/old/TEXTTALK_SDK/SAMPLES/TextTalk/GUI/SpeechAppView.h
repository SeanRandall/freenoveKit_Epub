// SpeechAppView.h : interface of the CSpeechAppView class
//
/////////////////////////////////////////////////////////////////////////////

#define WM_STATECHANGE WM_USER + 100
#include "ListCtrlExt.h"

class CSpeechAppView : public CFormView
{
protected: // create from serialization only
	CSpeechAppView();
	DECLARE_DYNCREATE(CSpeechAppView)

public:
	//{{AFX_DATA(CSpeechAppView)
	enum { IDD = IDD_SPEECHAPP_FORM };
	CButton	m_Load;
	CEdit	m_Text;
	CButton	m_SendText;
	CListCtrlExt m_List;
	CString	m_sText;
	//}}AFX_DATA

// Attributes
public:
	CSpeechAppDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpeechAppView)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo*);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSpeechAppView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif



protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CSpeechAppView)
	afx_msg void OnSendtextr();
	afx_msg void OnLoad();
	afx_msg void OnPhonepad();
	afx_msg void OnUpdatePhonepad(CCmdUI* pCmdUI);
	afx_msg void OnDestroy();
	afx_msg void OnUserdict();
	afx_msg void OnHelpMeRhonda();
	afx_msg void OnUsingme();
	//}}AFX_MSG

    afx_msg LRESULT OnStateChange( WPARAM, LPARAM );
    afx_msg LRESULT OnRowSelChange( WPARAM, LPARAM );
    afx_msg LRESULT OnPhonePadMsg( WPARAM, LPARAM );
	DECLARE_MESSAGE_MAP()
private:
	CString m_sSaveText;
	void LoadInitialText();
	static BOOL m_bFirstSystemCall;
	CDialog* m_pPhonePad;
};

#ifndef _DEBUG  // debug version in SpeechAppView.cpp
inline CSpeechAppDoc* CSpeechAppView::GetDocument()
   { return (CSpeechAppDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
