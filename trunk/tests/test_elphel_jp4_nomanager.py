# Copyright (C) 2008  Florent Thiery <florent.thiery@ubicast.eu>
# Released under the terms of the LGPL

import os
os.environ["GST_DEBUG"]="3"
import gst

output_caps = "video/x-raw-yuv, format=(fourcc)I420, width=(int)1920, height=(int)1088, framerate=(fraction)15/1"

t = "rtspsrc location=rtsp://192.168.1.9:554 latency=10 ! rtpjpegdepay ! jpegdec ! videorate ! %s ! queue ! jp462bayer ! queue ! bayer2rgb2 ! queue ! ffmpegcolorspace ! xvimagesink sync=false" %(output_caps)
p = gst.parse_launch(t)
p.set_state(gst.STATE_PLAYING)
import gtk
gtk.main()
