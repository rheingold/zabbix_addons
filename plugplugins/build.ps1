# build.ps1 - Build all measurement plugins
$ErrorActionPreference = 'Stop'
Write-Host 'Building plugins...' -F Cyan
if (!(Test-Path build)) { mkdir build | Out-Null }
Get-ChildItem *.c | ForEach-Object {
    $name = $_.BaseName
    $src = $_.FullName
    $out = "build\${name}.dll"
    $content = Get-Content $src -Raw
    $libs = @()
    if ($content -match 'pdh\.h') { $libs += '-lPdh' }
    if ($content -match 'psapi\.h') { $libs += '-lpsapi' }
    if ($content -match 'OpenSCManager|OpenService') { $libs += '-ladvapi32' }
    Write-Host "Building $name..." -F Yellow
    & gcc -shared -o $out $src @libs -Wall -O2 -static-libgcc
    if ($LASTEXITCODE -eq 0) { Write-Host "  OK" -F Green } else { Write-Host "  FAILED" -F Red }
}
Write-Host 'Done!' -F Green
