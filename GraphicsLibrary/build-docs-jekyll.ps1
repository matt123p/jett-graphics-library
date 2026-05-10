[CmdletBinding()]
param(
    [string]$Destination,
    [switch]$InstallDependencies,
    [switch]$Serve,
    [switch]$Incremental,
    [switch]$Drafts
)

$ErrorActionPreference = 'Stop'

function Get-BundleCommand {
    $bundle = Get-Command bundle -ErrorAction SilentlyContinue
    if ($null -ne $bundle) {
        return $bundle.Source
    }

    $bundler = Get-Command bundler -ErrorAction SilentlyContinue
    if ($null -ne $bundler) {
        return $bundler.Source
    }

    throw 'Bundler was not found on PATH. Install Ruby with Bundler before building the Jekyll documentation.'
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$docsDir = [System.IO.Path]::GetFullPath((Join-Path $scriptDir '..\docs'))

if (-not (Test-Path (Join-Path $docsDir '_config.yml'))) {
    throw "Jekyll configuration was not found in $docsDir."
}

$bundle = Get-BundleCommand

Push-Location $docsDir
try {
    if ($InstallDependencies) {
        & $bundle install
        if ($LASTEXITCODE -ne 0) {
            throw 'Bundler failed to install the Jekyll dependencies.'
        }
    }

    $arguments = @('exec', 'jekyll')
    if ($Serve) {
        $arguments += 'serve'
        $arguments += '--livereload'
    }
    else {
        $arguments += 'build'
    }

    if ($Incremental) {
        $arguments += '--incremental'
    }

    if ($Drafts) {
        $arguments += '--drafts'
    }

    if (-not [string]::IsNullOrWhiteSpace($Destination)) {
        $arguments += @('--destination', $Destination)
    }

    & $bundle @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Jekyll documentation generation failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}

Write-Host "Jekyll documentation generated successfully from $docsDir"