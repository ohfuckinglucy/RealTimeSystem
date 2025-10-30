#!/bin/bash

echo "Starting background noise generation... (PID: $$)"
echo "Press Ctrl+C in this terminal to stop."

while true; do
    find / -type f > /dev/null 2>&1 &
    
    dd if=/dev/urandom bs=1M count=10 | md5sum > /dev/null 2>&1 &

    ( head -c 100M /dev/zero | tr '\0' '\1' > /dev/null ) &

    sleep 0.5
done
