# clean_build.ps1
$buildDir = "build"

if (Test-Path $buildDir) {
    Write-Host "Removing existing build directory..."
    Remove-Item -Path $buildDir -Recurse -Force
}

New-Item -ItemType Directory -Path $buildDir | Out-Null
Set-Location $buildDir

# Attempt to find CMake and Qt. 
# Note: This assumes cmake is in the PATH or the user runs this from a Qt environment terminal.
# If running from VS Code with CMake Tools, this might not be needed, but it helps to reset.

Write-Host "Build directory cleaned. Please try to configure the project again in Qt Creator or VS Code."
Write-Host "If using Qt Creator: Right click project -> Run CMake (or Clear CMake Configuration first)"
