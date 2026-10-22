# -*- coding: utf-8 -*-
"""Contenta Converter PREMIUM 2026 安装器 Inno 结构静态解析（纯 Python 标准库，只读）。

用法（PowerShell，项目根目录；下载完成后执行）:
    python contenta_inno_scan.py [ContentaConverterPremium2026-Setup.exe]

做什么（对应任务 3a-3d）:
  a. 定位 Inno 数据区: 全文件扫 b'zlb' 段标记 + 尾部 b'idir' 结构（读 header offset/size）
  b. 扫描压缩流: zlib 头 0x78 01/5E/9C/DA 与裸 LZMA alone 流（属性字节 5D 00 00 xx 00），
     lzma.FORMAT_ALONE 试解，全部解压留档 contenta-unpacked\\stream_*.bin（≥64KB 才落盘）
  c. 文件名表: 在每个解压块里找路径形字符串（ASCII + UTF-16LE；{app}\\、lib\\、Formats\\、
     Profiles\\、.icm、camera、.xml 等），去重全量 dump 到 contenta-filenames.txt
  d. PE carve: 解压块里 MZ..PE\\0\\0 的段 carve 成 contenta-payload\\payload_NNN.exe/.dll，
     逐个做字符串分析（格式扩展名、LibRaw/dcraw/DNG/watermark/Qt 等功能词），
     报告写 contenta-strings-report.txt，另生成 contenta-unpack-result.md 摘要

安全: 只读静态分析；不运行安装器、不运行 carve 出的任何 PE、不下载/安装任何解包工具。
"""
import hashlib
import lzma
import os
import re
import struct
import sys
import zlib

SETUP = sys.argv[1] if len(sys.argv) > 1 else 'ContentaConverterPremium2026-Setup.exe'
UNP_DIR = 'contenta-unpacked'
PAY_DIR = 'contenta-payload'

MAX_STREAM_OUT = 512 * 1024 * 1024   # 单流解压上限（防炸弹）
MAX_ATTEMPTS = 60000                 # 试探次数上限（控时长）
MIN_KEEP = 4096                      # 输出小于此的流丢弃
PROBE = 256 * 1024                   # 首次试探切片
BIGSLICE = 8 * 1024 * 1024           # 确认是真流后的大切片
MAX_PE = 500                         # carve PE 数上限
SAVE_STREAM_MIN = 64 * 1024          # 解压流落盘下限

RE_ZLIB = re.compile(rb'\x78[\x01\x5e\x9c\xda]')
RE_LZMA = re.compile(rb'\x5d\x00\x00[\x00\x08\x10\x80]\x00')
RE_ZLB = re.compile(rb'zlb')

ASCII_RUN = re.compile(rb'[\x20-\x7e]{4,240}')
UTF16_RUN = re.compile(rb'(?:[\x20-\x7e]\x00){4,240}')
RE_BRACE = re.compile(r'^\{[a-z][a-z0-9_]*\}', re.I)

KEY_EXTS = ['.raf', '.cr2', '.cr3', '.nef', '.arw', '.orf', '.rw2', '.pef', '.srw',
            '.srf', '.x3f', '.dng', '.erf', '.kdc', '.dcr', '.mos', '.mrw', '.3fr',
            '.iiq', '.rwl', '.fff', '.mdc', '.pxn', '.ptx', '.thm', '.heic', '.heif',
            '.avif', '.jxl', '.jp2', '.j2k', '.psd', '.psb', '.eps', '.sid', '.djvu',
            '.pcd', '.xcf', '.icm', '.xml', '.ini']
KEY_WORDS = ['libraw', 'dcraw', 'exiftool', 'imagemagick', 'watermark', 'iptc', 'xmp',
             'dropbox', 'wordpress', 'sftp', 'qt5', 'qt6', 'qtcore', 'qtgui', 'libqc',
             'borland', 'delphi', 'inno setup', 'contenta', 'jansoft', 'camera']
MACHINES = {0x14c: 'x86', 0x8664: 'x64', 0xAA64: 'arm64'}


def try_stream(data, off, kind, big=False):
    """在 off 处试解 zlib / lzma-alone 流。小切片确认后用大切片正式解。"""
    blob = data[off: off + (BIGSLICE if big else PROBE)]
    try:
        if kind == 'z':
            d = zlib.decompressobj()
            out = d.decompress(blob, MAX_STREAM_OUT if big else 256)
        else:
            d = lzma.LZMADecompressor(format=lzma.FORMAT_ALONE)
            out = d.decompress(blob, MAX_STREAM_OUT if big else 256)
    except Exception:
        return None
    if not out or len(out) < (MIN_KEEP if big else 64):
        return None
    if not big:
        return try_stream(data, off, kind, big=True)
    return out


def add_if_path(s, sink):
    s = s.strip()
    if not (6 <= len(s) <= 240):
        return
    if s.lower().startswith(('http://', 'https://')):
        return
    if RE_BRACE.match(s) or '\\' in s or '/' in s:
        sink.add(s)


def extract_names(blob, sink):
    for m in ASCII_RUN.finditer(blob):
        add_if_path(m.group().decode('latin-1'), sink)
    for m in UTF16_RUN.finditer(blob):
        add_if_path(m.group().decode('utf-16-le', 'ignore'), sink)


def carve_pes(blob, counter):
    """找 MZ..PE\\0\\0，carve 到 PAY_DIR。返回 [(name, size, machine, is_dll, bytes)]。"""
    out = []
    pos = 0
    while len(out) < MAX_PE:
        i = blob.find(b'MZ', pos)
        if i < 0:
            break
        pos = i + 2
        if i + 0x40 > len(blob):
            continue
        e = struct.unpack_from('<I', blob, i + 0x3c)[0]
        if not (0 < e <= 0x1000) or i + e + 24 > len(blob):
            continue
        if blob[i + e:i + e + 4] != b'PE\x00\x00':
            continue
        nxt = blob.find(b'MZ', i + 0x200)
        end = len(blob) if nxt < 0 else nxt
        pe = blob[i:end]
        if len(pe) < 0x400:
            continue
        machine = struct.unpack_from('<H', pe, e + 4)[0]
        chars = struct.unpack_from('<H', pe, e + 22)[0]
        is_dll = bool(chars & 0x2000)
        counter[0] += 1
        name = 'payload_%03d.%s' % (counter[0], 'dll' if is_dll else 'exe')
        path = os.path.join(PAY_DIR, name)
        with open(path, 'wb') as f:
            f.write(pe)
        out.append((name, len(pe), machine, is_dll, pe))
        pos = end
    return out


def analyze_pe(pe_bytes):
    text = b' '.join(ASCII_RUN.findall(pe_bytes))
    text += b' ' + b' '.join(UTF16_RUN.findall(pe_bytes))
    low = text.lower()
    kw = {}
    for k in KEY_WORDS:
        c = low.count(k.encode())
        if c:
            kw[k] = c
    exts = {}
    for e in KEY_EXTS:
        c = low.count(e.encode())
        if c:
            exts[e] = c
    tops = []
    seen = set()
    for s in ASCII_RUN.findall(pe_bytes):
        if len(s) >= 8 and s not in seen:
            seen.add(s)
            tops.append(s.decode('latin-1'))
        if len(tops) >= 120:
            break
    return kw, exts, tops


def main():
    if not os.path.exists(SETUP):
        print('找不到 %s（先跑 contenta_resume_download.py）' % SETUP)
        return 1
    data = open(SETUP, 'rb').read()
    print('file size: %d bytes (%.1f MB)' % (len(data), len(data) / 1048576.0), flush=True)
    os.makedirs(UNP_DIR, exist_ok=True)
    os.makedirs(PAY_DIR, exist_ok=True)

    # ---- a) 结构标记 ----
    zlb_offsets = [m.start() for m in RE_ZLB.finditer(data)]
    idir_info = []
    i = data.rfind(b'idir')
    if i >= 16:
        off, size = struct.unpack_from('<qq', data, i - 16)
        if 0 <= off < len(data) and 0 < size <= len(data) - off:
            idir_info.append((off, size))
    print('idir tail: %s ; zlb markers: %d' % (idir_info or '未找到', len(zlb_offsets)), flush=True)

    # ---- b) 候选流 + 试解 ----
    cands = [(m.start(), 'z') for m in RE_ZLIB.finditer(data)]
    cands += [(m.start(), 'l') for m in RE_LZMA.finditer(data)]
    for z in zlb_offsets:                       # zlb 后 4..13 字节处也可能起始
        for d in (4, 5, 8, 9, 12, 13):
            cands.append((z + d, 'z'))
            cands.append((z + d, 'l'))
    cands.sort()

    names = set()
    pe_counter = [0]
    pe_records = []          # (name, size, machine, is_dll, src_off, kw, exts, tops)
    streams = []             # (idx, off, kind, out_len, saved_name_or '-')
    sha_seen = set()
    attempts = 0
    idx = 0
    for off, kind in cands:
        if attempts >= MAX_ATTEMPTS:
            print('到达试探上限 %d，停止扫描（已足够覆盖）' % MAX_ATTEMPTS, flush=True)
            break
        attempts += 1
        out = try_stream(data, off, kind)
        if out is None:
            continue
        h = hashlib.sha1(out).hexdigest()
        if h in sha_seen:
            continue
        sha_seen.add(h)
        idx += 1
        saved = '-'
        if len(out) >= SAVE_STREAM_MIN:
            saved = 'stream_%03d_off_%08x_%s.bin' % (idx, off, kind)
            with open(os.path.join(UNP_DIR, saved), 'wb') as f:
                f.write(out)
        streams.append((idx, off, kind, len(out), saved))
        # ---- c) 文件名表 ----
        extract_names(out, names)
        # ---- d) PE carve + 字符串 ----
        for (name, psz, machine, is_dll, pe) in carve_pes(out, pe_counter):
            kw, exts, tops = analyze_pe(pe)
            pe_records.append((name, psz, machine, is_dll, off, kw, exts, tops))
        if idx % 10 == 0:
            print('  已解出 %d 条流 (attempts=%d)' % (idx, attempts), flush=True)

    print('解压流: %d 条；PE: %d 个；路径字符串: %d 条' % (idx, len(pe_records), len(names)), flush=True)

    # ---- 产出 1: 文件名表 ----
    brace = sorted(n for n in names if RE_BRACE.match(n))
    absish = sorted(n for n in names if not RE_BRACE.match(n) and (':\\' in n or n.startswith('\\\\')))
    relish = sorted(n for n in names if n not in set(brace) | set(absish))
    with open('contenta-filenames.txt', 'w', encoding='utf-8') as f:
        f.write('Contenta Converter PREMIUM 2026 安装器内路径字符串表（去重）\n')
        f.write('来源: %s (%d bytes) ; 解压流 %d 条 ; 共 %d 条路径\n\n' % (SETUP, len(data), idx, len(names)))
        f.write('== Inno 常量开头的安装路径 (%d) ==\n' % len(brace))
        f.write('\n'.join(brace) + '\n\n')
        f.write('== 盘符/UNC 绝对路径 (%d) ==\n' % len(absish))
        f.write('\n'.join(absish) + '\n\n')
        f.write('== 其他含分隔符路径 (%d) ==\n' % len(relish))
        f.write('\n'.join(relish) + '\n')

    # ---- 产出 2: 字符串报告 ----
    with open('contenta-strings-report.txt', 'w', encoding='utf-8') as f:
        f.write('carve PE 字符串分析报告\n%s\n' % ('=' * 60))
        for (name, psz, machine, is_dll, off, kw, exts, tops) in pe_records:
            f.write('\n### %s (%d bytes, %s, %s, 源偏移 0x%08x)\n' %
                    (name, psz, MACHINES.get(machine, hex(machine)),
                     'DLL' if is_dll else 'EXE', off))
            f.write('关键词: %s\n' % (kw or '（无命中）'))
            f.write('格式扩展名: %s\n' % (exts or '（无命中）'))
            f.write('字符串样本:\n')
            for t in tops[:60]:
                f.write('  %s\n' % t[:160])

    # ---- 产出 3: 结果摘要 md ----
    with open('contenta-unpack-result.md', 'w', encoding='utf-8') as f:
        f.write('# contenta_inno_scan 运行结果\n\n')
        f.write('- 输入: %s (%d bytes)\n' % (SETUP, len(data)))
        f.write('- idir: %s ; zlb 标记: %d\n' % (idir_info or '未找到', len(zlb_offsets)))
        f.write('- 解压流: %d 条（留档 %s/，≥64KB 落盘）\n' % (idx, UNP_DIR))
        f.write('- carve PE: %d 个 → %s/（详见 contenta-strings-report.txt）\n' % (len(pe_records), PAY_DIR))
        f.write('- 路径字符串: %d 条 → contenta-filenames.txt\n' % len(names))
        f.write('\n## 每个 PE 的关键词命中\n\n')
        for (name, psz, machine, is_dll, off, kw, exts, tops) in pe_records:
            f.write('- **%s** (%d B, %s): 关键词 %s ; 扩展名 %s\n' %
                    (name, psz, MACHINES.get(machine, hex(machine)),
                     kw or '无', sorted(exts.items(), key=lambda x: -x[1])[:12] or '无'))
        f.write('\n## 引擎判断线索（人工判读）\n\n')
        f.write('- 文件名表搜 libraw/dcraw/Formats/Profiles/.icm/camera 逐项核对\n')
        f.write('- 字符串报告搜 .raf .cr2 .nef 命中数（RAW 引擎广度）\n')

    print('产出: contenta-filenames.txt / contenta-strings-report.txt / contenta-unpack-result.md / %s/ / %s/'
          % (UNP_DIR, PAY_DIR), flush=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
