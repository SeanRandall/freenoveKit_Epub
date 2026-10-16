# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=TTSDEMO - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to TTSDEMO - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "TTSDEMO - Win32 Release" && "$(CFG)" !=\
 "TTSDEMO - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "TTSDEMO.mak" CFG="TTSDEMO - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TTSDEMO - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "TTSDEMO - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Target_Last_Scanned "TTSDEMO - Win32 Debug"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TTSDEMO - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\TTSDEMO.exe"

CLEAN : 
	-@erase "$(INTDIR)\TTSDEMO.OBJ"
	-@erase "$(OUTDIR)\TTSDEMO.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/TTSDEMO.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/TTSDEMO.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 c:/dlgcdev/dialogic/lib/libsrlmt.lib c:/dlgcdev/dialogic/lib/libdxxmt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=c:/dlgcdev/dialogic/lib/libsrlmt.lib\
 c:/dlgcdev/dialogic/lib/libdxxmt.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/TTSDEMO.pdb" /machine:I386 /out:"$(OUTDIR)/TTSDEMO.exe" 
LINK32_OBJS= \
	"$(INTDIR)\TTSDEMO.OBJ" \
	"..\TTSSOFT\Release\TEXTTALK.lib"

"$(OUTDIR)\TTSDEMO.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "TTSDEMO - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\TTSDEMO.exe"

CLEAN : 
	-@erase "$(INTDIR)\TTSDEMO.OBJ"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\TTSDEMO.exe"
	-@erase "$(OUTDIR)\TTSDEMO.ilk"
	-@erase "$(OUTDIR)\TTSDEMO.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/TTSDEMO.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/TTSDEMO.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 c:/dlgcdev/dialogic/lib/libsrlmt.lib c:/dlgcdev/dialogic/lib/libdxxmt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=c:/dlgcdev/dialogic/lib/libsrlmt.lib\
 c:/dlgcdev/dialogic/lib/libdxxmt.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/TTSDEMO.pdb" /debug /machine:I386 /out:"$(OUTDIR)/TTSDEMO.exe" 
LINK32_OBJS= \
	"$(INTDIR)\TTSDEMO.OBJ" \
	"..\TTSSOFT\Release\TEXTTALK.lib"

"$(OUTDIR)\TTSDEMO.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

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

# Name "TTSDEMO - Win32 Release"
# Name "TTSDEMO - Win32 Debug"

!IF  "$(CFG)" == "TTSDEMO - Win32 Release"

!ELSEIF  "$(CFG)" == "TTSDEMO - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\TTSDEMO.CPP
DEP_CPP_TTSDE=\
	".\..\..\..\..\DLGCDEV\Dialogic\INC\d40low.h"\
	".\..\..\..\..\DLGCDEV\Dialogic\INC\dxcallp.h"\
	".\..\..\..\..\DLGCDEV\Dialogic\INC\dxdevdef.h"\
	".\..\..\..\..\DLGCDEV\Dialogic\INC\dxdigit.h"\
	".\..\..\..\..\DLGCDEV\Dialogic\INC\dxr2mf.h"\
	".\..\..\..\..\DLGCDEV\Dialogic\INC\dxsync.h"\
	".\..\..\..\..\DLGCDEV\Dialogic\INC\dxtables.h"\
	".\..\..\..\..\DLGCDEV\Dialogic\INC\srl_eblk.h"\
	".\..\..\..\..\DLGCDEV\Dialogic\INC\srltpt.h"\
	".\texttalk.h"\
	{$(INCLUDE)}"\dxxxlib.h"\
	{$(INCLUDE)}"\srllib.h"\
	

"$(INTDIR)\TTSDEMO.OBJ" : $(SOURCE) $(DEP_CPP_TTSDE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=\USERS\DIALOGIC\SAMPLES\TTSSOFT\Release\TEXTTALK.lib

!IF  "$(CFG)" == "TTSDEMO - Win32 Release"

!ELSEIF  "$(CFG)" == "TTSDEMO - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
