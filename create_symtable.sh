IN_F=./kernel.elf
OUT_F_SYM=./sym.bin
OUT_F_STR=./str.bin
SECTION_STR=.strtab
SECTION_SYM=.symtab


 
readelf -t kernel.elf |
  grep $SECTION_SYM -A1 | tail -1 |
  awk '{print "dd if='$IN_F' of='$OUT_F_SYM' bs=1 count=$[0x" $4 "] skip=$[0x" $3 "]"}' |
  bash


readelf -t kernel.elf |
  grep $SECTION_STR -A1 | tail -1 |
  awk '{print "dd if='$IN_F' of='$OUT_F_STR' bs=1 count=$[0x" $4 "] skip=$[0x" $3 "]"}' |
  bash

myfilesize1=$(wc -c ./sym.bin | awk '{print $1}')
printf "sym.bin size is %d\n" $myfilesize1

myfilesize2=$(wc -c ./str.bin | awk '{print $1}')
printf "str.bin size is %d\n" $myfilesize2
sum=$((myfilesize2 + myfilesize1))
printf "sector num %d\n" $((sum / 512 + 1))