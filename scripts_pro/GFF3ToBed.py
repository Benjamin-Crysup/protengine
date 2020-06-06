import sys

inFil = sys.stdin
outFil = sys.stdout
chromoId = "somewhere"
for argS in sys.argv[1:]:
    if argS.startswith("-chr="):
        chromoId = argS[5:]
    elif argS.startswith("-in="):
        inFil = open(argS[4:], "r")
    elif argS.startswith("-out="):
        outFil = open(argS[5:], "w")
    else:
        raise ValueError("Unknown argument " + argS)

for clin in inFil:
    if clin.strip() == "":
        continue
    csplit = clin.split()
    if len(csplit) < 9:
        continue
    if csplit[2] != "CDS":
        continue
    chromName = "chr" + csplit[0]
    startInd = int(csplit[3]) - 1
    endInd = int(csplit[4])
    protNam = None
    attribs = csplit[8].split(";")
    for atr in attribs:
        atspl = atr.split("=")
        if len(atspl) != 2:
            continue
        if atspl[0] == "protein_id":
            protNam = atspl[1]
    if protNam is None:
        continue
    outFil.write(chromName + "\t")
    outFil.write(repr(startInd) + "\t")
    outFil.write(repr(endInd) + "\t")
    outFil.write(protNam + "\t")
    outFil.write(csplit[6] + "\n")

inFil.close()
outFil.close()
