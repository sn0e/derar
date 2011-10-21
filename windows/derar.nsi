!include "MUI.nsh"

Name "deRAR"
OutFile "derar_setup.exe"
InstallDir "$PROGRAMFILES\deRAR"
SilentUnInstall silent

!define MUI_ICON "derar.ico"

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "Install"
	SetOutPath "$INSTDIR"

	File "deRAR.exe"
	WriteUninstaller "Uninstall.exe"

	WriteRegSTR HKLM "Software\deRAR" "InstallPath" "$INSTDIR\deRAR.exe"
	WriteRegSTR HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\deRAR" "DisplayName" "deRAR"
	WriteRegSTR HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\deRAR" "InstallLocation" "$INSTDIR"
	WriteRegSTR HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\deRAR" "UninstallString" "$INSTDIR\Uninstall.exe"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\deRAR" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\deRAR" "NoRepair" 1
SectionEnd

Section "Uninstall"
	Delete "$INSTDIR\deRAR.exe"
	Delete "$INSTDIR\Uninstall.exe"
	RMDir  "$INSTDIR"

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\deRAR"
SectionEnd

