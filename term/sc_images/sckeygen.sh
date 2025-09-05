#!/bin/bash

if (( $# < 2 )); then
	echo -e "Key generation script.\nUsage:\n$0 \"sizeof bytes\" sc_file.bin\n";
	exit 1;
fi
    head -c $1 /dev/random | uuencode -m - | tr -d "\n" | sed -e 's/.*644 -//' | head -c $1 > $2
