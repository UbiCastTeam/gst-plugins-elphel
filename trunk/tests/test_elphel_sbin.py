#!/usr/bin/env python
# -*- coding: utf-8 -*-

import logging, sys

logging.basicConfig(
    level=getattr(logging, "DEBUG"),
    format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
    stream=sys.stderr
)

from gstmanager.sbins.sources.elphel import ElphelSource
e = ElphelSource("192.168.1.9")

from gstmanager.sbins.sinks.xvimagesink import XVImageSink
s = XVImageSink()

from gstmanager.sbinmanager import SBinManager
man = SBinManager()
man.add_many(e, s)

from gstmanager.gstmanager import PipelineManager
pipelinel = PipelineManager(man.pipeline_desc)
pipelinel.run() 
import gtk
gtk.main()
