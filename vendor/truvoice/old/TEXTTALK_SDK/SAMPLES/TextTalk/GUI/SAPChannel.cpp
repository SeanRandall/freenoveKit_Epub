// SAPChannel.cpp
#include "stdafx.h"
#include "SAPChannel.h"
#include "texttalk.h"
#include "smartptr.h"


IMPLEMENT_DYNCREATE( CSAPChannel, CSMChannel );

// obligatory inits
BOOL CSAPChannel::m_bTTSInit = FALSE;
CString CSAPChannel::m_sDefaultGreeting;
CString CSAPChannel::m_sGreetingFileName = "";
int CSAPChannel::m_nSpeedStep = 10;
int CSAPChannel::m_nVolumeStep = 1;
int CSAPChannel::m_nPitchStep = 10;



// globals to this file
static BOOL s_bInit = FALSE;
static const char s_cCapsMark = '`';
static const CString s_sSpellOn = 
    CString( 0x1b ) + CString( "[1;2;3;4N" );
static const CString s_sSpellOff = 
    CString( 0x1b ) + CString( "[1;2;3;4F" );
static const CString s_sCapsOn = 
    CString( 0x1b ) + CString( "[13N" );
static const CString s_sCapsOff = 
    CString( 0x1b ) + CString( "[13F" );
    
CSAPChannel::CSAPChannel()
{
    TRACE( "CSAPChannel::CSAPChannel\n" );
    m_pEventHandler = EventHandler;
    m_nState = ST_IDLE;
    m_nTTSHandle = -1;
    m_nVoice = 0;
}

CSAPChannel::~CSAPChannel()
{
    ASSERT_VALID( this );
    TRACE( "CSAPChannel::~CSAPChannel\n" );
}


BOOL CSAPChannel::OnCreate()
{
    ASSERT_VALID( this );
    if( !CSMChannel::OnCreate() )
    {
        return FALSE;
    }


    SetEvtMask( DM_RINGS|DM_LCOFF|DM_DIGITS );

    if( 0 != SetRingCountNotify( 2 ) )
    {
        TRACE( "Ring count notify failed\n" );
    }

    if( !m_bTTSInit )
    {
        m_bTTSInit = TRUE;
        tts_init();
    }





    if( !s_bInit )
    {
        // get stuff from the INI file

        // make sure files can be opened
#ifdef _DEBUG
		if( !m_sGreetingFileName.IsEmpty() )
		{
			OFSTRUCT of;
			if( HFILE_ERROR == OpenFile( m_sGreetingFileName, &of, OF_EXIST ) )
			{
				TRACE( "WARNING: Cannot open greeting file\n" );
				//return FALSE;
			}
		}
#endif
        s_bInit = TRUE;
    }
    return TRUE;
}

void CSAPChannel::OnDestroy()
{
    ASSERT_VALID( this );
    if( 0 < m_nTTSHandle )
    {
        if( -1 != m_nTTSHandle )
        {
            int nRc = tts_close( m_nTTSHandle );
#ifdef _DEBUG
            if( 0 != nRc )
            {
                TRACE( "tts_close failed error = %s\n", 
                    tts_geterror( nRc ) );
                ASSERT( FALSE );
            }
#endif
        }
    }
    CSMChannel::OnDestroy();
}

long CSAPChannel::EventHandler( DWORD dwChannel )
{
    int nHandle = sr_getevtdev( 0 );
    long lEvent = sr_getevttype( 0 );
    DX_CST* cstp;
    TRACE( "Received event %d for handle %d\n", lEvent, nHandle );

    CSAPChannel* pCh;
    CSMChannel* pTemp;
    VERIFY( m_HandleToObject.Lookup( nHandle, pTemp ) );
    pCh = dynamic_cast< CSAPChannel* > (pTemp);
    if( NULL == pCh )
    {
        ASSERT( FALSE );
        return 1; // let the api figure it out
    }
    ASSERT_VALID( pCh );

    switch( lEvent )
    {
    case TDX_GETDIG:
        {
            int nMsk = ATDX_TERMMSK( pCh->m_nHandle );
            SAPEvent ev;
            TRACE( "Get Digit terminated code = %d\n", nMsk );
                
            switch( nMsk )
            {
            case TM_LCOFF:
                // we do nothing because this will be trapped by TDX_LCOFF
                TRACE( "GetDigit terminated by LCOFF\n" );
                break;
            case TM_MAXDTMF:
                TRACE( "GetDigit terminated with MAXDTMF\n" );
                ev = SEV_KEYS_ENTERED;
                break;
            case TM_IDDTIME:
                TRACE( "GetDigit terminated by IDDTIME digits\n" );
                ev = SEV_KEY_TIMEOUT;
                break;
            case TM_MAXTIME:
                TRACE( "GetDigit terminated by MAXTIME digits \n");
                ev = SEV_KEY_TIMEOUT;
                break;
            case TM_DIGIT:
                TRACE( "GetDigit terminated by DIGIT digits\n " );
                ev = SEV_KEYS_ENTERED;
                break;
            default:
                // let api figure it out
                return 1;
            }
            pCh->Transition( ev );
        }
        break;
    case TTS_EV_FILE_END:
		TRACE( "*****  EVENT IS PLAYDONE ******\n" );
        pCh->Transition( SEV_PLAYDONE );
        break;
    case TDX_SETHOOK:
        TRACE( "Sethook received\n" );
        cstp = (DX_CST*) sr_getevtdatap(0);
        if( DX_ONHOOK == cstp->cst_event )
        {
            // our side was dropped
            pCh->Transition( SEV_DISCONNECTED );

        }
        else
        {
            pCh->Transition( SEV_OFFHOOK );
        }
        break;
    case TDX_CST:
        TRACE( "CST received\n" );
        cstp = (DX_CST*) sr_getevtdatap(0);


        switch( cstp->cst_event )
        {
        case DE_LCOFF:
            // line went off
            // no matter what state we are in
            // drop the call
            TRACE( "Loop current off\n" );
            pCh->Transition( SEV_DISCONNECTED );
            break;
        case DE_RINGS:
            TRACE( "Phone is ringing\n" );
            pCh->Transition( SEV_RINGING );
            break;
        case DE_RNGOFF: // caller hangup
            TRACE( "Caller hangup before we answered\n" );
            break;
        case DE_DIGITS:
            {
                WORD wD = LOWORD( cstp->cst_data );
                char c = (char) LOBYTE( wD );
                TRACE( "digit in buffer %c\n", c );
                pCh->m_cDigit = c;
                pCh->Transition( SEV_KEY_PUNCHED );
            }
            break;
        }
        break;
    }

    return 0; // we ate the event.
}

int CSAPChannel::PlayString()
{
    int nRc;

    int nFind = m_sPlayBuffer.Find( s_cCapsMark ); // we always take a performance hit here!


	// check if homegrown pre-processing is needed
    if( -1 != nFind )
    {
        // FIXME:
        // FIXME:
        // FIXME:
        // FIXME:
        // this is a hack!!!!
        // this is a hack!!!!
        // this is a hack!!!!
        // this is a hack!!!!
        // this is a hack!!!!
        // FIXME:
        // FIXME:
        // FIXME:
        // FIXME:
        TRACE( "Opening the can of worms\n" );

        // this is a real hack

        // make a copy
        CString sText;
        CString sRight = m_sPlayBuffer; // No pain as yet because the MFC
										// will just bump up the reference count
        BOOL bOn = TRUE;

        sText = s_sCapsOff;  // doug wants it off!


        // FIXME:
        // FIXME:
        // FIXME:
        // FIXME:
        // THIS IS TERRIBLY SLOW ON LONG STRINGS!!!!!!
        // FIXME:
        // FIXME:
        // FIXME:
        // FIXME:

        while( -1 != nFind )
        {
            sText += sRight.Left( nFind );    // OUCH! Really hurts!
            if( bOn )
            {
                sText += s_sSpellOn; // ouch
                bOn = FALSE;
            }
            else
            {
                sText += s_sSpellOff; // ouch
                bOn = TRUE;
            }
            sRight = sRight.Mid( nFind + 1 ); // OUCH! Really hurts!
            nFind = sRight.Find( s_cCapsMark );
        }
        sText += sRight;  // Ouch again!

        // and keep it off!
        if( !bOn )
        {
            sText += s_sSpellOff;
        }
        sText += s_sCapsOn; // on by default

        nRc = tts_speakemail( m_nTTSHandle, 
            (LPTSTR) (LPCTSTR) sText, EV_ASYNC, NULL );

    }
    else
    {
        nRc = tts_speakemail( m_nTTSHandle, 
            (LPTSTR) (LPCTSTR) m_sPlayBuffer, EV_ASYNC, NULL );
    }

    if( 0 > nRc )
    {
#ifdef _OLDVERSION_
        TRACE( "tts_speakfile error %s\n", 
            tts_geterror( nRc ) );
#else
        TRACE( "tts_speakfile error %s\n", 
            tts_geterror( nRc ) );
#endif
        ASSERT( FALSE );
    }
    return nRc;
}

int CSAPChannel::PlayFile( LPCTSTR lpText, LPCTSTR lpDefault )
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    ASSERT( -1 != m_nTTSHandle );
    char* pStr;
    DWORD dwSize;
    CFile oFile;
    CFileException fE;



	if( NULL == lpText || 0 == *lpText || !oFile.Open( lpText, CFile::modeRead, &fE ) )
    {
        TRACE( "WARNING: Cannot open file error = %d\n", fE.m_cause );
		if( NULL != lpDefault )
		{
			m_sPlayBuffer = lpDefault;
			return PlayString();
		}
		else
		{
			return -1;
		}
    }

    // load the whole thing
    TRY
    {
        dwSize = oFile.GetLength();
    }
    CATCH( CFileException, pE )
    {
        TRACE( "file exception caught error = %d\n", pE->m_cause );
        ASSERT( FALSE );
        return -1;
    }
    END_CATCH;

    TRY
    {
        pStr = m_sPlayBuffer.GetBuffer( dwSize + 1 );
    }
    CATCH( CMemoryException, pE )
    {
        TRACE( "Out of memory\n" );
        ASSERT( FALSE );
        return -1;
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
        return -1;
    }
    END_CATCH;

    pStr[ dwSize ] = '\0';
    m_sPlayBuffer.ReleaseBuffer();

    return PlayString();

}

void CSAPChannel::InitTransTable()
{
    // build transition table
    ASSERT_VALID( this );
    PutTT( ST_IDLE, SEV_RINGING, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnPhoneRing );
    PutTT( ST_IDLE, SEV_DISCONNECTED, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnDisconnecting );

    PutTT( ST_ANSWERING, SEV_OFFHOOK, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnOffHook );
    PutTT( ST_ANSWERING, SEV_OFFHOOK, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnDisconnecting );

    PutTT( ST_GREETING, SEV_PLAYDONE, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnGreetingDone );
	// rewire to prevent crash
    //PutTT( ST_GREETING, SEV_DISCONNECTED, 
        //(PTRANSITIONHANDLER) &CSAPChannel::OnDisconnecting );
    PutTT( ST_GREETING, SEV_DISCONNECTED, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnStoppingPlay );

    PutTT( ST_WAITINGFORTEXT, SEV_DISCONNECTED, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnDisconnecting );
    PutTT( ST_WAITINGFORTEXT, SEV_KEY_PUNCHED, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnKeyWhileWaiting );


    PutTT( ST_DISCONNECTING, SEV_DISCONNECTED, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnDisconnected );

    PutTT( ST_PLAYINGTEST, SEV_PLAYDONE, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnTestPlayDone );
    PutTT( ST_PLAYINGTEST, SEV_KEY_PUNCHED, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnKeyWhileTest );
    PutTT( ST_PLAYINGTEST, SEV_DISCONNECTED, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnStoppingPlay );


    PutTT( ST_PAUSED, SEV_KEY_PUNCHED, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnKeyWhileTest );
    PutTT( ST_PAUSED, SEV_DISCONNECTED, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnDisconnecting );
    

	// this waits for a stop play before dropping
    PutTT( ST_WAITINGFORSTOP, SEV_PLAYDONE, 
        (PTRANSITIONHANDLER) &CSAPChannel::OnDisconnecting );


    CSMChannel::InitTransTable();


}


int CSAPChannel::OnPhoneRing()
{
    int nRc;
    ASSERT_VALID( this );
    // answer by setting phone offhook

    // try to get a tts handle
    ASSERT( -1 == m_nTTSHandle );
    m_nTTSHandle = tts_open( m_nHandle );
    if( 0 > m_nTTSHandle )
    {
        MessageBox( NULL, tts_geterror( m_nTTSHandle ),
            NULL, MB_OK );
        TRACE( "tts_open failed error = %s\n", 
            tts_geterror( m_nTTSHandle ) );
		m_nTTSHandle = -1;
        //ASSERT( FALSE );
        return OnDisconnected();
    }

    m_nVoice = 0;
    nRc = tts_getvoice( m_nTTSHandle, &m_nVoice );
    if( 0 > nRc )
    {
        TRACE( "tts_getvoice failed error = %s\n", 
            tts_geterror( nRc ) );
        ASSERT( FALSE );
        return FALSE;
    }

    if( 0 == SetHook( FALSE ) )
    {
        return ST_ANSWERING;
    }
    else
    {
        // drop call and remain idle
        return OnDisconnected();
    }
}

int CSAPChannel::OnStoppingPlay()
{
	TRACE( "Entering OnStoppingPlay\n" );
	int nRc = -1;
    ASSERT_VALID( this );
	ASSERT( -1 != m_nTTSHandle ); // we were playing
    if( -1 != m_nTTSHandle )
    {
        nRc = tts_stop( m_nTTSHandle );
		TRACE( "STOP WAS SENT\n" );
#ifdef _DEBUG
        if( 0 != nRc )
        {
            TRACE( "tts_close failed error = %s\n", 
                tts_geterror( nRc ) );
            ASSERT( FALSE );
        }
#endif
    }
	else
	{
		ASSERT( FALSE ); // again for emphasis!
	}

	if( 0 != nRc )
	{
		ASSERT( FALSE );
		// force a disconnect and hope the srl does not choke up!
		return OnDisconnecting();

	}


    return ST_WAITINGFORSTOP;
}


int CSAPChannel::OnDisconnecting()
{
    ASSERT_VALID( this );
    if( -1 != m_nTTSHandle )
    {
		int nRc;
		/////
        ///nRc = tts_stop( m_nTTSHandle );
		////
        nRc = tts_close( m_nTTSHandle );
#ifdef _DEBUG
        if( 0 != nRc )
        {
            TRACE( "tts_close failed error = %s\n", 
                tts_geterror( nRc ) );
            ASSERT( FALSE );
        }
#endif
    }
    m_nTTSHandle = -1;

	TRACE( "Setting phone on hook\n" );

	SetHook();

	TRACE( "Exiting disconnecting handler\n" );

    return ST_DISCONNECTING;
}



int CSAPChannel::OnDisconnected()
{
	TRACE( "Entering OnDisconnected\n" );
    ASSERT_VALID( this );

	m_sPlayBuffer = "";
    return ST_IDLE;
}

int CSAPChannel::OnGreetingDone()
{
    ASSERT_VALID( this );
    m_sPlayBuffer.Empty();
    return ST_WAITINGFORTEXT;

}

int CSAPChannel::OnTestPlayDone()
{
    ASSERT_VALID( this );
    return ST_WAITINGFORTEXT;
}

int CSAPChannel::OnOffHook()
{
    ASSERT_VALID( this );


    // play greeting
    if( 0 == PlayFile( m_sGreetingFileName, m_sDefaultGreeting ) )
    {
        return ST_GREETING;
    }
    else
    {
        SetHook();
        return ST_DISCONNECTING;
    }
}

int CSAPChannel::OnMenuOptionEntered()
{
    char szOpt[ 20 ];
    RetrieveDigits( szOpt, NULL );
    if( '1' == *szOpt )
    {
        SetHook();
        return ST_DISCONNECTING;
    }
    else
    {
        return OnOffHook();
    }
}

int CSAPChannel::OnKeyWhileWaiting()
{
	int nRc;

	switch( m_cDigit )
	{
	case '1':
		m_nVoice++;
		if( m_nVoice > 9 )
		{
			m_nVoice = 0;
		}
		nRc = tts_voice( m_nTTSHandle, m_nVoice );
		if( 0 > nRc )
		{
			TRACE( "tts_voice failed error = %s\n", 
				tts_geterror( nRc ) );
			ASSERT( FALSE );
		}
		break;
    case '2':
    case '3':
		AdjustSpeed( '3' == m_cDigit );
        break;
    case '5':
    case '6':
		AdjustVolume( '6' == m_cDigit );
        break;
    case '8':
    case '9':
		AdjustPitch( '9' == m_cDigit );
        break;
	}
	// fire to the UI
	if( NULL != m_hNotifyWnd && IsWindow( m_hNotifyWnd ) )
	{
		PostMessage( m_hNotifyWnd, m_nCallbackMsg, 0,
			(LPARAM) (LPCTSTR) m_sChanName );
	}

    return m_nState;
}

int CSAPChannel::OnKeyWhileTest()
{
    int nAttr;
    int nRc = 0;
    TRACE( "Digit on buffer = %c\n", m_cDigit );

    // block all digits if paused
    if( ST_PAUSED == m_nState )
    {

        if( '7' != m_cDigit && '*' != m_cDigit )
        {
            return ST_PAUSED;
        }
    }

    switch( m_cDigit )
    {
    case '2':
    case '3':
		AdjustSpeed( '3' == m_cDigit );
        break;
    case '5':
    case '6':
		AdjustVolume( '6' == m_cDigit );
        break;

    case '8':
    case '9':
		AdjustPitch( '9' == m_cDigit );
        break;
    case '7':

        if( ST_PAUSED == m_nState )
        {
            nRc = tts_resume( m_nTTSHandle );
            if( 0 > nRc )
            {
                TRACE( "tts_resume failed error = %s\n", 
                    tts_geterror( nRc ) );
                ASSERT( FALSE );
                return ST_PAUSED;
            }
            // drop down, to ST_PLAYINGTEST
        }
        else
        {
            nRc = tts_pause( m_nTTSHandle );
            if( 0 > nRc )
            {
                TRACE( "tts_pause failed error = %s\n", 
                    tts_geterror( nRc ) );
                ASSERT( FALSE );
                // drop down to ST_PLAYINGTEST
            }
            else
            {
                return ST_PAUSED;
            }
        }
        break;
        
    case '0':
    case '#':
        nAttr = '#' == m_cDigit ? 10 : -10;
        TRACE( "jumping %d words\n", nAttr );

        nRc = tts_jump( m_nTTSHandle, TTS_JUMP_WORD, nAttr );
        //nRc = tts_jump( m_nTTSHandle, TTS_JUMP_CHARACTER, nAttr );
        if( 0 > nRc )
        {
            TRACE( "tts_jump failed error = %s\n", 
                tts_geterror( nRc ) );
            ASSERT( FALSE );
        }
        break;
    case '*':
		{
			int nRc = tts_stop( m_nTTSHandle );
			if( 0 == nRc )
			{
				TRACE( "QUITTING\n" );
				return ST_WAITINGFORTEXT;
			}
			else
			{
				TRACE( "tts_stop failed error %s\n", tts_geterror( nRc ) );
			}
		}
    }

    return ST_PLAYINGTEST;
}



CString CSAPChannel::GetStateAsText()
{
    switch( m_nState )
    {
	case ST_WAITINGFORSTOP:
		return CString( "Stopping" );
    case ST_ANSWERING:
        return CString( "Answering call" );
    case ST_GREETING:
        return CString( "Playing greeting" );
    case ST_DISCONNECTING:
        return CString( "Disconnecting" );
    case ST_GETTINGMENUOPTION:
        return CString( "Getting menu option" );
    case ST_PLAYINGTEST:
        return CString( "Playing 'Text To Send'" );
    case ST_PAUSED:
        return CString( "PAUSED while Playing Text File" );
    case ST_WAITINGFORTEXT:
        return CString( "Waiting for text" );
    default:
        return CString( "IDLE/Waiting For Call" );
    }

}

BOOL CSAPChannel::IsPlayingText()
{
    return ( ST_PLAYINGTEST == m_nState || ST_GREETING == m_nState ||
        ST_PAUSED == m_nState );
}

CString CSAPChannel::GetPlayText()
{
    CString sRc;
    sRc.Empty();
    sRc = m_sPlayBuffer;
    return sRc;
}

BOOL CSAPChannel::IsWaitingForText()
{
    return ( ST_WAITINGFORTEXT == m_nState );
}

void CSAPChannel::SendText( CString sText )
{
    if( !IsWaitingForText() )
    {
        ASSERT( FALSE );
        return;
    }
    m_sPlayBuffer = sText;
    if( 0 == PlayString() )
    {
        m_nState = ST_PLAYINGTEST;
    }
    else
    {
        SetHook();
        m_nState =  ST_DISCONNECTING;
    }
    // fire to the UI
    if( NULL != m_hNotifyWnd && IsWindow( m_hNotifyWnd ) )
    {
        PostMessage( m_hNotifyWnd, m_nCallbackMsg, 0,
            (LPARAM) (LPCTSTR) m_sChanName );
    }
}


CString CSAPChannel::GetVoice()
{
    ASSERT_VALID( this );
    CString sTxt = "";
    if( -1 != m_nTTSHandle )
    {

        // CAVEAT: hardcoded names

        switch( m_nVoice )
        {
        case 0: sTxt = "PETER"; break;
        case 1: sTxt = "SIDNEY"; break;
        case 2: sTxt = "EDDIE"; break;
        case 3: sTxt = "DOUGLAS"; break;
        case 4: sTxt = "BIFF"; break;
        case 5: sTxt = "AMOS"; break;
        case 6: sTxt = "MELVIN"; break;
        case 7: sTxt = "ALEX"; break;
        case 8: sTxt = "WANDA"; break;
        case 9: sTxt = "JULIA"; break;
        default: break;
        }
    }
    return sTxt;
}

CString CSAPChannel::GetVolume()
{
    ASSERT_VALID( this );
    CString sTxt = "";
    if( -1 != m_nTTSHandle )
    {
        int nAttr = 0;
        int nRc = tts_getvolume( m_nTTSHandle, &nAttr );
        if( 0 > nRc )
        {
            TRACE( "tts_getspeechrate failed error = %s\n", 
                tts_geterror( nRc ) );
            ASSERT( FALSE );
        }
        sTxt.Format( "%d", nAttr );
    }
    return sTxt;
}

CString CSAPChannel::GetSpeed()
{
    ASSERT_VALID( this );
    CString sTxt = "";
    if( -1 != m_nTTSHandle )
    {
        int nAttr  = 0;
        int nRc = tts_getspeechrate( m_nTTSHandle, &nAttr );
        if( 0 > nRc )
        {
            TRACE( "tts_getspeechrate failed error = %s\n", 
                tts_geterror( nRc ) );
            ASSERT( FALSE );
        }
        sTxt.Format( "%d", nAttr );
    }
    return sTxt;
}

CString CSAPChannel::GetPitch()
{
    ASSERT_VALID( this );
       
    CString sTxt = "";
    if( -1 != m_nTTSHandle )
    {
        int nAttr = 0;
        int nRc = tts_getpitch( m_nTTSHandle, &nAttr );
        if( 0 > nRc )
        {
            TRACE( "tts_getspeechrate failed error = %s\n", 
                tts_geterror( nRc ) );
            ASSERT( FALSE );
        }
        sTxt.Format( "%d", nAttr );
    }
    return sTxt;
}

void CSAPChannel::StuffKey( const char cKey )
{
    // this simulates a key pad entry
    // couldn't do it over the board because it would mean
    // opening a channel connected to "this" channel
    m_cDigit = cKey;
    Transition( SEV_KEY_PUNCHED );
}

BOOL CSAPChannel::IsGreeting()
{
	return( ST_GREETING == m_nState );
}

BOOL CSAPChannel::IsIdle()
{
	return( ST_IDLE == m_nState );
}

void CSAPChannel::AdjustSpeed( BOOL bIncrease )
{
    TRACE( "Speed adjustment\n" );
	int nAttr;
    int nRc = tts_getspeechrate( m_nTTSHandle, &nAttr );
    if( 0 > nRc )
    {
        TRACE( "tts_getspeechrate failed error = %s\n", 
            tts_geterror( nRc ) );
        ASSERT( FALSE );
    }
    else
    {
        if( !bIncrease )
        {
            nAttr -= m_nSpeedStep;
        }
        else
        {
            nAttr += m_nSpeedStep;
        }
        nRc = tts_speechrate( m_nTTSHandle, nAttr );
        if( 0 > nRc && TTS_WPM_RANGE != nRc )
        {
            TRACE( "tts_speechrate failed error = %s\n", 
                tts_geterror( nRc ) );
            ASSERT( FALSE );
        }
    }
}

void CSAPChannel::AdjustVolume( BOOL bIncrease )
{
    TRACE( "volume adjustment\n" );
	int nAttr;
    int nRc = tts_getvolume( m_nTTSHandle, &nAttr );
    TRACE( "Current volume = %d\n", nAttr );
    if( 0 > nRc )
    {
        TRACE( "tts_getvolume failed error = %s\n", 
            tts_geterror( nRc ) );
        ASSERT( FALSE );
    }
    else
    {
        if( !bIncrease )
        {
            nAttr -= m_nVolumeStep;
        }
        else
        {
            nAttr += m_nVolumeStep;
        }

        TRACE( "Adjusting volume to %d\n", nAttr );

        nRc = tts_volume( m_nTTSHandle, nAttr );
        if( 0 > nRc && TTS_VOLUME_RANGE != nRc )
        {
            TRACE( "tts_volume failed error = %s\n", 
                tts_geterror( nRc ) );
            ASSERT( FALSE );
        }
    }
}

void CSAPChannel::AdjustPitch( BOOL bIncrease )
{
    TRACE( "pitch adjustment\n" );
	int nAttr;
    int nRc = tts_getpitch( m_nTTSHandle, &nAttr );
    TRACE( "Current volume = %d\n", nAttr );
    if( 0 > nRc )
    {
        TRACE( "tts_getvolume failed error = %s\n", 
            tts_geterror( nRc ) );
        ASSERT( FALSE );
    }
    else
    {
        if( !bIncrease )
        {
            nAttr -= m_nPitchStep;
        }
        else
        {
            nAttr += m_nPitchStep;
        }

        TRACE( "Adjusting pitch to %d\n", nAttr );

        nRc = tts_pitch( m_nTTSHandle, nAttr );
        if( 0 > nRc && TTS_PITCH_RANGE != nRc )
        {
            TRACE( "tts_volume failed error = %s\n", 
                tts_geterror( nRc ) );
            ASSERT( FALSE );
        }
    }
}
