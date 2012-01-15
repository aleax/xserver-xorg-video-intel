#ifndef SNA_DAMAGE_H
#define SNA_DAMAGE_H

#include <regionstr.h>
#include <list.h>

#include "compiler.h"

struct sna_damage {
	BoxRec extents;
	pixman_region16_t region;
	enum sna_damage_mode {
		DAMAGE_ADD = 0,
		DAMAGE_SUBTRACT,
		DAMAGE_ALL,
	} mode;
	int remain, dirty;
	BoxPtr box;
	struct {
		struct list list;
		int size;
		BoxRec box[8];
	} embedded_box;
};

#define DAMAGE_IS_ALL(ptr) (((uintptr_t)(ptr))&1)
#define DAMAGE_MARK_ALL(ptr) ((struct sna_damage *)(((uintptr_t)(ptr))|1))
#define DAMAGE_PTR(ptr) ((struct sna_damage *)(((uintptr_t)(ptr))&~1))

fastcall struct sna_damage *_sna_damage_add(struct sna_damage *damage,
					    RegionPtr region);
static inline void sna_damage_add(struct sna_damage **damage,
				  RegionPtr region)
{
	assert(!DAMAGE_IS_ALL(*damage));
	*damage = _sna_damage_add(*damage, region);
}

fastcall struct sna_damage *_sna_damage_add_box(struct sna_damage *damage,
						const BoxRec *box);
static inline void sna_damage_add_box(struct sna_damage **damage,
				      const BoxRec *box)
{
	assert(!DAMAGE_IS_ALL(*damage));
	*damage = _sna_damage_add_box(*damage, box);
}

struct sna_damage *_sna_damage_add_boxes(struct sna_damage *damage,
					 const BoxRec *box, int n,
					 int16_t dx, int16_t dy);
static inline void sna_damage_add_boxes(struct sna_damage **damage,
					const BoxRec *box, int n,
					int16_t dx, int16_t dy)
{
	assert(!DAMAGE_IS_ALL(*damage));
	*damage = _sna_damage_add_boxes(*damage, box, n, dx, dy);
}

struct sna_damage *_sna_damage_add_rectangles(struct sna_damage *damage,
					      const xRectangle *r, int n,
					      int16_t dx, int16_t dy);
static inline void sna_damage_add_rectangles(struct sna_damage **damage,
					     const xRectangle *r, int n,
					     int16_t dx, int16_t dy)
{
	if (damage) {
		assert(!DAMAGE_IS_ALL(*damage));
		*damage = _sna_damage_add_rectangles(*damage, r, n, dx, dy);
	}
}

struct sna_damage *_sna_damage_add_points(struct sna_damage *damage,
					  const DDXPointRec *p, int n,
					  int16_t dx, int16_t dy);
static inline void sna_damage_add_points(struct sna_damage **damage,
					 const DDXPointRec *p, int n,
					 int16_t dx, int16_t dy)
{
	if (damage) {
		assert(!DAMAGE_IS_ALL(*damage));
		*damage = _sna_damage_add_points(*damage, p, n, dx, dy);
	}
}

struct sna_damage *_sna_damage_is_all(struct sna_damage *damage,
				       int width, int height);
static inline bool sna_damage_is_all(struct sna_damage **_damage,
				     int width, int height)
{
	struct sna_damage *damage = *_damage;

	if (damage == NULL)
		return false;
	if (DAMAGE_IS_ALL(damage))
		return true;

	switch (damage->mode) {
	case DAMAGE_ALL:
		assert(0);
		return true;
	case DAMAGE_SUBTRACT:
		return false;
	default:
	case DAMAGE_ADD:
		if (damage->extents.x2 < width  || damage->extents.x1 > 0)
			return false;
		if (damage->extents.y2 < height || damage->extents.y1 > 0)
			return false;
		damage = _sna_damage_is_all(damage, width, height);
		if (damage->mode == DAMAGE_ALL) {
			*_damage = DAMAGE_MARK_ALL(damage);
			return true;
		} else {
			*_damage = damage;
			return false;
		}
	}
}

struct sna_damage *__sna_damage_all(struct sna_damage *damage,
				    int width, int height);
static inline struct sna_damage *
_sna_damage_all(struct sna_damage *damage,
		int width, int height)
{
	damage = __sna_damage_all(damage, width, height);
	return DAMAGE_MARK_ALL(damage);
}

static inline void sna_damage_all(struct sna_damage **damage,
				  int width, int height)
{
	if (!DAMAGE_IS_ALL(*damage))
		*damage = _sna_damage_all(*damage, width, height);
}

fastcall struct sna_damage *_sna_damage_subtract(struct sna_damage *damage,
						 RegionPtr region);
static inline void sna_damage_subtract(struct sna_damage **damage,
				       RegionPtr region)
{
	*damage = _sna_damage_subtract(DAMAGE_PTR(*damage), region);
}

fastcall struct sna_damage *_sna_damage_subtract_box(struct sna_damage *damage,
						     const BoxRec *box);
static inline void sna_damage_subtract_box(struct sna_damage **damage,
					   const BoxRec *box)
{
	*damage = _sna_damage_subtract_box(DAMAGE_PTR(*damage), box);
}

Bool _sna_damage_intersect(struct sna_damage *damage,
			  RegionPtr region, RegionPtr result);

static inline Bool sna_damage_intersect(struct sna_damage *damage,
					RegionPtr region, RegionPtr result)
{
	assert(damage);
	assert(RegionNotEmpty(region));
	assert(!DAMAGE_IS_ALL(damage));

	return _sna_damage_intersect(damage, region, result);
}

int _sna_damage_contains_box(struct sna_damage *damage,
			     const BoxRec *box);
static inline int sna_damage_contains_box(struct sna_damage *damage,
					  const BoxRec *box)
{
	if (DAMAGE_IS_ALL(damage))
		return PIXMAN_REGION_IN;

	return _sna_damage_contains_box(damage, box);
}
bool _sna_damage_contains_box__no_reduce(const struct sna_damage *damage,
					const BoxRec *box);
static inline bool
sna_damage_contains_box__no_reduce(const struct sna_damage *damage,
				   const BoxRec *box)
{
	assert(!DAMAGE_IS_ALL(damage));
	return _sna_damage_contains_box__no_reduce(damage, box);
}

int _sna_damage_get_boxes(struct sna_damage *damage, BoxPtr *boxes);
static inline int
sna_damage_get_boxes(struct sna_damage *damage, BoxPtr *boxes)
{
	assert(damage);

	if (DAMAGE_IS_ALL(damage)) {
		*boxes = &DAMAGE_PTR(damage)->extents;
		return 1;
	} else
		return _sna_damage_get_boxes(damage, boxes);
}

struct sna_damage *_sna_damage_reduce(struct sna_damage *damage);
static inline void sna_damage_reduce(struct sna_damage **damage)
{
	if (*damage == NULL)
		return;

	if (!DAMAGE_IS_ALL(*damage) && (*damage)->dirty)
		*damage = _sna_damage_reduce(*damage);
}

static inline void sna_damage_reduce_all(struct sna_damage **_damage,
					 int width, int height)
{
	struct sna_damage *damage = *_damage;

	DBG(("%s(width=%d, height=%d)\n", __FUNCTION__, width, height));

	if (damage == NULL || DAMAGE_IS_ALL(damage))
		return;

	if (damage->mode == DAMAGE_ADD &&
	    damage->extents.x1 <= 0 &&
	    damage->extents.y1 <= 0 &&
	    damage->extents.x2 >= width &&
	    damage->extents.y2 >= height) {
		if (damage->dirty) {
			damage = *_damage = _sna_damage_reduce(damage);
			if (damage == NULL)
				return;
		}

		if (damage->region.data == NULL)
			*_damage = _sna_damage_all(damage, width, height);
	}
}

void __sna_damage_destroy(struct sna_damage *damage);
static inline void sna_damage_destroy(struct sna_damage **damage)
{
	if (*damage == NULL)
		return;

	__sna_damage_destroy(DAMAGE_PTR(*damage));
	*damage = NULL;
}

#if DEBUG_DAMAGE && TEST_DAMAGE
void sna_damage_selftest(void);
#else
static inline void sna_damage_selftest(void) {}
#endif

#endif /* SNA_DAMAGE_H */
