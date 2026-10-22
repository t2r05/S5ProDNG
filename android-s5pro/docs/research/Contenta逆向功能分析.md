# Contenta Converter PREMIUM 9.0 逆向调研 —— 功能矩阵与 APK 借鉴

> 2026-10-21。目的：为 S5ProDNG 安卓 App 后续功能演进做参照。
> 来源：官网功能页/格式页（contenta-converter.com）+ 安装包元数据（Inno Setup 9.0.43，ContentaSoft AB 签名，微软 ID 验证）。
> 说明：只做功能层面的学习梳理，不涉及破解其授权。

## 1. 产品定位
- Windows 10/11 桌面批量图像转换器；免费试用 + **一次性买断**（无订阅）
- 卖点口号：批量省时、102 种输入格式、离线可用（AI/云功能除外）

## 2. 输入格式矩阵（官方口径：102 扩展名 / 21 族）
| 族 | 扩展名 |
|---|---|
| RAW（35 个） | 3fr arw cr2 cr3 crw dcr dng erf iiq kdc mef mos mrw nef nrw orf pef ptx pxn raf raw rw2 rwl sr2 srf srw thm x3f fff mdc |
| 标准 | jpg jpeg jif jxr png tif tiff bmp gif webp tga exr hdr ico wbmp xpm pbm pgm ppm targa thumb |
| 小众 | pcx dds iff lbm pnm sgi rgb jng mng xbm cut rle |
| JPEG 2000 | jp2 j2k jpf jpx j2c |
| 遗留 | pict pct sun ras pic |
| 元文件 | wmz emz emf wmf |
| HEIC/AVIF | heic heif avif |
| 文档 | pdf qrp ai |
| 立体 | jps pns mpo |
| Photoshop | psd psb |
| PostScript | eps ps |
| SVG | svg svgz |
| DjVu | djvu djv |
| JPEG XL | jxl |
| MrSID | sid |
| Apple | ithmb |
| SketchUp | skm | 
| Photo CD | pcd |
| Paint.NET | pdn |
| 传真 | （fax 类） |
| GIMP | xcf |

**输出**：jpg/jpeg/jif/png/webp/avif/heic/heif/jxl/gif ｜ tif/tiff/pdf/eps/jp2/jxr ｜ bmp/tga/wbmp/ppm/pgm/pbm/xpm/exr/hdr ｜ svg/ico

## 3. 功能清单（官方宣传的功能面）
1. **批量转换/缩放/重命名/特效**：整文件夹队列处理
2. **平台预设导出**：Amazon、Instagram、Zillow 等 20+ 平台一键尺寸优化
3. **AI 抠图**：一键去背景/换背景，支持文字提示改图（联网能力）
4. **水印**：文字/Logo，批量，可控位置/透明度/大小
5. **PDF 相册/接触表**：每页 1-48 张排版；多页 PDF 拆分为单图
6. **RAW 处理**：30+ RAW 格式，白平衡/降噪/色彩控制
7. **特效管线**：35+ 效果可链式批量（模糊/锐化/黑白/棕褐/亮度/对比度…）
8. **元数据**：EXIF/IPTC/XMP 读写，批量版权/作者/关键词
9. **云上传/交付**：FTP、SFTP、WordPress、Dropbox、Google Drive；ZIP 打包发邮件

## 4. 技术栈线索（已证实/推断）
- 安装器：Inno Setup（Delphi 生态），主程序大概率 Delphi/C++ 混合（字符串见 Delphi RTTI）
- 签名：ContentaSoft AB（瑞典于默奥），Microsoft ID Verified 证书
- WebSetup 桩仅 2.1MB，真实负载走在线下载；完整离线包 224MB（含全部格式引擎/配置）
- RAW 引擎未开包确认（Defender 拦截了 Inno 解包工具）；从格式广度推测底层是 dcraw/LibRaw 系 + 多解码器聚合（与我们的技术选型同源）

## 5. 对 S5ProDNG APK 的功能路线启示
按「贴合我们核心价值（S5 Pro 解码）+ 移动端可行性」排序：

### 近期（直接可抄的思路）
1. **批量队列 + 缩略图墙**：RAF 导入后用内嵌 JPEG 预览秒出缩略图墙，点选批量转 DNG（我们的 miniqt shim 已有 QImage 缩放能力）
2. **EXIF 面板**：LibRaw 解出的 ISO/光圈/快门/镜头直接展示（metadata 已在 SuperCCDMetadata 里）
3. **输出预设**：除 DNG 外加「JPEG 分享版」一键导出（手机 Lightroom 兼容场景）
4. **水印**：转 JPEG 分享版时叠加文字水印（安卓 Canvas 十行代码）

### 中期（差异化）
5. **S→R 合并曲线调节 UI**：把 --start/--delay/--smoothness 做成滑杆+实时预览（桌面版有，移动端无人做）
6. **批量重命名模板**：拍摄日期+序号
7. **PDF 接触表**：一次拍摄出一张联系表（Android PdfDocument 原生支持）

### 观望（成本/收益低）
- AI 抠图、云上传、30+ RAW 全家桶：与我们的定位（S5 Pro 专精）无关，不跟

## 6. 二进制层深挖（待办）
- 完整离线包（224MB）下载中；到手后用纯 Python 解析 Inno 头部文件表（Defender 拦截了 inno 提取工具，绕开二进制工具链）
- 可确认：RAW 引擎构成、Qt/Delphi 运行时、格式插件布局
