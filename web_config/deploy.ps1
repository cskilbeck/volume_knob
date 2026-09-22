#Requires -Version 5.1
<#
.SYNOPSIS
    Deploys the unified configurator to its per-domain web roots on the VPS.

.DESCRIPTION
    `npm run build` in web_unified_config produces dist-sites\<domain>\  -  one
    complete web root per domain, identical bundles with a per-domain <head>.
    This copies them to /var/www/<key>/ on the server.

    Files are copied over the top; nothing is deleted. That is deliberate: the
    asset filenames are content hashes, so an old index.html sitting in
    someone's cache still resolves to the bundle it names. Prune by hand once
    or twice a year.

    Nothing is built unless you pass -Build, so what gets deployed is exactly
    what you verified locally.

.PARAMETER Site
    Which domain to deploy, or 'all' (default) for every one.

.PARAMETER Build
    Run `npm run build` first.

.PARAMETER Force
    Skip the confirmation prompt.

.PARAMETER NoSudo
    Copy straight into the web root without sudo. Only works if the roots are
    owned by the ssh user (sudo chown -R chs:www-data /var/www/<key>).

.EXAMPLE
    .\deploy.ps1 -Build
    Build, then deploy all four domains after confirming.

.EXAMPLE
    .\deploy.ps1 -Site midiknob.com
    Deploy one domain from the existing build.

.EXAMPLE
    .\deploy.ps1 -WhatIf
    Show what would happen and touch nothing.
#>

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [ValidateSet('all', 'usbknob.com', 'midiknob.com', 'midiadapter.com', 'tinyvolumeknob.com')]
    [string] $Site = 'all',

    [switch] $Build,
    [switch] $Force,
    [switch] $NoSudo,

    [string] $Server = '167.71.142.52',
    [string] $User   = 'chs'
)

$ErrorActionPreference = 'Stop'

# domain -> web root name under /var/www
$ROOTS = [ordered]@{
    'midiadapter.com'    = 'midiadapter'
    'midiknob.com'       = 'midiknob'
    'tinyvolumeknob.com' = 'tinyvolumeknob'
    'usbknob.com'        = 'usbknob'
}

# usbknob.com is the URL already in the wild, with real users on it. Deploying
# there gets called out separately so it is never a thing you did by accident.
$LIVE = 'usbknob.com'

$AppDir    = Join-Path $PSScriptRoot 'web_unified_config'
$DistSites = Join-Path $AppDir 'dist-sites'

#------------------------------------------------------------------ helpers

function Invoke-External {
    param(
        [Parameter(Mandatory)] [string]   $Exe,
        [Parameter(Mandatory)] [string[]] $Arguments,
        [string] $What = 'command'
    )
    Write-Verbose "$Exe $($Arguments -join ' ')"
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$What failed (exit $LASTEXITCODE): $Exe $($Arguments -join ' ')"
    }
}

function Assert-Tooling {
    foreach ($exe in @('ssh.exe', 'scp.exe')) {
        if (-not (Get-Command $exe -ErrorAction SilentlyContinue)) {
            throw "$exe not found. Install it with: Add-WindowsCapability -Online -Name OpenSSH.Client~~~~0.0.1.0"
        }
    }
}

function Get-BundleName {
    # The hashed bundle this index.html actually points at  -  the one number
    # that tells you whether you are shipping what you think you are.
    param([string] $IndexPath)
    $html = Get-Content -Raw -LiteralPath $IndexPath
    if ($html -match '/assets/(index-[A-Za-z0-9_\-]+\.js)') { return $Matches[1] }
    return '(no bundle reference found)'
}

#------------------------------------------------------------------ preflight

Assert-Tooling

if ($Build) {
    Write-Host "Building..." -ForegroundColor Cyan
    Push-Location $AppDir
    try {
        Invoke-External -Exe 'npm.cmd' -Arguments @('run', 'build') -What 'npm run build'
    } finally {
        Pop-Location
    }
    Write-Host ""
}

if (-not (Test-Path -LiteralPath $DistSites)) {
    throw "$DistSites not found. Run this with -Build, or 'npm run build' in $AppDir first."
}

$targets = if ($Site -eq 'all') { $ROOTS.Keys } else { @($Site) }

# Validate everything before copying anything, so a half-built dist-sites
# cannot leave one domain updated and another stale.
$plan = foreach ($domain in $targets) {

    $src   = Join-Path $DistSites $domain
    $index = Join-Path $src 'index.html'

    if (-not (Test-Path -LiteralPath $index)) {
        throw "Missing $index  -  build output is incomplete for $domain."
    }
    if (-not (Test-Path -LiteralPath (Join-Path $src 'assets'))) {
        throw "Missing assets\ for $domain  -  build output is incomplete."
    }

    $files = Get-ChildItem -LiteralPath $src -Recurse -File

    [pscustomobject]@{
        Domain = $domain
        Root   = "/var/www/$($ROOTS[$domain])"
        Key    = $ROOTS[$domain]
        Source = $src
        Files  = $files.Count
        SizeKB = [math]::Round(($files | Measure-Object Length -Sum).Sum / 1KB)
        Built  = (Get-Item -LiteralPath $index).LastWriteTime
        Bundle = Get-BundleName -IndexPath $index
    }
}

Write-Host "About to deploy to $User@$Server" -ForegroundColor Cyan
$plan | Format-Table Domain, Root, Bundle, Files, SizeKB, Built -AutoSize | Out-String | Write-Host

$age = (Get-Date) - ($plan | Sort-Object Built | Select-Object -First 1).Built
if ($age.TotalHours -gt 12) {
    Write-Host ("Note: this build is {0:N1} hours old. Pass -Build to rebuild." -f $age.TotalHours) -ForegroundColor Yellow
}
if ($plan.Domain -contains $LIVE) {
    Write-Host "Note: $LIVE is the live site with existing users; its behaviour is meant to stay frozen." -ForegroundColor Yellow
}

if (-not $Force -and -not $WhatIfPreference) {
    $answer = Read-Host "Deploy? [y/N]"
    if ($answer -notmatch '^(y|yes)$') {
        Write-Host "Nothing deployed."
        return
    }
}

#------------------------------------------------------------------ deploy

$remote = "$User@$Server"

foreach ($item in $plan) {

    if (-not $PSCmdlet.ShouldProcess("$remote`:$($item.Root)", "deploy $($item.Domain)")) { continue }

    Write-Host "-> $($item.Domain) -> $($item.Root)" -ForegroundColor Green

    if ($NoSudo) {
        # Roots already writable by $User: copy straight in.
        Push-Location $DistSites
        try {
            Invoke-External -Exe 'scp.exe' `
                -Arguments @('-r', "$($item.Domain)/.", "$remote`:$($item.Root)/") `
                -What 'scp'
        } finally {
            Pop-Location
        }
    }
    else {
        $stage = "/tmp/deploy-$($item.Key)"

        # Stage in /tmp, then move into place with sudo. scp cannot elevate,
        # and the web roots are root-owned.
        Invoke-External -Exe 'ssh.exe' -Arguments @($remote, "rm -rf $stage") -What 'ssh (clear staging)'

        # Run from dist-sites so the local argument has no drive letter  - 
        # scp reads 'E:\...' as host 'E'.
        Push-Location $DistSites
        try {
            Invoke-External -Exe 'scp.exe' `
                -Arguments @('-r', $item.Domain, "$remote`:$stage") `
                -What 'scp'
        } finally {
            Pop-Location
        }

        # -t so sudo can prompt for a password if it needs one.
        # chmod a+rX because files arriving from Windows carry no useful mode
        # bits, and an unreadable asset is a 403 that looks like a 404.
        $cmd = "sudo mkdir -p $($item.Root) && " +
               "sudo cp -r $stage/. $($item.Root)/ && " +
               "sudo chmod -R a+rX $($item.Root) && " +
               "rm -rf $stage"

        Invoke-External -Exe 'ssh.exe' -Arguments @('-t', $remote, $cmd) -What 'ssh (install)'
    }

    Write-Host "  copied $($item.Files) files" -ForegroundColor DarkGray
}

#------------------------------------------------------------------ verify

if ($WhatIfPreference) { return }

Write-Host ""
Write-Host "Verifying (origin directly, bypassing Cloudflare):" -ForegroundColor Cyan

foreach ($item in $plan) {

    # curl.exe, not curl  -  in PowerShell `curl` is an alias for
    # Invoke-WebRequest, which has no --resolve.
    # -join because curl.exe returns one array element per line, and -match
    # against an array filters it instead of populating $Matches.
    $html = (& curl.exe -s --max-time 15 --resolve "$($item.Domain):443:$Server" "https://$($item.Domain)/" 2>$null) -join "`n"

    if ($LASTEXITCODE -ne 0 -or -not $html) {
        Write-Host ("  {0,-22} not serving yet (vhost, DNS or certificate still to do)" -f $item.Domain) -ForegroundColor Yellow
        continue
    }

    $title  = if ($html -match '<title>([^<]*)</title>') { $Matches[1] } else { '(no title)' }
    $bundle = if ($html -match '/assets/(index-[A-Za-z0-9_\-]+\.js)') { $Matches[1] } else { '(none)' }
    $ok     = ($bundle -eq $item.Bundle)

    Write-Host ("  {0,-22} {1}" -f $item.Domain, $title) -ForegroundColor ($(if ($ok) { 'Green' } else { 'Yellow' }))
    if (-not $ok) {
        Write-Host ("  {0,-22} serving {1}, expected {2}" -f '', $bundle, $item.Bundle) -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "Done." -ForegroundColor Cyan
