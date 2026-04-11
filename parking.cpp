#include "parking.h"
#include <stdio.h>
#include <stdlib.h>

ParkingLot *parking_f10 = NULL;
ParkingLot *parking_f11 = NULL;

ParkingLot *create_parking_lot(int intersection_id, int total_spots, int queue_size)
{
    ParkingLot *lot = (ParkingLot *)malloc(sizeof(ParkingLot));
    if (!lot)
        return NULL;

    lot->intersection_id = intersection_id;
    lot->total_spots = total_spots;
    lot->available_spots = total_spots;
    lot->queue_size = queue_size;
    lot->queue_available = queue_size;

    sem_init(&lot->spots_semaphore, 0, total_spots);
    sem_init(&lot->queue_semaphore, 0, queue_size);
    pthread_mutex_init(&lot->parking_mutex, NULL);

    lot->parked_vehicles = create_vehicle_list();

    return lot;
}

void destroy_parking_lot(ParkingLot *lot)
{
    if (!lot)
        return;

    sem_destroy(&lot->spots_semaphore);
    sem_destroy(&lot->queue_semaphore);
    pthread_mutex_destroy(&lot->parking_mutex);
    destroy_vehicle_list(lot->parked_vehicles);
    free(lot);
}

int try_acquire_parking_queue_slot(int intersection_id)
{
    ParkingLot *lot = (intersection_id == INTERSECTION_F10) ? parking_f10 : parking_f11;
    if (!lot)
        return 0;

    int result = sem_trywait(&lot->queue_semaphore);
    if (result == 0)
    {
        pthread_mutex_lock(&lot->parking_mutex);
        lot->queue_available--;
        pthread_mutex_unlock(&lot->parking_mutex);
        return 1;
    }
    return 0;
}

void release_parking_queue_slot(int intersection_id)
{
    ParkingLot *lot = (intersection_id == INTERSECTION_F10) ? parking_f10 : parking_f11;
    if (!lot)
        return;

    sem_post(&lot->queue_semaphore);
    pthread_mutex_lock(&lot->parking_mutex);
    lot->queue_available++;
    pthread_mutex_unlock(&lot->parking_mutex);
}

int try_acquire_parking_spot(int intersection_id)
{
    ParkingLot *lot = (intersection_id == INTERSECTION_F10) ? parking_f10 : parking_f11;
    if (!lot)
        return 0;

    int result = sem_trywait(&lot->spots_semaphore);
    if (result == 0)
    {
        pthread_mutex_lock(&lot->parking_mutex);
        lot->available_spots--;
        pthread_mutex_unlock(&lot->parking_mutex);
        return 1;
    }
    return 0;
}

void release_parking_spot(int intersection_id)
{
    ParkingLot *lot = (intersection_id == INTERSECTION_F10) ? parking_f10 : parking_f11;
    if (!lot)
        return;

    sem_post(&lot->spots_semaphore);
    pthread_mutex_lock(&lot->parking_mutex);
    lot->available_spots++;
    pthread_mutex_unlock(&lot->parking_mutex);
}

void park_vehicle(int intersection_id, Vehicle *vehicle)
{
    ParkingLot *lot = (intersection_id == INTERSECTION_F10) ? parking_f10 : parking_f11;
    if (!lot || !vehicle)
        return;

    // First get parking spot
    if (try_acquire_parking_spot(intersection_id))
    {
        // Release queue slot since we got a spot
        release_parking_queue_slot(intersection_id);

        // Add to parked vehicles list
        vehicle_list_add(lot->parked_vehicles, vehicle);

        pthread_mutex_lock(&display_mutex);
        printf("[TIME:%ld] Vehicle %d parked at intersection %d. Spots left: %d\n",
               get_current_time(), vehicle->id, intersection_id, lot->available_spots);
        pthread_mutex_unlock(&display_mutex);
    }
}

void leave_parking(int intersection_id, Vehicle *vehicle)
{
    ParkingLot *lot = (intersection_id == INTERSECTION_F10) ? parking_f10 : parking_f11;
    if (!lot || !vehicle)
        return;

    // Remove from parked vehicles
    vehicle_list_remove(lot->parked_vehicles, vehicle);

    // Release parking spot
    release_parking_spot(intersection_id);

    pthread_mutex_lock(&display_mutex);
    printf("[TIME:%ld] Vehicle %d left parking at intersection %d. Spots available: %d\n",
           get_current_time(), vehicle->id, intersection_id, lot->available_spots);
    pthread_mutex_unlock(&display_mutex);
}

int get_available_spots(int intersection_id)
{
    ParkingLot *lot = (intersection_id == INTERSECTION_F10) ? parking_f10 : parking_f11;
    return lot ? lot->available_spots : 0;
}

int get_available_queue_slots(int intersection_id)
{
    ParkingLot *lot = (intersection_id == INTERSECTION_F10) ? parking_f10 : parking_f11;
    return lot ? lot->queue_available : 0;
}