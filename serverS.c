#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVERM_PORT 44435
#define SERVERM_IP "127.0.0.1"
#define SINGLE_PORT 41435
#define NUM_ROOMS 50
#define FILE_PATH "./single.txt"

struct RoomStatus {
    char status[20];
    int vacancy;
};

struct RoomStatus room_status[NUM_ROOMS];
int room_num = 0;

void load_room_status() {
    FILE *file = fopen(FILE_PATH, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }
    int number = 0;
    while (fscanf(file, " %19[^,], %d", room_status[number].status, &room_status[number].vacancy) == 2) {
        room_num++;
        number++;
        printf("The Server S has sent the room status to the main server.\n");

        if (number >= NUM_ROOMS) {
            fprintf(stderr, "Maximum number of rooms exceeded\n");
            exit(1);
        }
    }
    fclose(file);
}

void reserve_room(const char *roomcode, int SS_Socket, struct sockaddr_in server_M_adder, socklen_t server_M_adder_len) {
    char message[5]; // Define message variable
    int i;
    for (i = 0; i < room_num; i++) {
        if (strcmp(roomcode, room_status[i].status) == 0) {
            if (room_status[i].vacancy > 0) {
                room_status[i].vacancy--;
                sprintf(message,"True");
                printf("Successful reservation. The count of Room %s is now %d.\n",roomcode,room_status[i].vacancy);
                sendto(SS_Socket, message, strlen(message), 0, (struct sockaddr*)&server_M_adder, server_M_adder_len);
                printf("The Server S finished sending the response and the updated room status to the main server.\n");
            } else {
                sprintf(message,"Fail");
                printf("Cannot make a reservation. Room %s is not available.\n",roomcode);
                sendto(SS_Socket, message, strlen(message), 0, (struct sockaddr*)&server_M_adder, server_M_adder_len);
                printf("The Server S finished sending the response to the main server.\n");
            }
            return;
        }
    }
    sprintf(message,"Cannot make a reservation. Not able to find the room layout.");
    sendto(SS_Socket, message, strlen(message), 0, (struct sockaddr*)&server_M_adder, server_M_adder_len);
}

int main(void) {
    // UDP socket creation
    int SS_Socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (SS_Socket == -1) {
        perror("Error creating the socket");
        exit(1);
    }

    struct sockaddr_in server_D_adder;
    server_D_adder.sin_family = AF_INET;
    server_D_adder.sin_addr.s_addr = INADDR_ANY;
    server_D_adder.sin_port = htons(SINGLE_PORT);

    // Bind the socket to the specified address and port for ServerS
    if (bind(SS_Socket, (struct sockaddr *)&server_D_adder, sizeof(server_D_adder)) == -1) {
        perror("Error binding the socket with an address");
        close(SS_Socket);
        exit(1);
    }

    printf("The Server S is up and running using UDP on port: %d\n", SINGLE_PORT);

    char roomcode[5];
    char recv[13];
    struct sockaddr_in server_M_adder;
    socklen_t server_M_adder_len = sizeof(server_M_adder);


    load_room_status();

    while (1) {
        // Getting the data from the main server
        ssize_t user_req= recvfrom(SS_Socket, roomcode, sizeof(roomcode) - 1, 0,
                                    (struct sockaddr *)&server_M_adder, &server_M_adder_len);
        roomcode[user_req] = '\0';

        ssize_t req_recv = recvfrom(SS_Socket, recv, sizeof(recv) - 1, 0,
                                    (struct sockaddr *)&server_M_adder, &server_M_adder_len);
        recv[req_recv]='\0';
        // printf("%s",recv);

        if (req_recv == -1) {
            perror("Error receiving data from the main server");
        } else {
            recv[req_recv] = '\0';
            // printf("The Server S received a request from the main server\n" );

            if (strcmp(recv, "Availability") == 0) {
                
                printf("The Server S received an availability request from the main server\n" );
                

                int f = 0;
                char message[5];
                for (int i = 0; i < room_num; i++) {
                    if (strcmp(roomcode, room_status[i].status) == 0) {
                        f = 1;
                        if (room_status[i].vacancy > 0) {
                            printf("Room is available\n");
                            sprintf(message, "True");
                        } else {
                            printf("Room is not available\n");
                            sprintf(message, "Fail");
                        }
                        sendto(SS_Socket, message, strlen(message), 0, (struct sockaddr *)&server_M_adder, server_M_adder_len);
                        printf("ServerS finished sending the availability status of code %s to the main server using UDP on port %d\n", roomcode, SINGLE_PORT);
                        
                        
                    }
                }

                if (!f) {
                    printf("Not able to find the room layout.");
                    sprintf(message, "Fail");
                    sendto(SS_Socket, message, strlen(message), 0, (struct sockaddr *)&server_M_adder, server_M_adder_len);
                }
            } else if (strcmp(recv, "Reservation") == 0) {

                printf("The Server S received a reservation request from the main server\n" );
             
                reserve_room(roomcode, SS_Socket, server_M_adder, server_M_adder_len);
            } else{

                int f = 0;
                char message[5];
                for (int i = 0; i < room_num; i++) {
                    if (strcmp(roomcode, room_status[i].status) == 0) {
                        f = 1;
                        if (room_status[i].vacancy > 0) {
                            printf("The requested room is available\n");
                            sprintf(message, "True");
                        } else {
                            printf("The requested room is not available\n");
                            sprintf(message, "Fail");
                        }
                        sendto(SS_Socket, message, strlen(message), 0, (struct sockaddr *)&server_M_adder, server_M_adder_len);
                        // printf("%s",message);
                        printf("ServerS finished sending the availability status of code %s to the main server using UDP on port %d\n", roomcode, SINGLE_PORT);
                        
                    }
                }

                if (!f) {
                    printf("The requested room doesn't exist\n");
                    sprintf(message, "Fail");
                    sendto(SS_Socket, message, strlen(message), 0, (struct sockaddr *)&server_M_adder, server_M_adder_len);
                }

            }
        }
    }

    close(SS_Socket);
    return 0;
}
