"""Scan PE for zlib streams, decompress, and grep for payload URLs / setup info."""
import zlib, re, sys

path = sys.argv[1] if len(sys.argv) > 1 else 'ContentaConverterPremium2026-WebSetup.exe'
data = open(path, 'rb').read()
print('size:', len(data))

found = 0
texts = []
for i in range(len(data) - 2):
    if data[i] == 0x78 and data[i+1] in (0x01, 0x9C, 0xDA, 0x5E):
        try:
            out = zlib.decompressobj().decompress(data[i:i+3_000_000])
        except Exception:
            continue
        if len(out) > 200:
            found += 1
            # collect printable text
            t = ' '.join(x.decode('latin-1') for x in re.findall(rb'[\x20-\x7e]{6,}', out))
            texts.append((i, len(out), t))

print('zlib streams:', found)
urls = set()
for _, _, t in texts:
    for u in re.findall(r'https?://[^\s"\'<>\\]{5,}', t):
        urls.add(u)
print('=== decompressed URLs ===')
for u in sorted(urls)[:50]:
    print(' ', u[:140])

# interesting lines
print('=== setup/exe/feature lines ===')
n = 0
seen = set()
for _, _, t in texts:
    for line in re.split(r'\s{2,}', t):
        ll = line.lower()
        if any(k in ll for k in ('contenta', '.exe', '.dll', 'raw', 'cr2', 'nef', 'raf', 'convert', 'setup.tmp', 'download')):
            if line not in seen and len(line) < 160:
                seen.add(line)
                print(' ', line[:150])
                n += 1
                if n > 60:
                    sys.exit(0)
