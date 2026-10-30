#==============================================================================
# VneXR Windows Build Script (PowerShell)
# Shares build/shared/<Config>/build-windows-cl-<cl-version> with build_windows.py.
#==============================================================================

param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$BuildType = "Debug",
    [ValidateSet("shared", "static")]
    [string]$LibType = "shared",
    [ValidateSet("configure", "build", "configure_and_build", "test", "run")]
    [string]$Action = "configure_and_build",
    [switch]$Clean,
    [int]$Jobs = 10,
    [switch]$Dev,
    [switch]$WithTests,
    [switch]$WithExamples,
    [switch]$WithOpenXR,
    [string]$Sample = "",
    [string]$RuntimeJson = ""
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir

# Resolve now, against where the script was started: the script later changes location to the project root.
if ($RuntimeJson) {
    $RuntimeJson = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($RuntimeJson)
}

function Get-ClVersion {
    $clCmd = Get-Command cl -ErrorAction SilentlyContinue
    if (-not $clCmd) {
        Write-Host "Error: cl.exe not found. Run from a Visual Studio Developer Command Prompt."
        exit 1
    }
    $previousErrorAction = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    $output = & cl 2>&1 | Out-String
    $ErrorActionPreference = $previousErrorAction
    if ($output -match "Version (\d+\.\d+\.\d+)") {
        return $Matches[1]
    }
    return "unknown"
}

function Get-CMakeGenerator([string]$CompilerVersion) {
    if ($CompilerVersion -match "^19\.(\d+)" -and [int]$Matches[1] -lt 30) {
        return "Visual Studio 16 2019"
    }
    if ($CompilerVersion -eq "2019" -or $CompilerVersion -like "16.*") {
        return "Visual Studio 16 2019"
    }
    return "Visual Studio 17 2022"
}

$CompilerVersion = Get-ClVersion
$GeneratorName = Get-CMakeGenerator $CompilerVersion
$Generator = "-G `"$GeneratorName`" -A x64 -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl"

if ($Dev) {
    if (-not $PSBoundParameters.ContainsKey("WithTests") -or $WithTests) { $WithTests = $true }
    if (-not $PSBoundParameters.ContainsKey("WithExamples") -or $WithExamples) { $WithExamples = $true }
}
if ($WithOpenXR -or $Action -eq "run") {
    $WithExamples = $true
}
if ($WithOpenXR -and -not $env:VULKAN_SDK) {
    Write-Host "Warning: VULKAN_SDK is not set; install the LunarG Vulkan SDK if CMake cannot find Vulkan."
}

$DevFlag = if ($Dev) { "ON" } else { "OFF" }
$TestsFlag = if ($WithTests) { "ON" } else { "OFF" }
$ExamplesFlag = if ($WithExamples) { "ON" } else { "OFF" }
# Always pass both switches: omitted flags would otherwise keep a cached ON from an earlier OpenXR configure.
$OpenXrFlags = "-DVNE_XR_WITH_OPENXR=OFF -DVNE_XR_BACKEND_VULKAN=OFF"
if ($WithOpenXR) {
    $OpenXrFlags = "-DVNE_XR_WITH_OPENXR=ON -DVNE_XR_WITH_RHI=ON -DVNE_XR_BACKEND_VULKAN=ON"
}

$BuildDir = Join-Path $ProjectRoot "build\$LibType\$BuildType\build-windows-cl-$CompilerVersion"
$ConfigureCmd = "cmake -B `"$BuildDir`" -S `"$ProjectRoot`" -DCMAKE_BUILD_TYPE=$BuildType -DVNE_XR_LIB_TYPE=$LibType -DVNE_XR_DEV=$DevFlag -DVNE_XR_TESTS=$TestsFlag -DVNE_XR_EXAMPLES=$ExamplesFlag $OpenXrFlags $Generator"
$BuildCmd = "cmake --build `"$BuildDir`" --config $BuildType --parallel $Jobs"
$TestCmd = "ctest --test-dir `"$BuildDir`" --output-on-failure -C $BuildType"

# $ErrorActionPreference does not stop on a failing native command, so check the exit code of every step.
function Invoke-Step([string]$Command) {
    $global:LASTEXITCODE = 0
    Invoke-Expression $Command
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error: step failed with exit code ${LASTEXITCODE}: $Command"
        exit $LASTEXITCODE
    }
}

function Clean-Build {
    if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

function Ensure-BuildDir {
    if (-not (Test-Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null }
}

function Find-Launcher {
    $samplesDir = Join-Path $BuildDir "bin\samples"
    foreach ($candidate in @(
            (Join-Path $samplesDir "$BuildType\vnexr_samples.exe"),
            (Join-Path $samplesDir "vnexr_samples.exe"))) {
        if (Test-Path $candidate) { return $candidate }
    }
    return $null
}

function Invoke-Sample {
    $launcher = Find-Launcher
    if (-not $launcher) {
        Write-Host "Error: vnexr_samples.exe not found under $(Join-Path $BuildDir 'bin\samples')"
        exit 1
    }
    $entries = @()
    $binConfig = Join-Path $BuildDir "bin\$BuildType"
    if (Test-Path $binConfig) { $entries += $binConfig }
    if ($env:VULKAN_SDK) {
        $vulkanBin = Join-Path $env:VULKAN_SDK "Bin"
        if (Test-Path $vulkanBin) { $entries += $vulkanBin }
    }
    if ($entries.Count -gt 0) {
        $env:PATH = ($entries -join ";") + ";" + $env:PATH
        Write-Host ("PATH prepended: " + ($entries -join ";"))
    }
    if ($RuntimeJson) {
        $env:XR_RUNTIME_JSON = $RuntimeJson
        Write-Host "XR_RUNTIME_JSON=$RuntimeJson"
    }
    $sampleArgs = @()
    if ($Sample) { $sampleArgs = @("--sample", $Sample) }
    Write-Host ("Running: " + $launcher + $(if ($sampleArgs.Count) { " " + ($sampleArgs -join " ") } else { "" }))
    & $launcher @sampleArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Write-Host "Windows :: cl-$CompilerVersion ($BuildType, $LibType)"
Write-Host "CMake generator: $GeneratorName"
Write-Host ""

if ($Action -eq "test" -and -not $WithTests) {
    Write-Host "ERROR: Tests are disabled. Reconfigure with -WithTests or -Dev."
    exit 1
}

Set-Location $ProjectRoot
switch ($Action) {
    "configure" {
        if ($Clean) { Clean-Build }; Ensure-BuildDir | Out-Null
        Invoke-Step $ConfigureCmd
    }
    "build" {
        if ($Clean) { Clean-Build }; Ensure-BuildDir | Out-Null
        Invoke-Step $ConfigureCmd
        Invoke-Step $BuildCmd
    }
    "configure_and_build" {
        if ($Clean) { Clean-Build }; Ensure-BuildDir | Out-Null
        Invoke-Step $ConfigureCmd
        Invoke-Step $BuildCmd
    }
    "test" {
        if ($Clean) { Clean-Build }; Ensure-BuildDir | Out-Null
        Invoke-Step $ConfigureCmd
        Invoke-Step $BuildCmd
        Invoke-Step $TestCmd
    }
    "run" {
        if ($Clean) { Clean-Build }; Ensure-BuildDir | Out-Null
        Invoke-Step $ConfigureCmd
        Invoke-Step $BuildCmd
        Invoke-Sample
    }
    default {
        Write-Host "Usage: .\build_windows.ps1 [-BuildType Debug|Release|...] [-LibType shared|static] [-Action configure|build|configure_and_build|test|run] [-Clean] [-Jobs N] [-Dev] [-WithTests] [-WithExamples] [-WithOpenXR] [-Sample name] [-RuntimeJson path]"
        exit 1
    }
}

Write-Host ""
Write-Host "=== Build completed successfully ==="
Write-Host "Build directory: $BuildDir"
Write-Host "Tests enabled: $WithTests | Examples enabled: $WithExamples | OpenXR: $WithOpenXR"
