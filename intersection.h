#ifndef INTERSECTION_H
#define INTERSECTION_H

#include "common.h"
#include "vehicle.h"

// Traffic light states
typedef enum
{
    RED = 0,
    YELLOW,
    GREEN,
    EMERGENCY_RED // Special state for emergency clearance
} TrafficLightState;

typedef struct
{
    int intersection_id;
    TrafficLightState current_state;
    Direction current_green_direction;
    pthread_mutex_t intersection_mutex;
    pthread_cond_t north_south_cond;
    pthread_cond_t east_west_cond;
    int vehicles_waiting_ns;
    int vehicles_waiting_ew;
    int emergency_active;
    Vehicle *emergency_vehicle;
    int pipe_fd[2]; // For IPC with other intersection
} Intersection;

// Intersection management
Intersection *create_intersection(int intersection_id);
void destroy_intersection(Intersection *intersection);
void request_intersection_crossing(int intersection_id, Vehicle *vehicle);
void emergency_preemption(int intersection_id, Vehicle *emergency_vehicle);
void clear_intersection_for_emergency(int intersection_id);
void resume_normal_operation(int intersection_id);
int is_conflict(Direction dir1, TurnDirection turn1, Direction dir2, TurnDirection turn2);
void change_traffic_light(int intersection_id, TrafficLightState new_state, Direction green_direction);

// Global intersections
extern Intersection *intersection_f10;
extern Intersection *intersection_f11;

#endif