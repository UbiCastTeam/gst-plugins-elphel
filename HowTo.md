**How to use the plugins with the Elphel camera:**

gst-launch-0.10 rtspsrc location=rtsp://192.168.1.9:554 protocols=0x00000001 latency=100 ! rtpjpegdepay ! jpegdec ! queue ! jp462bayer ! queue ! bayer2rgb2 ! queue ! ffmpegcolorspace ! videorate ! "video/x-raw-yuv, format=(fourcc)I420, width=(int)1920, height=(int)1088, framerate=(fraction)25/1" ! xvimagesink sync=false max-lateness=-1 -v

**How to use the plugins with an image:**

gst-launch-0.10 filesrc location=source/fruits.jp4 ! jpegdec ! queue ! jp462bayer ! queue ! bayer2rgb2 ! queue ! ffmpegcolorspace ! freeze ! xvimagesink sync=false max-lateness=-1 -v

**How to use the plugins with a video:**

gst-launch filesrc location=yourvideo.mov ! decodebin ! queue ! jp462bayer ! queue ! bayer2rgb2 method=0 ! ffmpegcolorspace ! xvimagesink sync=false max-lateness=-1 -v