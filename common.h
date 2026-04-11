#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

// Vehicle types
typedef enum
{
    AMBULANCE = 0,
    FIRETRUCK,
    BUS,
    CAR,
    BIKE,
    TRACTOR,
    VEHICLE_TYPE_COUNT
} VehicleType;

// Directions
typedef enum
{
    NORTH = 0,
    SOUTH,
    EAST,
    WEST,
    DIRECTION_COUNT
} Direction;

// Movement intentions
typedef enum
{
    STRAIGHT = 0,
    LEFT,
    RIGHT
} TurnDirection;

// Intersection IDs
typedef enum
{
    INTERSECTION_F10 = 10,
    INTERSECTION_F11 = 11
} IntersectionID;

// Message types for IPC
typedef enum
{
    MSG_EMERGENCY = 0,
    MSG_CLEAR_INTERSECTION,
    MSG_EMERGENCY_CLEARED,
    MSG_PARKING_FULL,
    MSG_PARKING_AVAILABLE,
    MSG_TERMINATE
} MessageType;

// Vehicle priority levels
typedef enum
{
    PRIORITY_HIGHEST = 0, // Ambulance, Firetruck
    PRIORITY_MEDIUM = 1,  // Bus
    PRIORITY_NORMAL = 2   // Car, Bike, Tractor
} PriorityLevel;

// IPC message structure
typedef struct
{
    int sender_id;
    MessageType msg_type;
    int vehicle_id;
    VehicleType vehicle_type;
    Direction direction;
    time_t timestamp;
} IPCMessage;

// Global configuration
typedef struct
{
    int total_vehicles;
    int parking_queue_size;
    int max_vehicles_per_intersection;
    int emergency_probability; // 1-100
    int vehicle_spawn_min_ms;
    int vehicle_spawn_max_ms;
    int parking_duration_min_ms;
    int parking_duration_max_ms;
    int traffic_light_duration_ms;
    int simulation_duration_sec;
} SimulationConfig;

// Global simulation state
extern volatile sig_atomic_t simulation_running;
extern pthread_mutex_t display_mutex;

// Function prototypes
const char *vehicle_type_to_string(VehicleType type);
const char *direction_to_string(Direction dir);
const char *turn_to_string(TurnDirection turn);
int get_priority(VehicleType type);
void sleep_ms(int milliseconds);
time_t get_current_time();

#endif