# build-windows-apk.ps1 - manual APK build (no Gradle/AGP)
#   native: NDK cmake -> libs5dng.so (arm64-v8a)
#   res:    aapt2 compile/link (+R.java)
#   java:   javac -> d8
#   pkg:    zip append -> zipalign -> apksigner
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path       # android-s5pro
$Proj = Split-Path -Parent $Root                              # workspace root
$EB   = Join-Path $Proj 'tools\android-build'

$jdkCand = Get-ChildItem "$EB\jdk" -Directory | Select-Object -First 1
if (Test-Path "$EB\jdk\bin\java.exe") { $jdk = "$EB\jdk" }
elseif ($jdkCand -and (Test-Path "$($jdkCand.FullName)\bin\java.exe")) { $jdk = $jdkCand.FullName }
else { throw "JDK not found under $EB\jdk" }
$sdk  = Join-Path $EB 'sdk'
$bt   = (Get-ChildItem "$sdk\build-tools" -Directory | Sort-Object Name -Descending | Select-Object -First 1).FullName
$jar  = "$sdk\platforms\android-34\android.jar"
$ndk  = (Get-ChildItem "$sdk\ndk" -Directory | Select-Object -First 1).FullName
$msysBin = Join-Path $Proj 'tools\msys64\mingw64\bin'

$env:JAVA_HOME = $jdk
$env:PATH = "$jdk\bin;$msysBin;$env:PATH"
$build = "$Root\android\app\build"
New-Item -ItemType Directory -Force -Path "$build\obj", "$build\gen" | Out-Null

Write-Host "== [1/6] native libs5dng.so (arm64-v8a) =="
$buildN = "$Root\build-android"
& "$msysBin\cmake.exe" -S $Root -B $buildN -G Ninja `
  -DCMAKE_MAKE_PROGRAM="$msysBin\ninja.exe" `
  -DCMAKE_TOOLCHAIN_FILE="$ndk\build\cmake\android.toolchain.cmake" `
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-29 `
  -DS5CONV_ANDROID=ON -DANDROID_STL=c++_static -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
& "$msysBin\cmake.exe" --build $buildN --target s5dng
if ($LASTEXITCODE -ne 0) { throw "native build failed" }
$so = "$buildN\libs5dng.so"
Write-Host "so: $((Get-Item $so).Length) bytes"

Write-Host "== [2/6] resources + R.java =="
& "$bt\aapt2.exe" compile --dir "$Root\android\app\src\main\res" -o "$build\res.zip"
if ($LASTEXITCODE -ne 0) { throw "aapt2 compile failed" }
& "$bt\aapt2.exe" link -o "$build\unsigned.apk" -I $jar `
   --min-sdk-version 29 --target-sdk-version 34 `
   --java "$build\gen" `
   --manifest "$Root\android\app\src\main\AndroidManifest.xml" "$build\res.zip"
if ($LASTEXITCODE -ne 0) { throw "aapt2 link failed" }

Write-Host "== [3/6] java compile =="
$sources = @(Get-ChildItem "$Root\android\app\src\main\java" -Recurse -Filter *.java | ForEach-Object { $_.FullName })
$sources += Get-ChildItem "$build\gen" -Recurse -Filter *.java | ForEach-Object { $_.FullName }
& "$jdk\bin\javac.exe" -encoding UTF-8 -classpath $jar -d "$build\obj" $sources
if ($LASTEXITCODE -ne 0) { throw "javac failed" }

Write-Host "== [4/6] dex =="
& "$jdk\bin\jar.exe" cf "$build\classes.jar" -C "$build\obj" .
& "$bt\d8.bat" --lib $jar --release --min-api 29 --output $build "$build\classes.jar"
if ($LASTEXITCODE -ne 0) { throw "d8 failed" }

Write-Host "== [5/6] add dex + native lib =="
python "$Root\scripts\apk_add.py" "$build\unsigned.apk" "$build\classes.dex" $so
if ($LASTEXITCODE -ne 0) { throw "apk add failed" }

Write-Host "== [6/6] align + sign =="
& "$bt\zipalign.exe" -f 4 "$build\unsigned.apk" "$build\aligned.apk"
$ks = Join-Path $Proj 'tools\s5debug.keystore'
if (-not (Test-Path $ks)) {
  & "$jdk\bin\keytool.exe" -genkeypair -keystore $ks -alias s5 -storepass s5pro123 `
    -keypass s5pro123 -dname "CN=s5dng" -keyalg RSA -validity 10000
}
& "$bt\apksigner.bat" sign --ks $ks --ks-pass pass:s5pro123 --out "$build\S5ProDNG.apk" "$build\aligned.apk"
if ($LASTEXITCODE -ne 0) { throw "apksigner failed" }
& "$bt\apksigner.bat" verify --print-certs "$build\S5ProDNG.apk" | Select-Object -First 3
& "$bt\aapt.exe" dump badging "$build\S5ProDNG.apk" | Select-Object -First 5
Write-Host ""
Write-Host "APK READY: $build\S5ProDNG.apk ($([math]::Round((Get-Item "$build\S5ProDNG.apk").Length/1MB,1)) MB)"
