import sys
from PIL import Image


def compare(path_a: str, path_b: str) -> int:
    img_a = Image.open(path_a)
    img_b = Image.open(path_b)

    if img_a.size != img_b.size:
        print(f"size mismatch: {path_a}={img_a.size} vs {path_b}={img_b.size}")
        return 2

    if img_a.mode != img_b.mode:
        print(f"mode differs: {img_a.mode} vs {img_b.mode}")

    if img_a.mode != "RGBA":
        img_a = img_a.convert("RGBA")
    if img_b.mode != "RGBA":
        img_b = img_b.convert("RGBA")

    pixels_a = img_a.load()
    pixels_b = img_b.load()
    width, height = img_a.size

    diffs = 0
    for y in range(height):
        for x in range(width):
            pa = pixels_a[x, y]
            pb = pixels_b[x, y]
            if pa != pb:
                diffs += 1
                a_bin = " ".join(f"{c:08b}" for c in pa[:3])
                b_bin = " ".join(f"{c:08b}" for c in pb[:3])
                print(f"({x},{y}): {pa} -> {pb}")
                print(f"         bin: {a_bin} -> {b_bin}")

    total = width * height
    print(f"\n{diffs}/{total} pixels differ ({100 * diffs / total:.4f}%)")
    return 1 if diffs else 0


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} <a.png> <b.png>", file=sys.stderr)
        sys.exit(64)
    sys.exit(compare(sys.argv[1], sys.argv[2]))
