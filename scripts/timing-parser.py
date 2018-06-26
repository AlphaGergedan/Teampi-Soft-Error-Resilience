#!/usr/bin/python3

import csv
import os
import sys
import matplotlib.pyplot as plt

data = {}
maxTime = 0.0

resDir = sys.argv[1]

for timing_file in os.listdir(resDir):
    if timing_file.endswith(".csv"):
        file_info = timing_file[:-4].split("-")[1:]
        world_rank = int(file_info[0])
        team_rank = int(file_info[1])
        rep_num = int(file_info[2])

        with open(resDir + "/" + timing_file,newline='') as csvfile:
            file_reader = csv.reader(csvfile, delimiter=",")
            for row in file_reader:
                if row[0] == "syncPoints":
                    data[world_rank] = [float(i) for i in row[1:]]
                    thisMaxTime = max(data[world_rank])
                    if thisMaxTime > maxTime:
                        maxTime = thisMaxTime


for rank in data:
        plt.scatter(data[rank], [rank]*len(data[rank]))


plt.grid(True)
plt.xlabel('time [t] = s')
plt.ylabel('world rank')

plt.show()

plt.clf()

for rank in data:
    if rank < 4:
        plt.scatter(data[rank], [rank]*len(data[rank]),c="r", alpha=0.75)
    else:
        plt.scatter(data[rank], [rank-4]*len(data[rank]),c="b", alpha=0.75)

plt.grid(True)
plt.xlabel('time [t] = s')
plt.ylabel('team rank')

plt.show()
