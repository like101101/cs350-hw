#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
// Other Libraries
#include <sys/types.h>
#include <unistd.h>
#include <math.h>


struct event {
    u_int8_t type; // 0 = birth_event, 1 = death_event, 2 = monitor_event
    u_int8_t class;
    double time;
    u_int32_t id;
    struct event *next_event;
};

struct request {
    u_int8_t type;
    u_int32_t id;
    double arrival_time;
    double service_time;
    double start_time;
    double finish_time;
    struct request *next_request;
};

struct load_generator {
    double lambda;
    struct event *timeline;
};

/**
struct server {
    struct request current_request;
    struct request *queue;
};
**/

struct state {
    double num_requests_finished_X;
    double num_requests_finished_Y;
    double num_requests_in_queue;
    double num_requests_in_system;
    double num_monitors;
    double total_waiting_time_X;
    double total_waiting_time_Y;
    double total_response_time_X;
    double total_response_time_Y;
    double busy_time;
    double num_requests_looped;
};


double get_exp(double lambda) {
    double u = (double)rand() / (double)RAND_MAX;
    return -log(1 - u) / lambda;
}

struct event *add_event(struct event *timeline, struct event *new_event) {
    struct event *current_event = timeline;
    struct event *previous_event = NULL;
    while (current_event != NULL && current_event->time < new_event->time) {
        previous_event = current_event;
        current_event = current_event->next_event;
    }
    if (previous_event == NULL) {
        new_event->next_event = timeline;
        timeline = new_event;
    } else {
        previous_event->next_event = new_event;
        new_event->next_event = current_event;
    }
    return timeline;
}



struct request* add_request(struct request *queue, struct request *new_request) {
    if (queue == NULL){
        return new_request;
    }
    struct request *current_request = queue;
    while (current_request->next_request != NULL) {
        current_request = current_request->next_request;
        if (current_request->next_request == current_request) {
            break;
        }
    }
    current_request->next_request = new_request;
    return queue;
}



struct state* update_complete(struct state* current_state, struct request* current_request) {
    if (current_request->type == 'X') {
        current_state->num_requests_finished_X += 1;
        current_state->total_waiting_time_X += current_request->start_time - current_request->arrival_time;
        current_state->total_response_time_X += current_request->finish_time - current_request->arrival_time;
        printf("%lf\n", current_state->num_requests_finished_X);
    } else {
        current_state->num_requests_finished_Y += 1;
        current_state->total_waiting_time_Y += current_request->start_time - current_request->arrival_time;
        current_state->total_response_time_Y += current_request->finish_time - current_request->arrival_time;
    }
    current_state->busy_time += current_request->service_time;
    return current_state;
} 

struct state* update_looped(struct state* current_state, struct request* current_request) {
    if (current_request->type == 'X') {
        current_state->total_waiting_time_X += current_request->start_time - current_request->arrival_time;
        current_state->total_response_time_X += current_request->finish_time - current_request->arrival_time;

    } else {
        current_state->total_waiting_time_Y += current_request->start_time - current_request->arrival_time;
        current_state->total_response_time_Y += current_request->finish_time - current_request->arrival_time;
    }
    current_state->busy_time += current_request->service_time;
    current_state->num_requests_looped+=1;
    return current_state;
} 

void printResults (struct state* current_state) {

    double averageResponseTimeX = current_state->total_response_time_X / current_state->num_requests_finished_X;
    double averageResponseTimeY = current_state->total_response_time_Y / current_state->num_requests_finished_Y;
    double averageWaitingTimeX = current_state->total_waiting_time_X / current_state->num_requests_finished_X;
    double averageWaitingTimeY = current_state->total_waiting_time_Y / current_state->num_requests_finished_Y;
    double averageRequestsInQueue = current_state->num_requests_in_queue / current_state->num_monitors;
    double averageRequestsInSystem = current_state->num_requests_in_system / current_state->num_monitors;
    double run = (
        current_state->num_requests_finished_X + current_state->num_requests_finished_Y + current_state->num_requests_looped
        )/(current_state->num_requests_finished_X + current_state->num_requests_finished_Y);

    printf("QAVG: %lf\n", averageRequestsInSystem);
    printf("WAVG: %lf\n", averageRequestsInQueue);
    printf("TRESP X: %lf\n", averageResponseTimeX);
    printf("TWAIT X: %lf\n", averageWaitingTimeX);
    printf("TRESP Y: %lf\n", averageResponseTimeY);
    printf("TWAIT Y: %lf\n", averageWaitingTimeY);
    printf("RUNS: %lf\n", run);

}



void simulate(
    double length_time,
    double lambda_X,
    double lambda_Y,
    double service_time_X,
    double service_time_Y,
    double pout
){

    // Initialize the events
    struct event *first_birth_event_x = malloc(sizeof(struct event));
    first_birth_event_x->type = '0';
    first_birth_event_x->class = 'X';
    first_birth_event_x->time = 0;
    first_birth_event_x->id = 0;
    first_birth_event_x->next_event = NULL;

    struct event *timeline = first_birth_event_x;

    struct event *first_birth_event_y = malloc(sizeof(struct event));
    first_birth_event_y->type = '0';
    first_birth_event_y->class = 'Y';
    first_birth_event_y->time = 0;
    first_birth_event_y->id = 0;
    first_birth_event_y->next_event = NULL;

    struct event *first_monitor_event = malloc(sizeof(struct event));
    first_monitor_event->type = '2';
    first_monitor_event->time = 0;
    first_monitor_event->next_event = NULL;

    struct state *my_state = malloc(sizeof(struct state));
    memset(my_state, 0, sizeof(struct state));

    struct request *server = malloc(sizeof(struct request));
    server = NULL;
    int queue_size = 0;


    timeline = add_event(timeline, first_birth_event_y);
    timeline = add_event(timeline, first_monitor_event);

    while (timeline != NULL && timeline->time < length_time) {
        struct event *current_event = timeline;
        timeline = timeline->next_event;
        if (current_event->type == '0') {
            // Birth Event
            struct request *new_request = malloc(sizeof(struct request));
            new_request->type = current_event->class;
            new_request->id = current_event->id;
            new_request->arrival_time = current_event->time;
            new_request->start_time = 0;
            new_request->finish_time = 0;
            new_request->next_request = NULL;
            if (current_event->class == 'X') {
                // Birth Event X
                new_request->service_time = get_exp(1/service_time_X);
                struct event *next_birth_event = malloc(sizeof(struct event));
                next_birth_event->type = '0';
                next_birth_event->class = 'X';
                next_birth_event->time = current_event->time + get_exp(lambda_X);
                next_birth_event->id = current_event->id + 1;
                next_birth_event->next_event = NULL;
                timeline = add_event(timeline, next_birth_event);
                printf("%c%d ARR: %lf LEN: %lf\n", new_request->type, new_request->id, new_request->arrival_time, new_request->service_time);
                if (server == NULL) {
                    new_request->start_time = current_event->time;
                    printf("%c%d START: %lf\n", new_request->type, new_request->id, new_request->start_time);
                    struct event *next_death_event = malloc(sizeof(struct event));
                    next_death_event->type = '1';
                    next_death_event->class = 'X';
                    next_death_event->time = current_event->time + new_request->service_time;
                    next_death_event->id = current_event->id;
                    next_death_event->next_event = NULL;
                    timeline = add_event(timeline, next_death_event);
                    server = new_request;
                }else{
                    server = add_request(server, new_request);
                }
                
            } else {
                // Birth Event Y
                new_request->service_time = get_exp(1/service_time_Y);
                struct event *next_birth_event = malloc(sizeof(struct event));
                next_birth_event->type = '0';
                next_birth_event->class = 'Y';
                next_birth_event->time = current_event->time + get_exp(lambda_Y);
                next_birth_event->id = current_event->id + 1;
                next_birth_event->next_event = NULL;
                timeline = add_event(timeline, next_birth_event);
                printf("%c%d ARR: %lf LEN: %lf\n", new_request->type, new_request->id, new_request->arrival_time, new_request->service_time);

                if (server == NULL) {
                    new_request->start_time = current_event->time;
                    printf("%c%d START: %lf\n", new_request->type, new_request->id, new_request->start_time);
                    struct event *next_death_event = malloc(sizeof(struct event));
                    next_death_event->type = '1';
                    next_death_event->class = 'Y';
                    next_death_event->time = current_event->time + new_request->service_time;
                    next_death_event->id = current_event->id;
                    next_death_event->next_event = NULL;
                    timeline = add_event(timeline, next_death_event);
                    server = new_request;
                }else{
                    server = add_request(server, new_request);
                }
            }
            queue_size +=1;
        } else if (current_event->type == '1') {
            // Death Event
            struct request *current_request = server;
            server = server->next_request;
            queue_size --;
            current_request->finish_time = current_event->time;

            double random_number = (double)rand() / (double)RAND_MAX;
            bool loop = (random_number < pout);
            if (loop) {
                my_state = update_complete(my_state, current_request);
                printf("%c%d DONE: %lf\n", current_request->type, current_request->id, current_request->finish_time);
            }else{
                my_state = update_looped(my_state, current_request);
                printf("%c%d LOOP: %lf\n", current_request->type, current_request->id, current_request->finish_time);
                struct request *looped_request = malloc(sizeof(struct request));
                looped_request->type = current_request->type;
                looped_request->id = current_request->id;
                looped_request->arrival_time = current_event->time;
                looped_request->start_time = 0;
                looped_request->finish_time = 0;
                looped_request->next_request = NULL;
                if (looped_request->type == 'X'){
                    looped_request->service_time = get_exp(1/service_time_X);
                }else{
                    looped_request->service_time = get_exp(1/service_time_Y);
                }
                server = add_request(server, looped_request);
                queue_size+=1;
            }

            if (server != NULL){
                server->start_time = current_event->time;
                printf("%c%d START: %lf\n", server->type, server->id, server->start_time);
                struct event *next_death_event = malloc(sizeof(struct event));
                next_death_event->type = '1';
                next_death_event->class = server->type;
                next_death_event->time = current_event->time + server->service_time;
                next_death_event->id = server->id;
                next_death_event->next_event = NULL;
                timeline = add_event(timeline, next_death_event);
            }
            
            free(current_request);
            
        } else {
            // Monitor Event
            struct event *next_monitor_event = malloc(sizeof(struct event));
            my_state->num_monitors += 1;
            my_state->num_requests_in_system += queue_size;
            if (server != NULL) {
                my_state->num_requests_in_queue += queue_size-1;
            }
            next_monitor_event->type = '2';
            next_monitor_event->time = current_event->time + get_exp(lambda_X);
            next_monitor_event->next_event = NULL;
            timeline = add_event(timeline, next_monitor_event);
        }
        free(current_event);
    }

    printf("Number of monitors: %lf\n", my_state->num_monitors);
    printf("Number of requests in system: %lf\n", my_state->num_requests_in_system);

    free(timeline);
    free(server);
    printResults(my_state);
    printf("UTIL: %lf\n", my_state->busy_time/length_time);
    free(my_state);
}

int main(int argc, char **argv){
    double length_time;
    double lambda_X;
    double lambda_Y;
    double service_time_X;
    double service_time_Y;
    double pout;
    
    length_time = atof(argv[1]);
    lambda_X = atof(argv[2]);
    lambda_Y = atof(argv[3]);

    service_time_X = atof(argv[4]);
    service_time_Y = atof(argv[5]);
    char *policy = argv[6];

    pout = atof(argv[7]) + 0.001;

    printf("what's up\n");

    
    /**
    length_time = 10000;
    lambda_X = 1;
    lambda_Y = 0.5;
    service_time_X = 0.1;
    service_time_Y = 0.2;
    pout = 0.3;
    **/
    
    
    simulate(
        length_time,
        lambda_X,
        lambda_Y,
        service_time_X,
        service_time_Y,
        pout
    );
    
    printf("%lf\n", pout);
}
