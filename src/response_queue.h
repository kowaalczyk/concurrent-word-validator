//
// Created by kowal on 04.01.18.
//

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef PW_VALIDATOR_RESPONSE_QUEUE_H
#define PW_VALIDATOR_RESPONSE_QUEUE_H


/**
 * Response queue naming convention:
 * [tester PID]-[response queue suffix]
 * (PID in dec format with leading zeros to fit sizeof pid_t)
 *
 * Response queue messages contain formatted output, ready to be printed out
 */

#define PID_FORMAT_LENGTH (sizeof(pid_t))
#define VALIDATOR_RESPONSE_QUEUE_NAME_SUFFIX "_validation_response_q"
#define VALIDATOR_RESPONSE_QUEUE_NAME_LENGTH (22+PID_FORMAT_LENGTH)

/// returns new name for response queue for a tester process with given pid
char * create_response_queue_name(pid_t pid) {
    char *ans = malloc(VALIDATOR_RESPONSE_QUEUE_NAME_LENGTH*sizeof(char));
    sprintf(ans, "%0*d%s", (int)sizeof(pid_t), pid, VALIDATOR_RESPONSE_QUEUE_NAME_SUFFIX);
    return ans;
}


#endif //PW_VALIDATOR_RESPONSE_QUEUE_H
