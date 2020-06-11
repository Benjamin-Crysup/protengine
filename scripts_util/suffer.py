'''
    Extract information from a suffix array.
'''

import os
import struct
import subprocess

import whodun_cigfq

def _killAllTempFiles(allFs, foldName, madeFold):
    for fn in allFs:
        if os.path.exists(fn):
            os.remove(fn)
    if madeFold and os.path.exists(foldName):
        os.rmdir(foldName)

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

class SuffixArraySearch:
    '''Get people by peptide.'''
    def __init__(self, progLoc, sufLoc, namFLoc, workFold, cutFile):
        '''
            Set up a search object.
            @param progLoc: The location of the Protengine program (and friends).
            @param sufLoc: The location of the suffix array folder (with data.bin and table.bin).
            @param namFLoc: The name of the protein name file.
            @param workFold: The folder to store temporary files in.
            @param cutFile: The location of the valid cut file.
        '''
        self.progLoc = progLoc
        self.sufLoc = sufLoc
        self.namFLoc = namFLoc
        self.workFold = workFold
        self.cutFile = cutFile
        # sys.executable for python program
    def listAllPeople(self):
        '''
            List all the people in the database.
            @return: A list of the names of the people in the array.
        '''
        inTabN = os.path.join(self.sufLoc, "table.bin")
        inDatN = os.path.join(self.sufLoc, "data.bin")
        inTab = open(inTabN, "rb")
        inDat = open(inDatN, "rb")
        numProt = struct.unpack(">Q",inTab.read(8))[0]
        # get the location of the first major name
        curEnt = struct.unpack(">QQQQQQQQ",inTab.read(64))
        firstMajL = curEnt[0]
        allPpl = []
        curDLoc = 0
        while curDLoc < firstMajL:
            curPLen = struct.unpack(">H",inDat.read(2))[0]
            curNam = inDat.read(curPLen).decode("utf-8")
            allPpl.append(curNam)
            curDLoc = curDLoc + 2 + curPLen
        inTab.close()
        inDat.close()
        return allPpl[1:]
    def loadProteinNames(self):
        allPNames = []
        pnamF = open(self.namFLoc, "r")
        for line in pnamF:
            if line.strip() == "":
                continue
            allPNames.append(line.strip())
        pnamF.close()
        return allPNames
    def runPeptideSearch(self, forPep, tmpSave):
        # build the fasta
        fastaName = os.path.abspath(os.path.join(self.workFold, "searchit.fa"))
        fastaF = open(fastaName, "w")
        for seq in forPep:
            fastaF.write(">lookie\n")
            fastaF.write(seq + "\n")
        fastaF.close()
        tmpSave.append(fastaName)
        # run protengine
        srcResN = os.path.abspath(os.path.join(self.workFold, "finds.txt"))
        srcRegN = os.path.abspath(os.path.join(self.workFold, "findr.txt"))
        tmpSave.append(srcResN)
        tmpSave.append(srcRegN)
        protProc = subprocess.Popen([os.path.abspath(os.path.join(self.progLoc, "Protengine"))], stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, cwd=self.workFold, universal_newlines=True)
        protProc.stdin.write("MaxRam 1000000000\n")
        protProc.stdin.write("Thread 8\n")
        protProc.stdin.write("IndexInit ind " + os.path.abspath(os.path.join(self.sufLoc, "ind")) + "\n")
        protProc.stdin.write("IndexReference ind e25\n")
        protProc.stdin.write("BuildSAPrecom ind e25\n")
        protProc.stdin.write("BuildComboPrecom ind ce25 e25\n")
        protProc.stdin.write("FindComboPrecom ind ce25 " + fastaName + " " + srcResN + "\n")
        protProc.stdin.write("IndexDumpSearchRegion " + fastaName + " " + srcResN + " 2 2 " + srcRegN + "\n")
        protProc.stdin.close()
        if protProc.wait() != 0:
            raise IOError("Problem running Protengine.")
        # filter on search cut validity
        srcFiltN = os.path.abspath(os.path.join(self.workFold, "filts.txt"))
        subprocess.run([os.path.abspath(os.path.join(self.progLoc, "FilterSearchCutValidity")), "-res=" + srcResN, "-reg=" + srcRegN, "-dig=" + self.cutFile, "-out=" + srcFiltN], stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
        tmpSave.append(srcFiltN)
        # load in the results
        pepProtLocs = [[] for _ in forPep]
        allPepProts = [[] for _ in forPep]
        filtF = open(srcFiltN, "r")
        for line in filtF:
            if line.strip() == "":
                continue
            lineS = line.split()
            inPep = int(lineS[0])
            inPro = int(lineS[3])
            inLoc = int(lineS[4])
            allPepProts[inPep].append(inPro)
            pepProtLocs[inPep].append(inLoc)
        filtF.close()
        return [allPepProts, pepProtLocs]
    def findProtDataEntries(self, allPepProts, allPNames, missProts):
        inTabN = os.path.join(self.sufLoc, "table.bin")
        inDatN = os.path.join(self.sufLoc, "data.bin")
        inTab = open(inTabN, "rb")
        inDat = open(inDatN, "rb")
        numProt = struct.unpack(">Q",inTab.read(8))[0]
        protEnts = {}
        for i in range(len(allPepProts)):
            curPepProts = allPepProts[i]
            for prot in curPepProts:
                if allPNames[prot] in protEnts:
                    continue
                mmSpl = allPNames[prot].split("_")
                protEnt = findProteinDataByName(mmSpl[0], mmSpl[1], inTab, inDat, numProt)
                if protEnt is None:
                    missProts.add(allPNames[prot])
                    continue
                protEnts[allPNames[prot]] = protEnt
        inTab.close()
        inDat.close()
        return protEnts
    def findOwners(self, forPep):
        '''
            Look for a set of peptides and find the individuals who have them.
            @param forPep: The peptides to look for. List of string.
            @return: The people who have each peptide. Three tuple, first thing is list of list of integer (person indices for each peptide), second thing is list of strings (people names), this is a list of strings (proteins that did not have relevant data).
        '''
        toRet = [None, None, None]
        progFold = self.progLoc
        allTmpFiles = []
        allPNames = self.loadProteinNames()
        #************************************************************
        # make the working folder
        madeWork = False
        workFold = self.workFold
        if not os.path.exists(workFold):
            os.mkdir(workFold)
            madeWork = True
        if not os.path.isdir(workFold):
            raise IOError("Working directory " + workFold + " is not a folder.")
        try:
            psRes = self.runPeptideSearch(forPep, allTmpFiles)
            allPepProts = psRes[0]
            pepProtLocs = psRes[1]
            missProts = set()
            protTabIs = self.findProtDataEntries(allPepProts, allPNames, missProts)
            #************************************************************
            # find their info in the table, note the addresses of the peoples
            inDat = open(os.path.join(self.sufLoc, "data.bin"), "rb")
            pepPeepAddrs = [set() for _ in forPep]
            for i in range(len(forPep)):
                curPepProts = allPepProts[i]
                curSet = pepPeepAddrs[i]
                for prot in curPepProts:
                    if not (allPNames[prot] in protTabIs):
                        continue
                    protEnt = protTabIs[allPNames[prot]]
                    inDat.seek(protEnt[2])
                    numNam = struct.unpack(">I",inDat.read(4))[0]
                    allINamLocs = struct.unpack(">" + ("I" * numNam), inDat.read(numNam*4))
                    curSet.update(allINamLocs)
            #************************************************************
            # grab the names
            allNameLocs = set()
            for curSet in pepPeepAddrs:
                allNameLocs.update(curSet)
            allNames = {}
            for loc in allNameLocs:
                inDat.seek(loc)
                allNames[loc] = inDat.read(struct.unpack(">H",inDat.read(2))[0]).decode("utf-8")
            linNames = []
            nameInds = {}
            for loc in allNames:
                nameInds[loc] = len(linNames)
                linNames.append(allNames[loc])
            #************************************************************
            # prepare the return
            fullDump = [[nameInds[pnl] for pnl in pepS] for pepS in pepPeepAddrs]
            toRet[0] = fullDump
            toRet[1] = linNames
            toRet[2] = list(missProts)
            inDat.close()
        finally:
            #************************************************************
            # clean up
            _killAllTempFiles(allTmpFiles, workFold , madeWork)
        #return
        return tuple(toRet)
    def proteinExpandCigarGenomeInfo(self, inDat, protEnt):
        # load the start locations
        inDat.seek(protEnt[3])
        numSL = struct.unpack(">H", inDat.read(2))[0]
        startLocs = struct.unpack(">" + "Q"*numSL, inDat.read(8*numSL))
        # end locs
        inDat.seek(protEnt[4])
        numEL = struct.unpack(">H", inDat.read(2))[0]
        endLocs = struct.unpack(">" + "Q"*numSL, inDat.read(8*numEL))
        if numSL != numEL:
            raise IOError("Malformed database.")
        # chromosome
        inDat.seek(protEnt[5]+2)
        onChrom = inDat.read(struct.unpack(">H",inDat.read(2))[0]).decode("utf-8")
        # cigar
        inDat.seek(protEnt[6])
        cigStr = inDat.read(struct.unpack(">I",inDat.read(4))[0]).decode("utf-8")
        # strand
        inDat.seek(protEnt[7])
        strand = inDat.read(struct.unpack(">H",inDat.read(2))[0]).decode("utf-8")
        # start and end
        allLocs = []
        for i in range(numSL):
            allLocs.extend(range(startLocs[i], endLocs[i]))
        if len(allLocs) % 3:
            if strand == "+":
                allLocs = allLocs[0:(3*(len(allLocs)//3))]
            else:
                allLocs = allLocs[(len(allLocs)%3):]
        # reverse if on the reverse strand
        if strand != "+":
            allLocs = list(reversed(allLocs))
        # cigar to index list
        cigLocRet = whodun_cigfq.cigarStringToReferenceLocations(bytearray(cigStr, "utf-8"), 0)
        if cigLocRet[-2] > 0:
            raise IOError("Can't handle soft clipping in database.")
        cigLocRet = cigLocRet[:-2]
        #print(startLocs)
        #print(endLocs)
        #print(cigStr)
        #print(strand)
        return [allLocs, cigLocRet, onChrom]
    def findSAPLocations(self, forPep, forChrom, forLocO):
        '''
            Look for a set of peptides and find the locations of SAPs withing them.
            @param forPep: The peptides to look for. List of string.
            @param forChrom: The chromosomes the SNPs of interest are on. List of string.
            @param forLoc: The genomic locations of the SNPs of interest. List of int.
            @return: The relevant location information of the peptides. TODO
        '''
        #pepcoord [pepInd] [matchInd] [start_prot, end_prot, coord_list, protInd, start_ref_prot, end_ref_prot]
        #sapcoord [sapInd] [matchInd] [peptide number, protein number, pep_ind, prot_ind]
        #return [pepcoord, sapcoord]
        toRet = [None, None]
        allTmpFiles = []
        allPNames = self.loadProteinNames()
        forLoc = [obl - 1 for obl in forLocO]
        #************************************************************
        # make the working folder
        madeWork = False
        workFold = self.workFold
        if not os.path.exists(workFold):
            os.mkdir(workFold)
            madeWork = True
        if not os.path.isdir(workFold):
            raise IOError("Working directory " + workFold + " is not a folder.")
        try:
            psRes = self.runPeptideSearch(forPep, allTmpFiles)
            allPepProts = psRes[0]
            pepProtLocs = psRes[1]
            missProts = set()
            protTabIs = self.findProtDataEntries(allPepProts, allPNames, missProts)
            # get location data for each protein
            protLocD = {}
            inDat = open(os.path.join(self.sufLoc, "data.bin"), "rb")
            for pname in protTabIs:
                #print(pname)
                protLocD[pname] = self.proteinExpandCigarGenomeInfo(inDat, protTabIs[pname])
            inDat.close()
            # fill in the data for the peptides
            pepRet = [[] for _ in forPep]
            for i in range(len(forPep)):
                curProts = allPepProts[i]
                curLocs = pepProtLocs[i]
                fillRet = pepRet[i]
                for j in range(len(curProts)):
                    curPI = curProts[j]
                    curL = curLocs[j]
                    locNote = ""
                    if allPNames[curPI] in protLocD:
                        curPL = protLocD[allPNames[curPI]]
                        curPGl = curPL[0]
                        curPCl = curPL[1]
                        curPChr = curPL[2]
                        pepCigLs = curPCl[curL:(curL+len(forPep[i]))]
                        pepCigLs = [ci for ci in pepCigLs if ci >= 0]
                        refPS = min(pepCigLs)
                        refPE = max(pepCigLs)
                        pepCigLs = [curPGl[3*ci] for ci in pepCigLs] + [curPGl[3*ci+1] for ci in pepCigLs] + [curPGl[3*ci+2] for ci in pepCigLs]
                        locNote = curPChr + ":" + repr(min(pepCigLs)) + "-" + repr(max(pepCigLs))
                    curRep = [curL, curL + len(forPep[i]), locNote, curPI, refPS, refPE]
                    fillRet.append(curRep)
            toRet[0] = pepRet
            # fill in the data for the snps
            snpRet = [[] for _ in forLoc]
            for i in range(len(forLoc)):
                # for each snp, note which proteins are on the same chromosome (and contain the address of interest)
                curValP = set([cpnam for cpnam in protLocD if (protLocD[cpnam][2] == forChrom[i]) and (forLoc[i] in protLocD[cpnam][0]) ])
                # run through all the peptides, note which ones have a valid protein
                livePeps = []
                for j in range(len(forPep)):
                    curPInf = pepRet[j]
                    for k in range(len(curPInf)):
                        if allPNames[curPInf[k][3]] in curValP:
                            livePeps.append([j,k])
                # run through all valid peptides, note where the snp is in the protein, and see if that is in the peptide
                winPeps = []
                for curPep in livePeps:
                    pi = curPep[0]
                    pj = curPep[1]
                    #[start_prot, end_prot, coord_list, protInd]
                    cpepInf = pepRet[pi][pj]
                    snpPLoc = protLocD[allPNames[cpepInf[3]]][0].index(forLoc[i])
                    sapPLoc = snpPLoc // 3
                    # check against the cigar
                    proCigLs = protLocD[allPNames[cpepInf[3]]][1]
                    if not (sapPLoc in proCigLs):
                        continue
                    sapPLoc = proCigLs.index(sapPLoc)
                    sapPepLoc = -1
                    if (sapPLoc >= cpepInf[0]) and (sapPLoc < cpepInf[1]):
                        # [pepInd, prot_ind, pep_ind]
                        sapPepLoc = sapPLoc - cpepInf[0]
                    snpRet[i].append([pi, cpepInf[3], sapPepLoc, sapPLoc])
            toRet[1] = snpRet
        finally:
            #************************************************************
            # clean up
            _killAllTempFiles(allTmpFiles, workFold , madeWork)
        #return
        return tuple(toRet)



