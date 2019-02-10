#! /bin/bash

if [ $# -ne 5 ]; then
  echo "Usage : ssed.sh stelashot.csv ss[-13:6] ii[0:9] a r" 1>&2
  exit 1
fi

csv=$1
ss=$2
ii=$3
a=$4
r=$5

#ssed.sh t.csv sss iii aaa repeat
declare -A sss;
for s in `seq -13 1 -7`; do sss[$s]=\"1/$(( 8000 / 2 ** (13 + $s) )).0\"; done
for s in `seq -6 1 -4`; do sss[$s]=\"1/$(( 60 / 2 ** (6 + $s) )).0\"; done
for s in `seq -3 1 -1`; do sss[$s]=\"1/$(( 2 ** (-$s) )).0\"; done
#1s超えると+2.1sしないと撮影画像の露出時間が設定したい時間にならない
for s in `seq 0 1 3`; do sss[$s]=$(( 2 ** ($s) + 2 )).10; done
for s in `seq 4 1 6`; do sss[$s]=$(( 15 * 2 ** ($s - 4) + 2)).10; done

#for ss in ${!sss[@]}; do echo $ss ${sss[$ss]}; done

declare -A isos;
for i in `seq 0 1 9`; do isos[$i]=$(( 100 * 2 ** $i )); done

#for iso in ${!isos[@]}; do echo $iso ${isos[$iso]}; done

grep SequenceCaption ${csv}
#無視されるっぽいけど
for i in ${ii}; do
for s in ${ss}; do
#echo $s $i
grep StartJd ${csv}
grep ExposureTime ${csv} | sed s#sss#${sss[$s]}# | sed s/iii/${isos[$i]}/ | sed s/aaa/$a/ | sed s/rrr/$r/ 

done
done
