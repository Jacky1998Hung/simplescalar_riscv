import sys
file_ = sys.argv[1]

f = open(file_)
lines = f.readlines()
#  x = [line for line in lines if "debug" in line]

#  y = [line for line in x if "instruction" not in line]
#  y = [line for line in y if "addr" not in line]
n = [line for line in lines if "sim_main:machine.def" in line]
s = list(set(n))
all_ = []
for line in s:
    start = line.find(":")
    end = line.find("[")
    s_ = line[start+1:end].strip()
    if "!" not in s_:
        all_.append(s_)
for i in all_:
    print(i)
