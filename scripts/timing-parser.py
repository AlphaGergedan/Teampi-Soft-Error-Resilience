#!/usr/bin/python3

import csv
import os
import matplotlib.pyplot as plt

data = {}
maxTime = 0.0

for timing_file in os.listdir("."):
    if timing_file.endswith(".csv"):
        file_info = timing_file[:-4].split("-")[1:]
        world_rank = int(file_info[0])
        team_rank = int(file_info[1])
        rep_num = int(file_info[2])
        #rank - team_rank - R#

        with open(timing_file,newline='') as csvfile:
            file_reader = csv.reader(csvfile, delimiter=",")
            for row in file_reader:
                if row[0] == "syncPoints":
                    data[world_rank] = [float(i) for i in row[1:]]
                    thisMaxTime = max(data[world_rank])
                    if thisMaxTime > maxTime:
                        maxTime = thisMaxTime


for rank in data:
    if rank < 4:
        plt.plot(data[rank], [1]*len(data[rank]))
    else:
        print("Rank", rank-4, ":", len(data[rank-4]))
        print("Rank", rank, ":", len(data[rank]))
        for i in range(len(data[rank])):

            plt.plot(data[rank-4][i], data[rank][i])
    
plt.xlabel('times')
plt.ylabel('rank')

plt.show()