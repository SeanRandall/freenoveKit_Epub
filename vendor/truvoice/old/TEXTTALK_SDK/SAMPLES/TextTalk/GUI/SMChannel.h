// SMChannel.h
// definition of CSMChannel -- a CChannel with a state machine.

#ifndef __SMCHANNEL_H__
#define __SMCHANNEL_H__

#include "afxtempl.h"
#include "channel.h"


class CSMTransition;
class CSMChannel;
typedef int (CSMChannel::*PTRANSITIONHANDLER ) ();

class CSMChannel : public CChannel
{
public:
	void SetWindowCallback( const HWND& hWnd, const int wm );

protected:
    int m_nCallbackMsg;
	HWND m_hNotifyWnd;
    static CMap< int, int&, CSMChannel*, CSMChannel* > m_HandleToObject;
    int m_nState;
    BOOL m_bHandlerInit;

    // note there is one transition table for one object
    // memory hog but who's counting
    CArray< CSMTransition*, CSMTransition*& > m_TransTable;


    // point this to your event handler
    PEVENTHANDLER m_pEventHandler;
protected:
    CSMChannel();
    virtual ~CSMChannel();

    // OVERIDABLES
    virtual void Transition( int );
    virtual BOOL OnCreate();
    virtual void OnDestroy();

    // called by OnCreate.  feeds the transition table
    // other derived classes may have different transition tables
    virtual void InitTransTable();


    virtual BOOL SetEventHandler();
    virtual void DisableEventHandler();


    // convenience functions
    // use this to add to the state transition table
    void PutTT( int nState, int nEvent, PTRANSITIONHANDLER pH );
    
private:
    // implementation
    DECLARE_DYNCREATE( CSMChannel );

};


// container for transitions
class CSMTransition
{
public:
    int m_nCurrentState;
    int m_nEvent;
    PTRANSITIONHANDLER m_pHandler;
    CSMTransition( int nState, int nEvent, PTRANSITIONHANDLER pH )
    {
        m_pHandler = pH;
        m_nCurrentState = nState;
        m_nEvent = nEvent;
    }
};


#endif

