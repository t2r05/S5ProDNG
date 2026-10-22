# 下载引擎配置（Motrix aria2c）

## 引擎路径
- aria2c: `C:\Users\nanxing\AppData\Local\Programs\Motrix\resources\engine\aria2c.exe`
- CA bundle: `<工作区>\tools\cacert.pem`（Mozilla bundle，aria2 引擎无内置证书，必须指定）

## 标准命令模板
```powershell
& 'C:\Users\nanxing\AppData\Local\Programs\Motrix\resources\engine\aria2c.exe' `
  -x16 -s16 -k1M --file-allocation=none `
  --ca-certificate="$PWD\tools\cacert.pem" `
  -d <输出目录> -o <文件名> <URL>
```

## 实测（2026-10-22）
| 站点 | 单线程 curl | aria2 16 线程 |
|---|---|---|
| contenta-software.com（224MB） | ~1MB/min（约 3.5 小时） | **20 秒完成** |

## 适用判断
- ✅ 慢速/限速站点的大文件（每连接限速类）
- ❌ dl.google.com（aria2 握手失败；curl/sdkmanager 本来就快，继续用原通道）
- 注意：GitHub 的 git push 不适用（那是 git 协议不是 HTTP 下载）
