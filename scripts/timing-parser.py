#!/usr/bin/python3

import csv
import os

data = {}


for timing_file in os.listdir():
    if timing_file.endswith(".csv"):
        file_info = timing_file[:-4].split("-")[1:]
        world_rank = int(file_info[0])
        team_rank = int(file_info[1])
        rep_num = int(file_info[2])
        #rank - team_rank - R#
        if team_rank not in data:
            data[team_rank] = {}
        data[team_rank][rep_num] = {}
        with open(timing_file,newline='') as csvfile:
            file_reader = csv.reader(csvfile, delimiter=",")
            for row in file_reader:
                data[team_rank][rep_num][row[0]] = list(row[1:])
        
             
for rank in data.keys():
    for rep1 in data[rank]:
        for rep2 in data[rank]:
            if rep1 < rep2:
                for log in data[rank][rep1].keys():
                    if log != 'iSendEnd':
#                         data[rank][log+'Diff'] = 0.0
                        diff = 0.0
                        for i,val in enumerate(data[rank][rep1][log]):
#                             data[rank][log+'Diff'] += float(data[rank][rep1][log][i]) - float(data[rank][rep2][log][i])
                            diff += float(data[rank][rep1][log][i]) - float(data[rank][rep2][log][i])
                        print("Rank {} {} diff = {}".format(rank, log, diff / len(data[rank][rep1][log])))
#             if len(data[rank][rep1]['iSendStart']) !=  len(data[rank][rep2]['iSendStart']): print("iSendStart")
# #             if len(data[rank][rep1]['iSendEnd']) !=  len(data[rank][rep2]['iSendEnd']): print("iSendEnd")
#             if len(data[rank][rep1]['iRecvStart']) !=  len(data[rank][rep2]['iRecvStart']): print("iRecvStart")
#             if len(data[rank][rep1]['iRecvEnd']) !=  len(data[rank][rep2]['iRecvEnd']): print("iRecvEnd")

# for rank in data.keys():
#     print(data[rank])