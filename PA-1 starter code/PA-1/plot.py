import matplotlib.pyplot as plt

sizes = [100, 1024, 5120]  # KB
times = [0.172, 0.174, 0.176] # seconds

plt.plot(sizes, times, marker='o')
plt.xlabel("File Size (KB)")
plt.ylabel("Execution Time (s)")
plt.title("Execution Time vs File Size")
plt.grid(True)
plt.savefig("plot.png")
plt.show()
