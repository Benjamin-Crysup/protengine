
import sys
import struct

inTabN = None
inDatN = None
outSaveN = None
for argS in sys.argv[1:]:
    if argS.startswith("-inT="):
        inTabN = argS[5:]
    elif argS.startswith("-inD="):
        inDatN = argS[5:]
    elif argS.startswith("-outI="):
        outSaveN = argS[6:]
    else:
        raise ValueError("Unknown argument " + argS)

def findProteinDataByName(protMajNam, protMinNam, inTabF, inDatF, numP):
    lowBnd = 0
    higBnd = numP
    while (higBnd - lowBnd) > 0:
        midBnd = (higBnd + lowBnd) // 2
        inTabF.seek(64*midBnd+8)
        curEnt = struct.unpack(">QQQQQQQQ",inTabF.read(64))
        inDatF.seek(curEnt[0])
        majNam = inDatF.read(struct.unpack(">H",inDatF.read(2))[0]).decode("utf-8")
        inDatF.seek(curEnt[1])
        minNam = inDatF.read(struct.unpack(">H",inDatF.read(2))[0]).decode("utf-8")
        if (majNam < protMajNam) or ((majNam == protMajNam) and (minNam < protMinNam)):
            lowBnd = midBnd + 1
        else:
            higBnd = midBnd
    inTabF.seek(64*lowBnd+8)
    curEnt = struct.unpack(">QQQQQQQQ",inTabF.read(64))
    inDatF.seek(curEnt[0])
    majNam = inDatF.read(struct.unpack(">H",inDatF.read(2))[0]).decode("utf-8")
    inDatF.seek(curEnt[1])
    minNam = inDatF.read(struct.unpack(">H",inDatF.read(2))[0]).decode("utf-8")
    if (majNam != protMajNam) or (minNam != protMinNam):
        # require sane nucleic acid space projection
        return None
    return curEnt

# open the files of interest
inTab = open(inTabN, "rb")
inDat = open(inDatN, "rb")
outSave = open(outSaveN, "w")
# number of unique sequences
numProt = struct.unpack(">Q",inTab.read(8))[0]

print("peptide_seq\tfound_in\tfound_who")
seenNames = {}
seenInds = {}
nextNI = 0
foundInInd = -1
for line in sys.stdin:
    lineSpl = line.split()
    if len(lineSpl) == 0:
        continue
    if foundInInd < 0:
        foundInInd = lineSpl.index("found_in")
    else:
        foundPep = lineSpl[0]
        foundIn = lineSpl[foundInInd]
        if foundIn == '""':
            print(foundPep + '\t""\t""')
            continue
        fiSpl = foundIn.split(",")
        allWho = []
        for majmin in fiSpl:
            mmSpl = majmin.split("_")
            protEnt = findProteinDataByName(mmSpl[0], mmSpl[1], inTab, inDat, numProt)
            if protEnt is None:
                allWho.append("")
                continue
            inDat.seek(protEnt[2])
            numNam = struct.unpack(">I",inDat.read(4))[0]
            allINamLocs = struct.unpack(">" + ("I" * numNam), inDat.read(numNam*4))
            allNames = []
            for namL in allINamLocs:
                if not (namL in seenNames):
                    inDat.seek(namL)
                    seenNames[namL] = inDat.read(struct.unpack(">H",inDat.read(2))[0]).decode("utf-8")
                    seenInds[namL] = repr(nextNI)
                    nextNI = nextNI + 1
                allNames.append(seenInds[namL])
            allWho.append("|".join(allNames))
        print(foundPep + "\t" + foundIn + "\t" + ",".join(allWho))
inTab.close()
inDat.close()

for namL in seenInds:
    outSave.write(seenInds[namL] + "\t" + seenNames[namL] + "\n")
outSave.close()
