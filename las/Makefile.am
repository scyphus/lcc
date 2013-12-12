#
# Copyright (c) 2013 Scyphus Solutions Co. Ltd.  All rights reserved.
# Authors:
#      Hirochika Asai  <asai@scyphus.co.jp>
#

bin_PROGRAMS = las

las_SOURCES = las.c las.h tokenizer.c preprocessor.c assembler.c \
	parser.c parser.h \
	instr.c instr.h expr.c \
	arch/x86_64/main.c arch/x86_64/binstr.c arch/x86_64/binstr.h \
	arch/x86_64/reg.c arch/x86_64/reg.h \
	arch/x86_64/eval.c arch/x86_64/eval.h \
	checkint.c checkint.h vector.c vector.h mvector.c mvector.h \
	hashtable.c hashtable.h

CLEANFILES = *~
