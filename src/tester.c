//
// Created by kowal on 27.12.17.
//

#include <stdio.h>
#include "automaton.h"


int main() {
    char buffer[STR_LEN_MAX];

    // TODO: Open queue for reading responses before sending actual request to any server
    // (this will not force the server to wait untill a message is received)

    while(fgets(buffer, sizeof(buffer), stdin)) {
        printf("%s", buffer);
        // TODO: Send request to validator server
        // TODO: Make sure to process empty strings correctly
    }
    // TODO: Wait for responses from validator server

    return 0;
}
