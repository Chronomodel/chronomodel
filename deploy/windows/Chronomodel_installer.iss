; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "Chronomodel"
#define MyAppVersion "1.0"
#define MyAppPublisher "CNRS"
#define MyAppURL "http://www.chronomodel.com"
#define MyAppExeName "Chronomodel.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{12F55A98-21A8-42B3-93EE-50082D194617}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile="..\LicenseGPL30.txt"
InfoBeforeFile="..\readme.rtf"
OutputDir=.\
OutputBaseFilename=Chronomodel_installer
SetupIconFile="..\..\icon\Chronomodel.ico"
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Files]
Source: "..\..\build\release\build\Chronomodel.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Chronomodel_user_Manual.pdf"; DestDir: "{app}"; Flags: ignoreversion
Source: "additionnal_files\icudt51.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "additionnal_files\icuin51.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "additionnal_files\icuuc51.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "additionnal_files\libgcc_s_dw2-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "additionnal_files\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "additionnal_files\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "additionnal_files\Qt5Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "additionnal_files\Qt5Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "additionnal_files\Qt5Svg.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "additionnal_files\Qt5Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "additionnal_files\libfftw3f-3.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "additionnal_files\imageformats"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "additionnal_files\platforms"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Calib"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
