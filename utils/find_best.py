import sys
import csv

case_name = sys.argv[1]
csv_path = "../records/csv/" + case_name + ".csv"

lowest_hpwl = 100000000000000.0
bestDict = {}
with open(csv_path, 'r') as file:
    reader = csv.DictReader(file)
    # reader = csv.reader(file)
    for row in reader:
        overlap_ratio = float(row["overlap ratio"][0:-1])
        final_hpwl = float(row["HPWL (legal)"])
        violations = int(row["Vio #"])
        if final_hpwl == -1 or violations == -1:
            continue
        elif final_hpwl < lowest_hpwl and violations == 0:
            bestDict = row
            lowest_hpwl = final_hpwl

print("Lowest hpwl: {}".format(lowest_hpwl))
print("Best Punishment: {}".format(bestDict["punishment"]))
print("Legal Mode: {}".format(bestDict["Legal mode"]))
print("Config: {}".format(bestDict["Config"]))
print(bestDict)
