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
        level=getattr(logging, "DEBUG"),
        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
        stream=sys.stderr
    )

    from gstmanager.gstmanager import PipelineManager
    output_caps = "video/x-raw-yuv, format=(fourcc)I420, width=(int)1920, height=(int)1088, framerate=(fraction)25/1"

    pipeline_desc = 'rtspsrc location=rtsp://192.168.1.9:554 protocols=0x00000001 latency=100 ! queue ! rtpjpegdepay ! jpegdec ! queue ! jp462bayer ! queue ! bayer2rgb2 ! queue ! ffmpegcolorspace ! videorate name=videorate ! %s !  queue ! xvimagesink max-lateness=-1 sync=false' %output_caps

    import gobject
    pipelinel = PipelineManager(pipeline_desc)
    gobject.idle_add(pipelinel.run)

    gobject.timeout_add(1000, get_numbers, pipelinel) 

    import gtk
    gtk.main()

