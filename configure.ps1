param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$BuildType = "Debug",
    [string]$BuildDir = "build",
    [string]$exe="myapp.exe",
    [string]$VcpkgRoot = $env:VCPKG_ROOT,
    [string]$MSYS_UCRT_ROOT = $env:MSYS_UCRT64, #this env was manually defined
    [switch]$Clean,
    [switch]$c,
    [switch]$b,
    [switch]$e,
    [switch]$cb,
    [switch]$be,
    [switch]$ce,
    [switch]$cbe,
    [switch]$debug
)

$ErrorActionPreference = "Stop"

$DoConfigure = $c -or $cb -or $cbe
$DoBuild = $b -or $cb -or $be -or $cbe
$DoExecute = $e -or $be -or $cbe

if (-not ($DoConfigure -or $DoBuild -or $DoExecute)) {
    Write-Output "No option selected, default: configure"
    $DoConfigure = $true
}

$ToolchainFile = "$VcpkgRoot\scripts\buildsystems\vcpkg.cmake"

$CC = "$MSYS_UCRT_ROOT\gcc.exe"
$CXX = "$MSYS_UCRT_ROOT\g++.exe"

Write-Output "Build Type: $BuildType"
Write-Output "Build Directory: $BuildDir"
Write-Output "Executable: $exe"
Write-Output "Vcpkg Root: $VcpkgRoot"
Write-Output "Toolchain File: $ToolchainFile"
Write-Output "C Compiler: $CC"
Write-Output "C++ Compiler: $CXX"

if ($DoConfigure) {
    if ([string]::IsNullOrWhiteSpace($VcpkgRoot)) {
        throw "The VCPKG_ROOT environment variable is not set."
    }

    if (-not (Test-Path $ToolchainFile)) {
        throw "Could not find the vcpkg toolchain file at: $ToolchainFile"
    }

    if (-not (Test-Path $CC)) {
        throw "Could not find the C compiler at: $CC"
    }

    if (-not (Test-Path $CXX)) {
        throw "Could not find the C++ compiler at: $CXX"
    }

    if ($Clean -and (Test-Path $BuildDir)) {
        Remove-Item -Recurse -Force $BuildDir
    }

    if (-not (Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir | Out-Null
    }

    cmake -S . -B $BuildDir -G Ninja `
      -DCMAKE_BUILD_TYPE="$BuildType" `
      -DCMAKE_CXX_COMPILER="$CXX" `
      -DCMAKE_TOOLCHAIN_FILE="$ToolchainFile" `
      -DVCPKG_TARGET_TRIPLET=x64-mingw-static `
      -DVCPKG_HOST_TRIPLET=x64-mingw-static `
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
}

if ($DoBuild) {
    cmake --build $BuildDir --config $BuildType
}

if ($DoExecute) {
    $AppCandidates = @(
        (Join-Path $BuildDir $exe),
        (Join-Path (Join-Path $BuildDir $BuildType) $exe)
    )

    $AppPath = $AppCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $AppPath) {
        throw "Could not find $exe in '$BuildDir'. Build with -b or use -cbe."
    }

    if($debug) {
        $env:VK_LOADER_DEBUG = "error, warn, debug"
    }

    & $AppPath
    
    if($debug) {
        $env:VK_LOADER_DEBUG = $null
    }

}