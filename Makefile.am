SUBDIRS = librange stored python
ACLOCAL_AMFLAGS    = -I m4

valgrind: librange/librange.la 
	(cd librange; make valgrind) 
	(cd rangexxstored; make valgrind)

