

"""Codes for mangling cigar strings, quality scores and other sam/fq information."""
import gpmatlan

def isCigarLetterDigit(testChar):
	"""
		Gets whether a cigar character is a digit.
		@param testChar The character to test
		@return Whether the test character is a digit
	"""
	zero = 48
	nine = 57
	gteZ = None
	gteZ = (testChar >= zero)
	lteN = None
	lteN = (testChar <= nine)
	retV = None
	retV = (gteZ and lteN)
	return retV


def cigarStringToReferenceLocations(cigar, refStart):
	"""
		Figures out the reference locations given a cigar string.
		@param refStart The first index for the cigar string
		@param cigar The cigar string in question.
		@return The locations the characters in the cigar string map to, the number of sequence characters to skip, the number of characters actually in the cigar string.
	"""
	zero = 0
	one = 1
	casciiS = 83
	casciiH = 72
	casciiP = 80
	casciiD = 68
	casciiN = 78
	casciiI = 73
	casciiM = 77
	casciiEq = 61
	casciiX = 88
	cigLen = None
	cigLen = len(cigar)
	firstOff = None
	firstOff = zero
	realLen = None
	realLen = zero
	curRLoc = None
	curRLoc = refStart
	cigLocs = [0 for _ in range(0, zero)]
	lastNum = bytearray([0 for _ in range(0, zero)])
	i = None
	i = zero
	im = None
	im = (i < cigLen)
	while im:
		pass
		curChar = None
		curChar = cigar[i]
		isDig = None
		isDig = isCigarLetterDigit(curChar)
		if isDig:
			pass
			lastNum.append(curChar)
		else:
			pass
			lnumLen = None
			lnumLen = len(lastNum)
			numlpro = None
			numlpro = (lnumLen == zero)
			if numlpro:
				pass
				raise ValueError("Cigar operation missing count.")
			lnum = None
			lnum = int(lastNum.decode("utf-8"))
			lastNum.clear()
			numlpro = (lnum < zero)
			if numlpro:
				pass
				raise ValueError("Cigar operation count less than zero.")
			isS = None
			isS = (curChar == casciiS)
			isH = None
			isH = (curChar == casciiH)
			isP = None
			isP = (curChar == casciiP)
			isD = None
			isD = (curChar == casciiD)
			isN = None
			isN = (curChar == casciiN)
			isDN = None
			isDN = (isD or isN)
			isI = None
			isI = (curChar == casciiI)
			isM = None
			isM = (curChar == casciiM)
			isEq = None
			isEq = (curChar == casciiEq)
			isX = None
			isX = (curChar == casciiX)
			isMEX = None
			isMEX = (isM or isEq)
			isMEX = (isMEX or isX)
			if isS:
				pass
				cigLSize = None
				cigLSize = len(cigLocs)
				cigLZ = None
				cigLZ = (cigLSize == zero)
				if cigLZ:
					pass
					firstOff = (firstOff + lnum)
			elif isH:
				pass
			elif isP:
				pass
			elif isDN:
				pass
				curRLoc = (curRLoc + lnum)
			elif isI:
				pass
				j = None
				j = zero
				jm = None
				jm = (j < lnum)
				while jm:
					pass
					negRLoc = None
					negRLoc = (curRLoc + one)
					negRLoc = -(negRLoc)
					cigLocs.append(negRLoc)
					j = (j + one)
					jm = (j < lnum)
				realLen = (realLen + lnum)
			elif isMEX:
				pass
				j = None
				j = zero
				jm = None
				jm = (j < lnum)
				while jm:
					pass
					cigLocs.append(curRLoc)
					curRLoc = (curRLoc + one)
					j = (j + one)
					jm = (j < lnum)
				realLen = (realLen + lnum)
			else:
				pass
				raise ValueError("Unknown operation code in CIGAR string.")
		i = (i + one)
		im = (i < cigLen)
	cigLocs.append(firstOff)
	cigLocs.append(realLen)
	return cigLocs


def phredAsciiToLogProb(phredAscii):
	"""
		Turns ascii phred scores to log probabilities.
		@param phredAscii The phred score string to convert.
		@return The phred scores as log probabilities.
	"""
	zero = 0
	one = 1
	negThirtyThree = -33.0
	negTen = -10.0
	phredLen = None
	phredLen = len(phredAscii)
	probVals = [0.0 for _ in range(0, phredLen)]
	i = None
	i = zero
	im = None
	im = (i < phredLen)
	while im:
		pass
		curChar = None
		curChar = phredAscii[i]
		curCharF = None
		curCharF = curChar
		curCharF = (curCharF + negThirtyThree)
		curCharF = (curCharF / negTen)
		probVals[i] = curCharF
		i = (i + one)
		im = (i < phredLen)
	return probVals


def logProbToPhredAscii(probVals):
	"""
		Turns log probabilities into quality scores.
		@param probVals The log probability values to convert.
		@return The phred quality scores
	"""
	zero = 0
	one = 1
	threeThreeI = 33
	oneTwoSix = 126
	negTen = -10.0
	phredLen = None
	phredLen = len(probVals)
	phredAscii = bytearray([0 for _ in range(0, phredLen)])
	i = None
	i = zero
	im = None
	im = (i < phredLen)
	while im:
		pass
		curChar = None
		curChar = probVals[i]
		curChar = (curChar * negTen)
		curCharI = None
		curCharI = int(curChar)
		curCharI = (curCharI + threeThreeI)
		isPro = None
		isPro = (curCharI < threeThreeI)
		if isPro:
			pass
			curCharI = threeThreeI
		isPro = (curCharI > oneTwoSix)
		if isPro:
			pass
			curCharI = oneTwoSix
		curCharC = None
		curCharC = 0x00FF & curCharI
		phredAscii[i] = curCharC
		i = (i + one)
		im = (i < phredLen)
	return phredAscii


def phredCharToLogProb(phredAscii):
	"""
		Turns ascii phred scores to log probabilities.
		@param phredAscii The phred score to convert.
		@return The phred score as a log probability.
	"""
	negThirtyThree = -33.0
	negTen = -10.0
	curChar = None
	curChar = phredAscii
	curCharF = None
	curCharF = curChar
	curCharF = (curCharF + negThirtyThree)
	curCharF = (curCharF / negTen)
	return curCharF


def logProbToPhredChar(probVals):
	"""
		Turns a log probability into a quality score.
		@param probVals The log probability value to convert.
		@return The phred quality score
	"""
	threeThreeI = 33
	oneTwoSix = 126
	negTen = -10.0
	curChar = None
	curChar = probVals
	curChar = (curChar * negTen)
	curCharI = None
	curCharI = int(curChar)
	curCharI = (curCharI + threeThreeI)
	isPro = None
	isPro = (curCharI < threeThreeI)
	if isPro:
		pass
		curCharI = threeThreeI
	isPro = (curCharI > oneTwoSix)
	if isPro:
		pass
		curCharI = oneTwoSix
	curCharC = None
	curCharC = 0x00FF & curCharI
	return curCharC

