//
// Created by kowal on 16.01.18.
//

void async_sender(const char * mq_name) {
    switch(fork()) {
        case -1:
            syserr("TEST: Error in fork");
        case 0:
            // child
            exit(0);
        default:
            break;
    }
}

int main() {
    return 0;
}