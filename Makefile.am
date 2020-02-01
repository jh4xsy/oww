## Process this file with automake to produce Makefile.in
## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

SUBDIRS = src pixmaps po

owwdocdir = ${prefix}/doc/oww
owwdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS\
	TODO

EXTRA_DIST = \
	$(owwdoc_DATA)\
	oww.ui \
	intltool-extract.in intltool-merge.in intltool-update.in config.rpath

DISTCLEANFILES = \
	intltool-extract intltool-merge intltool-update

install-data-local:
	@$(NORMAL_INSTALL)
	if test -f $(srcdir)/oww.ui; then \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE); \
	  $(INSTALL_DATA) $(srcdir)/oww.ui $(DESTDIR)$(pkgdatadir)/; \
	fi

uninstall-local:
	@$(NORMAL_UNINSTALL)
	if test -f $(DESTDIR)$(pkgdatadir)/oww.ui; then \
	  rm $(DESTDIR)$(pkgdatadir)/oww.ui; \
	fi


# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done


