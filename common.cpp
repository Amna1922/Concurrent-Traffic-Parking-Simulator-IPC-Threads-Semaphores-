#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

volatile sig_atomic_t simulation_running = 1;
pthread_mutex_t display_mutex = PTHREAD_MUTEX_INITIALIZER;

const char *vehicle_type_to_string(VehicleType type)
{
    static const char *names[] = {
        "AMBULANCE", "FIRETRUCK", "BUS", "CAR", "BIKE", "TRACTOR"};
    return (type < VEHICLE_TYPE_COUNT) ? names[type] : "UNKNOWN";
}

const char *direction_to_string(Direction dir)
{
    static const char *names[] = {
        "NORTH", "SOUTH", "EAST", "WEST"};
    return (dir < DIRECTION_COUNT) ? names[dir] : "UNKNOWN";
}

const char *turn_to_string(TurnDirection turn)
{
    static const char *names[] = {
        "STRAIGHT", "LEFT", "RIGHT"};
    return (turn < 3) ? names[turn] : "UNKNOWN";
}

int get_priority(VehicleType type)
{
    switch (type)
    {
    case AMBULANCE:
    case FIRETRUCK:
        return PRIORITY_HIGHEST;
    case BUS:
        return PRIORITY_MEDIUM;
    default:
        return PRIORITY_NORMAL;
    }
}

void sleep_ms(int milliseconds)
{
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

time_t get_current_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}