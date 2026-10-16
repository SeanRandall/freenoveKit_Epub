// ListCtrlExt.h : header file
//

#ifndef __LISTCTRLEXT_H__
#define __LISTCTRLEXT_H__

// Extension to CListCtrl
// Raoul Reyes 
// Portions written Mar-Apr 1996

// August 1996

/////////////////////////////////////////////////////////////////////////////
// CListCtrlExt window
// functionality of this control (as of 9/1/1996):
// 1. can copy one control to another (operator = )
// 2. initiates a drag  (OLE MUST BE INIT ON YOUR WINAPP)
// 3. can be a drop target (OLE MUST BE INIT ON YOUR WINAPP)
// 4. convenient row/col init (DON'T have to muck with WIN32 crap)
// 5. whole row can be highlighted when selected (this behaviour is
//    off by default, you must call SetFullRowSel method)


// message to fire if row sel changes
extern int g_wmLVRowSelChange;

// IF YOUR containing CWnd wants to be informed if
// a selection was changed (via click or keyboard) then
// YOU MUST PUT THIS ON YOUR MESSAGE MAP.
//
//  ON_REGISTERED_MESSAGE( g_wmLVRowSelChange, FnYourHandler )
//
//


typedef enum
{
    CT_ROWNUM = 1,
    CT_TEXT = 2,

} ColType;



typedef struct 
{
    int nDBCol;
    VARTYPE vt;
    ColType ctype;    
    LPCTSTR lpFormat;

} ColDef;


class COleVariantArray;
class CListCtrlDropTarget;
class CRSRRecordset;

class CListCtrlExt;

typedef struct 
{
    CListCtrlExt* m_pDragSource;
} ListCtrlExtDropCargo;

class CListCtrlExt : public CListCtrl
{
// Construction
public:
	int m_nOldSel;
	void SetCurSel( int nItem );
	BOOL m_bDragAnytime;
	int AddRow( CStringArray& sa );
	int m_nCurSel;
	void LoadFromDB( CRSRRecordset* pSet, ColDef* pColDef, WORD wCols, 
        LONG lParamCol = -1, BOOL bClearFirst = TRUE );
	void InitColumns( CStringArray& sa );
	void SetFullRowSel( BOOL bFullSel );
	CListCtrlExt();
    CListCtrlExt( CListCtrl& );
    CListCtrlExt( CListCtrlExt& );

    CListCtrlExt& operator = (CListCtrl& ); // to copy a regular one
    CListCtrlExt& operator = (CListCtrlExt& );


// methods for DROP target
    void InitAsDropTarget();
    virtual DROPEFFECT OnDragEnter( CWnd* pWnd, COleDataObject* pObj, 
                       DWORD dwKeyState, CPoint point );
    virtual DROPEFFECT OnDragOver( CWnd*, COleDataObject*, DWORD dwKeyState, CPoint );
    virtual void OnDragLeave( CWnd* );
    virtual BOOL OnDrop( CWnd*, COleDataObject*, DWORD dwKeyState, CPoint );



// overrides
protected:
	virtual void PostLoadRow( COleVariantArray& aV );
	virtual BOOL PreloadRow( COleVariantArray& av );
    // your derived classes must implement this
    virtual void InitDragCargo( HANDLE& hCargo );

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListCtrlExt)
	//}}AFX_VIRTUAL

private:
	BOOL m_bHackedCurSel;
	int m_cxStateImageOffset;
	int m_cyClient;
	int m_cxClient;
    void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
    LPCTSTR MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset);

// Implementation
public:
	virtual ~CListCtrlExt();

	// Generated message map functions
protected:
	//{{AFX_MSG(CListCtrlExt)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	void CommonConstruct();
	BOOL m_bFullSel;
	COLORREF m_clrText;
	COLORREF m_clrTextBk;
	COLORREF m_clrBkgnd;
    HANDLE m_hCargo;
    CListCtrlDropTarget* m_pDropTarget;
};

#endif


/////////////////////////////////////////////////////////////////////////////
