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
 * This plugin lets you convert raw JP46 frames from Elphel cameras to Bayer data.
 * For information about JP4 and demosaicing, see WhyJP4: http://code.google.com/p/gst-plugins-elphel/wiki/WhyJP4
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-0.10 filesrc location=jp46test.mkv ! decodebin ! ffmpegcolorspace !
 * queue ! jp462bayer threads=2 ! queue ! bayer2rgb2 ! queue ! ffmpegcolorspace ! xvimagesink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define SRC_CAPS "video/x-raw-bayer,format=(string){gbrg, bggr, grbg, rggb}," \
  "width=(int)[1,MAX],height=(int)[1,MAX],framerate=(fraction)[0/1,MAX]"

#define _GNU_SOURCE
#include <sched.h>
#include <gst/gst.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include "gstjp462bayer.h"

enum
{
	PROP_0,
    PROP_THREADS,
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_YUV ("I420") "; ")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SRC_CAPS)
    );

static void gst_jp462bayer_base_init (gpointer gclass);
static void gst_jp462bayer_class_init (GstJP462bayerClass * klass);
static void gst_jp462bayer_init (GstJP462bayer *filter,GstJP462bayerClass *gclass);
static void gst_jp462bayer_finalize (GstJP462bayer * jp462bayer);

static gboolean gst_jp462bayer_get_unit_size (GstBaseTransform * base, GstCaps * caps, guint * size);
static GstCaps *gst_jp462bayer_transform_caps(GstBaseTransform * trans, GstPadDirection direction, GstCaps * caps);
static gboolean gst_jp462bayer_set_caps (GstBaseTransform * btrans, GstCaps * incaps, GstCaps * outcaps);
static GstFlowReturn gst_jp462bayer_transform(GstBaseTransform * pad, GstBuffer *inbuf, GstBuffer *outbuf);


void init_thread(t_thread **thread, int num_thread, GstJP462bayer* filter, int start_hor, int start_vert);

static void gst_jp462bayer_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_jp462bayer_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

GST_BOILERPLATE (GstJP462bayer, gst_jp462bayer, GstBaseTransform,
    GST_TYPE_BASE_TRANSFORM);

/* GObject vmethod implementations */

static void
gst_jp462bayer_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
    "JP46 to Bayer conversion filter",
    "Filter/Effect/Video",
    "Converts raw JP46 data from Elphel cameras to raw Bayer data",
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
    GObjectClass *gobject_class = (GObjectClass *) klass;
    GstBaseTransformClass *trans_class = (GstBaseTransformClass *) klass;

    gobject_class->finalize = (GObjectFinalizeFunc) gst_jp462bayer_finalize;
    gobject_class->set_property = gst_jp462bayer_set_property;
    gobject_class->get_property = gst_jp462bayer_get_property;

    g_object_class_install_property (gobject_class,
      PROP_THREADS, g_param_spec_int ("threads", "threads", "Number of threads used by the plugin  (0 for automatic)", 0,
	  4, 1, G_PARAM_READWRITE));

    trans_class->transform_caps = GST_DEBUG_FUNCPTR (gst_jp462bayer_transform_caps);

    trans_class->get_unit_size =
        GST_DEBUG_FUNCPTR (gst_jp462bayer_get_unit_size);

    trans_class->set_caps =
        GST_DEBUG_FUNCPTR (gst_jp462bayer_set_caps);

    trans_class->transform =
        GST_DEBUG_FUNCPTR (gst_jp462bayer_transform);
}

/*
** Function that calculates image size for bayer2rgb2
*/
static gboolean
gst_jp462bayer_get_unit_size (GstBaseTransform * trans, GstCaps * caps, guint * size)
{
    GstStructure *structure;
    int width;
    int height;

    if ((structure = gst_caps_get_structure (caps, 0)) == NULL)
    {
        GST_ERROR("The structure is empty");
        return FALSE;
    }
    if (gst_structure_get_int (structure, "width", &width) &&
      gst_structure_get_int (structure, "height", &height))
    {
        if (strcmp (gst_structure_get_name (structure), "video/x-raw-bayer"))
                size[0] = GST_ROUND_UP_4 (width) * height * 1.5;
        else
                *size = width * height * 1.5;
        return TRUE;
      }
    return FALSE;
}


/*
**  Function that manage caps
**  This function change width and height for outputcaps
**  And change format from YUV to BAYER
*/
static GstCaps *
gst_jp462bayer_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps)
{
    GstStructure    *structure;
    GstStructure    *newstruct;
    GstCaps         *newcaps = NULL;
    const GValue    *width;
    gchar           *caps_str;
    gboolean        get_size = TRUE;
    static gboolean change_caps = FALSE;
    char            *str;
    int             i;
    static int      compare_size = 0;

    if ((structure = gst_caps_get_structure (caps, 0)) == NULL)
    {
        GST_ERROR("The structure is empty");
        return NULL;
    }
    newcaps = (direction == GST_PAD_SRC) ? gst_caps_new_simple ("video/x-raw-yuv", NULL)
                                         : gst_caps_new_simple ("video/x-raw-bayer", NULL);
    newstruct = gst_caps_get_structure (newcaps, 0);
    if (!change_caps)
    {
        caps_str = gst_caps_to_string(caps);
        str = strstr(caps_str, "width");
        for (i = 0; str[i] != ' '; i++)
            if (str[i] == '[')
                get_size = FALSE;
        if (get_size)
        {
            width = gst_structure_get_value(structure, "width");
            if (direction == GST_PAD_SRC)
                change_caps = compare_size != g_value_get_int(width) ? TRUE : FALSE;
            else
                compare_size = g_value_get_int(width);
        }
    }
    if (!get_size || !change_caps)
    {
        gst_structure_set_value (newstruct, "width",
            gst_structure_get_value (structure, "width"));
        gst_structure_set_value (newstruct, "height",
            gst_structure_get_value (structure, "height"));
    }
    else
    {
        change_caps = TRUE;
        gst_structure_set (newstruct,
      		"width", GST_TYPE_INT_RANGE, 1, G_MAXINT,
      		"height", GST_TYPE_INT_RANGE, 1, G_MAXINT, NULL);
    }
    gst_structure_set_value (newstruct, "framerate",
        gst_structure_get_value (structure, "framerate"));
    GST_DEBUG_OBJECT (newcaps, "transforming caps (into)");
    return newcaps;
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_jp462bayer_init (GstJP462bayer *filter,GstJP462bayerClass *gclass)
{
    gst_base_transform_set_in_place (GST_BASE_TRANSFORM (filter), TRUE);
    filter->threads = 0;
    filter->nb_threads = 1;
}

/*
** Function that free structure when you stop the pipeline
*/
static void
gst_jp462bayer_finalize (GstJP462bayer *filter)
{
    int             i;
    t_thread        *tmp;

    if (filter->threads != NULL)
    {
        for (i = 0, tmp = filter->threads; i < filter->nb_threads; i++)
        {
            tmp = filter->threads->next;
            g_free(filter->threads);
            filter->threads = tmp;
        }
    }
  G_OBJECT_CLASS (parent_class)->finalize (G_OBJECT (filter));
}


/*
** Function that set property for the plugin
*/
static void
gst_jp462bayer_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstJP462bayer *filter = GST_JP462BAYER (object);

  switch (prop_id) {
    case PROP_THREADS:
        filter->nb_threads = g_value_get_int (value);
        break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/*
** Function that get property for the plugin
*/
static void
gst_jp462bayer_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstJP462bayer *filter = GST_JP462BAYER (object);

  switch (prop_id) {
    case PROP_THREADS:
        g_value_set_int (value, filter->nb_threads);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/*
** Function that will detect number core in computer
*/
int detect_number_threads(void)
{
    unsigned int bit;
    int np;

    cpu_set_t p_aff;
    memset( &p_aff, 0, sizeof(p_aff) );
    sched_getaffinity( 0, sizeof(p_aff), &p_aff );
    for( np = 0, bit = 0; bit < sizeof(p_aff); bit++ )
        np += (((uint8_t *)&p_aff)[bit / 8] >> (bit % 8)) & 1;
    return np;
}

/*
** Function that make output caps with the good format and size.
** This function check if you resize image and how much thread will use.
*/
static gboolean
gst_jp462bayer_set_caps (GstBaseTransform *pad, GstCaps * incaps, GstCaps * outcaps)
{
	GstJP462bayer 	*filter;
	GstStructure 	*structure;
	gint 			width_out=0, height_out=0;
	int				i;

	filter = GST_JP462BAYER (pad);
	structure = gst_caps_get_structure (incaps, 0);
	gst_structure_get_int (structure, "width", &filter->width);
	gst_structure_get_int (structure, "height", &filter->height);
	structure = gst_caps_get_structure (outcaps, 0);
	gst_structure_get_int (structure, "width", &width_out);
	gst_structure_get_int (structure, "height", &height_out);
	if 	(filter->width == width_out && filter->height == height_out)
		filter->size = 1;
	else if ((float)filter->width / (float)width_out == 2.0 && (float)filter->height / (float)height_out  == 2.0)
		filter->size = 2;
	else if ((float)filter->width / (float)width_out == 4.0 && (float)filter->height / (float)height_out  == 4.0)
		filter->size = 4;
	else
	{
		GST_ERROR("Output caps resolution has to be divisible by 1, 2, or 4 for fast stream downscaling");
		return FALSE;
	}
    if (filter->size > 1 && filter->nb_threads > 1)
    {
        GST_ERROR("Multithreading currently unimplemented for fast stream downscaling, please use threads=1");
	    return FALSE;
    }
    if (filter->nb_threads == 0)
        filter->nb_threads = detect_number_threads();
    for (i = 0; i < filter->nb_threads; i++)
        init_thread(&filter->threads, i, filter,
            (((i % 2 && filter->nb_threads == 4) || (filter->nb_threads < 4)) ? 0 : 1),
            (((i >= 2) || (i % 2 && filter->nb_threads == 2)) ? 1 : 0));
    return TRUE;
}

/*
** Function that initialise each thread
*/
void init_thread(t_thread **thread, int num_thread, GstJP462bayer* filter, int start_hor, int start_vert)
{
    t_thread	*new_thread;
    t_thread   	*temp;
    float 		size;
    int          i;

    if ((new_thread = g_malloc(sizeof(*new_thread))) == NULL)
    {
        GST_ERROR("Failed to allocate memory");
	    exit(0);
    }
    new_thread->next = 0;
    new_thread->num_thread = num_thread;
    new_thread->nb_threads = filter->nb_threads;
    new_thread->width = filter->width;
    new_thread->height = filter->height;
    new_thread->size = filter->size;
    size = (filter->size == 1) ? 0.5 : (filter->size == 2) ? 1.0 : 2.0;
    new_thread->start_hor = start_hor;
    new_thread->start_vert = start_vert;
    for (i = 0; i < 16; i++)
    {
		new_thread->index1[i] = (i % 2 == 1) ? (new_thread->index1[i - 1] + 8) : (i * size);
		new_thread->index2[i] = new_thread->index1[i] * filter->width;
    }
    if (!*thread)
        *thread = new_thread;
    else
    {
      for (temp = *thread; temp->next; temp = temp->next);
        temp->next = new_thread;
    }
}

/*
** Function that calculate the good coordonate for the resizing
*/
int				get_value(guint32 x, guint32 value1, guint32 value2, guint32 b_of, t_thread *f, guint8 *data)
{
	guint32		value = 0;
	guint8	    k, l, m;

	for (l = 0, m = 0; l < f->size; l++)
		for (k = 0; k < f->size; k++, m++)
				value += data[x + value1 + value2 + k + b_of + f->width * l];
	return value / (f->size << (f->size / 2));
}

/*
**  Function that change pixel coordinates to have bayer image
*/
void *my_thread_process (void *structure)
{
    guint8 				i, j;
	guint32				value;
	guint32				y, x;
	guint32				b_of;
	guint32				h_of;
    t_thread            *f;

    f = structure;
    if (f->nb_threads == 1)
    {
        for (y = 0, b_of = 0, value = 0; y < f->height; y += 16, b_of += f->width << 4)
		    for (x = 0; x < f->width; x += 16)
		    	for (j = f->start_vert, h_of = f->start_vert * f->width; j < (16 / f->size); j += 1, h_of += f->width)
		    		for (i = f->start_hor; i < (16 / f->size); i += ((f->nb_threads >> 2) + 1))
		    		{
		    			value  = (f->size == 1) ? (f->indata[x + f->index1[i] + f->index2[j] + b_of]) :
		    						get_value(x, f->index1[i], f->index2[j], b_of, f, f->indata);
                        f->outdata[(x / f->size) + i + (h_of / f->size) + (b_of / (f->size << f->size / 2))] = value;
                    }
    }
    else
    {
        for (y = 0, b_of = 0, value = 0; y < f->height; y += 16, b_of += f->width << 4)
		    for (x = 0; x < f->width; x += 16)
		    	for (j = f->start_vert, h_of = f->start_vert * f->width; j < 16; j += 2, h_of += f->width)
		    		for (i = f->start_hor; i < 16; i += f->nb_threads / 2)
		    		{
		    			value  = f->indata[x + f->index1[i] + f->index2[j] + b_of];
                        f->outdata[x + i + (h_of * 2) + b_of - (f->width * f->start_vert)] = value;
                    }
    }
    pthread_exit(0);
}

/*
** Function that receive buffer and send the buffer modified
*/
static GstFlowReturn
gst_jp462bayer_transform(GstBaseTransform * pad, GstBuffer *inbuf, GstBuffer *outbuf)
{
	GstJP462bayer		*filter;
    t_thread            *tmp;
    int                 i;
    void                *ret;

    filter = GST_JP462BAYER(pad);
    for (i = 0, tmp = filter->threads; i < filter->nb_threads; i++)
    {
        tmp->indata = inbuf->data;
        tmp->outdata = outbuf->data;
        if (pthread_create (&tmp->th, NULL, my_thread_process, (void*) tmp) < 0)
        {
            GST_ERROR("pthread_create error for thread");
            exit (1);
        }
        (void)pthread_join(tmp->th, &ret);
        tmp = tmp->next;
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
 * exchange the string 'Template jp462bayer' with your jp462bayer description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "jp462bayer",
    "Converts raw JP46 data from Elphel cameras to raw Bayer data",
    jp462bayer_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://code.google.com/p/gst-plugins-elphel/"
)

