#ifndef PROTENGINE_ENGINE_INDEX_PRIVATE_H
#define PROTENGINE_ENGINE_INDEX_PRIVATE_H 1

/**The extension to use for indexible fasta files.*/
#define EXTENSION_FAILGZ ".failgz"
/**The extension to use for single string suffix arrays.*/
#define EXTENSION_FASSSA ".fasssa"
/**The extension to use for multi-string single-file suffix arrays.*/
#define EXTENSION_FASFSA ".fasfsa"
/**
 * This will figure out the full name of the fasta file for a reference.
 * @param ultFileName The name of the file to store the full name in.
 * @param curIndex THe index variable. IndexData*.
 * @param refName The name of the reference in question.
 */
#define GET_REFERENCE_FASTA_FILE(ultFileName,curIndex,refName) \
	char* ultFileName = (char*)malloc(strlen(curIndex->myFolder) + strlen(pathElementSep) + strlen(refName) + strlen(EXTENSION_FAILGZ) + 1);\
	strcpy(ultFileName, curIndex->myFolder);\
	strcat(ultFileName, pathElementSep);\
	strcat(ultFileName, refName);\
	strcat(ultFileName, EXTENSION_FAILGZ);

/**
 * This will figure out the full name of the single-string file for a reference.
 * @param ultFileName The name of the file to store the full name in.
 * @param curIndex THe index variable. IndexData*.
 * @param refName The name of the reference in question.
 */
#define GET_REFERENCE_SINGLESTRINGSA_FILE(ultFileName,curIndex,refName) \
	char* ultFileName = (char*)malloc(strlen(curIndex->myFolder) + strlen(pathElementSep) + strlen(refName) + strlen(EXTENSION_FASSSA) + 1);\
	strcpy(ultFileName, curIndex->myFolder);\
	strcat(ultFileName, pathElementSep);\
	strcat(ultFileName, refName);\
	strcat(ultFileName, EXTENSION_FASSSA);

/**
 * This will figure out the full name of a combo suffix array.
 * @param ultFileName The name of the file to store the full name in.
 * @param curIndex THe index variable. IndexData*.
 * @param refName The name of the combo in question.
 */
#define GET_COMBO_SUFFARR_FILE(ultFileName,curIndex,refName) \
	char* ultFileName = (char*)malloc(strlen(curIndex->myFolder) + strlen(pathElementSep) + strlen(refName) + strlen(EXTENSION_FASFSA) + 1);\
	strcpy(ultFileName, curIndex->myFolder);\
	strcat(ultFileName, pathElementSep);\
	strcat(ultFileName, refName);\
	strcat(ultFileName, EXTENSION_FASFSA);

/**Informationon a reference.*/
typedef struct{
	/**Number of sequences in this reference.*/
	uintptr_t numSeq;
	/**Whether a suffix array has been precomputed.*/
	bool haveSuffArr;
	/**Whether a merged suffix array has been precomputed.*/
	bool haveMergeSuffArr;
} ReferenceData;

/**The state of an index.*/
class IndexData{
public:
	/**Set up empty index data.*/
	IndexData();
	/**Tear down allocated memory.*/
	~IndexData();
	/**The name of this index.*/
	char* myName;
	/**The folder of this index.*/
	char* myFolder;
	/**The references in this index.*/
	std::map<std::string,ReferenceData> allReferences;
	/**All the combo files in this index.*/
	std::map< std::string,std::vector<std::string> > allComboIn;
	/**The string lengths for each combo.*/
	std::map< std::string,std::vector<uintptr_t> > allComboSS;
};

/**The state of all indices.*/
class IndexSet{
public:
	IndexSet();
	~IndexSet();
	/**All indices in the set.*/
	std::map<std::string,IndexData*> allIndices;
};


#endif