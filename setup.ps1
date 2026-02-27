<#
.SYNOPSIS
    Gorgon Engine Auto-Setup Script for Windows.
    Downloads Ninja and vcpkg into a local .tools directory.
#>

param (
    [string]$GitPath = ""
)

# --- 1. Find Git ---
if (!(Get-Command git -ErrorAction SilentlyContinue)) {
    if ($GitPath -and (Test-Path $GitPath)) {
        $GitDir = Split-Path $GitPath
        $env:PATH = "$GitDir;" + $env:PATH
        Write-Host "Using Git from: $GitPath" -ForegroundColor Gray
    } else {
        # Search common install locations for users who "just clicked next"
        $StandardPaths = @(
            "$env:ProgramFiles\Git\cmd\git.exe",
            "$env:ProgramFiles(x86)\Git\cmd\git.exe",
            "$env:LocalAppData\Programs\Git\cmd\git.exe",
            "$env:SystemDrive\Git\cmd\git.exe"
        )
        $found = $false
        foreach ($path in $StandardPaths) {
            if (Test-Path $path) {
                $GitDir = Split-Path $path
                $env:PATH = "$GitDir;" + $env:PATH
                $found = $true
                Write-Host "Found Git at: $path" -ForegroundColor Gray
                break
            }
        }
        if (!$found) {
            Write-Error "Git not found! Please install Git for Windows."
            exit 1
        }
    }
}

# --- 2. Define Local Directories ---
$ToolsDir = "$PSScriptRoot\.tools"
$NinjaDir = "$ToolsDir\ninja"
$VcpkgDir = "$ToolsDir\vcpkg"

if (!(Test-Path $ToolsDir)) { 
    New-Item -ItemType Directory -Path $ToolsDir | Out-Null 
}

# --- 3. Bootstrap Ninja ---
if (!(Test-Path "$NinjaDir\ninja.exe")) {
    Write-Host "--- Downloading Ninja Build System ---" -ForegroundColor Cyan
    if (!(Test-Path $NinjaDir)) { New-Item -ItemType Directory -Path $NinjaDir | Out-Null }
    
    $url = "https://github.com/ninja-build/ninja/releases/latest/download/ninja-win.zip"
    $zipPath = "$ToolsDir\ninja.zip"
    
    Invoke-WebRequest -Uri $url -OutFile $zipPath
    Expand-Archive -Path $zipPath -DestinationPath $NinjaDir -Force
    Remove-Item $zipPath
    Write-Host "Ninja installed successfully." -ForegroundColor Green
} else {
    Write-Host "Ninja is already installed." -ForegroundColor Gray
}

# --- 4. Bootstrap vcpkg ---
if (!(Test-Path "$VcpkgDir\vcpkg.exe")) {
    Write-Host "--- Cloning and Building vcpkg ---" -ForegroundColor Cyan
    # Use Git to clone the repository
    git clone https://github.com/microsoft/vcpkg.git $VcpkgDir
    
    # Run the bootstrap batch file
    Push-Location $VcpkgDir
    .\bootstrap-vcpkg.bat
    Pop-Location
    Write-Host "vcpkg bootstrapped successfully." -ForegroundColor Green
} else {
    Write-Host "vcpkg is already installed." -ForegroundColor Gray
}

# --- 5. Summary ---
Write-Host "`n===============================================" -ForegroundColor Green
Write-Host " Gorgon Setup Complete!" -ForegroundColor Green
Write-Host "===============================================" -ForegroundColor Green
Write-Host "Tools are located in: $ToolsDir"
Write-Host "You can now run CMake configuration."
