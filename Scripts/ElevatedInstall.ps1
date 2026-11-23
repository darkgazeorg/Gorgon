[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$CMakeExecutable,

    [Parameter(Mandatory = $true)]
    [string]$BuildDirectory,

    [Parameter(Mandatory = $true)]
    [ValidateSet('Release', 'Debug', 'RelWithDebInfo')]
    [string]$Configuration
)

$ErrorActionPreference = 'Stop'

try {
    $arguments = '--install "{0}" --config {1}' -f $BuildDirectory, $Configuration
    $process = Start-Process -FilePath $CMakeExecutable `
        -ArgumentList $arguments `
        -WorkingDirectory $BuildDirectory `
        -Verb RunAs `
        -Wait `
        -PassThru

    exit $process.ExitCode
}
catch {
    [Console]::Error.WriteLine("Elevated CMake install failed: {0}" -f $_.Exception.Message)
    exit 1
}
