package com.s5pro.converter;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class MainActivity extends Activity {

    private static final int PICK_RAF = 41;

    static {
        System.loadLibrary("s5dng");
    }

    public native String nativeVersion();

    public native String nativeConvert(String inPath, String outPath,
                                       double start, double delay, double smoothness);

    private TextView status;
    private ProgressDialog progress;
    private final Handler main = new Handler(Looper.getMainLooper());

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        status = findViewById(R.id.status);
        Button pick = findViewById(R.id.pick);
        pick.setOnClickListener(v -> {
            Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            i.setType("*/*");
            Intent extra = new Intent();
            extra.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{
                    "application/octet-stream", "image/x-fuji-raf", "image/*"});
            i.putExtra(Intent.EXTRA_EXCLUDE_MIME_TYPES, (String[]) null);
            try {
                startActivityForResult(Intent.createChooser(i, "选择 RAF 文件"), PICK_RAF);
            } catch (Exception e) {
                Toast.makeText(this, "无法打开文件选择器: " + e.getMessage(), Toast.LENGTH_LONG).show();
            }
        });

        status.setText("就绪\n" + nativeVersion());
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == PICK_RAF && resultCode == RESULT_OK && data != null && data.getData() != null) {
            convert(data.getData());
        }
    }

    private void convert(Uri rafUri) {
        String name = queryName(rafUri);
        if (name == null || name.isEmpty()) name = "DSCF0001.RAF";
        final String base = name.replaceAll("\\.[rR][aA][fF]$", "");
        final String outName = base + "_merged.dng";

        progress = ProgressDialog.show(this, "转换中", "解码 SuperCCD S+R 双平面…", true, false);
        status.setText("转换中: " + name);

        new Thread(() -> {
            String error = null;
            long outSize = 0;
            try {
                // 1) copy SAF stream to cache (LibRaw needs a real file)
                java.io.File cache = new java.io.File(getCacheDir(), "in.raf");
                copyStream(getContentResolver().openInputStream(rafUri), cache);

                // 2) convert into our private dir, then copy back via MediaStore
                java.io.File outFile = new java.io.File(getCacheDir(), base + ".dng");
                error = nativeConvert(cache.getAbsolutePath(), outFile.getAbsolutePath(),
                        0.75, 0.20, 0.65);
                if (error == null) {
                    Uri saved = saveToDownloads(outName, outFile);
                    outSize = outFile.length();
                    if (saved == null) error = "保存到 Download 失败（MediaStore 返回空）";
                }
                //noinspection ResultOfMethodCallIgnored
                cache.delete();
                //noinspection ResultOfMethodCallIgnored
                outFile.delete();
            } catch (Exception e) {
                error = e.getClass().getSimpleName() + ": " + e.getMessage();
            }
            final String err = error;
            final long size = outSize;
            main.post(() -> {
                if (progress != null) progress.dismiss();
                if (err == null) {
                    status.setText("完成 ✓\n" + outName + " (" + (size / 1048576) + " MB)\n已保存到 Download/s5dng/");
                    Toast.makeText(this, "DNG 已保存", Toast.LENGTH_LONG).show();
                } else {
                    status.setText("失败: " + err);
                    Toast.makeText(this, "转换失败: " + err, Toast.LENGTH_LONG).show();
                }
            });
        }).start();
    }

    private String queryName(Uri uri) {
        try (android.database.Cursor c = getContentResolver().query(uri, null, null, null, null)) {
            if (c != null && c.moveToFirst()) {
                int idx = c.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME);
                if (idx >= 0) return c.getString(idx);
            }
        } catch (Exception ignored) {
        }
        return null;
    }

    private static void copyStream(InputStream in, java.io.File dst) throws IOException {
        try (OutputStream out = new java.io.FileOutputStream(dst)) {
            byte[] buf = new byte[1 << 20];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        } finally {
            try {
                in.close();
            } catch (IOException ignored) {
            }
        }
    }

    /** Saves a file into MediaStore Downloads/s5dng/ (no permission needed on API 29+). */
    private Uri saveToDownloads(String displayName, java.io.File src) throws IOException {
        android.content.ContentValues cv = new android.content.ContentValues();
        cv.put(android.provider.MediaStore.Downloads.DISPLAY_NAME, displayName);
        cv.put(android.provider.MediaStore.Downloads.MIME_TYPE, "image/x-adobe-dng");
        cv.put(android.provider.MediaStore.Downloads.RELATIVE_PATH,
                android.os.Environment.DIRECTORY_DOWNLOADS + "/s5dng");
        cv.put(android.provider.MediaStore.Downloads.IS_PENDING, 1);
        Uri uri = getContentResolver().insert(
                android.provider.MediaStore.Downloads.EXTERNAL_CONTENT_URI, cv);
        if (uri == null) return null;
        try (OutputStream os = getContentResolver().openOutputStream(uri);
             InputStream in = new java.io.FileInputStream(src)) {
            byte[] buf = new byte[1 << 20];
            int n;
            while ((n = in.read(buf)) > 0) os.write(buf, 0, n);
        }
        cv.clear();
        cv.put(android.provider.MediaStore.Downloads.IS_PENDING, 0);
        getContentResolver().update(uri, cv, null, null);
        return uri;
    }
}
