@ECHO OFF
SETLOCAL EnableDelayedExpansion

SET title=HLAELiveLink
SET packageFileList=README.txt LICENSE
SET pluginFileList=hlaelivelink.xdl64 res
SET sevZip="C:\Program Files\7-Zip\7z.exe"
SET pluginRoot=plugins\HLAELiveLink\

ECHO Deleting existing archives...
IF EXIST *.7z DEL /P *.7z
IF EXIST *.zip DEL /P *.zip

SET /P versionMajor="Version Major: "
SET /P versionMinor="Version Minor: "
SET archName=%title% v%versionMajor%.%versionMinor%

MKDIR .\buildTemp\
FOR %%x IN (%packageFileList%) DO (
    XCOPY %%~fx .\buildTemp\ > nul
)

SET fullPluginFileList=
FOR %%x in (%pluginFileList%) DO (
    SET fullPluginFileList=!fullPluginFileList! %pluginRoot%%%x
)

MKDIR .\buildTemp\HLAELiveLink\
FOR %%x IN (%fullPluginFileList%) DO (
    IF EXIST %%~fx\* (
        MKDIR .\buildTemp\HLAELiveLink\%%~nx
        XCOPY %%~fx\* .\buildTemp\HLAELiveLink\%%~nx\ /E > nul
    ) ELSE (
        XCOPY %%~fx .\buildTemp\HLAELiveLink\ > nul
    )
)

ECHO.
DIR /B /S buildTemp
SET /P bCheck="Does this look right (Y/N)? "
IF /I NOT %bCheck%==y ( 
    ECHO Cleaning up...
    RMDIR /S /Q buildTemp
    EXIT /B
)

CD buildTemp
%sevZip% a -tzip "%archName%.zip" * > nul
MOVE "%archName%.zip" .. > nul
%sevZip% a -t7z "%archName%.7z" * > nul
MOVE "%archName%.7z" .. > nul

CD ..
RMDIR /S /Q buildTemp

ECHO Done.