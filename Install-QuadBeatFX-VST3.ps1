param(
    [string]$InstallRoot = '',
    [string]$SourceBundle = '',
    [switch]$AllUsers
)

$ErrorActionPreference = 'Stop'

$sourceBundlePath = if ($SourceBundle) {
    $SourceBundle
} elseif (Test-Path -LiteralPath (Join-Path $PSScriptRoot 'QuadBeat FX.vst3') -PathType Container) {
    Join-Path $PSScriptRoot 'QuadBeat FX.vst3'
} else {
    Join-Path $PSScriptRoot 'VST3\QuadBeat FX.vst3'
}
if (-not (Test-Path -LiteralPath $sourceBundlePath -PathType Container)) {
    throw "QuadBeat FX.vst3 was not found beside this installer. Extract the complete ZIP first."
}

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
$isAdministrator = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if ($AllUsers -and -not $InstallRoot -and -not $isAdministrator) {
    $arguments = @(
        '-NoLogo'
        '-NoProfile'
        '-ExecutionPolicy'
        'Bypass'
        '-File'
        ('"{0}"' -f $PSCommandPath)
        '-AllUsers'
    )
    $process = Start-Process -FilePath 'powershell.exe' -ArgumentList $arguments -Verb RunAs -Wait -PassThru
    exit $process.ExitCode
}

$vst3Root = if ($InstallRoot) {
    [System.IO.Path]::GetFullPath($InstallRoot)
} elseif ($AllUsers) {
    $commonFiles = [Environment]::GetFolderPath([Environment+SpecialFolder]::CommonProgramFiles)
    Join-Path $commonFiles 'VST3'
} else {
    Join-Path $env:LOCALAPPDATA 'Programs\Common\VST3'
}
$installedBundle = Join-Path $vst3Root 'QuadBeat FX.vst3'
$installedModule = Join-Path $installedBundle 'Contents\x86_64-win\QuadBeat FX.vst3'

New-Item -ItemType Directory -Path $vst3Root -Force | Out-Null
Copy-Item -LiteralPath $sourceBundlePath -Destination $vst3Root -Recurse -Force
Get-ChildItem -LiteralPath $installedBundle -Recurse -File | Unblock-File

if (-not (Test-Path -LiteralPath $installedModule -PathType Leaf)) {
    throw "The VST3 bundle copied, but its 64-bit module is missing: $installedModule"
}

$module = Get-Item -LiteralPath $installedModule
$digest = (Get-FileHash -LiteralPath $installedModule -Algorithm SHA256).Hash
Write-Host 'QuadBeat FX VST3 installed successfully.' -ForegroundColor Green
$installationScope = if ($AllUsers) { 'all Windows users' } else { 'current Windows user' }
Write-Host "Scope: $installationScope"
Write-Host "Bundle: $installedBundle"
Write-Host "64-bit module: $($module.Length) bytes"
Write-Host "SHA-256: $digest"
Write-Host ''
Write-Host 'In FL Studio: Options > Manage plugins, enable Verify plugins,'
Write-Host 'Rescan previously verified plugins, and Rescan plugins with errors, then Find installed plugins.'
Write-Host 'Load QuadBeat FX in a Mixer effect slot.'
