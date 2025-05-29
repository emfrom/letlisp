AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -I$(srcdir)/src -Isrc \
	$(READLINE_CFLAGS) \
	$(GC_CFLAGS) \
	$(GMP_CFLAGS) \
	-msse4.2

bin_PROGRAMS = letlisp
letlisp_SOURCES = \
	src/main.c \
	src/repl.c \
	src/parser.c \
	src/builtin.c \
	src/memory.c \
	src/env.c \
	src/hashtable.c \
	src/value.c


letlisp_LDADD = \
	$(READLINE_LIBS) \
	$(GC_LIBS) \
	$(GMP_LIBS)
