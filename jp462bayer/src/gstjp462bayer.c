/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2009 Anthony Violo <anthony.violo@ubicast.eu>
 * Copyright (C) 2009 Konstantin Kim <kimstik@gmail.com>
 * Copyright (C) 2009 Deschenaux Luc <luc.deschenaux.mta@sunrise.ch>
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
 * SECTION:element-jp462bayer
 *
 * FIXME:Describe jp462bayer here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! jp462bayer ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <string.h>
#include <stdint.h>
#include "gstjp462bayer.h"

GST_DEBUG_CATEGORY_STATIC (gst_jp462bayer_debug);
#define GST_CAT_DEFAULT gst_jp462bayer_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

GST_BOILERPLATE (GstJP462bayer, gst_jp462bayer, GstBaseTransform,
    GST_TYPE_BASE_TRANSFORM);

static void gst_jp462bayer_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_jp462bayer_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_jp462bayer_set_caps (GstBaseTransform * btrans, GstCaps * incaps, GstCaps * outcaps);
static GstFlowReturn gst_jp462bayer_transform(GstBaseTransform * pad, GstBuffer *inbuf, GstBuffer *outbuf);
static gboolean gst_bayer2rgb2_get_unit_size (GstBaseTransform * base, GstCaps * caps, guint * size);

/* GObject vmethod implementations */

static void
gst_jp462bayer_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
    "JP46 to Bayer convertion filter",
    "Filter/Effect/Video",
    "Converts raw JP46 data to raw bayer data",
    "Anthony Violo <anthony.violo@ubicast.eu>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the jp462bayer's class */
static void
gst_jp462bayer_class_init (GstJP462bayerClass * klass)
{
  GObjectClass		*gobject_class;
  GstElementClass	*gstelement_class;
  GstBaseTransformClass *gstbasetransform_class;

  gobject_class = (GObjectClass *) klass;

  gstelement_class = (GstElementClass *) klass;
  gstbasetransform_class = (GstBaseTransformClass *) klass;

  gobject_class->set_property = gst_jp462bayer_set_property;
  gobject_class->get_property = gst_jp462bayer_get_property;

  gstbasetransform_class->get_unit_size =
    GST_DEBUG_FUNCPTR (gst_bayer2rgb2_get_unit_size);

  gstbasetransform_class->set_caps =
    GST_DEBUG_FUNCPTR (gst_jp462bayer_set_caps);

  gstbasetransform_class->transform =
    GST_DEBUG_FUNCPTR (gst_jp462bayer_transform);

  GST_DEBUG_CATEGORY_INIT (gst_jp462bayer_debug, "jp462bayer", 0,
			    "YUV->Bayer");
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_jp462bayer_init (GstJP462bayer * filter,
    GstJP462bayerClass * gclass)
{
  gst_base_transform_set_in_place (GST_BASE_TRANSFORM (filter), TRUE);
}

static void
gst_jp462bayer_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstJP462bayer *filter = GST_JP462BAYER (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_jp462bayer_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstJP462bayer *filter = GST_JP462BAYER (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_bayer2rgb2_get_unit_size (GstBaseTransform * base, GstCaps * caps,
    guint * size)
{
  GstStructure *structure;
  int width;
  int height;
  int pixsize;
  int i = 0;
  const char *name;

  structure = gst_caps_get_structure (caps, 0);
  if (gst_structure_get_int (structure, "width", &width) &&
      gst_structure_get_int (structure, "height", &height))
    {
      name = gst_structure_get_name (structure);
      if (strcmp (name, "ANY"))
	{
	  if (size[i])
	    for (i = 0; size[i]; i++)
	      *size = width * height * 1.5;
	  else
	    size[0] = GST_ROUND_UP_4 (width) * height * 1.5;
	  return TRUE;
	}
      else
	{
	  if (gst_structure_get_int (structure, "bpp", &pixsize))
	    {
	      *size = width * height * 1.5;
	      return TRUE;
	    }
	}
    }
  GST_ELEMENT_ERROR (base, CORE, NEGOTIATION, (NULL),
		     ("Incomplete caps, some required field missing"));
  return FALSE;
}

static gboolean
gst_jp462bayer_set_caps (GstBaseTransform *pad, GstCaps * incaps, GstCaps * outcaps)
{
  GstJP462bayer *filter;
  GstStructure *structure;
  gint w=0, h=0;

  filter = GST_JP462BAYER (pad);
  structure = gst_caps_get_structure (incaps, 0);
  gst_structure_get_int (structure, "width", &w);
  filter->width = w;
  gst_structure_get_int (structure, "height", &h);
  filter->height = h;
  return TRUE;
}

static GstFlowReturn
gst_jp462bayer_transform(GstBaseTransform * pad, GstBuffer *inbuf, GstBuffer *outbuf)
{
  GstJP462bayer		*filter;
  guint32		y, x;
  guint32		b_of = 0;
  guint32		h_of;
  uint32_t		i, j;
  guint32		index1[16]={0,8,1,9,2,10,3,11,4,12,5,13,6,14,7,15};
  guint32		index2[16];

  filter = GST_JP462BAYER(pad);
  for (j = 0;j < 16; ++j)
    index2[j] = index1[j] * filter->width;
  for (y = 0; y < filter->height; y += 16, b_of += filter->width << 4)
    for (x = 0; x < filter->width; x += 16)
      for (j = 0, h_of = 0; j < 16; ++j, h_of += filter->width)
	for (i = 0; i < 16; ++i)
	  outbuf->data[x + i + h_of + b_of] = inbuf->data[x + index1[i] + index2[j] + b_of];
  return  GST_FLOW_OK;
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
jp462bayer_init (GstPlugin * jp462bayer)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template jp462bayer' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_jp462bayer_debug, "jp462bayer",
      0, "Template jp462bayer");

  return gst_element_register (jp462bayer, "jp462bayer", GST_RANK_NONE,
      GST_TYPE_JP462BAYER);
}

/* gstreamer looks for this structure to register jp462bayers
 *
 * exchange the string 'Template jp462bayer' with your jp462bayer description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "jp462bayer",
    "Converts raw JP46 data to raw bayer data",
    jp462bayer_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://www.ubicast.eu/"
)
