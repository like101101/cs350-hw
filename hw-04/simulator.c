#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
// Other Libraries
#include <sys/types.h>
#include <unistd.h>
#include <math.h>


// Class Definitions
struct event {
    u_int8_t type; // 0 = birth_event, 1 = death_event_at_s0, 2 = death_event_at_s1, 3 = death_event_at_s2, 4 = monitor_event
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


struct server {
    struct request current_request;
    struct request *queue;
};

struct state {
    double num_requests_finished_X;
    double num_requests_finished_Y;
    double num_monitors;
    double total_waiting_time_X;
    double total_waiting_time_Y;
    double total_response_time_X;
    double total_response_time_Y;
    double dropped_requests;
};

struct server_state {
    double num_requests_in_queue;
    double num_requests_in_system;
    double busy_time;
};



// Function Definitions
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



struct state* update(struct state* current_state, struct request* current_request, bool completed) {
    if (current_request->type == 'X') {
        if (completed) {
            current_state->num_requests_finished_X += 1;
        }
        current_state->total_waiting_time_X += current_request->start_time - current_request->arrival_time;
        current_state->total_response_time_X += current_request->finish_time - current_request->arrival_time;
    } else {
        if (completed) {
            current_state->num_requests_finished_Y += 1;
        }
        current_state->total_waiting_time_Y += current_request->start_time - current_request->arrival_time;
        current_state->total_response_time_Y += current_request->finish_time - current_request->arrival_time;
    }
    return current_state;
} 


void print_results (
    struct state* total_state, 
    struct server_state* s0_state, 
    struct server_state* s1_state,
    struct server_state* s2_state,
    double total_time
) {
    // for s0
    double s0_QAVG = s0_state->num_requests_in_system / total_state->num_monitors;
    double s0_WAVG = s0_state->num_requests_in_queue / total_state->num_monitors;
    printf("S0 UTIL: %lf\n", s0_state->busy_time/total_time);
    printf("S0 QAVG: %lf\n", s0_QAVG);
    printf("S0 WAVG: %lf\n", s0_WAVG);

    // for s1
    double s1_QAVG = s1_state->num_requests_in_system / total_state->num_monitors;
    double s1_WAVG = s1_state->num_requests_in_queue / total_state->num_monitors;
    printf("S1 UTIL: %lf\n", s1_state->busy_time/total_time);
    printf("S1 QAVG: %lf\n", s1_QAVG);
    printf("S1 WAVG: %lf\n", s1_WAVG);

    // for s2
    double s2_QAVG = s2_state->num_requests_in_system / total_state->num_monitors;
    double s2_WAVG = s2_state->num_requests_in_queue / total_state->num_monitors;
    printf("S2 UTIL: %lf\n", s2_state->busy_time/total_time);
    printf("S2 QAVG: %lf\n", s2_QAVG);
    printf("S2 WAVG: %lf\n", s2_WAVG);
    printf("S2 DROPPED: %lf\n", total_state->dropped_requests);

    // For overall
    double x_TRESP = total_state->total_response_time_X / total_state->num_requests_finished_X;
    double x_TWAIT = total_state->total_waiting_time_X / total_state->num_requests_finished_X;
    double y_TRESP = total_state->total_response_time_Y / total_state->num_requests_finished_Y;
    double y_TWAIT = total_state->total_waiting_time_Y / total_state->num_requests_finished_Y;

    printf("TRESP X: %lf\n", x_TRESP);
    printf("TWAIT X: %lf\n", x_TWAIT);
    printf("TRESP Y: %lf\n", y_TRESP);
    printf("TWAIT Y: %lf\n", y_TWAIT);
    printf("QAVG: %lf\n", s0_QAVG + s1_QAVG + s2_QAVG);

}



void simulate(
    double length_time,
    double lambda_X,
    double lambda_Y,
    double service_time_X,
    double service_time_Y,
    double p_to_s1_X,
    double p_to_s1_Y,
    double s1_service_time,
    double s2_service_time,
    double q_size_limit,
    double p_back_from_s1,
    double p_back_from_s2
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
    first_monitor_event->type = '4';
    first_monitor_event->time = 0;
    first_monitor_event->next_event = NULL;

    struct server_state *s0_state = malloc(sizeof(struct server_state));
    memset(s0_state, 0, sizeof(struct server_state));

    struct server_state *s1_state = malloc(sizeof(struct server_state));
    memset(s1_state, 0, sizeof(struct server_state));

    struct server_state *s2_state = malloc(sizeof(struct server_state));
    memset(s2_state, 0, sizeof(struct server_state));

    struct state *total_state = malloc(sizeof(struct state));
    memset(total_state, 0, sizeof(struct state));

    struct request *s0 = malloc(sizeof(struct request));
    s0 = NULL;
    double s0_queue_size = 0;

    struct request *s1 = malloc(sizeof(struct request));
    s1 = NULL;
    double s1_queue_size = 0;

    struct request *s2 = malloc(sizeof(struct request));
    s2 = NULL;
    double s2_queue_size = 0;


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
            struct event *next_birth_event = malloc(sizeof(struct event));
            if (current_event->class == 'X') {
                // Birth Event X
                new_request->service_time = get_exp(1/service_time_X);
                next_birth_event->time = current_event->time + get_exp(lambda_X);
            } else {
                // Birth Event Y
                new_request->service_time = get_exp(1/service_time_Y);
                next_birth_event->time = current_event->time + get_exp(lambda_Y);
            }
            
            next_birth_event->type = '0';
            next_birth_event->class = current_event->class;
            next_birth_event->id = current_event->id + 1;
            next_birth_event->next_event = NULL;
            timeline = add_event(timeline, next_birth_event);
            printf("%c%d ARR: %lf LEN: %lf\n", new_request->type, new_request->id, new_request->arrival_time, new_request->service_time);
            if (s0 == NULL) {
                new_request->start_time = current_event->time;
                printf("%c%d START S0: %lf\n", new_request->type, new_request->id, new_request->start_time);
                struct event *next_death_event = malloc(sizeof(struct event));
                next_death_event->type = '1';
                next_death_event->class = current_event->class;
                next_death_event->time = current_event->time + new_request->service_time;
                next_death_event->id = current_event->id;
                next_death_event->next_event = NULL;
                timeline = add_event(timeline, next_death_event);
                s0 = new_request;
            }else{
                s0 = add_request(s0, new_request);
            }
            s0_queue_size++;
             
        } else if (current_event->type == '1') {
            // Death Event in server 0
            struct request *current_request = s0;
            s0 = s0->next_request;
            s0_queue_size--;
            current_request->finish_time = current_event->time;
            s0_state->busy_time += current_request->service_time;
            total_state = update(total_state, current_request, false);
            double random_number = (double)rand() / (double)RAND_MAX;
            bool go_to_s1 = false;

            if (current_request->type == 'X'){
                go_to_s1 = random_number < p_to_s1_X;
            }else{
                go_to_s1 = random_number < p_to_s1_Y;
            }

            struct request *second_request = malloc(sizeof(struct request));
            second_request->type = current_request->type;
            second_request->id = current_request->id;
            second_request->arrival_time = current_event->time;
            second_request->start_time = 0;
            second_request->finish_time = 0;
            second_request->next_request = NULL;

            if (go_to_s1) {
                printf("%c%d FROM S0 TO S1: %lf\n", current_request->type, current_request->id, current_request->finish_time);
                second_request->service_time = get_exp(1/s1_service_time);
                if (s1 == NULL) {
                    second_request->start_time = current_event->time;
                    printf("%c%d START S1: %lf\n", second_request->type, second_request->id, second_request->start_time);
                    struct event *next_death_event = malloc(sizeof(struct event));
                    next_death_event->type = '2';
                    next_death_event->class = current_event->class;
                    next_death_event->time = current_event->time + second_request->service_time;
                    next_death_event->id = current_event->id;
                    next_death_event->next_event = NULL;
                    timeline = add_event(timeline, next_death_event);
                    s1 = second_request;
                }else{
                    s1 = add_request(s1, second_request);
                    
                }
                s1_queue_size++;
            }else{
                printf("%c%d FROM S0 TO S2: %lf\n", current_request->type, current_request->id, current_request->finish_time);
                second_request->service_time = get_exp(1/s2_service_time);
                if (s2_queue_size == q_size_limit){
                    total_state->dropped_requests++;
                    printf("%c%d DROP S2: %lf\n", current_request->type, current_request->id, current_request->finish_time);
                }else{
                    if (s2 == NULL) {
                        second_request->start_time = current_event->time;
                        printf("%c%d START S2: %lf\n", second_request->type, second_request->id, second_request->start_time);
                        struct event *next_death_event = malloc(sizeof(struct event));
                        next_death_event->type = '3';
                        next_death_event->class = current_event->class;
                        next_death_event->time = current_event->time + second_request->service_time;
                        next_death_event->id = current_event->id;
                        next_death_event->next_event = NULL;
                        timeline = add_event(timeline, next_death_event);
                        s2 = second_request;
                    }else{
                        s2 = add_request(s2, second_request);
                        
                    }
                    s2_queue_size++;
                }
            }
                
            
            if (s0 != NULL){
                s0->start_time = current_event->time;
                printf("%c%d START S0: %lf\n", s0->type, s0->id, s0->start_time);
                struct event *next_death_event = malloc(sizeof(struct event));
                next_death_event->type = '1';
                next_death_event->class = s0->type;
                next_death_event->time = current_event->time + s0->service_time;
                next_death_event->id = s0->id;
                next_death_event->next_event = NULL;
                timeline = add_event(timeline, next_death_event);
            }
            
            free(current_request);

        } else if (current_event->type == '2') {
            struct request *current_request = s1;
            s1 = s1->next_request;
            s1_queue_size --;
            current_request->finish_time = current_event->time;
            s1_state->busy_time += current_request->service_time;

            double random_number = (double)rand() / (double)RAND_MAX;
            bool goback = (random_number < p_back_from_s1);
            if (!goback) {
                total_state = update(total_state, current_request, true);
                printf("%c%d DONE S1: %lf\n", current_request->type, current_request->id, current_request->finish_time);
            }else{
                total_state = update(total_state, current_request, false);
                printf("%c%d FROM S1 TO S0: %lf\n", current_request->type, current_request->id, current_request->finish_time);
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

                if (s0 == NULL) {
                    looped_request->start_time = current_event->time;
                    printf("%c%d START S0: %lf\n", looped_request->type, looped_request->id, looped_request->start_time);
                    struct event *next_death_event = malloc(sizeof(struct event));
                    next_death_event->type = '1';
                    next_death_event->class = current_event->class;
                    next_death_event->time = current_event->time + looped_request->service_time;
                    next_death_event->id = current_event->id;
                    next_death_event->next_event = NULL;
                    timeline = add_event(timeline, next_death_event);
                    s0 = looped_request;
                }else{
                    s0 = add_request(s0, looped_request);
                    
                }
                s0_queue_size++;
            }

            if (s1 != NULL){
                s1->start_time = current_event->time;
                printf("%c%d START S1: %lf\n", s1->type, s1->id, s1->start_time);
                struct event *next_death_event = malloc(sizeof(struct event));
                next_death_event->type = '2';
                next_death_event->class = s1->type;
                next_death_event->time = current_event->time + s1->service_time;
                next_death_event->id = s1->id;
                next_death_event->next_event = NULL;
                timeline = add_event(timeline, next_death_event);
            }
            
            free(current_request);
        
        } else if (current_event->type == '3') {
            struct request *current_request = s2;
            s2 = s2->next_request;
            s2_queue_size --;
            current_request->finish_time = current_event->time;
            s2_state->busy_time += current_request->service_time;

            double random_number = (double)rand() / (double)RAND_MAX;
            bool goback = (random_number < p_back_from_s2);
            if (!goback) {
                total_state = update(total_state, current_request, true);
                printf("%c%d DONE S2: %lf\n", current_request->type, current_request->id, current_request->finish_time);
            }else{
                total_state = update(total_state, current_request, false);
                printf("%c%d FROM S2 TO S0: %lf\n", current_request->type, current_request->id, current_request->finish_time);
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

                if (s0 == NULL) {
                    looped_request->start_time = current_event->time;
                    printf("%c%d START S0: %lf\n", looped_request->type, looped_request->id, looped_request->start_time);
                    struct event *next_death_event = malloc(sizeof(struct event));
                    next_death_event->type = '1';
                    next_death_event->class = current_event->class;
                    next_death_event->time = current_event->time + looped_request->service_time;
                    next_death_event->id = current_event->id;
                    next_death_event->next_event = NULL;
                    timeline = add_event(timeline, next_death_event);
                    s0 = looped_request;
                }else{
                    s0 = add_request(s0, looped_request);
                    
                }
                s0_queue_size++;
            }

            if (s2 != NULL){
                s2->start_time = current_event->time;
                printf("%c%d START S2: %lf\n", s2->type, s2->id, s2->start_time);
                struct event *next_death_event = malloc(sizeof(struct event));
                next_death_event->type = '3';
                next_death_event->class = s2->type;
                next_death_event->time = current_event->time + s2->service_time;
                next_death_event->id = s2->id;
                next_death_event->next_event = NULL;
                timeline = add_event(timeline, next_death_event);
            }
            
            free(current_request);

        } else {
            // Monitor Event
            struct event *next_monitor_event = malloc(sizeof(struct event));
            total_state->num_monitors += 1;
            s0_state->num_requests_in_system += s0_queue_size;
            s1_state->num_requests_in_system += s1_queue_size;
            s2_state->num_requests_in_system += s2_queue_size;
            if (s0 != NULL) { s0_state->num_requests_in_queue += s0_queue_size-1;}
            if (s1 != NULL) { s1_state->num_requests_in_queue += s1_queue_size-1;}
            if (s2 != NULL) { s2_state->num_requests_in_queue += s2_queue_size-1;}
            next_monitor_event->type = '4';
            next_monitor_event->time = current_event->time + get_exp(lambda_X);
            next_monitor_event->next_event = NULL;
            printf("Total number of requsts in s0: %lf\n", s0_queue_size);
            timeline = add_event(timeline, next_monitor_event);
        }

        free(current_event);
    }


    free(timeline);
    free(s0);
    free(s1);
    free(s2);
    print_results(total_state, s0_state, s1_state, s2_state, length_time);
    free(total_state);
    free(s0_state);
    free(s1_state);
    free(s2_state);
    return;
}

int main(int argc, char **argv){
    double length_time;
    double lambda_X;
    double lambda_Y;
    double service_time_X;
    double service_time_Y;
    double p_to_s1_X;
    double p_to_s1_Y;
    double s1_service_time;
    double s2_service_time;
    double q_size_limit;
    double p_back_from_s1;
    double p_back_from_s2;
    char* policy;

    bool DEBUG = false;

    if (DEBUG){
        length_time = 10000;
        lambda_X = 1;
        lambda_Y = 0.5;
        service_time_X = 0.1;
        service_time_Y = 0.2;
        policy = "FCFS";
        p_to_s1_X = 0.5;
        p_to_s1_Y = 0.5;
        s1_service_time = 0.1;
        s2_service_time = 0.1;
        q_size_limit = 100;
        p_back_from_s1 = 0.5;
        p_back_from_s2 = 0.5;
    }else{
        length_time = atof(argv[1]);
        lambda_X = atof(argv[2]);
        lambda_Y = atof(argv[3]);

        service_time_X = atof(argv[4]);
        service_time_Y = atof(argv[5]);
        policy = argv[6];

        p_to_s1_X = atof(argv[7]);
        p_to_s1_Y = atof(argv[8]);
        s1_service_time = atof(argv[9]);
        s2_service_time = atof(argv[10]);
        q_size_limit = atof(argv[11]);
        p_back_from_s1 = atof(argv[12])+0.001;
        p_back_from_s2 = atof(argv[13])+0.001;
    }
    
    simulate(
        length_time,
        lambda_X,
        lambda_Y,
        service_time_X,
        service_time_Y,
        p_to_s1_X,
        p_to_s1_Y,
        s1_service_time,
        s2_service_time,
        q_size_limit,
        p_back_from_s1,
        p_back_from_s2
    );
}
