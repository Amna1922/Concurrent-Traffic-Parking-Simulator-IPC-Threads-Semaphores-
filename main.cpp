#include "common.h"
#include "intersection.h"
#include "parking.h"
#include "controller.h"
#include "visualization.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

// Global lists
VehicleList *f10_vehicles = NULL;
VehicleList *f11_vehicles = NULL;

void signal_handler(int sig)
{
    if (sig == SIGINT)
    {
        simulation_running = 0;
        printf("\nReceived SIGINT. Shutting down gracefully...\n");
    }
}

void setup_signal_handlers()
{
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
}

int main()
{
    srand(time(NULL));

    printf("=== Traffic Simulation System ===\n");
    printf("Starting simulation with 2 intersections (F10 & F11)\n");
    printf("Each with 10 parking spots and bounded waiting queue\n");
    printf("Press Ctrl+C to exit gracefully\n\n");

    // Setup signal handling
    setup_signal_handlers();

    // Initialize global vehicle lists
    f10_vehicles = create_vehicle_list();
    f11_vehicles = create_vehicle_list();

    // Start visualization thread
    pthread_t vis_thread;
    pthread_create(&vis_thread, NULL, visualization_thread_func, NULL);

    // Start controller processes
    printf("Starting controller processes...\n");
    start_controller_process(INTERSECTION_F10);
    sleep(1); // Small delay
    start_controller_process(INTERSECTION_F11);

    // Wait for simulation to complete
    while (simulation_running)
    {
        sleep(1);

        // Check for termination condition (e.g., after 30 seconds)
        static time_t start_time = get_current_time();
        if (get_current_time() - start_time > 30)
        {
            printf("Simulation time completed. Shutting down...\n");
            simulation_running = 0;
        }
    }

    // Wait for visualization thread
    pthread_join(vis_thread, NULL);

    // Cleanup
    destroy_vehicle_list(f10_vehicles);
    destroy_vehicle_list(f11_vehicles);

    printf("Simulation ended cleanly.\n");
    return 0;
}