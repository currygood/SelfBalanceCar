$versions = @('8.2.1','8.2.2','8.1.1','8.0.2')
$root = Split-Path -Parent $PSScriptRoot
$destBase = Join-Path $root '.gradle\wrapper\dists'
New-Item -ItemType Directory -Path $destBase -Force | Out-Null
foreach ($v in $versions) {
    $name = "gradle-$v-bin.zip"
    $url = "https://services.gradle.org/distributions/$name"
    $tmp = Join-Path $env:TEMP $name
    Write-Host "Trying $url"
    try {
        Invoke-WebRequest -Uri $url -OutFile $tmp -UseBasicParsing -ErrorAction Stop
        Expand-Archive -LiteralPath $tmp -DestinationPath $destBase -Force
        Remove-Item $tmp -Force
        Write-Host "Downloaded and extracted $name"
        break
    } catch {
        Write-Host ("Failed to download " + $name + ": " + ($_ | Out-String))
        if (Test-Path $tmp) { Remove-Item $tmp -Force }
    }
}
