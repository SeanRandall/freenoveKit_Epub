// smchannel.cpp
// SAPChannel.cpp
#include "stdafx.h"
#include "SMChannel.h"

IMPLEMENT_DYNCREATE( CSMChannel, CChannel );

// obligatory inits
CMap< int, int&, CSMChannel*, CSMChannel* > CSMChannel::m_HandleToObject;



CSMChannel::CSMChannel()
{
    TRACE( "CSMChannel::CSMChannel\n" );
    m_pEventHandler = NULL;
    m_bHandlerInit = FALSE;
    m_hNotifyWnd = NULL;
}

CSMChannel::~CSMChannel()
{
    int i;
    TRACE( "CSMChannel::~CSMChannel\n" );
    m_HandleToObject.RemoveAll(); // can't delete. don't own objects.
    for( i = 0; i < m_TransTable.GetSize(); i++ )
    {
        delete m_TransTable.GetAt( i );
    }
    m_TransTable.RemoveAll();
}


BOOL CSMChannel::OnCreate()
{
    // this does an auto open and auto set hook
    if( !Open() )
    {
        TRACE( "Open failed\n" );
		m_sOpenErrorMsg = m_sChanName + " dx_open failed";
        return FALSE;
    }


#ifdef _DEBUG
    {
        CSMChannel* pCh;
        ASSERT( !m_HandleToObject.Lookup( m_nHandle, pCh ) );
    }
#endif
    m_HandleToObject.SetAt( m_nHandle, this );

    if( 0 != SetHook() )
    {
        TRACE( "Set hook failed\n" );
		m_sOpenErrorMsg = m_sChanName + " could not set on hook";
        return FALSE;
    }

    // enable the handler
    if( !SetEventHandler() )
    {
		m_sOpenErrorMsg = m_sChanName + " could not set event handler";
        return FALSE;
    }

    // always async mode  
    SetAsyncMode();

    InitTransTable();
    return TRUE;
}

void CSMChannel::OnDestroy()
{
    DisableEventHandler();
    CChannel::OnDestroy();
}





BOOL CSMChannel::SetEventHandler()
{
    if( NULL == m_pEventHandler )
    {
        TRACE( "WARNING: no event handler\n" );
        ASSERT( FALSE );
        return TRUE; // let it go anyway
    }
    if( 0 != CChannel::SetEventHandler( m_pEventHandler ) )
    {   
        TRACE( "setting event handler failed\n" );
        return FALSE;
    }
    m_bHandlerInit = TRUE;
    return TRUE;
}

void CSMChannel::DisableEventHandler()
{
    if( m_bHandlerInit && NULL != m_pEventHandler )
    {
        CChannel::DisableEventHandler( m_pEventHandler );
        m_bHandlerInit = FALSE;
    }
}


void CSMChannel::Transition( int nEvent )
{
    // look for event in map given our current state

    int i;

    for( i = 0; i < m_TransTable.GetSize(); i++ )
    {
        CSMTransition* pT = m_TransTable.GetAt( i );
        if( m_nState == pT->m_nCurrentState &&
            nEvent == pT->m_nEvent )
        {
            // fire the handler
            if( NULL != pT->m_pHandler )
            {
                m_nState = (this->*pT->m_pHandler) ();
            }

            // fire to the UI
            if( NULL != m_hNotifyWnd && IsWindow( m_hNotifyWnd ) )
            {
                PostMessage( m_hNotifyWnd, m_nCallbackMsg, 0,
                    (LPARAM) (LPCTSTR) m_sChanName );
            }
            return;
        }
    }

    TRACE( "invalid transition current state = %d event = %d\n discarding",
        m_nState, nEvent );

    //ASSERT( FALSE );
}

void CSMChannel::PutTT( int nState, int nEvent, PTRANSITIONHANDLER pH )
{
    CSMTransition* pT;
    pT = new CSMTransition( nState, nEvent, pH );
    m_TransTable.Add( pT );
}

void CSMChannel::InitTransTable()
{

#ifdef _DEBUG
    for( int i = 0; i < m_TransTable.GetSize(); i++ )
    {
        CSMTransition* pT = m_TransTable.GetAt( i );
        TRACE( "trans %d  state %d event %d\n", i, 
            pT->m_nCurrentState, pT->m_nEvent );
    }
#endif
}



void CSMChannel::SetWindowCallback( const HWND& hWnd, const int nMsg )
{
    if( IsWindow( hWnd ) )
    {
        m_hNotifyWnd = hWnd;
        m_nCallbackMsg = nMsg;
    }
    else
    {
        ASSERT( FALSE );
    }
}
