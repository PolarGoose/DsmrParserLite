Function Info($msg) {
    Write-Host -ForegroundColor DarkGreen "`nINFO: $msg`n"
}

Function Error($msg) {
    Write-Host `n`n
    Write-Error $msg
    exit 1
}

Function CheckReturnCodeOfPreviousCommand($msg) {
    if(-Not $?) {
        Error "${msg}. Error code: $LastExitCode"
    }
}

Function RemoveFileIfExists($fileName) {
    Info "Remove '$fileName'"
    Remove-Item $fileName  -Force  -Recurse -ErrorAction SilentlyContinue
}

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"
$root = Resolve-Path "$PSScriptRoot"
$buildDir = "$root/build"

Info "Find Visual Studio installation path"
$vswhereCommand = Get-Command -Name "${Env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installationPath = & $vswhereCommand -prerelease -latest -property installationPath

Info "Open Visual Studio 2022 Developer PowerShell"
& "$installationPath\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64

Info "Cmake generate cache"
cmake `
  -S $root `
  -B $buildDir/out `
  -G Ninja `
  -DCMAKE_BUILD_TYPE=Release
CheckReturnCodeOfPreviousCommand "cmake cache failed"

Info "Cmake build"
cmake --build $buildDir/out
CheckReturnCodeOfPreviousCommand "cmake build failed"

Info "Run tests"
& $buildDir/out/test_executable.exe
CheckReturnCodeOfPreviousCommand "tests failed"

Info "Copy the header file to the publish directory and archive it"
New-Item $buildDir/publish -Force -ItemType "directory" > $null
Copy-Item -Path $buildDir/out/DsmrParser/DsmrParser/DsmrParser.h -Destination $buildDir/publish/DsmrParser.h
Compress-Archive -Force -Path $buildDir/publish/DsmrParser.h -DestinationPath $buildDir/publish/DsmrParser.h.zip
