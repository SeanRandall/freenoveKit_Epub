; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CSpeechAppView
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "SpeechApp.h"
LastPage=0

ClassCount=10
Class1=CSpeechAppApp
Class2=CSpeechAppDoc
Class3=CSpeechAppView
Class4=CMainFrame

ResourceCount=8
Resource1=IDD_HIHO
Resource2=IDR_MAINFRAME
Class5=CAboutDlg
Resource3=IDD_SENDTEXTDLG
Class6=CBlurbDlg
Resource4=IDD_BLURB
Class7=CListCtrlExt
Class8=CTTSDlg
Resource5=IDD_TTSDLG
Class9=CSendDlg
Resource6=IDD_PHONEPAD
Class10=CPhonePad
Resource7=IDD_SPEECHAPP_FORM
Resource8=IDD_ABOUTBOX

[CLS:CSpeechAppApp]
Type=0
HeaderFile=SpeechApp.h
ImplementationFile=SpeechApp.cpp
Filter=N
BaseClass=CWinApp
VirtualFilter=AC

[CLS:CSpeechAppDoc]
Type=0
HeaderFile=SpeechAppDoc.h
ImplementationFile=SpeechAppDoc.cpp
Filter=N

[CLS:CSpeechAppView]
Type=0
HeaderFile=SpeechAppView.h
ImplementationFile=SpeechAppView.cpp
Filter=D
LastObject=IDC_HELP_ME_RHONDA
BaseClass=CFormView
VirtualFilter=VWC

[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T
LastObject=IDC_USINGME



[CLS:CAboutDlg]
Type=0
HeaderFile=SpeechApp.cpp
ImplementationFile=SpeechApp.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_APP_EXIT
Command2=ID_VIEW_TOOLBAR
Command3=ID_VIEW_STATUS_BAR
Command4=IDM_PHONEPAD
Command5=IDC_USERDICT
Command6=IDC_USINGME
Command7=ID_APP_ABOUT
CommandCount=7

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_PRINT
Command5=ID_EDIT_UNDO
Command6=ID_EDIT_CUT
Command7=ID_EDIT_COPY
Command8=ID_EDIT_PASTE
Command9=ID_EDIT_UNDO
Command10=ID_EDIT_CUT
Command11=ID_EDIT_COPY
Command12=ID_EDIT_PASTE
Command13=ID_NEXT_PANE
Command14=ID_PREV_PANE
CommandCount=14

[DLG:IDD_SPEECHAPP_FORM]
Type=1
Class=CSpeechAppView
ControlCount=7
Control1=IDC_STATIC,static,1342308352
Control2=IDC_LIST1,SysListView32,1350631437
Control3=IDC_STATIC,static,1342308352
Control4=IDC_EDIT1,edit,1353783492
Control5=IDC_SENDTEXTR,button,1476460544
Control6=IDC_LOAD,button,1476460544
Control7=IDC_HELP_ME_RHONDA,button,1342242816

[TB:IDR_MAINFRAME]
Type=1
Command1=ID_APP_ABOUT
Command2=IDM_PHONEPAD
Command3=IDC_USERDICT
CommandCount=3

[DLG:IDD_BLURB]
Type=1
Class=CBlurbDlg
ControlCount=1
Control1=IDC_STATIC,static,1342308352

[CLS:CBlurbDlg]
Type=0
HeaderFile=BlurbDlg.h
ImplementationFile=BlurbDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=CBlurbDlg

[CLS:CListCtrlExt]
Type=0
HeaderFile=ListCtrlExt.h
ImplementationFile=ListCtrlExt.cpp
BaseClass=CListCtrl
LastObject=IDC_EDIT1

[DLG:IDD_TTSDLG]
Type=1
Class=CTTSDlg
ControlCount=3
Control1=IDOK,button,1342242817
Control2=IDC_BLURB,static,1342308352
Control3=IDC_EDIT1,edit,1353779396

[CLS:CTTSDlg]
Type=0
HeaderFile=TTSDlg.h
ImplementationFile=TTSDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CTTSDlg

[DLG:IDD_SENDTEXTDLG]
Type=1
Class=CSendDlg
ControlCount=5
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_EDIT1,edit,1353781444
Control5=IDC_LOAD,button,1342242816

[CLS:CSendDlg]
Type=0
HeaderFile=SendDlg.h
ImplementationFile=SendDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_LOAD

[DLG:IDD_PHONEPAD]
Type=1
Class=CPhonePad
ControlCount=21
Control1=IDC_1,button,1342210048
Control2=IDC_2,button,1342210048
Control3=IDC_3,button,1342210048
Control4=IDC_4,button,1476427776
Control5=IDC_5,button,1342210048
Control6=IDC_6,button,1342210048
Control7=IDC_7,button,1342210048
Control8=IDC_8,button,1342210048
Control9=IDC_9,button,1342210048
Control10=IDC_10,button,1342210048
Control11=IDC_11,button,1342210048
Control12=IDC_12,button,1342210048
Control13=IDC_STATIC,static,1342177294
Control14=IDC_STATIC,static,1342177294
Control15=IDC_STATIC,static,1342177294
Control16=IDC_STATIC,static,1342177294
Control17=IDC_STATIC,static,1342177294
Control18=IDC_STATIC,static,1342177294
Control19=IDC_STATIC,static,1342177294
Control20=IDC_STATIC,static,1342177294
Control21=IDC_STATIC,static,1342177294

[CLS:CPhonePad]
Type=0
HeaderFile=PhonePad.h
ImplementationFile=PhonePad.cpp
BaseClass=CDialog
Filter=D
LastObject=CPhonePad
VirtualFilter=dWC

[DLG:IDD_HIHO]
Type=1
ControlCount=2
Control1=IDC_STATIC,static,1342308352
Control2=IDC_STATIC,static,1342308352

