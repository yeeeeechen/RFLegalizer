import sys
import csv
import os

case_name = sys.argv[1]
global_log_path = sys.argv[2]
legal_log_path = sys.argv[3]
csv_path = sys.argv[4]

if not os.path.exists(csv_path):
    with open(csv_path, 'w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(["punishment", "overlap ratio", "HPWL (Global)", 
                        "Legal mode", "Config", "HPWL (legal)", "Vio #"])

# extract punishment, overlap ratio, global hpwl
with open(global_log_path, 'r') as global_log:
    f = global_log.read().split("\n")

    for line in f:
        if "Punishment is set to" in line:
            punishment = float(line.split()[-1])
        if "Overlap Ratio" in line:
            overlap_ratio = line.split()[-2]
        if "Estimated HPWL" in line:
            global_hpwl = float(line.split()[-1])

# extract hpwl and violation number, legalization mode, config
with open(legal_log_path, 'r') as legal_log:
    line = legal_log.readline()
    mode = -1
    configFile = ""
    while line:
        if "TOOMUCHOVERLAP" in line or "Impossible to solve" in line:
            legal_hpwl = -1
            violations = -1
            break
        elif "Legalization mode:" in line:
            mode = int(line.split()[-1])
        elif "Reading Configs from:" in line:
            configFile = line.split()[-1]
        elif "Total Violations:" in line:
            violations = int(line.split()[-1])
        elif "Best hpwl =" in line:
            legal_hpwl = float(line.split()[-1])
        line = legal_log.readline()

with open(csv_path, 'a', newline='') as file:
    writer = csv.writer(file)
    writer.writerow([punishment, overlap_ratio, global_hpwl, mode, configFile, legal_hpwl, violations])