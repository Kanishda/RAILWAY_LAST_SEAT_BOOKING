/*
 * Railway Last-Seat Booking - Mutual Exclusion Simulation (C, POSIX threads)
 *
 * Several passenger threads try to book the SAME last seat.
 * A mutex protects the critical section so only one passenger can
 * check and update the seat status at a time.
 *
 * Compile : gcc railway_booking.c -o railway_booking -pthread
 * Run     : ./railway_booking            (safe, with mutual exclusion)
 *           ./railway_booking --unsafe   (shows the race condition, no lock)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_PASSENGERS 8
#define SEAT_NUMBER    42

typedef struct {
    int number;
    int booked;      /* 0 = available, 1 = booked */
    int passenger;   /* id of passenger who owns the seat */
} Seat;

Seat seat = { SEAT_NUMBER, 0, -1 };
int  confirmed_count = 0;
int  use_lock = 1;

pthread_mutex_t seat_lock  = PTHREAD_MUTEX_INITIALIZER;  /* guards the seat   */
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;  /* guards console    */

void *book_seat(void *arg)
{
    int id = *(int *)arg;
    int ok = 0;

    usleep((rand() % 50) * 1000);            /* requests arrive at random times */

    pthread_mutex_lock(&print_lock);
    printf("[REQUEST ] Passenger-%d wants seat %d\n", id, seat.number);
    pthread_mutex_unlock(&print_lock);

    if (use_lock)
        pthread_mutex_lock(&seat_lock);      /* ENTRY SECTION */

    /* ---------------- CRITICAL SECTION ---------------- */
    if (!seat.booked) {
        usleep(10000);                       /* processing delay (makes race visible) */
        seat.booked    = 1;
        seat.passenger = id;
        ok = 1;
    }
    /* -------------------------------------------------- */

    if (use_lock)
        pthread_mutex_unlock(&seat_lock);    /* EXIT SECTION */

    pthread_mutex_lock(&print_lock);
    if (ok) {
        printf("[SUCCESS ] Passenger-%d: CONFIRMED\n", id);
        confirmed_count++;
    } else {
        printf("[FAILED  ] Passenger-%d: REJECTED (seat already booked)\n", id);
    }
    pthread_mutex_unlock(&print_lock);
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t threads[NUM_PASSENGERS];
    int ids[NUM_PASSENGERS];
    int i;

    if (argc > 1 && strcmp(argv[1], "--unsafe") == 0)
        use_lock = 0;

    srand(42);   /* fixed seed only for the delays; thread scheduling still varies */

    printf("=======================================================\n");
    printf(" Railway Last-Seat Booking - %s\n",
           use_lock ? "WITH mutual exclusion (Mutex)"
                    : "WITHOUT mutual exclusion (UNSAFE)");
    printf("=======================================================\n");
    printf("Seat %d is the last available seat.\n", seat.number);
    printf("%d passengers are trying to book it...\n\n", NUM_PASSENGERS);

    for (i = 0; i < NUM_PASSENGERS; i++) {
        ids[i] = i + 1;
        pthread_create(&threads[i], NULL, book_seat, &ids[i]);
    }
    for (i = 0; i < NUM_PASSENGERS; i++)
        pthread_join(threads[i], NULL);

    printf("\n-------------------------------------------------------\n");
    printf("Total successful bookings : %d\n", confirmed_count);
    printf("Seat %d final owner        : Passenger-%d\n", seat.number, seat.passenger);
    if (confirmed_count == 1)
        printf("RESULT: Seat allocated to exactly ONE passenger. No duplicate booking.\n");
    else
        printf("RESULT: DUPLICATE BOOKING! %d passengers got the same seat.\n", confirmed_count);

    pthread_mutex_destroy(&seat_lock);
    pthread_mutex_destroy(&print_lock);
    return 0;
}
