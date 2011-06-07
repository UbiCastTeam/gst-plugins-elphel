 # plugindir is set in configure

##############################################################################
# change libgstplugin.la to something more suitable, e.g. libmysomething.la  #
##############################################################################
plugin_LTLIBRARIES = libgstbayer2rgb2.la
ORC_SOURCE=gstbayerorc
include $(top_srcdir)/orc.mak

##############################################################################
# for the next set of variables, rename the prefix if you renamed the .la,   #
#  e.g. libgstplugin_la_SOURCES => libmysomething_la_SOURCES                 #
#       libgstplugin_la_CFLAGS  => libmysomething_la_CFLAGS                  #
#       libgstplugin_la_LIBADD  => libmysomething_la_LIBADD                  #
#       libgstplugin_la_LDFLAGS => libmysomething_la_LDFLAGS                 #
##############################################################################

# sources used to compile this plug-in
libgstbayer2rgb2_la_SOURCES =	gstbayer2rgb2.c	\
				bayer.c

# flags used to compile this plugin
# add other _CFLAGS and _LIBS as needed
libgstbayer2rgb2_la_CFLAGS =	$(GST_CFLAGS)   \
				$(ORC_CFLAGS)   \
				$(GST_BASE_CFLAGS) 	\
				$(GSTPB_BASE_CFLAGS)

libgstbayer2rgb2_la_LIBADD =	$(GST_LIBS)		\
		$(ORC_LIBS)\
				$(GST_BASE_LIBS)	\
				$(GSTPB_BASE_LIBS)

libgstbayer2rgb2_la_LDFLAGS =	$(GST_PLUGIN_LDFLAGS)
libgstbayer2rgb2_la_LIBTOOLFLAGS = --tag=disable-static
nodist_libgstbayer2rgb2_la_SOURCES = $(ORC_NODIST_SOURCES)
# headers we need but don't want installed
noinst_HEADERS = gstbayer2rgb2.h
