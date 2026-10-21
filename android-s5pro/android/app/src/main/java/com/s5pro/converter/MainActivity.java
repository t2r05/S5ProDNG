package com.s5pro.converter;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ContentValues;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.graphics.pdf.PdfDocument;
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
import android.widget.ArrayAdapter;
import android.widget.FrameLayout;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MainActivity extends Activity {

    private static final int PICK_RAF = 41;

    static { System.loadLibrary("s5dng"); }

    public native String nativeVersion();
    public native String nativeConvert(String inPath, String outPath,
                                       double start, double delay, double smoothness);
    public native boolean nativeExtractThumbnail(String inPath, String outJpegPath);
    public native String nativeReadMetadata(String inPath);
    public native boolean nativeRenderFujiJpeg(String inPath, String outJpegPath,
                                               int maxSize, int demosaic, boolean halfSize);

    static final int ST_PENDING = 0, ST_THUMB_OK = 1, ST_WORKING = 2, ST_DONE = 3, ST_ERROR = 4;

    static class RafItem {
        Uri uri; String name; long size;
        int state = ST_PENDING; String info = "排队中";
        String thumbJpeg;          // embedded preview, full extracted size (<=512)
        Bitmap rowThumb; String meta;
        boolean shareSaved;
        String fujiWmPath;         // watermarked fuji-render cache (for compare)
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
    android.widget.CheckBox fujiCheck;
    Spinner sizePreset;
    Spinner demoSpinner;
    Spinner filmSpinner;
    android.widget.CheckBox effBW, effSepia, effBright, effContrast, renameDate;
    android.widget.CheckBox halfCheck, dngCheck;
    Button exportPdf;
    final List<String> pdfSources = new ArrayList<>();
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
        fujiCheck = findViewById(R.id.fujiCheck);
        fujiCheck.setChecked(true);
        sizePreset = findViewById(R.id.sizePreset);
        ArrayAdapter<String> sa = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item,
                new String[]{"长边 1600px", "长边 1080px", "方图 1080×1080", "全尺寸"});
        sizePreset.setAdapter(sa);
        demoSpinner = findViewById(R.id.demoSpinner);
        ArrayAdapter<String> da = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item,
                new String[]{"解码管线：默认 AHD", "解码管线：PPG", "解码管线：VNG",
                        "解码管线：线性（最快）", "解码管线：DCB", "解码管线：DHT", "解码管线：AAHD"});
        demoSpinner.setAdapter(da);
        filmSpinner = findViewById(R.id.filmSpinner);
        ArrayAdapter<String> fa = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item,
                new String[]{"胶片模式：不套用", "胶片：Standard 标准", "胶片：F1 影棚人像",
                        "胶片：F1a 高饱和", "胶片：F1b 肤色平滑", "胶片：F1c 高对比锐利", "胶片：F2 Velvia 风"});
        filmSpinner.setAdapter(fa);
        halfCheck = findViewById(R.id.halfCheck);
        dngCheck = findViewById(R.id.dngCheck);
        dngCheck.setChecked(true);
        effBW = findViewById(R.id.effBW);
        effSepia = findViewById(R.id.effSepia);
        effBright = findViewById(R.id.effBright);
        effContrast = findViewById(R.id.effContrast);
        renameDate = findViewById(R.id.renameDate);
        exportPdf = findViewById(R.id.exportPdf);
        exportPdf.setOnClickListener(v -> exportContactSheet());
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

        list.setOnItemClickListener((parent, view, position, id) -> openCompare(items.get(position)));

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
            String base = outputBase(it);
            File dng = new File(getCacheDir(), base + "_merged.dng");
            if (!dngCheck.isChecked() && !fujiCheck.isChecked()) {
                cleanup(raf);
                return "DNG 与 JPEG 输出均未勾选";
            }
            if (dngCheck.isChecked()) {
                String derr = nativeConvert(raf.getAbsolutePath(), dng.getAbsolutePath(),
                        advStart(), advDelay(), advSmooth());
                if (derr != null) { cleanup(raf); return derr; }
                Uri savedDng = saveToDownloads(base + "_merged.dng", dng, "image/x-adobe-dng");
                if (savedDng == null) { cleanup(raf); return "保存 DNG 到 Download 失败"; }
            }
            boolean share = false;
            File fujiJpg = new File(getCacheDir(), base + "_fuji.jpg");
            if (fujiCheck.isChecked()) {
                // Fuji color mode: full LibRaw display render (camera WB +
                // dcraw S5 Pro matrix + sRGB + fuji_rotate), selectable
                // demosaic pipeline, then watermark.
                int edge = presetEdge() > 0 ? Math.max(presetEdge(), 1600) : 0;
                if (nativeRenderFujiJpeg(raf.getAbsolutePath(), fujiJpg.getAbsolutePath(),
                        edge, demosaicSel(), halfCheck.isChecked())) {
                    File out = new File(getCacheDir(), base + "_wm.jpg");
                    if (watermarkJpeg(fujiJpg, out)) {
                        Uri u = saveToDownloads(base + "_fuji.jpg", out, "image/jpeg");
                        share = (u != null);
                        if (share) {
                            pdfSources.add(out.getAbsolutePath());
                            it.fujiWmPath = out.getAbsolutePath();   // kept for compare
                        }
                    }
                }
                cleanup(fujiJpg);
            } else if (it.thumbJpeg != null) {
                share = makeShareJpeg(it, base);
                if (share) {
                    File wm = new File(getCacheDir(), base + "_wm.jpg");
                    if (wm.exists()) pdfSources.add(wm.getAbsolutePath());
                }
            }
            cleanup(raf); cleanup(dng);
            it.shareSaved = share;
            return null;
        } catch (Exception e) {
            return e.getClass().getSimpleName() + ": " + e.getMessage();
        }
    }

    /** Downscaled (<=1600px) watermarked JPEG of the embedded preview. */
    boolean makeShareJpeg(RafItem it, String base) {
        try {
            if (it.thumbJpeg == null) return false;
            File src = new File(it.thumbJpeg);
            File out = new File(getCacheDir(), base + "_wm.jpg");
            boolean ok = watermarkJpeg(src, out);
            if (!ok) return false;
            Uri u = saveToDownloads(base + "_share.jpg", out, "image/jpeg");
            return u != null;
        } catch (Exception e) {
            return false;
        }
    }

    String outputBase(RafItem it) {
        String base = it.name == null ? "DSCF" : it.name.replaceAll("\\.[rR][aA][fF]$", "");
        if (renameDate.isChecked() && it.meta != null) {
            Matcher m = Pattern.compile("\"dateTime\":\"([^\"]+)\"").matcher(it.meta);
            if (m.find()) {
                String dt = m.group(1).replaceAll("[^0-9]", "");
                if (dt.length() >= 14) base = "S5_" + dt.substring(0, 14);
            }
        }
        return base;
    }

    /** Preset long-edge target for the final JPEG; -1 = full size. */
    int presetEdge() {
        switch (sizePreset.getSelectedItemPosition()) {
            case 1: return 1080;
            case 2: return 1080;
            case 3: return -1;
            default: return 1600;
        }
    }

    boolean presetSquare() { return sizePreset.getSelectedItemPosition() == 2; }

    /** Film-simulation approximation matrix; null = no-op. Approximations of
     *  Standard / F1 / F1a / F1b / F1c / F2 (HS-V3 allows re-selecting film
     *  modes at development time; exact Fuji matrices are proprietary). */
    ColorMatrix filmMatrix() {
        ColorMatrix m = new ColorMatrix();
        int sel = filmSpinner.getSelectedItemPosition();
        if (sel <= 0) return null;
        ColorMatrix sat = new ColorMatrix();
        ColorMatrix tone = new ColorMatrix();
        ColorMatrix tint = null;
        switch (sel) {
            case 1: // Standard: identity (LibRaw render is already the standard look)
                return null;
            case 2: // F1 studio-portrait chrome: gently muted, warm skin, soft contrast
                sat.setSaturation(0.88f);
                tone.setScale(0.96f, 0.96f, 0.96f, 1f);
                tint = new ColorMatrix(new float[]{
                        1, 0, 0, 0, 6,
                        0, 1, 0, 0, 2,
                        0, 0, 1, 0, -4,
                        0, 0, 0, 1, 0});
                break;
            case 3: // F1a: more saturation, slight contrast
                sat.setSaturation(1.18f);
                tone.setScale(1.06f, 1.06f, 1.06f, 1f);
                tint = new ColorMatrix(new float[]{
                        1, 0, 0, 0, -8,
                        0, 1, 0, 0, -8,
                        0, 0, 1, 0, -8,
                        0, 0, 0, 1, 0});
                break;
            case 4: // F1b daylight-portrait: smoother/lower contrast, gentle warm
                sat.setSaturation(0.95f);
                tone.setScale(0.92f, 0.92f, 0.92f, 1f);
                tint = new ColorMatrix(new float[]{
                        1, 0, 0, 0, 12,
                        0, 1, 0, 0, 4,
                        0, 0, 1, 0, 0,
                        0, 0, 0, 1, 0});
                break;
            case 5: // F1c: contrasty/sharp look
                sat.setSaturation(1.05f);
                tone.setScale(1.10f, 1.10f, 1.10f, 1f);
                tint = new ColorMatrix(new float[]{
                        1, 0, 0, 0, -14,
                        0, 1, 0, 0, -14,
                        0, 0, 1, 0, -14,
                        0, 0, 0, 1, 0});
                break;
            case 6: // F2 Velvia-like: punchy
                sat.setSaturation(1.35f);
                tone.setScale(1.12f, 1.12f, 1.12f, 1f);
                tint = new ColorMatrix(new float[]{
                        1, 0, 0, 0, -16,
                        0, 1, 0, 0, -16,
                        0, 0, 1, 0, -16,
                        0, 0, 0, 1, 0});
                break;
        }
        m.postConcat(sat);
        m.postConcat(tone);
        if (tint != null) m.postConcat(tint);
        return m;
    }

    /** Combined ColorMatrix from the quick-effect checkboxes (composed with
     *  the film preset); null = no-op. */
    ColorMatrix effectMatrix() {
        ColorMatrix m = new ColorMatrix();
        if (effBW.isChecked()) m.setSaturation(0f);
        if (effSepia.isChecked()) {
            ColorMatrix tone = new ColorMatrix();
            tone.setScale(1.2f, 1.0f, 0.78f, 1f);
            m.postConcat(tone);
            ColorMatrix sat = new ColorMatrix();
            sat.setSaturation(0.65f);
            m.postConcat(sat);
        }
        if (effBright.isChecked()) {
            ColorMatrix b = new ColorMatrix();
            b.setScale(1.25f, 1.25f, 1.25f, 1f);
            m.postConcat(b);
        }
        if (effContrast.isChecked()) {
            ColorMatrix c = new ColorMatrix(new float[]{
                    1.18f, 0, 0, 0, -24,
                    0, 1.18f, 0, 0, -24,
                    0, 0, 1.18f, 0, -24,
                    0, 0, 0, 1, 0});
            m.postConcat(c);
        }
        ColorMatrix film = filmMatrix();
        if (film != null) m.postConcat(film);
        return m;
    }

    /** Decodes src with preset size/effects, draws the watermark, writes JPEG. */
    boolean watermarkJpeg(File src, File out) {
        try {
            int edge = presetEdge() > 0 ? presetEdge() : 4096;
            BitmapFactory.Options o = new BitmapFactory.Options();
            o.inSampleSize = sampleFor(src.getAbsolutePath(), edge);
            Bitmap b = BitmapFactory.decodeFile(src.getAbsolutePath(), o);
            if (b == null) return false;
            Bitmap work = b;
            int edgeLimit = presetEdge() > 0 ? presetEdge() : Math.max(b.getWidth(), b.getHeight());
            if (presetSquare()) {
                int side = Math.min(b.getWidth(), b.getHeight());
                int x0 = (b.getWidth() - side) / 2, y0 = (b.getHeight() - side) / 2;
                work = Bitmap.createBitmap(b, x0, y0, side, side);
                if (side > edgeLimit) {
                    Bitmap s2 = Bitmap.createScaledBitmap(work, edgeLimit, edgeLimit, true);
                    if (s2 != work) { work = s2; }
                }
            } else if (b.getWidth() > edgeLimit || b.getHeight() > edgeLimit) {
                work = Bitmap.createScaledBitmap(b, edgeLimit, edgeLimit * b.getHeight() / b.getWidth(), true);
            }
            Bitmap outB = work.copy(Bitmap.Config.ARGB_8888, true);
            Canvas c = new Canvas(outB);
            Paint draw = new Paint(Paint.FILTER_BITMAP_FLAG | Paint.ANTI_ALIAS_FLAG);
            ColorMatrix cm = effectMatrix();
            if (cm != null) draw.setColorFilter(new ColorMatrixColorFilter(cm));
            c.drawBitmap(work, 0, 0, draw);
            Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
            p.setColor(Color.WHITE);
            p.setShadowLayer(4, 2, 2, Color.argb(160, 0, 0, 0));
            p.setTextSize(Math.max(28, work.getWidth() / 26f));
            p.setTypeface(Typeface.DEFAULT_BOLD);
            String txt = "SuperCCD SR · S5ProDNG";
            float tw = p.measureText(txt);
            c.drawText(txt, work.getWidth() - tw - work.getWidth() * 0.03f,
                    work.getHeight() - work.getWidth() * 0.03f, p);
            try (FileOutputStream fos = new FileOutputStream(out)) {
                outB.compress(Bitmap.CompressFormat.JPEG, 90, fos);
            }
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    int demosaicSel() {
        switch (demoSpinner.getSelectedItemPosition()) {
            case 1: return 2;   // PPG
            case 2: return 1;   // VNG
            case 3: return 0;   // linear
            case 4: return 4;   // DCB
            case 5: return 11;  // DHT
            case 6: return 12;  // AAHD
            default: return -1; // LibRaw default (AHD)
        }
    }

    // ---------------- before/after compare ----------------

    void openCompare(RafItem it) {
        if (it.thumbJpeg == null || it.fujiWmPath == null) {
            Toast.makeText(this, "对比需要：已生成预览 + 已完成富士色彩转换", Toast.LENGTH_SHORT).show();
            return;
        }
        Bitmap before = decodeScaled(it.thumbJpeg, 1080);
        Bitmap after = decodeScaled(it.fujiWmPath, 1080);
        if (before == null || after == null) {
            Toast.makeText(this, "图片解码失败", Toast.LENGTH_SHORT).show();
            return;
        }
        int w = Math.min(before.getWidth(), after.getWidth());
        int h = Math.min(before.getHeight(), after.getHeight());
        before = Bitmap.createScaledBitmap(before, w, h, true);
        after = Bitmap.createScaledBitmap(after, w, h, true);

        final FrameLayout fl = new FrameLayout(this);
        ImageView ivB = new ImageView(this);
        ivB.setImageBitmap(before);
        ivB.setScaleType(ImageView.ScaleType.FIT_CENTER);
        ImageView ivA = new ImageView(this);
        ivA.setImageBitmap(after);
        ivA.setScaleType(ImageView.ScaleType.FIT_CENTER);
        fl.addView(ivB, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        fl.addView(ivA, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        LinearLayout labels = new LinearLayout(this);
        labels.setOrientation(LinearLayout.HORIZONTAL);
        TextView lb = new TextView(this);
        lb.setText("左：内嵌预览"); lb.setTextColor(Color.WHITE); lb.setPadding(8, 4, 8, 4);
        TextView la = new TextView(this);
        la.setText("右：富士渲染"); la.setTextColor(Color.WHITE); la.setPadding(8, 4, 8, 4);
        la.setGravity(1);
        labels.addView(lb, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        labels.addView(la, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));

        SeekBar sb = new SeekBar(this);
        sb.setMax(100);
        sb.setProgress(50);

        LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.VERTICAL);
        box.addView(fl, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, (int) (getResources().getDisplayMetrics().widthPixels * 0.66f)));
        box.addView(labels, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        box.addView(sb, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        sb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar s, int prog, boolean fromUser) {
                float frac = prog / 100f;
                ivA.post(() -> ivA.setClipBounds(new Rect(0, 0,
                        (int) (ivA.getWidth() * frac), ivA.getHeight())));
            }
            @Override public void onStartTrackingTouch(SeekBar s) { }
            @Override public void onStopTrackingTouch(SeekBar s) { }
        });
        ivA.post(() -> ivA.setClipBounds(new Rect(0, 0, ivA.getWidth() / 2, ivA.getHeight())));

        new AlertDialog.Builder(this)
                .setTitle("前后对比 · " + it.name)
                .setView(box)
                .setPositiveButton("关闭", null)
                .show();
    }

    Bitmap decodeScaled(String path, int target) {
        BitmapFactory.Options o = new BitmapFactory.Options();
        o.inSampleSize = sampleFor(path, target);
        Bitmap b = BitmapFactory.decodeFile(path, o);
        if (b != null && b.getWidth() > target) {
            b = Bitmap.createScaledBitmap(b, target, target * b.getHeight() / b.getWidth(), true);
        }
        return b;
    }

    void exportContactSheet() {
        if (pdfSources.isEmpty()) {
            Toast.makeText(this, "先转换出 JPEG，再导出接触表", Toast.LENGTH_SHORT).show();
            return;
        }
        Toast.makeText(this, "正在生成接触表…", Toast.LENGTH_SHORT).show();
        new Thread(() -> {
            String err = null;
            try {
                File pdf = new File(getCacheDir(), "sheet.pdf");
                PdfDocument doc = new PdfDocument();
                final int W = 595, H = 842, perPage = 6;
                final int cellW = W / 3, cellH = (H - 40) / 2;
                Paint head = new Paint(Paint.ANTI_ALIAS_FLAG);
                head.setColor(Color.BLACK); head.setTextSize(11); head.setTypeface(Typeface.DEFAULT_BOLD);
                Paint cap = new Paint(Paint.ANTI_ALIAS_FLAG);
                cap.setColor(Color.DKGRAY); cap.setTextSize(7);
                int pages = (pdfSources.size() + perPage - 1) / perPage;
                SimpleDateFormat f = new SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.US);
                for (int pg = 0; pg < pages; pg++) {
                    PdfDocument.Page page = doc.startPage(
                            new PdfDocument.PageInfo.Builder(W, H, pg + 1).create());
                    Canvas c = page.getCanvas();
                    c.drawText("S5ProDNG Contact Sheet · " + f.format(new Date())
                            + " · " + (pg + 1) + "/" + pages, 12, 18, head);
                    for (int k = 0; k < perPage; k++) {
                        int idx = pg * perPage + k;
                        if (idx >= pdfSources.size()) break;
                        int cx = (k % 3) * cellW, cy = 28 + (k / 3) * cellH;
                        Bitmap b = decodeFit(pdfSources.get(idx), cellW - 12, cellH - 26);
                        if (b != null) {
                            c.drawBitmap(b, null, new RectF(cx + 6, cy + 4,
                                    cx + cellW - 6, cy + cellH - 22), null);
                            b.recycle();
                        }
                        c.drawText(new File(pdfSources.get(idx)).getName(),
                                cx + 8, cy + cellH - 8, cap);
                    }
                    doc.finishPage(page);
                }
                try (FileOutputStream fos = new FileOutputStream(pdf)) {
                    doc.writeTo(fos);
                }
                doc.close();
                Uri u = saveToDownloads("S5ProDNG_contact_"
                        + (System.currentTimeMillis() / 1000) + ".pdf", pdf, "application/pdf");
                cleanup(pdf);
                if (u == null) err = "保存 PDF 失败";
            } catch (Exception e) {
                err = e.getClass().getSimpleName() + ": " + e.getMessage();
            }
            final String e2 = err;
            main.post(() -> Toast.makeText(this,
                    e2 == null ? "接触表已存 Download/s5dng/" : "PDF 失败: " + e2,
                    Toast.LENGTH_LONG).show());
        }).start();
    }

    Bitmap decodeFit(String path, int tw, int th) {
        BitmapFactory.Options o = new BitmapFactory.Options();
        o.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(path, o);
        int s = 1;
        while (o.outWidth / (s * 2) >= tw * 2 && o.outHeight / (s * 2) >= th * 2) s *= 2;
        o = new BitmapFactory.Options();
        o.inSampleSize = s;
        return BitmapFactory.decodeFile(path, o);
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
