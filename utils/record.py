import sys
import csv
import os
import re

case_name = sys.argv[1]
global_log_path = sys.argv[2]
legal_log_path = sys.argv[3]
csv_path = sys.argv[4]

if not os.path.exists(csv_path):
    with open(csv_path, 'w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(["punishment", "overlap ratio", "HPWL (Global)", 
                        "Legal mode", "Legal strategy", "HPWL (legal)", "Vio #"])

# extract punishment, overlap ratio, global hpwl
with open(global_log_path, 'r') as global_log:
    f = global_log.read().split("\n")

    punishment_line = f[2]
    punishment_list = punishment_line.split(" ")
    try:
        punishment = float(punishment_list[-1])
    except ValueError:
        punishment = 0.05

    for line in f:
        if "Overlap Ratio" in line:
            overlap_ratio = line.split()[-2]
        if "Estimated HPWL" in line:
            global_hpwl = float(line.split()[-1])

# extract digits from log file
s_list = re.findall(r"s\d+", legal_log_path)
strategy = s_list[-1][1:]
m_list = re.findall(r"m\d+", legal_log_path)
mode = m_list[-1][1:]

# extract hpwl and violation number
with open(legal_log_path, 'r') as legal_log:
    line = legal_log.readline()
    while line:
        if "TOOMUCHOVERLAP" in line or "Impossible to solve" in line:
            legal_hpwl = -1
            violations = -1
            break
        elif "Total Violations:" in line:
            violations = int(line.split()[-1])
        elif "Best hpwl =" in line:
            legal_hpwl = float(line.split()[-1])
        line = legal_log.readline()

with open(csv_path, 'a', newline='') as file:
    writer = csv.writer(file)
    writer.writerow([punishment, overlap_ratio, global_hpwl, mode, strategy, legal_hpwl, violations])