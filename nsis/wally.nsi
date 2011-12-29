; wally.nsi

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"


;--------------------------------

XPStyle on
SetCompressor lzma

VIAddVersionKey "ProductName" "Wally"
VIAddVersionKey "Comments" "A Qt4 desktop wallpaper changer"
VIAddVersionKey "CompanyName" "BeCrux"
VIAddVersionKey "LegalTrademarks" ""
VIAddVersionKey "LegalCopyright" ""
VIAddVersionKey "FileDescription" "Wally Installer"
VIAddVersionKey "FileVersion" "2.4.4"

VIProductVersion "2.4.4.0"

; The name of the installer
Name "Wally 2.4.4"

; The file to write
OutFile "WallySetup.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Wally

; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\BeCrux\NSIS\Wally" "Install Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel none

;--------------------------------
;Variables

  Var StartMenuFolder

;--------------------------------
;Interface Configuration

  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\nsis.bmp" ; optional
  !define MUI_ABORTWARNING

;--------------------------------
; Pages
  !insertmacro MUI_PAGE_WELCOME
    !define MUI_LICENSEPAGE_RADIOBUTTONS
    !define MUI_LICENSEPAGE_RADIOBUTTONS_TEXT_ACCEPT "I accept this license && disclaimer"
    !define MUI_LICENSEPAGE_RADIOBUTTONS_TEXT_DECLINE "I decline this license && disclaimer"
  !insertmacro MUI_PAGE_LICENSE "License_Disclaimer.txt"
  !insertmacro MUI_PAGE_DIRECTORY

;--------------------------------

  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

;--------------------------------
  !insertmacro MUI_PAGE_INSTFILES

  # These indented statements modify settings for MUI_PAGE_FINISH
    !define MUI_FINISHPAGE_NOAUTOCLOSE
    !define MUI_FINISHPAGE_RUN
    !define MUI_FINISHPAGE_RUN_CHECKED
    !define MUI_FINISHPAGE_RUN_TEXT "Launch Wally"
    !define MUI_FINISHPAGE_RUN_FUNCTION "LaunchWally"
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------

Function LaunchWally
  Exec "$INSTDIR\Wally.exe"
FunctionEnd

; The stuff to install
Section "Wally"

  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Put file there
  File "Wally.exe"

  ; Write the installation path into the registry
  WriteRegStr HKLM "Software\BeCrux\NSIS\Wally" "Install Dir" "$INSTDIR"

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Wally" "DisplayName" "Wally"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Wally" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Wally" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Wally" "NoRepair" 1
  WriteUninstaller "uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Wally.lnk" "$INSTDIR\Wally.exe"

    WriteRegStr HKLM "Software\BeCrux\NSIS\Wally" "StartMenuFolder" "$StartMenuFolder"
  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd


;--------------------------------

; Uninstaller

Section "Uninstall"

  !include WinMessages.nsh
  FindWindow $0 "QWidget" "Wally"
  SendMessage $0 ${WM_USER} 1234 5678

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Wally"

  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "Wally"

  MessageBox MB_YESNO "Would you like to keep your settings?" IDYES keep_settings
  DeleteRegKey HKCU "Software\BeCrux\Wally"
keep_settings:

  ; Remove files and uninstaller
  Delete $INSTDIR\Wally.exe
  Delete $INSTDIR\uninstall.exe

  ReadRegStr $StartMenuFolder HKLM "Software\BeCrux\NSIS\Wally" "StartMenuFolder"
  DeleteRegKey HKLM "Software\BeCrux\NSIS\Wally"

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\$StartMenuFolder\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\$StartMenuFolder"
  RMDir "$INSTDIR"

SectionEnd
