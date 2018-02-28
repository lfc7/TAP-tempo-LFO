#! /bin/bash

# bash script to help generate numbers for a waveForm looking-Up table
# output on stdout

# the function "round()" was taken from 
# http://stempell.com/2009/08/rechnen-in-bash/

# the round function:
round()
{
echo $(printf %.$2f $(echo "scale=$2;(((10^$2)*$1)+0.5)/(10^$2)" | bc))
};

#strip first n char from string
stringStripNCharsFromStart() {
    echo "${1:$2:${#1}}"
};

usage()
{
	echo
	echo "$0 generate looking up table for some waveForms"
	echo
	echo "USAGE: $0 X Y "
	echo "where X is the number of steps (default = 48)"
	echo "where Y is output resolution (default = 255)"
	echo

}

[[ ! -n $1 ]] && usage

XMAX=$1
YMAX=$2

[[ -z $XMAX ]] && XMAX=48 && YMAX=255
[[ -z $YMAX ]] && YMAX=255

XMAX=$((XMAX - 1))

echo
echo "Ramp in $((XMAX + 1)) steps:"
for x in $(seq 0 $XMAX);
do
y=$((x * YMAX / XMAX))
ramp="${ramp},$y"
done
ramp=$(stringStripNCharsFromStart ${ramp} 1 )
echo ${ramp}

echo
echo "invRamp in $((XMAX + 1)) steps:"
for x in $(seq 0 $XMAX);
do
y=$(((XMAX - x) * YMAX / XMAX))
invramp="${invramp},$y"
done
invramp=$(stringStripNCharsFromStart ${invramp} 1 )
echo ${invramp}

echo
echo "triangle in $((XMAX + 1)) steps:"
for x in $(seq 0 $((XMAX / 2)));
do
y=$((x * YMAX * 2 / XMAX))
triangle="${triangle},$y"
done
for x in $(seq $(( (XMAX / 2) + 1 )) $XMAX);
do
y=$(((XMAX - x) * YMAX * 2 / XMAX))
triangle="${triangle},$y"
done
triangle=$(stringStripNCharsFromStart ${triangle} 1 )
echo ${triangle}

echo
echo "rect in $((XMAX + 1)) steps:"
for x in $(seq 0 $((XMAX / 2)));
do
y=$YMAX
rect="${rect},$y"
done
for x in $(seq $(( (XMAX / 2) + 1 )) $XMAX);
do
y=0
rect="${rect},$y"
done
rect=$(stringStripNCharsFromStart ${rect} 1 )
echo ${rect}

echo
echo "invRect in $((XMAX + 1)) steps:"
for x in $(seq 0 $((XMAX / 2)));
do
y=0
invrect="${invrect},$y"
done
for x in $(seq $(( (XMAX / 2) + 1 )) $XMAX);
do
y=$YMAX
invrect="${invrect},$y"
done
invrect=$(stringStripNCharsFromStart ${invrect} 1 )
echo ${invrect}

echo
echo "Sine in $((XMAX + 1)) steps:"
for x in $(seq 0 $XMAX);
do
div=$(echo "scale=10; 3.1415 * 2 * $x / $XMAX" | bc -l)
#y=$( echo "scale=10; s($div - 1.5707)* ($YMAX / 2) + ($YMAX / 2)" | bc -l | cut -d '.' -f 1)
y=$( echo "scale=10; s($div - 1.5707)* ($YMAX / 2) + ($YMAX / 2)" | bc -l )
y=$( round ${y} 0)
#y=$( echo "scale=0; $sin * 1.0" | bc )
sine="${sine},$y"
done
sine=$(stringStripNCharsFromStart ${sine} 1 )
echo ${sine}

echo
echo "Sine2 in $((XMAX + 1)) steps:"
for x in $(seq 0 $XMAX);
do
div=$(echo "scale=10; 3.1415 * 2 * $x / $XMAX" | bc -l)
#y=$( echo "scale=10; (s($div + 1.5707)* ($YMAX / 2)) + ($YMAX / 2)" | bc -l | cut -d '.' -f 1)
y=$( echo "scale=10; (s($div + 1.5707)* ($YMAX / 2)) + ($YMAX / 2)" | bc -l )
y=$( round ${y} 0)
#y=$( echo "scale=0; $sin * 1.0" | bc )
sine2="${sine2},$y"
done
sine2=$(stringStripNCharsFromStart ${sine2} 1 )
echo ${sine2}

echo
echo "all done exiting... bye"
exit 0
