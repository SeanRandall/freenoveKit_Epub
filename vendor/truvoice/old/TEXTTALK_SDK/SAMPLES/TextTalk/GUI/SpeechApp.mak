# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=SpeechApp - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to SpeechApp - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "SpeechApp - Win32 Release" && "$(CFG)" !=\
 "SpeechApp - Win32 Debug" && "$(CFG)" != "SpeechApp - Win32 StaticRelease"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "SpeechApp.mak" CFG="SpeechApp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SpeechApp - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "SpeechApp - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "SpeechApp - Win32 StaticRelease" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "SpeechApp - Win32 StaticRelease"
RSC=rc.exe
CPP=cl.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "SpeechApp - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
TargetName=SpeechApp
# End Custom Macros

ALL : "$(OUTDIR)\SpeechApp.exe" ".\SpeechApp.EXE"

CLEAN : 
	-@erase ".\SpeechApp.EXE"
	-@erase ".\Release\SpeechApp.exe"
	-@erase ".\Release\ListCtrlExt.obj"
	-@erase ".\Release\SpeechApp.pch"
	-@erase ".\Release\BlurbDlg.obj"
	-@erase ".\Release\SpeechAppDoc.obj"
	-@erase ".\Release\PhonePad.obj"
	-@erase ".\Release\TTSDlg.obj"
	-@erase ".\Release\channel.obj"
	-@erase ".\Release\SpeechAppView.obj"
	-@erase ".\Release\SpeechApp.obj"
	-@erase ".\Release\SAPChannel.obj"
	-@erase ".\Release\StdAfx.obj"
	-@erase ".\Release\SMChannel.obj"
	-@erase ".\Release\MainFrm.obj"
	-@erase ".\Release\SendDlg.obj"
	-@erase ".\Release\SpeechApp.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GR /GX /O2 /I "..\..\..\inc" /D "WIN32" /D "NDEBUG"\
 /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/SpeechApp.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/SpeechApp.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/SpeechApp.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 ..\..\..\lib\libsrlmt.lib ..\..\..\lib\libdxxmt.lib ..\..\..\lib\libfaxmt.lib ..\..\..\lib\texttalk.lib /nologo /subsystem:windows /machine:I386
LINK32_FLAGS=..\..\..\lib\libsrlmt.lib ..\..\..\lib\libdxxmt.lib\
 ..\..\..\lib\libfaxmt.lib ..\..\..\lib\texttalk.lib /nologo /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)/SpeechApp.pdb" /machine:I386\
 /out:"$(OUTDIR)/SpeechApp.exe" 
LINK32_OBJS= \
	"$(INTDIR)/ListCtrlExt.obj" \
	"$(INTDIR)/BlurbDlg.obj" \
	"$(INTDIR)/SpeechAppDoc.obj" \
	"$(INTDIR)/PhonePad.obj" \
	"$(INTDIR)/TTSDlg.obj" \
	"$(INTDIR)/channel.obj" \
	"$(INTDIR)/SpeechAppView.obj" \
	"$(INTDIR)/SpeechApp.obj" \
	"$(INTDIR)/SAPChannel.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/SMChannel.obj" \
	"$(INTDIR)/MainFrm.obj" \
	"$(INTDIR)/SendDlg.obj" \
	"$(INTDIR)/SpeechApp.res"

"$(OUTDIR)\SpeechApp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Copying EXE to my DIR
TargetPath=.\Release\SpeechApp.exe
TargetName=SpeechApp
InputPath=.\Release\SpeechApp.exe
SOURCE=$(InputPath)

"$(TargetName).EXE" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   copy "$(TargetPath)" .

# End Custom Build

!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
TargetName=SpeechApp
# End Custom Macros

ALL : "$(OUTDIR)\SpeechApp.exe" ".\SpeechApp.EXE"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\SpeechApp.pch"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\SpeechApp.ilk"
	-@erase ".\Debug\BlurbDlg.obj"
	-@erase ".\Debug\SAPChannel.obj"
	-@erase ".\Debug\MainFrm.obj"
	-@erase ".\Debug\StdAfx.obj"
	-@erase ".\Debug\SendDlg.obj"
	-@erase ".\Debug\ListCtrlExt.obj"
	-@erase ".\Debug\PhonePad.obj"
	-@erase ".\Debug\TTSDlg.obj"
	-@erase ".\Debug\SpeechAppDoc.obj"
	-@erase ".\Debug\SpeechApp.obj"
	-@erase ".\Debug\SMChannel.obj"
	-@erase ".\Debug\channel.obj"
	-@erase ".\Debug\SpeechAppView.obj"
	-@erase ".\Debug\SpeechApp.res"
	-@erase ".\Debug\SpeechApp.pdb"
	-@erase ".\SpeechApp.EXE"
	-@erase ".\Debug\SpeechApp.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /Zi /Od /I "..\..\..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GR /GX /Zi /Od /I "..\..\..\inc" /D "WIN32" /D\
 "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/SpeechApp.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/SpeechApp.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/SpeechApp.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 ..\..\..\lib\libsrlmt.lib ..\..\..\lib\libdxxmt.lib ..\..\..\lib\libfaxmt.lib ..\..\..\lib\texttalk.lib /nologo /subsystem:windows /debug /machine:I386
LINK32_FLAGS=..\..\..\lib\libsrlmt.lib ..\..\..\lib\libdxxmt.lib\
 ..\..\..\lib\libfaxmt.lib ..\..\..\lib\texttalk.lib /nologo /subsystem:windows\
 /incremental:yes /pdb:"$(OUTDIR)/SpeechApp.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/SpeechApp.exe" 
LINK32_OBJS= \
	"$(INTDIR)/BlurbDlg.obj" \
	"$(INTDIR)/SAPChannel.obj" \
	"$(INTDIR)/MainFrm.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/SendDlg.obj" \
	"$(INTDIR)/ListCtrlExt.obj" \
	"$(INTDIR)/PhonePad.obj" \
	"$(INTDIR)/TTSDlg.obj" \
	"$(INTDIR)/SpeechAppDoc.obj" \
	"$(INTDIR)/SpeechApp.obj" \
	"$(INTDIR)/SMChannel.obj" \
	"$(INTDIR)/channel.obj" \
	"$(INTDIR)/SpeechAppView.obj" \
	"$(INTDIR)/SpeechApp.res"

"$(OUTDIR)\SpeechApp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Copying EXE to my DIR
TargetPath=.\Debug\SpeechApp.exe
TargetName=SpeechApp
InputPath=.\Debug\SpeechApp.exe
SOURCE=$(InputPath)

"$(TargetName).EXE" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   copy "$(TargetPath)" .

# End Custom Build

!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "SpeechAp"
# PROP BASE Intermediate_Dir "SpeechAp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "StaticRelease"
# PROP Intermediate_Dir "StaticRelease"
# PROP Target_Dir ""
OUTDIR=.\StaticRelease
INTDIR=.\StaticRelease
# Begin Custom Macros
TargetName=SpeechApp
# End Custom Macros

ALL : "$(OUTDIR)\SpeechApp.exe" ".\SpeechApp.EXE"

CLEAN : 
	-@erase ".\SpeechApp.EXE"
	-@erase ".\StaticRelease\SpeechApp.exe"
	-@erase ".\StaticRelease\PhonePad.obj"
	-@erase ".\StaticRelease\SpeechApp.pch"
	-@erase ".\StaticRelease\TTSDlg.obj"
	-@erase ".\StaticRelease\StdAfx.obj"
	-@erase ".\StaticRelease\ListCtrlExt.obj"
	-@erase ".\StaticRelease\SpeechApp.obj"
	-@erase ".\StaticRelease\channel.obj"
	-@erase ".\StaticRelease\SpeechAppView.obj"
	-@erase ".\StaticRelease\SAPChannel.obj"
	-@erase ".\StaticRelease\SMChannel.obj"
	-@erase ".\StaticRelease\SpeechAppDoc.obj"
	-@erase ".\StaticRelease\BlurbDlg.obj"
	-@erase ".\StaticRelease\MainFrm.obj"
	-@erase ".\StaticRelease\SendDlg.obj"
	-@erase ".\StaticRelease\SpeechApp.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MT /W3 /GR /GX /O2 /I "..\..\..\inc" /D "WIN32" /D "NDEBUG"\
 /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)/SpeechApp.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\StaticRelease/
CPP_SBRS=
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFX_NO_CFILE_RESOURCES"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/SpeechApp.res" /d "NDEBUG" /d\
 "_AFX_NO_CFILE_RESOURCES" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/SpeechApp.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 libsrlmt.lib libdxxmt.lib libfaxmt.lib texttalk.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ..\..\..\lib\libsrlmt.lib ..\..\..\lib\libdxxmt.lib ..\..\..\lib\libfaxmt.lib ..\..\..\lib\texttalk.lib /nologo /subsystem:windows /machine:I386
LINK32_FLAGS=..\..\..\lib\libsrlmt.lib ..\..\..\lib\libdxxmt.lib\
 ..\..\..\lib\libfaxmt.lib ..\..\..\lib\texttalk.lib /nologo /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)/SpeechApp.pdb" /machine:I386\
 /out:"$(OUTDIR)/SpeechApp.exe" 
LINK32_OBJS= \
	"$(INTDIR)/PhonePad.obj" \
	"$(INTDIR)/TTSDlg.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/ListCtrlExt.obj" \
	"$(INTDIR)/SpeechApp.obj" \
	"$(INTDIR)/channel.obj" \
	"$(INTDIR)/SpeechAppView.obj" \
	"$(INTDIR)/SAPChannel.obj" \
	"$(INTDIR)/SMChannel.obj" \
	"$(INTDIR)/SpeechAppDoc.obj" \
	"$(INTDIR)/BlurbDlg.obj" \
	"$(INTDIR)/MainFrm.obj" \
	"$(INTDIR)/SendDlg.obj" \
	"$(INTDIR)/SpeechApp.res"

"$(OUTDIR)\SpeechApp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Copying EXE to my DIR
TargetPath=.\StaticRelease\SpeechApp.exe
TargetName=SpeechApp
InputPath=.\StaticRelease\SpeechApp.exe
SOURCE=$(InputPath)

"$(TargetName).EXE" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   copy "$(TargetPath)" .

# End Custom Build

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "SpeechApp - Win32 Release"
# Name "SpeechApp - Win32 Debug"
# Name "SpeechApp - Win32 StaticRelease"

!IF  "$(CFG)" == "SpeechApp - Win32 Release"

!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"

!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "SpeechApp - Win32 Release"

!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"

!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SpeechApp.cpp
DEP_CPP_SPEEC=\
	".\StdAfx.h"\
	".\SpeechApp.h"\
	".\MainFrm.h"\
	".\SpeechAppDoc.h"\
	".\SpeechAppView.h"\
	".\SAPChannel.h"\
	".\BlurbDlg.h"\
	".\ListCtrlExt.h"\
	".\SMChannel.h"\
	".\channel.h"\
	".\..\..\..\inc\srllib.h"\
	".\..\..\..\inc\dxxxlib.h"\
	".\..\..\..\inc\faxlib.h"\
	".\..\..\..\INC\srl_eblk.h"\
	".\..\..\..\INC\srltpt.h"\
	".\..\..\..\INC\d40low.h"\
	".\..\..\..\INC\dxdevdef.h"\
	".\..\..\..\INC\dxcallp.h"\
	".\..\..\..\INC\dxdigit.h"\
	".\..\..\..\INC\dxr2mf.h"\
	".\..\..\..\INC\dxsync.h"\
	".\..\..\..\INC\dxtables.h"\
	

!IF  "$(CFG)" == "SpeechApp - Win32 Release"


"$(INTDIR)\SpeechApp.obj" : $(SOURCE) $(DEP_CPP_SPEEC) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"


"$(INTDIR)\SpeechApp.obj" : $(SOURCE) $(DEP_CPP_SPEEC) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"


"$(INTDIR)\SpeechApp.obj" : $(SOURCE) $(DEP_CPP_SPEEC) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "SpeechApp - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W3 /GR /GX /O2 /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/SpeechApp.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\SpeechApp.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /GR /GX /Zi /Od /I "..\..\..\inc" /D "WIN32" /D\
 "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/SpeechApp.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\SpeechApp.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MT /W3 /GR /GX /O2 /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)/SpeechApp.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\SpeechApp.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MainFrm.cpp
DEP_CPP_MAINF=\
	".\MainFrm.h"\
	".\SpeechApp.h"\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "SpeechApp - Win32 Release"


"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"


"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"


"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SpeechAppDoc.cpp
DEP_CPP_SPEECH=\
	".\SpeechApp.h"\
	".\SpeechAppDoc.h"\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "SpeechApp - Win32 Release"


"$(INTDIR)\SpeechAppDoc.obj" : $(SOURCE) $(DEP_CPP_SPEECH) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"


"$(INTDIR)\SpeechAppDoc.obj" : $(SOURCE) $(DEP_CPP_SPEECH) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"


"$(INTDIR)\SpeechAppDoc.obj" : $(SOURCE) $(DEP_CPP_SPEECH) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SpeechAppView.cpp
DEP_CPP_SPEECHA=\
	".\StdAfx.h"\
	".\SpeechApp.h"\
	".\SpeechAppDoc.h"\
	".\SpeechAppView.h"\
	".\SAPChannel.h"\
	".\TTSDlg.h"\
	".\PhonePad.h"\
	".\gettysburg.h"\
	".\ListCtrlExt.h"\
	".\SMChannel.h"\
	".\channel.h"\
	".\..\..\..\inc\srllib.h"\
	".\..\..\..\inc\dxxxlib.h"\
	".\..\..\..\inc\faxlib.h"\
	".\..\..\..\INC\srl_eblk.h"\
	".\..\..\..\INC\srltpt.h"\
	".\..\..\..\INC\d40low.h"\
	".\..\..\..\INC\dxdevdef.h"\
	".\..\..\..\INC\dxcallp.h"\
	".\..\..\..\INC\dxdigit.h"\
	".\..\..\..\INC\dxr2mf.h"\
	".\..\..\..\INC\dxsync.h"\
	".\..\..\..\INC\dxtables.h"\
	

!IF  "$(CFG)" == "SpeechApp - Win32 Release"


"$(INTDIR)\SpeechAppView.obj" : $(SOURCE) $(DEP_CPP_SPEECHA) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"


"$(INTDIR)\SpeechAppView.obj" : $(SOURCE) $(DEP_CPP_SPEECHA) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"


"$(INTDIR)\SpeechAppView.obj" : $(SOURCE) $(DEP_CPP_SPEECHA) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SpeechApp.rc
DEP_RSC_SPEECHAP=\
	".\Res\Bitmap1.bmp"\
	".\res\bitmap2.bmp"\
	".\Res\bmp00001.bmp"\
	".\Res\bmp00002.bmp"\
	".\Res\bmp00003.bmp"\
	".\Res\bmp00004.bmp"\
	".\Res\bmp00005.bmp"\
	".\Res\bmp00006.bmp"\
	".\Res\bmp00007.bmp"\
	".\Res\bmp00008.bmp"\
	".\Res\bmp00009.bmp"\
	".\res\Crdfle13.ico"\
	".\Res\Cursor1.cur"\
	".\Res\Dialogic.ico"\
	".\res\doug.ico"\
	".\Res\SpeechApp.rc2"\
	".\Res\SpeechAppDoc.ico"\
	".\Res\speed.bmp"\
	".\Res\Toolbar.bmp"\
	

!IF  "$(CFG)" == "SpeechApp - Win32 Release"


"$(INTDIR)\SpeechApp.res" : $(SOURCE) $(DEP_RSC_SPEECHAP) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"


"$(INTDIR)\SpeechApp.res" : $(SOURCE) $(DEP_RSC_SPEECHAP) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"


"$(INTDIR)\SpeechApp.res" : $(SOURCE) $(DEP_RSC_SPEECHAP) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\channel.cpp
DEP_CPP_CHANN=\
	".\StdAfx.h"\
	".\channel.h"\
	".\..\..\..\inc\srllib.h"\
	".\..\..\..\inc\dxxxlib.h"\
	".\..\..\..\inc\faxlib.h"\
	".\..\..\..\INC\srl_eblk.h"\
	".\..\..\..\INC\srltpt.h"\
	".\..\..\..\INC\d40low.h"\
	".\..\..\..\INC\dxdevdef.h"\
	".\..\..\..\INC\dxcallp.h"\
	".\..\..\..\INC\dxdigit.h"\
	".\..\..\..\INC\dxr2mf.h"\
	".\..\..\..\INC\dxsync.h"\
	".\..\..\..\INC\dxtables.h"\
	

!IF  "$(CFG)" == "SpeechApp - Win32 Release"


"$(INTDIR)\channel.obj" : $(SOURCE) $(DEP_CPP_CHANN) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"


"$(INTDIR)\channel.obj" : $(SOURCE) $(DEP_CPP_CHANN) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"


"$(INTDIR)\channel.obj" : $(SOURCE) $(DEP_CPP_CHANN) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SAPChannel.cpp
DEP_CPP_SAPCH=\
	".\StdAfx.h"\
	".\SAPChannel.h"\
	".\..\..\..\inc\texttalk.h"\
	".\SmartPtr.h"\
	".\SMChannel.h"\
	".\channel.h"\
	".\..\..\..\inc\srllib.h"\
	".\..\..\..\inc\dxxxlib.h"\
	".\..\..\..\inc\faxlib.h"\
	".\..\..\..\INC\srl_eblk.h"\
	".\..\..\..\INC\srltpt.h"\
	".\..\..\..\INC\d40low.h"\
	".\..\..\..\INC\dxdevdef.h"\
	".\..\..\..\INC\dxcallp.h"\
	".\..\..\..\INC\dxdigit.h"\
	".\..\..\..\INC\dxr2mf.h"\
	".\..\..\..\INC\dxsync.h"\
	".\..\..\..\INC\dxtables.h"\
	

!IF  "$(CFG)" == "SpeechApp - Win32 Release"


"$(INTDIR)\SAPChannel.obj" : $(SOURCE) $(DEP_CPP_SAPCH) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"


"$(INTDIR)\SAPChannel.obj" : $(SOURCE) $(DEP_CPP_SAPCH) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"


"$(INTDIR)\SAPChannel.obj" : $(SOURCE) $(DEP_CPP_SAPCH) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SMChannel.cpp
DEP_CPP_SMCHA=\
	".\StdAfx.h"\
	".\SMChannel.h"\
	".\channel.h"\
	".\..\..\..\inc\srllib.h"\
	".\..\..\..\inc\dxxxlib.h"\
	".\..\..\..\inc\faxlib.h"\
	".\..\..\..\INC\srl_eblk.h"\
	".\..\..\..\INC\srltpt.h"\
	".\..\..\..\INC\d40low.h"\
	".\..\..\..\INC\dxdevdef.h"\
	".\..\..\..\INC\dxcallp.h"\
	".\..\..\..\INC\dxdigit.h"\
	".\..\..\..\INC\dxr2mf.h"\
	".\..\..\..\INC\dxsync.h"\
	".\..\..\..\INC\dxtables.h"\
	

!IF  "$(CFG)" == "SpeechApp - Win32 Release"


"$(INTDIR)\SMChannel.obj" : $(SOURCE) $(DEP_CPP_SMCHA) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"


"$(INTDIR)\SMChannel.obj" : $(SOURCE) $(DEP_CPP_SMCHA) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"


"$(INTDIR)\SMChannel.obj" : $(SOURCE) $(DEP_CPP_SMCHA) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BlurbDlg.cpp
DEP_CPP_BLURB=\
	".\BlurbDlg.h"\
	".\SpeechApp.h"\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "SpeechApp - Win32 Release"


"$(INTDIR)\BlurbDlg.obj" : $(SOURCE) $(DEP_CPP_BLURB) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"


"$(INTDIR)\BlurbDlg.obj" : $(SOURCE) $(DEP_CPP_BLURB) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"


"$(INTDIR)\BlurbDlg.obj" : $(SOURCE) $(DEP_CPP_BLURB) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ListCtrlExt.cpp
DEP_CPP_LISTC=\
	".\ListCtrlExt.h"\
	".\SpeechApp.h"\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "SpeechApp - Win32 Release"


"$(INTDIR)\ListCtrlExt.obj" : $(SOURCE) $(DEP_CPP_LISTC) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"


"$(INTDIR)\ListCtrlExt.obj" : $(SOURCE) $(DEP_CPP_LISTC) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"


"$(INTDIR)\ListCtrlExt.obj" : $(SOURCE) $(DEP_CPP_LISTC) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\TTSDlg.cpp
DEP_CPP_TTSDL=\
	".\SpeechApp.h"\
	".\StdAfx.h"\
	".\TTSDlg.h"\
	

!IF  "$(CFG)" == "SpeechApp - Win32 Release"


"$(INTDIR)\TTSDlg.obj" : $(SOURCE) $(DEP_CPP_TTSDL) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"


"$(INTDIR)\TTSDlg.obj" : $(SOURCE) $(DEP_CPP_TTSDL) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"


"$(INTDIR)\TTSDlg.obj" : $(SOURCE) $(DEP_CPP_TTSDL) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SendDlg.cpp
DEP_CPP_SENDD=\
	".\SendDlg.h"\
	".\SpeechApp.h"\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "SpeechApp - Win32 Release"


"$(INTDIR)\SendDlg.obj" : $(SOURCE) $(DEP_CPP_SENDD) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"


"$(INTDIR)\SendDlg.obj" : $(SOURCE) $(DEP_CPP_SENDD) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"


"$(INTDIR)\SendDlg.obj" : $(SOURCE) $(DEP_CPP_SENDD) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PhonePad.cpp
DEP_CPP_PHONE=\
	".\PhonePad.h"\
	".\SpeechApp.h"\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "SpeechApp - Win32 Release"


"$(INTDIR)\PhonePad.obj" : $(SOURCE) $(DEP_CPP_PHONE) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 Debug"


"$(INTDIR)\PhonePad.obj" : $(SOURCE) $(DEP_CPP_PHONE) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ELSEIF  "$(CFG)" == "SpeechApp - Win32 StaticRelease"


"$(INTDIR)\PhonePad.obj" : $(SOURCE) $(DEP_CPP_PHONE) "$(INTDIR)"\
 "$(INTDIR)\SpeechApp.pch"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
################################################################################
# Section SpeechApp : {0C2DD025-FCE2-11CF-AB9D-0000C049A1B3}
# 	0:11:channel.cpp:D:\winx\SpeechApp\channel.cpp
# 	0:9:channel.h:D:\winx\SpeechApp\channel.h
# 	2:11:channel.cpp:channel.cpp
# 	2:15:CLASS: CChannel:CChannel
# 	2:19:Application Include:SpeechApp.h
# 	2:9:channel.h:channel.h
# End Section
################################################################################
################################################################################
# Section SpeechApp : {0C2DD027-FCE2-11CF-AB9D-0000C049A1B3}
# 	0:11:channel.cpp:D:\winx\SpeechApp\channel.cpp
# 	0:9:channel.h:D:\winx\SpeechApp\channel.h
# 	2:11:channel.cpp:channel.cpp
# 	2:15:CLASS: CChannel:CChannel
# 	2:19:Application Include:SpeechApp.h
# 	2:9:channel.h:channel.h
# End Section
################################################################################
################################################################################
# Section SpeechApp : {8BC1A240-01A5-11D0-ABA0-0000C049A1B3}
# 	0:13:ListCtrlExt.h:D:\winx\SpeechApp\ListCtrlExt.h
# 	0:15:ListCtrlExt.cpp:D:\winx\SpeechApp\ListCtrlExt.cpp
# 	2:26:CLASS: CListCtrlDropTarget:CListCtrlDropTarget
# 	2:13:ListCtrlExt.h:ListCtrlExt.h
# 	2:19:CLASS: CListCtrlExt:CListCtrlExt
# 	2:19:Application Include:SpeechApp.h
# 	2:15:ListCtrlExt.cpp:ListCtrlExt.cpp
# End Section
################################################################################
