#!/usr/bin/env python
from __future__ import print_function
import argparse
import sys
from autocurator import csv2xml

parser = argparse.ArgumentParser("autocurator",
                                 formatter_class=argparse.ArgumentDefaultsHelpFormatter)

parser.add_argument(
    "-i",
    "--input",
    help="input csv file from autocurtor",
    required=True)

parser.add_argument(
    "-o",
    "--output",
    help="name of output file",
    required=True)

args = parser.parse_args()
out_file = args.output
in_file = args.input

with open(out_file, "w") as f:
    print('<?xml version="1.0"?>', file=f)
    print('<!DOCTYPE dataset SYSTEM "http://www-pcmdi.llnl.gov/software/cdms/cdml.dtd">', file=f)
    print(csv2xml(in_file), file=f)
