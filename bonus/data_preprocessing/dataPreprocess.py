# Preprocess the output data for later visualization
from xlwt import *

output = Workbook()
sheet = output.add_sheet("sheet1")

x = [i for i in range(20, 105, 5)]
infile = open("./raw_data/cpu_output.txt", "r")  # can change to io_output.txt or mixed_output.txt
# infile = open("./raw_data/io_output.txt", "r")
# infile = open("./raw_data/mixed_output.txt", "r")

data = infile.readlines()

for i in range(len(x)):
    sheet.write(0, i, x[i])

row = 1
counter = 0
for item in data:
    if item[:4] == "real":
        value = eval(item[4:])
        if counter < len(x):
            sheet.write(row, counter, value)
            counter += 1
        else:
            row += 1
            counter = 0
            sheet.write(row, counter, value)
            counter += 1

output.save("./tidy_data/cpu_output.xls")  # can change to io_output.xls or mixed_output.xls
# output.save("./tidy_data/io_output.xls")
# output.save("./tidy_data/mixed_output.xls")