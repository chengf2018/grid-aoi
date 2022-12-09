#include "grid-aoi.c"

void test_hashmap() {
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
}

void test_hashmap1() {
	struct hash_map map;
	hash_map_init(&map);
	srand(time(0));
	
	for (int i=0; i<1000000; i++) {
		hash_map_put(&map, rand()%1000000, 1);
	}
	for (int i=0; i<1000000; i++) {
		if (hash_map_get(&map, i) )
			hash_map_remove(&map, i);
	}

	hash_map_clear(&map);
}

void test_aoi() {
	aoi_contextptr context = aoi_new(1000.0f, 1000.0f, 20, 1);

	struct int_valuevec enter_self, enter_other, view_list, leave_other;
	valuevec_init(&enter_self);
	valuevec_init(&enter_other);
    valuevec_init(&view_list);
    valuevec_init(&leave_other);

	aoi_enter(context, 1, 1, 1, 1, 0xFFFF, &enter_self, &enter_other);
	valuevec_clear(&enter_self);
	valuevec_clear(&enter_other);
	aoi_enter(context, 2, 1, 1, 1, 0xFFFF, &enter_self, &enter_other);
	valuevec_clear(&enter_self);
	valuevec_clear(&enter_other);
	aoi_enter(context, 3, 1, 1, 1, 0xFFFF, &enter_self, &enter_other);
	valuevec_clear(&enter_self);
	valuevec_clear(&enter_other);
    aoi_enter(context, 4, 100, 100, 1, 0xFFFF, &enter_self, &enter_other);
    valuevec_clear(&enter_self);
	valuevec_clear(&enter_other);
	aoi_move(context, 1, 100, 100, &enter_self, &enter_other);

    aoi_viewlist(context, 2, &view_list);
    valuevec_clear(&view_list);

    aoi_leave(context, 3, &leave_other);
    aoi_viewlist(context, 2, &view_list);
	aoi_delete(context);
}

int main(int argc, char** argv) {
	//test_hashmap();
    test_aoi();
	//test_hashmap1();
	printf("Finished\n");
}