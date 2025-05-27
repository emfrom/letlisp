AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -I$(srcdir)/src -Isrc \
	$(READLINE_CFLAGS) \
	$(GC_CFLAGS) \
	$(GMP_CFLAGS)

bin_PROGRAMS = minilisp
minilisp_SOURCES = \
	src/main.c \
	src/repl.c \
	src/parser.c \
	src/builtin.c \
	src/memory.c \
	src/env.c


minilisp_LDADD = \
	$(READLINE_LIBS) \
	$(GC_LIBS) \
	$(GMP_LIBS)
