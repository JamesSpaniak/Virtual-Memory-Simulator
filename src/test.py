import sys
import os

sizes = [1,2,4,8,16,32]
algorithms = ["FIFO", "LRU", "Clock"]
pre = ["+", "-"]


for p in pre:
    for algo in algorithms:
        for size in sizes:
            command = f"./main plist ptrace {size} {algo} {p}"
            print(f"command: {command}")
            os.system(command)