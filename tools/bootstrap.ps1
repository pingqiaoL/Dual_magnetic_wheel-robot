[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$manifest = @{}

Get-Content -LiteralPath (Join-Path $projectRoot 'config\versions.env') | ForEach-Object {
    $line = $_.Trim()
    if ($line -and -not $line.StartsWith('#')) {
        $key, $value = $line.Split('=', 2)
        $manifest[$key] = $value
    }
}

function Install-VerifiedArchive {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Version,
        [Parameter(Mandatory)][string]$Url,
        [Parameter(Mandatory)][string]$Sha512,
        [Parameter(Mandatory)][string]$ArchiveName,
        [Parameter(Mandatory)][string]$ExtractedName,
        [Parameter(Mandatory)][string]$Destination,
        [Parameter(Mandatory)][string]$Commit
    )

    $markerName = '.cboard-source-version'
    $expectedMarker = "$Name $Version $Sha512 $Commit"
    if (Test-Path -LiteralPath $Destination) {
        $marker = Join-Path $Destination $markerName
        if (-not (Test-Path -LiteralPath $marker)) {
            throw "$Destination exists without $markerName. Refusing to replace it."
        }

        $actualMarker = (Get-Content -LiteralPath $marker -Raw).Trim()
        if ($actualMarker -ne $expectedMarker) {
            throw "$Name source marker mismatch. Refusing to replace an existing tree."
        }

        Write-Host "[OK] $Name $Version ($Commit)"
        return
    }

    $upstream = Split-Path -Parent $Destination
    $cache = Join-Path $upstream '.cache'
    New-Item -ItemType Directory -Force -Path $cache | Out-Null
    $archive = Join-Path $cache $ArchiveName

    if (-not (Test-Path -LiteralPath $archive)) {
        $partial = "$archive.part"
        Write-Host "Downloading $Name $Version..."
        if (Test-Path -LiteralPath $partial) {
            & curl.exe -L --fail --retry 8 --retry-delay 3 -C - -o $partial $Url
        } else {
            & curl.exe -L --fail --retry 8 --retry-delay 3 -o $partial $Url
        }
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to download $Name. Partial data was retained at $partial."
        }
        Move-Item -LiteralPath $partial -Destination $archive
    }

    $actualHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA512).Hash.ToLowerInvariant()
    if ($actualHash -ne $Sha512.ToLowerInvariant()) {
        throw "$Name archive checksum mismatch at $archive."
    }

    $extract = "$Destination.extract-$PID"
    New-Item -ItemType Directory -Path $extract | Out-Null
    & tar.exe -xzf $archive -C $extract
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to extract $Name. Incomplete data was retained at $extract."
    }

    $source = Join-Path $extract $ExtractedName
    if (-not (Test-Path -LiteralPath $source -PathType Container)) {
        throw "Expected archive root $ExtractedName was not found in $archive."
    }

    Set-Content -LiteralPath (Join-Path $source $markerName) -Value $expectedMarker -Encoding ascii -NoNewline
    Move-Item -LiteralPath $source -Destination $Destination
    Remove-Item -LiteralPath $extract
    Write-Host "[OK] $Name $Version ($Commit)"
}

$upstream = Join-Path $projectRoot 'upstream'
New-Item -ItemType Directory -Force -Path $upstream | Out-Null

Install-VerifiedArchive -Name 'NuttX' `
    -Version $manifest['NUTTX_VERSION'] `
    -Url $manifest['NUTTX_ARCHIVE_URL'] `
    -Sha512 $manifest['NUTTX_ARCHIVE_SHA512'] `
    -ArchiveName "apache-nuttx-$($manifest['NUTTX_VERSION']).tar.gz" `
    -ExtractedName 'nuttx' `
    -Destination (Join-Path $upstream 'nuttx') `
    -Commit $manifest['NUTTX_COMMIT']

Install-VerifiedArchive -Name 'NuttX Apps' `
    -Version $manifest['NUTTX_APPS_VERSION'] `
    -Url $manifest['NUTTX_APPS_ARCHIVE_URL'] `
    -Sha512 $manifest['NUTTX_APPS_ARCHIVE_SHA512'] `
    -ArchiveName "apache-nuttx-apps-$($manifest['NUTTX_APPS_VERSION']).tar.gz" `
    -ExtractedName 'apps' `
    -Destination (Join-Path $upstream 'apps') `
    -Commit $manifest['NUTTX_APPS_COMMIT']

Write-Host 'Verified upstream source bootstrap completed.'
