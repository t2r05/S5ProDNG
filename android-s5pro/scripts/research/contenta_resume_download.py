# -*- coding: utf-8 -*-
"""Contenta Converter PREMIUM 2026 完整安装器断点续传下载（纯标准库，curl 优先 + urllib 兜底）。

用法（PowerShell，项目根目录）:
    python contenta_resume_download.py

行为:
  1. 先取 Content-Length（HEAD，失败时用 Range: bytes=0-0 的 Content-Range 兜底）
  2. 循环: 每轮 curl.exe --max-time 500 断点续传（-C -），轮间隔 30 秒
     curl 不存在时自动改用 urllib Range 续传（同样 500 秒/轮）
  3. 终止条件: 本地大小 == Content-Length（成功）；
     或连续 3 轮大小零增长且仍未达总长（报告失败，保留半成品，下次再跑即可续传）
  4. 只下载，绝不运行安装器；无任何第三方依赖。
"""
import os
import subprocess
import sys
import time
import urllib.error
import urllib.request

URL = 'https://www.contenta-software.com/ContentaConverterPremium2026-Setup.exe'
OUT = 'ContentaConverterPremium2026-Setup.exe'
MAXTIME = 500          # 每轮传输上限（秒）
POLL = 30              # 轮间隔（秒）
STABLE_LIMIT = 3       # 连续零增长轮数上限


def size_of(path):
    return os.path.getsize(path) if os.path.exists(path) else 0


def get_total():
    """HEAD 拿总长；405/失败时改用 Range: bytes=0-0 读 Content-Range。"""
    req = urllib.request.Request(URL, method='HEAD',
                                 headers={'User-Agent': 'Mozilla/5.0 cc-resume/1.0'})
    try:
        with urllib.request.urlopen(req, timeout=60) as r:
            cl = r.headers.get('Content-Length')
            if cl:
                return int(cl), 'HEAD'
    except Exception as e:
        print('HEAD failed: %r' % e, flush=True)
    req = urllib.request.Request(URL, headers={'Range': 'bytes=0-0',
                                               'User-Agent': 'Mozilla/5.0 cc-resume/1.0'})
    with urllib.request.urlopen(req, timeout=60) as r:
        cr = r.headers.get('Content-Range') or ''
        # 形如 bytes 0-0/235401328
        if '/' in cr:
            return int(cr.rsplit('/', 1)[1]), 'RANGE'
    raise RuntimeError('无法取得 Content-Length（HEAD 与 Range 均失败）')


def curl_pass():
    """按任务规定参数跑一轮 curl 断点续传；返回 rc（curl 不存在返回 None）。"""
    cmd = ['curl.exe', '-sL', '--retry', '15', '--retry-delay', '3', '-C', '-',
           '--max-time', str(MAXTIME), '-o', OUT, URL]
    try:
        return subprocess.call(cmd)
    except FileNotFoundError:
        return None


def urllib_pass():
    """urllib Range 续传一轮（≤MAXTIME 秒）。服务器不给 206 时自动从零重下。"""
    have = size_of(OUT)
    req = urllib.request.Request(URL, headers={'Range': 'bytes=%d-' % have,
                                               'User-Agent': 'Mozilla/5.0 cc-resume/1.0'})
    mode = 'ab'
    try:
        r = urllib.request.urlopen(req, timeout=MAXTIME)
    except urllib.error.HTTPError as e:
        if e.code == 416:          # Range 超界 = 已下完
            return 0
        raise
    with r:
        if r.status != 206 and have > 0:
            print('服务器不支持 Range（status=%d），从零重下' % r.status, flush=True)
            mode = 'wb'
        got = 0
        deadline = time.time() + MAXTIME
        with open(OUT, mode) as f:
            while True:
                if time.time() > deadline:
                    break
                chunk = r.read(65536)
                if not chunk:
                    break
                f.write(chunk)
                got += len(chunk)
        return got


def main():
    total, how = get_total()
    print('总长(%s) = %d bytes (%.1f MB)' % (how, total, total / 1048576.0), flush=True)
    if size_of(OUT) == total:
        print('本地文件已与总长一致，无需下载。', flush=True)
        return 0
    stable = 0
    rnd = 0
    while True:
        rnd += 1
        before = size_of(OUT)
        rc = curl_pass()
        if rc is None:
            print('curl.exe 不可用，改用 urllib 续传', flush=True)
            urllib_pass()
        else:
            print('round %d: curl rc=%s  %d -> %d / %d' % (rnd, rc, before, size_of(OUT), total), flush=True)
        now = size_of(OUT)
        if total > 0 and now >= total:
            break
        stable = stable + 1 if now == before else 0
        if stable >= STABLE_LIMIT:
            print('连续 %d 轮零增长且未达总长，停止（保留半成品，可重跑续传）。' % stable, flush=True)
            return 1
        time.sleep(POLL)
    final = size_of(OUT)
    ok = (total > 0 and final == total)
    print('完成校验: 本地 %d bytes, 总长 %d bytes, %s' %
          (final, total, '一致 ✓' if ok else '不一致 ✗（重跑本脚本可继续）'), flush=True)
    if ok:
        print('下一步: python contenta_inno_scan.py', flush=True)
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
