#! /bin/bash
tee </dev/ttyS3 derp.txt | while read LINE
do
	echo $LINE
	if [[ $LINE == "NEXT" ]]; then
		cscript derp.vbs
	fi
	if [[ $LINE == "PREV" ]]; then
		cscript herp.vbs
	fi
done

