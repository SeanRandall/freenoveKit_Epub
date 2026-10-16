// ListCtrlExt.cpp : implementation file
//

#include "stdafx.h"
#include "afxole.h"
#include "SpeechApp.h"
#include "ListCtrlExt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// offsets for first and other columns
#define OFFSET_FIRST	2
#define OFFSET_OTHER	6


////////////////////////////////////////////////////////////////////////////////////////
// SetItemData is NOT USED
// user of control can use this area
////////////////////////////////////////////////////////////////////////////////////////




// caveat !!! called at appstart
int g_wmLVRowSelChange = RegisterWindowMessage( "MLR_LISTCTRL_SELCHANGE" );



class CListCtrlDropTarget : public COleDropTarget
{
public:
    // the cargo
    CListCtrlExt* m_pDropControl; // control receiving the drop
public:
    CListCtrlDropTarget( CListCtrlExt& ctrl );
    DROPEFFECT OnDragEnter( CWnd*, COleDataObject*, DWORD dwKeyState, CPoint );
    DROPEFFECT OnDragOver( CWnd*, COleDataObject*, DWORD dwKeyState, CPoint );
    void OnDragLeave( CWnd* );
    BOOL OnDrop( CWnd*, COleDataObject*, DWORD dwKeyState, CPoint );
};

///////////////////////////////////////////////////////////////////////////////////
// drop control

CListCtrlDropTarget::CListCtrlDropTarget( CListCtrlExt& ctrl )
{
    m_pDropControl = &ctrl;
}

// these methods merely allow the listcontrol to process the drag drop messages
DROPEFFECT CListCtrlDropTarget::OnDragEnter( CWnd* pWnd, COleDataObject* pObj, 
                       DWORD dwKeyState, CPoint point )
{
    ASSERT_VALID( m_pDropControl );
    ASSERT_KINDOF( CListCtrl, m_pDropControl );
    return m_pDropControl->OnDragEnter( pWnd, pObj, dwKeyState, point );
}

DROPEFFECT CListCtrlDropTarget::OnDragOver( CWnd* pWnd, COleDataObject* pObj, 
                                           DWORD dwKeyState, CPoint point)
{
    ASSERT_VALID( m_pDropControl );
    ASSERT_KINDOF( CListCtrl, m_pDropControl );
    return m_pDropControl->OnDragOver( pWnd, pObj, dwKeyState, point);
}

void CListCtrlDropTarget::OnDragLeave( CWnd* pWnd )
{
    ASSERT_VALID( m_pDropControl );
    ASSERT_KINDOF( CListCtrl, m_pDropControl );
    m_pDropControl->OnDragLeave( pWnd );
}

BOOL CListCtrlDropTarget::OnDrop( CWnd* pWnd, COleDataObject* pObj, 
                                 DWORD dwKeyState, CPoint point )
{
    ASSERT_VALID( m_pDropControl );
    ASSERT_KINDOF( CListCtrl, m_pDropControl );
    return m_pDropControl->OnDrop( pWnd, pObj, dwKeyState, point );
}


/////////////////////////////////////////////////////////////////////////////
// CListCtrlExt

CListCtrlExt::CListCtrlExt()
{
    CommonConstruct();
}

CListCtrlExt::~CListCtrlExt()
{
    if( NULL != m_pDropTarget )
    {
        m_pDropTarget->Revoke();
        delete m_pDropTarget;
    }
}

CListCtrlExt& CListCtrlExt::operator = (CListCtrl& Copy )
{
    int i;
    ASSERT_VALID( this );
    ASSERT( IsWindow( Copy.m_hWnd ) );
    ASSERT( IsWindow( m_hWnd ) );

    // first zap myself
    if( GetItemCount() > 0 )
    {
        VERIFY( DeleteAllItems() );
    }

    while( DeleteColumn( 0 ) );


    // copy column attributes
    int nCols = 0;
    while( 1 )
    {
        char szBuff[ 80 ];
        LV_COLUMN lvCopy;
        lvCopy.mask = LVCF_FMT|LVCF_TEXT|LVCF_WIDTH;
        lvCopy.pszText = (LPSTR) szBuff;
        lvCopy.cchTextMax = 80;
        lvCopy.iSubItem = 0;
        if( !Copy.GetColumn( nCols, &lvCopy ) )
        {
            break;
        }

        lvCopy.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
        lvCopy.iSubItem = nCols;

        TRACE( "CListCtrlExt:  Inserting text %s\n", lvCopy.pszText );
        if( -1 == InsertColumn( nCols, &lvCopy ) )
        {
            AfxThrowMemoryException();
        }

        nCols++;
    }


    // copy rows
    for( i = 0; i < Copy.GetItemCount(); i++ )
    {
        char szBuff[ 80 ];
        int j;
        int nItemInsert;
        LV_ITEM lv;

        // get the "Item" column 
        lv.mask = LVIF_TEXT|LVIF_PARAM|LVIF_STATE|LVIF_IMAGE;
        lv.iItem = i;
        lv.iSubItem = 0;
        lv.pszText = (LPSTR) szBuff;
        lv.cchTextMax = 80;
        lv.iImage = 0;
        lv.lParam = 0;
        VERIFY( Copy.GetItem( &lv ) );
        nItemInsert = InsertItem( &lv );
        ASSERT( -1 != nItemInsert );
        TRACE( "CListCtrlExt: Inserting row %s lParam = %d\n",
            szBuff, lv.lParam );

        for( j = 1; j < nCols; j++ )
        {
            // just get the text for each column
            CString sTxt = Copy.GetItemText( i, j );
            VERIFY( SetItemText( nItemInsert, j, (LPSTR) (LPCTSTR) sTxt  ) );
        }
    }
    return *this;
}

CListCtrlExt& CListCtrlExt::operator = (CListCtrlExt& Copy )
{
    return operator = ( (CListCtrl&) Copy );
}

BEGIN_MESSAGE_MAP(CListCtrlExt, CListCtrl)
	//{{AFX_MSG_MAP(CListCtrlExt)
	ON_WM_SIZE()
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBegindrag)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemchanged)
	//}}AFX_MSG_MAP

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListCtrlExt message handlers

void CListCtrlExt::SetFullRowSel( BOOL bFullSel )
{
    BOOL bOK = FALSE;
	LockWindowUpdate();

	m_bFullSel =  bFullSel;

	if( m_bFullSel )
    {
		bOK = ModifyStyle( 0L, LVS_OWNERDRAWFIXED );
    }
	else
    {
		bOK = ModifyStyle( LVS_OWNERDRAWFIXED, 0L );
    }

	// repaint window if we are not changing view type
	if( bOK && (GetStyle() & LVS_TYPEMASK)==LVS_REPORT )
    {
		Invalidate();
    }
	// repaint changes
	UnlockWindowUpdate();

}

void CListCtrlExt::CommonConstruct()
{
    m_bFullSel = FALSE;
	m_clrText=::GetSysColor(COLOR_WINDOWTEXT);
	m_clrTextBk=::GetSysColor(COLOR_WINDOW);
	m_clrBkgnd=::GetSysColor(COLOR_WINDOW);
	m_cxClient=0;
	m_cxStateImageOffset=0;
    m_pDropTarget = NULL;
    m_nOldSel = m_nCurSel = -1;
    m_bDragAnytime = FALSE;
	m_bHackedCurSel = FALSE;
}



void CListCtrlExt::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    // this was lifted from the MFC sampes

	CDC* pDC=CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rcItem(lpDrawItemStruct->rcItem);
	UINT uiFlags=ILD_TRANSPARENT;
	CImageList* pImageList;
	int nItem=lpDrawItemStruct->itemID;
	BOOL bFocus=(GetFocus()==this);
	COLORREF clrTextSave, clrBkSave;
	COLORREF clrImage=m_clrBkgnd;
	static _TCHAR szBuff[MAX_PATH];
	LPCTSTR pszText;

// get item data

	LV_ITEM lvi;
	lvi.mask=LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	lvi.iItem=nItem;
	lvi.iSubItem=0;
	lvi.pszText=szBuff;
	lvi.cchTextMax=sizeof(szBuff);
	lvi.stateMask=0xFFFF;		// get all state flags
	GetItem(&lvi);

	BOOL bSelected=(bFocus || (GetStyle() & LVS_SHOWSELALWAYS)) && lvi.state & LVIS_SELECTED;
	bSelected=bSelected || (lvi.state & LVIS_DROPHILITED);

// set colors if item is selected

	CRect rcAllLabels;
	GetItemRect(nItem,rcAllLabels,LVIR_BOUNDS);
	CRect rcLabel;
	GetItemRect(nItem,rcLabel,LVIR_LABEL);
	rcAllLabels.left=rcLabel.left;
	if( rcAllLabels.right<m_cxClient)
		rcAllLabels.right=m_cxClient;

	if(bSelected)
	{
		clrTextSave=pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		clrBkSave=pDC->SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));

		pDC->FillRect(rcAllLabels,&CBrush(::GetSysColor(COLOR_HIGHLIGHT)));
	}
	else
		pDC->FillRect(rcAllLabels,&CBrush(m_clrTextBk));

// set color and mask for the icon

	if(lvi.state & LVIS_CUT)
	{
		clrImage=m_clrBkgnd;
		uiFlags|=ILD_BLEND50;
	}
	else if(bSelected)
	{
		clrImage=::GetSysColor(COLOR_HIGHLIGHT);
		uiFlags|=ILD_BLEND50;
	}

// draw state icon

	UINT nStateImageMask=lvi.state & LVIS_STATEIMAGEMASK;
	if(nStateImageMask)
	{
		int nImage=(nStateImageMask>>12)-1;
		pImageList=GetImageList(LVSIL_STATE);
		if(pImageList)
			pImageList->Draw(pDC,nImage,CPoint(rcItem.left,rcItem.top),ILD_TRANSPARENT);
	}

// draw normal and overlay icon

	CRect rcIcon;
	GetItemRect(nItem,rcIcon,LVIR_ICON);

	pImageList=GetImageList(LVSIL_SMALL);
	if(pImageList)
	{
		UINT nOvlImageMask=lvi.state & LVIS_OVERLAYMASK;
		if(rcItem.left<rcItem.right-1)
			ImageList_DrawEx(pImageList->m_hImageList,lvi.iImage,pDC->m_hDC,rcIcon.left,rcIcon.top,16,16,m_clrBkgnd,clrImage,uiFlags | nOvlImageMask);
	}

// draw item label

	GetItemRect(nItem,rcItem,LVIR_LABEL);
	rcItem.right-=m_cxStateImageOffset;

	pszText=MakeShortString(pDC,szBuff,rcItem.right-rcItem.left,2*OFFSET_FIRST);

	rcLabel=rcItem;
	rcLabel.left+=OFFSET_FIRST;
	rcLabel.right-=OFFSET_FIRST;

	pDC->DrawText(pszText,-1,rcLabel,DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);

// draw labels for extra columns

	LV_COLUMN lvc;
	lvc.mask=LVCF_FMT | LVCF_WIDTH;

	for(int nColumn=1; GetColumn(nColumn,&lvc); nColumn++)
	{
		rcItem.left=rcItem.right;
		rcItem.right+=lvc.cx;

		int nRetLen=GetItemText(nItem,nColumn,szBuff,sizeof(szBuff));
		if(nRetLen==0) continue;

		pszText=MakeShortString(pDC,szBuff,rcItem.right-rcItem.left,2*OFFSET_OTHER);

		UINT nJustify=DT_LEFT;

		if(pszText==szBuff)
		{
			switch(lvc.fmt & LVCFMT_JUSTIFYMASK)
			{
			case LVCFMT_RIGHT:
				nJustify=DT_RIGHT;
				break;
			case LVCFMT_CENTER:
				nJustify=DT_CENTER;
				break;
			default:
				break;
			}
		}

		rcLabel=rcItem;
		rcLabel.left+=OFFSET_OTHER;
		rcLabel.right-=OFFSET_OTHER;

		pDC->DrawText(pszText,-1,rcLabel,nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);
	}

// draw focus rectangle if item has focus

	if(lvi.state & LVIS_FOCUSED && bFocus)
		pDC->DrawFocusRect(rcAllLabels);

// set original colors if item was selected

	if(bSelected)
	{
	        pDC->SetTextColor(clrTextSave);
		pDC->SetBkColor(clrBkSave);
	}
}


void CListCtrlExt::OnSize(UINT nType, int cx, int cy) 
{
	m_cxClient=cx;
	CListCtrl::OnSize(nType, cx, cy);
}


LPCTSTR CListCtrlExt::MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
{
	static const _TCHAR szThreeDots[]=_T("...");

	int nStringLen=lstrlen(lpszLong);

	if(nStringLen==0 || pDC->GetTextExtent(lpszLong,nStringLen).cx+nOffset<=nColumnLen)
		return(lpszLong);

	static _TCHAR szShort[MAX_PATH];

	lstrcpy(szShort,lpszLong);
	int nAddLen=pDC->GetTextExtent(szThreeDots,sizeof(szThreeDots)).cx;

	for(int i=nStringLen-1; i>0; i--)
	{
		szShort[i]=0;
		if(pDC->GetTextExtent(szShort,i).cx+nOffset+nAddLen<=nColumnLen)
			break;
	}

	lstrcat(szShort,szThreeDots);

	return(szShort);
}

void CListCtrlExt::InitColumns( CStringArray& sa )
{
    // first zap myself
    if( GetItemCount() > 0 )
    {
        VERIFY( DeleteAllItems() );
    }

    while( DeleteColumn( 0 ) );

    if( sa.GetSize() == 0 )
    {
        return;
    }

    TEXTMETRIC tm;
    {
        // get a DC and get textmetrics
        CClientDC dc( this );
        dc.GetTextMetrics( &tm );
    }

    for( int i = 0; i < sa.GetSize(); i++ )
    {
        LV_COLUMN lvcolumn;
        CString sLabel = sa[ i ];
        int nWidth;
        lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
        lvcolumn.fmt = LVCFMT_LEFT;
        lvcolumn.pszText = (LPSTR) (LPCTSTR) sLabel;
        lvcolumn.iSubItem = i;
        nWidth = lstrlen( lvcolumn.pszText ) * tm.tmAveCharWidth;
        lvcolumn.cx = nWidth;
        VERIFY( -1 != InsertColumn( i, &lvcolumn ) );
    }
}



void CListCtrlExt::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;

    TRACE( "CListCtrlExt: Drag initiated\n" );
    TRACE( "Calling function to initialize the cargo" );
    InitDragCargo( m_hCargo );
    if( NULL == m_hCargo )
    {
        // don't even bother
        return;
    }
    TRACE( "CListCtrlExt: drag target is started\n" );

    COleDataSource dSource;
    dSource.CacheGlobalData( CF_TEXT, m_hCargo );
    dSource.DoDragDrop( DROPEFFECT_LINK|DROPEFFECT_MOVE|DROPEFFECT_COPY );
}


void CListCtrlExt::InitDragCargo( HANDLE& hCargo )
{
    /////////////////////////////////////////////////////////////////////////////
    // default implementation will just stuff "this" into the drag item
    TRACE( "CListCtrlExt::InitDragCargo\n" );
    hCargo = NULL; // nothing

    if( -1 == m_nCurSel ) // for our protection
    {
        TRACE( "WARNING:  Dragged but no selection\n" );
        ASSERT( FALSE );
        return;
    }

    if( !m_bDragAnytime )
    {
#ifdef _DEBUG
        TRACE( "ClistCtrlExt::InitDragCargo %s Dragged but not enabled\n", THIS_FILE );
#endif
        return;
    }

    hCargo = GlobalAlloc( GHND, sizeof( ListCtrlExtDropCargo ) );
    if( NULL == hCargo )
    {
        AfxThrowMemoryException();
    }
    ListCtrlExtDropCargo* p = (ListCtrlExtDropCargo*) GlobalLock( hCargo );
    p->m_pDragSource = this;
}


// methods for the drop target
void CListCtrlExt::InitAsDropTarget()
{
    // 
    // create a drop target

#ifdef _DEBUG
    if( !IsWindow( m_hWnd ) )
    {
        TRACE( "Please create the window object first\n" );
        ASSERT( FALSE );
        return;
    }
#endif
    
    TRY
    {
        m_pDropTarget = new CListCtrlDropTarget( *this );
    }
    CATCH( CMemoryException, pE )
    {
        THROW_LAST();
    }
    END_CATCH

    VERIFY( m_pDropTarget->Register( this ) );
}


// these will take care of the UI
DROPEFFECT CListCtrlExt::OnDragEnter( CWnd* pWnd, COleDataObject* pObj, 
                  DWORD dwKeyState, CPoint point )
{
    return DROPEFFECT_COPY;
}


DROPEFFECT CListCtrlExt::OnDragOver( CWnd*, COleDataObject*, DWORD dwKeyState, CPoint )
{
    return DROPEFFECT_COPY;
}

void CListCtrlExt::OnDragLeave( CWnd* )
{
    // nothing
}

BOOL CListCtrlExt::OnDrop( CWnd*, COleDataObject*, DWORD dwKeyState, CPoint )
{
    // you must implement this method as this does nothing
    AfxMessageBox( "drop function not implemented" );
    return TRUE;
}



/*
CAVEAT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Raoul NOTES:
there seems to be a problem with message reflection.  If reflection is 
implemented in a control, the parent cannot get the notification anymore.
This is the CWnd::OnNotify in Wincore.cpp

	// reflect notification to child window control
	if (ReflectLastMsg(hWndCtrl, pResult))
		return TRUE;        // eaten by child

	AFX_NOTIFY notify;
	notify.pResult = pResult;
	notify.pNMHDR = pNMHDR;
	return OnCmdMsg(nID, MAKELONG(nCode, WM_NOTIFY), &notify, NULL);

Inside wincore, mfc always checks if a reflect is mapped, if it is mapped then
it does not bother to call the derived class' handler.  DUUUUUUUHHHHHH.

*/


void CListCtrlExt::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TRACE( "**** CListCtrlExt::OnItemChanged  %d\n", m_bHackedCurSel );
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    if( pNMListView->uNewState & LVIS_SELECTED )
    {
#ifdef _DEBUG
		{
			int nState = GetItemState( m_nCurSel, LVIS_SELECTED );
			TRACE( "Gaining state = %d\n", nState );
		}
#endif
	    m_nOldSel = m_nCurSel;
	    m_nCurSel = pNMListView->iItem;	
		if( m_nOldSel != m_nCurSel )
		{
			CWnd* pWnd = GetParent();
			if( NULL != pWnd && IsWindow( pWnd->m_hWnd ) && !m_bHackedCurSel )
			{
				//pWnd->PostMessage( WM_LVROWSELCHANGE, 0, (LPARAM) this );
				pWnd->PostMessage( g_wmLVRowSelChange, 0, (LPARAM) this );
			}
			TRACE( "*** Setting hack to false\n" );
		}
		m_bHackedCurSel = FALSE;
    }
    else
    {
        //m_nCurSel = -1;
#ifdef _DEBUG
		{
			int nState = GetItemState( m_nCurSel, LVIS_SELECTED );
			TRACE( "Losing nState = %d\n", nState );
		}
#endif
    }
	*pResult = 0;
}

int CListCtrlExt::AddRow( CStringArray& sa )
{
    int nRet = -1;
    for( int i = 0; i < sa.GetSize(); i++ )
    {
        LV_ITEM lvitem;
        CString sTxt = sa.GetAt( i );
        if( i == 0 )
        {
            lvitem.iItem = GetItemCount();
            lvitem.state = 0;
            lvitem.iSubItem = 0;
            lvitem.stateMask = 0;
            lvitem.mask = LVIF_TEXT;
            lvitem.pszText = (LPSTR) (LPCTSTR) sTxt;
            lvitem.iItem = InsertItem( &lvitem );
            nRet = lvitem.iItem;
            if( -1 == lvitem.iItem )
            {
                ASSERT( FALSE );
                AfxThrowMemoryException();
            }
        }
        else
        {
            lvitem.mask = LVIF_TEXT;
            lvitem.pszText = (LPSTR) (LPCTSTR) sTxt;
            lvitem.iSubItem = i;
            SetItem( &lvitem );
        }
    }
    return nRet;
}

BOOL CListCtrlExt::PreloadRow( COleVariantArray& av )
{
    // default implementation always loads
    return TRUE;
}

void CListCtrlExt::PostLoadRow( COleVariantArray& aV )
{
    // 
    TRACE( "*** PostLoadRow of list control base class does nothing\n" );
}

void CListCtrlExt::SetCurSel( int nItem )
{
	ASSERT( 0 <= nItem && nItem < GetItemCount() );

	TRACE( "CListCtrlExt::SetCurSel\n" );
	m_bHackedCurSel = TRUE; // make it behave like a CListBox::SetCurSel

	// remove the current selection
	for( int i = 0; i < GetItemCount(); i++ )
	{
		TRACE( "Resetting i = %d\n", i );
		VERIFY( SetItemState( nItem, 0, 0 ) );
	}


	TRACE( "Setting to select %d\n", nItem );

	if( SetItemState( nItem, LVIS_SELECTED, 2 ) )
	{
		m_nCurSel = nItem;
	}
	else
	{
		ASSERT( FALSE );
	}

#ifdef _DEBUG
	{
		int nState = GetItemState( nItem, LVIS_SELECTED );
		TRACE( "nState = %d\n", nState );
	}
#endif

	TRACE( "Calling ensurevisible\n" );
	VERIFY( EnsureVisible( nItem, FALSE ) );
	TRACE( "Calling redraw\n" );
	VERIFY( RedrawItems( 0, GetItemCount() ) );
}

/*

  rreyes: was remove because depended on some DAO classes which were my own!
void CListCtrlExt::LoadFromDB( CRSRRecordset* pSet, ColDef* pColDef, WORD wCols,
                              LONG lParamCol, BOOL bClearFirst )
{
    ASSERT_VALID( this );
    BOOL bFirst = TRUE;
    if( bClearFirst )
    {
        DeleteAllItems();
    }

    ASSERT_VALID( pSet );
    // load the list
    while( !pSet->IsEOF() )
    {
        LV_ITEM lvitem;
        COleVariant v;
        COleVariantArray av;
        CString sTxt;
        
        pSet->GetRecord( &av );
        
        if( PreloadRow( av ) )
        {
            for( int i = 0; i < (int) wCols; i++ )
            {
                switch( pColDef[ i ].ctype )
                {
                case CT_ROWNUM:
                    sTxt.Format( "%d", GetItemCount() + 1 );
                    break;
                case CT_TEXT:
                    {
                        if( -1 == pColDef[ i ].nDBCol )
                        {
                            sTxt = "";
                        }
                        else
                        {
                            v = av.GetAt( pColDef[ i ].nDBCol );
                            if( v.vt != VT_NULL && v.vt != pColDef[ i ].vt )
                            {
                                TRACE( "Different type returned as expected\n"
                                    "DB Type = %d\nColType = %d\n",
                                    v.vt, pColDef[ i ].vt );
                                ASSERT( FALSE );
                                AfxMessageBox( IDS_TYPEMISMATCH );
                            }
                            switch( pColDef[ i ].vt )
                            {
                            case VT_BSTR:
                                {
                                    sTxt = (LPCSTR) v.pbVal;
                                    break;
                                }
                            case VT_I4:
                                {
                                    ASSERT( NULL != pColDef[ i ].lpFormat );
                                    sTxt.Format( pColDef[ i ].lpFormat, v.lVal );
                                    break;
                                }
                            case VT_I2:
                                {
                                    ASSERT( NULL != pColDef[ i ].lpFormat );
                                    sTxt.Format( pColDef[ i ].lpFormat, v.iVal );
                                    break;
                                }
                            case VT_CY:
                                v.ChangeType( VT_R8 );
                            case VT_R8:
                                {
                                    ASSERT( NULL != pColDef[ i ].lpFormat );
                                    sTxt.Format( pColDef[ i ].lpFormat, v.dblVal );
                                    break;
                                }
                            case VT_DATE:
                                {
                                    COleDateTime dt = v;
                                    sTxt.Format( "%02m/%02d/%04d",
                                        dt.GetMonth(), dt.GetYear(), dt.GetYear() );
                                    break;
                                }
                            default:
                                ASSERT( FALSE );
                                break;
                            }
                        }
                        break;
                    }
                default: ASSERT( FALSE ); break;
                }
                // special handling for fol 0
                if( i == 0 )
                {
                    int nMask = LVIF_TEXT;
                    lvitem.iItem = GetItemCount();
                    lvitem.state = 0;
                    lvitem.iSubItem = 0;
                    lvitem.stateMask = 0;
                    if( -1 != lParamCol )
                    {
                        v = av.GetAt( lParamCol );
                        ASSERT( v.vt == VT_I4 );
                        nMask |= LVIF_PARAM;
                        lvitem.lParam = v.lVal;
                    }
    #ifdef _DEBUG
                    else
                    {
                        TRACE( "WARNING: Loading List Ctrl with no item data\n" );
                    }
    #endif
                    lvitem.mask = nMask;
                    lvitem.pszText = (LPSTR) (LPCTSTR) sTxt;
                    lvitem.iItem = InsertItem( &lvitem );
                    if( -1 == lvitem.iItem )
                    {
                        ASSERT( FALSE );
                        throw CJerryException( IDS_TOOMUCHDATA );
                    }

                    // NOTE: the first item is always selected
                    if( bFirst )
                    {
                        SetItemState( lvitem.iItem, TRUE, LVIS_SELECTED );
                    }
                }
                else
                {
                    lvitem.mask = LVIF_TEXT;
                    lvitem.pszText = (LPSTR) (LPCTSTR) sTxt;
                    lvitem.iSubItem = i;
                    SetItem( &lvitem );
                }
            }
            PostLoadRow( av );
        }
        pSet->MoveNext();
    }
}
*/

