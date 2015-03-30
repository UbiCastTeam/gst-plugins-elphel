# About demosaic / JP4 #

Elphel cameras have an on-FPGA color demosaicing algorithm which is quick but has limitations on the final image quality. This is what is being used when the Elphel 353 Camera is in "color" or "mono" image mode (in the web interface).

To improve overall image quality, the Elphel cameras have a special mode which bypasses the onboard demosaicing algorithm and transmit variations of raw Bayer (named jp4, jp46, ...) payloaded in jpeg images, so that the color interpolation can be done in software, client-side, with better interpolation algorithms.

# Demosaicing variations #

![http://wiki.elphel.com/images/0/00/Detail01.jpg](http://wiki.elphel.com/images/0/00/Detail01.jpg)

[Client-side demosaic enhancement examples on wiki.elphel.com](http://wiki.elphel.com/index.php?title=Demosaic_on_client_side)

# JP4 information #
  * [Preparation of the sensor raw Bayer data for efficient JPEG compression (JP4 mode)](http://www.linuxfordevices.com/files/article083/color_proc_jp46.html)
  * [How do decode JP4](http://wiki.elphel.com/index.php?title=JP4)