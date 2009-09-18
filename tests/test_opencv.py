from gstmanager.event import EventListener 
import logging
logger = logging.getLogger('message_test')

# You need to have gst-opencv installed
# bzr branch lp:~lenoam/gst-opencv/trunk

class Actioner(EventListener):
    def __init__(self):
        EventListener.__init__(self)
        self.registerEvent("eos")
        self.registerEvent("face")

    def evt_eos(self, event):
        logger.info("EOS Received")

    def evt_face(self, event):
        data = event.content["data"]
        x = data["x"]
        y = data["y"]
        width = data["width"]
        height = data["height"]
        logger.info("Face of %sx%s found at [%s:%s]" %(width, height, x, y))

if __name__ == '__main__':
    # Width, height for face detection processing
    awidth, aheight, framerate = 320, 240, 25
    ip = "192.168.1.9"

    import logging, sys 

    logging.basicConfig(
        level=getattr(logging, "INFO"),
        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
        stream=sys.stderr
    )

    from gstmanager.sbins.sources.elphel import ElphelSource
    e = ElphelSource(ip)

    analysis_caps = "video/x-raw-yuv, format=(fourcc)I420, width=(int)%s, height=(int)%s, framerate=(fraction)%s/1" %(awidth, aheight, framerate)

    from gstmanager.gstmanager import PipelineManager
    pipeline_desc = "%s ! queue ! videoscale ! %s ! queue ! ffmpegcolorspace ! queue ! facedetect ! ffmpegcolorspace ! xvimagesink max-lateness=-1 v_src_tee. ! queue ! xvimagesink max-lateness=-1" %(e.sbin, analysis_caps)

    actioner = Actioner()
    pipelinel = PipelineManager(pipeline_desc)
    pipelinel.run()

    import gtk
    gtk.main()
