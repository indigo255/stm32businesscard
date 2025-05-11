#!/usr/bin/env python3
from matplotlib import pyplot as plt
import numpy as np

res_x = 80
res_y = 160
img = np.zeros((res_x, res_y))
iters = 200
cutoff  = 4
range_real = np.linspace(-0.7604598999023438, -0.7678184509277344, res_x)
range_imag = np.linspace(-0.09853172302246094, -0.09117317199707031, res_y)
#range_real = np.linspace(-1.4805909395217896, -1.4803047180175781, res_x)
#range_imag = np.linspace(-0.0011744499206542969 - 0.0001431107521057129, -0.0011744499206542969 + 0.0001431107521057129, res_y)

print("starting")

for py, y in enumerate(range_imag):
  for px, x in enumerate(range_real):
    c = complex(x, y)
    z = 0
    n = 0
    while abs(z) < cutoff and n < iters:
      z = z**2 + c
      n += 1
    img[px][py] = n

plt.imshow(img)
plt.show()
