//
// Created by kowal on 27.12.17.
//

#include <stdio.h>


int main() {
    char buffer[1000]; // TODO: Constantize

    // TODO: Open queue for reading responses before sending actual request to any server
    // (this will not force the server to wait untill a message is received)

    while(fgets(buffer, sizeof(buffer), stdin)) {
        printf("%s", buffer);
        // TODO: Send request to validator server
    }
    // TODO: Wait for responses from validator serven

    return 0;
}
