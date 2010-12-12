SUBDIRS = po src

ACLOCAL_AMFLAGS = -I m4
INTLTOOL_FILES = intltool-extract.in \
                 intltool-merge.in \
                 intltool-update.in

EXTRA_DIST = $(INTLTOOL_FILES)

DISTCLEANFILES = intltool-extract \
                 intltool-merge \
                 intltool-update \
                 po/.intltool-merge-cache
