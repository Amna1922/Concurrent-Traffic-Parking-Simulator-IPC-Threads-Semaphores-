#ifndef PARKING_H
#define PARKING_H

#include "common.h"
#include "vehicle.h"

typedef struct
{
    int intersection_id;
    int total_spots;
    int available_spots;
    int queue_size;
    int queue_available;
    sem_t spots_semaphore;
    sem_t queue_semaphore;
    pthread_mutex_t parking_mutex;
    VehicleList *parked_vehicles;
} ParkingLot;

// Parking lot management
ParkingLot *create_parking_lot(int intersection_id, int total_spots, int queue_size);
void destroy_parking_lot(ParkingLot *lot);
int try_acquire_parking_queue_slot(int intersection_id);
void release_parking_queue_slot(int intersection_id);
int try_acquire_parking_spot(int intersection_id);
void release_parking_spot(int intersection_id);
void park_vehicle(int intersection_id, Vehicle *vehicle);
void leave_parking(int intersection_id, Vehicle *vehicle);
int get_available_spots(int intersection_id);
int get_available_queue_slots(int intersection_id);

// Global parking lots
extern ParkingLot *parking_f10;
extern ParkingLot *parking_f11;

#endif