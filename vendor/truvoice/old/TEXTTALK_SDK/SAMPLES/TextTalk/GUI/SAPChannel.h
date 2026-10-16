// SAPChannel.h
#ifndef __SAPCHANNEL_H__
#define __SAPCHANNEL_H__
#include "afxtempl.h"
#include "SMChannel.h"



typedef enum 
{
    ST_IDLE,
    ST_ANSWERING,
    ST_GREETING,
	ST_WAITINGFORSTOP,   // waiting for stop before connecting
    ST_DISCONNECTING,
    ST_GETTINGMENUOPTION,
    ST_PLAYINGTEST,
    ST_PAUSED,
    ST_WAITINGFORTEXT,
} SAPState;

typedef enum
{
    SEV_RINGING,
    SEV_OFFHOOK,
    SEV_PLAYDONE,
    SEV_DISCONNECTED,
    SEV_KEYS_ENTERED,
    SEV_KEY_TIMEOUT,
    SEV_KEY_PUNCHED,
} SAPEvent;



class CSAPChannel : public CSMChannel
{
public:
	BOOL IsIdle();
	BOOL IsGreeting();


	static CString m_sDefaultGreeting;
	static CString m_sGreetingFileName;
	static int m_nSpeedStep;
	static int m_nVolumeStep;
	static int m_nPitchStep;


	void StuffKey( const char );
	CString GetPitch();
	CString GetVolume();
	CString GetSpeed();
	CString GetVoice();
	void SendText( CString sText );
	BOOL IsWaitingForText();
	BOOL IsPlayingText();
	CString m_sPlayBuffer;
	CString GetStateAsText();

protected:
	void AdjustPitch( BOOL bIncrease );
	void AdjustVolume( BOOL bIncrease );
	void AdjustSpeed( BOOL bIncrease );
	int m_nVoice;
	int PlayString();
	CString GetPlayText();
    static BOOL m_bTTSInit;
	int m_nTTSHandle;

protected:
    CSAPChannel();
    virtual ~CSAPChannel();


    // OVERIDABLES
    virtual BOOL OnCreate();
    virtual void OnDestroy();

    // called by OnCreate.  feeds the transition table
    // other derived classes may have different transition tables
    virtual void InitTransTable();

    
private:
    // implementation

    // state machine
    static long EventHandler( DWORD );


    // state handlers
    int OnKeyWhileTest();
    int OnKeyWhileWaiting();
    int OnPhoneRing();
    int OnOffHook();
    int OnGreetingDone();
    int OnDisconnected();
    int OnDisconnecting();
    int OnMenuOptionEntered();
    int OnTestPlayDone();
    int OnStoppingPlay();


    // some convenience functions
    int PlayFile( LPCTSTR lpFile, LPCTSTR lpDefault = NULL );

    DECLARE_DYNCREATE( CSAPChannel );

};

#endif


