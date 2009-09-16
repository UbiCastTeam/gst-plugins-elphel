#!/bin/env/python
# -*- coding: utf-8 -*
# Requires http://code.google.com/p/gstmanager/
# Copyright (C) 2008  Florent Thiery <florent.thiery@ubicast.eu>
# Released under the terms of the LGPL

from gstmanager.event import EventListener 

def get_numbers(pipelinel):
    if pipelinel.get_state() == "GST_STATE_PLAYING":
        drop = pipelinel.get_property_on_element(element_name="videorate", property_name="drop")
        duplicate = pipelinel.get_property_on_element(element_name="videorate", property_name="duplicate")
        print "Dropped frames: %s Duplicated frames: %s" %(drop, duplicate)
    return True

if __name__ == '__main__':
    import logging, sys

    logging.basicConfig(
        level=getattr(logging, "INFO"),
        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
        stream=sys.stderr
    )

    from gstmanager.gstmanager import PipelineManager
    output_caps = "video/x-raw-yuv, format=(fourcc)I420, width=(int)1920, height=(int)1088, framerate=(fraction)25/1"

    pipeline_desc = 'rtspsrc location=rtsp://192.168.1.9:554 protocols=0x00000001 latency=0 ! queue ! rtpjpegdepay ! jpegdec ! videorate name=videorate ! %s !  queue ! xvimagesink max-lateness=-1' %output_caps

    pipelinel = PipelineManager(pipeline_desc)
    pipelinel.run()

    import gobject
    gobject.timeout_add(1000, get_numbers, pipelinel) 

    import gtk
    gtk.main()

