#!/bin/bash

echo "UNRN-----------------1"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=U --transA=N --sidesX=R --diagsA=N
echo "UNRU-----------------2"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=U --transA=N --sidesX=R --diagsA=U
echo "UNLN-----------------3"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=U --transA=N --sidesX=L --diagsA=N
echo "UNLU-----------------4"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=U --transA=N --sidesX=L --diagsA=U

echo "UTRN-----------------5"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=U --transA=T --sidesX=R --diagsA=N
echo "UTRU-----------------6"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=U --transA=T --sidesX=R --diagsA=U
echo "UTLN-----------------7"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=U --transA=T --sidesX=L --diagsA=N
echo "UTLU-----------------8"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=U --transA=T --sidesX=L --diagsA=U


echo "LNRN-----------------9"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=T --transA=N --sidesX=R --diagsA=N
echo "LNRU-----------------10"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=T --transA=N --sidesX=R --diagsA=U
echo "LNLN-----------------11"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=T --transA=N --sidesX=L --diagsA=N
echo "LNLU-----------------12"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=T --transA=N --sidesX=L --diagsA=U

echo "LTRN-----------------13"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=T --transA=T --sidesX=R --diagsA=N
echo "LTRU-----------------14"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=T --transA=T --sidesX=R --diagsA=U
echo "LTLN-----------------15"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=T --transA=T --sidesX=L --diagsA=N
echo "LTLU-----------------16"
./trsm_d -M 4096 -N 4096 -K 4096 --fillsA=T --transA=T --sidesX=L --diagsA=U


























