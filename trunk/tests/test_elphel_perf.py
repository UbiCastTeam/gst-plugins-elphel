#!/bin/env/python
# -*- coding: utf-8 -*
# Requires http://code.google.com/p/gstmanager/

from gstmanager.event import EventListener 
import sys

class StatsGetter(object):
    def __init__(self, framerate):
        self.framerate = framerate
        self.last_dup = 0
        self.last_drop = 0

    def get_numbers(self, pipelinel):
        if pipelinel.get_state() == "GST_STATE_PLAYING":
            drop = pipelinel.get_property_on_element(element_name="videorate", property_name="drop")
            duplicate = pipelinel.get_property_on_element(element_name="videorate", property_name="duplicate")

            dup_diff = duplicate - self.last_dup 
            drop_diff = drop - self.last_drop 
            real_fps = self.framerate - dup_diff + drop_diff

            if real_fps < 0:
                real_fps = "less than real time"
            s = "\rDropped frames: %s Duplicated frames: %s, real fps: %s                  " %(drop, duplicate, real_fps)
            sys.stdout.write(s)
            sys.stdout.flush()
            self.last_dup = duplicate
            self.last_drop = drop

        return True

if __name__ == '__main__':
    import logging

    logging.basicConfig(
        level=getattr(logging, "INFO"),
        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
        stream=sys.stderr
    )

    mode = "jp46"
    #mode = "color"
    method = "0"
    ip = "192.168.0.9"

    if mode == "jp46":
        supposed_framerate = 30
    elif mode == "color":
        supposed_framerate = 25

    from gstmanager.gstmanager import PipelineManager
    output_caps = "video/x-raw-yuv, format=(fourcc)I420, width=(int)1920, height=(int)1088, framerate=(fraction)%s/1" %supposed_framerate

    if mode == "color":
        pipeline_desc = 'rtspsrc location=rtsp://%s:554 protocols=0x00000001 latency=0 ! queue ! rtpjpegdepay ! jpegdec ! videorate name=videorate ! %s !  queue ! xvimagesink max-lateness=-1' %(ip, output_caps)
    elif mode == "jp46":
        pipeline_desc = 'rtspsrc location=rtsp://%s:554 protocols=0x00000001 latency=0 ! rtpjpegdepay ! jpegdec ! queue leaky=2 ! jp462bayer ! queue leaky=2 ! bayer2rgb2 method=%s ! queue ! ffmpegcolorspace ! videorate name=videorate ! %s !  queue ! xvimagesink max-lateness=-1 sync=false' %(ip, method, output_caps)

    import gobject
    pipelinel = PipelineManager(pipeline_desc)
    pipelinel.run()

    s = StatsGetter(framerate=supposed_framerate)
    gobject.timeout_add(1000, s.get_numbers, pipelinel) 

    import gtk
    gtk.main()
