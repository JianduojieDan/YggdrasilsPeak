# Chalk outlines of the flat-lay kit renders (pack v4, 2026-11-13): when something is used up, its outline stays on the
# tarp so you see what is missing. Saved/ItemIcons/render_flat/<name>.png -> render_flat_line/<name>.png
# (white line ~3 px at 768, slightly broken like chalk on canvas, transparent elsewhere).
import glob, os, random
from PIL import Image, ImageFilter, ImageChops
SRC = 'F:/UE_project/YggdrasilsPeak/Saved/ItemIcons/render_flat'
OUT = 'F:/UE_project/YggdrasilsPeak/Saved/ItemIcons/render_flat_line'
os.makedirs(OUT, exist_ok=True)
rng = random.Random(3)
for f in sorted(glob.glob(SRC + '/*.png')):
    a = Image.open(f).convert('RGBA').split()[3]
    a = a.point(lambda v: 255 if v > 40 else 0).filter(ImageFilter.MaxFilter(5))     # silhouette, small holes closed
    line = ImageChops.subtract(a.filter(ImageFilter.MaxFilter(5)), a.filter(ImageFilter.MinFilter(3)))
    # chalk: break the line here and there and soften it
    noise = Image.effect_noise(a.size, 90).filter(ImageFilter.GaussianBlur(3)).point(lambda v: 255 if v > 110 else 70)
    line = ImageChops.multiply(line, noise).filter(ImageFilter.GaussianBlur(0.8))
    out = Image.new('RGBA', a.size, (236, 230, 216, 0))
    out.putalpha(line.point(lambda v: int(v * 0.8)))
    out.save(os.path.join(OUT, os.path.basename(f)))
    print('outline', os.path.basename(f))
