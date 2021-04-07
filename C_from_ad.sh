#!/bin/sh
#	Turn the help text file into something that C can deal with
SRC=XCalim.ad
DST=xcalim_ad.h
( 	echo 'static String fallbackResources[] = {'
   	sed -e '
	/^!/d
    /[Tt]ranslations:.*\\$/{
        :loop
        N
        s/\\$/\\/
        t loop
    }
	s/^/"/
	s/$/",/' $SRC
	echo 'NULL,
};'
) > $DST
