// channel.h

#ifndef __CHANNEL_H__
#define __CHANNEL_H__



#include <srllib.h>
#include <dxxxlib.h>
#include <faxlib.h>

typedef enum 
{
    close = 0,
    open,
} CH_STATUS;



typedef long (* PEVENTHANDLER ) ( DWORD );


///////////////////////////////////////////////////////////////////////////////////////
// errors
#define ERROR_BASE 1000
#define ERROR_CANTOPENVOXFILE           ERROR_BASE + 1
#define ERROR_GENERIC                   ERROR_BASE + 2
#define ERROR_PLAYFAILED                ERROR_BASE + 3
#define ERROR_CANNOTSETLCOFF            ERROR_BASE + 4
#define ERROR_CANNOTSETEVTMASK          ERROR_BASE + 5
#define ERROR_SETPARM                   ERROR_BASE + 6
#define ERROR_CANTSETDIGTYP             ERROR_BASE + 7
#define ERROR_CANTCLRDIGBUF             ERROR_BASE + 8
#define ERROR_CANTCLRTPT                ERROR_BASE + 9
#define ERROR_CANTGETDIGIT              ERROR_BASE + 10
#define ERROR_CANTSTOP                  ERROR_BASE + 11
#define ERROR_CANTOPENFAX               ERROR_BASE + 12
#define ERROR_CANTCLOSEFAX              ERROR_BASE + 13
#define ERROR_CANTDIAL                  ERROR_BASE + 14
#define ERROR_CANTSETCALLP              ERROR_BASE + 15
#define ERROR_CANTOPENFAXFILE           ERROR_BASE + 16
#define ERROR_SENDINGFAX                ERROR_BASE + 17
#define ERROR_INITSTAT                  ERROR_BASE + 18
#define ERROR_FAXFORMATNOTSUPPORTED     ERROR_BASE + 19
#define ERROR_CANTSETHOOK               ERROR_BASE + 20
#define ERROR_CANTSETEVENTHANDLER       ERROR_BASE + 21
#define ERROR_CANTDISABLEEVENTHANDLER   ERROR_BASE + 22
#define ERROR_INVALIDVOLUME             ERROR_BASE + 23
#define ERROR_CANTADJUSTVOLUME          ERROR_BASE + 24
#define ERROR_INVALIDSPEED              ERROR_BASE + 25
#define ERROR_CANTADJUSTSPEED           ERROR_BASE + 26
#define ERROR_CANTADDVOLDIGIT           ERROR_BASE + 27
#define ERROR_CANTADDSPEEDDIGIT         ERROR_BASE + 28
#define ERROR_NOBOARDS                  ERROR_BASE + 29
#define ERROR_CANTOPENBOARD             ERROR_BASE + 30
#define ERROR_CANTCREATECPPOBJECT       ERROR_BASE + 31
#define ERROR_CANTINITIALIZE            ERROR_BASE + 32

///////////////////////////////////////////////////////////////////////////////////////


class CChannelError
{
public:
	CString m_sMsg;
	int m_nSeverity;
	CChannelError()
	{
		m_sMsg.Empty();
		m_nSeverity = 0;
	}
	CChannelError( LPCTSTR sMsg, int nSeverity = 0 )
	{
		m_sMsg = sMsg;
		m_nSeverity = nSeverity;
	}
};

class CChannel : public CObject
{

    DECLARE_DYNCREATE( CChannel )


public:

    // 
	static CChannel* GetChannelFromName( const CString& );
    static void Initialize( CRuntimeClass* pClass, CStringArray& sNames,
        LPCTSTR sIniFile = NULL );
    static void Destroy();
    static void SaveCfg( LPCTSTR sIniName );
    static void LoadCfg( LPCTSTR sIniName );


	BOOL IsOpen();
	int AddVolumeDigit( char cDigit, short nAdjVal = SV_ADD2DB );
	int AddSpeedDigit( char cDigit, short nAdjVal = SV_ADD10PCT );
	int SetSpeed( int nSpeed );
	int SetVolume( int nVol );
	int DisableEventHandler( PEVENTHANDLER pFunc );
	int SetEventHandler( PEVENTHANDLER );
	int DisableFaxEventHandler( PEVENTHANDLER pFunc );
	int SetFaxEventHandler( PEVENTHANDLER );
	int StopFax();
	void CloseFaxFile();
	int AsyncSendFax( LPCTSTR lpFileName, LPCTSTR lpHdr= NULL, 
        LPCTSTR lpId = NULL, WORD wType = DF_ASCII );
	static CString CapToString( int nCap );
	int AsyncDial( LPCTSTR lpszNo, UINT nMode = DX_CALLP, DX_CAP* pCapp = NULL );
	int CloseFax();
	int OpenFax();
	int GetCallerID( LPTSTR szID );
	int Stop();
	int ClearDigits();
	void RetrieveDigits( LPTSTR lpszDigits, LPCTSTR szTerminator = NULL );
	int AsyncGetDigits( char* pszTermMask, WORD* pwMaxDig, 
        WORD* pwMaxIDDTime,
        WORD* pwMaxTime, // max time is in seconds!
        BOOL bClearFirst = TRUE );
	int SetRingCountNotify( int nRings );
	int SetEvtMask( int nMask );
	int NotifyOnLineDrop();
	void ClosePlayFile();

    int PlayVOX( LPCTSTR lpszFileName, BOOL bPlayTillEnd = FALSE );
	void SetPlayRecordRate( int nMode );
	int SetHook( BOOL bOnHook = TRUE );
	void EnableCallerID( BOOL bEnable = TRUE );
	void NotifyOnRing( int nRingCount = 0 );
	void SetSyncMode();
	void SetAsyncMode();
	void ToggleHook();


public:
    CString m_sChanName;
    CH_STATUS m_cStatus;
	int m_nFaxHandle;
    int m_nHandle;
    BOOL m_bOnHook;
    int m_nMode;
    int m_nCallerIDState;  // -1 NONE  0 Disabled 1 Enabled

    // params for play
    DX_IOTT m_iott;
    DV_TPT m_tpt;
    DX_XPB m_xpb;



    // params for getdigit
    DV_TPT* m_pTPTGetDigit;
    DV_DIGIT m_DigitBuff;
    int m_nSizeTPTGetDigit;
	DF_IOTT m_FaxIOTT;



public:
    BOOL Open();
    void Close();
protected:
	static CString m_sOpenErrorMsg;
	char m_cDigit;
    static CMapStringToOb m_Map;
	int m_nSlotNum;
	int m_nChanNum;
	int GetTSlot();
	int GetSpeed();
	virtual int DoBaseChannelSettings();
    CChannel();
    virtual ~CChannel();

    // cannot make these pure virtuals as DYNCREATE won't work
    virtual void operator >> (CString& sOut )
    {
    }
    virtual void operator << (CString& sIn )
    {
    }
    virtual BOOL OnCreate()
    {
        return TRUE;
    }
    virtual void OnDestroy(); // base implememntation hooks and closes.


private:
	LONG m_lFaxType;
	int m_nSendFaxFileHandle;
	void AddNewTPT( DV_TPT*& );
};
#endif
