DOCS = \
	AUTHORS				\
	COPYING.LGPLv2.1		\
	NEWS				\
	README				\
	$(NULL)

DEBIANFILES = \
	changelog.in			\
	compat				\
	control.in			\
	copyright			\
	mvt-base.install.in		\
	mvt-ffmpeg.install.in		\
	mvt-gstreamer.install.in	\
	mvt-reference.install.in	\
	rules				\
	$(NULL)

DEBIANGENFILES = \
	changelog			\
	control				\
	mvt-base.install		\
	mvt-ffmpeg.install		\
	mvt-gstreamer.install		\
	mvt-reference.install		\
	$(NULL)

EXTRA_DIST = $(DEBIANFILES)

dist_noinst_DATA = $(DEBIANGENFILES)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in $(DEBIANGENFILES)
