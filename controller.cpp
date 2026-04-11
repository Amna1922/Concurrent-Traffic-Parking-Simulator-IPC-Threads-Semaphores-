#include "controller.h"
#include "intersection.h"
#include "parking.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Global vehicle counters
int vehicle_counter = 0;
pthread_mutex_t vehicle_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

void start_controller_process(int intersection_id)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork failed");
        exit(1);
    }
    else if (pid == 0)
    { // Child process (controller)
        printf("Controller for intersection %d started (PID: %d)\n",
               intersection_id, getpid());

        // Initialize intersection
        if (intersection_id == INTERSECTION_F10)
        {
            intersection_f10 = create_intersection(INTERSECTION_F10);
        }
        else
        {
            intersection_f11 = create_intersection(INTERSECTION_F11);
        }

        // Initialize parking lot
        if (intersection_id == INTERSECTION_F10)
        {
            parking_f10 = create_parking_lot(INTERSECTION_F10, 10, 5); // 10 spots, queue size 5
        }
        else
        {
            parking_f11 = create_parking_lot(INTERSECTION_F11, 10, 5);
        }

        // Start controller thread
        pthread_t controller_thread;
        pthread_create(&controller_thread, NULL, controller_thread_func, (void *)(long)intersection_id);

        // Handle IPC messages in main thread
        handle_ipc_messages(intersection_id);

        pthread_join(controller_thread, NULL);

        // Cleanup
        if (intersection_id == INTERSECTION_F10)
        {
            destroy_intersection(intersection_f10);
            destroy_parking_lot(parking_f10);
        }
        else
        {
            destroy_intersection(intersection_f11);
            destroy_parking_lot(parking_f11);
        }

        exit(0);
    }
    // Parent continues
}

void *controller_thread_func(void *arg)
{
    int intersection_id = (int)(long)arg;

    // Start traffic light cycle
    traffic_light_cycle(intersection_id);

    // Spawn vehicles periodically
    while (simulation_running)
    {
        spawn_vehicle_at_intersection(intersection_id);
        sleep_ms(2000 + (rand() % 3000)); // Spawn every 2-5 seconds
    }

    return NULL;
}

void spawn_vehicle_at_intersection(int intersection_id)
{
    pthread_mutex_lock(&vehicle_counter_mutex);
    int vehicle_id = ++vehicle_counter;
    pthread_mutex_unlock(&vehicle_counter_mutex);

    // Random vehicle type (with lower probability for emergency vehicles)
    VehicleType type;
    int r = rand() % 100;
    if (r < 5)
    { // 5% chance for ambulance
        type = AMBULANCE;
    }
    else if (r < 10)
    { // 5% chance for firetruck
        type = FIRETRUCK;
    }
    else if (r < 25)
    { // 15% chance for bus
        type = BUS;
    }
    else if (r < 60)
    { // 35% chance for car
        type = CAR;
    }
    else if (r < 85)
    { // 25% chance for bike
        type = BIKE;
    }
    else
    { // 15% chance for tractor
        type = TRACTOR;
    }

    Vehicle *vehicle = create_vehicle(vehicle_id, type, intersection_id);
    if (!vehicle)
        return;

    // Create vehicle thread
    pthread_create(&vehicle->thread_id, NULL, vehicle_thread_func, vehicle);

    // Don't join here - let vehicle thread run independently
    pthread_detach(vehicle->thread_id);
}

void handle_ipc_messages(int intersection_id)
{
    Intersection *intersection = (intersection_id == INTERSECTION_F10) ? intersection_f10 : intersection_f11;
    if (!intersection)
        return;

    IPCMessage msg;

    while (simulation_running)
    {
        // Check for messages in pipe
        int bytes_read = read(intersection->pipe_fd[0], &msg, sizeof(IPCMessage));

        if (bytes_read > 0)
        {
            switch (msg.msg_type)
            {
            case MSG_EMERGENCY:
                pthread_mutex_lock(&display_mutex);
                printf("[TIME:%ld] IPC: Intersection %d received emergency alert from %d\n",
                       get_current_time(), intersection_id, msg.sender_id);
                pthread_mutex_unlock(&display_mutex);

                // Clear intersection for incoming emergency
                clear_intersection_for_emergency(intersection_id);
                break;

            case MSG_TERMINATE:
                simulation_running = 0;
                break;

            default:
                break;
            }
        }

        sleep_ms(100); // Check every 100ms
    }
}

void send_emergency_alert(int from_intersection, int to_intersection, Vehicle *emergency_vehicle)
{
    Intersection *intersection = (from_intersection == INTERSECTION_F10) ? intersection_f10 : intersection_f11;
    if (!intersection || !emergency_vehicle)
        return;

    IPCMessage msg;
    msg.sender_id = from_intersection;
    msg.msg_type = MSG_EMERGENCY;
    msg.vehicle_id = emergency_vehicle->id;
    msg.vehicle_type = emergency_vehicle->type;
    msg.direction = emergency_vehicle->entry_direction;
    msg.timestamp = get_current_time();

    // In real implementation, you'd write to the other intersection's pipe
    // This is simplified - actual implementation needs proper pipe routing
    pthread_mutex_lock(&display_mutex);
    printf("[TIME:%ld] IPC: Intersection %d sending emergency alert to %d\n",
           get_current_time(), from_intersection, to_intersection);
    pthread_mutex_unlock(&display_mutex);
}

void traffic_light_cycle(int intersection_id)
{
    while (simulation_running)
    {
        // North-South green
        change_traffic_light(intersection_id, GREEN, NORTH);
        sleep_ms(5000); // Green for 5 seconds

        // Yellow warning
        change_traffic_light(intersection_id, YELLOW, NORTH);
        sleep_ms(2000); // Yellow for 2 seconds

        // Red
        change_traffic_light(intersection_id, RED, NORTH);
        sleep_ms(1000); // All red for 1 second

        // East-West green
        change_traffic_light(intersection_id, GREEN, EAST);
        sleep_ms(5000);

        // Yellow warning
        change_traffic_light(intersection_id, YELLOW, EAST);
        sleep_ms(2000);

        // Red
        change_traffic_light(intersection_id, RED, EAST);
        sleep_ms(1000);
    }
}