"""Append classes.dex and the native .so into an aapt2-produced apk (zip)."""
import os
import sys
import zipfile

apk, dex, so = sys.argv[1], sys.argv[2], sys.argv[3]
tmp = apk + '.tmp'
with zipfile.ZipFile(apk) as zin, zipfile.ZipFile(tmp, 'w', zipfile.ZIP_DEFLATED) as zout:
    for it in zin.infolist():
        if it.filename in ('classes.dex', 'lib/arm64-v8a/libs5dng.so'):
            continue
        zout.writestr(it, zin.read(it.filename))
    zout.write(dex, 'classes.dex')
    zout.write(so, 'lib/arm64-v8a/libs5dng.so')
os.replace(tmp, apk)
print('apk now:', os.path.getsize(apk), 'bytes')
