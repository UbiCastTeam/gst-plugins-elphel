# Requirements #

Tested on Ubuntu 10.04 / gstreamer-0.10.34

To compile, additionally to the basic development tools (from build-essential, automake, autoconf, libtool), you will need (from apt on Ubuntu/Debian systems):
  * libgstreamer-plugins-base0.10-dev
  * libgstreamer0.10-dev
  * liborc-dev (>= 0.4)

# Get the source #

svn checkout http://gst-plugins-elphel.googlecode.com/svn/trunk/ gst-plugins-elphel

# Compilation #
## jp462bayer ##

From your gst-plugins-elphel directory:
  * cd jp462bayer
  * ./autogen.sh
  * make
  * cp src/.libs/libgstjp462bayer.so ~/.gstreamer-0.10/plugins/

## bayer2rgb2 ##

From your gst-plugins-elphel directory:
  * cd bayer2rgb2
  * With edgesence method (disabled by default: **patent-protected in the U.S.**) :
> > ./configure --enable-orc --enable-edgesence
  * Without edgesence method :
> > ./configure --enable-orc
  * ./autogen.sh
  * make
  * cp /src/.libs/libgstbayer2rgb2.so ~/.gstreamer-0.10/plugins/

# Checking #

Now you can use gst-inspect to verify the plugins have been built and installed successfully:

  * gst-inspect bayer2rgb2
  * gst-inspect jp462bayer


If gst-inspect doesn't find the new plugins delete the plugin registry at

` ~/.gstreamer-0.10/registry.* `

It will be recreated with updated index when doing:
  * gst-inspect bayer2rgb2
  * gst-inspect jp462bayer