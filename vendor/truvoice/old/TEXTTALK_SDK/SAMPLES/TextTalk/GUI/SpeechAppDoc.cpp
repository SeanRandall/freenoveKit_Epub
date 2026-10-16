// SpeechAppDoc.cpp : implementation of the CSpeechAppDoc class
//

#include "stdafx.h"
#include "SpeechApp.h"

#include "SpeechAppDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpeechAppDoc

IMPLEMENT_DYNCREATE(CSpeechAppDoc, CDocument)

BEGIN_MESSAGE_MAP(CSpeechAppDoc, CDocument)
	//{{AFX_MSG_MAP(CSpeechAppDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpeechAppDoc construction/destruction

CSpeechAppDoc::CSpeechAppDoc()
{
	// TODO: add one-time construction code here

}

CSpeechAppDoc::~CSpeechAppDoc()
{
}

BOOL CSpeechAppDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CSpeechAppDoc serialization

void CSpeechAppDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSpeechAppDoc diagnostics

#ifdef _DEBUG
void CSpeechAppDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CSpeechAppDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSpeechAppDoc commands
