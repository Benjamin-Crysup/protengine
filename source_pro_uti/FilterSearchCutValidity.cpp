
#include <vector>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <iostream>

#define WHITESPACE " \t\r\n"

/**
 * This will parse a hex nibble.
 * @param cval The nibble text.
 * @return The value, or -1 if invalid.
 */
int parseHexNibble(char cval){
	if((cval >= '0') && (cval <= '9')){
		return cval - '0';
	}
	else if((cval >= 'A') && (cval <= 'F')){
		return 10 + (cval - 'A');
	}
	else if((cval >= 'a') && (cval <= 'a')){
		return 10 + (cval - 'a');
	}
	return -1;
}

/**
 * This will parse a hex byte.
 * @param cvalA The first value.
 * @param cvalB The second value.
 * @return The byte, or -1 if invalid.
 */
int parseHex(char cvalA, char cvalB){
	int ivalA = parseHexNibble(cvalA);
	if(ivalA < 0){ return -1; }
	int ivalB = parseHexNibble(cvalB);
	if(ivalB < 0){ return -1; }
	return (ivalA << 4) + ivalB;
}

/**A rule for a cut.*/
class CuttingRule{
public:
	/**Whether this is a bad rule.*/
	bool badRule;
	/**The length of the start prefix.*/
	int startPreLen;
	/**The valid characters of the start prefix, in reverse order.*/
	char** startPreRev;
	/**Whether all characters must be accounted for in the prefix.*/
	bool startPreAll;
	/**The length of the start suffix.*/
	int startPostLen;
	/**The valid characters of the start suffix.*/
	char** startPost;
	/**The length of the end prefix.*/
	int endPreLen;
	/**The valid characters of the end prefix, in reverse order.*/
	char** endPreRev;
	/**The length of the end suffix.*/
	int endPostLen;
	/**The valid characters of the end suffix.*/
	char** endPost;
	/**Whether all characters must be accounted for in the suffix.*/
	bool endPostAll;
	/**
	 * This will make a cutting rule by parsing a line.
	 * @param parseLine The line to parse.
	 */
	CuttingRule(const char* parseLine){
		//defaults
			badRule = true;
			startPreAll = false;
			endPostAll = false;
			startPreRev = 0;
			startPost = 0;
			endPreRev = 0;
			endPost = 0;
		const char* focChar = parseLine;
		//definitions: simple to use in the presence of errors
			std::vector<char*> allCharClass;
			#define ADVANCE_CHAR focChar++; if(!*focChar){ goto err_handler; }
			#define SKIP_WHITESPACE focChar = focChar + strspn(focChar, WHITESPACE); if(!*focChar){ goto err_handler; }
			#define PARSE_CHAR_CLASS \
				std::string hotChars;\
				char* curAdd;\
				switch(*focChar){\
					case '.':\
						curAdd = (char*)malloc(256);\
						for(int i=1; i<256; i++){curAdd[i-1] = i;}\
						curAdd[255] = 0;\
						allCharClass.push_back(curAdd);\
						break;\
					case '\\':\
						ADVANCE_CHAR\
						curAdd = (char*)malloc(2);\
						curAdd[0] = *focChar;\
						curAdd[1] = 0;\
						allCharClass.push_back(curAdd);\
						break;\
					case '[':\
						ADVANCE_CHAR\
						while(*focChar != ']'){\
							if(*focChar == '\\'){\
								ADVANCE_CHAR\
								hotChars.push_back(*focChar);\
							}\
							else{\
								char hexCA = *focChar;\
								ADVANCE_CHAR\
								char hexCB = *focChar;\
								int hexV = parseHex(hexCA, hexCB);\
								if(hexV < 0){ goto err_handler; }\
								hotChars.push_back(hexV);\
							}\
							ADVANCE_CHAR\
						}\
						curAdd = (char*)malloc(hotChars.size() + 1);\
						memcpy(curAdd, hotChars.c_str(), hotChars.size());\
						curAdd[hotChars.size()] = 0;\
						allCharClass.push_back(curAdd);\
						break;\
					case ' ': break;\
					case '\t': break;\
					default:\
						{\
							char hexCA = *focChar;\
							ADVANCE_CHAR\
							char hexCB = *focChar;\
							curAdd = (char*)malloc(2);\
							int hexV = parseHex(hexCA, hexCB);\
							if(hexV < 0){ goto err_handler; }\
							curAdd[0] = hexV;\
							curAdd[1] = 0;\
							allCharClass.push_back(curAdd);\
						}\
						break;\
				}\
				ADVANCE_CHAR
			#define BUILD_CHAR_CLASS(lenVar,seqVar,revInd) \
				lenVar = allCharClass.size();\
				seqVar = (char**)malloc(sizeof(char*)*lenVar);\
				for(unsigned ijk = 0; ijk<allCharClass.size(); ijk++){\
					seqVar[revInd] = allCharClass[ijk];\
				}
		SKIP_WHITESPACE
		//see if it needs all
		if(*focChar == '^'){
			startPreAll = true;
			ADVANCE_CHAR
		}
		//get characters until pipe
			while(*focChar != '|'){
				PARSE_CHAR_CLASS
			}
			BUILD_CHAR_CLASS(startPreLen,startPreRev,startPreLen-(ijk+1))
			ADVANCE_CHAR
		//get characters until dash
			allCharClass.clear();
			while(*focChar != '-'){
				PARSE_CHAR_CLASS
			}
			BUILD_CHAR_CLASS(startPostLen,startPost,ijk)
			ADVANCE_CHAR
		//get characters until pipe
			allCharClass.clear();
			while(*focChar != '|'){
				PARSE_CHAR_CLASS
			}
			BUILD_CHAR_CLASS(endPreLen,endPreRev,endPreLen-(ijk+1))
			ADVANCE_CHAR
		//get characters until exclamation or dollar
			allCharClass.clear();
			while(!((*focChar == '!') || (*focChar == '$'))){
				PARSE_CHAR_CLASS
			}
			BUILD_CHAR_CLASS(endPostLen,endPost,ijk)
		//see if the end needs all
		if(*focChar == '$'){
			endPostAll = true;
		}
		badRule = false;
		return;
		
		err_handler:
			for(unsigned ijk=0; ijk<allCharClass.size(); ijk++){free(allCharClass[ijk]);}
	}
	/**Kill the memory.*/
	~CuttingRule(){
		if(startPreRev){
			for(int i = 0; i<startPreLen; i++){
				free(startPreRev[i]);
			}
			free(startPreRev);
		}
		if(startPost){
			for(int i = 0; i<startPostLen; i++){
				free(startPost[i]);
			}
			free(startPost);
		}
		if(endPreRev){
			for(int i = 0; i<endPreLen; i++){
				free(endPreRev[i]);
			}
			free(endPreRev);
		}
		if(endPost){
			for(int i = 0; i<endPostLen; i++){
				free(endPost[i]);
			}
			free(endPost);
		}
	}
};

class CuttingRules{
public:
	/**The rules in question.*/
	std::vector<CuttingRule*> theRules;
	/**
	 * Start a search for matching rules.
	 * @param addLive The place to add live indices to.
	 */
	void startMatchSearch(std::vector<int>* addLive){
		addLive->clear();
		for(unsigned i = 0; i < theRules.size(); i++){
			addLive->push_back(i);
		}
	}
#define SCAN_CODE(lenVar, seqVar, needAGet, forRevIndGet) \
		int splen = strlen(toTest);\
		std::vector<int> nextLive;\
		for(unsigned i = 0; i<toCull->size(); i++){\
			int ci = (*toCull)[i];\
			CuttingRule* curRule = (theRules[ci]);\
			int testLen = curRule->lenVar;\
			char** testVals = curRule->seqVar;\
			bool needAll = needAGet;\
			if(splen < testLen){\
				continue;\
			}\
			if(needAll && (splen > testLen)){\
				continue;\
			}\
			bool foundIt = true;\
			for(int j = 0; j<testLen; j++){\
				char* valChars = testVals[j];\
				int curChar = toTest[forRevIndGet];\
				if(strchr(valChars, curChar) == 0){\
					foundIt = false;\
					break;\
				}\
			}\
			if(foundIt){\
				nextLive.push_back(ci);\
			}\
		}\
		toCull->clear();\
		toCull->insert(toCull->begin(), nextLive.begin(), nextLive.end());
	/**
	 * This will see if the prefix matches the prefix of the starting cut.
	 * @param toCull The live rules to work with.
	 * @param toTest The stuff before the match.
	 */
	void checkStartCutPrefix(std::vector<int>* toCull, const char* toTest){
		SCAN_CODE(startPreLen,startPreRev,curRule->startPreAll,splen-(j+1))
	}
	/**
	 * This will see if the suffix matches the suffix of the starting cut.
	 * @param toCull The live rules to work with.
	 * @param toTest The match.
	 */
	void checkStartCutSuffix(std::vector<int>* toCull, const char* toTest){
		SCAN_CODE(startPostLen,startPost,false,j)
	}
	/**
	 * This will see if the prefix matches the prefix of the ending cut.
	 * @param toCull The live rules to work with.
	 * @param toTest The match.
	 */
	void checkEndCutPrefix(std::vector<int>* toCull, const char* toTest){
		SCAN_CODE(endPreLen,endPreRev,false,splen-(j+1))
	}
	/**
	 * This will see if the suffix matches the suffix of the ending cut.
	 * @param toCull The live rules to work with.
	 * @param toTest The stuff after the match.
	 */
	void checkEndCutSuffix(std::vector<int>* toCull, const char* toTest){
		SCAN_CODE(endPostLen,endPost,curRule->endPostAll,j)
	}
	/**
	 * This will find all cuts the given match satisfies.
	 * @param preMatch The stuff before the match.
	 * @param match The match.
	 * @param postMatch The stuff after the match.
	 * @param toFill The place to put the indices of the matching rules.
	 */
	void getMatchingCuts(const char* preMatch, const char* match, const char* postMatch, std::vector<int>* toFill){
		toFill->clear();
		startMatchSearch(toFill);
		checkStartCutPrefix(toFill, preMatch);
		checkStartCutSuffix(toFill, match);
		checkEndCutPrefix(toFill, match);
		checkEndCutSuffix(toFill, postMatch);
	}
	/**
	 * Loads cutting rules from a file.
	 * @param toParse The file to parse. Closed on return.
	 * @param toRepErr The place to put errors, if any.
	 */
	CuttingRules(std::istream* toParse, std::ostream* toRepErr){
		int lineCount = 0;
		std::string curLine;
		while(std::getline(*toParse, curLine)){
			lineCount++;
			if(strspn(curLine.c_str(), WHITESPACE) == curLine.size()){
				continue;
			}
			CuttingRule* curRule = new CuttingRule(curLine.c_str());
			if(curRule->badRule){
				if(toRepErr){
					*toRepErr << "ERROR: Bad cutting rule on line " << lineCount << ": " << curLine << std::endl;
				}
				delete(curRule);
			}
			else{
				theRules.push_back(curRule);
			}
		}
	}
	/**Kill the memory.*/
	~CuttingRules(){
		for(unsigned i = 0; i<theRules.size(); i++){
			delete (theRules[i]);
		}
	}
};

#define CLARG_RES "-res="
#define CLARG_REG "-reg="
#define CLARG_DIG "-dig="
#define CLARG_OUT "-out="

const char* getParenthesisContents(const char* curFoc, std::string* toFill){
	toFill->clear();
	curFoc = curFoc + strspn(curFoc, WHITESPACE);
		if(*curFoc != '('){
			std::cerr << "Malformed region file." << std::endl;
			return 0;
		}
	curFoc++;
	while(true){
		switch(*curFoc){
			case 0:
				std::cerr << "Malformed region." << std::endl;
				return 0;
			case ')':
				goto loop_done;
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				break;
			default:
				toFill->push_back(*curFoc);
		}
		curFoc++;
	}
	loop_done:
	return curFoc + 1;
}

int collapseParenthesisNibbles(std::string* fromA, std::string* toA){
	if(fromA->size() % 2){
		return 1;
	}
	toA->clear();
	const char* startA = fromA->c_str();
	const char* endA = startA + fromA->size();
	while(startA != endA){
		int curBt = parseHex(*startA, startA[1]);
		if(curBt < 0){ return 1; }
		toA->push_back(curBt);
		startA += 2;
	}
	return 0;
}

int main(int argc, char** argv){
	//parse the arguments
		char* searchResN = 0;
		char* searchRegN = 0;
		char* digSpecN = 0;
		char* filtN = 0;
		#define CHECK_ARG(wantArg, endFoc) \
			if(strncmp(argv[i], wantArg, strlen(wantArg)) == 0){\
				endFoc = argv[i] + strlen(wantArg);\
				continue;\
			}
		for(int i = 1; i<argc; i++){
			CHECK_ARG(CLARG_RES, searchResN)
			CHECK_ARG(CLARG_REG, searchRegN)
			CHECK_ARG(CLARG_DIG, digSpecN)
			CHECK_ARG(CLARG_OUT, filtN)
		}
		if(!(searchResN && searchRegN && digSpecN && filtN)){
			std::cerr << "Missing required argument" << std::endl;
			return 1;
		}
	//get the rules
		std::ifstream ruleStr(digSpecN);
		CuttingRules digestRule(&ruleStr, &(std::cerr));
		ruleStr.close();
	//open the result and region files
		std::ifstream resFile(searchResN);
		std::ifstream regFile(searchRegN);
	//and read
		std::ofstream outFile(filtN);
		std::string preRegA; std::string preRegB;
		std::string midRegA; std::string midRegB;
		std::string posRegA; std::string posRegB;
		std::vector<int> winRules;
		std::string resline;
		std::string regline;
		int lineCount = 0;
		while(std::getline(resFile, resline)){
			if(!(std::getline(regFile, regline))){
				std::cerr << "Result and region file mismatched." << std::endl;
				return 1;
			}
			lineCount++;
			//std::cout << "Line " << lineCount << ":" << std::flush;
			//split the regline into parenthetical regions
				const char* curFoc = regline.c_str();
				curFoc = getParenthesisContents(curFoc, &preRegA);
				if(curFoc == 0){ return 1; }
				//std::cout << "GPre " << std::flush;
				curFoc = getParenthesisContents(curFoc, &midRegA);
				if(curFoc == 0){ return 1; }
				//std::cout << "GMid " << std::flush;
				curFoc = getParenthesisContents(curFoc, &posRegA);
				if(curFoc == 0){ return 1; }
				//std::cout << "GPos " << std::flush;
			//convert hex codes to characters
				if(collapseParenthesisNibbles(&preRegA, &preRegB)){
					return 1;
				}
				//std::cout << "CPre " << std::flush;
				if(collapseParenthesisNibbles(&midRegA, &midRegB)){
					return 1;
				}
				//std::cout << "CMid " << std::flush;
				if(collapseParenthesisNibbles(&posRegA, &posRegB)){
					return 1;
				}
				//std::cout << "CPos " << std::flush;
			//run through the rules
				digestRule.getMatchingCuts(preRegB.c_str(), midRegB.c_str(), posRegB.c_str(), &winRules);
				//std::cout << "Cut ";
			//pass resline through if at least one rule passes
				if(winRules.size()){
					outFile << resline << std::endl;
				}
			//std::cout << std::endl;
		}
	resFile.close();
	regFile.close();
	outFile.close();
	return 0;
}
