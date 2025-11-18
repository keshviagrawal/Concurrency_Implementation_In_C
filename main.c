// #include <stdio.h>
// #include <stdlib.h>
// #include <pthread.h>
// #include <unistd.h>
// #include <time.h>
// #include <stdbool.h>

// #define SHOP_CAPACITY 25
// #define SOFA_SEATS 4
// #define NUM_CHEFS 4

// // Forward declarations
// struct Bakery;
// struct Customer;
// struct Chef;

// // Enum for customer state, making the logic clearer
// typedef enum {
//     ARRIVED,
//     STANDING,
//     SEATED,
//     REQUESTING_CAKE,
//     WAITING_FOR_CAKE,
//     CAKE_READY,
//     WAITING_TO_PAY,
//     PAYING,
//     WAITING_PAYMENT_ACCEPT,
//     LEAVING
// } CustomerState;

// // Struct to hold all shared data and synchronization primitives
// typedef struct Bakery {
//     pthread_mutex_t mutex;
//     pthread_cond_t work_for_chef_cond;

//     struct Customer* sofa_queue[SHOP_CAPACITY];
//     struct Customer* standing_queue[SHOP_CAPACITY];
//     struct Customer* payment_queue[SHOP_CAPACITY];
//     int sofa_q_count, standing_q_count, payment_q_count;

//     int customers_in_shop;
//     int sofa_occupancy;
//     int ovens_in_use;
//     bool cash_register_in_use;
//     bool bakery_is_open;
// } Bakery;

// // Struct for an individual customer's data
// typedef struct Customer {
//     int id;
//     pthread_t thread;
//     Bakery* bakery;
//     pthread_cond_t personal_cond; // A condition variable for this specific customer
//     CustomerState state;
// } Customer;

// // Struct for an individual chef's data
// typedef struct Chef {
//     int id;
//     pthread_t thread;
//     Bakery* bakery;
// } Chef;

// // Global timing variables (for active implementation)
// struct timespec start_time;      // program start
// struct timespec sim_start_wall;  // wallclock when first customer thread started
// long first_input_ts = -1;        // earliest input timestamp
// int sim_started = 0;             // becomes 1 after first customer creation

// // Function Prototypes
// void* customer_thread(void* arg);
// void* chef_thread(void* arg);
// long get_timestamp();
// void print_action(const char* entity, int id, const char* action);
// void enqueue(struct Customer** queue, int* count, struct Customer* c);
// struct Customer* dequeue(struct Customer** queue, int* count);

// int main() {
//     clock_gettime(CLOCK_REALTIME, &start_time);

//     typedef struct { long arrival; int id; } CustomerInput;
//     CustomerInput inputs[100];
//     int input_count = 0;
//     long arrival_in;
//     int id_in;

//     // Read all customer inputs before starting the simulation
//     while (scanf("%ld Customer %d", &arrival_in, &id_in) == 2) {
//         if (input_count >= 100) {
//             // Safety break to prevent buffer overflow if input is unexpectedly large
//             fprintf(stderr, "Warning: Exceeded maximum supported customer inputs (100).\\n");
//             break;
//         }
//         inputs[input_count].arrival = arrival_in;
//         inputs[input_count].id = id_in;
//         if (first_input_ts == -1) first_input_ts = arrival_in; // assume non-decreasing input timestamps
//         input_count++;
//     }

//     // Initialize the bakery's shared state
//     Bakery bakery = {0};
//     pthread_mutex_init(&bakery.mutex, NULL);
//     pthread_cond_init(&bakery.work_for_chef_cond, NULL);
//     bakery.bakery_is_open = true;

//     // Create and start the chef threads
//     Chef chefs[NUM_CHEFS];
//     for (int i = 0; i < NUM_CHEFS; i++) {
//         chefs[i].id = i + 1;
//         chefs[i].bakery = &bakery;
//         pthread_create(&chefs[i].thread, NULL, chef_thread, &chefs[i]);
//     }

//     // Create customer threads with timed arrivals
//     Customer* customers[100];
//     long last_arrival_sim_time = first_input_ts; // baseline so first arrival has no sleep

//     for (int i = 0; i < input_count; i++) {
//         // Use usleep to space out customer arrivals according to the input file
//         long delay_needed_us = (inputs[i].arrival - last_arrival_sim_time) * 1000000;
//         if (delay_needed_us > 0) {
//             usleep(delay_needed_us);
//         }
//         last_arrival_sim_time = inputs[i].arrival;

//         customers[i] = malloc(sizeof(Customer));
//         customers[i]->id = inputs[i].id;
//         customers[i]->bakery = &bakery;
//         customers[i]->state = ARRIVED;
//     pthread_cond_init(&customers[i]->personal_cond, NULL);
//     if (!sim_started) { clock_gettime(CLOCK_REALTIME, &sim_start_wall); sim_started = 1; }
//     pthread_create(&customers[i]->thread, NULL, customer_thread, customers[i]);
//     }

//     // Wait for all customer threads to complete their lifecycle
//     for (int i = 0; i < input_count; i++) {
//         pthread_join(customers[i]->thread, NULL);
//         pthread_cond_destroy(&customers[i]->personal_cond);
//         free(customers[i]);
//     }

//     // Signal the chefs to shut down now that all customers are served
//     pthread_mutex_lock(&bakery.mutex);
//     bakery.bakery_is_open = false;
//     pthread_cond_broadcast(&bakery.work_for_chef_cond); // Wake up all sleeping chefs
//     pthread_mutex_unlock(&bakery.mutex);

//     // Wait for all chef threads to exit gracefully
//     for (int i = 0; i < NUM_CHEFS; i++) {
//         pthread_join(chefs[i].thread, NULL);
//     }

//     return 0;
// }

// void* customer_thread(void* arg) {
//     Customer* self = (Customer*)arg;
//     Bakery* bakery = self->bakery;

//     // 1. Enter Bakery (takes 1 sec)
//     pthread_mutex_lock(&bakery->mutex);
//     if (bakery->customers_in_shop >= SHOP_CAPACITY) {
//         // print_action("Customer", self->id, "turned away (shop full)");
//         pthread_mutex_unlock(&bakery->mutex);
//         return NULL;
//     }
//     bakery->customers_in_shop++;
//     print_action("Customer", self->id, "enters");
//     pthread_mutex_unlock(&bakery->mutex);
//     sleep(1);

//     // 2. Sit on Sofa or Stand (takes 1 sec)
//     pthread_mutex_lock(&bakery->mutex);
//     if (bakery->sofa_occupancy < SOFA_SEATS) {
//         bakery->sofa_occupancy++;
//         self->state = SEATED;
//         print_action("Customer", self->id, "sits");
//     } else {
//         self->state = STANDING;
//         enqueue(bakery->standing_queue, &bakery->standing_q_count, self);
//         while (self->state == STANDING) {
//             pthread_cond_wait(&self->personal_cond, &bakery->mutex);
//         }
//         // If we woke up, it means a chef seated us.
//         print_action("Customer", self->id, "sits");
//     }
//     pthread_mutex_unlock(&bakery->mutex);
//     sleep(1); // Simulate 1s action of sitting down.

//     // 3. Request cake and wait for it (requesting takes 1 sec)
//     pthread_mutex_lock(&bakery->mutex);
//     self->state = REQUESTING_CAKE;
//     print_action("Customer", self->id, "requests cake");
//     pthread_mutex_unlock(&bakery->mutex);
//     sleep(1); // Simulate the 1s action of requesting.

//     pthread_mutex_lock(&bakery->mutex);
//     self->state = WAITING_FOR_CAKE;
//     enqueue(bakery->sofa_queue, &bakery->sofa_q_count, self); // Now join the queue for baking
//     pthread_cond_signal(&bakery->work_for_chef_cond);
//     while (self->state == WAITING_FOR_CAKE) {
//         pthread_cond_wait(&self->personal_cond, &bakery->mutex);
//     }

//     // After cake is ready, customer proceeds to payment.
//     // The mutex is still held from the wait loop.
//     self->state = WAITING_PAYMENT_ACCEPT;
//     enqueue(bakery->payment_queue, &bakery->payment_q_count, self);
//     pthread_cond_signal(&bakery->work_for_chef_cond); // Signal chef about payment
//     print_action("Customer", self->id, "pays");

//     // Unlock to allow other threads to run during sleep
//     pthread_mutex_unlock(&bakery->mutex);
//     sleep(1);
//     pthread_mutex_lock(&bakery->mutex);

//     // Wait until payment is accepted and state changes to LEAVING
//     while (self->state != LEAVING) {
//         pthread_cond_wait(&self->personal_cond, &bakery->mutex);
//     }
//     pthread_mutex_unlock(&bakery->mutex);

//     // 5. Leave (takes 1 sec)
//     pthread_mutex_lock(&bakery->mutex);
//     print_action("Customer", self->id, "leaves");
//     bakery->customers_in_shop--;
//     bakery->sofa_occupancy--; // The reserved seat is now free
//     pthread_cond_broadcast(&bakery->work_for_chef_cond); // Signal all chefs
//     pthread_mutex_unlock(&bakery->mutex);
//     sleep(1);

//     return NULL;
// }

// void* chef_thread(void* arg) {
//     Chef* self = (Chef*)arg;
//     Bakery* bakery = self->bakery;

//     while (1) {
//         pthread_mutex_lock(&bakery->mutex);

//         // Wait for work, but only if no possible action can be taken (no busy-waiting)
//         while (bakery->bakery_is_open &&
//                (bakery->payment_q_count == 0 || bakery->cash_register_in_use) &&
//                (bakery->sofa_q_count == 0 || bakery->ovens_in_use >= NUM_CHEFS) &&
//                (bakery->standing_q_count == 0 || bakery->sofa_occupancy >= SOFA_SEATS)) {
//             pthread_cond_wait(&bakery->work_for_chef_cond, &bakery->mutex);
//         }

//                   // Condition to exit the loop: bakery is closed and no customers are left to serve
//                   if (!bakery->bakery_is_open && bakery->payment_q_count == 0 && bakery->sofa_q_count == 0 && bakery->standing_q_count == 0) {
//                       pthread_mutex_unlock(&bakery->mutex);
//                       break;
//                   }
//         // Priority 1: Accept Payment
//         if (bakery->payment_q_count > 0 && !bakery->cash_register_in_use) {
//             bakery->cash_register_in_use = true;
//             Customer* c = dequeue(bakery->payment_queue, &bakery->payment_q_count);
//             char paybuf[64];
//             snprintf(paybuf, sizeof(paybuf), "accepts payment for Customer %d", c->id);
//             print_action("Chef", self->id, paybuf);
//             pthread_mutex_unlock(&bakery->mutex);
//             sleep(2); // Chef action takes 2 seconds

//             pthread_mutex_lock(&bakery->mutex);
//             c->state = LEAVING;
//             bakery->cash_register_in_use = false;
//             pthread_cond_signal(&c->personal_cond);
//             pthread_cond_broadcast(&bakery->work_for_chef_cond); // Wake all chefs as register is now free
//             pthread_mutex_unlock(&bakery->mutex);
//             continue;
//         }
//         // Priority 2: Bake Cake
//         else if (bakery->sofa_q_count > 0 && bakery->ovens_in_use < NUM_CHEFS) {
//             bakery->ovens_in_use++;
//             Customer* c = dequeue(bakery->sofa_queue, &bakery->sofa_q_count);
//             char bakebuf[64];
//             snprintf(bakebuf, sizeof(bakebuf), "bakes for Customer %d", c->id);
//             print_action("Chef", self->id, bakebuf);
//             pthread_mutex_unlock(&bakery->mutex);
//             sleep(2); // Chef action takes 2 seconds

//             pthread_mutex_lock(&bakery->mutex);
//             c->state = CAKE_READY;
//             bakery->ovens_in_use--;
//             pthread_cond_signal(&c->personal_cond);
//             pthread_cond_broadcast(&bakery->work_for_chef_cond); // Wake all chefs as an oven is now free
//             pthread_mutex_unlock(&bakery->mutex);
//             continue;
//         }
//                   // Priority 3: Move standing customer to sofa
//                   else if (bakery->standing_q_count > 0 && bakery->sofa_occupancy < SOFA_SEATS) {
//                       Customer* c = dequeue(bakery->standing_queue, &bakery->standing_q_count);
//                       bakery->sofa_occupancy++;
//                       c->state = SEATED;
//                       pthread_cond_signal(&c->personal_cond); // Wake up the customer's thread
//                       pthread_mutex_unlock(&bakery->mutex);
//                       continue;
//                   }        else {
//             // Unlock if no action was taken to prevent deadlock
//             pthread_mutex_unlock(&bakery->mutex);
//         }
//     }
//     return NULL;
// }

// // --- Utility Function Implementations ---

// long get_timestamp() {
//     struct timespec now; clock_gettime(CLOCK_REALTIME, &now);
//     if (!sim_started) return (first_input_ts == -1 ? 0 : first_input_ts);
//     long delta = now.tv_sec - sim_start_wall.tv_sec;
//     return (first_input_ts == -1 ? 0 : first_input_ts) + delta;
// }

// void print_action(const char* entity, int id, const char* action) {
//     printf("%ld %s %d %s\n", get_timestamp(), entity, id, action);
//     fflush(stdout);
// }

// // Simple FIFO Enqueue
// void enqueue(struct Customer** queue, int* count, struct Customer* c) {
//     queue[*count] = c;
//     (*count)++;
// }

// // Simple FIFO Dequeue
// struct Customer* dequeue(struct Customer** queue, int* count) {
//     if (*count == 0) return NULL;
//     struct Customer* c = queue[0];
//     for (int i = 0; i < *count - 1; i++) {
//         queue[i] = queue[i + 1];
//     }
//     (*count)--;
//     return c;
// }


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

#define SHOP_CAPACITY 25
#define SOFA_SEATS 4
#define NUM_CHEFS 4

// Forward declarations
struct Bakery;
struct Customer;
struct Chef;

// Enum for customer state, making the logic clearer
typedef enum {
    ARRIVED,
    STANDING,
    SEATED,
    REQUESTING_CAKE,
    WAITING_FOR_CAKE,
    CAKE_READY,
    WAITING_TO_PAY,
    PAYING,
    WAITING_PAYMENT_ACCEPT,
    LEAVING
} CustomerState;

// Struct to hold all shared data and synchronization primitives
typedef struct Bakery {
    pthread_mutex_t mutex;
    pthread_cond_t work_for_chef_cond;

    struct Customer* sofa_queue[SHOP_CAPACITY];
    struct Customer* standing_queue[SHOP_CAPACITY];
    struct Customer* payment_queue[SHOP_CAPACITY];
    int sofa_q_count, standing_q_count, payment_q_count;

    int customers_in_shop;
    int sofa_occupancy;
    int ovens_in_use;
    bool cash_register_in_use;
    bool bakery_is_open;
} Bakery;

// Struct for an individual customer's data
typedef struct Customer {
    int id;
    pthread_t thread;
    Bakery* bakery;
    pthread_cond_t personal_cond; // A condition variable for this specific customer
    CustomerState state;
} Customer;

// Struct for an individual chef's data
typedef struct Chef {
    int id;
    pthread_t thread;
    Bakery* bakery;
} Chef;

// Global timing variables (for active implementation)
struct timespec start_time;      // program start
struct timespec sim_start_wall;  // wallclock when first customer thread started
long first_input_ts = -1;        // earliest input timestamp
int sim_started = 0;             // becomes 1 after first customer creation

// Function Prototypes
void* customer_thread(void* arg);
void* chef_thread(void* arg);
long get_timestamp();
void print_action(const char* entity, int id, const char* action);
void enqueue(struct Customer** queue, int* count, struct Customer* c);
struct Customer* dequeue(struct Customer** queue, int* count);

int main() {
    clock_gettime(CLOCK_REALTIME, &start_time);

    typedef struct { long arrival; int id; } CustomerInput;
    CustomerInput inputs[100];
    int input_count = 0;
    long arrival_in;
    int id_in;

    // Read all customer inputs before starting the simulation
    while (scanf("%ld Customer %d", &arrival_in, &id_in) == 2) {
        if (input_count >= 100) {
            // Safety break to prevent buffer overflow if input is unexpectedly large
            fprintf(stderr, "Warning: Exceeded maximum supported customer inputs (100).\\n");
            break;
        }
        inputs[input_count].arrival = arrival_in;
        inputs[input_count].id = id_in;
        if (first_input_ts == -1) first_input_ts = arrival_in; // assume non-decreasing input timestamps
        input_count++;
    }

    // Initialize the bakery's shared state
    Bakery bakery = {0};
    pthread_mutex_init(&bakery.mutex, NULL);
    pthread_cond_init(&bakery.work_for_chef_cond, NULL);
    bakery.bakery_is_open = true;

    // Create and start the chef threads
    Chef chefs[NUM_CHEFS];
    for (int i = 0; i < NUM_CHEFS; i++) {
        chefs[i].id = i + 1;
        chefs[i].bakery = &bakery;
        pthread_create(&chefs[i].thread, NULL, chef_thread, &chefs[i]);
    }

    // Create customer threads with timed arrivals
    Customer* customers[100];
    long last_arrival_sim_time = first_input_ts; // baseline so first arrival has no sleep

    for (int i = 0; i < input_count; i++) {
        // Use usleep to space out customer arrivals according to the input file
        long delay_needed_us = (inputs[i].arrival - last_arrival_sim_time) * 1000000;
        if (delay_needed_us > 0) {
            usleep(delay_needed_us);
        }
        last_arrival_sim_time = inputs[i].arrival;

        customers[i] = malloc(sizeof(Customer));
        customers[i]->id = inputs[i].id;
        customers[i]->bakery = &bakery;
        customers[i]->state = ARRIVED;
    pthread_cond_init(&customers[i]->personal_cond, NULL);
    if (!sim_started) { clock_gettime(CLOCK_REALTIME, &sim_start_wall); sim_started = 1; }
    pthread_create(&customers[i]->thread, NULL, customer_thread, customers[i]);
    }

    // Wait for all customer threads to complete their lifecycle
    for (int i = 0; i < input_count; i++) {
        pthread_join(customers[i]->thread, NULL);
        pthread_cond_destroy(&customers[i]->personal_cond);
        free(customers[i]);
    }

    // Signal the chefs to shut down now that all customers are served
    pthread_mutex_lock(&bakery.mutex);
    bakery.bakery_is_open = false;
    pthread_cond_broadcast(&bakery.work_for_chef_cond); // Wake up all sleeping chefs
    pthread_mutex_unlock(&bakery.mutex);

    // Wait for all chef threads to exit gracefully
    for (int i = 0; i < NUM_CHEFS; i++) {
        pthread_join(chefs[i].thread, NULL);
    }

    return 0;
}

void* customer_thread(void* arg) {
    Customer* self = (Customer*)arg;
    Bakery* bakery = self->bakery;

    // 1. Enter Bakery (takes 1 sec)
    pthread_mutex_lock(&bakery->mutex);
    if (bakery->customers_in_shop >= SHOP_CAPACITY) {
        // print_action("Customer", self->id, "turned away (shop full)");
        pthread_mutex_unlock(&bakery->mutex);
        return NULL;
    }
    bakery->customers_in_shop++;
    print_action("Customer", self->id, "enters");
    pthread_mutex_unlock(&bakery->mutex);
    sleep(1);

    // 2. Sit on Sofa or Stand (takes 1 sec)
    pthread_mutex_lock(&bakery->mutex);
    if (bakery->sofa_occupancy < SOFA_SEATS) {
        bakery->sofa_occupancy++;
        self->state = SEATED;
        print_action("Customer", self->id, "sits");
    } else {
        self->state = STANDING;
        enqueue(bakery->standing_queue, &bakery->standing_q_count, self);
        while (self->state == STANDING) {
            pthread_cond_wait(&self->personal_cond, &bakery->mutex);
        }
        // If we woke up, it means a chef seated us.
        print_action("Customer", self->id, "sits");
    }
    pthread_mutex_unlock(&bakery->mutex);
    sleep(1); // Simulate 1s action of sitting down.

    // 3. Request cake and wait for it (requesting takes 1 sec)
    pthread_mutex_lock(&bakery->mutex);
    self->state = REQUESTING_CAKE;
    print_action("Customer", self->id, "requests cake");
    pthread_mutex_unlock(&bakery->mutex);
    sleep(1); // Simulate the 1s action of requesting.

    pthread_mutex_lock(&bakery->mutex);
    self->state = WAITING_FOR_CAKE;
    enqueue(bakery->sofa_queue, &bakery->sofa_q_count, self); // Now join the queue for baking
    pthread_cond_signal(&bakery->work_for_chef_cond);
    while (self->state == WAITING_FOR_CAKE) {
        pthread_cond_wait(&self->personal_cond, &bakery->mutex);
    }

    // After cake is ready, customer proceeds to payment.
    // The mutex is still held from the wait loop.
    
    // The "pays" action starts now and takes 1 second.
    print_action("Customer", self->id, "pays");
    
    // Unlock the mutex to allow other threads to run and time to pass during the sleep.
    pthread_mutex_unlock(&bakery->mutex);
    sleep(1);
    pthread_mutex_lock(&bakery->mutex);

    // After 1 second, the payment is ready to be collected. Add to queue and signal a chef.
    self->state = WAITING_PAYMENT_ACCEPT;
    enqueue(bakery->payment_queue, &bakery->payment_q_count, self);
    pthread_cond_signal(&bakery->work_for_chef_cond); 

    // Wait until payment is accepted and state changes to LEAVING
    while (self->state != LEAVING) {
        pthread_cond_wait(&self->personal_cond, &bakery->mutex);
    }
    pthread_mutex_unlock(&bakery->mutex);


    // 5. Leave (takes 1 sec)
    pthread_mutex_lock(&bakery->mutex);
    print_action("Customer", self->id, "leaves");
    bakery->customers_in_shop--;
    bakery->sofa_occupancy--; // The reserved seat is now free
    pthread_cond_broadcast(&bakery->work_for_chef_cond); // Signal all chefs
    pthread_mutex_unlock(&bakery->mutex);
    sleep(1);

    return NULL;
}

void* chef_thread(void* arg) {
    Chef* self = (Chef*)arg;
    Bakery* bakery = self->bakery;

    while (1) {
        pthread_mutex_lock(&bakery->mutex);

        // Wait for work, but only if no possible action can be taken (no busy-waiting)
        while (bakery->bakery_is_open &&
               (bakery->payment_q_count == 0 || bakery->cash_register_in_use) &&
               (bakery->sofa_q_count == 0 || bakery->ovens_in_use >= NUM_CHEFS) &&
               (bakery->standing_q_count == 0 || bakery->sofa_occupancy >= SOFA_SEATS)) {
            pthread_cond_wait(&bakery->work_for_chef_cond, &bakery->mutex);
        }

        // Condition to exit the loop: bakery is closed and no customers are left to serve
        if (!bakery->bakery_is_open && bakery->payment_q_count == 0 && bakery->sofa_q_count == 0 && bakery->standing_q_count == 0) {
            pthread_mutex_unlock(&bakery->mutex);
            break;
        }
        // Priority 1: Accept Payment
        if (bakery->payment_q_count > 0 && !bakery->cash_register_in_use) {
            bakery->cash_register_in_use = true;
            Customer* c = dequeue(bakery->payment_queue, &bakery->payment_q_count);
            char paybuf[64];
            snprintf(paybuf, sizeof(paybuf), "accepts payment for Customer %d", c->id);
            print_action("Chef", self->id, paybuf);
            pthread_mutex_unlock(&bakery->mutex);
            sleep(2); // Chef action takes 2 seconds

            pthread_mutex_lock(&bakery->mutex);
            c->state = LEAVING;
            bakery->cash_register_in_use = false;
            pthread_cond_signal(&c->personal_cond);
            pthread_cond_broadcast(&bakery->work_for_chef_cond); // Wake all chefs as register is now free
            pthread_mutex_unlock(&bakery->mutex);
            continue;
        }
        // Priority 2: Bake Cake
        else if (bakery->sofa_q_count > 0 && bakery->ovens_in_use < NUM_CHEFS) {
            bakery->ovens_in_use++;
            Customer* c = dequeue(bakery->sofa_queue, &bakery->sofa_q_count);
            char bakebuf[64];
            snprintf(bakebuf, sizeof(bakebuf), "bakes for Customer %d", c->id);
            print_action("Chef", self->id, bakebuf);
            pthread_mutex_unlock(&bakery->mutex);
            sleep(2); // Chef action takes 2 seconds

            pthread_mutex_lock(&bakery->mutex);
            c->state = CAKE_READY;
            bakery->ovens_in_use--;
            pthread_cond_signal(&c->personal_cond);
            pthread_cond_broadcast(&bakery->work_for_chef_cond); // Wake all chefs as an oven is now free
            pthread_mutex_unlock(&bakery->mutex);
            continue;
        }
        // Priority 3: Move standing customer to sofa
        else if (bakery->standing_q_count > 0 && bakery->sofa_occupancy < SOFA_SEATS) {
            Customer* c = dequeue(bakery->standing_queue, &bakery->standing_q_count);
            bakery->sofa_occupancy++;
            c->state = SEATED;
            pthread_cond_signal(&c->personal_cond); // Wake up the customer's thread
            pthread_mutex_unlock(&bakery->mutex);
            continue;
        }
        else {
            // Unlock if no action was taken to prevent deadlock
            pthread_mutex_unlock(&bakery->mutex);
        }
    }
    return NULL;
}

// --- Utility Function Implementations ---

long get_timestamp() {
    struct timespec now; clock_gettime(CLOCK_REALTIME, &now);
    if (!sim_started) return (first_input_ts == -1 ? 0 : first_input_ts);
    long delta = now.tv_sec - sim_start_wall.tv_sec;
    return (first_input_ts == -1 ? 0 : first_input_ts) + delta;
}

void print_action(const char* entity, int id, const char* action) {
    printf("%ld %s %d %s\n", get_timestamp(), entity, id, action);
    fflush(stdout);
}

// Simple FIFO Enqueue
void enqueue(struct Customer** queue, int* count, struct Customer* c) {
    queue[*count] = c;
    (*count)++;
}

// Simple FIFO Dequeue
struct Customer* dequeue(struct Customer** queue, int* count) {
    if (*count == 0) return NULL;
    struct Customer* c = queue[0];
    for (int i = 0; i < *count - 1; i++) {
        queue[i] = queue[i + 1];
    }
    (*count)--;
    return c;
}