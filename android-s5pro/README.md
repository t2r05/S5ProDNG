# S5Pro DNG —— 富士 S3/S5 Pro RAF 解码器（安卓 App + 桌面 CLI）

把 GitHub 开源项目 superccd（唯一完整解码 SuperCCD SR「S+R 双平面」的转换器）移植成了
**无 Qt 依赖** 的 headless 核心，并套上安卓 App。手机上选 RAF → 本地完成 S/R 合并 →
输出 Adobe DNG，全程离线。

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

### ① GitHub Actions（推荐，零本地环境）
```bash
cd android-s5pro
git init && git add -A && git commit -m "s5dng"
# 建私有仓库后：
git remote add origin https://github.com/<你>/<repo>.git
git push -u origin main        # Actions 自动构建，Artifact 里拿 APK
```
注：third_party 体积约 60MB（LibRaw/libtiff 源码），普通仓库可容纳。

### ② Termux 全手机自建
```bash
pkg install -y clang cmake ninja libraw libtiff openjdk-21 aapt2 apksigner dz
# 1) 原生库
bash build-termux-native.sh
# 2) APK
bash build-termux-apk.sh
```

### ③ Android Studio
打开 `android/`，等 Gradle sync 完成（NDK/CMake 由 IDE 自动装），Run。

## App 用法
1. 装 APK（arm64-v8a，Android 10+）
2. 「选择 RAF 文件」→ 系统文件器里挑相机拷出的 `.RAF`
3. 等待（旗舰机约 10-30 秒/张）→ DNG 存到 `Download/s5dng/<原名>_merged.dng`
4. Lightroom Mobile / Snapseed 直接打开编辑

## 输出说明
- `*_merged.dng`：S/R 合并后的 6MP CFA DNG（宽动态数据完整保留）
- 画面默认偏暗（高光保护取向）、SuperCCD 原生 45° 旋转几何——Lightroom 里拉曝光+裁剪即可
  （或用仓库 docs/s3pro_dng.pp3 的 RawTherapee 预设做基准）
- CLI 参数：`--start= --delay= --smoothness= --r-luma-nr=` 微调 S→R 合并曲线（默认 0.75/0.20/0.65）

## 已知限制
- 合成样张验证了全链路机械正确性；真实相机 RAF 的最终确认需装机后用实拍文件
- R 平面高光合并的视觉调优（start/delay/smoothness）建议在真机 RAF 上微调
- exiftool 依赖路径在手机上不可用（EXIF 元数据改由 DngWriter 直写，不影响主流程）
