# PW: Validator (kk385830)  
This is a detailed overview of my concurrent programming project, which was to create 3 applications (**tester**, **validator**, and **run**) that perform a concurrent word validation by given automata, and communicate using POSIX message queues.  
  
In this overview, I will refer to the 3 applications as the applications, and to the libraries I decided to implement as libraries.  

# Assumptions  
## General  

**Number of processes**  
I assumed **there can only be one validator**, this can be easily extended to one validator per one tester by providing the tester with name of validator message queue in program arguments.  
Number of testers is not limited by the implementation, as well as number of runs (which depends on number of words send to the validator, and their possible states) - I assumed **the programs are given enough resources by the system to run smoothly**.  In particular, the **process limit** and **message queue limit** must be high enough to accommodate all created queues and processes.   
  
**Message queue limitations**  
While number of processes should be big enough on most modern computers, the number of message queues is usually small by default, and our applications create **one message queue per each tester and validator application instance** (not to confuse with the number of child processes created by them). Another theoretically possible restriction is size of message queue message, but as word lengths are capped at 1000 chars this should not be a problem (default POSIX mq message size is 8KB on most modern systems).    
  
**Application behavior**  
I assumed that the validator should work forever, unless one of the tester receives a **!** marking the end of word processing.  
I also assumed **all programs will receive a properly-formatted input**. If they don’t, applications’ behavior is undefined (if compiled without `-DNDEBUG`, assertions should prevent most of possible mistakes, but they don’t guarantee clearing system resources properly). Important thing is, that even in case of bad user input, if a system call error occurs, applications will free system resources for other programs before exiting (see next section for details).

## Synchronization

In order to allow most fluent flow of data between processes, **all blocking tasks in validator must be performed asynchronously** to prevent a deadlock (by “asynchronously” I mean by creating a fork to perform the task, that will be collected at the end of lifetime of the main process). In main process, validator only waits in a loop for arriving messages (occasionally called *“requests”* within documentation), and upon receiving a request it updates logs and creates child to perform appropriate task.  
  
This approach guarantees incredibly fast processing, however it poses some serious challenges for synchronization of messages, in particular:  

- when **fork sending a message from validator to tester** needs to update number of sent messages in parent process  
  - this is achieved by using **custom real time signals** with **`*sa_restart*` flag enabled to send information about each successful request  
  - similar solution is implemented in **tester**  
- when **performing an exec call to create run** and sending a message through pipe  
  - this is handled by 2 forks: one to create a pipe and other to exec a run  
- **receiving a “!”** (in documentation called *“halt signal”* or *“halt flag”*) from one of the testers and properly ending communication with each of them  
  - even with just one tester messages can get mixed up if there is a large amount of them, and the message queue is blocked - asynchronous **senders can be woken up at different order than that of their creation**  
  - first attempt to solve this was to implement a tester-side counting and server *“complete”* callback to tester after receiving a halt flag   
  - final solution: **passing expected number of messages to be received by the tester** along with message itself and flag. This allows tester to know exactly how much more messages will be received in case of *“completed”* message arriving before some of them, and guarantees that before shutting down communication between validator and tester, both processes have same expectations as to how much more messages will be sent.  

  
Aside from that, there are also some important solutions implemented in the tester:

- tester has 2 processes - a main one handling messages received from the validator. This process closes input file descriptor immediately after creating child process, in order to prevent a data race. Before that, main process sets a process mask for handling different signals.
- child process is responsible for reading standard input and passing read words to the validator until en EOF or **“!”** is reached. After successful send, it **notifies parent process using custom signal**, to make sure that the count of sent messages is correct. Last message is send with a special flag, making sure that the validator will no longer wait for more messages from that process.  
- When parent process receives a *“completed”* flag from validator, it **sends different rt-signal to its child**, making sure it will not send useless messages.  

  
In terms of synchronization, there are no interesting solutions in run - it is a basic recursive forking process, it waits for information from validator and sends validation results back synchronously from main process.  

## Error handling  

Within the project I tried to follow a convention that library functions should not handle errors, but instead pass them on to the applications to be handled correctly.  

**Libraries**  
Each library function performing system calls, has an extra argument `bool * err`.
In case of a system call failure, err is set to `true` and function returns immediately. In that case:   

- there are no guarantees as to what is a return value of failed functions  
- that error has to be immediately handled or passed further on  

  
In `config.h` I defined several macros useful for error handling, as well as logging.  
  
**Applications**  
Each application attempts to handle errors in a way that minimizes total errors within the whole project environment, and its impact on the system:  

- **validator** will kill any known processes in case of a system call error, which results in:  
  - no zombie testers or runs in case of validator fail  
  - if a tester is not known to validator, it means it has not sent any message yet - upon sending a message, it will receive error from closed validator message queue and terminate  
- **testers** do not spread errors to other applications (including other testers):  
  - if a tester fails it would be unreasonable to kill validator, which may further serve other testers  
  - error in a single tester will not kill the validator  
- **runs** receive validator main process pid via program aruments, which they use to:  
  - kill validator immediately (via signal, this triggers validator to clear it’s fork children and testers)  
  - exit immediately, so that if error occurs in a child run process, the parent will soon exit too  

In every error testing scenario that I tried to perform, system resources (message queues, processes) were freed as expected.  
  
**Memory**  
Validator is the only application using dynamic memory within this project. In case of error, it cannot be freed as `free()` is not a safe async function. In this case, I rely on operating system to manage memory freeing in case of error. Child processes of validator do not access its dynamic memory, and in case of their failure it should not be affected as well (as far as I know, most system implement copy-on-write shared memory for forks).  

# Project structure  
## Applications  

All applications’ helper functions are documented in detail within application source code. Functions that perform blocking tasks (long system calls, etc.) are marked as *“Blocking”* in their documentation, and asynchronous functions are marked as *“Asynchronous”*.  Not all functions implement default error throwing and catching behavior, all such cases are documented in code too.  

Besides helper functions that serve rather visual than functional purpose, applications also use a considerable amount of signal handlers that make sure all possible resources are freed in case of error.  
  
**Tester**  
Loads words from `stdin` until `EOF` is reached. To prevent blocking and possible deadlocks, tester is actually split into 2 processes - one reading words from `stdin` and sending them to the validator, the other waiting for messages from validator and outputting answers.  
  
**Validator**  
Server working as a middleware between validators and runs. It waits for messages in an infinite loop, and if a received request requires execution of blocking function, creates a fork child to perform that task asynchronously. Pipes and program arguments are used to pass information to runs.  
  
**Run**  
Run is an application intended only to be created by validator. It receives automaton description and word via pipe, and knows pid of its creator passed via program arguments. Runs on a given word are performed concurrently, and may require a considerable amount of system resources to do so (when a word validation reaches a state which has $$N$$ possible transitions, it will create $$N-1$$ fork children). Children of run process communicate via exit codes - this is not the most elegant, but definitely the simplest solution, as no error code inspection is required when collecting child processes: signal-based error handling guarantees validator will receive an error message before a run returns.  

## Libraries  

Detailed documentation of library functions and structures is available inside their header files. All library functions that perform blocking tasks (long system calls, etc.) are marked as *“Blocking”* in their documentation.  
  
**Config**  
A config header file, containing useful macros for error handling, as well as some functions for logging and general assumptions used throughout entire project.  
  
**Tester MQ**  
POSIX message queue wrapper designed for tester as a receiving application. Allows creating multiple message queues for each running tester, by using their pid for identification. Library follows standard error behavior described above (see Assumptions→Error Handling).  
  
**Validator MQ**  
POSIX message queue wrapper designed for validator as a receiving application. Unlike **tester MQ**, it creates a message queue with pre-defined name, as I made an assumption that only one validator will run at any given time. Besides that, it behaves exactly like **tester mq**.  
   
**Tester List**  
Linked list of `tester_t` structures, that represent tester data held in **validator** application. These are necessary to keep track of number of request sent to and from **testers** as well as to know their pids. Linked list might not be the fastest implementation for this task, but given the restrictions imposed by most systems on message queue numbers, it did not seem reasonable to go for a faster solution like hashing tester pids into an array. Library follows standard error behavior.  
  
**Pid List**  
Linked list implemented for **validator** to be able to signal child processes in case of system call error. Despite being a linked list, this limited usage does not impact performance of the application - new items are added at the beginning in O(1), and long operations (traversal and deletion) are only performed at the end of lifetime of the validator app. Interface as well as error handling is similar to that of a **tester list**.  
  
    
    
    
-----  
    
This project was completed by Krzysztof Kowalczyk  
(kk385830@students.mimuw.edu.pl | k.kowaalczyk@gmail.com)  
as a assignment for concurrent programming course at University of Warsaw.  
Description of assignment is available here:  
https://www.mimuw.edu.pl/~mp249046/teaching/pw2017z/zadania/zadanie3/task3.html  
    
-----  
      
Copyright (c) Krzysztof Kowalczyk, all rights reserved.  


