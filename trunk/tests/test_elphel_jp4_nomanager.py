import os
os.environ["GST_DEBUG"]="fake*:5"
import gst
t = "rtspsrc location=rtsp://192.168.1.9:554 ! rtpjpegdepay ! jpegdec ! queue ! jp462bayer ! queue ! bayer2rgb2 ! queue ! ffmpegcolorspace ! xvimagesink sync=false"
p = gst.parse_launch(t)
p.set_state(gst.STATE_PLAYING)
import gtk
gtk.main()
