#!/bin/bash
bnfc -m -cpp zp.cf
make
make -f Makefile.codegen

