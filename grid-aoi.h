#ifndef _GRID_AOI_H_
#define _GRID_AOI_H_
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
extern "C"
{
#endif


typedef struct int_valuevec {
	int *values;
	size_t n;
	size_t size;
}*int_valuevecptr;

typedef struct hash_node
{
	void *value;
	int key;
	int next;
}*hash_node_ptr;

typedef struct hash_map {
	struct hash_node *nodes;
	struct hash_node *lastfree;
	size_t size;
	size_t n;
}*hash_map_ptr;

typedef struct aoi_context {
	float width;
	float height;
	int grid_width;
	int view_range;
	int x_grid_count;
	int y_grid_count;
	struct hash_map obj_map;
	struct hash_map grid_map;
}*aoi_contextptr;


void valuevec_init(int_valuevecptr valuevec);
void valuevec_push(int_valuevecptr valuevec, int value);
int valuevec_get(int_valuevecptr valuevec, size_t index);
void valuevec_clear(int_valuevecptr valuevec);
size_t valuevec_count(int_valuevecptr valuevec);
size_t valuevec_size(int_valuevecptr valuevec);
void valuevec_free(int_valuevecptr valuevec);


aoi_contextptr aoi_new(float width, float height, int grid_width, int view_range);
void aoi_delete(aoi_contextptr context);
int aoi_enter(aoi_contextptr context, int id, float x, float y, short layer,
		short view_layer, int_valuevecptr enter_self, int_valuevecptr enter_other);
int aoi_leave(aoi_contextptr context, int id, int_valuevecptr leave_other);
int aoi_move(aoi_contextptr context, int id, float x, float y, int_valuevecptr enter,
			int_valuevecptr leave);
int aoi_viewlist(aoi_contextptr context, int id, int_valuevecptr viewed);


#ifdef __cplusplus
}
#endif

#endif