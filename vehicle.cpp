#include "vehicle.h"
#include "intersection.h"
#include "parking.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Global vehicle lists for each intersection
extern VehicleList *f10_vehicles;
extern VehicleList *f11_vehicles;

Vehicle *create_vehicle(int id, VehicleType type, IntersectionID start_intersection)
{
    Vehicle *vehicle = (Vehicle *)malloc(sizeof(Vehicle));
    if (!vehicle)
        return NULL;

    vehicle->id = id;
    vehicle->type = type;
    vehicle->current_intersection = start_intersection;
    vehicle->entry_direction = get_random_direction();
    vehicle->intended_turn = get_random_turn();
    vehicle->destination_intersection = (start_intersection == INTERSECTION_F10) ? INTERSECTION_F11 : INTERSECTION_F10;
    vehicle->priority = get_priority(type);
    vehicle->arrival_time = get_current_time();
    vehicle->wants_to_park = vehicle_should_park(type);
    vehicle->is_parked = 0;
    vehicle->has_parking_queue_slot = 0;
    vehicle->next = NULL;

    // Set exit direction based on entry and turn
    vehicle_set_destination(vehicle);

    return vehicle;
}

void destroy_vehicle(Vehicle *vehicle)
{
    free(vehicle);
}

void vehicle_set_destination(Vehicle *vehicle)
{
    // Simplified: For straight movement, exit opposite to entry
    // For real simulation, you'd implement proper direction logic
    switch (vehicle->entry_direction)
    {
    case NORTH:
        vehicle->exit_direction = SOUTH;
        break;
    case SOUTH:
        vehicle->exit_direction = NORTH;
        break;
    case EAST:
        vehicle->exit_direction = WEST;
        break;
    case WEST:
        vehicle->exit_direction = EAST;
        break;
    default:
        vehicle->exit_direction = SOUTH;
    }
}

int vehicle_should_park(VehicleType type)
{
    // Only some vehicle types attempt to park
    if (type == CAR || type == BIKE || type == TRACTOR || type == BUS)
    {
        // 40% chance to want parking
        return (rand() % 100) < 40;
    }
    return 0;
}

Direction get_random_direction()
{
    return (Direction)(rand() % DIRECTION_COUNT);
}

TurnDirection get_random_turn()
{
    return (TurnDirection)(rand() % 3);
}

void *vehicle_thread_func(void *arg)
{
    Vehicle *vehicle = (Vehicle *)arg;

    pthread_mutex_lock(&display_mutex);
    printf("[TIME:%ld] Vehicle %d (%s) created at intersection %d from %s\n",
           get_current_time(), vehicle->id, vehicle_type_to_string(vehicle->type),
           vehicle->current_intersection, direction_to_string(vehicle->entry_direction));
    pthread_mutex_unlock(&display_mutex);

    while (simulation_running)
    {
        // Attempt to cross intersection
        if (vehicle->current_intersection == INTERSECTION_F10)
        {
            request_intersection_crossing(INTERSECTION_F10, vehicle);
        }
        else
        {
            request_intersection_crossing(INTERSECTION_F11, vehicle);
        }

        // If vehicle wants to park and has reached destination
        if (vehicle->wants_to_park &&
            vehicle->current_intersection == vehicle->destination_intersection)
        {

            // Try to get parking queue slot
            if (vehicle->current_intersection == INTERSECTION_F10)
            {
                vehicle->has_parking_queue_slot = try_acquire_parking_queue_slot(INTERSECTION_F10);
            }
            else
            {
                vehicle->has_parking_queue_slot = try_acquire_parking_queue_slot(INTERSECTION_F11);
            }

            if (vehicle->has_parking_queue_slot)
            {
                // Park the vehicle
                if (vehicle->current_intersection == INTERSECTION_F10)
                {
                    park_vehicle(INTERSECTION_F10, vehicle);
                }
                else
                {
                    park_vehicle(INTERSECTION_F11, vehicle);
                }

                vehicle->is_parked = 1;
                sleep_ms(1000 + (rand() % 3000)); // Park for 1-4 seconds

                // Leave parking
                if (vehicle->current_intersection == INTERSECTION_F10)
                {
                    leave_parking(INTERSECTION_F10, vehicle);
                }
                else
                {
                    leave_parking(INTERSECTION_F11, vehicle);
                }

                vehicle->is_parked = 0;
                vehicle->has_parking_queue_slot = 0;
            }
            else
            {
                // No parking available, circle around
                pthread_mutex_lock(&display_mutex);
                printf("[TIME:%ld] Vehicle %d: No parking available, circling...\n",
                       get_current_time(), vehicle->id);
                pthread_mutex_unlock(&display_mutex);

                sleep_ms(2000); // Wait before retrying
            }
        }

        // Move to other intersection
        if (vehicle->current_intersection == INTERSECTION_F10)
        {
            vehicle->current_intersection = INTERSECTION_F11;
        }
        else
        {
            vehicle->current_intersection = INTERSECTION_F10;
        }

        // Update entry direction for next intersection
        vehicle->entry_direction = get_random_direction();
        vehicle_set_destination(vehicle);

        // Check if we should terminate (reached destination enough times)
        static int crossings = 0;
        crossings++;
        if (crossings > 3)
        {
            break;
        }

        sleep_ms(500); // Travel time between intersections
    }

    pthread_mutex_lock(&display_mutex);
    printf("[TIME:%ld] Vehicle %d finished its journey\n",
           get_current_time(), vehicle->id);
    pthread_mutex_unlock(&display_mutex);

    destroy_vehicle(vehicle);
    return NULL;
}

// Vehicle list implementation
VehicleList *create_vehicle_list()
{
    VehicleList *list = (VehicleList *)malloc(sizeof(VehicleList));
    if (!list)
        return NULL;

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    pthread_mutex_init(&list->mutex, NULL);
    return list;
}

void vehicle_list_add(VehicleList *list, Vehicle *vehicle)
{
    pthread_mutex_lock(&list->mutex);

    vehicle->next = NULL;

    if (list->tail)
    {
        list->tail->next = vehicle;
        list->tail = vehicle;
    }
    else
    {
        list->head = vehicle;
        list->tail = vehicle;
    }

    list->count++;
    pthread_mutex_unlock(&list->mutex);
}

void vehicle_list_remove(VehicleList *list, Vehicle *vehicle)
{
    pthread_mutex_lock(&list->mutex);

    Vehicle *current = list->head;
    Vehicle *prev = NULL;

    while (current)
    {
        if (current == vehicle)
        {
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                list->head = current->next;
            }

            if (current == list->tail)
            {
                list->tail = prev;
            }

            list->count--;
            break;
        }
        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&list->mutex);
}

Vehicle *vehicle_list_find(VehicleList *list, int id)
{
    pthread_mutex_lock(&list->mutex);

    Vehicle *current = list->head;
    while (current)
    {
        if (current->id == id)
        {
            pthread_mutex_unlock(&list->mutex);
            return current;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&list->mutex);
    return NULL;
}

void destroy_vehicle_list(VehicleList *list)
{
    pthread_mutex_lock(&list->mutex);

    Vehicle *current = list->head;
    while (current)
    {
        Vehicle *next = current->next;
        destroy_vehicle(current);
        current = next;
    }

    pthread_mutex_unlock(&list->mutex);
    pthread_mutex_destroy(&list->mutex);
    free(list);
}