#!/bin/bash
./build.sh
while true; do
    bin/autoref
    sleep .2
done
