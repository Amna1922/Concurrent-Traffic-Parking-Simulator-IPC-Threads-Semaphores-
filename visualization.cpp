#include "visualization.h"
#include "intersection.h"
#include "parking.h"
#include <ncurses.h>
#include <string.h>

WINDOW *main_win;
WINDOW *f10_win;
WINDOW *f11_win;
WINDOW *log_win;

void init_visualization()
{
    initscr();
    cbreak();
    noecho();
    cursor(0);
    start_color();

    // Initialize color pairs
    init_pair(1, COLOR_RED, COLOR_BLACK);    // Emergency vehicles
    init_pair(2, COLOR_YELLOW, COLOR_BLACK); // Buses
    init_pair(3, COLOR_WHITE, COLOR_BLACK);  // Normal vehicles
    init_pair(4, COLOR_GREEN, COLOR_BLACK);  // Green light
    init_pair(5, COLOR_RED, COLOR_BLACK);    // Red light

    // Create main window
    main_win = newwin(25, 80, 0, 0);
    box(main_win, 0, 0);

    // Create intersection windows
    f10_win = newwin(10, 38, 2, 1);
    f11_win = newwin(10, 38, 2, 41);
    log_win = newwin(10, 78, 13, 1);

    scrollok(log_win, TRUE);

    refresh();
    wrefresh(main_win);
}

void update_display()
{
    pthread_mutex_lock(&display_mutex);

    // Clear windows
    werase(f10_win);
    werase(f11_win);

    // Draw F10 intersection
    box(f10_win, 0, 0);
    mvwprintw(f10_win, 0, 2, "F10 INTERSECTION");

    // Draw traffic light state for F10
    if (intersection_f10)
    {
        const char *state;
        int color_pair;
        if (intersection_f10->current_state == GREEN)
        {
            state = "GREEN";
            color_pair = 4;
        }
        else if (intersection_f10->current_state == RED)
        {
            state = "RED";
            color_pair = 5;
        }
        else if (intersection_f10->current_state == YELLOW)
        {
            state = "YELLOW";
            color_pair = 2;
        }
        else
        {
            state = "EMERGENCY";
            color_pair = 1;
        }

        wattron(f10_win, COLOR_PAIR(color_pair));
        mvwprintw(f10_win, 2, 2, "Signal: %s", state);
        wattroff(f10_win, COLOR_PAIR(color_pair));

        // Show waiting vehicles
        mvwprintw(f10_win, 3, 2, "Waiting NS: %d", intersection_f10->vehicles_waiting_ns);
        mvwprintw(f10_win, 4, 2, "Waiting EW: %d", intersection_f10->vehicles_waiting_ew);
    }

    // Draw parking info for F10
    if (parking_f10)
    {
        mvwprintw(f10_win, 6, 2, "Parking: %d/10 spots", 10 - parking_f10->available_spots);
        mvwprintw(f10_win, 7, 2, "Queue: %d/5 slots", 5 - parking_f10->queue_available);

        // Draw progress bar
        mvwprintw(f10_win, 8, 2, "[");
        for (int i = 0; i < 10; i++)
        {
            if (i < (10 - parking_f10->available_spots))
            {
                waddch(f10_win, ACS_CKBOARD);
            }
            else
            {
                waddch(f10_win, ' ');
            }
        }
        wprintw(f10_win, "]");
    }

    // Draw F11 intersection (similar to F10)
    box(f11_win, 0, 0);
    mvwprintw(f11_win, 0, 2, "F11 INTERSECTION");

    if (intersection_f11)
    {
        const char *state;
        int color_pair;
        if (intersection_f11->current_state == GREEN)
        {
            state = "GREEN";
            color_pair = 4;
        }
        else if (intersection_f11->current_state == RED)
        {
            state = "RED";
            color_pair = 5;
        }
        else if (intersection_f11->current_state == YELLOW)
        {
            state = "YELLOW";
            color_pair = 2;
        }
        else
        {
            state = "EMERGENCY";
            color_pair = 1;
        }

        wattron(f11_win, COLOR_PAIR(color_pair));
        mvwprintw(f11_win, 2, 2, "Signal: %s", state);
        wattroff(f11_win, COLOR_PAIR(color_pair));

        mvwprintw(f11_win, 3, 2, "Waiting NS: %d", intersection_f11->vehicles_waiting_ns);
        mvwprintw(f11_win, 4, 2, "Waiting EW: %d", intersection_f11->vehicles_waiting_ew);
    }

    if (parking_f11)
    {
        mvwprintw(f11_win, 6, 2, "Parking: %d/10 spots", 10 - parking_f11->available_spots);
        mvwprintw(f11_win, 7, 2, "Queue: %d/5 slots", 5 - parking_f11->queue_available);

        mvwprintw(f11_win, 8, 2, "[");
        for (int i = 0; i < 10; i++)
        {
            if (i < (10 - parking_f11->available_spots))
            {
                waddch(f11_win, ACS_CKBOARD);
            }
            else
            {
                waddch(f11_win, ' ');
            }
        }
        wprintw(f11_win, "]");
    }

    // Update main window title
    mvwprintw(main_win, 0, 2, "TRAFFIC SIMULATION: F10 & F11 INTERSECTIONS");
    mvwprintw(main_win, 0, 60, "Ctrl+C to exit");

    // Refresh all windows
    wrefresh(f10_win);
    wrefresh(f11_win);
    wrefresh(main_win);

    pthread_mutex_unlock(&display_mutex);
}

void cleanup_visualization()
{
    delwin(f10_win);
    delwin(f11_win);
    delwin(log_win);
    delwin(main_win);
    endwin();
}

void *visualization_thread_func(void *arg)
{
    init_visualization();

    while (simulation_running)
    {
        update_display();
        sleep_ms(100); // Update every 100ms
    }

    cleanup_visualization();
    return NULL;
}

// Log message to log window
void log_message(const char *message)
{
    pthread_mutex_lock(&display_mutex);
    wprintw(log_win, "%s\n", message);
    wrefresh(log_win);
    pthread_mutex_unlock(&display_mutex);
}