; Installer Windows (Inno Setup 6.3+). Dikompilasi dari folder proyek:
;   ISCC /DAppVersion=0.1.0 tools\windows\installer.iss
; Hasil: dist\TriVerse-Chorus-<versi>-Windows-Setup.exe

#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#define Root "..\.."
#define Art  Root + "\build\TriVerseChorus_artefacts\Release"

[Setup]
AppName=TriVerse Chorus
AppVersion={#AppVersion}
AppPublisher=TriVerse Audio
DefaultDirName={autopf}\TriVerse Chorus
DefaultGroupName=TriVerse Chorus
OutputDir={#Root}\dist
OutputBaseFilename=TriVerse-Chorus-{#AppVersion}-Windows-Setup
LicenseFile={#Root}\LICENSE
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
UninstallDisplayName=TriVerse Chorus

[Components]
Name: "vst3"; Description: "VST3 plugin"; Types: full compact custom; Flags: fixed
Name: "standalone"; Description: "Standalone application"; Types: full

[Files]
Source: "{#Art}\VST3\TriVerse Chorus.vst3\*"; DestDir: "{commoncf64}\VST3\TriVerse Chorus.vst3"; Flags: recursesubdirs ignoreversion; Components: vst3
Source: "{#Art}\Standalone\TriVerse Chorus.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone

[Icons]
Name: "{autoprograms}\TriVerse Chorus"; Filename: "{app}\TriVerse Chorus.exe"; Components: standalone
