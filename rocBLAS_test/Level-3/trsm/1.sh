#!/bin/bash

echo "UNRN-----------------"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=U --transA=N --sidesX=R --diagsA=N
echo "UNRU-----------------"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=U --transA=N --sidesX=R --diagsA=U
echo "UNLN-----------------"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=U --transA=N --sidesX=L --diagsA=N
echo "UNLU-----------------"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=U --transA=N --sidesX=L --diagsA=U

echo "UTRN-----------------"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=U --transA=T --sidesX=R --diagsA=N
echo "UTRU-----------------"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=U --transA=T --sidesX=R --diagsA=U
echo "UTLN-----------------"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=U --transA=T --sidesX=L --diagsA=N
echo "UTLU-----------------"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=U --transA=T --sidesX=L --diagsA=U


echo "LNRN-----------------9"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=T --transA=N --sidesX=R --diagsA=N
echo "LNRU-----------------10"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=T --transA=N --sidesX=R --diagsA=U
echo "LNLN-----------------11"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=T --transA=N --sidesX=L --diagsA=N
echo "LNLU-----------------12"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=T --transA=N --sidesX=L --diagsA=U

echo "LTRN-----------------13"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=T --transA=T --sidesX=R --diagsA=N
echo "LTRU-----------------14"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=T --transA=T --sidesX=R --diagsA=U
echo "LTLN-----------------15"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=T --transA=T --sidesX=L --diagsA=N
echo "LTLU-----------------16"
./trsm -M 25000 -N 25000 -K 25000 --fillsA=T --transA=T --sidesX=L --diagsA=U


























