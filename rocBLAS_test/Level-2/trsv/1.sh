#!/bin/bash

echo "UNN-----------------1"
./trsv -M 20480 -N 20480 -K 20480 --fillsA=U --transA=N --diagsA=N
echo "UNU-----------------2"
./trsv -M 20480 -N 20480 -K 20480 --fillsA=U --transA=N --diagsA=U

echo "UTN-----------------3"
./trsv -M 20480 -N 20480 -K 20480 --fillsA=U --transA=T --diagsA=N
echo "UTU-----------------4"
./trsv -M 20480 -N 20480 -K 20480 --fillsA=U --transA=T --diagsA=U

echo "LNN-----------------5"
./trsv -M 20480 -N 20480 -K 20480 --fillsA=L --transA=N --diagsA=N
echo "LNU-----------------6"
./trsv -M 20480 -N 20480 -K 20480 --fillsA=L --transA=N --diagsA=U

echo "LTN-----------------7"
./trsv -M 20480 -N 20480 -K 20480 --fillsA=L --transA=T --diagsA=N
echo "LTU-----------------8"
./trsv -M 20480 -N 20480 -K 20480 --fillsA=L --transA=T --diagsA=U


























