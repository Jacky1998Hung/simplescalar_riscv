import sys
filea = sys.argv[1]
fileb = sys.argv[2]
a = open(filea)
b = open(fileb)

a = set(a.readlines())
b = set(b.readlines())
diff = list(a - b)
diff2 = list(b - a)

print(f"in {filea} not in {fileb}")
for i in diff:
    print(i, end="")
print(f"in {fileb} not in {filea}")
for i in diff2:
    print(i, end="")
