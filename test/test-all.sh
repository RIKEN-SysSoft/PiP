#!/bin/sh

for ENV in thread process:preload process:pipclone
do
export PIP_MODE=${ENV}
./test.sh
done
