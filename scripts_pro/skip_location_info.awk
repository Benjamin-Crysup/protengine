BEGIN{
	foundInI = -1
	foundAtI = -1
	foundCntI = -1
	foundRLI = -1
	infMissI = -1
	nonCodI = -1
	cigProI = -1
}
{
	if(NR == 1){
        anyPrin = 0
		for(i = 1; i<=NF; i++){
			if($i~"info_missing"){
				infMissI = i
                continue
			}
			else if($i~"ref_non_codon"){
				nonCodI = i
                continue
			}
			else if($i~"ref_cigar_mismatch"){
				cigProI = i
                continue
			}
			else if($i~"found_in"){
				foundInI = i
                continue
			}
			else if($i~"found_at"){
				foundAtI = i
                continue
			}
			else if($i~"found_count"){
				foundCntI = i
                continue
			}
			else if($i~"found_refloc"){
				foundRLI = i
                continue
			}
			if(anyPrin != 0){ printf "\t"; }
			anyPrin = 1
			printf "%s", $i
		}
		printf "\n"
	}
	else{
		anyPrin = 0
		for(i = 1; i<=NF; i++){
			if(i == foundInI){ continue; }
			if(i == foundAtI){ continue; }
			if(i == foundCntI){ continue; }
			if(i == foundRLI){ continue; }
			if(i == infMissI){ continue; }
			if(i == nonCodI){ continue; }
			if(i == cigProI){ continue; }
			if(anyPrin != 0){ printf "\t"; }
			anyPrin = 1
			printf "%s", $i
		}
		printf "\n"
	}
}
