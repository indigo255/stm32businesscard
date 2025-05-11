#!/usr/bin/env python3
from matplotlib import pyplot as plt
import numpy as np

img = np.genfromtxt('out.csv', delimiter=',')
plt.imshow(img)
plt.show()
