//
// Created by kowal on 27.12.17.
//

#include <stdio.h>
#include <zconf.h>
#include <mqueue.h>
#include "automaton.h"
#include "err.h"
#include "validator_queues.h"


int main() {
    char * response_q_name = create_response_queue_name(getpid());
    mqd_t response_q = mq_open(response_q_name, O_RDONLY | O_CREAT);
    if(response_q == -1) {
        syserr("TESTER: Failed to create response queue");
    }

    char * request_q_name = PW_VQ_REQUEST_NAME_PREFIX;
    mqd_t request_q = mq_open(request_q_name, O_WRONLY);
    if(request_q == -1) {
        syserr("TESTER: Failed to create requests queue");
    }

    size_t awaiting_responses = 0;
    char input_buffer[STR_LEN_MAX];
    char send_buffer[PW_VQ_REQUEST_BUFFSIZE];
    while(fgets(input_buffer, sizeof(input_buffer), stdin)) {
        printf("%s", input_buffer);

        mq_send(request_q, input_buffer, strlen(input_buffer), 1);
        // TODO: Send request to validator server
        // TODO: Make sure to process empty strings correctly
    }
    while(awaiting_responses > 0) {
        // TODO: Wait for responses from validator server
        // TODO: This may stall response queue when a lot of words are validated simultaneously
        awaiting_responses--;
    }

    // clean queues, requests queue is closed by server (validator)
    if (mq_close(response_q)) syserr("TESTER: Failed to close queue");
    if (mq_unlink(response_q_name)) syserr("TESTER: Failed to unlink queue");
    // clean memory
    free(response_q_name);
    return 0;
}
