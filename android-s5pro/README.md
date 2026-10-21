# S5Pro DNG —— 富士 S3/S5 Pro RAF 解码器（安卓 App + 桌面 CLI）

把 GitHub 开源项目 superccd（唯一完整解码 SuperCCD SR「S+R 双平面」的转换器）移植成了
**无 Qt 依赖** 的 headless 核心，并套上安卓 App。**批量**选 RAF → 本地完成 S/R 合并 →
输出 Adobe DNG + 水印分享 JPEG，全程离线。

## App 功能（v1.1，对标 Contenta Converter 逆向调研的移动端落地）
- **批量队列**：系统文件器多选 RAF，逐项状态跟踪（排队/缩略图/转换中/完成/失败）
- **缩略图墙**：直接从 RAF 内嵌预览秒出小图（无需先解码 RAW）
- **EXIF 面板**：长按条目查看机型/镜头/ISO/光圈/快门/白平衡/动态范围参数
- **DNG 输出**：S/R 合并后 6MP CFA DNG，存 `Download/s5dng/<名>_merged.dng`（Lightroom/Snapseed 可直接编辑）
- **水印分享 JPEG**：每张同时输出 ≤1600px 带水印 JPEG（`<名>_share.jpg`），发圈免二次处理
- **S→R 合并曲线调节**：起点/宽度/平滑度三滑杆（对应 CLI `--start/--delay/--smoothness`）
- 全程离线 · 无任何第三方依赖 · API 29+（Android 10/11/12/13/14+）

## 本机验证结论（2026-10-21，Windows x64，MSYS2 GCC 16.2）

- `s5conv.exe`（2.7MB，静态链接）：`s5conv sample-full.RAF out.dng --6mp-cfa` → Conversion OK
- 合成 S5 Pro 样张（由 exiftool 测试库的真实 S5Pro RAF 头 + 双平面合成像素拼装，25MB×2 平面）
  端到端走通：RAF 解析 → S/R 读取 → 合并 → DNG 写出 → rawpy 再解码 → 渲染出经典的
  SuperCCD 45° 菱形构图（fuji_rotate 前）
- 验证脚本：`../build_sample.py`（造样张）、`../probe_raw.py`（LibRaw 校验）、`merged_render.png`（渲染证据）

## 目录

```
android-s5pro/
├── CMakeLists.txt          # 统一构建：Windows CLI / Android JNI / Termux
├── miniqt/                 # mini-Qt 兼容层（QString/QImage/QFile... -> STL + stb）
│   ├── miniqt.h/.cpp
│   ├── include/            # <QString> 等重定向桩
│   ├── s5conv_main.cpp     # 桌面 CLI 入口
│   └── s5_jni.cpp          # 安卓 JNI 桥
├── core/                   # superccd 原版核心（未改算法，仅换 include 路径即可编译）
├── third_party/            # LibRaw 0.21.5 / libtiff 4.7.0 / zlib 1.3.1 / stb（源码随工程编译）
├── android/                # Android 工程（minSdk 29，Java，无依赖）
│   ├── app/src/main/       # MainActivity（SAF 选文件 → 缓存 → nativeConvert → MediaStore 存 Download/s5dng/）
│   ├── app/build.gradle
│   └── settings.gradle
├── .github/workflows/      # 推到 GitHub 即自动出 APK
├── build-termux-native.sh  # 手机 Termux 本机编 .so
└── build-termux-apk.sh     # 手机 Termux 本机出 APK
```

## 三条出包通道（按省事程度排序）

### ① Windows 本机脚本（本项目已验证）
```powershell
# 需要 tools\android-build\{jdk, sdk}（见 SETUP-REPORT.md）
.\build-windows-apk.ps1      # NDK 编 .so → javac/d8 → aapt2 → 签名 → android\app\build\S5ProDNG.apk
```

### ② GitHub Actions
需要 token 带 workflow 权限：把仓库根 `.github/workflows/build-apk.yml` 推上去（存稿在 S5ProDNG-staging\）。

### ③ Termux 全手机自建
```bash
pkg install -y clang cmake ninja libraw libtiff openjdk-21 aapt2 apksigner
bash build-termux-native.sh && bash build-termux-apk.sh
```

### ④ Android Studio
打开 `android/`，等 Gradle sync 完成（NDK/CMake 由 IDE 自动装），Run。

## App 用法
1. 装 APK（arm64-v8a，Android 10+）
2. 「+ 选择 RAF」多选相机拷出的 `.RAF` → 缩略图墙自动生成，长按看 EXIF
3. 「转换全部」→ 每张输出 `Download/s5dng/<名>_merged.dng` + `<名>_share.jpg`（带水印分享版）
4. 「参数」里可调 S→R 合并曲线（起点/宽度/平滑度）
5. DNG 用 Lightroom Mobile / Snapseed 直接编辑（画面偏暗、45° 菱形构属 SuperCCD 原生几何，拉曝光+裁剪即可）

## 输出说明
- `*_merged.dng`：S/R 合并后的 6MP CFA DNG（宽动态数据完整保留）
- 画面默认偏暗（高光保护取向）、SuperCCD 原生 45° 旋转几何——Lightroom 里拉曝光+裁剪即可
  （或用仓库 docs/s3pro_dng.pp3 的 RawTherapee 预设做基准）
- CLI 参数：`--start= --delay= --smoothness= --r-luma-nr=` 微调 S→R 合并曲线（默认 0.75/0.20/0.65）

## 已知限制
- 合成样张验证了全链路机械正确性；真实相机 RAF 的最终确认需装机后用实拍文件
- R 平面高光合并的视觉调优（start/delay/smoothness）建议在真机 RAF 上微调
- exiftool 依赖路径在手机上不可用（EXIF 元数据改由 DngWriter 直写，不影响主流程）
