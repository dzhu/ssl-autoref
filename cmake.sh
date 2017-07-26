#!/bin/bash

rm -rf build
mkdir build
cd build
exec cmake ..
