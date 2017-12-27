//
// Created by kowal on 27.12.17.
//

#include <stdbool.h>

int main() {
    // TODO: Load description from stdin
    // TODO: Create mq for incoming requests

    // TODO: Move used standards and #DEFINEs to separate file with config
    /**
     * Request form: "[TYPE]-[PID]-[DATA]", where:
     * TYPE : {START_VALIDATION, FINISH_VALIDATION, HALT} (1. is sent from tester, 2. from run, 3. sent from tester)
     * PID : (pid of a tester requesting the validation)
     *
     */

    // TODO: HALT flag for knowing when to stop, child counter for knowing exactly when to stop
    while(true) {
        // TODO: Handle START_VALIDATION by creating an answer mq, and spinning up a run process with provided word and stored description
        // TODO: Handle FINISH_VALIDATION by pushing an answer into correct answer mq, and updating local logs
        // TODO: Handle HALT by setting local flag which is checked before processing any new word
    }

    return 0;
}