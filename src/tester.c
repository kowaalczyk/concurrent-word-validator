//
// Created by kowal on 27.12.17.
//

#include <stdio.h>
#include <zconf.h>
#include <mqueue.h>
#include "automaton.h"
#include "response_queue.h"
#include "request_queue.h"
#include "err.h"


int main() {
    char * response_q_name = create_response_queue_name(getpid());
    mqd_t response_q = mq_open(response_q_name, O_RDONLY | O_CREAT);
    if(response_q == -1) {
        syserr("TESTER: Failed to create response queue");
    }

    char * request_q_name = VALIDATOR_INCOMING_REQUESTS_MQ_NAME;
    mqd_t request_q = mq_open(request_q_name, O_WRONLY);
    if(request_q == -1) {
        syserr("TESTER: Failed to create requests queue");
    }

    size_t awaiting_responses = 0;
    char buffer[STR_LEN_MAX];
    while(fgets(buffer, sizeof(buffer), stdin)) {
        printf("%s", buffer);
        mq_send(request_q, buffer, strlen(buffer), 1);
        // TODO: Before sending requests, check if any responses are present (server won't have to wait)
        // TODO: Send request to validator server
        // TODO: Make sure to process empty strings correctly
    }
    // TODO: Wait for responses from validator server

    // clean queues, requests queue is closed by server (validator)
    if (mq_close(response_q)) syserr("TESTER: Failed to close queue");
    if (mq_unlink(response_q_name)) syserr("TESTER: Failed to unlink queue");
    // clean memory
    free(response_q_name);
    return 0;
}
