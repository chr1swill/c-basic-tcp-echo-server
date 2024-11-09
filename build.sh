#!/bin/sh

outdir="bin"
cflags="-Wall -Wextra -ggdb"

gcc -o ${outdir}/server server.c $cflags
#gcc -o ${outdir}/client client.c $cflags
