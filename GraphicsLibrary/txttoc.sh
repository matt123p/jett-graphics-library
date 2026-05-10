#!/bin/sh

#  txttoc.sh
#  GenericOrdering
#
#  Created by Matt Pyne on 10/05/2012.
#  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
export PATH=$PATH:/bin
if [[ $# -ne 1 ]]; then
echo "Usage: $0 IN_FILENAME"
exit 1
fi
file=$1

if [[ ! -f "$file" ]]; then
echo "File not found: $file"
exit 1
fi

cname=`basename $1`
cname=${cname//-/_}
cname=${cname//./_}

echo "static char $cname[] = {" > $file.h
od -t x1 $file | cut -b 9- | sed -e 's/[0-9,a-f][0-9,a-f]/0x&,/g' >> $file.h
echo "0 };" >> $file.h

