#! /bin/bash
# bash script to generate numbers for the waveForm looking-Up table
# output on stdout
#

#resolutions
XMAX=47
YMAX=255

## ok go !

echo "Ramp"
for x in $(seq 0 $XMAX);
do
y=$((x * YMAX / XMAX))
ramp="${ramp},$y"
done
echo ${ramp}

echo "invRamp"
for x in $(seq 0 $XMAX);
do
y=$(((XMAX - x) * YMAX / XMAX))
invramp="${invramp},$y"
done
echo ${invramp}

echo "triangle"
for x in $(seq 0 $((XMAX / 2)));
do
y=$((x * YMAX * 2 / XMAX))
triangle="${triangle},$y"
done
for x in $(seq $((XMAX / 2)) $XMAX);
do
y=$(((XMAX - x) * YMAX * 2 / XMAX))
triangle="${triangle},$y"
done
echo ${triangle}

echo "rect"
for x in $(seq 0 $((XMAX / 2)));
do
y=$YMAX
rect="${rect},$y"
done
for x in $(seq $((XMAX / 2)) $XMAX);
do
y=0
rect="${rect},$y"
done
echo ${rect}

echo "Sine"
for x in $(seq 0 $XMAX);
do
div=$(echo "scale=10; 3.1415 * 2 * $x / $XMAX" | bc -l)
y=$( echo "scale=10; s($div - 1.5707)* ($YMAX / 2) + ($YMAX / 2)" | bc -l | cut -d '.' -f 1)
#y=$( echo "scale=0; $sin * 1.0" | bc )
sine="${sine},$y"
done
echo ${sine}

echo "Sine2"
for x in $(seq 0 $XMAX);
do
div=$(echo "scale=10; 3.1415 * 2 * $x / $XMAX" | bc -l)
y=$( echo "scale=10; (s($div + 1.5707)* ($YMAX / 2)) + ($YMAX / 2)" | bc -l | cut -d '.' -f 1)
#y=$( echo "scale=0; $sin * 1.0" | bc )
sine2="${sine2},$y"
done
echo ${sine2}
