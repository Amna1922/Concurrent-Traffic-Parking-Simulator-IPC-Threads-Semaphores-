#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include "common.h"

void init_visualization();
void update_display();
void cleanup_visualization();
void *visualization_thread_func(void *arg);

#endif