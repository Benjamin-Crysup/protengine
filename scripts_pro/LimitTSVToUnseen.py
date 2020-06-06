
import sys

haveFoundN = []
wantFindN = None
dumpN = None
for argS in sys.argv[1:]:
    if argS.startswith("-kno="):
        haveFoundN.append(argS[5:])
    elif argS.startswith("-unk="):
        wantFindN = argS[5:]
    elif argS.startswith("-out="):
        dumpN = argS[5:]
    else:
        raise ValueError("Unknown argument " + argS)

# open the output
dumpF = open(dumpN, "w")

# open the known, skip first line
haveF = [open(cfN, "r") for cfN in haveFoundN]
for cf in haveF:
    cf.readline()
def filterHave(havF, havL):
    i = 0
    while i < len(havF):
        havEnd = False
        while havL[i].strip() == "":
            if havL[i] == "":
                havEnd = True
                break
            havL[i] = havF[i].readline()
        if havEnd:
            havF[i].close()
            havF.pop(i)
            havL.pop(i)
        else:
            i = i + 1

# open the search, pipe first line to output
wantF = open(wantFindN, "r")
dumpF.write(wantF.readline())

# read the first data line of want and have
curWant = wantF.readline()
curHave = [cf.readline() for cf in haveF]
filterHave(haveF, curHave)
while (curWant != "") and (len(curHave) > 0):
    if curWant.strip() == "":
        curWant = wantF.readline()
        continue
    ateWant = False
    recycleWant = False
    wantSpl = curWant.split()
    for i in range(len(haveF)):
        haveSpl = curHave[i].split()
        if wantSpl[0] == haveSpl[0]:
            ateWant = True
            break
        if wantSpl[0] > haveSpl[0]:
            recycleWant = True
            curHave[i] = haveF[i].readline()
    if recycleWant:
        filterHave(haveF, curHave)
    if ateWant:
        curWant = wantF.readline()
    if recycleWant or ateWant:
        continue
    dumpF.write(curWant)
    curWant = wantF.readline()
while curWant != "":
    if curWant.strip() == "":
        curWant = wantF.readline()
        continue
    dumpF.write(curWant)
    curWant = wantF.readline()
wantF.close()
dumpF.close()
for cf in haveF:
    cf.close()
