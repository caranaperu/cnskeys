[Setup]
AppName=Key Status
AppVersion=1.0
DefaultDirName={commonpf}\keystatus
DefaultGroupName=Key Status
OutputBaseFilename=KeyStatSetup
Compression=lzma
SolidCompression=yes
CloseApplications=force
RestartApplications=yes

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
Name: "runatstartup"; Description: "Run application when Windows starts"; GroupDescription: "Startup options:"; Flags: unchecked

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "KeyStatus"; ValueData: "{app}\cnskeys.exe"; Tasks: runatstartup; Flags: uninsdeletevalue


[Run]
Filename: "{app}\cnskeys.exe"; Description: "Ejecutar key stat"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: files; Name: "{userappdata}\CarSoft\keystat\config.cfg"
Type: dirifempty; Name: "{userappdata}\CarSoft\keystat"
Type: dirifempty; Name: "{userappdata}\CarSoft"

[UninstallRun]
Filename: {sys}\taskkill.exe; Parameters: "/f /im cnskeys.exe"; Flags: skipifdoesntexist runhidden