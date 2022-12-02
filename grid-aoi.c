#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#ifndef unlikely
	#define likely(x) __builtin_expect(!!(x), 1)
	#define unlikely(x) __builtin_expect(!!(x), 0)
#endif
#define MIN_VALUEVEC_SIZE 32
#define HASH_MAP_MINSIZE 32
#define AOI_MAXLAYER 8
#define calchash(key, size) ((size_t)(key)%(size_t)(size))

typedef struct dlink_node {
	struct dlink_node *last;
	struct dlink_node *next;
}*dlink_nodeptr;

typedef struct dlink {
	struct dlink_node head;
	struct dlink_node tail;
}*dlinkptr;

typedef struct int_valuevec {
	int *values;
	size_t n;
	size_t size;
}*int_valuevecptr;

typedef struct aoi_object {
	struct dlink_node node;
	int id;
	short layer;//对象当前的层
	short view_layer;//对象能看到的层,16位
	float x;
	float y;
}*aoi_objectptr;

typedef struct aoi_grid {
	int id;
	dlinkptr layer[AOI_MAXLAYER];
}*aoi_gridptr;

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

/* hash map begin */
static inline int hash_node_isempty(hash_node_ptr node) {
	return node->value == NULL;
}

static void hash_map_init(hash_map_ptr map) {
	memset(map, 0, sizeof(*map));
	map->nodes = malloc(sizeof(*map->nodes) * HASH_MAP_MINSIZE);
	memset(map->nodes, 0, sizeof(*map->nodes) * HASH_MAP_MINSIZE);
	map->size = HASH_MAP_MINSIZE;
	map->lastfree = &map->nodes[HASH_MAP_MINSIZE - 1];
}
static void hash_map_clear(hash_map_ptr map) {
	if (map->nodes) {
		free(map->nodes);
	}
	map->nodes = NULL;
	map->size = map->n = 0;
}
static hash_node_ptr hash_map_getfreepos(hash_map_ptr map) {
	for (int i = 0; i < 2; i++) {
		while (map->lastfree > map->nodes) {
			if (hash_node_isempty(map->lastfree))
				return map->lastfree;
			map->lastfree--;
		}
		map->lastfree = &map->nodes[map->size - 1];//try again
	}
	return NULL;
}
static void hash_map_newkey(hash_map_ptr map, int key, void *value) {
	int hash = calchash(key, map->size);
	hash_node_ptr node = &map->nodes[hash];
	if (!hash_node_isempty(node)) {
		hash_node_ptr newnode = hash_map_getfreepos(map);
		assert(newnode);
		int nodehash = calchash(node->key, map->size);
		if (nodehash == hash) { //main position?
			if (node->next != 0)
				newnode->next = (int)((node->next + node) - newnode);
			node->next = (int)(newnode - node);
			node = newnode;
		}
		else {
			hash_node_ptr othernode = &map->nodes[nodehash];
			while (othernode + othernode->next != node) { //find previous
				othernode += othernode->next;
			}
			othernode->next = newnode - othernode;
			*newnode = *node;
			if (node->next != 0) {
				newnode->next += (int)(node - newnode);
				node->next = 0;
			}
		}
	}
	node->value = value;
	node->key = key;
	map->n++;
}

static void hash_map_rehash(hash_map_ptr map, hash_map_ptr newmap) {
	for (size_t i = 0; i < map->size; i++) {
		hash_node_ptr p = &map->nodes[i];
		if (!hash_node_isempty(p)) {
			hash_map_newkey(newmap, p->key, p->value);
		}
	}
	free(map->nodes);
	map->nodes = newmap->nodes;
	map->size = newmap->size;
}
static void hash_map_reserve(hash_map_ptr map) {
	size_t newsize = map->size;
	if (map->n > HASH_MAP_MINSIZE * 0.5 && map->n * 1.5 > map->size) {
		//expand
		newsize *= 2;
	}
	else if (map->size > HASH_MAP_MINSIZE && map->n * 3 < map->size) {
		//reduce
		newsize *= 0.5;
	}
	if (newsize != map->size) {
		hash_node_ptr newnodes = malloc(sizeof(*map->nodes) * newsize);
		memset(newnodes, 0, sizeof(*newnodes) * newsize);

		struct hash_map newmap;
		hash_map_init(&newmap);
		newmap.nodes = newnodes;
		newmap.size = newsize;
		newmap.lastfree = &newnodes[newsize - 1];
		hash_map_rehash(map, &newmap);
	}
}

static hash_node_ptr hash_map_get(hash_map_ptr map, int key) {
	int hash = calchash(key, map->size);
	hash_node_ptr node = &map->nodes[hash];
	if (hash_node_isempty(node)) {
		return NULL;
	}
	while (node->key != key) {
		if (node->next == 0) return NULL;
		node += node->next;
	}
	return node;
}

static int hash_map_put(hash_map_ptr map, int key, void *value) {
	hash_map_reserve(map);
	hash_node_ptr slot = hash_map_get(map, key);
	if (slot) {
		//slot->value = value;
		return 0;
	}
	hash_map_newkey(map, key, value);
	return 1;
}

static void hash_map_remove(hash_map_ptr map, int key) {
	int hash = calchash(key, map->size);
	hash_node_ptr node = &map->nodes[hash];
	if (hash_node_isempty(node)) {
		return;
	}
	hash_node_ptr previous = NULL;
	while (node->key != key) {
		if (node->next == 0) return;
		previous = node;
		node += node->next;
	}
	if (previous) {
		if (node->next) {
			previous->next = node + node->next - previous;
		} else {
			previous->next = 0;
		}
	} else {
		//main position
		if (node->next) {
			hash_node_ptr nxt = node + node->next;
			*node = *nxt;
			if (nxt->next != 0) {
				node->next = nxt + nxt->next - node;
			}
			node = nxt;
		} 
	}
	node->next = node->key = 0;
	node->value = NULL;
	map->n--;
	if (map->n > 0) hash_map_reserve(map);
}

static hash_node_ptr hash_map_next(hash_map_ptr map, size_t index, size_t *outi) {
	for (size_t i = index; i < map->size; i++) {
		hash_node_ptr node = &map->nodes[i];
		if (!hash_node_isempty(node)) {
			if (outi) *outi = i + 1;
			return node;
		}
	}
	return NULL;
}
/* hash map end */

/* dlink begin */
static dlinkptr dlink_new() {
	dlinkptr link = (dlinkptr)malloc(sizeof(*link));
	memset(link, 0, sizeof(*link));
	link->head.next = &link->tail;
	link->tail.last = &link->head;
	return link;
}

static void dlink_free(dlinkptr link) {
	free(link);
}

static void dlink_add(dlinkptr link, dlink_nodeptr node) {
	node->last = link->tail.last;
	node->next = &link->tail;
	link->tail.last = link->tail.last->next = node;
}

static void dlink_remove(dlink_nodeptr node) {
	node->last->next = node->next;
	node->next->last = node->last;
}
/* dlink end */

/* value vec begin */
static void reservevaluevec(int_valuevecptr valuevec) {
	if (!valuevec->values) {
		int *values = malloc(sizeof(int) * MIN_VALUEVEC_SIZE);
		memset(values, 0, sizeof(int) * MIN_VALUEVEC_SIZE);

		valuevec->values = values;
		valuevec->n = 0;
		valuevec->size = MIN_VALUEVEC_SIZE;
	} else if(valuevec->n + 1 > valuevec->size) {
		size_t oldsize = valuevec->size;
		valuevec->size *= 2;
		int *values = realloc(valuevec->values, sizeof(int) * valuevec->size);
		memset(values + oldsize, 0, sizeof(int) * 
									(valuevec->size - oldsize));
		valuevec->values = values;
	}
}

static inline void valuevec_init(int_valuevecptr valuevec) {
	valuevec->values = NULL;
	valuevec->n = valuevec->size = 0;
}

static void valuevec_push(int_valuevecptr valuevec, int value) {
	reservevaluevec(valuevec);
	valuevec->values[valuevec->n] = value;
	valuevec->n ++;
}
static inline int valuevec_get(int_valuevecptr valuevec, size_t index) {
	return valuevec->values[index];
}

static inline void valuevec_clear(int_valuevecptr valuevec) {
	valuevec->n = 0;
}

static inline size_t valuevec_count(int_valuevecptr valuevec) {
	return valuevec->n;
}

static inline size_t valuevec_size(int_valuevecptr valuevec) {
	return valuevec->size;
}

static void valuevec_free(int_valuevecptr valuevec) {
	if (valuevec->values)
		free(valuevec->values);
	valuevec->values = NULL;
	valuevec->n = valuevec->size = 0;
}
/* valuevec end */

/* aoi module interface begin*/
static int getgridid(aoi_contextptr context, float x, float y) {
	return (int)(y/context->grid_width) * context->x_grid_count + (int)(x/context->grid_width);
}

static inline int calcgridid(int x, int y, int x_grid_count) {
	return y * x_grid_count + x;
}

static void calc_rect(aoi_contextptr context, float x, float y, int* lt_x, int* lt_y,
				int* rb_x, int* rb_y) {
	int xindex = (int)(x / context->grid_width);
	int yindex = (int)(y / context->grid_width);
	*lt_y = yindex - context->view_range;
	*rb_y = yindex + context->view_range;
	*lt_x = xindex - context->view_range;
	*rb_x = xindex + context->view_range;
	if (*lt_y < 0) *lt_y = 0;
	if (*lt_x < 0) *lt_x = 0;
	if (*rb_y > context->y_grid_count) *rb_y = context->y_grid_count;
	if (*rb_x > context->x_grid_count) *rb_x = context->x_grid_count;
}

static aoi_objectptr newobj(int id, short layer, short view_layer, float x, float y) {
	aoi_objectptr obj = (aoi_objectptr)malloc(sizeof(*obj));
	memset(obj, 0, sizeof(*obj));
	obj->id = id;
	obj->layer = layer;
	obj->view_layer = view_layer;
	obj->x = x;
	obj->y = y;
	return obj;
}

static void freeobj(aoi_objectptr obj) {
	free(obj);
}

static aoi_gridptr newgrid(int id) {
	aoi_gridptr grid = (aoi_gridptr)malloc(sizeof(*grid));
	memset(grid, 0, sizeof(*grid));
	grid->id = id;
	return grid;
}

static void freegrid(aoi_gridptr grid) {
	for (int i=0; i<AOI_MAXLAYER; i++) {
		if (grid->layer[i]) {
			free(grid->layer[i]);
		}
	}
	free(grid);
}

static void grid_push(aoi_contextptr context, int gridid, aoi_objectptr obj) {
	hash_node_ptr gridnode = hash_map_get(&context->grid_map, gridid);
	aoi_gridptr grid;
	if (gridnode) {
		grid = (aoi_gridptr)gridnode->value;
	} else {
		grid = newgrid(gridid);
		hash_map_put(&context->grid_map, gridid, grid);
	}
	if (!grid->layer[obj->layer]) {
		grid->layer[obj->layer] = dlink_new();
	}
	dlink_add(grid->layer[obj->layer], &obj->node);
}

static void grid_pop(aoi_contextptr context, int gridid, aoi_objectptr obj) {
	hash_node_ptr gridnode = hash_map_get(&context->grid_map, gridid);
	if (gridnode) {
		aoi_gridptr grid = (aoi_gridptr)gridnode->value;
		if (grid->layer[obj->layer]) {
			dlink_remove(&obj->node);
		}
	}
}

static void grid_viewlist(aoi_gridptr grid, aoi_objectptr obj, int_valuevecptr outvec) {
	for (int layer=0; layer < AOI_MAXLAYER; layer++) {
		if (obj->view_layer & (1<<layer)) {
			dlinkptr link = grid->layer[layer];
			if (link) {
				aoi_objectptr p = (aoi_objectptr)link->head.next;
				while (p != (aoi_objectptr)&link->tail) {
					if (p != obj) {
						valuevec_push(outvec, p->id);
					}
					p = (aoi_objectptr)p->node.next;
				}
			}
		}
	}
}

static void grid_viewedlist(aoi_gridptr grid, aoi_objectptr obj, int_valuevecptr outvec) {
	for (int layer=0; layer < AOI_MAXLAYER; layer++) {
		dlinkptr link = grid->layer[layer];
		if (link) {
			aoi_objectptr p = (aoi_objectptr)link->head.next;
			while (p != (aoi_objectptr)&link->tail) {
				if (p != obj) {
					if (p->view_layer & (1<<obj->layer)) {
						valuevec_push(outvec, p->id);
					}
				}
				p = (aoi_objectptr)p->node.next;
			}
		}
	}
}

aoi_contextptr aoi_new(float width, float height, int grid_width, int view_range) {
	aoi_contextptr newcontext = (aoi_contextptr)malloc(sizeof(*newcontext));
	memset(newcontext, 0, sizeof(*newcontext));
	if (grid_width <= 0 || width <= 0 || height <= 0) return NULL;

	newcontext->width = width;
	newcontext->height = height;
	newcontext->grid_width = grid_width;
	newcontext->view_range = view_range;
	newcontext->x_grid_count = (int)(width / grid_width) + 1;
	newcontext->y_grid_count = (int)(height / grid_width) + 1;
	hash_map_init(&newcontext->obj_map);
	hash_map_init(&newcontext->grid_map);
	return newcontext;
}

void aoi_delete(aoi_contextptr context) {
	if (!context) return;
	size_t index = 0;
	hash_map_ptr map = &context->obj_map;
	hash_node_ptr node = hash_map_next(map, 0, &index);
	while (node) {
		aoi_objectptr obj = (aoi_objectptr)node->value;
		freeobj(obj);
		node = hash_map_next(map, index, &index);
	}
	hash_map_clear(map);

	map = &context->grid_map;
	node = hash_map_next(map, 0, &index);
	while (node) {
		aoi_gridptr grid = (aoi_gridptr)node->value;
		for (int i=0; i<AOI_MAXLAYER; i++) {
			if (grid->layer[i]) {
				dlink_free(grid->layer[i]);
			}
		}
		freegrid(grid);
		node = hash_map_next(map, index, &index);
	}

	free(context);
}

int aoi_enter(aoi_contextptr context, int id, float x, float y, short layer,
		short view_layer, int_valuevecptr enter_self, int_valuevecptr enter_other) {
	if (unlikely(x < 0.0f || y < 0.0f || x > context->width || y > context->height)) {
		return 1;
	}
	aoi_objectptr obj = newobj(id, layer, view_layer, x, y);
	if(unlikely(hash_map_put(&context->obj_map, id, obj) == 0)) {//already existed
		freeobj(obj);
		return 2;
	}
	//calc rect
	int lt_x, lt_y, rb_x, rb_y;
	calc_rect(context, x, y, &lt_x, &lt_y, &rb_x, &rb_y);

	//view list and viewed list
	for (int y = lt_y; y <= rb_y; y++) {
		for (int x = lt_x; x <= rb_x; x++) {
			int gridid = calcgridid(x, y, context->x_grid_count);
			hash_node_ptr gridnode = hash_map_get(&context->grid_map, gridid);
			if (gridnode) {
				aoi_gridptr grid = (aoi_gridptr)gridnode->value;
				grid_viewlist(grid, obj, enter_self);
				grid_viewedlist(grid, obj, enter_other);
			}
		}
	}
	
	//push grid
	int gridid = getgridid(context, x, y);
	grid_push(context, gridid, obj);
	return 0;
}

int aoi_leave(aoi_contextptr context, int id, int_valuevecptr leave_other) {
	hash_node_ptr objnode = hash_map_get(&context->obj_map, id);
	if (unlikely(objnode == NULL)) {
		return 1;
	}
	aoi_objectptr obj = (aoi_objectptr)objnode->value;
	//pop grid
	int gridid = getgridid(context, obj->x, obj->y);
	grid_pop(context, gridid, obj);
	//calc rect
	int lt_x, lt_y, rb_x, rb_y;
	calc_rect(context, obj->x, obj->y, &lt_x, &lt_y, &rb_x, &rb_y);

	//leave view list
	for (int y = lt_y; y <= rb_y; y++) {
		for (int x = lt_x; x <= rb_x; x++) {
			int gridid = calcgridid(x, y, context->x_grid_count);
			hash_node_ptr gridnode = hash_map_get(&context->grid_map, gridid);
			if (gridnode) {
				aoi_gridptr grid = (aoi_gridptr)gridnode->value;
				grid_viewedlist(grid, obj, leave_other);
			}
		}
	}
	
	freeobj(obj);
	return 0;
}

int aoi_move(aoi_contextptr context, int id, float x, float y, int_valuevecptr enter,
			int_valuevecptr leave) {
	hash_node_ptr objnode = hash_map_get(&context->obj_map, id);
	if (unlikely(objnode == NULL)) {
		return 1;
	}
	aoi_objectptr obj = (aoi_objectptr)objnode->value;

	int ogridid = getgridid(context, obj->x, obj->y);
	int ngridid = getgridid(context, x, y);
	if (ogridid == ngridid) {
		return 0;
	}
	
	grid_pop(context, ogridid, obj);

	int olt_x, olt_y, orb_x, orb_y;
	int nlt_x, nlt_y, nrb_x, nrb_y;
	calc_rect(context, obj->x, obj->y, &olt_x, &olt_y, &orb_x, &orb_y);
	calc_rect(context, x, y, &nlt_x, &nlt_y, &nrb_x, &nrb_y);
	obj->x = x;
	obj->y = y;

	//leave
	for (int y = olt_y; y <= orb_y; y++) {
		for (int x = olt_x; x <= orb_x; x++) {
			if (x >= nlt_x && x <= nrb_x && y >= nlt_y && y <= nrb_y)
				continue;
			int gridid = calcgridid(x, y, context->x_grid_count);
			hash_node_ptr gridnode = hash_map_get(&context->grid_map, gridid);
			if (gridnode) {
				aoi_gridptr grid = (aoi_gridptr)gridnode->value;
				grid_viewedlist(grid, obj, leave);
			}
		}
	}
	//enter
	for (int y = nlt_y; y <= nrb_y; y++) {
		for (int x = nlt_x; x <= nrb_x; x++) {
			if (x >= olt_x && x <= orb_x && y >= olt_y && y <= orb_y)
				continue;
			int gridid = calcgridid(x, y, context->x_grid_count);
			hash_node_ptr gridnode = hash_map_get(&context->grid_map, gridid);
			if (gridnode) {
				aoi_gridptr grid = (aoi_gridptr)gridnode->value;
				grid_viewlist(grid, obj, enter);
			}
		}
	}
	grid_push(context, ngridid, obj);
	return 0;
}

int aoi_viewlist(aoi_contextptr context, int id, int_valuevecptr viewed) {
	hash_node_ptr objnode = hash_map_get(&context->obj_map, id);
	if (!objnode) {
		return 1;
	}
	aoi_objectptr obj = (aoi_objectptr)objnode->value;
	int lt_x, lt_y, rb_x, rb_y;
	calc_rect(context, obj->x, obj->y, &lt_x, &lt_y, &rb_x, &rb_y);

	//leave view list
	for (int y = lt_y; y <= rb_y; y++) {
		for (int x = lt_x; x <= rb_x; x++) {
			int gridid = calcgridid(x, y, context->x_grid_count);
			hash_node_ptr gridnode = hash_map_get(&context->grid_map, gridid);
			if (gridnode) {
				aoi_gridptr grid = (aoi_gridptr)gridnode->value;
				grid_viewedlist(grid, obj, viewed);
			}
		}
	}
	return 0;
}
/* aoi module interface end*/