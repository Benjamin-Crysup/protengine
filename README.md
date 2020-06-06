## Building the Program

To make use of these programs, you will need g++, gcc, zlib, m4, head, tail, sort, tar and gzip.
Additionally, you will need Python 3.6+.
Most of these are installed by default on \*nix systems, Windows users will need to install msys (and add the binary folder to their path).

Once all requirements are installed, you can build the program by running the appropriate shell script/batch file.
The programs will be in builds: look for the appropriate bin folder.

An example of how to build an search is also inside the bin folder (example).
This README is \*nix focused, though many of the commands remain the same on Windows (provided msys is installed).
The biggest issue will be making sure the correct utilities are used (Windows provides an incompatible version of sort, and python is invoked without a 3).

## Preparing the Reference

An example of preparing a reference suffix array and extra data is in the example folder.
This section is an explanation of the commands in buildref\_example.sh.

To prepare a reference, you will need gff3 files specifying the genomic locations of each protein, a fasta file listing the protein sequences, and a lookup table of which individuals have which protein variants.
See example/reference\_files/testlocations.gff3, testrefraw.fa and testpeople.tsv (respectively) for examples.

First, the gff3 files will need to be converted to a sorted bed file.
If you have all of the information in one gff3 file, the commands are:

```
python3 ../GFF3ToBed.py -in=reference\_files/testlocations.gff3 > reference\_files/allprotloc.bed
sort -k 4 reference\_files/allprotloc.bed > reference\_files/allprotsort.bed
```

If you have the information in multiple gff3 files, you can run the GFF3ToBed python script for all of them, appending to the same file.
This can be done easily with shell scripting utilities.
For example, in bash:

```
for v in reference\_files/*.gff3; do python3 ../GFF3ToBed.py -in=$v >> reference\_files/allprotloc.bed; done
sort -k 4 reference\_files/allprotloc.bed > reference\_files/allprotsort.bed
```

and in cmd.exe

```
FOR %%v IN (reference\_files/*.gff3) DO python3 ..\\GFF3ToBed.py -in=%%v >> reference\_files\\allprotloc.bed
sort -k 4 reference\_files/allprotloc.bed > reference\_files/allprotsort.bed
```

For mass-spec proteomic applications, the next step is to convert isoleucines to leucines.

```
cat reference\_files/testrefsraw.fa | tr I L > reference\_files/testrefs.fa
```

With the fasta, a suffix array can be built using Protengine.
Protengine expects a series of commands to come over its standard input, so the lines

```
cd reference
mkdir ind
../../Protengine < buildref.pebat
cd ..
```

prepare the storage for the suffix array, start up Protengine, and feed in commands from a file.
The commands in the example are:

```
MaxRam 1000000000
Thread 8
IndexInit ind ind
IndexReference ind e25 ../reference\_files/testrefs.fa
BuildComboPrecom ind ce25 e25
IndexDumpComboNames ind ce25 proteome\_names
```

MaxRam and Thread alter the amount of used ram and threads.
IndexInit tells Protengine where to store the created files for this "project".
IndexReference adds a fasta to the project.
BuildComboPrecom builds a full suffix array.
IndexDumpComboNames writes out the names of each protein variant in order.
More information about Protengine commands can be had using the Help command.

If you have your exome data split into multiple fasta files, you will need to do the isoleucine conversion for all of them.
The simplest way to do this is to dump all the converted files into one large file.

```
cat reference\_files/*.fa | tr I L > reference\_files/testrefs.fa
```

With this, basic searches against the suffix array can be performed.
However, to interpret the results, information about individuals with each variant and the locations of the proteins in the genome would be helpful.
The next step is to package up that information in a relatively quick format.

```
python3 ../LUTBedMerge.py -inB=reference\_files/allprotsort.bed -inL=reference\_files/testpeople.tsv -outT=reference/table.bin -outD=reference/data.bin
```

This command takes the location information (allprotsort.bed) and the population information (testpeople.tsv) and produces a raw dump of data (data.bin) and an index into that data (table.bin).
Building suffix arrays can take a while (days to a week for 1000 genomes, depending on ram).
Once you've built a suffix array, you might want to cache it for later.

## Single Individual Search

If you have a single trypsin digest file and you want extra information on the peptides in that file (which proteins they are found in, their frequency), there are a suite of programs that can be used.
This section covers the searchref\_example.sh script.

Protengine performs searches using a fasta as input, and for speed MergeResultsInTSV requires that the peptides are sorted.
The first step is to get a fasta from the digest file (removing duplicates to speed up the search).
The first four commands accomplish this.

```
export LC\_ALL=C
head -n 1 lookie.fa.tryp > lookieS.tsv
tail -n +2 lookie.fa.tryp | sort -u -k1,1 >> lookieS.tsv
awk '{print $1}' lookieS.tsv | tail -n +2 | tr I L | sed '/.*/i >tofind' > lookie.fa
```

Changing the locale (LC\_ALL) makes sort use ascii ordering, rather than whatever the default is.
The next command passes the header through.
The third command sorts the actual entries of the digest.
The forth monstrosity extracts the peptide sequences, changes isoleucines to leucines, and formats as a fasta.

The next step is the actual suffix array search.

```
../../Protengine < DoSearch.pebat | tee search.out
```

The contents of DoSearch.pebat look similar to those when building the suffix array: Protengine needs to know where the suffix array is and what's in it.

```
MaxRam 1000000000
Thread 8
IndexInit ind ../reference/ind
IndexReference ind e25
BuildComboPrecom ind ce25 e25
FindComboPrecom ind ce25 lookie.fa rawfind.txt
IndexDumpSearchRegion lookie.fa rawfind.txt 2 2 region.txt
```

The last two lines are of interest.
FindComboPrecom will open a fasta, search through a suffix array for those entries, and put the results in a text file (rawfind.txt).
IndexDumpSearchRegion will take a search (fasta and result) and dump out whe bases of the match, along with neighboring amino acids (two before and two after in this case).

We need the amino acid sequence because not all peptide matches could actually be produced by a trypsin digest: CDE is in ACDEFG, but will never result from it.
Matches need to be filtered on whether they could actually be produced.

```
../../FilterSearchCutValidity -res=rawfind.txt -reg=region.txt -dig=../other\_files/trypsin\_only.dig -out=filtfind.txt
```

The format used to specify what is and is not valid is described at the end of this document.

The next step is to add in location and frequency data.

```
../../LimitSearchToUnique -inT=../reference/table.bin -inD=../reference/data.bin -rfN=../reference/proteome\_names -dig=filtfind.txt -ori=lookie.fa -out=markup.txt
```

And the final step is to translate the results back to a tsv format.

```
../../MergeResultsInTSV -inU=markup.txt -tsv=lookieS.tsv -ori=lookie.fa -out=foundie.tsv -hap=10
```

Hap is the number of haplotypes in the original population: 5008 for 1000 genomes, 10 in the example.

## Multiple Individual Search

WILL REVAMP

If there are multiple trypsin digests to add data to, running each individually can become time consuming.
A few utilities are provided so that, when a peptide is seen in one digest, the search is only performed once.

These utilities can become cumbersom, so some utility m4 scripts have been provided.
They expect that the original digest files are named as "sa###.fa.tryp" (with three digits, numbers start at 001).
In the folder with the files, run the following commands (change data as necessary):

```
m4 -D NUM\_RUNS=25 -D REFERENCE\_FOLDER=../PathPath/1000GenomeFolder ../PathPath/AnalyzeSearch.m4 > DoSearch.pebat
m4 -D WIN32 -D NUM\_FILE=25 -D HAP\_COUNT=5008 -D SCRIPT\_FOLDER=../PathPath/bin\_x64\_win32 -D PROGRAM\_FOLDER=../PathPath/bin\_x64\_win32 -D REFERENCE\_FOLDER=../PathPath/1000GenomeFolder -D REFNAME\_NAME=1KGenome\_names -D DIGEST\_FILE=../PathPath/trypsin\_only.dig ../PathPath/AnalyzeFiles.m4 > DoAnalysis.bat
DoAnalysis.bat
```

NUM\_RUNS and NUM\_FILES are the number of individuals you will be searching for.
REFERENCE\_FOLDER is the path to the already built reference.
On Windows, you will need to define WIN32 (-D WIN32).
HAP\_COUNT is the number of haplotypes in the reference.
SCRIPT\_FOLDER and PROGRAM\_FOLDER are the locations of the various programs (probably bin\_?\_?, though you are free to move these files).
REFERENCE\_NAME is a file (in the reference folder) with a list of protein names in the reference.
DIGEST\_FILE is a path to a file describing the valid patterns for a given digest enzyme.

## Digest Specification Files

You can describe what constitutes a valid cut by your enzyme(s) by using pattern(s) on individual lines of a text file.
The pattern is broken up into four regions: what must precede the start of a valid cut product, what must be at the start of such a product, what must be at the end, and what must follow.

```
precede | start - end | follow
```

In the above pattern, the pipes represent the cuts, and the dash represents the internal (and irrelevant) portion of the peptide.

Each piece describes some number of characters.
Literal characters can be described as is (escaped with a backslash), or they can be described as a hexadecimal byte.
A dot can be used for any character, and a character set (any one of these is valid) can be specified using brackets.

If there can be nothing before the precede portion (i.e. the peptide had to come from the start of the protein), add a carat (^) to the beginning of the pattern.
If there can be nothing after the follow portion (peptide comes from the end), add a dollar sign to the end (otherwise, add an exclamation point).

An example of this sort of specification (for trypsin) follows.

```
^|-|$
^\\M|-|$
^|-[\\K\\R]!
^\\M|-[\\K\\R]!
[\\K\\R]|-|$
[\\K\\R]|-|[\\K\\R]!
```

If the peptide contains the beginning and end of the protein, it is a valid product of a trypsin digest.
If the peptide is only preceded by a methionine, that methionine is probably the start codon, so the peptide contains the start.
If the peptide contains the beginning, and ends in a K or R, it is valid.
Same story with methionine as before.
If the peptide is preceded by a K or R, and contains the end, it is valid.
If the peptide is preceded by and ends with either K or R, it is valid.


