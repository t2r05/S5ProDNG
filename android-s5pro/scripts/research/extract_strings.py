"""Extract strings (ASCII + UTF-16LE) from a PE and filter for interesting bits."""
import re, sys, collections

path = sys.argv[1] if len(sys.argv) > 1 else 'ContentaConverterPremium2026-WebSetup.exe'
data = open(path, 'rb').read()

ascii_runs = re.findall(rb'[\x20-\x7e]{6,}', data)
utf16_runs = re.findall(rb'(?:[\x20-\x7e]\x00){6,}', data)
texts = [s.decode('latin-1') for s in ascii_runs]
texts += [s.decode('utf-16-le') for s in utf16_runs]

urls = sorted(set(re.findall(r'https?://[^\s"\'<>]{5,}', ' '.join(texts))))
print('=== URLS ===')
for u in urls[:40]:
    print(' ', u[:130])

keys = ['raf', 'cr2', 'cr3', 'nef', 'arw', 'dng', 'orf', 'rw2', 'pef', 'srw', 'x3f',
        'convert', 'batch', 'resize', 'watermark', 'raw', 'thumbnail', 'exif', 'preset',
        'heic', 'tiff', 'jpeg', 'png', 'flip', 'rotate', 'rename', 'timestamp', 'premium']
hits = collections.Counter()
for t in texts:
    tl = t.lower()
    for k in keys:
        if k in tl and len(t) < 200:
            hits[t.strip()] += 1

print('=== FEATURE-ISH STRINGS (deduped sample) ===')
seen = set()
n = 0
for t, c in hits.most_common(400):
    if t in seen:
        continue
    seen.add(t)
    print(' ', t[:150])
    n += 1
    if n >= 80:
        break
