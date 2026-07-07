@echo off
setlocal

echo === Step 1: Copy project ===
if exist "f:\Android\SelfBalanceCar_build" (
    echo Removing old build directory...
    rmdir /s /q "f:\Android\SelfBalanceCar_build"
)
xcopy "f:\Android\SelfBalanceCar\app" "f:\Android\SelfBalanceCar_build\app\" /E /I /Y /EXCLUDE:f:\Android\SelfBalanceCar\exclude.txt
copy "f:\Android\SelfBalanceCar\build.gradle.kts" "f:\Android\SelfBalanceCar_build\" /Y
copy "f:\Android\SelfBalanceCar\settings.gradle.kts" "f:\Android\SelfBalanceCar_build\" /Y
copy "f:\Android\SelfBalanceCar\gradlew.bat" "f:\Android\SelfBalanceCar_build\" /Y
copy "f:\Android\SelfBalanceCar\gradlew" "f:\Android\SelfBalanceCar_build\" /Y
if exist "f:\Android\SelfBalanceCar\gradle" xcopy "f:\Android\SelfBalanceCar\gradle" "f:\Android\SelfBalanceCar_build\gradle\" /E /I /Y
if exist "f:\Android\SelfBalanceCar\gradle.properties" copy "f:\Android\SelfBalanceCar\gradle.properties" "f:\Android\SelfBalanceCar_build\" /Y

echo === Step 2: Build ===
cd /d "f:\Android\SelfBalanceCar_build"
call gradlew.bat assembleDebug

echo === Step 3: Install ===
if exist "app\build\outputs\apk\debug\app-debug.apk" (
    echo APK found, installing...
    adb install -r "app\build\outputs\apk\debug\app-debug.apk"
    echo === Installed! ===
) else (
    echo APK not found, build may have failed.
)

endlocal