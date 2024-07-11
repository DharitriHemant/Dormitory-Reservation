Name: Dharitri Hemant Toshikhane
Student ID: 35493814

->The submitted project identifies the problem of Dormatry Room Reservation System. A Socket programming has been programmed using the basic C language. The structure consists of three parts: a main server, a client and three backend servers. The main server is connected to client via TCP and to the backend servers via UDP. The details of these componenets are given below:

1) Main Server:
The main server acts as a relay between the client and the backend servers. It receieves the username and password from the clients and verifies them using the given input file. After verifying it sends an authentication back to the client. The main server also decrypts the given username and password.The main server receievis the room code from the client and by comparing the prefix of the code given by the user it sends the request and the username to the respective backend server. It can connect with multiple clients at the same time using the child sockets.

2) Client Server:
The client server is responsible for communicating with the user. The client asks for username and password from the users and encrypts it. These encrypted username and passwords are sent to the main server for Authentication. If both username and password are correct the user is considered as a Member and is allowed to either check the Availability of the required room or can Reserve it as well. If the username is mentioned and the password isnt, then the user is considered to be a Guest and is only allowd to check the availability of the room. The client at the end prints the status of the requested room.

3) Backend Server:
The backend server stores the data of respective rooms they are assigned with. There are three different backend servers handelling three different types of rooms namely: the sinle room, the double room and the suite. These are represnted with their prefixes S,D,and U. The Backend server is responsible for storing the statuses of these rooms and uodate them as they get reserved by users. 

-> There are two types of text files:

1) members.text:
This contains the encrypted username and passwords, used by the main server to authenticate. These usernames and passwords are separated by comma.

2) single.txt , double.txt  and suite.txt:
This contains the room codes and their availability, which are seperated by a 
comma.



-> Limitation: When entering the user details for Guest, in case of password '0' must be entered. When the main server is ran on the terminal it displays a warning, which doesnt affect the process flow of the project.

Note:
Reused socket creation, recv and send commands from Beej textbook














