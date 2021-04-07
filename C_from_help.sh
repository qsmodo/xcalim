#!/bin/sh
#	Turn the help text file into something that C can deal with
SRC=XCalim.help
DST=xcalim_help.h
( 	echo 'char helpdata[] = "\'
   	sed -e 's/$/\\n\\/' $SRC
	echo '";'
) > $DST
