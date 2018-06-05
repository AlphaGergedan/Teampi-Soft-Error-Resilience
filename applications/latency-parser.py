import os

for filename in os.listdir("."):
    if (".txt") in filename:
        f_parts = filename.split("-")
        r_factor = int(f_parts[1][1:])
        start = int(f_parts[2][1:])
        end = int(f_parts[3][1:])
        incr = int(f_parts[4][1:])
        rank = int(f_parts[5][:1])


            