import numpy as np
import matplotlib.pyplot as plt

with open("bin/timestamps.bin", "rb") as f:
    v = np.fromfile(f, np.float64)

errors = np.diff(v)

plt.plot(abs(errors - 1))
plt.xlabel("calls of BTnearMe")
plt.ylabel("errors in seconds")
plt.show()

print(f"Mean Error: {np.mean(errors-1)}")
