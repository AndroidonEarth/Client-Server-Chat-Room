/*
 * Program name: chatclient
 * Author: Andrew Swaim
 * Date: October 2019
 * Description: A chat client that attempts to connect to a host at
 *     a specified host name and port number. Once the connection is
 *     established, can chat back and forth with the other host until
 *     one of them sends a "\quit" command, in which case the
 *     connection is closed and this chat client exits out.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MSGLEN 500 // max message length for sending or receiving
#define DIGLEN 3   // max number of digits for the length of the message (i.e. "500")
#define USRLEN 10  // max username length for client and server

typedef enum { false, true} bool; // bool type for C89/C99 compilation

/*
 * Declarations
 */
int join(char *host, char *port);
void getUser(char *name);
void chat(int conn, char* user, char* server);
bool checkQuit(char *msg);
int sendMsg(int conn, char *msg, int *len);


/*
 * Main
 */
int main(int argc, char *argv[]) {

    // validate num of command line args
    if (argc != 3) {
        fprintf(stderr, "USAGE: %s [host] [port]\n", argv[0]); exit(1);
    }
    // validate port num
    int port = atoi(argv[2]);
    if (port < 0 || port > 65535) {
        fprintf(stderr, "ERROR, invalid port: %d\n", port); exit(1);
    }
    if (port < 50000) {
        printf("WARNING, recommended to use port number above 50000\n");
    }
    
    printf("Welcome to chatclient, your friendly chatting client!\n");
    
    char user[USRLEN+2]; // 1 extra space for validating the length
    getUser(user);

    // attempt to connect to the host and port
    int conn = join(argv[1], argv[2]);

    // handshake with the server
    char server[USRLEN+1];
    memset(server, '\0', sizeof(server));
    if ((send(conn, user, strlen(user), 0) == -1) 
        || (recv(conn, server, USRLEN, 0) == -1)) {
        perror("ERROR, could not handshake with server\n");
        exit(1);
    }

    printf("Now chatting with %s, say hello!\n", server);
    chat(conn, user, server); // begin chat loop
    close(conn); // if the chat loop ends normally close the connection

    printf("chatclient is exiting... Goodbye!\n");
    return 0;
}

/*
 * Definitions
 */

// Name:
// Desc:
// Pre :
// Post:
// Rtrn:
int join(char *host, char *port) {
    
    // Socket setup and connection from Beej's section A Simple Stream Client
    // https://beej.us/guide/bgnet/html/#a-simple-stream-client
    int sock;
    struct addrinfo addr, *serv, *ptr;

    // setup server address struct
    memset(&addr, 0, sizeof(addr));
    addr.ai_family = AF_UNSPEC;
    addr.ai_socktype = SOCK_STREAM;

    // get address info of the server host
    if (getaddrinfo(host, port, &addr, &serv) != 0) {
        fprintf(stderr, "ERROR, could not get address info for host: %s port: %s\n", host, port);
        exit(1);
    }

    // loop through all possible connections and try to connect
    for (ptr = serv; ptr != NULL; ptr = ptr->ai_next) {
        if ((sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) >= 0) {
            if (connect(sock, ptr->ai_addr, ptr->ai_addrlen) == -1) {
                close(sock); // make sure to close connection if connect resulted in err
            }
            else {
                break; // connection successful, exit loop
            }
        }
    }

    if (ptr == NULL) {
        fprintf(stderr, "ERROR, failed to connect to host: %s on port: %s\n", host, port);
        exit(1);
    }

    return sock;
}

// Name:
// Desc:
// Pre :
// Post:
// Rtrn:
void getUser(char *name) {
    
    bool valid = false;

    // loop until a valid username is entered
    while (!valid) {
        printf("Please enter a one word username, up to 10 characters\n");
        memset(name, '\0', sizeof(name));
        fgets(name, sizeof(name), stdin);
        name[strcspn(name, "\n")] = '\0'; // remove any trailing newlines if present
        valid = true; // assume name is good until checks prove otherwise

        // validate that name is one word up to 10 characters
        if (strlen(name) >= 1 && strlen(name) <= 10) { 
            int i;
            for (i = 0; i < strlen(name); i++) {
               if (!isalpha(name[i])) {
                   printf("Invalid username format: username can only contain letters\n");
                   valid = false;
                   break;
               }
            }
        } else {
            printf("Invalid username format: must be between 1 and 10 characters\n");
            valid = false;
        }
    }
}

// Name:
// Desc:
// Pre :
// Post:
// Rtrn:
void chat(int conn, char *user, char *server) {

    int usrLen = strlen(user);
    char buf[MSGLEN-usrLen-1]; // leave room for prepending the user name + "> "
    char msg[MSGLEN+1]; // username + buffer = full message
    char rsp[MSGLEN+2]; // the server response
    int len;

    while (true) {
        
        // get user message to send
        memset(buf, '\0', sizeof(buf));
        printf("%s> ", user); 
        fflush(stdout);
        fgets(buf, sizeof(buf), stdin);
        buf[strcspn(buf, "\n")] = '\0'; 

        // create full message with prepended username
        memset(msg, '\0', sizeof(msg));
        snprintf(msg, sizeof(msg), "%s> %s", user, buf);

        // send the message
        len = strlen(msg);
        if (sendMsg(conn, msg, &len) == -1) {
            fprintf(stderr, "ERROR: only %d characters were sent to server\n", len);
            close(conn);
            exit(1);
        }
       
        // check if quit command was entered by client
        if (checkQuit(buf)) { 
            printf("You have left the chatroom.\n");
            break; 
        }
        else {
            // get server response
            memset(rsp, '\0', sizeof(rsp));
            if (recv(conn, rsp, MSGLEN, 0) == -1) {
                perror("ERROR: unable to receive message from server\n");
                close(conn);
                exit(1);
            }
            // check if server sent quit command
            if (checkQuit(rsp)) {
                printf("%s has ended the chat.\n", server);
                break; 
            } 
            else {
                printf("%s\n", rsp);
            }
        }
    }
}

// Name:
// Desc:
// Pre :
// Post:
// Rtrn:
bool checkQuit(char *msg) {

    char cmd[4] = "quit";

    int i, j;
    bool cmp = false;
    for (i = 0, j = 0; i < strlen(msg) && j < 4; i++) {
        if (!cmp) {
            if (msg[i] == ' ' || msg[i] == '\t') {} // skip whitespace
            else if (msg[i] == '\\') { cmp = true; } // start of '\quit' message encountered, start comparing
        } else if (tolower(msg[i]) == cmd[j])  {
            j++;
        } else {
            return false;
        }
    }
    if (cmp) { return true; } // if the comparison checks passed
    else { return false; } // if not comparison every happened (no '\' was encountered)
}

// Name:
// Desc:
// Pre :
// Post:
// Rtrn:
int sendMsg(int conn, char *msg, int *len) {
    
    // handling partial sends adpted from Beej's guide
    // Handling Partial send()s
    // https://beej.us/guide/bgnet/html/#sendall
    int total = 0;
    int rem = *len;
    int n;

    while (total < *len) {
        n = send(conn, msg+total, rem, 0);
        if (n == -1) { break; }
        total += n;
        rem -= n;
    }

    *len = total;

    return ((n == -1) ? -1 : 0);
}
