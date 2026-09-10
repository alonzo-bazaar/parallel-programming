#!/usr/bin/env perl
use strict;
use warnings;
open(NP, 'numpy.csv');
open(CL, 'opencl.csv');

my @npt = map { my @s = split(/,/, $_); $s[-1] } <NP>;
my @clt = map { my @s = split(/,/, $_); $s[-1] } <CL>;
foreach (1..$#npt) {
    print $npt[$_] / $clt[$_], "\n";
}
