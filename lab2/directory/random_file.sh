#!/bin/bash
filename=$(cat /dev/urandom | tr -cd 'a-f0-9' | head -c 8) 
dd if=/dev/urandom bs=1024 count=1 > $filename
