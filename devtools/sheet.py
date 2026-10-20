"""sheet.py DIR OUT.bmp [SCALE] [COLS]: a contact sheet of DIR's 24-bit frame-NNNN.bmp pictures."""
import pathlib, struct, sys
src, out = pathlib.Path(sys.argv[1]), sys.argv[2]
scale = int(sys.argv[3]) if len(sys.argv) > 3 else 4
cols = int(sys.argv[4]) if len(sys.argv) > 4 else 8
frames = sorted(src.glob("frame-*.bmp"))
def load(p):
    b = p.read_bytes()
    off, = struct.unpack_from("<I", b, 10)
    w, h = struct.unpack_from("<ii", b, 18)
    pitch = (w * 3 + 3) & ~3
    rows = [b[off + pitch * r: off + pitch * r + w * 3] for r in range(abs(h))]
    if h > 0: rows.reverse()          # bottom-up file
    return w, abs(h), rows
w, h, _ = load(frames[0])
tw, th, gap = w // scale, h // scale, 4
rows_n = (len(frames) + cols - 1) // cols
W, H = cols * (tw + gap) + gap, rows_n * (th + gap) + gap
canvas = [bytearray(b"\x30\x30\x30" * W) for _ in range(H)]
for i, p in enumerate(frames):
    _, _, rows = load(p)
    x0, y0 = gap + (i % cols) * (tw + gap), gap + (i // cols) * (th + gap)
    for y in range(th):
        row = rows[y * scale]
        small = b"".join(row[x * scale * 3: x * scale * 3 + 3] for x in range(tw))
        canvas[y0 + y][x0 * 3: x0 * 3 + tw * 3] = small
pitch = (W * 3 + 3) & ~3
data = b"".join(bytes(r) + b"\0" * (pitch - W * 3) for r in reversed(canvas))
open(out, "wb").write(b"BM" + struct.pack("<IHHI", 54 + len(data), 0, 0, 54) + struct.pack("<IiiHHIIiiII", 40, W, H, 1, 24, 0, len(data), 2835, 2835, 0, 0) + data)
print(len(frames), "frames", W, H)
