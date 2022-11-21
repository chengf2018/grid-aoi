#include <stdio.h>
#include <malloc.h>
#include <string.h>

#define HASH_MAP_MINSIZE 32
#define abs(n) ((n)<0?(-n):(n))

typedef struct dlink_node {
	struct dlink_node *last;
	struct dlink_node *next;
}*dlink_nodeptr;

typedef struct dlink {
	struct dlink_node head;
	struct dlink_node tail;
}*dlinkptr;

typedef struct aoi_object {
	struct dlink_node node;
	int id;
	short layer;//对象当前的层
	short view_layer;//对象能看到的层
	float x;
	float y;
}*aoi_objectptr;


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
	int hash = abs(key % map->size);
	hash_node_ptr node = &map->nodes[hash];
	if (!hash_node_isempty(node)) {
		hash_node_ptr newnode = hash_map_getfreepos(map);
		//assert(newnode);
		int nodehash = abs(node->key % map->size);
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
	for (int i = 0; i < map->size; i++) {
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
	if (!map->nodes) {
		map->nodes = malloc(sizeof(*map->nodes) * HASH_MAP_MINSIZE);
		memset(map->nodes, 0, sizeof(*map->nodes) * HASH_MAP_MINSIZE);
		map->size = HASH_MAP_MINSIZE;
		map->lastfree = &map->nodes[HASH_MAP_MINSIZE - 1];
		return;
	}
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
	int hash = abs(key % map->size);
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

static void hash_map_put(hash_map_ptr map, int key, void *value) {
	hash_map_reserve(map);
	hash_node_ptr slot = hash_map_get(map, key);
	if (slot) {
		slot->value = value;
	} else {
		hash_map_newkey(map, key, value);
	}
}
static void hash_map_remove(hash_map_ptr map, int key) {
	int hash = abs(key % map->size);
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

aoi_contextptr aoi_new(float width, float height) {
	aoi_contextptr newcontext = (aoi_contextptr)malloc(sizeof(*newcontext));
	memset(newcontext, 0, sizeof(*newcontext));
	newcontext->width = width;
	newcontext->height = height;
	return newcontext;
}

void aoi_delete(aoi_contextptr context) {
	free(context);
}

int aoi_enter(aoi_contextptr context, int id, float x, float y, int layer, int view_layer) {

}

int aoi_print(aoi_contextptr context) {

}

int aoi_move(aoi_contextptr context, float x, float y) {}
int aoi_leave(aoi_contextptr context) {}


int main(int argc, char** argv) {
	struct hash_map map;
	hash_map_init(&map);

	hash_map_put(&map, 0, "123456");
	hash_map_put(&map, 32, "99999");
	hash_map_put(&map, 64, "666666");
	hash_map_put(&map, 128, "4444444");
	hash_map_put(&map, 256, "333333");
	hash_map_put(&map, 512, "22222");
	hash_map_put(&map, 1024, "111111");
	hash_map_put(&map, 2048, "33453");
	hash_map_put(&map, 30, "000000");

	hash_map_put(&map, 109, "000000");
	hash_map_put(&map, 110, "``````");
	hash_map_put(&map, 111, "``````");
	hash_map_put(&map, 112, "``````");
	hash_map_put(&map, 113, "``````");
	hash_map_put(&map, 114, "``````");
	hash_map_put(&map, 115, "``````");
	hash_map_put(&map, 116, "``````");
	hash_map_put(&map, 117, "``````");
	hash_map_put(&map, 118, "``````");
	hash_map_put(&map, 119, "``````");
	hash_map_put(&map, 120, "``````");

	size_t index = 0;
	hash_node_ptr node = hash_map_next(&map, 0, &index);
	while (node) {
		printf("---%d,%d, %s\n", node->key, index - 1, node->value);
		node = hash_map_next(&map, index, &index);
	}
	printf("--------------------\n");
	hash_map_remove(&map, 0);

	index = 0;
	node = hash_map_next(&map, 0, &index);
	while (node) {
		printf("---%d,%d, %s\n", node->key, index - 1, node->value);
		node = hash_map_next(&map, index, &index);
	}
	printf("--------------------\n");

 	hash_map_remove(&map, 32);


	hash_map_remove(&map, 64);
	hash_map_remove(&map, 128);
	hash_map_remove(&map, 256);
	hash_map_remove(&map, 512);
	hash_map_remove(&map, 1024);
	hash_map_remove(&map, 2048);
	hash_map_remove(&map, 109);
	hash_map_remove(&map, 110);
	hash_map_remove(&map, 111);
	hash_map_remove(&map, 112);
	hash_map_remove(&map, 113);

	hash_map_put(&map, 101, "123456");
	hash_map_put(&map, 102, "99999");
	hash_map_put(&map, 103, "666666");
	hash_map_put(&map, 104, "4444444");
	hash_map_put(&map, 105, "333333");
	hash_map_put(&map, 106, "22222");
	hash_map_put(&map, 107, "111111");


	hash_map_put(&map, 108, "33453");
	hash_map_put(&map, 109, "000000");
	hash_map_put(&map, 110, "``````");
	hash_map_put(&map, 111, "``````");
	hash_map_put(&map, 112, "``````");
	hash_map_put(&map, 113, "``````");
	hash_map_put(&map, 114, "``````");
	hash_map_put(&map, 115, "``````");
	hash_map_put(&map, 116, "``````");
	hash_map_put(&map, 117, "``````");
	hash_map_put(&map, 118, "``````");
	hash_map_put(&map, 119, "``````");
	hash_map_put(&map, 120, "``````");

	index = 0;
	node = hash_map_next(&map, 0, &index);
	while (node) {
		printf("---%d,%d, %s\n", node->key, index-1, node->value);
		node = hash_map_next(&map, index, &index);
	}
	printf("map size:%d, map n:%d\n", map.size, map.n);
	printf("Finished\n");
}