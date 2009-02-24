# Microsoft Developer Studio Project File - Name="LibIO" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=LibIO - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "LibIO.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "LibIO.mak" CFG="LibIO - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LibIO - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe
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
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\..\Tools" /I "..\..\..\..\base\include" /I "..\..\..\..\base\include\os\WIN32" /I "..\..\..\..\base\src\ca" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ca.lib Com.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\..\base\lib\WIN32"
# Begin Target

# Name "LibIO - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ArchiveException.cpp
# End Source File
# Begin Source File

SOURCE=.\ArchiveI.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\ArgParser.cpp
# End Source File
# Begin Source File

SOURCE=.\BinArchive.cpp
# End Source File
# Begin Source File

SOURCE=.\BinChannel.cpp
# End Source File
# Begin Source File

SOURCE=.\BinChannelIterator.cpp
# End Source File
# Begin Source File

SOURCE=.\BinCtrlInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\BinValue.cpp
# End Source File
# Begin Source File

SOURCE=.\BinValueIterator.cpp
# End Source File
# Begin Source File

SOURCE=.\ChannelI.cpp
# End Source File
# Begin Source File

SOURCE=.\ChannelIteratorI.cpp
# End Source File
# Begin Source File

SOURCE=.\CtrlInfoI.cpp
# End Source File
# Begin Source File

SOURCE=.\DataFile.cpp
# End Source File
# Begin Source File

SOURCE=.\DirectoryFile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\FilenameTool.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\GenericException.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\gnu_regex.c
# End Source File
# Begin Source File

SOURCE=.\HashTable.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\MsgLogger.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\osiTimeHelper.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\RegularExpression.cpp
# End Source File
# Begin Source File

SOURCE=.\test.cpp
# End Source File
# Begin Source File

SOURCE=.\ValueI.cpp
# End Source File
# Begin Source File

SOURCE=.\ValueIteratorI.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ArchiveI.h
# End Source File
# Begin Source File

SOURCE=.\BinArchive.h
# End Source File
# Begin Source File

SOURCE=.\BinChannel.h
# End Source File
# Begin Source File

SOURCE=.\BinChannelIterator.h
# End Source File
# Begin Source File

SOURCE=.\BinCtrlInfo.h
# End Source File
# Begin Source File

SOURCE=.\BinValue.h
# End Source File
# Begin Source File

SOURCE=.\BinValueIterator.h
# End Source File
# Begin Source File

SOURCE=.\ChannelI.h
# End Source File
# Begin Source File

SOURCE=.\ChannelIteratorI.h
# End Source File
# Begin Source File

SOURCE=.\CtrlInfoI.h
# End Source File
# Begin Source File

SOURCE=.\DataFile.h
# End Source File
# Begin Source File

SOURCE=.\ValueI.h
# End Source File
# Begin Source File

SOURCE=.\ValueIteratorI.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
