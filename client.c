#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h> 

#define SERVER_PORT 41435
#define SERVER_IP "127.0.0.1"
#define MAXDATASIZE 100 
#define MAX_ACTION_LENGTH 20 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void encryptValue(char *value, char *encryptedValue) {
    while (*value != '\0') {
        if (isalpha(*value)) {
            if (isupper(*value))
                *encryptedValue = ((*value - 'A' + 3) % 26) + 'A';
            else
                *encryptedValue = ((*value - 'a' + 3) % 26) + 'a';
        } else if (isdigit(*value)) {
            *encryptedValue = ((*value - '0' + 3) % 10) + '0';
        } else {
            *encryptedValue = *value; // no encryption for special characters
        }
        value++;
        encryptedValue++;
    }
    *encryptedValue = '\0'; // Null-terminate the encrypted string
}

int main() {
    // Create Socket
    int C_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (C_Socket == -1) {
        perror("Error creating socket");
        exit(1);    
    }

    // Server Address setup
    struct sockaddr_in serverAddress;

    // Set the address family to AF_INET, indicating IPv4
    serverAddress.sin_family = AF_INET;
    // Converted to network byte order
    serverAddress.sin_port = htons(SERVER_PORT);
    // Convert the IP address from string representation to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serverAddress.sin_addr) <= 0) {
        // If conversion fails, print an error message and exit the program
        perror("Error converting IP address");
        exit(1);
    }

    // Connecting with the main server socket
    if (connect(C_Socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Error connecting to server");
        exit(1);
    }

    printf("Client is up and running\n");

    // Get username and password
    char username[50], password[50];
    char encryptUsername[50], encryptPassword[50];

    while (1) {
        printf("Please enter the username: ");
        scanf("%s", username);

        printf("Please enter the password (Enter 0 for Guest): ");
        scanf("%s", password);

        encryptValue(username, encryptUsername);
        encryptValue(password, encryptPassword);

        // printf("%s, %s\n", encryptUsername, encryptPassword);

        // To separate the username and password with (',')
        char message[1024];
        sprintf(message, "%s,%s", encryptUsername, encryptPassword);

        // Send message to server
        send(C_Socket, message, strlen(message), 0);
        printf("%s sent an authentication request to the main server\n", username);

        // send(C_Socket, userType, strlen(userType), 0);
        
        //Receiving the Authentication from the server
        char result[26];
        ssize_t authentication_result = recv(C_Socket, result, sizeof(result) - 1, 0); 
        if  (authentication_result == -1) {
            perror("Error receiving data from server\n");
            exit(1);
        } else if (authentication_result == 0) {
            printf("Connection closed by server\n");
            exit(1);
        }
        result[authentication_result] = '\0'; // Null-terminate the received data
        
        // printf("%s\n", result);

        // Proceed with room code
        if (strcmp(result, "Authentication successful")==0) {

            printf("Welcome member %s\n", username);
            
            // Member is Authenticated
            while (1) {
                
                //Getting the room code from user
                char Room_Code[4];
                printf("Please enter the room code: \n");
                scanf("%s", Room_Code);
                send(C_Socket, Room_Code, strlen(Room_Code), 0);
                // printf("%s\n", Room_Code);
                printf("%s sent an availability request to the main server.\n", username);

                // Prompt the user to choose the desired action
                char action[12];
                printf("Would you like to search for 'Availability' or make a 'Reservation'?: \n");
                scanf("%s", action);
                send(C_Socket,action, strlen(action),0);

                char response[5];
                ssize_t info_server = recv(C_Socket, response, sizeof(response), 0);
                response[info_server] = '\0';

                // For checking the Availability
                if (strcmp(action, "Availability") == 0) {

                    printf("The client received the response from the main server using TCP over port %d.\n\n", SERVER_PORT);
                    // printf("the response is: %s\n",response);
                    // printf(%strlen(response));

                    if (strcmp(response, "True") == 0) {
                        printf("The client received the response from the main server using TCP over port %d. The requested room is available\n", SERVER_PORT);
                        // printf("%s\n", response);
                    } else if (strcmp(response, "Fail") ==0) { 
                        printf("The client received the response from the main server using TCP over port %d. The requested room is not available.\n", SERVER_PORT);      
                    } else {
                        printf("The client received the response from the main server using TCP over port %d. The requested room layout not found.\n", SERVER_PORT);
                    }
                } 

                // For making reservations
                else if (strcmp(action, "Reservation") == 0) {

                    // printf("%s\n", response);
                    

                    if (strcmp(response, "True") == 0) {
                        printf("The client received the response from the main server using TCP over port %d. The requested room is Reserved\n", SERVER_PORT);
                        
                    } else if (strcmp(response, "Fail") == 0) { 
                        printf("The client received the response from the main server using TCP over port %d. The requested room is not Reserved.\n", SERVER_PORT);      
                    } else {
                        printf("The client received the response from the main server using TCP over port %d.The requested room is not Reserved.\n", SERVER_PORT);
                    }
                } 
                else {
                    printf("Invalid action\n");
                }
            }
        } //guest authenticatio
        else if (strcmp(result, "Guest login") == 0) {
            printf("Welcome guest %s\n", username);

            while(1){
  //Getting the room code from user
                char Room_Code[4];
                printf("Please enter the room code: \n");
                scanf("%s", Room_Code);
                send(C_Socket, Room_Code, strlen(Room_Code), 0);
                // printf("%s\n", Room_Code);
                printf("%s sent an availability request to the main server.\n", username);

                // Prompt the user to choose the desired action
                char action[12];
                sprintf(action,"Availability");
                send(C_Socket,action, strlen(action),0);

                char response[5];
                ssize_t info_server = recv(C_Socket, response, sizeof(response), 0);
                response[info_server] = '\0';

                // For checking the Availability
                if (strcmp(action, "Availability") == 0) {

                    printf("The client received the response from the main server using TCP over port %d.\n\n", SERVER_PORT);
                    // printf("the response is: %s\n",response);
                    // printf(%strlen(response));

                    if (strcmp(response, "True") == 0) {
                        printf("The client received the response from the main server using TCP over port %d. The requested room is available\n", SERVER_PORT);
                        // printf("%s\n", response);
                    } else if (strcmp(response, "Fail") ==0) { 
                        printf("The client received the response from the main server using TCP over port %d. The requested room is not available.\n", SERVER_PORT);      
                    } else {
                        printf("The client received the response from the main server using TCP over port %d. The requested room layout not found.\n", SERVER_PORT);
                    }
                } 

                // For making reservations
                else if (strcmp(action, "Reservation") == 0) {

                    printf("Sorry Guests can not make Reservations\n");
                
                } 
                else {
                    printf("Invalid action\n");
                }
        } 

        }else {
            printf("Authentication failed\n");
            break;
        }
    }

    close(C_Socket);
    return 0;
}
