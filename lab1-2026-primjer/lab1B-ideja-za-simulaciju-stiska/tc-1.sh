#!/bin/bash

echo 0 > tipka.txt

echo "Za 10 s stiscem tipku na 0.5 s"
sleep 10
echo "Stiscem tipku"
echo 1 > tipka.txt
sleep 0.5
echo "Otpustam tipku"
echo 0 > tipka.txt
