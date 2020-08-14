@setlocal
@set TMPPRJ=polyView
@set TMP3RD=D:\Projects\3rdParty.x64
@set TMPINST=D:\UTILS\TEMP
@set TMPSRC=..
@if NOT EXIST %TMPSRC%\CMakeLists.txt goto NOCM
@set DOINST=1

@set TMPOPTS=
@REM #######################################
@REM *** ADJUST TO SUIT YOUR ENVIRONMENT ***
@set TMPOPTS=%TMPOPTS% -DCMAKE_INSTALL_PREFIX:PATH=%TMPINST%
@set TMPOPTS=%TMPOPTS% -DCMAKE_PREFIX_PATH:PATH=%TMP3RD%
@REM #######################################

@call chkmsvc %TMPPRJ%
@call setupqt5
@if ERRORLEVEL 1 goto NOQT

@set TMPLOG=bldlog-1.txt
@REM Uncomment if multiple build logs required
@REM call cycbldlog

@echo Building %TMPPRJ% source: [%TMPSRC%] %DATE% %TIME% output to %TMPLOG%
@echo Building %TMPPRJ% source: [%TMPSRC%] %DATE% %TIME% > %TMPLOG%

cmake %TMPSRC% %TMPOPTS% >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR1

cmake --build . --config Debug >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR2

@REM if these are also required
@REM cmake --build . --config RelWithDebInfo >> %TMPLOG% 2>&1
@REM if ERRORLEVEL 1 goto ERR4

@REM cmake --build . --config MinSizeRel >> %TMPLOG% 2>&1
@REM if ERRORLEVEL 1 goto ERR5

cmake --build . --config Release >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR3
:DONEREL

@echo.
@echo Appear successful...  >> %TMPLOG% 2>&1
@if %DOINST% EQU 1 goto ADD_INST
@echo Appear successful... NO INSTALL configured...
@echo.
@goto END

:ADD_INST
@echo Continue with install to %TMPINST%? Only Ctrl+C aborts...
@echo.
@pause

cmake --build . --config Release --target INSTALL >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR6

@echo Done install

@goto END




:NOCM
@echo ERROR: Can NOT locate %TMPSRC%\CMakeLists.txt! WHERE IS IT! FIX ME!!!
@goto ISERR

:ERR1
@echo cmake config gen ERROR!
@goto ISERR

:ERR2
@echo cmake build Debug ERROR!
@goto ISERR

:ERR3
@echo ERROR: Cmake build Release FAILED!
@goto ISERR

:ERR4
@echo cmake build RelWithDebInfo ERROR!
@goto ISERR

:ERR5
@echo cmake build MinSizeRel ERROR!
@goto ISERR

:NOQT
@echo Error in setupqt5.bat! *** FIX ME ***
@goto ISERR

:ISERR
@endlocal
@exit /b 1

:END
@endlocal
@exit /b 0

@REM eof
