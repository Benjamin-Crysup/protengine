import sys
import struct

inBedN = None
inLUTN = None
outTabN = None
outDatN = None
maxBed = 100000
skipFirst = True
for argS in sys.argv[1:]:
    if argS.startswith("-inB="):
        inBedN = argS[5:]
    elif argS.startswith("-inL="):
        inLUTN = argS[5:]
    elif argS.startswith("-outT="):
        outTabN = argS[6:]
    elif argS.startswith("-outD="):
        outDatN = argS[6:]
    else:
        raise ValueError("Unknown argument " + argS)

# open the output files
outTab = open(outTabN, "wb")
outDat = open(outDatN, "wb")
# put a placeholder zero in outTab
outTab.write(struct.pack(">Q", 0))

# note the individuals in the LUT
allIndivL = {}
inLUT = open(inLUTN, "r")
for cline in inLUT:
    if cline.strip() == "":
        continue
    csplit = cline.split()
    if len(csplit) < 3:
        continue
    if csplit[1] in allIndivL:
        continue
    curL = outDat.tell()
    toOutS = bytes(csplit[1],"utf-8")
    outDat.write(struct.pack(">H",len(toOutS)))
    outDat.write(toOutS)
    allIndivL[csplit[1]] = curL
inLUT.close()

def scanForProteins(onBedEnts, inLUTFN, outTabF, outDatF, indivLocs):
    # map the bed entries to indices
    bedIndM = {}
    for benti in range(len(onBedEnts)):
        bedIndM[onBedEnts[benti][0]] = benti
    # create space for the load
    # inLUTEnt[major_name][minor_name] = [cigar,[names]]
    inLUTEnt = {}
    for bent in onBedEnts:
        inLUTEnt[bent[0]] = {}
    # run through the lookup table
    inLUTF = open(inLUTFN, "r")
    for cline in inLUTF:
        if cline.strip() == "":
            continue
        csplit = cline.split()
        if len(csplit) < 3:
            continue
        proMajMin = csplit[0].split("_")
        if not (proMajMin[0] in inLUTEnt):
            continue
        centMaj = inLUTEnt[proMajMin[0]]
        if not (proMajMin[1] in centMaj):
            centMaj[proMajMin[1]] = [csplit[2],[]]
        namLst = centMaj[proMajMin[1]][1]
        namLst.append(csplit[1])
    inLUTF.close()
    # dump out the data
    #   Q major name offset
    #   Q minor name offset
    #   Q individual array offset
    #   Q start array offset
    #   Q end array offset
    #   Q chromo array offset
    #   Q cigar offset
    #   Q strand offset
    numWritten = 0
    for bent in onBedEnts:
        # dump out the common stuff, note their locations
        majNameLoc = outDatF.tell()
        toOutS = bytes(bent[0],"utf-8")
        outDatF.write(struct.pack(">H",len(toOutS)))
        outDatF.write(toOutS)
        startArrLoc = outDatF.tell()
        outDatF.write(struct.pack(">H",len(bent[2])))
        for spos in bent[2]:
            outDatF.write(struct.pack(">Q",spos))
        endArrLoc = outDatF.tell()
        outDatF.write(struct.pack(">H",len(bent[3])))
        for spos in bent[3]:
            outDatF.write(struct.pack(">Q",spos))
        chrArrLoc = outDatF.tell()
        outDatF.write(struct.pack(">H",len(bent[1])))
        for spos in bent[1]:
            toOutS = bytes(spos,"utf-8")
            outDatF.write(struct.pack(">H",len(toOutS)))
            outDatF.write(toOutS)
        strandLoc = outDatF.tell()
        toOutS = bytes(bent[4],"utf-8")
        outDatF.write(struct.pack(">H",len(toOutS)))
        outDatF.write(toOutS)
        # run through the variants
        centMaj = inLUTEnt[bent[0]]
        centKeySrt = sorted(centMaj)
        for minorN in centKeySrt:
            # write the info for this variant
            minDat = centMaj[minorN]
            minorNLoc = outDatF.tell()
            toOutS = bytes(minorN,"utf-8")
            outDatF.write(struct.pack(">H",len(toOutS)))
            outDatF.write(toOutS)
            cigarLoc = outDatF.tell()
            toOutS = bytes(minDat[0],"utf-8")
            outDatF.write(struct.pack(">I",len(toOutS)))
            outDatF.write(toOutS)
            namLis = minDat[1]
            namLisLoc = outDatF.tell()
            outDatF.write(struct.pack(">I",len(namLis)))
            for cnam in namLis:
                outDatF.write(struct.pack(">I",indivLocs[cnam]))
            # and note the locations in the main file
            outTabF.write(struct.pack(">Q",majNameLoc))
            outTabF.write(struct.pack(">Q",minorNLoc))
            outTabF.write(struct.pack(">Q",namLisLoc))
            outTabF.write(struct.pack(">Q",startArrLoc))
            outTabF.write(struct.pack(">Q",endArrLoc))
            outTabF.write(struct.pack(">Q",chrArrLoc))
            outTabF.write(struct.pack(">Q",cigarLoc))
            outTabF.write(struct.pack(">Q",strandLoc))
            numWritten = numWritten + 1
    return numWritten

# run through the bed file, chunking into groups of 1000 entries
totNumW = 0
inBed = open(inBedN, "r")
bedCollect = []
lastCollect = None
for cline in inBed:
    if cline.strip() == "":
        continue
    if skipFirst:
        skipFirst = False
        continue
    csplit = cline.split()
    if len(csplit) < 5:
        continue
    if (lastCollect is None) or (lastCollect[0] != csplit[3]):
        if len(bedCollect) >= maxBed:
            totNumW = totNumW + scanForProteins(bedCollect, inLUTN, outTab, outDat, allIndivL)
            bedCollect = []
        lastCollect = [csplit[3], [], [], [], csplit[4]]
        bedCollect.append(lastCollect)
    lastCollect[1].append(csplit[0])
    lastCollect[2].append(int(csplit[1]))
    lastCollect[3].append(int(csplit[2]))
if len(bedCollect) > 0:
    totNumW = totNumW + scanForProteins(bedCollect, inLUTN, outTab, outDat, allIndivL)
inBed.close()
outDat.close()

# go back and note the number written
outTab.seek(0)
outTab.write(struct.pack(">Q", totNumW))
outTab.close()
