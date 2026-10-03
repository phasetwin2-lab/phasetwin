param(
    [Parameter(Mandatory=$true)][string]$JuceDir,
    [Parameter(Mandatory=$true)][string]$PluginvalPath,
    [string]$BuildDir = "$PSScriptRoot/build-release"
)
$ErrorActionPreference = 'Stop'
if (!(Test-Path "$JuceDir/CMakeLists.txt")) { throw 'JUCE source directory required.' }
if (!(Test-Path $PluginvalPath)) { throw 'pluginval.exe path required.' }
$BuildDir = [IO.Path]::GetFullPath($BuildDir)
New-Item -ItemType Directory -Force "$BuildDir/logs" | Out-Null
function Run-Checked([string]$Program, [string[]]$Arguments, [string]$Log) {
    & $Program @Arguments 2>&1 | Tee-Object -FilePath $Log
    if ($LASTEXITCODE -ne 0) { throw "$Program failed ($LASTEXITCODE). See $Log" }
}
Run-Checked 'cmake' @('-S',$PSScriptRoot,'-B',$BuildDir,'-A','x64',"-DJUCE_DIR=$JuceDir") "$BuildDir/logs/configure.txt"
Run-Checked 'cmake' @('--build',$BuildDir,'--config','Release','--parallel') "$BuildDir/logs/build.txt"
Run-Checked 'ctest' @('--test-dir',$BuildDir,'-C','Release','--output-on-failure') "$BuildDir/logs/tests.txt"
$Plugin = "$BuildDir/PhaseTwin_artefacts/Release/VST3/PhaseTwin.vst3"
if (!(Test-Path $Plugin)) { throw 'VST3 build output missing.' }
Run-Checked $PluginvalPath @('--strictness-level','5',$Plugin) "$BuildDir/logs/pluginval-5.txt"
Run-Checked $PluginvalPath @('--strictness-level','10',$Plugin) "$BuildDir/logs/pluginval-10.txt"
Write-Host "Build, tests and pluginval passed. Plugin: $Plugin"
Write-Host 'DAW listening, routing, automation, bypass and offline-render acceptance remains required.'
