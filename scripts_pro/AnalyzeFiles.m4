
ifdef(`WIN32',`',`export LC_ALL=C')
define(`LIT_QUOTE',changequote([,])[changequote([,])'changequote(`,')]changequote(`,'))
define(`QUOTE',`LIT_QUOTE()`'$1`'LIT_QUOTE()')
ifdef(`WIN32',`define(`QUOTE',`"`'$1`'"')')
ifdef(`WIN32',`define(`PYTHON',`python')',`define(`PYTHON',`python')')

ifdef(`INPUT_FILES',`include(INPUT_FILES)')
ifdef(`OUTPUT_FILES',`include(OUTPUT_FILES)')
ifdef(`SMALLPUT_FILES',`include(SMALLPUT_FILES)')

dnl need a pebat build m4 script
dnl need NUM_FILE, SCRIPT_FOLDER, PROGRAM_FOLDER, REFERENCE_FOLDER, HAP_COUNT, DIGEST_FILE, REFNAME_NAME

define(`HEAD_TSV',`ifelse(eval($1<=NUM_FILE),0,`',`head -n 1 sa`'eval($1,10,3)`'.fa.tryp > sort`'eval($1,10,3)`'.tsv
tail -n +2 sa`'eval($1,10,3)`'.fa.tryp | sort -u -k1,1 >> sort`'eval($1,10,3)`'.tsv
HEAD_TSV(eval($1 + 1))')')
HEAD_TSV(1)

define(`ENUM_KNOWN',`ifelse(eval($1 < $2),0,`',`-kno=novel`'eval($1,10,3)`'.tsv ENUM_KNOWN(eval($1 + 1),$2)')')
define(`MAKE_UNIQUE',`ifelse(eval($1<=NUM_FILE),0,`',`PYTHON SCRIPT_FOLDER/LimitTSVToUnseen.py ENUM_KNOWN(1,$1) -unk=sort`'eval($1,10,3)`'.tsv -out=novel`'eval($1,10,3)`'.tsv
MAKE_UNIQUE(eval($1 + 1))')')
MAKE_UNIQUE(1)

define(`MAKE_FASTA',`ifelse(eval($1<=NUM_FILE),0,`',`awk QUOTE(`{print `$'1}') novel`'eval($1,10,3)`'.tsv | tail -n +2 | tr I L | sed QUOTE(`/.*/i >tofind') > fast`'eval($1,10,3)`'.fa
MAKE_FASTA(eval($1 + 1))')')
MAKE_FASTA(1)

PROGRAM_FOLDER/Protengine < DoSearch.pebat | tee search.out

define(`DIG_FILTER',`ifelse(eval($1<=NUM_FILE),0,`',`dnl
PROGRAM_FOLDER/FilterSearchCutValidity -res=rawfind`'eval($1,10,3)`'.txt -reg=region`'eval($1,10,3)`'.txt -dig=DIGEST_FILE -out=filtfind`'eval($1,10,3)`'.txt
DIG_FILTER(eval($1 + 1))')')
DIG_FILTER(1)

define(`ANNOTATE_RES',`ifelse(eval($1<=NUM_FILE),0,`',`dnl
PROGRAM_FOLDER/LimitSearchToUnique -inT=REFERENCE_FOLDER/table.bin -inD=REFERENCE_FOLDER/data.bin -rfN=REFERENCE_FOLDER/REFNAME_NAME -dig=filtfind`'eval($1,10,3)`'.txt -ori=fast`'eval($1,10,3)`'.fa -out=markup`'eval($1,10,3)`'.txt
ANNOTATE_RES(eval($1 + 1))')')
ANNOTATE_RES(1)

define(`MERGE_RES',`ifelse(eval($1<=NUM_FILE),0,`',`dnl
PROGRAM_FOLDER/MergeResultsInTSV -inU=markup`'eval($1,10,3)`'.txt -tsv=novel`'eval($1,10,3)`'.tsv -ori=fast`'eval($1,10,3)`'.fa -out=uninov`'eval($1,10,3)`'.tsv -hap=HAP_COUNT
MERGE_RES(eval($1 + 1))')')
MERGE_RES(1)

define(`ENUM_FOUND',`ifelse(eval($1 <= $2),0,`',`-kno=uninov`'eval($1,10,3)`'.tsv ENUM_FOUND(eval($1 + 1),$2)')')
define(`FOLD_RES',`ifelse(eval($1<=NUM_FILE),0,`',`dnl
PYTHON SCRIPT_FOLDER/FoldInUnseen.py ENUM_FOUND(1,$1) -unk=sort`'eval($1,10,3)`'.tsv -out=unimark`'eval($1,10,3)`'.tsv
FOLD_RES(eval($1 + 1))')')
FOLD_RES(1)

define(`MAKE_SMALLER',`ifelse(eval($1<=NUM_FILE),0,`',`dnl
cat unimark`'eval($1,10,3)`'.tsv | awk -f SCRIPT_FOLDER/skip_location_info.awk > smallmark`'eval($1,10,3)`'.tsv
MAKE_SMALLER(eval($1 + 1))')')
MAKE_SMALLER(1)

define(`GZIP_BIG',`ifelse(eval($1<=NUM_FILE),0,`',`gzip unimark`'eval($1,10,3)`'.tsv
GZIP_BIG(eval($1 + 1))')')
define(`GZIP_SMALL',`ifelse(eval($1<=NUM_FILE),0,`',`gzip unimark`'eval($1,10,3)`'.tsv
GZIP_SMALL(eval($1 + 1))')')


tar -cvzf temps.tar.gz fast*.fa filtfind*.txt markup*.txt novel*.tsv rawfind*.txt region*.txt sort*.tsv uninov*.tsv
rm fast*.fa filtfind*.txt markup*.txt novel*.tsv rawfind*.txt region*.txt sort*.tsv uninov*.tsv
GZIP_BIG(1)
GZIP_SMALL(1)

