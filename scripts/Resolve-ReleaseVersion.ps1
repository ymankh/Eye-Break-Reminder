param(
    [Parameter(Mandatory = $true)]
    [string]$Version
)

$ErrorActionPreference = 'Stop'

if ($Version -notmatch '^v?(\d+)\.(\d+)\.(\d+)(?:[-+].*)?$') {
    throw "Version '$Version' must match vMajor.Minor.Patch or Major.Minor.Patch."
}

$installerVersion = "$($matches[1]).$($matches[2]).$($matches[3])"
$releaseTag = if ($Version.StartsWith('v')) { $Version } else { "v$Version" }

if ($env:GITHUB_OUTPUT) {
    "installer_version=$installerVersion" >> $env:GITHUB_OUTPUT
    "release_tag=$releaseTag" >> $env:GITHUB_OUTPUT
}

[pscustomobject]@{
    InstallerVersion = $installerVersion
    ReleaseTag = $releaseTag
}
