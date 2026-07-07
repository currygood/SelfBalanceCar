@echo off
cd /d f:\Android\SelfBalanceCar
taskkill /F /IM java.exe 2>nul
timeout /t 3 /nobreak >nul
del /f /q "app\build\intermediates\compile_and_runtime_not_namespaced_r_class_jar\debug\R.jar" 2>nul
call gradlew.bat assembleDebug