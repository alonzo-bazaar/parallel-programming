import matplotlib as mpl
import matplotlib.pyplot as plt

fig, (ax1, ax2) = plt.subplots(2, 1)
ax1.plot([1, 2, 3, 4], [1, 4, 2, 3])
ax2.plot([0, 0, 4, 4], [1, 4, 2, 3])
plt.show()
