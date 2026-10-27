@echo off
rem ===========================================================================
rem  jlink_upload.bat - Carga un .hex en el CH579 con SEGGER J-Link (SWD)
rem  Uso: jlink_upload.bat <carpeta_tool> <archivo.hex> <dispositivo> <kHz>
rem
rem  Busca JLink.exe en este orden:
rem    1. Variable de entorno JLINK_PATH (carpeta que contiene JLink.exe)
rem    2. C:\Program Files\SEGGER\JLink\JLink.exe
rem    3. La version mas reciente de C:\Program Files\SEGGER\JLink_V*\
rem    4. Lo mismo en C:\Program Files (x86)\SEGGER\
rem ===========================================================================
setlocal

set "TOOLDIR=%~1"
set "HEXFILE=%~2"
set "DEVICE=%~3"
set "SPEED=%~4"
if "%DEVICE%"=="" set "DEVICE=CH579M"
if "%SPEED%"=="" set "SPEED=4000"

if not exist "%HEXFILE%" (
  echo [CH579] No se encuentra el archivo a cargar: "%HEXFILE%"
  exit /b 1
)

rem --- 1. Definicion del dispositivo para J-Link (XML + flash loader) --------
set "DEVDIR=%APPDATA%\SEGGER\JLinkDevices\WCH\%DEVICE%"
if not exist "%DEVDIR%" mkdir "%DEVDIR%"
copy /Y "%TOOLDIR%\CH579M\JLinkDevices.xml" "%DEVDIR%\JLinkDevices.xml" >nul
copy /Y "%TOOLDIR%\CH579M\CH57x.FLM" "%DEVDIR%\CH57x.FLM" >nul

rem --- 2. Buscar JLink.exe ---------------------------------------------------
set "JLINK="
set "PF64=%ProgramFiles%"
set "PF86=%ProgramFiles(x86)%"
if defined JLINK_PATH if exist "%JLINK_PATH%\JLink.exe" set "JLINK=%JLINK_PATH%\JLink.exe"
if not defined JLINK if exist "%PF64%\SEGGER\JLink\JLink.exe" set "JLINK=%PF64%\SEGGER\JLink\JLink.exe"
if not defined JLINK for /d %%D in ("%PF64%\SEGGER\JLink_V*") do if exist "%%~D\JLink.exe" set "JLINK=%%~D\JLink.exe"
if not defined JLINK if exist "%PF86%\SEGGER\JLink\JLink.exe" set "JLINK=%PF86%\SEGGER\JLink\JLink.exe"
if not defined JLINK for /d %%D in ("%PF86%\SEGGER\JLink_V*") do if exist "%%~D\JLink.exe" set "JLINK=%%~D\JLink.exe"

if not defined JLINK (
  echo [CH579] No se encontro JLink.exe.
  echo [CH579] Instale "J-Link Software and Documentation Pack" de SEGGER, o
  echo [CH579] defina la variable de entorno JLINK_PATH con la carpeta de JLink.exe
  exit /b 1
)
echo [CH579] J-Link: "%JLINK%"
echo [CH579] Dispositivo %DEVICE%, SWD a %SPEED% kHz

rem --- 3. Script de comandos de J-Link Commander -----------------------------
set "SCRIPT=%TEMP%\ch579_jlink_upload.jlink"
> "%SCRIPT%" echo r
>>"%SCRIPT%" echo h
>>"%SCRIPT%" echo loadfile "%HEXFILE%"
>>"%SCRIPT%" echo r
>>"%SCRIPT%" echo g
>>"%SCRIPT%" echo exit

rem --- 4. Cargar ---------------------------------------------------------------
"%JLINK%" -device %DEVICE% -if SWD -speed %SPEED% -autoconnect 1 -NoGui 1 -ExitOnError 1 -CommanderScript "%SCRIPT%"
set "RC=%ERRORLEVEL%"
del "%SCRIPT%" >nul 2>&1

if not "%RC%"=="0" (
  echo [CH579] La carga fallo ^(codigo %RC%^). Revise el cableado SWD: SWDIO=PB16, SWCLK=PB17, GND y VTref a 3V3.
  exit /b %RC%
)
echo [CH579] Carga completada.
exit /b 0
