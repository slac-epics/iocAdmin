# Microsoft Developer Studio Project File - Name="ChannelArchiveLibrary" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ChannelArchiveLibrary - Win32 FinalCheck
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ChannelArchiveLibrary.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ChannelArchiveLibrary.mak" CFG="ChannelArchiveLibrary - Win32 FinalCheck"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ChannelArchiveLibrary - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ChannelArchiveLibrary - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "ChannelArchiveLibrary - Win32 FinalCheck" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ChannelArchiveLibrary - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\Library" /I "..\..\base\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "EPICS_DLL" /D __STDC__=0 /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Com.lib ca.lib /nologo /subsystem:console /machine:I386 /libpath:"..\..\base\lib\Win32"

!ELSEIF  "$(CFG)" == "ChannelArchiveLibrary - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "..\Library" /I "..\..\base\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "EPICS_DLL" /D __STDC__=0 /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Com.lib ca.lib /nologo /subsystem:console /profile /map /debug /machine:I386 /libpath:"..\..\base\lib\Win32"

!ELSEIF  "$(CFG)" == "ChannelArchiveLibrary - Win32 FinalCheck"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ChannelArchiveLibrary___Win32_FinalCheck0"
# PROP BASE Intermediate_Dir "ChannelArchiveLibrary___Win32_FinalCheck0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ChannelArchiveLibrary___Win32_FinalCheck0"
# PROP Intermediate_Dir "ChannelArchiveLibrary___Win32_FinalCheck0"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\Library" /I "..\..\base\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "EPICS_DLL" /D __STDC__=0 /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\Library" /I "..\..\base\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "EPICS_DLL" /D __STDC__=0 /YX /FD /GZ /NMbcOn /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Com.lib ca.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\..\base\lib\Win32"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Com.lib ca.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\..\base\lib\Win32"

!ENDIF 

# Begin Target

# Name "ChannelArchiveLibrary - Win32 Release"
# Name "ChannelArchiveLibrary - Win32 Debug"
# Name "ChannelArchiveLibrary - Win32 FinalCheck"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Archive.cpp
# End Source File
# Begin Source File

SOURCE=.\ArchiveException.cpp
# End Source File
# Begin Source File

SOURCE=..\Library\Benchmark.cpp
# End Source File
# Begin Source File

SOURCE=.\DataFile.cpp
# End Source File
# Begin Source File

SOURCE=.\DirectoryFile.cpp
# End Source File
# Begin Source File

SOURCE=..\Library\FilenameTool.cpp
# End Source File
# Begin Source File

SOURCE=..\Library\GenericException.cpp
# End Source File
# Begin Source File

SOURCE=.\HashTable.cpp
# End Source File
# Begin Source File

SOURCE=..\Library\MsgLogger.cpp
# End Source File
# Begin Source File

SOURCE=.\test.cpp
# End Source File
# Begin Source File

SOURCE=..\Library\Timer.cpp
# End Source File
# Begin Source File

SOURCE=.\Value.cpp
# End Source File
# Begin Source File

SOURCE=.\ValueIterator.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Archive.h
# End Source File
# Begin Source File

SOURCE=.\ArchiveException.h
# End Source File
# Begin Source File

SOURCE=.\ArchiveTypes.h
# End Source File
# Begin Source File

SOURCE=..\Library\Benchmark.h
# End Source File
# Begin Source File

SOURCE=.\DataFile.h
# End Source File
# Begin Source File

SOURCE=.\DirectoryFile.h
# End Source File
# Begin Source File

SOURCE=..\Library\FilenameTool.h
# End Source File
# Begin Source File

SOURCE=..\Library\GenericException.h
# End Source File
# Begin Source File

SOURCE=.\HashTable.h
# End Source File
# Begin Source File

SOURCE=..\Library\MsgLogger.h
# End Source File
# Begin Source File

SOURCE=..\Library\Timer.h
# End Source File
# Begin Source File

SOURCE=.\Value.h
# End Source File
# Begin Source File

SOURCE=.\ValueIterator.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\types.txt
# End Source File
# End Target
# End Project
