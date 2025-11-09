[Setup]
AppName=Key Status
AppVersion=1.0
DefaultDirName={pf}\keystatus
DefaultGroupName=Key Status
OutputBaseFilename=KeyStatSetup
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Files]
Source: "E:\winprojects\cnskeys\cnskeys\config.cfg"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\winprojects\cnskeys\cnskeys\x64\Release\cnskeys.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Key Status"; Filename: "{app}\cnskeys.exe"
Name: "{userdesktop}\Key Status"; Filename: "{app}\cnskeys.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Crear acceso directo en el escritorio"; GroupDescription: "Opciones adicionales:"

[Run]
Filename: "{app}\cnskeys.exe"; Description: "Ejecutar key stat"; Flags: nowait postinstall skipifsilent
