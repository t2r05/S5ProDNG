"""Probe mark4: dump what the open sequence computes. Use LibRaw C API via
ctypes against our own static lib? Too heavy. Instead: binary search the
'data corrupted' — 12606982 ≈ payload offset. Compute: payload starts at
cfa0 + tiff_len2 = 38452+80=38532; 12606982-38532 = 12568450 ≈ row 1444*2 bytes
per row * ... = raw_width*raw_height*2 = 4352*1444*2 = 12568576!
=> LibRaw still uses 4352x1444 (the value from parse_fuji 0x0100 RawImageFullSize),
NOT our CFA TIFF dims. So the CFA sub-TIFF tags (0xf001/0xf002) set
tiff_ifd dims but apply_tiff chose an IFD with those dims... yet raw_width
stays 4352? Actually raw dims printed = 4352x1444 — matches fuji tags 0x0100.
=> our sub-TIFF was NOT accepted (maybe II/hdr parse failed) OR overridden.

Also byte order: read_shorts reads little-endian words on x86; my payload is
big-endian packed. Fix payload to '<HH'.
And derror fires when (RAW >>= load_flags) >> bits non-zero with maximum=0x3e00
=> bits=16, any value >0x3e00... my R values up to 36000 (0x8CA0) trigger!
With is_raw==2, shot_select=0 -> max 0x3e00=15872; S values ≤3400 OK; but
unpacked_load_raw checks FULL raw_image incl. R pixels >15872 -> derror!
So: R values must be ≤ 0x3e00 too... that kills 'R = highlights' concept at
the LibRaw level, but S5 real files DO have R up to 16383 (14-bit). maximum
0x3e00 = 16064? Hmm 0x3e00=15872. Real S5 R pixels go to 16383 — above that?
they'd derror spuriously? No: real files have is_raw==2 and the R image is
the SECOND shot (shot_select=1, max 0x2f00=12032). shot0 data = S pixels only
(6MP, max 15872). shot1 data = R pixels (6MP, max 12032)!!
"""
import struct

SRC = 'FujiFilm-test.raf'
OUT = 'sample-full.RAF'

data = bytearray(open(SRC, 'rb').read())

def u32(off):
    return struct.unpack_from('>I', data, off)[0]

def w32(off, val):
    struct.pack_into('>I', data, off, val)

fuji_off = u32(92)
entries = struct.unpack_from('>I', data, fuji_off)[0]
p = fuji_off + 4
tags = {}
for _ in range(entries):
    tag, ln = struct.unpack_from('>HH', data, p)
    tags[tag] = (p + 4, ln)
    p += 4 + ln

o, _ = tags[0x0100]
raw_h, raw_w = struct.unpack_from('>HH', data, o)   # 1444 x 4352 = 6.28MP = ONE plane
PLANE_W = raw_w          # 4352
PLANE_H = raw_h          # 1444
plane_bytes = PLANE_W * PLANE_H * 2
print(f'one plane: {PLANE_W} x {PLANE_H}, {plane_bytes} bytes')

def build_cfa_tiff(data_off_abs, w, h):
    e = bytearray()
    e += b'II\x2a\x00' + struct.pack('<I', 8)
    tags_t = [
        (0xf001, 3, 1, w),
        (0xf002, 3, 1, h),
        (0xf003, 3, 1, 16),
        (0x0103, 3, 1, 1),
        (0x0111, 4, 1, data_off_abs),
        (0x0117, 4, 1, w * h * 2),
    ]
    e += struct.pack('<H', len(tags_t))
    for tag, typ, cnt, val in tags_t:
        e += struct.pack('<HHI', tag, typ, cnt)
        if typ == 3:
            e += struct.pack('<HH', val & 0xFFFF, 0)
        else:
            e += struct.pack('<I', val)
    e += struct.pack('<I', 0)
    return bytes(e)

tiff_len = 8 + 2 + 12 * 6 + 4

def make_plane(seed):
    rows = bytearray()
    for y in range(PLANE_H):
        for x in range(PLANE_W):
            v = (x * 7 + y * 13 + seed) % 12000 + 200   # keep <= 12032
            rows += struct.pack('<H', v)                 # little-endian
    return bytes(rows)

new = bytearray(data)
while len(new) % 4:
    new += b'\x00'
cfa0 = len(new)
hdr0 = build_cfa_tiff(cfa0 + tiff_len, PLANE_W, PLANE_H)
new += hdr0
new += make_plane(0)
cfa1 = len(new)
hdr1 = build_cfa_tiff(cfa1 + tiff_len, PLANE_W, PLANE_H)
new += hdr1
new += make_plane(6000)

w32(100, cfa0); w32(104, len(hdr0) + plane_bytes)
w32(128, cfa1); w32(132, len(hdr1) + plane_bytes)
w32(120, 25362)

open(OUT, 'wb').write(new)
print('wrote', OUT, len(new), 'cfa0@', cfa0, 'cfa1@', cfa1)
