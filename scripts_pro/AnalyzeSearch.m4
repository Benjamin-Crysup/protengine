MaxRam 1000000000
Thread 8
IndexInit ind REFERENCE_FOLDER/ind
IndexReference ind e25
BuildSAPrecom ind e25
BuildComboPrecom ind ce25 e25
define(`Run_One',`ifelse(eval($1 <= NUM_RUNS),0,`',`FindComboPrecom ind ce25 fast`'eval($1,10,3)`'.fa rawfind`'eval($1,10,3)`'.txt
IndexDumpSearchRegion fast`'eval($1,10,3)`'.fa rawfind`'eval($1,10,3)`'.txt 2 2 region`'eval($1,10,3)`'.txt
Run_One(eval($1 + 1))')')Run_One(1)
