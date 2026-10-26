"""Tileable streak noise for blowing snow ("snow snakes"): long along X (the wind), thin across Y.
R = long sparse streaks, G = shorter, softer ones. Seamless by construction (filtered white noise in Fourier space)."""
import sys
import numpy as np
from PIL import Image

N = 512
rng = np.random.default_rng(11)
fx = np.fft.fftfreq(N)[None, :]
fy = np.fft.fftfreq(N)[:, None]


def streaks(stretch, peak, width):
    # band-pass noise whose features are `stretch` times longer along X than along Y
    k = np.sqrt((fx * stretch) ** 2 + fy ** 2) * N
    spec = np.exp(-((k - peak) / width) ** 2)
    spec[0, 0] = 0
    noise = np.fft.ifft2(np.fft.fft2(rng.standard_normal((N, N))) * spec).real
    noise = (noise - noise.mean()) / noise.std()
    return noise


a = streaks(8.0, 4.0, 3.0) * 0.8 + streaks(8.0, 10.0, 5.0) * 0.35
b = streaks(5.0, 7.0, 4.0) * 0.7 + streaks(4.0, 16.0, 8.0) * 0.3
to8 = lambda x: (np.clip(0.5 + 0.18 * x, 0, 1) * 255).astype(np.uint8)
img = np.stack([to8(a), to8(b), np.zeros((N, N), np.uint8)], -1)
Image.fromarray(img, "RGB").save(sys.argv[1])
print("ok", a.std(), b.std())
