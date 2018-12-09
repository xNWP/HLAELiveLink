@ECHO OFF
SETLOCAL
CLS

CALL copy_dlls.bat

SET ArchName="HLAELiveLink_%1_%2"

ECHO Gathering Files For Archiving.

MKDIR btemp\HLAELiveLink
XCOPY res btemp\HLAELiveLink\res /I /E
XCOPY hlaelivelink.xdl64 btemp\HLAELiveLink /I
XCOPY package\*.* btemp

ECHO Deleting Symbol/Debug Files.
CD btemp\HLAELiveLink\res\libs\win64
DEL /F /Q *.gitignore *.exp *.ilk *.pdb *.lib

ECHO Packaging All.
CD ..\..\..\..

7z a -tzip %ArchName%.zip *.* HLAELiveLink
MOVE %ArchName%.zip ..\%ArchName%.zip
7z a -t7z %ArchName%.7z *.* HLAELiveLink
MOVE %ArchName%.7z ..\%ArchName%.7z

ECHO Cleaning Up Temp Files.
CD ..
RMDIR /S /Q btemp

ECHO Done.