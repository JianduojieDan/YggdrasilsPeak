# One presentation image per HUD v5 direction: the whole frame (day) on top, then close-ups of the HUD on the day and
# the storm frame side by side, with a title. Saved/Review/hud_v5_present_<A|B|C>.png
from PIL import Image, ImageDraw, ImageFont
P = 'F:/UE_project/YggdrasilsPeak/Saved/Review/'
T = ImageFont.truetype('E:/UE5.8/UE_5.8/Engine/Content/Slate/Fonts/Roboto-Light.ttf', 34)
S = ImageFont.truetype('E:/UE5.8/UE_5.8/Engine/Content/Slate/Fonts/Roboto-Regular.ttf', 18)
info = {
    'A': ('A  \u00b7  Altimeter', 'Bottom left: big light numerals, a route line with every camp and a marker for you, five precise readouts (SpO2, core temperature, body, food, water).'),
    'B': ('B  \u00b7  Horizon', 'Bottom centre, like a helmet display: altitude above a fading hairline, tick gauges along it, breath as the bright middle of the line.'),
    'C': ('C  \u00b7  Column', 'Bottom left: five slim vertical gauges with their values, the altitude beside them, the route line under it.'),
}
for k, (title, sub) in info.items():
    day = Image.open(P + f'hud_v5_{k}_day.png').convert('RGB'); storm = Image.open(P + f'hud_v5_{k}_storm.png').convert('RGB')
    box = (0, 690, 960, 1080) if k != 'B' else (480, 690, 1440, 1080)
    W = 1920
    out = Image.new('RGB', (W, 110 + 1080 + 390), (10, 14, 22))
    d = ImageDraw.Draw(out)
    d.text((40, 22), title, font=T, fill=(238, 243, 248)); d.text((40, 68), sub, font=S, fill=(176, 188, 202))
    out.paste(day, (0, 110))
    out.paste(day.crop(box), (0, 1190)); out.paste(storm.crop(box), (960, 1190))
    d.text((20, 1200), 'bright snow', font=S, fill=(10, 14, 22)); d.text((980, 1200), 'storm, bright sky', font=S, fill=(10, 14, 22))
    out.save(P + f'hud_v5_present_{k}.png')
    print('saved', k)
