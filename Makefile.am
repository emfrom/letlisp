AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -I$(srcdir)/src -Isrc $(READLINE_CFLAGS)

bin_PROGRAMS = minilisp
minilisp_SOURCES = \
	src/main.c \
	src/repl.c


minilisp_LDADD = $(READLINE_LIBS)
