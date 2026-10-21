#!/data/data/com.termux/files/usr/bin/bash
# build-termux-apk.sh - build the S5Pro DNG APK entirely on-device (Termux)
# Prereqs (one-time):
#   pkg install -y git openjdk-21 aapt2 apksigner d8 gradle
#   (or: pkg install -y openjdk-17; gradle gets the AGP it needs)
# Then:  bash build-termux-apk.sh
set -e
cd "$(dirname "$0")/android"

echo "==> [1/5] compile java"
mkdir -p build/obj build/apk
javac --release 11 -d build/obj \
  app/src/main/java/com/s5pro/converter/MainActivity.java

echo "==> [2/5] dex"
d8 --release --min-api 29 --output build $(find build/obj -name '*.class')

echo "==> [3/5] package resources + native lib"
# native lib must be built first:  ./build-termux-native.sh
mkdir -p build/apk/lib/arm64-v8a
cp ../build-termux/libs5dng.so build/apk/lib/arm64-v8a/
aapt2 compile --dir app/src/main/res -o build/res.zip
aapt2 link -o build/unsigned.apk -I "$ANDROID_HOME/platforms/android-34/android.jar" \
  --manifest app/src/main/AndroidManifest.xml build/res.zip \
  --java build/gen 2>/dev/null || \
aapt2 link -o build/unsigned.apk -I "$PREFIX/share/aapt2/android.jar" \
  --manifest app/src/main/AndroidManifest.xml build/res.zip
cd build && zip -q unsigned.apk classes.dex lib/arm64-v8a/libs5dng.so && cd ..

echo "==> [4/5] align"
zipalign -f 4 build/unsigned.apk build/aligned.apk

echo "==> [5/5] sign (debug key, auto-generated)"
KS="$HOME/.s5debug.keystore"
[ -f "$KS" ] || keytool -genkeypair -keystore "$KS" -alias s5 \
  -storepass s5pro123 -keypass s5pro123 -dname "CN=s5dng" -keyalg RSA -validity 10000
apksigner sign --ks "$KS" --ks-pass pass:s5pro123 --out build/S5ProDNG.apk build/aligned.apk

echo ""
echo "APK ready: android/build/S5ProDNG.apk"
echo "install:   termux-setup-storage && cp build/S5ProDNG.apk /sdcard/Download/"
