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
 * SECTION:element-bayer2rgb
 *
 * Decodes raw camera bayer (fourcc BA81) to RGB.
 */

/*
 * In order to guard against my advancing maturity, some extra detailed
 * information about the logic of the decode is included here.  Much of
 * this was inspired by a technical paper from siliconimaging.com, which
 * in turn was based upon an article from IEEE,
 * T. Sakamoto, C. Nakanishi and T. Hase,
 * “Software pixel interpolation for digital still cameras suitable for
 *  a 32-bit MCU,”
 * IEEE Trans. Consumer Electronics, vol. 44, no. 4, November 1998.
 *
 * The code assumes a Bayer matrix of the type produced by the fourcc
 * BA81 (v4l2 format SBGGR8) of width w and height h which looks like:
 *       0 1 2 3  w-2 w-1
 *
 *   0   B G B G ....B G
 *   1   G R G R ....G R
 *   2   B G B G ....B G
 *       ...............
 * h-2   B G B G ....B G
 * h-1   G R G R ....G R
 *
 * We expand this matrix, producing a separate {r, g, b} triple for each
 * of the individual elements.  The algorithm for doing this expansion is
 * as follows.
 *
 * We are designing for speed of transformation, at a slight expense of code.
 * First, we calculate the appropriate triples for the four corners, the
 * remainder of the top and bottom rows, and the left and right columns.
 * The reason for this is that those elements are transformed slightly
 * differently than all of the remainder of the matrix. Finally, we transform
 * all of the remainder.
 *
 * The transformation into the "appropriate triples" is based upon the
 * "nearest neighbor" principal, with some additional complexity for the
 * calculation of the "green" element, where an "adaptive" pairing is used.
 *
 * For purposes of documentation and indentification, each element of the
 * original array can be put into one of four classes:
 *   R   A red element
 *   B   A blue element
 *   GR  A green element which is followed by a red one
 *   GB  A green element which is followed by a blue one
 */

/**
 * SECTION:element-bayer2rgb2
 *
 * Converts raw bayer data to RGB images with selectable demosaicing methods and JP46 (Elphel) mode support.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-0.10 rtspsrc location=rtsp://192.168.1.9:554 protocols=0x00000001 ! rtpjpegdepay ! jpegdec ! queue ! jp462bayer
 * ! queue ! bayer2rgb2 ! queue ! ffmpegcolorspace ! videorate ! "video/x-raw-yuv, format=(fourcc)I420, width=(int)1920,
 * height=(int)1088, framerate=(fraction)25/1" ! xvimagesink sync=false max-lateness=-1 -v
 * ]|
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
/*
    NEAREST,
    SIMPLE,
*/
    BILINEAR,
    HQLINEAR,
    DOWNSAMPLE,
    EDGESENSE,
    VNG,
    AHD
  };

#define SINK_CAPS "video/x-raw-bayer,format=(string){gbrg, bggr, grbg, rggb}," \
  "width=(int)[1,MAX],height=(int)[1,MAX],framerate=(fraction)[0/1,MAX]"

#define	SRC_CAPS                                   \
    GST_VIDEO_CAPS_RGB ";"                         \
    GST_VIDEO_CAPS_BGR ";"                         \
    GST_VIDEO_CAPS_RGBx ";"                        \
    GST_VIDEO_CAPS_xRGB ";"                        \
    GST_VIDEO_CAPS_BGRx ";"                        \
    GST_VIDEO_CAPS_xBGR ";"                        \
    GST_VIDEO_CAPS_RGBA ";"                        \
    GST_VIDEO_CAPS_ARGB ";"                        \
    GST_VIDEO_CAPS_BGRA ";"                        \
    GST_VIDEO_CAPS_ABGR ";"

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
   GST_STATIC_CAPS(SRC_CAPS)
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
static void gst_bayer2rgb2_reset (GstBayer2rgb2 * filter);
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
/*
	      { NEAREST, "Nearest", "Nearest" },
	      { SIMPLE,  "Simple",  "Simple" },
*/
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
    gst_bayer2rgb2_reset (filter);
    gst_base_transform_set_in_place (GST_BASE_TRANSFORM (filter), TRUE);
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
      /* Our name must be either video/x-raw-bayer video/x-raw-rgb */
      if (strcmp (name, "video/x-raw-rgb"))
	    {
            /* For bayer, we handle only BA81 (BGGR), which is BPP=24 */
	        *size = GST_ROUND_UP_4 (width) * height;
	        return TRUE;
	    }
      else
	    {
            /* For output, calculate according to format */
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
    newcaps = gst_caps_from_string ("video/x-raw-bayer,"
        "format=(string){gbrg, bggr,grbg,rggb}");
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

/* Routine to convert colormask value into relative byte offset */
static int
get_pix_offset (int mask, int bpp)
{
  int bpp32 = (bpp / 8) - 3;

  switch (mask) {
    case 255:
      return 2 + bpp32;
    case 65280:
      return 1 + bpp32;
    case 16711680:
      return 0 + bpp32;
    case -16777216:
      return 0;
    default:
      GST_ERROR ("Invalid color mask 0x%08x", mask);
      return -1;
  }
}

static gboolean gst_bayer2rgb2_set_caps
(GstBaseTransform * btrans, GstCaps * incaps, GstCaps * outcaps)
{
    GstBayer2rgb2 *bayer2rgb2 = GST_BAYER2RGB2 (btrans);
    GstStructure *structure;
    int val, bpp;
    const char *format;

    GST_DEBUG ("in caps %" GST_PTR_FORMAT " out caps %" GST_PTR_FORMAT, incaps,
          outcaps);
    structure = gst_caps_get_structure (incaps, 0);
    gst_structure_get_int (structure, "width", &bayer2rgb2->width);
    gst_structure_get_int (structure, "height", &bayer2rgb2->height);
    bayer2rgb2->stride = GST_ROUND_UP_4 (bayer2rgb2->width);
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

    /* To cater for different RGB formats, we need to set params for later */
    structure = gst_caps_get_structure (outcaps, 0);
    gst_structure_get_int (structure, "bpp", &bpp);
    bayer2rgb2->pixsize = bpp / 8;
    gst_structure_get_int (structure, "red_mask", &val);
    bayer2rgb2->r_off = get_pix_offset (val, bpp);
    gst_structure_get_int (structure, "green_mask", &val);
    bayer2rgb2->g_off = get_pix_offset (val, bpp);
    gst_structure_get_int (structure, "blue_mask", &val);
    bayer2rgb2->b_off = get_pix_offset (val, bpp);
    return TRUE;
}

static void
gst_bayer2rgb2_reset (GstBayer2rgb2 * filter)
{
  filter->width = 0;
  filter->height = 0;
  filter->stride = 0;
  filter->pixsize = 0;
  filter->r_off = 0;
  filter->g_off = 0;
  filter->b_off = 0;
  filter->header = 0;
  filter->methode = 0;
  filter->format = 0;
  filter->bpp = FALSE;
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

/*
 * We check if we have bayer 8 bits or 16 bits.
*/
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

/*
 * We define values for the colors, just to make the code more readable.
 */
#define	RED	0               /* Pure red element */
#define	GREENB	1               /* Green element which is on a blue line */
#define	BLUE	2               /* Pure blue element */
#define	GREENR	3               /* Green element which is on a red line */

static int
get_pixel_type (GstBayer2rgb2 * filter, int x, int y)
{
  int type;

  if (((x ^ filter->format) & 1)) {
    if ((y ^ (filter->format >> 1)) & 1)
      type = RED;
    else
      type = GREENB;
  } else {
    if ((y ^ (filter->format >> 1)) & 1)
      type = GREENR;
    else
      type = BLUE;
  }
  return type;
}

/* Routine to generate the top and bottom edges (not including corners) */
static void
hborder (uint8_t * input, uint8_t * output, int bot_top,
    int typ, GstBayer2rgb2 * filter)
{
  uint8_t *op;                  /* output pointer */
  uint8_t *ip;                  /* input pointer */
  uint8_t *nx;                  /* next line pointer */
  int ix;                       /* loop index */

  op = output + (bot_top * filter->width * (filter->height - 1) + 1) *
      filter->pixsize;
  ip = input + bot_top * filter->stride * (filter->height - 1);
  /* calculate minus or plus one line, depending upon bot_top flag */
  nx = ip + (1 - 2 * bot_top) * filter->stride;
  /* Stepping horizontally */
  for (ix = 1; ix < filter->width - 1; ix++, op += filter->pixsize) {
    switch (typ) {
      case RED:
        op[filter->r_off] = ip[ix];
        op[filter->g_off] = (ip[ix + 1] + ip[ix - 1] + nx[ix] + 1) / 3;
        op[filter->b_off] = (nx[ix + 1] + nx[ix - 1] + 1) / 2;
        typ = GREENR;
        break;
      case GREENR:
        op[filter->r_off] = (ip[ix + 1] + ip[ix - 1] + 1) / 2;
        op[filter->g_off] = ip[ix];
        op[filter->b_off] = nx[ix];
        typ = RED;
        break;
      case GREENB:
        op[filter->r_off] = nx[ix];
        op[filter->g_off] = ip[ix];
        op[filter->b_off] = (ip[ix + 1] + ip[ix - 1] + 1) / 2;
        typ = BLUE;
        break;
      case BLUE:
        op[filter->r_off] = (nx[ix + 1] + nx[ix - 1] + 1) / 2;
        op[filter->g_off] = (ip[ix + 1] + ip[ix - 1] + nx[ix] + 1) / 3;
        op[filter->b_off] = ip[ix];
        typ = GREENB;
        break;
    }
  }
}

/* Routine to generate the left and right edges, not including corners */
static void
vborder (uint8_t * input, uint8_t * output, int right_left,
    int typ, GstBayer2rgb2 * filter)
{
  uint8_t *op;                  /* output pointer */
  uint8_t *ip;                  /* input pointer */
  uint8_t *la;                  /* line above pointer */
  uint8_t *lb;                  /* line below pointer */
  int ix;                       /* loop index */
  int lr;                       /* 'left-right' flag - +1 is right, -1 is left */

  lr = (1 - 2 * right_left);
  /* stepping vertically */
  for (ix = 1; ix < filter->height - 1; ix++) {
    ip = input + right_left * (filter->width - 1) + ix * filter->stride;
    op = output + (right_left * (filter->width - 1) + ix * filter->width) *
        filter->pixsize;
    la = ip + filter->stride;
    lb = ip - filter->stride;
    switch (typ) {
      case RED:
        op[filter->r_off] = ip[0];
        op[filter->g_off] = (la[0] + ip[lr] + lb[0] + 1) / 3;
        op[filter->b_off] = (la[lr] + lb[lr] + 1) / 2;
        typ = GREENB;
        break;
      case GREENR:
        op[filter->r_off] = ip[lr];
        op[filter->g_off] = ip[0];
        op[filter->b_off] = (la[lr] + lb[lr] + 1) / 2;
        typ = BLUE;
        break;
      case GREENB:
        op[filter->r_off] = (la[lr] + lb[lr] + 1) / 2;
        op[filter->g_off] = ip[0];
        op[filter->b_off] = ip[lr];
        typ = RED;
        break;
      case BLUE:
        op[filter->r_off] = (la[lr] + lb[lr] + 1) / 2;
        op[filter->g_off] = (la[0] + ip[lr] + lb[0] + 1) / 3;
        op[filter->b_off] = ip[0];
        typ = GREENR;
        break;
    }
  }
}

/* Produce the four (top, bottom, left, right) edges */
static void
do_row0_col0 (uint8_t * input, uint8_t * output, GstBayer2rgb2 * filter)
{
  /* Horizontal edges */
  hborder (input, output, 0, get_pixel_type (filter, 1, 0), filter);
  hborder (input, output, 1, get_pixel_type (filter, 1, filter->height - 1),
      filter);

  /* Vertical edges */
  vborder (input, output, 0, get_pixel_type (filter, 0, 1), filter);
  vborder (input, output, 1, get_pixel_type (filter, filter->width - 1, 1),
      filter);
}

static void
corner (uint8_t * input, uint8_t * output, int x, int y,
    int xd, int yd, int typ, GstBayer2rgb2 * filter)
{
  uint8_t *ip;                  /* input pointer */
  uint8_t *op;                  /* output pointer */
  uint8_t *nx;                  /* adjacent line */

  op = output + y * filter->width * filter->pixsize + x * filter->pixsize;
  ip = input + y * filter->stride + x;
  nx = ip + yd * filter->stride;
  switch (typ) {
    case RED:
      op[filter->r_off] = ip[0];
      op[filter->g_off] = (nx[0] + ip[xd] + 1) / 2;
      op[filter->b_off] = nx[xd];
      break;
    case GREENR:
      op[filter->r_off] = ip[xd];
      op[filter->g_off] = ip[0];
      op[filter->b_off] = nx[0];
      break;
    case GREENB:
      op[filter->r_off] = nx[0];
      op[filter->g_off] = ip[0];
      op[filter->b_off] = ip[xd];
      break;
    case BLUE:
      op[filter->r_off] = nx[xd];
      op[filter->g_off] = (nx[0] + ip[xd] + 1) / 2;
      op[filter->b_off] = ip[0];
      break;
  }
}

static void
do_corners (uint8_t * input, uint8_t * output, GstBayer2rgb2 * filter)
{
  /* Top left */
  corner (input, output, 0, 0, 1, 1, get_pixel_type (filter, 0, 0), filter);
  /* Bottom left */
  corner (input, output, 0, filter->height - 1, 1, -1,
      get_pixel_type (filter, 0, filter->height - 1), filter);
  /* Top right */
  corner (input, output, filter->width - 1, 0, -1, 0,
      get_pixel_type (filter, filter->width - 1, 0), filter);
  /* Bottom right */
  corner (input, output, filter->width - 1, filter->height - 1, -1, -1,
      get_pixel_type (filter, filter->width - 1, filter->height - 1), filter);
}

static void
do_body (uint8_t * input, uint8_t * output, GstBayer2rgb2 * filter)
{
  int ip, op;                   /* input and output pointers */
  int w, h;                     /* loop indices */
  int type;                     /* calculated colour of current element */
  int a1, a2;
  int v1, v2, h1, h2;

  /*
   * We are processing row (line) by row, starting with the second
   * row and continuing through the next to last.  Each row is processed
   * column by column, starting with the second and continuing through
   * to the next to last.
   */
  for (h = 1; h < filter->height - 1; h++) {
    /*
     * Remember we are processing "row by row". For each row, we need
     * to set the type of the first element to be processed.  Since we
     * have already processed the edges, the "first element" will be
     * the pixel at position (1,1).  Assuming BG format, this should
     * be RED for odd-numbered rows and GREENB for even rows.
     */
    type = get_pixel_type (filter, 1, h);
    /* Calculate the starting position for the row */
    op = h * filter->width * filter->pixsize;   /* output (converted) pos */
    ip = h * filter->stride;    /* input (bayer data) pos */
    for (w = 1; w < filter->width - 1; w++) {
      op += filter->pixsize;    /* we are processing "horizontally" */
      ip++;
      switch (type) {
        case RED:
          output[op + filter->r_off] = input[ip];
          output[op + filter->b_off] = (input[ip - filter->stride - 1] +
              input[ip - filter->stride + 1] +
              input[ip + filter->stride - 1] +
              input[ip + filter->stride + 1] + 2) / 4;
          v1 = input[ip + filter->stride];
          v2 = input[ip - filter->stride];
          h1 = input[ip + 1];
          h2 = input[ip - 1];
          a1 = abs (v1 - v2);
          a2 = abs (h1 - h2);
          if (a1 < a2)
            output[op + filter->g_off] = (v1 + v2 + 1) / 2;
          else if (a1 > a2)
            output[op + filter->g_off] = (h1 + h2 + 1) / 2;
          else
            output[op + filter->g_off] = (v1 + h1 + v2 + h2 + 2) / 4;
          type = GREENR;
          break;
        case GREENR:
          output[op + filter->r_off] = (input[ip + 1] + input[ip - 1] + 1) / 2;
          output[op + filter->g_off] = input[ip];
          output[op + filter->b_off] = (input[ip - filter->stride] +
              input[ip + filter->stride] + 1) / 2;
          type = RED;
          break;
        case GREENB:
          output[op + filter->r_off] = (input[ip - filter->stride] +
              input[ip + filter->stride] + 1) / 2;
          output[op + filter->g_off] = input[ip];
          output[op + filter->b_off] = (input[ip + 1] + input[ip - 1] + 1) / 2;
          type = BLUE;
          break;
        case BLUE:
          output[op + filter->r_off] = (input[ip - filter->stride - 1] +
              input[ip - filter->stride + 1] +
              input[ip + filter->stride - 1] +
              input[ip + filter->stride + 1] + 2) / 4;
          output[op + filter->b_off] = input[ip];
          v1 = input[ip + filter->stride];
          v2 = input[ip - filter->stride];
          h1 = input[ip + 1];
          h2 = input[ip - 1];
          a1 = abs (v1 - v2);
          a2 = abs (h1 - h2);
          if (a1 < a2)
            output[op + filter->g_off] = (v1 + v2 + 1) / 2;
          else if (a1 > a2)
            output[op + filter->g_off] = (h1 + h2 + 1) / 2;
          else
            output[op + filter->g_off] = (v1 + h1 + v2 + h2 + 2) / 4;
          type = GREENB;
          break;
      }
    }
  }
}

static GstFlowReturn
gst_bayer2rgb2_transform(GstBaseTransform * pad, GstBuffer *inbuf, GstBuffer *outbuf)
{
    GstBayer2rgb2		*filter;
    uint8_t		        *input;
    uint8_t		        *output;

    filter = GST_BAYER2RGB2(pad);
    input = (uint8_t *) GST_BUFFER_DATA (inbuf);
    output = (uint8_t *) GST_BUFFER_DATA (outbuf);
    if (filter->methode == 0)
    {
        do_corners (input, output, filter);
        do_row0_col0 (input, output, filter);
        do_body (input, output, filter);
    }
    else
    {
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
    }
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

