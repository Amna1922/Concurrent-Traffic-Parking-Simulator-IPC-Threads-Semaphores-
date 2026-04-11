#include "intersection.h"
#include "controller.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Intersection *intersection_f10 = NULL;
Intersection *intersection_f11 = NULL;

Intersection *create_intersection(int intersection_id)
{
    Intersection *intersection = (Intersection *)malloc(sizeof(Intersection));
    if (!intersection)
        return NULL;

    intersection->intersection_id = intersection_id;
    intersection->current_state = RED;
    intersection->current_green_direction = NORTH;
    intersection->emergency_active = 0;
    intersection->emergency_vehicle = NULL;
    intersection->vehicles_waiting_ns = 0;
    intersection->vehicles_waiting_ew = 0;

    pthread_mutex_init(&intersection->intersection_mutex, NULL);
    pthread_cond_init(&intersection->north_south_cond, NULL);
    pthread_cond_init(&intersection->east_west_cond, NULL);

    // Create pipe for IPC
    if (pipe(intersection->pipe_fd) == -1)
    {
        perror("pipe creation failed");
        free(intersection);
        return NULL;
    }

    return intersection;
}

void destroy_intersection(Intersection *intersection)
{
    if (!intersection)
        return;

    close(intersection->pipe_fd[0]);
    close(intersection->pipe_fd[1]);
    pthread_mutex_destroy(&intersection->intersection_mutex);
    pthread_cond_destroy(&intersection->north_south_cond);
    pthread_cond_destroy(&intersection->east_west_cond);
    free(intersection);
}

void request_intersection_crossing(int intersection_id, Vehicle *vehicle)
{
    Intersection *intersection = (intersection_id == INTERSECTION_F10) ? intersection_f10 : intersection_f11;
    if (!intersection || !vehicle)
        return;

    pthread_mutex_lock(&intersection->intersection_mutex);

    // Emergency vehicles get immediate access
    if (vehicle->priority == PRIORITY_HIGHEST)
    {
        emergency_preemption(intersection_id, vehicle);
        pthread_mutex_unlock(&intersection->intersection_mutex);
        return;
    }

    // Check if there's an active emergency
    while (intersection->emergency_active)
    {
        pthread_mutex_lock(&display_mutex);
        printf("[TIME:%ld] Vehicle %d waiting for emergency to clear at intersection %d\n",
               get_current_time(), vehicle->id, intersection_id);
        pthread_mutex_unlock(&display_mutex);

        pthread_cond_wait(&intersection->north_south_cond, &intersection->intersection_mutex);
    }

    // Determine which condition variable to wait on
    if (vehicle->entry_direction == NORTH || vehicle->entry_direction == SOUTH)
    {
        intersection->vehicles_waiting_ns++;

        pthread_mutex_lock(&display_mutex);
        printf("[TIME:%ld] Vehicle %d waiting at intersection %d (NS direction)\n",
               get_current_time(), vehicle->id, intersection_id);
        pthread_mutex_unlock(&display_mutex);

        // Wait for green light in NS direction
        while (intersection->current_state != GREEN ||
               intersection->current_green_direction != NORTH)
        {
            pthread_cond_wait(&intersection->north_south_cond, &intersection->intersection_mutex);
        }

        intersection->vehicles_waiting_ns--;
    }
    else
    {
        intersection->vehicles_waiting_ew++;

        pthread_mutex_lock(&display_mutex);
        printf("[TIME:%ld] Vehicle %d waiting at intersection %d (EW direction)\n",
               get_current_time(), vehicle->id, intersection_id);
        pthread_mutex_unlock(&display_mutex);

        // Wait for green light in EW direction
        while (intersection->current_state != GREEN ||
               intersection->current_green_direction != EAST)
        {
            pthread_cond_wait(&intersection->east_west_cond, &intersection->intersection_mutex);
        }

        intersection->vehicles_waiting_ew--;
    }

    // Vehicle is crossing
    pthread_mutex_lock(&display_mutex);
    printf("[TIME:%ld] Vehicle %d crossing intersection %d from %s turning %s\n",
           get_current_time(), vehicle->id, intersection_id,
           direction_to_string(vehicle->entry_direction),
           turn_to_string(vehicle->intended_turn));
    pthread_mutex_unlock(&display_mutex);

    // Simulate crossing time
    pthread_mutex_unlock(&intersection->intersection_mutex);
    sleep_ms(500); // Crossing time
    pthread_mutex_lock(&intersection->intersection_mutex);

    pthread_mutex_unlock(&intersection->intersection_mutex);
}

void emergency_preemption(int intersection_id, Vehicle *emergency_vehicle)
{
    Intersection *intersection = (intersection_id == INTERSECTION_F10) ? intersection_f10 : intersection_f11;
    if (!intersection)
        return;

    intersection->emergency_active = 1;
    intersection->emergency_vehicle = emergency_vehicle;

    // Change to emergency red state
    change_traffic_light(intersection_id, EMERGENCY_RED, NORTH);

    // Notify other intersection via IPC
    if (intersection_id == INTERSECTION_F10)
    {
        send_emergency_alert(INTERSECTION_F10, INTERSECTION_F11, emergency_vehicle);
    }
    else
    {
        send_emergency_alert(INTERSECTION_F11, INTERSECTION_F10, emergency_vehicle);
    }

    pthread_mutex_lock(&display_mutex);
    printf("[TIME:%ld] EMERGENCY: Vehicle %d (%s) preempting intersection %d\n",
           get_current_time(), emergency_vehicle->id,
           vehicle_type_to_string(emergency_vehicle->type), intersection_id);
    pthread_mutex_unlock(&display_mutex);

    // Clear intersection
    clear_intersection_for_emergency(intersection_id);

    // Let emergency vehicle cross
    pthread_mutex_lock(&display_mutex);
    printf("[TIME:%ld] EMERGENCY: Vehicle %d crossing intersection %d\n",
           get_current_time(), emergency_vehicle->id, intersection_id);
    pthread_mutex_unlock(&display_mutex);

    sleep_ms(200); // Emergency crossing is faster

    // Resume normal operation after emergency
    resume_normal_operation(intersection_id);
}

void clear_intersection_for_emergency(int intersection_id)
{
    Intersection *intersection = (intersection_id == INTERSECTION_F10) ? intersection_f10 : intersection_f11;
    if (!intersection)
        return;

    // Signal all waiting vehicles to clear
    pthread_cond_broadcast(&intersection->north_south_cond);
    pthread_cond_broadcast(&intersection->east_west_cond);

    // Wait a moment for vehicles to clear
    sleep_ms(100);
}

void resume_normal_operation(int intersection_id)
{
    Intersection *intersection = (intersection_id == INTERSECTION_F10) ? intersection_f10 : intersection_f11;
    if (!intersection)
        return;

    intersection->emergency_active = 0;
    intersection->emergency_vehicle = NULL;

    // Return to normal red state
    change_traffic_light(intersection_id, RED, NORTH);

    // Notify waiting vehicles
    pthread_cond_broadcast(&intersection->north_south_cond);
    pthread_cond_broadcast(&intersection->east_west_cond);

    pthread_mutex_lock(&display_mutex);
    printf("[TIME:%ld] Intersection %d resuming normal operation\n",
           get_current_time(), intersection_id);
    pthread_mutex_unlock(&display_mutex);
}

int is_conflict(Direction dir1, TurnDirection turn1, Direction dir2, TurnDirection turn2)
{
    // Simplified conflict detection
    // In real implementation, you'd have a proper conflict matrix
    if (dir1 == dir2)
        return 1; // Same direction

    // Opposite directions always conflict
    if ((dir1 == NORTH && dir2 == SOUTH) || (dir1 == SOUTH && dir2 == NORTH) ||
        (dir1 == EAST && dir2 == WEST) || (dir1 == WEST && dir2 == EAST))
    {
        return 1;
    }

    // Left turns conflict with straight movements from certain directions
    // Add more complex logic as needed

    return 0; // No conflict
}

void change_traffic_light(int intersection_id, TrafficLightState new_state, Direction green_direction)
{
    Intersection *intersection = (intersection_id == INTERSECTION_F10) ? intersection_f10 : intersection_f11;
    if (!intersection)
        return;

    pthread_mutex_lock(&intersection->intersection_mutex);
    intersection->current_state = new_state;
    intersection->current_green_direction = green_direction;

    // Signal appropriate condition variables
    if (new_state == GREEN)
    {
        if (green_direction == NORTH)
        {
            pthread_cond_broadcast(&intersection->north_south_cond);
        }
        else
        {
            pthread_cond_broadcast(&intersection->east_west_cond);
        }
    }

    pthread_mutex_unlock(&intersection->intersection_mutex);

    const char *state_str = "UNKNOWN";
    switch (new_state)
    {
    case RED:
        state_str = "RED";
        break;
    case YELLOW:
        state_str = "YELLOW";
        break;
    case GREEN:
        state_str = "GREEN";
        break;
    case EMERGENCY_RED:
        state_str = "EMERGENCY_RED";
        break;
    }

    pthread_mutex_lock(&display_mutex);
    printf("[TIME:%ld] Intersection %d: Traffic light changed to %s (Direction: %s)\n",
           get_current_time(), intersection_id, state_str, direction_to_string(green_direction));
    pthread_mutex_unlock(&display_mutex);
}