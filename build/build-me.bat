@setlocal
@set TMPPRJ=polyView
@echo %TMPPRJ% - Uses Qt5.1 64-bits
@set VCVERS=14
@set TMPLOG=bldlog-1.txt
@set DOTINST=0
@set DOINSTALL=0
@set DOINSTREL=1
@set BLDDIR=%CD%

@echo Doing build output to %TMPLOG%
@echo Doing build output to %TMPLOG% > %TMPLOG%

@if "%Platform%x" == "x64x" goto DNMSVC

@set SET_BAT=%ProgramFiles(x86)%\Microsoft Visual Studio %VCVERS%.0\VC\vcvarsall.bat
@if NOT EXIST "%SET_BAT%" goto NOBAT
@echo Doing: 'call "%SET_BAT%" AMD64'
@echo Doing: 'call "%SET_BAT%" AMD64' >> %TMPLOG%
@call "%SET_BAT%" AMD64 >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR0

:DNMSVC
@cd %BLDDIR%

@REM Setup Qt5 - 64-bit
@call setupqt5.6
@if EXIST tempsp.bat @del tempsp.bat
@REM call setupqt64
@call setupqt564 -N
@if NOT EXIST tempsp.bat goto NOSP
@call tempsp
@if "%Qt5_DIR%x" == "x" goto NOQT5

@call chkpath C:\Qt\4.8.6\bin
@if ERRORLEVEL 1 goto BADPATH

@set TMPSRC=..
@set TMPBGN=%TIME%
@set TMPINS=C:\MDOS
@set TMPCM=%TMPSRC%\CMakeLists.txt
@set DOPAUSE=pause

@call chkmsvc %TMPPRJ% 

@if EXIST build-cmake.bat (
@call build-cmake
)

@if NOT EXIST %TMPCM% goto NOCM

@set TMPOPTS=-DCMAKE_INSTALL_PREFIX=%TMPINS% -G "Visual Studio %VCVERS% Win64"
@set TMPOPTS=%TMPOPTS% -DCMAKE_PREFIX_PATH=%Qt5_DIR%

:RPT
@if "%~1x" == "x" goto GOTCMD
@set TMPOPTS=%TMPOPTS% %1
@shift
@goto RPT
:GOTCMD

@echo Build start %DATE% %TIME% >> %TMPLOG%
@echo Doing: 'cmake %TMPSRC% %TMPOPTS%'
@echo Doing: cmake %TMPSRC% %TMPOPTS% >> %TMPLOG% 2>&1
@cmake %TMPSRC% %TMPOPTS% >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR1

@set TMPDBG=Debug
@REM set TMPDBG=RelWithDebInfo

@echo Doing: 'cmake --build . --config %TMPDBG%' >> %TMPLOG% 2>&1
@echo Doing: 'cmake --build . --config %TMPDBG%'
cmake --build . --config %TMPDBG% >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR2

@echo Doing: 'cmake --build . --config Release'
@echo Doing: 'cmake --build . --config Release' >> %TMPLOG% 2>&1
cmake --build . --config Release >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR3
:DONEREL

@REM fa4 "***" %TMPLOG%
@call elapsed %TMPBGN%
@echo Appears a successful build... see %TMPLOG%
@echo.
@if "%DOINSTALL%x" == "0x" (
@echo Skipping install for now... Set DOINSTALL=1
@if EXIST updexe.bat (
@echo The updexe.bat **MAY** work for you... check it out...
)
@echo.
@goto END
)

@if %DOINSTREL% EQU 1 (
@echo Continue with Release install to %TMPINS%? Only Ctrl+c aborts...
@%DOPAUSE%
@goto DOINSREL
)

@echo Continue with install to %TMPINS%? Only Ctrl+c aborts...
@%DOPAUSE%
cmake --build . --config %TMPDBG%  --target INSTALL >> %TMPLOG% 2>&1

:DOINSREL

@echo Doing: 'cmake --build . --config Release  --target INSTALL'
cmake --build . --config Release  --target INSTALL >> %TMPLOG% 2>&1

@fa4 " -- " %TMPLOG%

@call elapsed %TMPBGN%
@echo All done... see %TMPLOG%

@goto END

:NOBAT
@echo Can NOT locate MSVC setup batch "%SET_BAT%"! *** FIX ME ***
@goto ISERR

:NOQT5
@echo Can NOT locate Qt5_DIR in ENVIRONMENT! *** FIX ME ***
@goto ISERR

:ERR0
@echo Failed to setup 64-bit MSVC environment! *** FIX ME ***
@goto ISERR

:NOCM
@echo Error: Can NOT locate %TMPCM%
@goto ISERR

:ERR1
@echo cmake configuration or generations ERROR
@goto ISERR

:ERR2
@echo ERROR: Cmake build Debug FAILED!
@goto ISERR

:ERR3
@fa4 "mt.exe : general error c101008d:" %TMPLOG% >nul
@if ERRORLEVEL 1 goto ERR33
:ERR34
@echo ERROR: Cmake build Release FAILED!
@goto ISERR
:ERR33
@echo Try again due to this STUPID STUPID STUPID error
@echo Try again due to this STUPID STUPID STUPID error >>%TMPLOG%
cmake --build . --config Release >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR34
@goto DONEREL

:BADPATH
@echo Error: Found C:\Qt\4.8.6\bin in the PATH! This is Qt4! *** FIX ME ***
@echo Try running setuppath.bat, after you have set the Qt5_DIR ENV variable to point to Qt5 root,
@echo like setupqt5.6.bat ...
@goto ISERR

:ISERR
@echo See %TMPLOG% for details...
@endlocal
@exit /b 1

:END
@endlocal
@exit /b 0

@REM eof
