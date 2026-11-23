[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$CMakeExecutable,

    [Parameter(Mandatory = $true)]
    [string]$BuildDirectory,

    [Parameter(Mandatory = $true)]
    [ValidateSet('Release', 'Debug', 'RelWithDebInfo')]
    [string]$Configuration,

    [switch]$Elevated
)

$ErrorActionPreference = 'Stop'
$installDirectory = 'C:\Program Files (x86)\Gorgon'

try {
    if (-not $Elevated) {
        $cmakePath = (Get-Command $CMakeExecutable -CommandType Application).Source
        $arguments = '-NoProfile -ExecutionPolicy Bypass -File "{0}" -CMakeExecutable "{1}" -BuildDirectory "{2}" -Configuration {3} -Elevated' -f `
            $PSCommandPath, $cmakePath, $BuildDirectory, $Configuration
        $process = Start-Process -FilePath (Join-Path $PSHOME 'powershell.exe') `
            -ArgumentList $arguments `
            -WorkingDirectory $BuildDirectory `
            -Verb RunAs `
            -Wait `
            -PassThru

        exit $process.ExitCode
    }

    Write-Host 'Wiping old SDK installation...' -ForegroundColor Cyan
    if (Test-Path -LiteralPath $installDirectory) {
        Remove-Item -LiteralPath $installDirectory -Recurse -Force
    }

    & $CMakeExecutable --install $BuildDirectory --config $Configuration
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    Write-Host 'Clean install complete.' -ForegroundColor Green
    exit 0
}
catch {
    [Console]::Error.WriteLine("Elevated clean install failed: {0}" -f $_.Exception.Message)
    exit 1
}
