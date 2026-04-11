#ifndef VEHICLE_H
#define VEHICLE_H

#include "common.h"

typedef struct Vehicle
{
    int id;
    VehicleType type;
    IntersectionID current_intersection;
    Direction entry_direction;
    TurnDirection intended_turn;
    IntersectionID destination_intersection;
    Direction exit_direction;
    int priority;
    time_t arrival_time;
    int wants_to_park;
    int is_parked;
    int has_parking_queue_slot;
    pthread_t thread_id;
    struct Vehicle *next; // For linked list
} Vehicle;

// Vehicle management functions
Vehicle *create_vehicle(int id, VehicleType type, IntersectionID start_intersection);
void destroy_vehicle(Vehicle *vehicle);
void *vehicle_thread_func(void *arg);
void vehicle_set_destination(Vehicle *vehicle);
int vehicle_should_park(VehicleType type);
Direction get_random_direction();
TurnDirection get_random_turn();

// Vehicle list management (since we can't use vectors)
typedef struct
{
    Vehicle *head;
    Vehicle *tail;
    int count;
    pthread_mutex_t mutex;
} VehicleList;

VehicleList *create_vehicle_list();
void vehicle_list_add(VehicleList *list, Vehicle *vehicle);
void vehicle_list_remove(VehicleList *list, Vehicle *vehicle);
Vehicle *vehicle_list_find(VehicleList *list, int id);
void destroy_vehicle_list(VehicleList *list);

#endif