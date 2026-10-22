"""Probe mark3: verify the synthesized CFA sub-TIFF parses via rawpy sizes.
Success = raw dims become 8704 x 1444 and no IO error at unpack.
"""
import rawpy, struct, sys

f = 'sample-full.RAF'
try:
    raw = rawpy.imread(f)
    s = raw.sizes
    print('raw dims:', s.raw_width, 'x', s.raw_height)
    print('visible:', s.width, 'x', s.height, 'margins', s.left_margin, s.top_margin)
    img = raw.raw_image
    print('raw_image shape:', img.shape, img.dtype)
    print('corner values R0: ', img[0, :6])
except Exception as e:
    print('ERR:', repr(e)[:250])
