Set-Location "f:\Android\SelfBalanceCar"
Stop-Process -Name java -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 3
Remove-Item -Path "f:\Android\SelfBalanceCar\app\build" -Recurse -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1
& ".\gradlew.bat" assembleDebug 2>&1