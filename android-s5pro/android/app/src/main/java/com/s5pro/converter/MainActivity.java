package com.s5pro.converter;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ContentValues;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.provider.MediaStore;
import android.provider.OpenableColumns;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends Activity {

    private static final int PICK_RAF = 41;

    static { System.loadLibrary("s5dng"); }

    public native String nativeVersion();
    public native String nativeConvert(String inPath, String outPath,
                                       double start, double delay, double smoothness);
    public native boolean nativeExtractThumbnail(String inPath, String outJpegPath);
    public native String nativeReadMetadata(String inPath);

    static final int ST_PENDING = 0, ST_THUMB_OK = 1, ST_WORKING = 2, ST_DONE = 3, ST_ERROR = 4;

    static class RafItem {
        Uri uri; String name; long size;
        int state = ST_PENDING; String info = "排队中";
        String thumbJpeg;          // embedded preview, full extracted size (<=512)
        Bitmap rowThumb; String meta;
        boolean shareSaved;
    }

    class RafAdapter extends BaseAdapter {
        @Override public int getCount() { return items.size(); }
        @Override public Object getItem(int p) { return items.get(p); }
        @Override public long getItemId(int p) { return p; }
        @Override public View getView(int p, View cv, ViewGroup parent) {
            View v = cv;
            if (v == null) {
                v = LayoutInflater.from(MainActivity.this).inflate(R.layout.item_raf, parent, false);
            }
            RafItem it = items.get(p);
            ImageView iv = v.findViewById(R.id.thumb);
            TextView name = v.findViewById(R.id.name);
            TextView info = v.findViewById(R.id.info);
            if (it.rowThumb != null) iv.setImageBitmap(it.rowThumb);
            else iv.setImageResource(android.R.drawable.ic_menu_camera);
            name.setText(it.name);
            info.setText(it.info);
            int c = it.state == ST_DONE ? Color.rgb(0, 128, 0)
                  : it.state == ST_ERROR ? Color.rgb(200, 0, 0)
                  : it.state == ST_WORKING ? Color.rgb(180, 120, 0) : Color.GRAY;
            info.setTextColor(c);
            return v;
        }
    }

    final List<RafItem> items = new ArrayList<>();
    RafAdapter adapter;
    ListView list;
    TextView overall;
    LinearLayout advPanel;
    SeekBar sbStart, sbDelay, sbSmooth;
    TextView advVals;
    volatile boolean converting;
    final Handler main = new Handler(Looper.getMainLooper());

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        list = findViewById(R.id.list);
        adapter = new RafAdapter();
        list.setAdapter(adapter);
        overall = findViewById(R.id.overall);
        advPanel = findViewById(R.id.advPanel);
        sbStart = findViewById(R.id.sbStart);
        sbDelay = findViewById(R.id.sbDelay);
        sbSmooth = findViewById(R.id.sbSmooth);
        advVals = findViewById(R.id.advVals);
        updateAdvLabel();

        SeekBar.OnSeekBarChangeListener advL = new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar s, int p, boolean u) { updateAdvLabel(); }
            @Override public void onStartTrackingTouch(SeekBar s) { }
            @Override public void onStopTrackingTouch(SeekBar s) { }
        };
        sbStart.setOnSeekBarChangeListener(advL);
        sbDelay.setOnSeekBarChangeListener(advL);
        sbSmooth.setOnSeekBarChangeListener(advL);

        Button pick = findViewById(R.id.pick);
        pick.setOnClickListener(v -> {
            Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            i.setType("*/*");
            i.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true);
            try {
                startActivityForResult(Intent.createChooser(i, "选择 RAF 文件（可多选）"), PICK_RAF);
            } catch (Exception e) {
                Toast.makeText(this, "无法打开文件选择器", Toast.LENGTH_LONG).show();
            }
        });

        Button adv = findViewById(R.id.adv);
        adv.setOnClickListener(v -> {
            advPanel.setVisibility(advPanel.getVisibility() == View.VISIBLE ? View.GONE : View.VISIBLE);
        });

        Button convert = findViewById(R.id.convertAll);
        convert.setOnClickListener(v -> {
            if (converting) { Toast.makeText(this, "正在转换中", Toast.LENGTH_SHORT).show(); return; }
            boolean any = false;
            for (RafItem it : items) if (it.state != ST_DONE) any = true;
            if (!any) { Toast.makeText(this, "没有待转换的文件", Toast.LENGTH_SHORT).show(); return; }
            startConversion();
        });

        list.setOnItemLongClickListener((parent, view, position, id) -> {
            RafItem it = items.get(position);
            if (it.meta == null || it.meta.isEmpty()) {
                Toast.makeText(this, "该文件还没有元数据（等缩略图生成后重试）", Toast.LENGTH_SHORT).show();
                return true;
            }
            new AlertDialog.Builder(this)
                    .setTitle(it.name)
                    .setMessage(prettyMeta(it.meta))
                    .setPositiveButton("关闭", null)
                    .show();
            return true;
        });

        overall.setText("就绪 · " + nativeVersion() + "\n添加 RAF 后长按条目可看 EXIF");
    }

    String prettyMeta(String json) {
        return json.replace(",", "\n").replace("{", "").replace("}", "").replace("\"", "");
    }

    void updateAdvLabel() {
        advVals.setText(String.format("S→R 合并: 起 %.2f · 宽 %.2f · 平滑 %.2f",
                sbStart.getProgress() / 100.0,
                Math.max(1, sbDelay.getProgress()) / 100.0,
                sbSmooth.getProgress() / 100.0));
    }

    double advStart() { return sbStart.getProgress() / 100.0; }
    double advDelay() { return Math.max(1, sbDelay.getProgress()) / 100.0; }
    double advSmooth() { return sbSmooth.getProgress() / 100.0; }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != PICK_RAF || resultCode != RESULT_OK || data == null) return;
        int added = 0;
        if (data.getClipData() != null) {
            int n = data.getClipData().getItemCount();
            for (int i = 0; i < n; i++) {
                Uri u = data.getClipData().getItemAt(i).getUri();
                if (addItem(u)) added++;
            }
        } else if (data.getData() != null) {
            if (addItem(data.getData())) added++;
        }
        if (added > 0) {
            adapter.notifyDataSetChanged();
            updateOverall();
            pumpThumbnails();
        } else {
            Toast.makeText(this, "没有新增文件（重复的会跳过）", Toast.LENGTH_SHORT).show();
        }
    }

    boolean addItem(Uri uri) {
        for (RafItem it : items) if (it.uri.equals(uri)) return false;
        RafItem it = new RafItem();
        it.uri = uri;
        it.name = queryName(uri);
        if (it.name == null || it.name.isEmpty()) it.name = "DSCF" + (1000 + items.size()) + ".RAF";
        it.size = querySize(uri);
        items.add(it);
        return true;
    }

    String queryName(Uri uri) {
        try (android.database.Cursor c = getContentResolver().query(uri, null, null, null, null)) {
            if (c != null && c.moveToFirst()) {
                int idx = c.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (idx >= 0) return c.getString(idx);
            }
        } catch (Exception ignored) { }
        return null;
    }

    long querySize(Uri uri) {
        try (android.database.Cursor c = getContentResolver().query(uri, null, null, null, null)) {
            if (c != null && c.moveToFirst()) {
                int idx = c.getColumnIndex(OpenableColumns.SIZE);
                if (idx >= 0) return c.getLong(idx);
            }
        } catch (Exception ignored) { }
        return -1;
    }

    // ---------------- thumbnail pipeline (sequential) ----------------

    volatile boolean thumbBusy;
    final Object thumbLock = new Object();

    void pumpThumbnails() {
        synchronized (thumbLock) {
            if (thumbBusy) return;
            RafItem next = null;
            for (RafItem it : items) if (it.thumbJpeg == null && it.state == ST_PENDING) { next = it; break; }
            if (next == null) return;
            thumbBusy = true;
        }
        new Thread(() -> {
            RafItem it = null;
            synchronized (thumbLock) {
                for (RafItem x : items) if (x.thumbJpeg == null && x.state == ST_PENDING) { it = x; break; }
                if (it == null) { thumbBusy = false; return; }
            }
            final RafItem item = it;
            try {
                File raf = new File(getCacheDir(), "thumb.raf");
                File jpg = new File(getCacheDir(), "thumb.jpg");
                copyStream(getContentResolver().openInputStream(item.uri), raf);
                boolean ok = nativeExtractThumbnail(raf.getAbsolutePath(), jpg.getAbsolutePath());
                if (ok) {
                    item.thumbJpeg = jpg.getAbsolutePath();
                    BitmapFactory.Options o = new BitmapFactory.Options();
                    o.inSampleSize = sampleFor(jpg.getAbsolutePath(), 128);
                    Bitmap full = BitmapFactory.decodeFile(jpg.getAbsolutePath(), o);
                    if (full != null) {
                        item.rowThumb = Bitmap.createScaledBitmap(full, 128, 128 * full.getHeight() / Math.max(1, full.getWidth()), true);
                        if (item.rowThumb != full) full.recycle();
                    }
                    item.meta = nativeReadMetadata(raf.getAbsolutePath());
                    item.state = ST_THUMB_OK;
                    item.info = "就绪 · " + (item.size > 0 ? item.size / 1048576 + "MB" : "");
                } else {
                    item.info = "预览提取失败（仍可转换）";
                }
                //noinspection ResultOfMethodCallIgnored
                raf.delete();
                // keep jpg until conversion (used for share version)
            } catch (Exception e) {
                item.info = "读取失败: " + e.getClass().getSimpleName();
            }
            main.post(() -> {
                adapter.notifyDataSetChanged();
                synchronized (thumbLock) { thumbBusy = false; }
                pumpThumbnails();
            });
        }).start();
    }

    static int sampleFor(String path, int target) {
        BitmapFactory.Options o = new BitmapFactory.Options();
        o.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(path, o);
        int s = 1;
        while (o.outWidth / (s * 2) >= target) s *= 2;
        return s;
    }

    // ---------------- conversion pipeline ----------------

    void startConversion() {
        converting = true;
        new Thread(() -> {
            doneCount = 0; failCount = 0;
            int total = 0;
            for (RafItem it : items) if (it.state != ST_DONE) total++;
            int idx = 0;
            for (RafItem it : new ArrayList<>(items)) {
                if (it.state == ST_DONE) continue;
                idx++;
                final int no = idx, tot = total;
                main.post(() -> { overall.setText("转换中 " + no + "/" + tot + "：" + it.name); adapter.notifyDataSetChanged(); });
                it.state = ST_WORKING;
                it.info = "解码 S/R 双平面…";
                main.post(adapter::notifyDataSetChanged);
                String err = convertOne(it);
                if (err == null) {
                    it.state = ST_DONE; doneCount++;
                    it.info = "✓ DNG" + (it.shareSaved ? " + 分享JPEG" : "") + " 已存 Download/s5dng/";
                } else {
                    it.state = ST_ERROR; failCount++;
                    it.info = "✗ " + err;
                }
                main.post(() -> { adapter.notifyDataSetChanged(); updateOverall(); });
            }
            converting = false;
            final int d = doneCount, f = failCount;
            main.post(() -> {
                overall.setText("全部完成 ✓" + d + " 失败 " + f);
                Toast.makeText(this, "完成：成功 " + d + "，失败 " + f, Toast.LENGTH_LONG).show();
            });
        }).start();
    }

    int doneCount, failCount;

    /** Returns null on success, error message otherwise. */
    String convertOne(RafItem it) {
        try {
            File raf = new File(getCacheDir(), "conv.raf");
            copyStream(getContentResolver().openInputStream(it.uri), raf);
            String base = it.name.replaceAll("\\.[rR][aA][fF]$", "");
            File dng = new File(getCacheDir(), base + "_merged.dng");
            String err = nativeConvert(raf.getAbsolutePath(), dng.getAbsolutePath(),
                    advStart(), advDelay(), advSmooth());
            if (err != null) { cleanup(raf); return err; }
            Uri savedDng = saveToDownloads(base + "_merged.dng", dng, "image/x-adobe-dng");
            boolean share = false;
            if (it.thumbJpeg != null) {
                share = makeShareJpeg(it, base);
            }
            cleanup(raf); cleanup(dng);
            if (savedDng == null) return "保存 DNG 到 Download 失败";
            it.shareSaved = share;
            return null;
        } catch (Exception e) {
            return e.getClass().getSimpleName() + ": " + e.getMessage();
        }
    }

    /** Downscaled (<=1600px) watermarked JPEG of the embedded preview. */
    boolean makeShareJpeg(RafItem it, String base) {
        try {
            BitmapFactory.Options o = new BitmapFactory.Options();
            o.inSampleSize = sampleFor(it.thumbJpeg, 1600);
            Bitmap b = BitmapFactory.decodeFile(it.thumbJpeg, o);
            if (b == null) return false;
            Bitmap work = b;
            if (b.getWidth() > 1600) {
                work = Bitmap.createScaledBitmap(b, 1600, 1600 * b.getHeight() / b.getWidth(), true);
            }
            Bitmap out = work.copy(Bitmap.Config.ARGB_8888, true);
            if (out != work) work = out;
            Canvas c = new Canvas(work);
            Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
            p.setColor(Color.WHITE);
            p.setShadowLayer(4, 2, 2, Color.argb(160, 0, 0, 0));
            p.setTextSize(Math.max(28, work.getWidth() / 26f));
            p.setTypeface(Typeface.DEFAULT_BOLD);
            String txt = "SuperCCD SR · S5ProDNG";
            float tw = p.measureText(txt);
            c.drawText(txt, work.getWidth() - tw - work.getWidth() * 0.03f,
                    work.getHeight() - work.getWidth() * 0.03f, p);
            File jpg = new File(getCacheDir(), base + "_share.jpg");
            try (FileOutputStream fos = new FileOutputStream(jpg)) {
                work.compress(Bitmap.CompressFormat.JPEG, 90, fos);
            }
            Uri u = saveToDownloads(base + "_share.jpg", jpg, "image/jpeg");
            cleanup(jpg);
            return u != null;
        } catch (Exception e) {
            return false;
        }
    }

    void cleanup(File f) { //noinspection ResultOfMethodCallIgnored
        f.delete(); }

    void updateOverall() {
        int done = 0, err = 0, total = items.size();
        for (RafItem it : items) { if (it.state == ST_DONE) done++; if (it.state == ST_ERROR) err++; }
        overall.setText("已添加 " + total + " · 完成 " + done + " · 失败 " + err
                + (converting ? " · 转换中…" : ""));
    }

    static void copyStream(InputStream in, File dst) throws IOException {
        try (OutputStream out = new FileOutputStream(dst)) {
            byte[] buf = new byte[1 << 20];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        } finally {
            try { in.close(); } catch (IOException ignored) { }
        }
    }

    /** Saves a file into MediaStore Downloads/s5dng/ (no permission needed on API 29+). */
    Uri saveToDownloads(String displayName, File src, String mime) throws IOException {
        ContentValues cv = new ContentValues();
        cv.put(MediaStore.Downloads.DISPLAY_NAME, displayName);
        cv.put(MediaStore.Downloads.MIME_TYPE, mime);
        cv.put(MediaStore.Downloads.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS + "/s5dng");
        cv.put(MediaStore.Downloads.IS_PENDING, 1);
        Uri uri = getContentResolver().insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, cv);
        if (uri == null) return null;
        try (OutputStream os = getContentResolver().openOutputStream(uri);
             InputStream in = new FileInputStream(src)) {
            byte[] buf = new byte[1 << 20];
            int n;
            while ((n = in.read(buf)) > 0) os.write(buf, 0, n);
        }
        cv.clear();
        cv.put(MediaStore.Downloads.IS_PENDING, 0);
        getContentResolver().update(uri, cv, null, null);
        return uri;
    }
}
