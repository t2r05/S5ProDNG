# 富士 FinePix S5 Pro（SuperCCD SR）官方 RAW 转换器演进考据

调查日期：2026-10-22
方法：全部网页结论基于本 run 的 fetch_content 抓取或 web_search 搜索摘要，来源分级标注：**[已抓取]**=全文抓取验证；**[摘要]**=搜索结果摘要，未整页抓取；**[推断]**=由证据推理，非直接陈述。引文保留原文（英/日/中/德），附 URL。

---

## 一、结论速览：演进时间线

| 时间 | 软件 | 内核引擎 | 定位 | S5 Pro | 今天官方下载 |
|---|---|---|---|---|---|
| 2002 | RAW File Converter LE | 未证实（详见下文，**几乎不可能是 SILKYPIX**） | 随机附赠的拖放式简易转换器 | ✓（随机 CD） | ✗ 无下载页 |
| 2004（随 S3 Pro） | Hyper-Utility HS-V3 | 富士自家（内含模块名就叫 **RAW File Converter EX**；新版为 SR II 加入 **3D-LUT**） | 付费/另购高级套件：16-bit RAW 转换 + 相机遥控 | ✓（RAW+遥控） | ✗ 无下载页，仅存于随机/零售 CD |
| 2005-11 发表、2007 春出货 | （S5 Pro 机身：SuperCCD SR II，14-bit，DR 100–400%） | — | — | — | — |
| ~2010 | RAW File Converter EX powered by SILKYPIX（EX 1.0，SILKYPIX 3 内核） | SILKYPIX（市川软件实验室 Ichikawa Soft Laboratory） | 官网**免费下载**，取代 HS-V3 的转换职能 | ✓ | **✓ 仍在，持续维护** |
| ~2013–2018 | EX 2.0（Ver 4.x） | SILKYPIX | 同上 | ✓ | ✗ 官方声明停止支持 |
| 2017-11（Mac）/ 2018-02（Win） | FUJIFILM X RAW Studio | **相机硬件**（X-Processor Pro，需 USB 连机） | 借用现役 X 系机身处理 | **✗ 不支持** | ✓（但与 S5 Pro 无关） |
| 2018-11 起 | **RAW FILE CONVERTER EX 3.0**（Ver 8.x） | SILKYPIX 8/10 系 | 官网免费下载，现役 | **✓** | **✓ 最新 Ver.8.1.20.0（2026-03-02）** |

**2026 年今天，S5 Pro 用户可用的官方路线只有两条：**
1. **RAW FILE CONVERTER EX 3.0 powered by SILKYPIX**（免费）；
2. SILKYPIX Developer Studio Pro12 for FUJIFILM（付费 ¥6,930，Windows 专用，自称 Based on RAW FILE CONVERTER EX 3.0，兼容表含 S2Pro/S3Pro/S5Pro）。

---

## 二、逐款详解

### 1. RAW File Converter LE（随机附赠，~2002 起）

- **形态**：拖放式简易批处理转换器。dpreview S2 Pro 评测（2002-08-03）[已抓取]：「To use RAW File Converter LE simply drag and drop an image or group of images onto the application then click on 'Convert'」——https://www.dpreview.com/reviews/6679467365/fujis2pro
- **随机附赠身份**：台湾富士总代理页 [已抓取] 在描述 HS-V3 时明确对比：「RAW File Converter EX 提供多種高級功能（**與機器附贈 LE 版本不同**）」；富士 S9100/S9600 说明书 [摘要] 写明随机软件含 LE（「Minimum 256 MB when RAW FILE CONVERTER LE is used」）。LE 是 2002–2008 年富士全线（单反+消费机）的入门捆绑件。
- **内核**：任务假设「SILKYPIX 内核」**未能证实**。专门搜索零结果；且时间线矛盾——S2 Pro 的 LE（2002）早于 SILKYPIX 1.0（2004，ISL 手册版权行「Ichikawa Soft Laboratory 2004-」佐证）。**结论：S2 时代的 LE 几乎不可能是 SILKYPIX，按存疑处理**。[推断]
- **今天下载**：无官方下载页（CD 时代软件，fujifilm.com/fujifilm-x.com 均未找到）。空结果。

### 2. Hyper-Utility HS-V3（S3/S5 Pro 时代高级套件，2004–2008）

- **版本谱系**（德国存档站 troeszter.net 全矩阵表 [已抓取]，https://www.troeszter.net/Fujisupport-HyperUtils.html ）：
  `HS-S2（S2 Pro 时代）→ HS-V2 Ver.2.0 → HS-V2 Ver.3.0 → HS-V2 Ver.3.1 → HS-V3 / HS-V3UP`
  矩阵关键行：「FinePix S5Pro：HS-V2 各版本 No/No，**HS-V3/HS-V3UP Ja(Ja)**」——**S5 Pro 只能由 HS-V3/HS-V3UP 处理**；S3 Pro 需 HS-V2 v3.1 起或 HS-V3。
- **内核与组成**：台湾富士总代理商品页（2008-01-14 更新，[已抓取]，https://www.fuji.com.tw/dscacc.asp?AID=4105 ）：
  > 「HS-V3 中包括 **RAW File Converter EX** 和 Camera Shooting，使您可用電腦隨意處理 CCD-RAW 檔和控制相機功能。RAW File Converter EX 提供多種高級功能（與機器附贈 LE 版本不同），可指定檔轉換過程中的參數，如**色調曲線，白平衡，清晰度，飽和度及亮度**等，還可輸出 **16BIT 的 TIFF** 檔……」
  > 「新版的 FUJIFILM 原廠 HS-V3 搖控瀏覽擷取軟體加入了專為**第 4 代 SUPER CCD SR II** 開發的 **3D-LUT 技術**，讓色彩真實呈現，影像細節更精細。」
  → 两个关键事实：①「RAW File Converter EX」这个名字**最早是 HS-V3 套件内的转换模块名**，2009/2010 年起的免费独立版是同名续任者；②官方针对 SR II 的色彩技术是 **3D-LUT**（立体查找表），这是「富士官方色」的硬件级渲染管线，第三方引擎只有标准色彩矩阵。
- **模块兼容表**（同页 [已抓取]）：HS-V3 内含的 RAW File Converter EX 支持 S5pro / S3 Pro / S2 Pro / S20 Pro / S7000 / S5000 / S5500 / S5100 / F710 / F810 / F700 / E550；Camera Shooting 遥控仅 S5pro / S3 Pro / S2 Pro / S20 Pro。
- **随机附送还是另购**：**按市场而异，无法确证美版标配**。
  - 意大利官网配件页 [摘要]：「HS-V3 Software … Hyper-Utility Software di conversione file RAW / **16 bits per S5Pro**」——作为 S5 Pro 配件零售（https://www.fujifilm.it/aree/prodotti/accessori.asp?cod=35871 ）。
  - B&H 零售页存在 [摘要，商品页被 bot 验证墙挡住]：https://www.bhphotovideo.com/c/product/477338-REG/Fujifilm_600006016_Hyper_Utility_Software_HS_V3.html ，摘要含「specify output image size and processing parameters (tone curve, white balance, sharpness, color and dynamic…)」。
  - dpreview S5 Pro 评测（2007-07-05，[已抓取]）规格表 Remote 一栏：「USB 2.0 remote control using **optional software**」；论坛亦有用户问「Does HYPER UTILITY HS-V3 come with Fuji S5? If it's an option, where can I buy it」——美版更像另购。台湾总代理页状态：「產品已暫停銷售/停止銷售/停產」。
  - 零售价：**未找到**确切数字（两轮搜索零命中），未知。
- **官方现状**：现行官网无 HS-V3 下载页（site:fujifilm.com 搜索零结果）。S5 Pro 官方固件页是唯一官方提及处 [已抓取]：https://www.fujifilm-x.com/en-us/support/download/firmware/cameras/s5-pro ，固件日志多条与 HS-V3 相关（Ver1.09「改善 HS-V3/HS-V3 UP 下的 live view 画质」、Ver1.12 修「HS-V3 内含的 PC controlling software 导致死机」）。
- **今天还能用吗**：dpreview 2019 对比帖楼主 EE-TV 在现代 Windows 上需以 XP/Vista 兼容模式运行 HS-V3 [已抓取]。

### 3. RAW File Converter EX powered by SILKYPIX（★ S5 Pro 今天的主答案）

- **三个官方 URL（本次全部整页抓取验证，2026-10 仍存活）**：
  - 下载页（EX 3.0）：https://www.fujifilm-x.com/en-us/support/download/software/raw-file-converter-ex-powered-by-silkypix
  - 兼容表：https://www.fujifilm-x.com/en-us/support/compatibility/software/raw-file-converter-ex-powered-by-silkypix
  - 产品页：https://www.fujifilm-x.com/en-us/products/software/raw-file-converter-ex-powered-by-silkypix
- **「还能下载」的正面回答**：**能，且 2026 年仍在更新**。下载页 [已抓取] 显示最新 **Ver.8.1.20.0**，新闻日期 **2026-03-02**（美式日期 03.02.2026，新增 X-T30 III 支持），Mac 安装包 RFCEX30Ins81200.dmg；页首声明「Support for the RAW FILE CONVERTER EX or EX2.0 … **has been discontinued**」——EX 1.x/2.0 已停支持，仅 EX 3.0 现役。软件方标注 TECHNO HORIZON CO., LTD.（SILKYPIX 经销）。系统要求 Windows 11/10、macOS 10.13–26。
- **S5 Pro 支持的直接证据**：兼容表 [已抓取] 明确列出 FinePix **S5 Pro ✓、S3 Pro ✓、S2 Pro ✓**（另有 S100FS、HS/EXR 系列与全部 X/GFX 机型）。
- **版本沿革与断代证据**：
  - EX 1.0（约 2010，**SILKYPIX 3 内核**）：dpreview 2010-12-16 帖 [摘要]「Silkypix Raw file converter from Fuji website now supports Tiff conversion too」；fujix-forum 2013-05 帖 [摘要]「Raw File Converter EX (Silkypix 3)」。
  - EX 2.0（Ver 4.x）：fujiaddict 2018-08-24 [摘要]「EX 2.0 Ver 4.3.2.0 adds XF10」。
  - EX 3.0（Ver 8.x，2018-11 起）：fujiaddict 2018-11-30 [摘要]（EX 3.0 与 X RAW Studio 等一同加 GFX50R 支持）；下载页版本日志 [已抓取] 8.0.1.0 配合 GFX50R/Mojave → 8.1.x 逐年加新机型 → 8.1.20.0（2026-03-02）。
- **付费正统续任**：SILKYPIX Developer Studio Pro12 for FUJIFILM [已抓取]，https://www.silkypix.com/en/ext/special/dsp12fuji ——Windows 专用，¥6,930，30 天试用；支持列表含 **S2Pro/S3Pro/S5Pro**；FAQ 明确**可继承 EX 3.0 的调整参数**；自称「Based on RAW FILE CONVERTER EX 3.0」。旧付费版 dsp9fuji 存在于 silkypix.isl.co.jp（该页本次 fetch 失败一次，仅搜索摘要可用）。

### 4. FUJIFILM X RAW Studio（与 S5 Pro 无关）

- **原理**：通过 USB 连接**现役 X 系机身**，用相机内 **X-Processor Pro** 硬件加速处理 RAW——相机本身才是引擎（fujirumors 发布公告 [摘要]；dpreview 2024-02 帖 [摘要] 确认需连机）。
- **发布**：2017-11-29 Mac 首发（配合 X-T2/X-T20/GFX50S 固件，fujiaddict [摘要]）；2018-02-28 Windows v1.1（fujirumors [摘要]）。
- **S5 Pro 不支持（用户假设已证实）**：官方兼容表 [已抓取] https://www.fujifilm-x.com/en-us/support/compatibility/software/x-raw-studio 机型仅 X/GFX（X-H2S…X100F、GFX100 II…），**无任何 FinePix**。dpreview 用户 fPrime（2021-08-29）[已抓取] 也曾误会后自行引用该兼容表更正：「Unfortunately it doesn't support the older SuperCCD cameras」。
- **下载页**：https://www.fujifilm-x.com/en-us/support/download/software/x-raw-studio [摘要，未整页抓取]。

---

## 三、「新软件颜色更好」的技术原因

### 1. 根本前提：RAF 是「双帧」文件，第三方引擎先天吃亏

SuperCCD SR II 每个像点有两颗光电二极管：S 像素（常规灵敏度）+ R 像素（低灵敏度、专捕高光），各 6.17MP、14-bit（dpreview 评测 [已抓取]：「CCD-RAW (14 bit) – S only mode」「CCD-RAW (14 bit x2) – S+R mode」）。因此 RAF 实际携带**两套图像数据**，而通用引擎只读懂一套。pixls 论坛 JoaCHIP [已抓取，https://discuss.pixls.us/t/rawtherapee-for-raf-files-of-fuji-s5-pro/3458 ]：

> 「Fuji S5/S3 Pro raw files have **two sets of images**… RT processes only the first one」
> 「neither dcraw or Adobe Lightroom/Camera Raw have understood the raw files of this camera well enough to even match the JPEG output… If you have a Fuji S2/S3/S5 pro, **you have to use the software that comes with the camera**… probably because Fuji are the only ones who know the actual sensor design」

（他引用的传感器拆包分析页 www.robotplanet.dk/photo/fuji-raw-format/ 本次两次抓取失败，标记不可用。）

第三方便件也在撤离：DxO 官方声明（dpreview 帖内引用 [已抓取]）：「The support of RAW file for Fuji S3, S5, and S100FS (Super CCD) is no longer ensured in DxO PhotoLab (…to avoid the maintenance of **2 different RAW engines**…)」——维护 SuperCCD 需要独立引擎，侧面证明其特殊性。

### 2. 官方/持牌引擎独占的「富士色彩科技」

- **3D-LUT 色彩管线**：HS-V3 新版「专为第 4 代 SuperCCD SR II 开发 3D-LUT 技术」（台湾总代理页 [已抓取]，见上文引文）。3D 查找表对 S+R 合成后的色彩做立体映射，第三方引擎只拿得到常规 3×3 色彩矩阵级别的近似。
- **持牌继承**：dpreview 2023-08-22 帖 JNR [已抓取，https://www.dpreview.com/forums/threads/why-not-use-raw-file-converter-ex-3-0-powered-by-silkypix.4724766 ]：「Bottom line, **only Silkypix and Capture One have access to the Fuji color technology**」。富士把色彩技术授权给了 SILKYPIX（EX 系列与 Pro for FUJIFILM 付费版）与 Capture One，Adobe 不在此列。

### 3. R 像素与 DR 100%–400%：肤色密度的来源

- S5 Pro 机身提供 6 档 DR：100 / 130 / 170 / 230 / 300 / 400%（dpreview 评测规格表 [已抓取]）。
- DR 可在转换器里**后期重设**：photo.net frank_bunnik [已抓取，https://www.photo.net/discuss/threads/fujifilm-finepix-s5-pro-raw-or-jpeg-workflow-lightroom-or-hyper-utility.297693 ]：「The software Fuji provides with the camera will let you **change the dynamic range from 0 to 400% afterwards**, provided you shot in raw」。
- R 像素带来的过曝冗余：同帖 walker2k5：「S5 could be **overexposed for at least 1-2 stops without loss of highlights (due to R photosites)**」——向右曝光后由官方引擎把 R 像素数据展开为平滑高光，肤色因此有「密度」而不死白。
- **反方证据（照实记录）**：同帖另一意见认为「Hyper Utility only marginally better than latest ACR 5. In fact, **ACR 5 provides better recovery of highlights** than Fuji's own software」——即部分场景下 Adobe 的高光算法反而更强，「官方更好」并非全场景绝对。

### 4. 新一代（EX 3.0 / SILKYPIX 8-10 系）比 HS-V3 更好的实测机制

dpreview 2019 对比帖第 2 页（[已抓取]，https://www.dpreview.com/forums/threads/fuji-s5-pro-post-processing-comparisons-love-cameras-hate-computers-software.4369074/page-2 ），用户 fPrime 2021-08-30 试 EX 3.0 转同一 RAF：

> 「**Color me impressed. This has gotten the tones and contrast just about perfect.** I also believe that I was able to find an accurate WB more easily using the grey balance tool in Fuji RAW Converter than … Lightroom or Hyper Utility. … the fact that I could lower the contrast helped greatly in **reducing the skin tones from blocking up. Now the face looks much more natural.** I also seem to be able to **preserve more color detail and saturation** than I could with either Lightroom or HU.」

用户 rjcsmtx（#52）：「Free download from Fuji website - Fuji Raw Convertor is hands down the best option… **Color handling matches HU** and saves 12mp jpegs.」

机制归纳 [推断，基于以上证词]：新引擎颜色更好的原因 = ① SILKYPIX 8/10 系去马赛克与色彩矩阵算法比 2004–2006 年 HS-V3 自家引擎更新（SuperCCD 6MP 插值成 12MP 的算法有代差）；② 白平衡求解更准（灰点工具实测更易命中）；③ 色调曲线更柔性、对比可降、肤色不堵塞；④ 完整继承官方 3D-LUT/色彩科技（SILKYPIX Pro12 自称 Based on EX 3.0、可继承 EX 3.0 参数）；⑤ 同为「真双帧」处理，而 Adobe 只用 S 帧。

### 5. HS-V3 是否仍无可替代？

楼主 EE-TV（2019-02-22）[已抓取]：「I would say **Hyper Utility really does have 'that certain look' to it**」；第 2 页搜索摘要 [摘要] 有人称「For me personally no conversion gives me true Fuji S5 Pro colors other than Hyper Utility and the camera's SOOC JPEG」。但 fPrime 的 EX 3.0 实测（上文）显示新一代已能追平甚至超越 HU 的肤色渲染，且另一个用户指出「The image quality from this converter beats LR but **the rendering engine is so painfully SLOW**」（2023 帖，SILKYPIX 的代价是速度）。

---

## 四、社区复刻「富士官方转换器肤色」配方

### 路线 A：Lightroom 近似（写实派）

1. **配置文件回退法**（dpreview fPrime，2021-08-30 [已抓取]）：把 LR 默认的「Adobe Color」配置文件换成旧的 **「Adobe Standard」**——「got the closest match to my HU VS-3 results by reverting Lightroom 'Adobe Color' to the old 'Adobe Standard' profile which **lowered the contrast curve enough**」。要点：S5 Pro 的 Adobe Standard 曲线更平，肤色不易堵塞。
2. **Defringe 清零教训**（同帖 #51 [已抓取]）：fPrime 曾把外场调的 Defringe（去边）设置存成 S5 Pro 导入默认值，导致室内照色彩错乱被误判为「LR 颜色崩坏」。复刻时**不要带 Defringe 默认值**。
3. **Camera Calibration 手调法**（stevehuffphoto S5 Pro 回顾评论区，RichieRich [已抓取，https://www.stevehuffphoto.com/2014/08/26/flashback-the-fuji-s5-pro-gorgeous-color-reproduction ]）：「**custom camera calibration** just so that renderings look pretty close…」——用 LR「校准」面板（原色蓝/绿/红主色饱和度与色相）逐项调到接近官方 JPEG。
4. **机身基准设定法**（dpreview 2009「1st impressions of the S5 Pro」[摘要]）：「Set **DR to 400 and color to High**. Play with the tone…」——先拍一张 DR400+色彩浓（Color High）的 SOOC JPEG 作为色彩目标，再在 LR 里对 RAF 逼近。这是复刻的「靶子」。
5. **方法论参考**（非 SuperCCD 专用）：photographylife《How to Get Accurate Fuji Colors in Lightroom》（2018-02-11）[摘要]：https://photographylife.com/how-to-get-accurate-fuji-colors-in-lightroom
6. **现成 profile 下载现状**：GitHub abpy/FujifilmCameraProfiles [摘要]（https://github.com/abpy/FujifilmCameraProfiles ）提供基于 Adobe 相机匹配 profile 制作的 dng/dcp——**但面向 X 系相机**；S5 Pro 没有现成的官方 DCP（Adobe 仅随附 Adobe Standard），双帧结构也使 X 系 profile 无法直接套用。[推断] 想要 S5 Pro 专属 DCP 只能自拍 ColorChecker + DNG Profile Editor 自制。

### 路线 B：免费正解——EX 3.0（社区共识）

同一对比帖中，多数人最终选择「别复刻了，直接用官方的」：EX 3.0 免费下载、兼容 S5 Pro、社区实测「Color handling matches HU」且「tones and contrast just about perfect」。**这是成本最低、还原度最高的方案。**

### 路线 C：付费正统——SILKYPIX Developer Studio Pro12 for FUJIFILM

¥6,930（Windows 专用），官方自称 Based on EX 3.0、可继承 EX 3.0 参数、支持 S2/S3/S5 Pro（https://www.silkypix.com/en/ext/special/dsp12fuji [已抓取]）。适合想要 EX 3.0 色彩 + 更强编辑功能的用户。

### 路线 D：情怀完全体——HS-V3

找随机/零售 CD 安装（官网无下载），现代 Windows 需 XP/Vista 兼容模式；少数派认为只有它和 SOOC JPEG 才是「真 S5 色」（EE-TV、第 2 页摘要引文），但速度慢、系统兼容是硬伤。

### 路线 E：免费替代引擎

LightZone（免费、能读富士 RAF，stevehuffphoto 评论文提及 [已抓取]）；RawTherapee/darktable 只处理 S 帧第一帧（pixls 帖 [已抓取]），高光扩展无从谈起，颜色上限受限。

### YouTube 教程与 flickr 群组：未找到

- YouTube：两轮检索（含 site:youtube.com）**未找到**针对 S5 Pro 的肤色复刻教程视频；仅有 X 系通用方法文（Fstoppers《Get Fuji Color on Any Camera With Lightroom Profiles》[摘要]，https://fstoppers.com/gear/how-get-fuji-color-any-camera-lightroom-profiles-716979 ）。空结果，如实记录。
- flickr S5 Pro 群组：两轮不同关键词搜索均**零命中**。空结果。

---

## 五、检索建议（供后续自查）

- `site:fujifilm-x.com "RAW FILE CONVERTER EX" S5 Pro`（下载/兼容页）
- `"Hyper-Utility HS-V3" download OR price`（HS-V3 残留信息）
- `dpreview "S5 Pro" "Hyper Utility" OR "Raw Converter" comparison`（对比帖）
- `pixls "superccd" OR "S5 Pro" raw two images`（双帧技术讨论）
- `"S5 Pro" skin tone "Lightroom" preset OR profile OR calibration`（复刻配方）
- `"S5 Pro" RAF "R pixel" OR "S pixel" dcraw`（文件结构分析）

## 六、证据分级与未验证清单

**已验证（本 run 抓取全文）**：EX 3.0 下载页（Ver.8.1.20.0 / 2026-03-02 / EX 1.x-2.0 停支持声明）；EX 3.0 兼容表（S2/S3/S5 ✓）；X RAW Studio 兼容表（无 FinePix）；S5 Pro 官方固件页（HS-V3 相关日志）；SILKYPIX Pro12 for FUJIFILM 页（¥6,930 / S2-S5 支持 / 继承 EX 3.0 参数）；台湾总代理 HS-V3 商品页（套件含 RAW File Converter EX + Camera Shooting、3D-LUT、LE 对比、兼容矩阵、已停产）；troeszter.net Hyper-Utility 版本矩阵；pixls 双帧帖；dpreview 2019 对比帖第 1、2 页；dpreview 2023 EX 3.0 帖；photo.net DR 0–400% 帖；dpreview S5 Pro 评测（2007-07-05，SR II 结构/14-bit/DR 六档/optional software）；stevehuffphoto 回顾文。

**搜索摘要级（未整页抓取）**：EX 断代三帖（dpreview 2010-12、fujix-forum 2013-05、fujiaddict 2018-08/11）；X RAW Studio 发布（fujiaddict 2017-11、fujirumors 2018-02）；X RAW Studio 下载页；B&H HS-V3 零售页；fujifilm.it HS-V3 配件页；photographylife 方法文；Fstoppers profiles 文；GitHub abpy profiles。

**未验证/未知**：① LE 是否 SILKYPIX 内核（未证实，时间线反而矛盾，按「几乎不可能」处理）；② HS-V3 零售价与精确首发年（S3 Pro 2004 系推断）；③ X RAW Studio 下载页整页内容；④ robotplanet.dk 的 RAF 拆包分析（两次抓取失败）；⑤ flickr 群组与 YouTube 专项教程（搜索零命中）；⑥ dpreview 新帖 .4812597、B&H 商品页、camera-wiki.org（bot 验证/维护中，无法抓取）。
