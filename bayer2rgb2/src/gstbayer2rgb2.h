/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2009 Anthony Violo <<user@hostname.org>>
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

#ifndef __GST_BAYER2RGB2_H__
#define __GST_BAYER2RGB2_H__

#include <stdint.h>
#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_BAYER2RGB2 \
  (gst_bayer2rgb2_get_type())
#define GST_BAYER2RGB2(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_BAYER2RGB2,GstBayer2rgb2))
#define GST_BAYER2RGB2_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_BAYER2RGB2,GstBayer2rgb2Class))
#define GST_IS_BAYER2RGB2(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_BAYER2RGB2))
#define GST_IS_BAYER2RGB2_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_BAYER2RGB2))

#define DC1394_COLOR_FILTER_MIN DC1394_COLOR_FILTER_RGGB
#define DC1394_COLOR_FILTER_MAX DC1394_COLOR_FILTER_BGGR
#define DC1394_COLOR_FILTER_NUM (DC1394_COLOR_FILTER_MAX - DC1394_COLOR_FILTER_MIN + 1)
#define DC1394_ERROR_MIN DC1394_BASLER_UNKNOWN_SFF_CHUNK
#define DC1394_ERROR_MAX DC1394_SUCCESS
#define DC1394_ERROR_NUM (DC1394_ERROR_MAX-DC1394_ERROR_MIN+1)

typedef struct _GstBayer2rgb2      GstBayer2rgb2;
typedef struct _GstBayer2rgb2Class GstBayer2rgb2Class;


struct		_GstBayer2rgb2
{
  GstBaseTransform element;
  uint8_t	*tmp;
  gboolean	bpp;
  gboolean	header;
  gboolean	gpu;
  guint8      	methode;
  gint32      	format;
  gint32      	width;
  gint32      	height;

    /***** Bayer2rgb variable *****/
  int stride;
  int pixsize;                  /* bytes per pixel */
  int r_off;                    /* offset for red */
  int g_off;                    /* offset for green */
  int b_off;                    /* offset for blue */
};

struct		_GstBayer2rgb2Class
{
  GstBaseTransformClass parent_class;
};

typedef enum
  {
    DC1394_BAYER_METHOD_BILINEAR,
    DC1394_BAYER_METHOD_HQLINEAR,
    DC1394_BAYER_METHOD_DOWNSAMPLE,
    DC1394_BAYER_METHOD_EDGESENSE,
    DC1394_BAYER_METHOD_VNG,
    DC1394_BAYER_METHOD_AHD
  }	dc1394bayer_method_t;

typedef enum
  {
    DC1394_COLOR_FILTER_RGGB = 512,
    DC1394_COLOR_FILTER_GBRG,
    DC1394_COLOR_FILTER_GRBG,
    DC1394_COLOR_FILTER_BGGR
  }	dc1394color_filter_t ;

/**
* Error codes returned by most libdc1394 functions.
*
* General rule: 0 is success, negative denotes a problem.
*/
typedef enum
  {
    DC1394_SUCCESS = 0,
    DC1394_FAILURE = -1,
    DC1394_NOT_A_CAMERA = -2,
    DC1394_FUNCTION_NOT_SUPPORTED = -3,
    DC1394_CAMERA_NOT_INITIALIZED = -4,
    DC1394_MEMORY_ALLOCATION_FAILURE = -5,
    DC1394_TAGGED_REGISTER_NOT_FOUND = -6,
    DC1394_NO_ISO_CHANNEL = -7,
    DC1394_NO_BANDWIDTH = -8,
    DC1394_IOCTL_FAILURE = -9,
    DC1394_CAPTURE_IS_NOT_SET = -10,
    DC1394_CAPTURE_IS_RUNNING = -11,
    DC1394_RAW1394_FAILURE = -12,
    DC1394_FORMAT7_ERROR_FLAG_1 = -13,
    DC1394_FORMAT7_ERROR_FLAG_2 = -14,
    DC1394_INVALID_ARGUMENT_VALUE = -15,
    DC1394_REQ_VALUE_OUTSIDE_RANGE = -16,
    DC1394_INVALID_FEATURE = -17,
    DC1394_INVALID_VIDEO_FORMAT = -18,
    DC1394_INVALID_VIDEO_MODE = -19,
    DC1394_INVALID_FRAMERATE = -20,
    DC1394_INVALID_TRIGGER_MODE = -21,
    DC1394_INVALID_TRIGGER_SOURCE = -22,
    DC1394_INVALID_ISO_SPEED = -23,
    DC1394_INVALID_IIDC_VERSION = -24,
    DC1394_INVALID_COLOR_CODING = -25,
    DC1394_INVALID_COLOR_FILTER = -26,
    DC1394_INVALID_CAPTURE_POLICY = -27,
    DC1394_INVALID_ERROR_CODE = -28,
    DC1394_INVALID_BAYER_METHOD = -29,
    DC1394_INVALID_VIDEO1394_DEVICE = -30,
    DC1394_INVALID_OPERATION_MODE = -31,
    DC1394_INVALID_TRIGGER_POLARITY = -32,
    DC1394_INVALID_FEATURE_MODE = -33,
    DC1394_INVALID_LOG_TYPE = -34,
    DC1394_INVALID_BYTE_ORDER = -35,
    DC1394_INVALID_STEREO_METHOD = -36,
    DC1394_BASLER_NO_MORE_SFF_CHUNKS = -37,
    DC1394_BASLER_CORRUPTED_SFF_CHUNK = -38,
    DC1394_BASLER_UNKNOWN_SFF_CHUNK = -39
  }	dc1394error_t;

typedef enum
  {
    DC1394_FALSE= 0,
    DC1394_TRUE
  }	dc1394bool_t;

dc1394error_t	dc1394_bayer_decoding_8bit
(const uint8_t * bayer,
 uint8_t * rgb, uint32_t sx,
 uint32_t sy, dc1394color_filter_t tile,
 dc1394bayer_method_t method);

dc1394error_t	dc1394_bayer_decoding_16bit
(const uint16_t * bayer,
 uint16_t * rgb, uint32_t sx,
 uint32_t sy, dc1394color_filter_t tile,
 dc1394bayer_method_t method, uint32_t bits);
GType gst_bayer2rgb2_get_type(void);
G_END_DECLS

#endif /* __GST_BAYER2RGB2_H__ */

