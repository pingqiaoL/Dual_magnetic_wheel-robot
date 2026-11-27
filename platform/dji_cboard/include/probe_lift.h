#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int board_probe_lift_initialize(void);
int board_probe_lift_initialize_switch(void);
void board_probe_lift_write(bool in1, bool in2);
bool board_probe_lift_work_pressed(void);

#ifdef __cplusplus
}
#endif
