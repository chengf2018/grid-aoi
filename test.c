#include "grid-aoi.h"

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
    test_aoi();
	printf("Finished\n");
}