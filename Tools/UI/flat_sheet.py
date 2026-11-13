# Contact sheet of the flat-lay kit renders (Saved/ItemIcons/render_flat) for a quick look.
import glob
from PIL import Image
d = 'F:/UE_project/YggdrasilsPeak/Saved/ItemIcons/render_flat/'
fs = sorted(glob.glob(d + '*.png'))
ims = [Image.open(f).convert('RGBA').resize((256, 256)) for f in fs]
sheet = Image.new('RGBA', (256 * 5, 256 * ((len(ims) + 4) // 5)), (40, 38, 34, 255))
for i, im in enumerate(ims):
    sheet.alpha_composite(im, ((i % 5) * 256, (i // 5) * 256))
sheet.convert('RGB').save('F:/UE_project/YggdrasilsPeak/Saved/ItemIcons/flat_sheet.png')
print([f.split('/')[-1].split('\\')[-1] for f in fs])
