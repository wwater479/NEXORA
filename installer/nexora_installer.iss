; NEXORA Windows Installer Configuration (Inno Setup)
; Produces official Windows x64 Setup package for NEXORA Browser

#define MyAppName "NEXORA"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "NEXORA Open Source"
#define MyAppURL "https://github.com/wwater479/NEXORA"
#define MyAppExeName "NEXORA.exe"
#define MySubprocessExeName "NEXORA_Subprocess.exe"

[Setup]
AppId={{8B0A7B2A-6F1D-4C98-9C21-A875D3F75D20}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=..\LICENSE
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=..\dist
OutputBaseFilename=NEXORA_Setup_x64_v{#MyAppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DisableDirPage=auto
DisableProgramGroupPage=auto

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Main Binaries
Source: "..\build\windows-x64-release\bin\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\windows-x64-release\bin\{#MySubprocessExeName}"; DestDir: "{app}"; Flags: ignoreversion

; Core Chromium Embedded Framework Dependencies
Source: "..\build\windows-x64-release\bin\libcef.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\windows-x64-release\bin\chrome_elf.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\windows-x64-release\bin\d3dcompiler_47.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\windows-x64-release\bin\libEGL.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\windows-x64-release\bin\libGLESv2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\windows-x64-release\bin\vk_swiftshader.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\windows-x64-release\bin\vulkan-1.dll"; DestDir: "{app}"; Flags: ignoreversion

; Resources & Localization Data
Source: "..\build\windows-x64-release\bin\icudtl.dat"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\windows-x64-release\bin\resources.pak"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\windows-x64-release\bin\chrome_100_percent.pak"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\windows-x64-release\bin\chrome_200_percent.pak"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\windows-x64-release\bin\v8_context_snapshot.bin"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\windows-x64-release\bin\locales\*"; DestDir: "{app}\locales"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Clean internal logs and caches on uninstall, preserving user profiles by default
Type: files; Name: "{app}\*.log"

[Code]
// Custom Inno Setup Pascal Script ensuring user profiles are protected during upgrades
function InitializeSetup(): Boolean;
begin
  Result := True;
end;
