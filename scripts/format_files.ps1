# formats C/H/MD files: replace tabs with 4 spaces, trim trailing whitespace, ensure newline at EOF
$files = Get-ChildItem -Recurse -Include *.c,*.h,*.md -File
if (-not $files) {
    Write-Output "No files found to format."
    exit 0
}
foreach ($f in $files) {
    $path = $f.FullName
    try {
        $text = Get-Content -Raw -Encoding UTF8 $path
    } catch {
        Write-Output "Skip (read error): $path"
        continue
    }
    $text = $text -replace "`t", "    "
    $lines = $text -split "\r?\n"
    $lines = $lines | ForEach-Object { $_.TrimEnd() }
    $out = ($lines -join "`r`n") + "`r`n"
    try {
        Set-Content -Path $path -Value $out -Encoding UTF8
        Write-Output "Formatted: $path"
    } catch {
        Write-Output "Failed to write: $path"
    }
}
