#include <iostream> 
#include <string>
#include <fstream>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>


#define WSVERS MAKEWORD(2,2)
#define DEFAULT_PORT "21"
#define USE_IPV6 true
#define BUFFER_SIZE 200

bool debugMode(int argc, char * argv[]);                                            // Returns true if user indicated that debug mode should be on.
int startWinsock(bool debug);                                                       // Starts WSA.
int getServerAddressInfo(struct addrinfo * &result, int argc, char *argv[], bool debug);    // Gets the servers address information based on arguments.
int allocateServerSocket(SOCKET &s, struct addrinfo *result, bool debug);           // Allocates the server's socket.
int bindServerSocket(SOCKET &s, struct addrinfo *result, bool debug);               // Bind server socket to result address.
int getServerNameInfo(SOCKET &s, char serverHost[], char serverService[], bool debug);  // Gets the host and service information for the server.

int serverListen(SOCKET &s, bool debug);                                            // Listen for client communication and deal with it accordingly.
int startListen(SOCKET &s, bool debug);                                             // Starts server listening for incoming connections.
int acceptClients(SOCKET &s, bool debug);                                           // Accepts new clients and deals with commands.
int acceptNewClient(SOCKET &ns, SOCKET &s, struct sockaddr_storage &clientAddress, char clientHost[], char clientService[], bool debug);    // Takes incoming connection and assigns new socket.

bool communicateWithClient(SOCKET &ns, SOCKET &sDataActive, bool &authroisedLogin, bool debug); // Receive and handle messages from client, returns true if client ends connection.
bool receiveMessage(SOCKET &ns, char receiveBuffer[], bool debug);                  // Receives message and saves it in receive buffer, returns true if message was received.
void closeClientConnection(SOCKET &ns, bool debug);                                 // Sends client the closing connection method and closes the socket.
bool commandUserName(SOCKET &ns, char receiveBuffer[], char userName[], bool &authroisedLogin, bool debug); // Client sent USER command, returns flase if fails.
bool commandPassword(SOCKET &ns, char receiveBuffer[], char password[], bool authroisedLogin, bool debug);  // Client sent PASS command, returns flase if fails.
bool commandSystemInformation(SOCKET &ns, bool debug);                              // Client sent SYST command, returns flase if fails.
bool commandQuit(SOCKET &ns, bool debug);                                           // Client sent QUIT command, returns flase if fails.
bool commandDataPort(SOCKET &ns, SOCKET &sDataActive, char receiveBuffer[], bool debug);    // Client sent PORT command, returns flase if fails.
bool commandList(SOCKET &ns, SOCKET &sDataActive, bool debug);                      // Client sent LIST command, returns flase if fails.
bool commandRetrieve(SOCKET &ns, SOCKET &sDataActive, char receiveBuffer[], bool debug);    // Client sent RETR command, returns flase if fails.
bool commandStore(SOCKET &ns, SOCKET &sDataActive, char receiveBuffer[], bool debug);   // Client sent STORE command, returns flase if fails.
bool commandUnknown(SOCKET &ns, bool debug);                                        // Client sent unknown command, returns flase if send fails.

void removeCommand(const char inputString[], char outputString[]);                  // Takes a string with a 4 letter command at beggining and saves an output string with this removed.
bool isValidUserName(const char userName[]);                                        // Returns true if valid user name.
bool isValidPassword(const char userName[], bool authroisedLogin);                  // Returns true if valid password.
bool isEmailAddress(const char address[]);                                          // Check is inputted string is valid email address.
bool isAlphabetical(const char c);                                                  // Returns true if the character is alphabetical.
bool isNumerical(const char c);                                                     // Returns true if the character is a number.

bool getClientIPPort(SOCKET &ns, char receiveBuffer[], char ipDecimal[], char portBuffer[], bool debug);    // Gets the client's IP and port number for active connection.
bool getClientAddressInfoActive(SOCKET &ns, struct addrinfo * &result, const char ipBuffer[], const char portBuffer[], bool debug); // Gets the client's address information based on arguments.
bool allocateDataTransferSocket(SOCKET &sDataActive, struct addrinfo *result, bool debug);  // Allocates the socket for data transfer.
bool connectDataTransferSocket(SOCKET &sDataActive, struct addrinfo *result, bool debug);   // Bind data transfer socket to result address.
bool sendFailedActiveConnection(SOCKET &ns, bool debug);                            // Sends the client a message to say data connection failed.
bool sendArgumentSyntaxError(SOCKET &ns, bool debug);                               // Sends the client a message to say data connection failed.

int sendFile(SOCKET &ns, SOCKET &sDataActive, const char fileName[], bool debug);   // Sends specified file to client.
bool saveFile(SOCKET &ns, SOCKET &sDataActive, const char fileName[], bool debug);  // Sends specified file to client.
bool receiveFileContents(SOCKET &sDataActive, char receiveBuffer[]);                // Receives message and saves it in receive buffer, returns false if connection ended.

