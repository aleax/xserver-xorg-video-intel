/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
Copyright © 2002 David Dawes

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   David Dawes <dawes@xfree86.org>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>

#ifndef _SNA_H_
#define _SNA_H_

#include "xf86_OSproc.h"
#include "compiler.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "xf86Cursor.h"
#include "xf86xv.h"
#include "vgaHW.h"
#include "xf86Crtc.h"
#include "xf86RandR12.h"

#include "xorg-server.h"
#include <pciaccess.h>

#include "xf86drm.h"
#include "xf86drmMode.h"

#define _XF86DRI_SERVER_
#include "dri.h"
#include "dri2.h"
#include "i915_drm.h"

#if HAVE_UDEV
#include <libudev.h>
#endif

#define DBG(x)

#define DEBUG_ALL 0
#define DEBUG_ACCEL (DEBUG_ALL || 0)
#define DEBUG_BATCH (DEBUG_ALL || 0)
#define DEBUG_BLT (DEBUG_ALL || 0)
#define DEBUG_COMPOSITE (DEBUG_ALL || 0)
#define DEBUG_DAMAGE (DEBUG_ALL || 0)
#define DEBUG_DISPLAY (DEBUG_ALL || 0)
#define DEBUG_DRI (DEBUG_ALL || 0)
#define DEBUG_GRADIENT (DEBUG_ALL || 0)
#define DEBUG_GLYPHS (DEBUG_ALL || 0)
#define DEBUG_IO (DEBUG_ALL || 0)
#define DEBUG_KGEM (DEBUG_ALL || 0)
#define DEBUG_RENDER (DEBUG_ALL || 0)
#define DEBUG_STREAM (DEBUG_ALL || 0)
#define DEBUG_TRAPEZOIDS (DEBUG_ALL || 0)
#define DEBUG_VIDEO (DEBUG_ALL || 0)
#define DEBUG_VIDEO_TEXTURED (DEBUG_ALL || 0)
#define DEBUG_VIDEO_OVERLAY (DEBUG_ALL || 0)

#define DEBUG_NO_RENDER 0
#define DEBUG_NO_BLT 0
#define DEBUG_NO_IO 0

#define DEBUG_FLUSH_CACHE 0
#define DEBUG_FLUSH_BATCH 0
#define DEBUG_FLUSH_SYNC 0

#define TEST_ALL 0
#define TEST_ACCEL (TEST_ALL || 0)
#define TEST_BATCH (TEST_ALL || 0)
#define TEST_BLT (TEST_ALL || 0)
#define TEST_COMPOSITE (TEST_ALL || 0)
#define TEST_DAMAGE (TEST_ALL || 0)
#define TEST_GRADIENT (TEST_ALL || 0)
#define TEST_GLYPHS (TEST_ALL || 0)
#define TEST_IO (TEST_ALL || 0)
#define TEST_KGEM (TEST_ALL || 0)
#define TEST_RENDER (TEST_ALL || 0)

#include "intel_driver.h"
#include "kgem.h"
#include "sna_damage.h"
#include "sna_render.h"

static inline void list_add_tail(struct list *new, struct list *head)
{
	__list_add(new, head->prev, head);
}

enum DRI2FrameEventType {
	DRI2_SWAP,
	DRI2_ASYNC_SWAP,
	DRI2_FLIP,
	DRI2_WAITMSC,
};

#ifndef CREATE_PIXMAP_USAGE_SCRATCH_HEADER
#define CREATE_PIXMAP_USAGE_SCRATCH_HEADER -1
#endif

typedef struct _DRI2FrameEvent {
	XID drawable_id;
	XID client_id;	/* fake client ID to track client destruction */
	ClientPtr client;
	enum DRI2FrameEventType type;
	int frame;
	int pipe;

	/* for swaps & flips only */
	DRI2SwapEventPtr event_complete;
	void *event_data;
	DRI2BufferPtr front;
	DRI2BufferPtr back;
} DRI2FrameEventRec, *DRI2FrameEventPtr;

#define SNA_CURSOR_X			64
#define SNA_CURSOR_Y			SNA_CURSOR_X

struct sna_pixmap {
	PixmapPtr pixmap;
	struct kgem_bo *gpu_bo, *cpu_bo;
	struct sna_damage *gpu_damage, *cpu_damage;

	struct list list;

#define SOURCE_BIAS 4
	uint16_t source_count;
	uint8_t mapped :1;
	uint8_t pinned :1;
	uint8_t gpu_only :1;
	uint8_t flush :1;
};

static inline PixmapPtr get_drawable_pixmap(DrawablePtr drawable)
{
	ScreenPtr screen = drawable->pScreen;

	if (drawable->type == DRAWABLE_PIXMAP)
		return (PixmapPtr)drawable;
	else
		return screen->GetWindowPixmap((WindowPtr)drawable);
}

extern DevPrivateKeyRec sna_pixmap_index;

static inline struct sna_pixmap *sna_pixmap(PixmapPtr pixmap)
{
	return dixGetPrivate(&pixmap->devPrivates, &sna_pixmap_index);
}

static inline struct sna_pixmap *sna_pixmap_from_drawable(DrawablePtr drawable)
{
	return sna_pixmap(get_drawable_pixmap(drawable));
}

static inline void sna_set_pixmap(PixmapPtr pixmap, struct sna_pixmap *sna)
{
	dixSetPrivate(&pixmap->devPrivates, &sna_pixmap_index, sna);
}

enum {
	OPTION_TILING_FB,
	OPTION_TILING_2D,
	OPTION_PREFER_OVERLAY,
	OPTION_COLOR_KEY,
	OPTION_VIDEO_KEY,
	OPTION_SWAPBUFFERS_WAIT,
	OPTION_HOTPLUG,
	OPTION_THROTTLE,
	OPTION_RELAXED_FENCING,
	OPTION_VMAP,
	OPTION_ZAPHOD,
};

enum {
	FLUSH_TIMER = 0,
	EXPIRE_TIMER,
	NUM_TIMERS
};

struct sna {
	ScrnInfoPtr scrn;

	unsigned flags;
#define SNA_NO_THROTTLE		0x1
#define SNA_SWAP_WAIT		0x2

	int timer[NUM_TIMERS];
	int timer_active;

	struct list deferred_free;
	struct list dirty_pixmaps;

	PixmapPtr front, shadow;

	struct sna_mode {
		uint32_t fb_id;
		drmModeResPtr mode_res;
		int cpp;

		drmEventContext event_context;
		DRI2FrameEventPtr flip_info;
		int flip_count;
		int flip_pending[2];
		unsigned int fe_frame;
		unsigned int fe_tv_sec;
		unsigned int fe_tv_usec;

		struct list outputs;
		struct list crtcs;
	} mode;

	unsigned int tiling;
#define SNA_TILING_FB		0x1
#define SNA_TILING_2D		0x2
#define SNA_TILING_3D		0x4
#define SNA_TILING_ALL (~0)

	int Chipset;
	EntityInfoPtr pEnt;
	struct pci_device *PciInfo;
	struct intel_chipset chipset;

	ScreenBlockHandlerProcPtr BlockHandler;
	ScreenWakeupHandlerProcPtr WakeupHandler;
	CloseScreenProcPtr CloseScreen;

	union {
		struct gen2_render_state gen2;
		struct gen3_render_state gen3;
		struct gen4_render_state gen4;
		struct gen5_render_state gen5;
		struct gen6_render_state gen6;
	} render_state;
	uint32_t have_render;

	Bool directRenderingOpen;
	char *deviceName;

	/* Broken-out options. */
	OptionInfoPtr Options;

	/* Driver phase/state information */
	Bool suspended;

#if HAVE_UDEV
	struct udev_monitor *uevent_monitor;
	InputHandlerProc uevent_handler;
#endif

	struct kgem kgem;
	struct sna_render render;
};

Bool sna_mode_pre_init(ScrnInfoPtr scrn, struct sna *sna);
extern void sna_mode_init(struct sna *sna);
extern void sna_mode_remove_fb(struct sna *sna);
extern void sna_mode_fini(struct sna *sna);

extern int sna_crtc_id(xf86CrtcPtr crtc);
extern int sna_output_dpms_status(xf86OutputPtr output);

extern Bool sna_do_pageflip(struct sna *sna,
			    PixmapPtr pixmap,
			    DRI2FrameEventPtr flip_info, int ref_crtc_hw_id);

static inline struct sna *
to_sna(ScrnInfoPtr scrn)
{
	return (struct sna *)(scrn->driverPrivate);
}

static inline struct sna *
to_sna_from_screen(ScreenPtr screen)
{
	return to_sna(xf86Screens[screen->myNum]);
}

static inline struct sna *
to_sna_from_drawable(DrawablePtr drawable)
{
	return to_sna_from_screen(drawable->pScreen);
}

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif
#define ALIGN(i,m)	(((i) + (m) - 1) & ~((m) - 1))
#define MIN(a,b)	((a) < (b) ? (a) : (b))

extern xf86CrtcPtr sna_covering_crtc(ScrnInfoPtr scrn, BoxPtr box,
				      xf86CrtcPtr desired, BoxPtr crtc_box_ret);

extern bool sna_wait_for_scanline(struct sna *sna, PixmapPtr pixmap,
				  xf86CrtcPtr crtc, RegionPtr clip);

Bool sna_dri2_open(struct sna *sna, ScreenPtr pScreen);
void sna_dri2_close(struct sna *sna, ScreenPtr pScreen);
void sna_dri2_frame_event(unsigned int frame, unsigned int tv_sec,
			  unsigned int tv_usec, DRI2FrameEventPtr flip_info);
void sna_dri2_flip_event(unsigned int frame, unsigned int tv_sec,
			 unsigned int tv_usec, DRI2FrameEventPtr flip_info);

extern Bool sna_crtc_on(xf86CrtcPtr crtc);
int sna_crtc_to_pipe(xf86CrtcPtr crtc);

/* sna_render.c */
void sna_kgem_reset(struct kgem *kgem);
void sna_kgem_flush(struct kgem *kgem);
void sna_kgem_context_switch(struct kgem *kgem, int new_mode);

CARD32 sna_format_for_depth(int depth);

void sna_debug_flush(struct sna *sna);

static inline void
get_drawable_deltas(DrawablePtr drawable, PixmapPtr pixmap, int16_t *x, int16_t *y)
{
#ifdef COMPOSITE
	if (drawable->type == DRAWABLE_WINDOW) {
		*x = -pixmap->screen_x;
		*y = -pixmap->screen_y;
		return;
	}
#endif
	*x = *y = 0;
}

static inline int
get_drawable_dx(DrawablePtr drawable)
{
#ifdef COMPOSITE
	if (drawable->type == DRAWABLE_WINDOW)
		return -get_drawable_pixmap(drawable)->screen_x;
#endif
	return 0;
}

static inline int
get_drawable_dy(DrawablePtr drawable)
{
#ifdef COMPOSITE
	if (drawable->type == DRAWABLE_WINDOW)
		return -get_drawable_pixmap(drawable)->screen_y;
#endif
	return 0;
}

static inline Bool pixmap_is_scanout(PixmapPtr pixmap)
{
	ScreenPtr screen = pixmap->drawable.pScreen;
	return pixmap == screen->GetScreenPixmap(screen);
}

struct sna_pixmap *sna_pixmap_attach(PixmapPtr pixmap);

PixmapPtr sna_pixmap_create_upload(ScreenPtr screen,
				   int width, int height, int depth);

void sna_pixmap_move_to_cpu(PixmapPtr pixmap, bool write);
struct sna_pixmap *sna_pixmap_move_to_gpu(PixmapPtr pixmap);
struct sna_pixmap *sna_pixmap_force_to_gpu(PixmapPtr pixmap);

void
sna_drawable_move_region_to_cpu(DrawablePtr drawable,
				RegionPtr region,
				Bool write);

static inline void
sna_drawable_move_to_cpu(DrawablePtr drawable, bool write)
{
	RegionRec region;

	pixman_region_init_rect(&region,
				0, 0, drawable->width, drawable->height);
	sna_drawable_move_region_to_cpu(drawable, &region, write);
}

static inline Bool
sna_drawable_move_to_gpu(DrawablePtr drawable)
{
	return sna_pixmap_move_to_gpu(get_drawable_pixmap(drawable)) != NULL;
}

static inline Bool
sna_pixmap_is_gpu(PixmapPtr pixmap)
{
	struct sna_pixmap *priv = sna_pixmap(pixmap);
	return priv && priv->gpu_bo;
}

static inline struct kgem_bo *sna_pixmap_get_bo(PixmapPtr pixmap)
{
	return sna_pixmap(pixmap)->gpu_bo;
}

static inline struct kgem_bo *sna_pixmap_pin(PixmapPtr pixmap)
{
	struct sna_pixmap *priv;

	priv = sna_pixmap_force_to_gpu(pixmap);
	if (!priv)
		return NULL;

	priv->pinned = 1;
	return priv->gpu_bo;
}


static inline Bool
_sna_transform_point(const PictTransform *transform,
		     int64_t x, int64_t y, int64_t result[3])
{
	int j;

	for (j = 0; j < 3; j++)
		result[j] = (transform->matrix[j][0] * x +
			     transform->matrix[j][1] * y +
			     transform->matrix[j][2]);

	return result[2] != 0;
}

static inline void
_sna_get_transformed_coordinates(int x, int y,
				 const PictTransform *transform,
				 float *x_out, float *y_out)
{

	int64_t result[3];

	_sna_transform_point(transform, x, y, result);
	*x_out = result[0] / (double)result[2];
	*y_out = result[1] / (double)result[2];
}

void
sna_get_transformed_coordinates(int x, int y,
			       	const PictTransform *transform,
				float *x_out, float *y_out);

Bool
sna_get_transformed_coordinates_3d(int x, int y,
				   const PictTransform *transform,
				   float *x_out, float *y_out, float *z_out);

Bool sna_transform_is_affine(const PictTransform *t);
Bool sna_transform_is_integer_translation(const PictTransform *t,
					  int16_t *tx, int16_t *ty);
Bool sna_transform_is_translation(const PictTransform *t,
				  pixman_fixed_t *tx, pixman_fixed_t *ty);


static inline uint32_t pixmap_size(PixmapPtr pixmap)
{
	return (pixmap->drawable.height - 1) * pixmap->devKind +
		pixmap->drawable.width * pixmap->drawable.bitsPerPixel/8;
}

static inline struct kgem_bo *pixmap_vmap(struct kgem *kgem, PixmapPtr pixmap)
{
	struct sna_pixmap *priv;

	if (kgem->wedged)
		return NULL;

	priv = sna_pixmap_attach(pixmap);
	if (priv == NULL)
		return NULL;

	if (priv->cpu_bo == NULL) {
		priv->cpu_bo = kgem_create_map(kgem,
					       pixmap->devPrivate.ptr,
					       pixmap_size(pixmap),
					       0);
		if (priv->cpu_bo)
			priv->cpu_bo->pitch = pixmap->devKind;
	}

	return priv->cpu_bo;
}

Bool sna_accel_pre_init(struct sna *sna);
Bool sna_accel_init(ScreenPtr sreen, struct sna *sna);
void sna_accel_block_handler(struct sna *sna);
void sna_accel_wakeup_handler(struct sna *sna);
void sna_accel_close(struct sna *sna);
void sna_accel_free(struct sna *sna);

Bool sna_accel_create(struct sna *sna);
void sna_composite(CARD8 op,
		   PicturePtr src,
		   PicturePtr mask,
		   PicturePtr dst,
		   INT16 src_x,  INT16 src_y,
		   INT16 mask_x, INT16 mask_y,
		   INT16 dst_x,  INT16 dst_y,
		   CARD16 width, CARD16 height);
void sna_composite_rectangles(CARD8		 op,
			      PicturePtr		 dst,
			      xRenderColor	*color,
			      int			 num_rects,
			      xRectangle		*rects);
void sna_composite_trapezoids(CARD8 op,
			      PicturePtr src,
			      PicturePtr dst,
			      PictFormatPtr maskFormat,
			      INT16 xSrc, INT16 ySrc,
			      int ntrap, xTrapezoid *traps);

Bool sna_gradients_create(struct sna *sna);
void sna_gradients_close(struct sna *sna);

Bool sna_glyphs_init(ScreenPtr screen);
Bool sna_glyphs_create(struct sna *sna);
void sna_glyphs(CARD8 op,
		PicturePtr src,
		PicturePtr dst,
		PictFormatPtr mask,
		INT16 xSrc, INT16 ySrc,
		int nlist,
		GlyphListPtr list,
		GlyphPtr *glyphs);
void sna_glyph_unrealize(ScreenPtr screen, GlyphPtr glyph);
void sna_glyphs_close(struct sna *sna);

void sna_read_boxes(struct sna *sna,
		    struct kgem_bo *src_bo, int16_t src_dx, int16_t src_dy,
		    PixmapPtr dst, int16_t dst_dx, int16_t dst_dy,
		    const BoxRec *box, int n);
void sna_write_boxes(struct sna *sna,
		     struct kgem_bo *dst_bo, int16_t dst_dx, int16_t dst_dy,
		     const void *src, int stride, int bpp, int16_t src_dx, int16_t src_dy,
		     const BoxRec *box, int n);

struct kgem_bo *sna_replace(struct sna *sna,
			    struct kgem_bo *bo,
			    int width, int height, int bpp,
			    const void *src, int stride);

Bool
sna_compute_composite_extents(BoxPtr extents,
			      PicturePtr src, PicturePtr mask, PicturePtr dst,
			      INT16 src_x,  INT16 src_y,
			      INT16 mask_x, INT16 mask_y,
			      INT16 dst_x,  INT16 dst_y,
			      CARD16 width, CARD16 height);
Bool
sna_compute_composite_region(RegionPtr region,
			     PicturePtr src, PicturePtr mask, PicturePtr dst,
			     INT16 src_x,  INT16 src_y,
			     INT16 mask_x, INT16 mask_y,
			     INT16 dst_x,  INT16 dst_y,
			     CARD16 width, CARD16 height);

void
memcpy_blt(const void *src, void *dst, int bpp,
	   uint16_t src_stride, uint16_t dst_stride,
	   int16_t src_x, int16_t src_y,
	   int16_t dst_x, int16_t dst_y,
	   uint16_t width, uint16_t height);

#define SNA_CREATE_FB 0x10

#endif /* _SNA_H */