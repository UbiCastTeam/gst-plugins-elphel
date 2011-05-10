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
#include <gst/video/video.h>
#include <string.h>
#include <stdint.h>
#include "gstjp462bayer.h"

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
	PROP_0,
	PROP_SILENT,
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
static gboolean gst_jp462bayer_get_unit_size (GstBaseTransform * base, GstCaps * caps, guint * size);
static GstCaps *gst_jp462bayer_transform_caps(GstBaseTransform * trans, GstPadDirection direction, GstCaps * caps);

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
  GstBaseTransformClass *trans_class;

  gobject_class = (GObjectClass *) klass;

  gstelement_class = (GstElementClass *) klass;
  trans_class = (GstBaseTransformClass *) klass;

  gobject_class->set_property = gst_jp462bayer_set_property;
  gobject_class->get_property = gst_jp462bayer_get_property;

	trans_class->transform_caps = GST_DEBUG_FUNCPTR (gst_jp462bayer_transform_caps);

  trans_class->get_unit_size =
    GST_DEBUG_FUNCPTR (gst_jp462bayer_get_unit_size);

  trans_class->set_caps =
    GST_DEBUG_FUNCPTR (gst_jp462bayer_set_caps);

  trans_class->transform =
    GST_DEBUG_FUNCPTR (gst_jp462bayer_transform);
}

static gboolean
gst_jp462bayer_get_unit_size (GstBaseTransform * trans, GstCaps * caps, guint * size)
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
	      	*size = width * height;
	      	return TRUE;
	    	}
		}
	}
	//GST_ELEMENT_ERROR (base, CORE, NEGOTIATION, (NULL),
		//     ("Incomplete caps, some required field missing"));
	return FALSE;
}

static GstCaps *
gst_jp462bayer_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps)
{
  GstCaps *ret;
  GstStructure *structure = NULL;

  /* this function is always called with a simple caps */
  g_return_val_if_fail (GST_CAPS_IS_SIMPLE (caps), NULL);

  GST_DEBUG_OBJECT (trans,
      "Transforming caps %" GST_PTR_FORMAT " in direction %s", caps,
      (direction == GST_PAD_SINK) ? "sink" : "src");

  ret = gst_caps_copy (caps);
		structure = gst_structure_copy (gst_caps_get_structure (ret, 0));
	gst_structure_set (structure,
      		"width", GST_TYPE_INT_RANGE, 1, G_MAXINT,
      		"height", GST_TYPE_INT_RANGE, 1, G_MAXINT, NULL);
		gst_caps_append_structure (ret, structure);
  /* if pixel aspect ratio, make a range of it */
  if (gst_structure_has_field (structure, "pixel-aspect-ratio")) {
	gst_structure_set (structure, "pixel-aspect-ratio", GST_TYPE_FRACTION_RANGE,
	1, G_MAXINT, G_MAXINT, 1, NULL);
  }
  GST_DEBUG_OBJECT (trans, "returning caps: %" GST_PTR_FORMAT, ret);
  return ret;
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
gst_jp462bayer_set_caps (GstBaseTransform *pad, GstCaps * incaps, GstCaps * outcaps)
{
	GstJP462bayer 	*filter;
	GstStructure 	*structure;
	gint 			width_in=0, height_in=0;
	gint 			width_out=0, height_out=0;
	float 			size;
	int				i, j;

	filter = GST_JP462BAYER (pad);
	structure = gst_caps_get_structure (incaps, 0);
	gst_structure_get_int (structure, "width", &width_in);
	gst_structure_get_int (structure, "height", &height_in);
	filter->width = width_in;
	filter->height = height_in;
	structure = gst_caps_get_structure (outcaps, 0);
	gst_structure_get_int (structure, "width", &width_out);
	gst_structure_get_int (structure, "height", &height_out);
	if 	(width_in == width_out && height_in == height_out)
		filter->size = 1;
	else if ((float)width_in / (float)width_out == 2.0 && (float)height_in / (float)height_out  == 2.0)
		filter->size = 2;
	else if ((float)width_in / (float)width_out == 4.0 && (float)height_in / (float)height_out  == 4.0)
		filter->size = 4;
	else
	{
		GST_ERROR("you should put output resolution divisible by 1, 2, 3 or 4");
		return FALSE;
	}
	size = (filter->size == 1) ? 0.5 : (filter->size == 2) ? 1.0 : 2.0;
	for (i = 0, j = 0; i < 16; i++, j++)
    {
		filter->index1[i] = (i % 2 == 1) ? (filter->index1[i - 1] + 8) : (i * size);
		filter->index2[j] = filter->index1[j] * filter->width;
    }
    return TRUE;
}

int				get_value(guint32 x, guint32 value1, guint32 value2, guint32 b_of, GstJP462bayer *f, guint8 *data)
{
	guint32		value = 0;
	guint8	k, l, m;

	for (l = 0, m = 0; l < f->size; l++)
		for (k = 0; k < f->size; k++, m++)
				value += data[x + value1 + value2 + k + b_of + f->width * l];
	return value / (f->size << (f->size / 2));
}

static GstFlowReturn
gst_jp462bayer_transform(GstBaseTransform * pad, GstBuffer *inbuf, GstBuffer *outbuf)
{
	GstJP462bayer		*filter;
	guint8 				i, j;
	guint32				value;
	guint32				y, x;
	guint32				b_of;
	guint32				h_of;

	filter = GST_JP462BAYER(pad);
	for (y = 0, b_of = 0, value = 0; y < filter->height; y += 16, b_of += filter->width << 4)
			for (x = 0; x < filter->width; x += 16)
				for (j = 0, h_of = 0; j < (16 / filter->size); ++j, h_of += filter->width)
					for (i = 0; i < (16 / filter->size); ++i)
					{
						value  = (filter->size == 1) ? (inbuf->data[x + filter->index1[i] + filter->index2[j] + b_of]) :
									get_value(x, filter->index1[i], filter->index2[j], b_of, filter, inbuf->data);
						outbuf->data[(x / filter->size) + i + (h_of  / filter->size) + (b_of / (filter->size << filter->size / 2))] = value;
					}
	return  GST_FLOW_OK;
}

static gboolean
jp462bayer_init (GstPlugin * jp462bayer)
{
  if (!gst_element_register (jp462bayer, "jp462bayer", GST_RANK_NONE,
          GST_TYPE_JP462BAYER))
    return FALSE;
  GST_DEBUG_CATEGORY_INIT (gst_jp462bayer_debug, "jp462bayer", 0,
      "jp462bayer element");
  return TRUE;
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

