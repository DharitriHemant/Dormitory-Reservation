#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

// Define constants
#define MEMBERS_FILE "./member.txt"
#define LOOPBACK_IP "127.0.0.1"
#define PORT 41435  // the port clients will be connecting to
#define PORT_UDP 44435 // the port subservers will be connecting to
#define SERVER_S_PORT 41435 // UDP port # of server that handles Single rooms
#define SERVER_D_PORT 42435 // UDP port # of server that handles Double rooms
#define SERVER_U_PORT 43435 // UDP port # of server that handles Suite rooms
#define MAX_USERS 50
#define BACKLOG 50   // how many pending connections queue will hold

// Define a structure to hold user information
struct user {
    char usernames[20];
    char passwords[20];
};

// Declare an array to store user information
struct user users[MAX_USERS];

// Function to decrypt a username
void decrypt_User(char *encryptedValue, char *decryptedValue) {
    for (int i = 0; encryptedValue[i] != '\0'; ++i) {
        if (isalpha(encryptedValue[i])) {
            char decrypt = isupper(encryptedValue[i]) ? 'A' : 'a';
            decryptedValue[i] = (encryptedValue[i] - decrypt + 21) % 26 + decrypt;
        } 
        else if (isdigit(encryptedValue[i])) {
            decryptedValue[i] = (encryptedValue[i] - '0' + 3) % 10 + '0';
        } else {
            decryptedValue[i] = encryptedValue[i]; // do nothing for symbols
        }
    }
}

// Function to validate username and password
void validate_Username_Password(char *username, char *password, char message[26], char userType[10]) {
    FILE *file = fopen(MEMBERS_FILE, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }
    printf("Main Server loaded the member list\n");
    int num = 0;
    bool userFound = false;
    bool passwordMatch = false;
    // Loop through the user database
    while (fscanf(file, "%19[^,],%19s ", users[num].usernames, users[num].passwords) == 2)  {
        printf("Read from file: %s, %s\n", users[num].usernames, users[num].passwords);

        if (strcmp(username, users[num].usernames) == 0) {
            userFound = true;

            if (password != 0 && strcmp(password, users[num].passwords) == 0) {
                passwordMatch = true;
                strcpy(message, "Authentication successful");
                strcpy(userType, "Regular"); // Regular user type
                // printf("%s", message);
                printf("Username and password match. Authentication successful\n");
                fclose(file);
                return;

            } else if (password != NULL && !passwordMatch) {
                // If password is not entered, consider the user as a guest
                strcpy(message, "Guest login");
                strcpy(userType, "Guest"); // Guest user type
                printf("Guest login\n");
                fclose(file);
                return;
            }
        }
        num++;
    }
    fclose(file);
    if (!userFound) {
        strcpy(message, "Authentication failed: User not existing");
        printf("%s not registered\n", username);
    } else if (password != NULL && !passwordMatch) {
        strcpy(message, "Authentication failed");
        printf("Username found but password does not match\n");
    } else {
        strcpy(message, "Error: Unexpected condition occurred");
        printf("Unexpected condition occurred\n");
    }
}

// Function to get the IP address from a sockaddr structure
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Function to request room status from a subservers
void request_room_status(char* roomtype, char room_message[50], char Decrypted_Username[50], char userType[10], char choice[12]) {
    char prefix = roomtype[0];
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1) {
        perror("Error creating UDP socket");
        exit(1);
    }
    struct sockaddr_in server_adder;
    server_adder.sin_family = AF_INET;
    char Sub_Server;
    int port_num;
 
        switch (prefix) {
            case 'S':
                inet_pton(AF_INET, LOOPBACK_IP, &server_adder.sin_addr);
                server_adder.sin_port = htons(SERVER_S_PORT);
                Sub_Server = 'S';
                port_num = SERVER_S_PORT;
                break;
            case 'D':
                inet_pton(AF_INET, LOOPBACK_IP, &server_adder.sin_addr);
                server_adder.sin_port = htons(SERVER_D_PORT);
                Sub_Server = 'D';
                port_num = SERVER_D_PORT;
                break;
            case 'U':
                inet_pton(AF_INET, LOOPBACK_IP, &server_adder.sin_addr);
                server_adder.sin_port = htons(SERVER_U_PORT);
                Sub_Server = 'U';
                port_num = SERVER_U_PORT;
                break;
            default:
                printf("Did not find %s in the Room code list\n", roomtype);
                
                return;
        }
        // Send requests to the subservers
        sendto(udp_socket, roomtype, strlen(roomtype), 0, (struct sockaddr*)&server_adder, sizeof(server_adder));
        sendto(udp_socket, choice, strlen(choice), 0, (struct sockaddr*)&server_adder, sizeof(server_adder));
        printf("The main server has received the room status from Server %c using UDP over port %d.\n", Sub_Server, port_num);
        sendto(udp_socket, Decrypted_Username, strlen(Decrypted_Username), 0, (struct sockaddr*)&server_adder, sizeof(server_adder));
        struct sockaddr_in server_adder_len;
        socklen_t server_adder_len_size = sizeof(server_adder);
        // Receive response from the subservers
        ssize_t received_stat = recvfrom(udp_socket, room_message, sizeof(room_message) - 1, 0, 
                                    (struct sockaddr*)&server_adder, &server_adder_len_size);
        if (received_stat == -1) {
            perror("Error receiving the data from Sub-Servers");
        } else {
            room_message[received_stat] = '\0';
        }
    
    close(udp_socket);
}

// Signal handler for SIGCHLD
void sigchld_handler(int s) {
    (void)s; // quiet unused variable warning
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

int main() {
    // Set up signal handler for SIGCHLD
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // Create TCP socket
    int S_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (S_Socket == -1) {
        perror("Error creating socket");
        exit(1);
    }

    // Set up server address
    struct sockaddr_in server_adder;
    server_adder.sin_family = AF_INET;
    server_adder.sin_addr.s_addr = INADDR_ANY;
    server_adder.sin_port = htons(PORT);

    // Bind socket to address
    if (bind(S_Socket, (struct sockaddr*)&server_adder, sizeof(server_adder)) == -1) {
        perror("Error binding socket");
        exit(1);
    }

    // Listen for connections
    if (listen(S_Socket, BACKLOG) == -1) {
        perror("Error listening for connections");
        exit(1);
    }

    printf("The main server is up and running.\n");

    // Main server loop
    while (1) {
        // Accept incoming connection
        int C_Socket = accept(S_Socket, NULL, NULL);
        if (C_Socket == -1) {
            perror("Error accepting connection");
            exit(1);
        }
        printf("Successfully established connection with client over port %d\n", PORT);

        // Fork a child process
        pid_t pid = fork();
        if (pid == -1) {
            perror("Error forking");
            exit(1);
        } else if (pid == 0) { // Child process
            close(S_Socket); // Close listening socket in child process

            // Communication loop with client
            while (1) {
                char data[1024];
                ssize_t received_data = recv(C_Socket, data, sizeof(data), 0);
                if (received_data == -1) {
                    perror("Error receiving data from client");
                    break;
                } else if (received_data == 0) {
                    printf("Client closed connection\n");
                    break;
                }
                data[received_data] = '\0';

                char Username[50];
                char Password[50];
                
                strncpy(Username, strtok(data, ","), sizeof(Username) - 1);
                strncpy(Password, strtok(NULL, ","), sizeof(Password) - 1);

                printf("The main server received the authentication for %s using TCP over port %d.\n", Username, PORT);

                char message[26];
                char Decrypted_Username[25];
                char room_message[50];
                char userType[10];

                validate_Username_Password(Username, Password, message, userType);
                printf("%s\n", message);
                send(C_Socket, message, strlen(message), 0);
                printf("The main server sent the authentication result to the client.\n");

                if (strcmp(message, "Authentication successful") == 0) {
                    decrypt_User(Username, Decrypted_Username);

                    // Room request loop
                    while (1) {
                        char roomtype[4];
                        ssize_t roomtype_info = recv(C_Socket, roomtype, sizeof(roomtype), 0);
                        if (roomtype_info == -1) {
                            perror("Error receiving the room code");
                            break;
                        } else if (roomtype_info == 0) {
                            printf("Client closed connection\n");
                            break;
                        }
                        roomtype[roomtype_info] = '\0';

                        char choice[12];
                        ssize_t user_choice = recv(C_Socket, choice, sizeof(choice), 0);
                        if (user_choice == -1) {
                            perror("Error receiving the user choice");
                            break;
                        } else if (user_choice == 0) {
                            printf("Client closed connection\n");
                            break;
                        }
                        choice[user_choice] = '\0';

                        printf("Main Server received the room request for %s from client using TCP over port: %d\n", roomtype, PORT_UDP);

                        if (strlen(roomtype) > 0) {
                            if (strcmp(choice, "Reservation") == 0) {

                                printf("The main server received a request for %s using TCP over port %d.\n",Decrypted_Username,PORT);
                                request_room_status(roomtype, room_message, Decrypted_Username, userType, choice);
                                printf("The main server sent the response to the client.\n");
                            } else {
                                request_room_status(roomtype, room_message, Decrypted_Username, userType, choice);
                                printf("The main server has received the availability request on Room %s from %s using TCP over port %d.",roomtype,Decrypted_Username,PORT);
                                printf("%s can make an availability\n", Decrypted_Username);
                                printf("The room message is %s\n", room_message);
                            }
                        } else {
                            printf("Received empty room code\n");
                        }
                        send(C_Socket, room_message, strlen(room_message), 0);
                        printf("The main server sent the authentication result to the client.\n");
                    }

                }else if (strcmp(message, "Guest login") == 0) {
                    decrypt_User(Username, Decrypted_Username);

                    // Room request loop
                    while (1) {
                        char roomtype[4];
                        ssize_t roomtype_info = recv(C_Socket, roomtype, sizeof(roomtype), 0);
                        if (roomtype_info == -1) {
                            perror("Error receiving the room code");
                            break;
                        } else if (roomtype_info == 0) {
                            printf("Client closed connection\n");
                            break;
                        }
                        roomtype[roomtype_info] = '\0';

                        char choice[12];
                        ssize_t user_choice = recv(C_Socket, choice, sizeof(choice), 0);
                        if (user_choice == -1) {
                            perror("Error receiving the user choice");
                            break;
                        } else if (user_choice == 0) {
                            printf("Client closed connection\n");
                            break;
                        }
                        choice[user_choice] = '\0';

                        printf("Main Server received the room request for %s from client using TCP over port: %d\n", roomtype, PORT_UDP);

                        if (strlen(roomtype) > 0) {
                            if (strcmp(choice, "Reservation") == 0 || strcmp(userType, "Guest") == 0) {

                                printf("The main server received a request for %s using TCP over port %d.\n",Decrypted_Username,PORT);
                                request_room_status(roomtype, room_message, Decrypted_Username, userType, choice);
                                printf("The main server sent the response to the client.\n");
                            } else {
                                request_room_status(roomtype, room_message, Decrypted_Username, userType, choice);
                                printf("The main server has received the availability request on Room %s from %s using TCP over port %d.",roomtype,Decrypted_Username,PORT);
                                printf("%s can make an availability\n", Decrypted_Username);
                                printf("The room message is %s\n", room_message);
                            }
                        } else {
                            printf("Received empty room code\n");
                        }
                        send(C_Socket, room_message, strlen(room_message), 0);
                        printf("The main server sent the authentication result to the client.\n");
                    }
                } else {
                    printf("Authentication failed\n");
                    
                }
            }
            close(C_Socket); // Close client socket in child process
            exit(0); // Exit child process
        } else { // Parent process
            close(C_Socket); // Close client socket in parent process
        }
    }
    close(S_Socket);
    return 0;
}