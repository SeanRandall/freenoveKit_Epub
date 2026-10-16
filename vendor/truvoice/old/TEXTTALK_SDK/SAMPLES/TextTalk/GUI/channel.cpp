// channel.cpp
#include "stdafx.h"
#include "channel.h"
#include <fcntl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE( CChannel, CObject )


// obligatory inits
CMapStringToOb CChannel::m_Map;
CString CChannel::m_sOpenErrorMsg = "";

///////////////////////////////////////////////////////////////////////////////////////////
// some utility funcs
template <class T>
static inline void DELETE_STRUC( T*& pSt )
{
    if( NULL != pSt )
    {
        delete pSt;
        pSt = NULL;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////




CChannel::CChannel()
{
    m_cStatus = close;
    m_nHandle = -1;
    m_nMode = EV_SYNC;
    m_nFaxHandle = -1;


    m_pTPTGetDigit = NULL;
    m_nSendFaxFileHandle = -1;
}

CChannel::~CChannel()
{
    DELETE_STRUC( m_pTPTGetDigit );
}

BOOL CChannel::Open()
{
    ASSERT( m_cStatus == close );
    WORD wParam = 0;
    m_nHandle = dx_open( (LPCTSTR) m_sChanName, NULL );
    if( -1 == m_nHandle )
    {
        return FALSE;
    }
    m_bOnHook = ( ATDX_HOOKST( m_nHandle ) == DX_ONHOOK );
    
#define _NT_BUG_
#ifdef _NT_BUG_
//#pragma message ("caller id detect disabled in NT causing problems" )
// 4.25 SC problem!
    m_nCallerIDState = -1;
#else
    // determine if channel supports Caller ID
    if( dx_getparm( m_nHandle, DXCH_CALLID, &wParam ) == 0) 
    {
	    m_nCallerIDState = ( wParam == DX_CALLIDDISABLE ) ? 0 : 1;
    } 
    else 
    {
	    // not supported
	    m_nCallerIDState = -1;
    }
#endif

    int nErr = DoBaseChannelSettings(); // overridable
    if( 0 != nErr )
    {
        ASSERT( FALSE );
        return FALSE;
    }

#ifdef _RELEASE_TRACE_
    OutputDebugString( "Channel opened\n" );
#endif
    m_cStatus = open;
    return TRUE;
}

void CChannel::Close()
{
    ASSERT( m_cStatus != close );
    ASSERT( -1 != m_nHandle );
    dx_clrsvcond( m_nHandle );
    if( -1 == dx_close( m_nHandle ) )
    {
        TRACE( "Close failed error = %s\n", ATDV_ERRMSGP( m_nHandle ) );
        ASSERT( FALSE );
    }
    m_nHandle = -1;
    m_cStatus = close;
}

void CChannel::ToggleHook()
{
    ASSERT( m_nMode == EV_ASYNC ); // dropped support for sync
    int nParam = ( ATDX_HOOKST( m_nHandle ) == DX_ONHOOK ) ? DX_OFFHOOK : DX_ONHOOK;
    Stop();

    if( -1 == dx_sethook( m_nHandle, nParam, m_nMode ) == -1 )
    {
        ASSERT( FALSE );
        AfxMessageBox( "Error on hook" );
    }

    
#ifdef _RELEASE_TRACE_
    OutputDebugString( "HOOK Toggled\n" );
#endif

    m_bOnHook = ( ATDX_HOOKST( m_nHandle ) == DX_ONHOOK ); // note for async mode  this does not mean anything

}


void CChannel::SetAsyncMode()
{
    m_nMode = EV_ASYNC;
}

void CChannel::SetSyncMode()
{
    m_nMode = EV_SYNC;
}

void CChannel::NotifyOnRing( int nRingCount )
{
    ASSERT( FALSE ); /// cannot use this

    
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    if( -1 == dx_setevtmsk( m_nHandle, DM_RINGS) )
    {
        TRACE( "cannot enable ring mask\n" );
        ASSERT( FALSE );
        return;
    }
    if( -1 == dx_setparm( m_nHandle, DXCH_RINGCNT, &nRingCount ) )
    {
        TRACE( "cannot set ring count\n" );
        ASSERT( FALSE );
        return;
    }
}

void CChannel::EnableCallerID( BOOL bEnable )
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    if( m_nCallerIDState != -1 )
    {
        WORD wParam = bEnable ? DX_CALLIDENABLE : DX_CALLIDDISABLE;

        if( dx_setparm( m_nHandle, DXCH_CALLID, &wParam) == 0 ) 
        {
            m_nCallerIDState = bEnable ? 1 : 0;
        }
    }
}

int CChannel::SetHook( BOOL bOnHook /*= TRUE */)
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    int nParam = bOnHook ? DX_ONHOOK : DX_OFFHOOK;

    if( -1 == dx_sethook( m_nHandle, nParam, m_nMode ) )
    {
        TRACE( "dx_sethook error = %d\n", ATDV_ERRMSGP( m_nHandle ) );
        ASSERT( FALSE );
        return ERROR_CANTSETHOOK;
    }

    // FIXME:
    // FIXME:
    // FIXME:
    // FIXME:  NEED TO CLEAN THIS UP
    // FIXME:
    // FIXME:
    // FIXME:
    // THIS FLAG IS USED ONLY BY THE VIEW.  NEED A BETTER WAY as THIS BINDS THIS CLASS TO 
    // THE VIEW 
    m_bOnHook = ( ATDX_HOOKST( m_nHandle ) == DX_ONHOOK ); // note for async mode  this does not mean anything
    return 0;
}

void CChannel::SetPlayRecordRate( int nMode )
{

}

int CChannel::PlayVOX( LPCTSTR lpszFileName, BOOL bPlayTillEnd )
{
    // plays one VOX file
    int nRc = 0;
    m_iott.io_fhandle = dx_fileopen( lpszFileName, O_RDONLY|O_BINARY );
    if( -1 == m_iott.io_fhandle )
    {
        return ERROR_CANTOPENVOXFILE;
    }

    m_iott.io_type = IO_DEV|IO_EOT; // from a file and only one
    m_iott.io_bufp = 0;
    m_iott.io_offset = 0;
    m_iott.io_length = -1;
    m_iott.io_nextp = 0;
    m_iott.io_prevp = 0;

    // set it to the default for VOX
    m_xpb.wFileFormat = FILE_FORMAT_VOX;
    m_xpb.wDataFormat = DATA_FORMAT_DIALOGIC_ADPCM;
    m_xpb.nSamplesPerSec = DRT_6KHZ;
    m_xpb.wBitsPerSample = 4;


    m_tpt.tp_type = IO_EOT;
    if( -1 != dx_clrtpt( &m_tpt, 1 ) )
    {

        m_tpt.tp_termno = bPlayTillEnd ? DX_MAXTIME : DX_MAXDTMF;
        m_tpt.tp_length = bPlayTillEnd ? 6000 : 1; // terminate on 1 digit
        m_tpt.tp_flags = TF_MAXDTMF;
        if( -1 == dx_playiottdata( m_nHandle, &m_iott, &m_tpt, &m_xpb, m_nMode ) )
        {
            TRACE( "last error = %d\n", ATDV_LASTERR( m_nHandle ) );
            nRc = ERROR_PLAYFAILED;
        }
    }
    else
    {
        nRc = ERROR_GENERIC;
    }
    if( 0 != nRc )
    {
        dx_fileclose( m_iott.io_fhandle );
    }
    return nRc;
}

void CChannel::ClosePlayFile()
{
    dx_fileclose( m_iott.io_fhandle );
}

int CChannel::NotifyOnLineDrop()
{
    ASSERT( FALSE ); /// cannot use this


    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    if( -1 == dx_setevtmsk( m_nHandle, DM_LCOFF ) )
    {
        TRACE( "cannot enable ring mask\n" );
        ASSERT( FALSE );
        return ERROR_CANNOTSETLCOFF;
    }
    return 0;
}

int CChannel::SetEvtMask( int nMask )
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );

    if( -1 == dx_setevtmsk( m_nHandle, nMask ) )
    {
        return ERROR_CANNOTSETEVTMASK;
    }
    return 0;
}

int CChannel::SetRingCountNotify( int nRings )
{
    if( -1 == dx_setparm( m_nHandle, DXCH_RINGCNT, &nRings ) )
    {
        TRACE( "cannot set ring count\n" );
        ASSERT( FALSE );
        return ERROR_SETPARM;
    }
    return 0;
}

int CChannel::DoBaseChannelSettings() // virtual and called at open
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    int nRc = 0;

    if( -1 == dx_setdigtyp( m_nHandle, D_DTMF|D_DPD ) )
    {
        ASSERT( FALSE );
        return ERROR_CANTSETDIGTYP;
    }
    dx_deltones( m_nHandle );
    if( -1 == dx_initcallp( m_nHandle ) )
    {
        TRACE( "InitCallp error = %d %s\n", ATDV_LASTERR( m_nHandle ), 
            ATDV_ERRMSGP( m_nHandle) );
        ASSERT( FALSE );
        return ERROR_CANTSETCALLP;
    }
    return nRc;
}

int CChannel::AsyncGetDigits( char* pszTermMask, WORD* pwMaxDig, WORD* pwMaxIDDTime,
                             WORD* pwMaxTime, BOOL bClearFirst )
{

    // NOTE: LCOFF is always enabled

    CWordArray MaskHolder;

    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );

    // NOTE cannot handle multiple AsyncGetDigits!!!!

    if( NULL != m_pTPTGetDigit )
    {
#ifdef _DEBUG
        TRACE( "WARNING: From %s clearing unused m_pTPTGetDigit\n", THIS_FILE );
#endif
        DELETE_STRUC( m_pTPTGetDigit );
    }

    if( bClearFirst )
    {
        if( -1 == dx_clrdigbuf( m_nHandle ) )
        {
            ASSERT( FALSE );
            return ERROR_CANTCLRDIGBUF;
        }
    }

    m_nSizeTPTGetDigit = 1; // LCOFF is always here

    // make sure valid mask
    if( NULL != pszTermMask )
    {
        char* pP = pszTermMask;
        while( '\0' != *pP )
        {
            int nMask;
            switch( *pP )
            {
            case '0' : nMask = DM_0; break;
            case '1' : nMask = DM_1; break;
            case '2' : nMask = DM_2; break;
            case '3' : nMask = DM_3; break;
            case '4' : nMask = DM_4; break;
            case '5' : nMask = DM_5; break;
            case '6' : nMask = DM_6; break;
            case '7' : nMask = DM_7; break;
            case '8' : nMask = DM_8; break;
            case '9' : nMask = DM_9; break;
            case '*' : nMask = DM_S; break;
            case '#' : nMask = DM_P; break;
            default:ASSERT( FALSE );break;
            }
            MaskHolder.Add( (WORD) nMask );
            pP++;
        }
    }


    m_nSizeTPTGetDigit += MaskHolder.GetSize();

    if( NULL != pwMaxDig )
    {
        m_nSizeTPTGetDigit++;
    }
    if( NULL != pwMaxTime )
    {
        m_nSizeTPTGetDigit++;
    }
    if( NULL != pwMaxIDDTime )
    {
        m_nSizeTPTGetDigit++;
    }

    TRY
    {
        m_pTPTGetDigit = new DV_TPT[ m_nSizeTPTGetDigit ]; // LCOFF is always active
    }
    CATCH( CMemoryException, pE )
    {
        ASSERT( FALSE );
        THROW_LAST();
    }
    END_CATCH

    if( -1 == dx_clrtpt( m_pTPTGetDigit, m_nSizeTPTGetDigit ) )
    {
        ASSERT( FALSE );
        DELETE_STRUC( m_pTPTGetDigit );
        return ERROR_CANTCLRTPT;
    }

    {
        DV_TPT* pTPT = NULL;
    
        AddNewTPT( pTPT );
        pTPT->tp_type = IO_EOT;
        pTPT->tp_termno = DX_LCOFF;
        pTPT->tp_length = 3; // 30 ms
        pTPT->tp_flags = TF_LCOFF|TF_10MS;

        for( int i = 0; i < MaskHolder.GetSize(); i++ )
        {
            AddNewTPT( pTPT );
            pTPT->tp_type = IO_EOT;
            pTPT->tp_termno = DX_DIGMASK;
            pTPT->tp_length = (int) MaskHolder.GetAt( i );
            pTPT->tp_flags = TF_DIGMASK; // level triggered
        }

        if( NULL != pwMaxDig )
        {
            AddNewTPT( pTPT );
            pTPT->tp_type = IO_EOT;
            pTPT->tp_termno = DX_MAXDTMF;
            pTPT->tp_length = *pwMaxDig;
            pTPT->tp_flags = TF_MAXDTMF; // level triggered
        }

        if( NULL != pwMaxIDDTime )
        {
            AddNewTPT( pTPT );
            pTPT->tp_type = IO_EOT;
            pTPT->tp_termno = DX_IDDTIME;
            pTPT->tp_length = *pwMaxIDDTime*100;
            pTPT->tp_flags = TF_IDDTIME|TF_10MS;
        }

        if( NULL != pwMaxTime )
        {
            AddNewTPT( pTPT );
            pTPT->tp_type = IO_EOT;
            pTPT->tp_termno = DX_MAXTIME;
            pTPT->tp_flags = TF_MAXTIME|TF_10MS;
            pTPT->tp_length = *pwMaxTime*100;
        }
    }
    // let rip
    if( -1 == dx_getdig( m_nHandle, m_pTPTGetDigit, &m_DigitBuff, EV_ASYNC ) )
    {
        ASSERT( FALSE );
        DELETE_STRUC( m_pTPTGetDigit );
        TRACE( "last error = %d\n", ATDV_LASTERR( m_nHandle ) );
        return ERROR_CANTGETDIGIT;
    }


    return 0;
    
}

void CChannel::RetrieveDigits( LPTSTR lpszDigits, LPCTSTR szTerm )
{
    *lpszDigits = '\0';
    if( NULL == m_pTPTGetDigit )
    {
        ASSERT( FALSE );
        return;
    }

    ASSERT( lstrlen( m_DigitBuff.dg_value ) < 31 );
    lstrcpy( lpszDigits, m_DigitBuff.dg_value );
    DELETE_STRUC( m_pTPTGetDigit );
    VERIFY( -1 != dx_clrdigbuf( m_nHandle ) );

    if( NULL != szTerm )
    {
        char* pP = strstr( lpszDigits, szTerm );
        if( NULL != pP )
        {
            *pP = '\0';
        }
    }
}

void CChannel::AddNewTPT( DV_TPT*& pTPT)
{
    // will rework later for link list
    if( NULL == pTPT )
    {
        pTPT = m_pTPTGetDigit;
        ASSERT( NULL != pTPT );
    }
    else
    {
        // set previous link
        pTPT->tp_type = IO_CONT;
        pTPT++;
    }
}

int CChannel::ClearDigits()
{
    if( -1 == dx_clrdigbuf( m_nHandle ) )
    {
        ASSERT( FALSE );
        return ERROR_CANTCLRDIGBUF;
    }
    return 0;
}

int CChannel::Stop()
{
    if( -1 == dx_stopch( m_nHandle, EV_ASYNC ) )
    {
        return ERROR_CANTSTOP;
    }
    else 
    {
        return 0;
    }
}

int CChannel::GetCallerID( LPTSTR szID )
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    *szID = '\0';
    if( m_nCallerIDState != 1 )
    {
        return 0;
    }
    dx_gtcallid( m_nHandle, (BYTE*) szID );
    switch( ATDV_LASTERR( m_nHandle ) )
    {
    case EDX_NOERROR:
        break;
    case EDX_CLIDINFO:
        TRACE( "Call received, no Caller ID.\n");
        *szID = '\0';
	    break;
    case EDX_CLIDBLK:
        TRACE( "Call received, Caller ID was blocked.\n");
        *szID = '\0';
	    break;
    case EDX_CLIDOOA:
        *szID = '\0';
        TRACE( "Call received, Caller ID out of area.");
	    break;
    default:
        TRACE( "Call received, Caller ID Unknown error %d.");
        *szID = '\0';
	    break;
    }
    return 0;
}

int CChannel::OpenFax()
{

    ASSERT_VALID( this );
    if( -1 != m_nFaxHandle )
    {
        TRACE( "opening an opened already opened fax channel\n" );
        ASSERT( FALSE );
        return -1;
    }

    m_nFaxHandle = fx_open( (LPTSTR) (LPCTSTR) m_sChanName, NULL );
    if( -1 == m_nFaxHandle )
    {
        return ERROR_CANTOPENFAX;
    }
    else 
    {
        m_lFaxType = ATFX_CHTYPE( m_nFaxHandle );
        ASSERT( AT_FAILURE != m_lFaxType );
        //VERIFY( -1 != sr_enbhdlr( m_nFaxHandle, EV_ANYEVT, CStateMachine::SRFaxEventHandler ) );
        return 0;
    }
}

int CChannel::CloseFax()
{
    ASSERT_VALID( this );
    if( -1 == m_nFaxHandle )
    {
        ASSERT( FALSE );
        return -1;
    }

    if( -1 ==  fx_close( m_nFaxHandle ) )
    {
        return ERROR_CANTCLOSEFAX;
    }
    else 
    {
        //VERIFY( -1 != sr_dishdlr( m_nFaxHandle, EV_ANYEVT, CStateMachine::SRFaxEventHandler ) );
        m_nFaxHandle = -1;
        return 0;
    }
}

int CChannel::AsyncDial( LPCTSTR lpszNo, UINT nMode /*= DX_CALLP*/, DX_CAP* pCapp /*= NULL*/ )
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    if( -1 == dx_dial( m_nHandle, lpszNo, pCapp, EV_ASYNC|nMode ) )
    {
        return ERROR_CANTDIAL;
    }
    return 0;
}

CString CChannel::CapToString( int nCap ) // static
{
    CString sR;
    sR.Empty();
    switch( nCap )
    {
    case CR_BUSY: sR = "Far end was busy"; break;
    case CR_CEPT: sR = "Operator intercept"; break;
    case CR_ERROR: sR = "Call analysis error"; break;
    case CR_FAXTONE: sR = "Fax tone"; break;
    case CR_NOANS: sR = "No answer"; break;
    case CR_NODIALTONE: sR = "No dial tone"; break;
    case CR_NORB: sR = "No ring back"; break;
    case CR_STOPD: sR = "Call analysis stopped"; break;
    case CR_CNCT: sR = "Call connected"; break;
    default: ASSERT( FALSE ); break;
    }
    return sR;
}

int CChannel::AsyncSendFax( LPCTSTR lpFileName, LPCTSTR lpHdr, LPCTSTR lpId,
                           WORD wType /*= DF_ASCII */ )
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nFaxHandle );
    ASSERT( -1 == m_nSendFaxFileHandle );
    m_nSendFaxFileHandle = dx_fileopen( lpFileName, O_RDONLY|O_BINARY, NULL );
    if( -1 == m_nSendFaxFileHandle )
    {
        ASSERT( FALSE );
        return ERROR_CANTOPENFAXFILE;
    }

    if( -1 == fx_initstat( m_nFaxHandle, DF_TX ) )
    {
        TRACE( "initstat ERROR error = %d %s\n", ATDV_LASTERR( m_nFaxHandle ), 
            ATDV_ERRMSGP( m_nFaxHandle) );
        ASSERT( FALSE );
        return ERROR_INITSTAT;
    }

    // 
    {
        if( NULL != lpId )
        {
            VERIFY( 0 == fx_setparm( m_nFaxHandle, FC_LOCALID, (void*) lpId ) );
        }
        if( NULL != lpHdr )
        {
            VERIFY( 0 == fx_setparm( m_nFaxHandle, FC_HDRUSER, (void*) lpHdr ) );
        }
    }
    fx_setiott( &m_FaxIOTT, m_nSendFaxFileHandle, wType, DFC_EOP );
    m_FaxIOTT.io_type |= IO_EOT;
    m_FaxIOTT.io_coding = DF_MH;

    if( DF_TIFF == wType )
    {
        LONG lCont = DFC_MPS;

        VERIFY( 0 == fx_setparm( m_nFaxHandle, FC_SENDCONT, (void*) &lCont ) );
    }


    if( -1 == fx_sendfax( m_nFaxHandle, &m_FaxIOTT, EV_ASYNC|DF_PHASEB|DF_PHASED ) )
    {
        TRACE( "SendFax error = %d %s\n", ATDV_LASTERR( m_nFaxHandle ), 
            ATDV_ERRMSGP( m_nFaxHandle) );
        ASSERT( FALSE );
        return ERROR_SENDINGFAX;
    }
    return 0;
}

void CChannel::CloseFaxFile()
{
    if( -1 != m_nSendFaxFileHandle )
    {
        dx_fileclose( m_nSendFaxFileHandle );
    }
    else
    {
        TRACE( "**** WARNING:  CloseFaxFile called on a non opened file\n" );
    }
    m_nSendFaxFileHandle = -1;
}

int CChannel::StopFax()
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nFaxHandle );
    return fx_stopch( m_nFaxHandle, EV_ASYNC );
}

int CChannel::SetEventHandler( PEVENTHANDLER pFunc )
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    ASSERT( NULL != pFunc );
    if( -1 == sr_enbhdlr( m_nHandle, EV_ANYEVT, pFunc ) )
    {
        TRACE( "sr_enbhdlr failed %s\n", ATDV_ERRMSGP( m_nHandle ) );
        ASSERT( FALSE );
        return ERROR_CANTSETEVENTHANDLER;
    }
    return 0;
}

int CChannel::DisableEventHandler( PEVENTHANDLER pFunc )
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    ASSERT( NULL != pFunc );
    if( -1 == sr_dishdlr( m_nHandle, EV_ANYEVT, pFunc ) )
    {
        ASSERT( FALSE );
        return ERROR_CANTDISABLEEVENTHANDLER;
    }
    return 0;

}

int CChannel::SetVolume( int nVol )
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    // nVol is 0 to 20
    if( nVol > 20 || nVol < 0 )
    {
        ASSERT( FALSE );
        return ERROR_INVALIDVOLUME;
    }
    if( -1 == dx_adjsv( m_nHandle, SV_VOLUMETBL, SV_ABSPOS, nVol - 10 ) )
    {
        TRACE( "ERROR CODE  = %d\n", ATDV_LASTERR( m_nHandle ) );
        ASSERT( FALSE );
        return ERROR_CANTADJUSTVOLUME;
    }
    return 0;
}

int CChannel::SetSpeed( int nSpeed )
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    // nSpeed is 0 to 20
    if( nSpeed > 20 || nSpeed < 0 )
    {
        ASSERT( FALSE );
        return ERROR_INVALIDSPEED;
    }
    if( -1 == dx_adjsv( m_nHandle, SV_SPEEDTBL, SV_ABSPOS, nSpeed - 10 ) )
    {
        TRACE( "ERROR CODE  = %d\n", ATDV_LASTERR( m_nHandle ) );
        ASSERT( FALSE );
        return ERROR_CANTADJUSTSPEED;
    }
    return 0;
}

int CChannel::SetFaxEventHandler( PEVENTHANDLER pFunc )
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nFaxHandle );
    ASSERT( NULL != pFunc );
    if( -1 == sr_enbhdlr( m_nFaxHandle, EV_ANYEVT, pFunc ) )
    {
        ASSERT( FALSE );
        return ERROR_CANTSETEVENTHANDLER;
    }
    return 0;
}

int CChannel::DisableFaxEventHandler( PEVENTHANDLER pFunc )
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nFaxHandle );
    ASSERT( NULL != pFunc );
    if( -1 == sr_dishdlr( m_nFaxHandle, EV_ANYEVT, pFunc ) )
    {
        ASSERT( FALSE );
        return ERROR_CANTDISABLEEVENTHANDLER;
    }
    return 0;

}



int CChannel::AddSpeedDigit( char cDigit, short nAdjVal )
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    if( -1 == dx_addspddig( m_nHandle, cDigit, nAdjVal ) )
    {
        return ERROR_CANTADDSPEEDDIGIT;
    }
    return 0;
}

int CChannel::AddVolumeDigit( char cDigit, short nAdjVal )
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    if( -1 == dx_addvoldig( m_nHandle, cDigit, nAdjVal ) )
    {
        return ERROR_CANTADDVOLDIGIT;
    }
    return 0;
}

BOOL CChannel::IsOpen()
{
    ASSERT_VALID( this );
    ASSERT( -1 != m_nHandle );
    return ( m_cStatus == open );
}




// this needs to be reworked to allow only a set of
// channels to be created.  i.e. different channels can
// have different derived classes
void CChannel::Initialize( CRuntimeClass* pClass, 
                         CStringArray& sNames, LPCTSTR sIniFile /* = NULL */ )
{

    // make sure caller is asking us to construct something we know
    ASSERT( pClass->IsDerivedFrom( RUNTIME_CLASS( CChannel ) ) );

    int nBoards, i;
    sr_getboardcnt( DEV_CLASS_VOICE, &nBoards );
    if( 0 == nBoards )
    {
		throw CChannelError( "No boards found", 10 );
    }

    for( i = 1; i <= nBoards; i++ )
    {
        CString sTxt;
        int nCBoard;
        int nSubDev;
        LPSTR* lplpszNameDev;
        sTxt.Format( "dxxxB%d", i );
        nCBoard = dx_open( (LPCTSTR) sTxt, NULL );
        if( -1 == nCBoard )
        {
			CString sMsg;
			sMsg.Format( "Cannot open board %s", (LPCTSTR) sTxt );
            throw CChannelError( sMsg, 10 );
        }

        // get number of subdivisions
        nSubDev = ATDV_SUBDEVS( nCBoard );
        lplpszNameDev = ATDX_CHNAMES( nCBoard );
        for( int j = 0; j < nSubDev; j++ )
        {
            CString sTxt;
            CChannel* pCH = dynamic_cast<CChannel*> ( pClass->CreateObject() );
            if( NULL == pCH )
            {
                ASSERT( FALSE );
                throw CChannelError( "Out of memory", 10 );
            }
            ASSERT_VALID( pCH );
            ASSERT_KINDOF( CChannel, pCH );


            pCH->m_sChanName = *(lplpszNameDev+j);
            pCH->m_cStatus = close;
            pCH->m_nChanNum = j;
            pCH->m_nSlotNum = i;
            sTxt = *(lplpszNameDev+j);
            sNames.Add( sTxt );
            m_Map.SetAt( sTxt, dynamic_cast <CObject*> (pCH) );

            // allow derived classes to do their own initializations
			// OnCreate's may throw their own exceptions
            if( !pCH->OnCreate() )
            {
				if( m_sOpenErrorMsg.IsEmpty() )
				{
					m_sOpenErrorMsg = "Initialization failed";
				}
                throw CChannelError( m_sOpenErrorMsg, 10 );
            }
        }

        dx_close( nCBoard );
    }

    LoadCfg( sIniFile );  // allow derived classes to do whatever
                          // they want from the INI file

}


void CChannel::LoadCfg( LPCTSTR sIniFile )
{
    CString sFile = sIniFile;
    for( POSITION pos = m_Map.GetStartPosition(); NULL != pos; )
    {
        CString sKey;
        CChannel* pCh;
        m_Map.GetNextAssoc( pos, sKey, (CObject*&) pCh );
        ASSERT_KINDOF( CChannel, pCh );
        *pCh << sFile;
    }
}


void CChannel::SaveCfg( LPCTSTR sIniFile )
{
    CString sFile = sIniFile;
    for( POSITION pos = m_Map.GetStartPosition(); NULL != pos; )
    {
        CString sKey;
        CChannel* pCh;
        m_Map.GetNextAssoc( pos, sKey, (CObject*&) pCh );
        ASSERT_KINDOF( CChannel, pCh );
        *pCh >> sFile;
    }
}

void CChannel::OnDestroy()
{
    if( IsOpen() )
    {
        SetHook( TRUE );
        Close();
    }
}

void CChannel::Destroy()
{
    for( POSITION pos = m_Map.GetStartPosition(); NULL != pos; )
    {
        CString sKey;
        CChannel* pCh;
        m_Map.GetNextAssoc( pos, sKey, (CObject*&) pCh );
        ASSERT_KINDOF( CChannel, pCh );

        // allow derived classes to do their own exit stuff
        pCh->OnDestroy();
        delete pCh;
    }

    m_Map.RemoveAll();
}

int CChannel::GetSpeed()
{
    int nSpeed, nVolume;
    if( -1 == dx_getcursv( m_nHandle, &nVolume, &nSpeed ) )
    {
        TRACE( "dx_getcursv error %d\n", ATDV_LASTERR( m_nHandle ) );
        return -1;
    }
    return nSpeed;
}

int CChannel::GetTSlot()
{
    return m_nSlotNum;
}

CChannel* CChannel::GetChannelFromName( const CString& sName )
{
    CObject* pObj;
    if( !m_Map.Lookup( sName, pObj ) )
    {
        return NULL;
    }


    ASSERT_VALID( pObj );
    return dynamic_cast <CChannel*> (pObj);    
}
