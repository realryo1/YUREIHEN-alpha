# tools/convert-encoding.ps1
# バックアップを取り、一般的なソースファイルを
# システム既定のエンコーディング (ANSI/CP932) で読み込み
# UTF-8 で上書きします。実行前にコミットまたはバックアップを推奨。

$exts = @('*.cpp','*.c','*.h','*.hpp','*.txt','*.hlsl','*.md')
$files = Get-ChildItem -Path . -Recurse -Include $exts -File | Where-Object { $_.FullName -notmatch '\\.git\\' -and $_.Extension -ne '.cso' }

if (-not $files) {
    Write-Host 'No files found to convert.'
    exit 0
}

$backup = "repo_backup_src_$(Get-Date -Format yyyyMMdd_HHmmss).zip"
Compress-Archive -Path ($files | ForEach-Object { $_.FullName }) -DestinationPath $backup -Force
Write-Host "Backup created: $backup"

Write-Host 'Converting files: (read with Default -> write as UTF8)'
foreach ($f in $files) {
    $p = $f.FullName
    try {
        $content = Get-Content -Path $p -Encoding Default -Raw
        Set-Content -Path $p -Value $content -Encoding UTF8
        Write-Host "Converted: $p"
    } catch {
        Write-Host "Failed: $p -> $_"
    }
}

Write-Host 'Conversion finished.'
