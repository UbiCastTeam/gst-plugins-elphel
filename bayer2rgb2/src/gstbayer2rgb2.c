/*
 * GStreamer
 * Copyright (C) 2009 Anthony Violo <anthony.violo@ubicast.eu>
 * Bayer pattern decoding functions from libdc1394 project
 * ("http://damien.douxchamps.net/ieee1394/libdc1394/"), written by
 * Damien Douxchamps and Frederic Devernay; the original VNG and AHD Bayer
 * decoding are from Dave Coffin's DCRAW.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-bayer2rgb2
 *
 * Converts raw bayer data to RGB images with selectable demosaicing methods and JP46 (Elphel) mode support.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-0.10 filesrc location=test/fruits.jp4 ! jpegdec ! bayer2rgb2 jp46-mode=true method=6 ! ffmpegcolorspace ! freeze ! ximagesink
 * ]|
 * |[
 * gst-launch-0.10 rtspsrc location=rtsp://192.168.1.9:554 protocols=0x00000001 ! rtpjpegdepay ! jpegdec ! queue ! jp462bayer
 * ! queue ! bayer2rgb2 ! queue ! ffmpegcolorspace ! videorate ! "video/x-raw-yuv, format=(fourcc)I420, width=(int)1920,
 * height=(int)1088, framerate=(fraction)25/1" ! xvimagesink sync=false max-lateness=-1 -v
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <string.h>
#include "gstbayer2rgb2.h"

GST_DEBUG_CATEGORY_STATIC (gst_bayer2rgb2_debug);
#define GST_CAT_DEFAULT gst_bayer2rgb2_debug


enum
{
  LAST_SIGNAL
};

enum
  {
    PROP_0,
    PROP_METHODE,
    PROP_BPP,
  };

enum
  {
    NEAREST,
    SIMPLE,
    BILINEAR,
    HQLINEAR,
    DOWNSAMPLE,
    EDGESENSE,
    VNG,
    AHD
  };
#define SINK_CAPS "video/x-raw-bayer,format=(string){gbrg, bggr, grbg, rggb}," \
  "width=(int)[1,MAX],height=(int)[1,MAX],framerate=(fraction)[0/1,MAX]"

enum
{
  GST_BAYER_2_RGB_FORMAT_BGGR = 0,
  GST_BAYER_2_RGB_FORMAT_GBRG,
  GST_BAYER_2_RGB_FORMAT_GRBG,
  GST_BAYER_2_RGB_FORMAT_RGGB
};

static const GstElementDetails gst_alpha_color_details =
GST_ELEMENT_DETAILS
  (
   "Bayer to RGB conversion filter",
   "Filter/Effect/Video",
   "Converts raw bayer data to RGB images with selectable demosaicing methods and JP46 (Elphel) mode support",
   "Anthony Violo <anthony.violo@ubicast.eu>"
   );

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE
  ("sink",
   GST_PAD_SINK,
   GST_PAD_ALWAYS,
   GST_STATIC_CAPS(SINK_CAPS)
   );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE
  ("src",
   GST_PAD_SRC,
   GST_PAD_ALWAYS,
   GST_STATIC_CAPS(GST_VIDEO_CAPS_RGB ";")
   );


#define DEBUG_INIT(bla) \
  GST_DEBUG_CATEGORY_INIT (gst_bayer2rgb2_debug, "bayer2rgb", 0, "bayer2rgb element");

GST_BOILERPLATE_FULL(GstBayer2rgb2, gst_bayer2rgb2, GstBaseTransform,
		GST_TYPE_BASE_TRANSFORM, DEBUG_INIT);

static void gst_bayer2rgb2_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_bayer2rgb2_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_bayer2rgb2_transform
(GstBaseTransform * pad, GstBuffer *inbuf, GstBuffer *outbuf);
static GstCaps *gst_bayer2rgb2_transform_caps
(GstBaseTransform * btrans, GstPadDirection direction, GstCaps * caps);
static gboolean gst_bayer2rgb2_set_caps
(GstBaseTransform * btrans, GstCaps * incaps, GstCaps * outcaps);
static gboolean gst_bayer2rgb2_get_unit_size (GstBaseTransform * base,
				     GstCaps * caps, guint * size);

static GType
gst_methode_pattern_get_type (int flag)
{
  static GType methode_pattern_type = 0;

  if (!methode_pattern_type)
	{
	  static GEnumValue pattern_types[] =
	    {
	      { NEAREST, "Nearest", "Nearest" },
	      { SIMPLE,  "Simple",  "Simple" },
	      { BILINEAR, "Bilinear", "Bilinear" },
	      { HQLINEAR, "Hqlinear", "Hqlinear" },
	      { DOWNSAMPLE, "Downsample", "Downsample" },
	      { EDGESENSE, "Edgesense", "Edgesense" },
	      { VNG, "Vng", "Vng" },
	      { AHD, "Ahd", "Ahd" },
	      {0, NULL, NULL },
	    };
	  methode_pattern_type =
	    g_enum_register_static ("GstMethodeRGB",
				    pattern_types);
	}
  return methode_pattern_type;
}

static void
gst_bayer2rgb2_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);
  gst_element_class_set_details(element_class, &gst_alpha_color_details);
  gst_element_class_add_pad_template(element_class,
     gst_static_pad_template_get (&sink_factory));
  gst_element_class_add_pad_template (element_class,
     gst_static_pad_template_get (&src_factory));
}

static void
gst_bayer2rgb2_class_init (GstBayer2rgb2Class * klass)
{
  GObjectClass		*gobject_class;
  GstElementClass	*gstelement_class;
  GstBaseTransformClass *gstbasetransform_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasetransform_class = (GstBaseTransformClass *) klass;
  gobject_class->set_property = gst_bayer2rgb2_set_property;
  gobject_class->get_property = gst_bayer2rgb2_get_property;
  gstbasetransform_class->transform_caps =
    GST_DEBUG_FUNCPTR (gst_bayer2rgb2_transform_caps);
  GST_BASE_TRANSFORM_CLASS (klass)->get_unit_size =
      GST_DEBUG_FUNCPTR (gst_bayer2rgb2_get_unit_size);
  gstbasetransform_class->set_caps =
    GST_DEBUG_FUNCPTR (gst_bayer2rgb2_set_caps);
  gstbasetransform_class->transform =
    GST_DEBUG_FUNCPTR (gst_bayer2rgb2_transform);
  g_object_class_install_property
    (gobject_class, PROP_BPP,
     g_param_spec_boolean
     ("bpp", "Bpp", "Bits per pixel (needed with raw sensor data): \n\t\t        FALSE = 8 bpp, TRUE = 16 bpp.",
      FALSE, G_PARAM_READWRITE));
  g_object_class_install_property
    (gobject_class, PROP_METHODE,
     g_param_spec_enum
     ("method", "Method",
      "Demosaicing interpolation algorithm",
      gst_methode_pattern_get_type(0), 1, G_PARAM_READWRITE));
   GST_DEBUG_CATEGORY_INIT (gst_bayer2rgb2_debug, "bayer2rgb2", 0,
			    "YUV->RGB");
}

static void
gst_bayer2rgb2_init (GstBayer2rgb2 * filter,
		     GstBayer2rgb2Class * gclass)
{
  gst_base_transform_set_in_place (GST_BASE_TRANSFORM (filter), TRUE);
  filter->header = 0;
  filter->methode = 1;
  filter->format = 0;
  filter->width = 0;
  filter->height = 0;
  filter->bpp = FALSE;
}

static gboolean
gst_bayer2rgb2_get_unit_size (GstBaseTransform * base, GstCaps * caps,
    guint * size)
{
  GstStructure *structure;
  int width;
  int height;
  int pixsize;
  const char *name;

  structure = gst_caps_get_structure (caps, 0);
  if (gst_structure_get_int (structure, "width", &width) &&
      gst_structure_get_int (structure, "height", &height))
    {
      name = gst_structure_get_name (structure);
      if (strcmp (name, "video/x-raw-rgb"))
	    {
	        size[0] = width * height * 1.5;
	        return TRUE;
	    }
      else
	    {
            gst_structure_get_int (structure, "bpp", &pixsize);
	        *size = width * height * (pixsize / 8);
	        return TRUE;
	    }
    }
  GST_ELEMENT_ERROR (base, CORE, NEGOTIATION, (NULL),
		     ("Incomplete caps, some required field missing"));
  return FALSE;
}

static GstCaps *gst_bayer2rgb2_transform_caps
(GstBaseTransform * btrans, GstPadDirection direction, GstCaps * caps)
{
  GstStructure *structure;
  GstCaps *newcaps;
  GstStructure *newstruct;

    GST_DEBUG_OBJECT (caps, "transforming caps (from)");
  structure = gst_caps_get_structure (caps, 0);
  if (direction == GST_PAD_SRC)
    newcaps = gst_caps_new_simple ("video/x-raw-bayer", NULL);
  else
    newcaps = gst_caps_new_simple ("video/x-raw-rgb", NULL);
  newstruct = gst_caps_get_structure (newcaps, 0);
  gst_structure_set_value (newstruct, "width",
      gst_structure_get_value (structure, "width"));
  gst_structure_set_value (newstruct, "height",
      gst_structure_get_value (structure, "height"));
  gst_structure_set_value (newstruct, "framerate",
      gst_structure_get_value (structure, "framerate"));
  GST_DEBUG_OBJECT (newcaps, "transforming caps (into)");
  return newcaps;
}

static gboolean gst_bayer2rgb2_set_caps
(GstBaseTransform * btrans, GstCaps * incaps, GstCaps * outcaps)
{
    GstBayer2rgb2 *bayer2rgb2 = GST_BAYER2RGB2 (btrans);
    GstStructure *structure;
    const char *format;

    GST_DEBUG ("in caps %" GST_PTR_FORMAT " out caps %" GST_PTR_FORMAT, incaps,
          outcaps);
    structure = gst_caps_get_structure (incaps, 0);
    gst_structure_get_int (structure, "width", &bayer2rgb2->width);
    gst_structure_get_int (structure, "height", &bayer2rgb2->height);
    format = gst_structure_get_string (structure, "format");
    if (g_str_equal (format, "bggr"))
        bayer2rgb2->format = GST_BAYER_2_RGB_FORMAT_BGGR;
    else if (g_str_equal (format, "gbrg"))
        bayer2rgb2->format = GST_BAYER_2_RGB_FORMAT_GBRG;
    else if (g_str_equal (format, "grbg"))
        bayer2rgb2->format = GST_BAYER_2_RGB_FORMAT_GRBG;
    else if (g_str_equal (format, "rggb"))
        bayer2rgb2->format = GST_BAYER_2_RGB_FORMAT_RGGB;
    else
        return FALSE;
  return TRUE;
}


static void
gst_bayer2rgb2_set_property (GObject * object, guint prop_id,
			     const GValue * value, GParamSpec * pspec)
{
  GstBayer2rgb2 *filter = GST_BAYER2RGB2 (object);

  switch (prop_id)
    {
    case PROP_METHODE:
      filter->methode = g_value_get_enum (value);
      break;
    case PROP_BPP:
      filter->bpp = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gst_bayer2rgb2_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBayer2rgb2 *filter = GST_BAYER2RGB2 (object);
  switch (prop_id)
    {
    case PROP_METHODE:
      g_value_set_enum (value, filter->methode);
      break;
    case PROP_BPP:
      g_value_set_boolean (value, filter->bpp);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

uint8_t	*bayer2rgb2(GstBayer2rgb2 *filter, uint8_t *input, uint8_t *output)
{
  uint32_t	bpp = 0;
  uint32_t	in_size = 0;

  bpp = filter->bpp ? 16 : 8;
  in_size = filter->width * filter->height;
  switch(bpp)
    {
    case 8:
      dc1394_bayer_decoding_8bit(input, output, filter->width, filter->height, filter->format, filter->methode);
      break;
    case 16:
    default:
      {
	uint8_t		tmp = 0;
	uint32_t	i = 0;
	for(i = 0; i < in_size; i += 2)
	  {
	    tmp = *(((uint8_t*)input) + i);
	    *(((uint8_t*)input) + i) = *(((uint8_t*)input) + i + 1);
	    *(((uint8_t*)input) + i + 1) = tmp;
	  }
      }
      dc1394_bayer_decoding_16bit((const uint16_t*)input, (uint16_t*)output, filter->width, filter->height, filter->format, filter->methode, 16);
      break;
    }
  return output;
}

static GstFlowReturn
gst_bayer2rgb2_transform(GstBaseTransform * pad, GstBuffer *inbuf, GstBuffer *outbuf)
{
  GstBayer2rgb2		*filter;
  uint8_t		*input;
  uint8_t		*output;

  filter = GST_BAYER2RGB2(pad);
  input = (uint8_t *) GST_BUFFER_DATA (inbuf);
  output = (uint8_t *) GST_BUFFER_DATA (outbuf);
  if (filter->format == GST_BAYER_2_RGB_FORMAT_RGGB)
    filter->format = (guint16) DC1394_COLOR_FILTER_RGGB;
  if (filter->format == GST_BAYER_2_RGB_FORMAT_GBRG)
    filter->format = (guint16) DC1394_COLOR_FILTER_GRBG;
  if (filter->format == GST_BAYER_2_RGB_FORMAT_GRBG)
    filter->format = (guint16) DC1394_COLOR_FILTER_GBRG;
 if (filter->format == GST_BAYER_2_RGB_FORMAT_BGGR)
    filter->format = (guint16) DC1394_COLOR_FILTER_BGGR;
  if (filter->height > 8 && filter->width > 8)
    output = bayer2rgb2(filter, input, output);
  return GST_FLOW_OK;
}

static gboolean
bayer2rgb2_init (GstPlugin * bayer2rgb2)
{
  GST_DEBUG_CATEGORY_INIT (gst_bayer2rgb2_debug, "bayer2rgb2",
       0, "bayer2rgb2");
  return gst_element_register (bayer2rgb2, "bayer2rgb2", GST_RANK_NONE,
      GST_TYPE_BAYER2RGB2);
}

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "bayer2rgb2",
    "Converts raw bayer data to rgb images",
    bayer2rgb2_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://www.ubicast.eu/"
)

