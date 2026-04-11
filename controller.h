#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "common.h"
#include "vehicle.h"

// Controller process functions
void start_controller_process(int intersection_id);
void *controller_thread_func(void *arg);
void spawn_vehicle_at_intersection(int intersection_id);
void handle_ipc_messages(int intersection_id);
void send_emergency_alert(int from_intersection, int to_intersection, Vehicle *emergency_vehicle);
void receive_emergency_alert(int intersection_id);
void traffic_light_cycle(int intersection_id);

#endif