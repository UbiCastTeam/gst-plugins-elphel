from gstmanager.event import EventListener 

class EOS_actioner(EventListener):
    # This class will subscribe to proxied eos messages
    def __init__(self):
        EventListener.__init__(self)
        self.registerEvent("eos")

    def evt_eos(self, event):
    # This is the callback used for every evt_MSGNAME received
        logger.info("EOS Recieved")

def get_numbers(pipelinel):
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

    pipeline_desc = 'rtspsrc location=rtsp://192.168.1.9:554 protocols=0x00000001 latency=10 ! queue ! rtpjpegdepay ! jpegdec ! videorate name=videorate ! %s !  queue ! xvimagesink max-lateness=-1 sync=false' %output_caps

    pipelinel = PipelineManager(pipeline_desc)
    pipelinel.run()

    import gobject
    gobject.timeout_add(1000, get_numbers, pipelinel) 

    import gtk
    gtk.main()

