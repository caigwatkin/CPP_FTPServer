//=======================================================================================================================
//ACTIVE FTP SERVER Start-up Code for Assignment 1 (WinSock 2)

//This code gives parts of the answers away.
//Firstly, you must change parts of this program to make it IPv6-compliant (replace all data structures that work only with IPv4).
//This step would require creating a makefile, as the IPv6-compliant functions require data structures that can be found only by linking with the appropritate library files. 
//The sample TCP server codes will help you accomplish this.

//OVERVIEW
//The connection is established by ignoring USER and PASS, but sending the appropriate 3 digit codes back
//only the active FTP mode connection is implemented (watch out for firewall issues - do not block your own FTP server!).

//The ftp LIST command is fully implemented, in a very naive way using redirection and a temporary file.
//The list may have duplications, extra lines etc, don't worry about these. You can fix it as an exercise, 
//but no need to do that for the assignment.
//In order to implement RETR you can use the LIST part as a startup.  RETR carries a filename, 
//so you need to replace the name when opening the file to send.

//STOR is also a few steps away, use your implementation of RETR and invert it to save the file on the server's dir
//=======================================================================================================================

#include "server.h"

// Arguments:
//      0:  Program name
//      1:  Port number
//      2:  Debug mode (true/false)
int main(int argc, char *argv[])
{
    bool debug = debugMode(argc, argv);                                             // Debug mode off by default.

    int error = startWinsock(debug);                                                // Start winsock.
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }
    
    struct addrinfo *result = NULL;                                                 // Structure for server's address information.

    error = getServerAddressInfo(result, argc, argv, debug);                        // Get server's address information.
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    SOCKET s = INVALID_SOCKET;                                                      // Socket for listening.

    error = allocateServerSocket(s, result, debug);                                 // Allocate server socket.
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    error = bindServerSocket(s, result, debug);                                     // Bind the server socket
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    freeaddrinfo(result);                                                           // Free memory.

    char serverHost[NI_MAXHOST];                                                    // The server's IP number.
    char serverService[NI_MAXSERV];                                                 // The server's port number.

    error = getServerNameInfo(s, serverHost, serverService, debug);                 // Get the server name information.
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    error = serverListen(s, debug);                                                 // Listen for client connections and deal with them accordingly.
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    return 0;                                                                       // Program completed without error.
}

// Returns true if user indicated that debug mode should be on.
bool debugMode(int argc, char * argv[])
{
    if (argc > 2)                                                                   // Check if there are more than 2 arguments.
    {
        if (strcmp(argv[2], "true") == 0)                                           // Check if argument 3 is debug command.
        {
            return true;                                                            // Set debug mode on.
        }
    }

    return false;                                                                   // Set debug mode off.
}

// Starts WSA.
int startWinsock(bool debug)
{
    WSADATA wsadata;
    int err = WSAStartup(WSVERS, &wsadata);                                         // Initialise use of winsock dll and save error code.

    if (err != 0)                                                                   // Check if there was an error starting winsock.
    {
        std::cout << "WSAStartup failed with error: " << err << std::endl;          // Tell the user that we could not fInd a usable WinsockDLL.
        WSACleanup();                                                               // Clean up winsock.

        return 1;                                                                   // Return error code.
    }
    else if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wVersion) != 2)        // Ensure the use of winsock 2.2 is supported.
    {
        std::cout << "Could not fInd a usable version of Winsock.dll" << std::endl;;
        WSACleanup();                                                               // Clean up winsock.

        return 2;                                                                   // Return error code.
    }
    else
    {
        std::cout << std::endl << "===============================" << std::endl;
        std::cout <<              "     159.334 FTP Server        ";
        std::cout << std::endl << "===============================" << std::endl;
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Winsock started." << std::endl;
    }

    return 0;                                                                       // Completed without error.
}

// Gets the servers address information based on arguments.
int getServerAddressInfo(struct addrinfo * &result, int argc, char *argv[], bool debug)
{
    struct addrinfo hints;                                                          // Create address info hint structure.
    memset(&hints, 0, sizeof(struct addrinfo));                                     // Clean up the structure.

    hints.ai_family = AF_INET;                                                      // Set address family as internet (IPv4).
    hints.ai_socktype = SOCK_STREAM;                                                // Set socket type as socket stream (for TCP).
    hints.ai_protocol = IPPROTO_TCP;                                                // Set protocol as TCP.
    hints.ai_flags = AI_PASSIVE;                                                    // Set flags as passive.

    int iResult;                                                                    // Create return value.
    if (argc > 1)
    {
        iResult = getaddrinfo(NULL, argv[1], &hints, &result);                      // Resolve local address and port to be used by the server.
    }
    else
    {
        iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);                 // Resolve local address and port to be used by the server.
    }
    
    if (iResult != 0)                                                               // Check for expected execution of getaddrinfo.
    {
        std::cout << "getaddrinfo() failed: " << iResult << std::endl;
        WSACleanup();                                                               // Clean up winsock.

        return 3;                                                                   // Return error code.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Server address information created." << std::endl;
    }

    return 0;                                                                       // Completed without error.
}

// Allocates the server's socket.
int allocateServerSocket(SOCKET &s, struct addrinfo *result, bool debug)
{
    s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);        // Allocate socket.

    if (s == INVALID_SOCKET)                                                        // Check for error in socket allocation.
    {
        std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);                                                       // Free memory.
        WSACleanup();                                                               // Clean up winsock.

        return 4;                                                                   // Return error code.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Server socket allocated." << std::endl;
    }

    return 0;                                                                       // Completed without error.
}

// Bind server socket to result address.
int bindServerSocket(SOCKET &s, struct addrinfo *result, bool debug)
{
    int iResult = bind(s, result->ai_addr, (int) result->ai_addrlen);               // Bind the listening socket to the server's IP address and port number.

    if (iResult == SOCKET_ERROR)                                                    // Check if error occurred in socket binding.
    {
        std::cout << "Bind failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);                                                       // Free memory.
        closesocket(s);                                                             // Close socket.
        WSACleanup();                                                               // Clean up winsock.

        return 5;                                                                   // Return error code.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Server socket bound." << std::endl;
    }

    return 0;                                                                       // Completed without error.
}

// Gets the host and service information for the server.
int getServerNameInfo(SOCKET &s, char serverHost[], char serverService[], bool debug)
{
    struct sockaddr_storage serverAddress;                                          // The server's address information.

    int addressLength = sizeof(serverAddress);                                      // Get the size of the client address sockaddr structure.

    DWORD returnValue = getsockname(s, (struct sockaddr *) &serverAddress, (socklen_t *) &addressLength);   // Get socket information.
    if(returnValue != 0)                                                            // Check that sock name obtained as expected.
    {
        std::cout << "getsockname() failed: " << returnValue;

        return 6;                                                                   // Return error code.
    }

    returnValue = getnameinfo((struct sockaddr *) &serverAddress, addressLength,    // Get name info for server.
                              serverHost, NI_MAXHOST,
                              serverService, NI_MAXSERV,
                              NI_NUMERICHOST);

    if(returnValue != 0)                                                            // Check that name info obtained as expected.
    {
        std::cout << "getnameinfo() failed: " << returnValue;

        return 7;                                                                   // Return error code.
    }
    else
    {
        std::cout << "Sever intialised at Port: " << serverService << std::endl;
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Server name information collected." << std::endl;
    }

    return 0;                                                                       // Completed without error.
}

// Listen for client communication and deal with it accordingly.
int serverListen(SOCKET &s, bool debug)
{
    int error = startListen(s, debug);                                              // Start server listening for incoming connections.
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    while (!error)                                                                  // Start main listening loop which continues infInitely while server runs.
    {
        error = acceptClients(s, debug);                                            // Accept clients and deal with commands.
    }

    closesocket(s);                                                                 // Close listening socket.
    std::cout << "SERVER SHUTTING DOWN..." << std::endl;

    return error;                                                                   // Return error code.
}

// Starts server listening for incoming connections.
int startListen(SOCKET &s, bool debug)
{
    if (listen(s, SOMAXCONN) == SOCKET_ERROR)                                       // Start listening to socket s and check if error.
    {
        std::cout << "Listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(s);                                                             // Close socket.
        WSACleanup();                                                               // Clean up winsock.

        return 8;                                                                   // Exit with error code.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Server started listening." << std::endl;
    }

    return 0;                                                                       // Completed without error.
}

// Accepts new clients and deals with commands.
int acceptClients(SOCKET &s, bool debug)
{
    struct sockaddr_storage clientAddress;                                          // The client's address information.
    char clientHost[NI_MAXHOST];                                                    // The client's IP number.
    char clientService[NI_MAXSERV];                                                 // The client's port number.

    memset(clientHost, 0, sizeof(clientHost));                                      // Ensure blank.
    memset(clientService, 0, sizeof(clientService));                                // Ensure blank.

    SOCKET ns = INVALID_SOCKET;                                                     // New socket for new client.

    int error = acceptNewClient(ns, s, clientAddress, clientHost, clientService, debug);    // Takes incoming connection and assigns new socket.
    if (error)                                                                      // Check if error occurred.
    {
        return error;                                                               // Exit with error code.
    }

    char sendBuffer[BUFFER_SIZE];                                                  // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                           // Ensure blank.

    sprintf(sendBuffer,"220 FTP Server ready.\r\n");                               // Add accept message for client to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                      // Send accept message to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if ((bytes == SOCKET_ERROR) || (bytes == 0))                                    // If send failed client has disconnected.
    {
        closeClientConnection(ns, debug);                                           // Close client connection.

        return 0;                                                                   // Connection ended, leave function.
    }

    bool success = true;                                                            // The status of the connection with the client.
    bool authroisedLogin = false;                                                   // Flag for if user logging in is authorised or not.

    SOCKET sDataActive = INVALID_SOCKET;                                            // Socket for data transfer.

    while (success)
    {
        success = communicateWithClient(ns, sDataActive, authroisedLogin, debug);   // Receive and handle messages from client.
    }

    closeClientConnection(ns, debug);                                               // Close client connection.

    return 0;                                                                       // Completed without error.
}

// Takes incoming connection and assigns new socket.
int acceptNewClient(SOCKET &ns, SOCKET &s, struct sockaddr_storage &clientAddress, char clientHost[], char clientService[], bool debug)
{
    int addressLength = sizeof(clientAddress);                                      // Get the size of the client address sockaddr structure.

    ns = accept(s, (struct sockaddr *) &clientAddress, (socklen_t *) &addressLength);   // Accept a new client on new socket.

    if (ns == INVALID_SOCKET)                                                       // Check if client not accepted as expected.
    {
        std::cout << "Accept failed: " << WSAGetLastError() << std::endl;
        closesocket(s);                                                             // Close socket.
        WSACleanup();                                                               // Clean up winsock.

        return 9;                                                                   // Exit with error code.
    }
    else
    {
        std::cout << std::endl << "A client has been accepted." << std::endl;

        DWORD returnValue = getnameinfo((struct sockaddr *)&clientAddress, addressLength,   // Get name info for client.
                                  clientHost, NI_MAXHOST,
                                  clientService, NI_MAXSERV,
                                  NI_NUMERICHOST);

        if (returnValue != 0)                                                       // Check that name info obtained as expected.
        {
            std::cout << "getnameinfo() failed: " << returnValue;

            return 10;                                                              // Exit with error code.
        }
        else
        {
            std::cout << "Connected to client with IP address: " << clientHost;
            std::cout << ", at Port: " << clientService << std::endl;
        }
    }

    return 0;                                                                       // Completed without error.
}

// Receive and handle messages from client, returns false if client ends connection.
bool communicateWithClient(SOCKET &ns, SOCKET &sDataActive, bool &authroisedLogin, bool debug)
{
    char receiveBuffer[BUFFER_SIZE];                                                // Set up receive buffer.
    char userName[80];                                                              // To store the client's username.
    char password[80];                                                              // To store the client's password.

    memset(&receiveBuffer, 0, BUFFER_SIZE);                                         // Ensure blank.
    memset(&userName, 0, 80);                                                       // Initialise as blank string.
    memset(&password, 0, 80);                                                       // Initialise as blank string.
    
    bool receiptSuccessful = true;                                                  // True if reply sent successfully.

    receiptSuccessful = receiveMessage(ns, receiveBuffer, debug);                   // Receives message and saves it in receive buffer.
    if (!receiptSuccessful)                                                         // Check if user ended connection.
    {
        return receiptSuccessful;                                                   // Return that user ended connection.
    }

    bool success = true;                                                            // True if reply sent successfully.

    if (strncmp(receiveBuffer, "USER", 4) == 0)                                     // Check if USER message received from client.
    {
        int i = 0;                                                                  // Count number of login attempts.

        do
        {
            success = commandUserName(ns, receiveBuffer, userName, authroisedLogin, debug); // Handle command.

            if (!success)
            {
                i++;                                                                // Increment number of login attempts.

                receiptSuccessful = receiveMessage(ns, receiveBuffer, debug);       // Receives message and saves it in receive buffer.
                if (!receiptSuccessful)                                             // Check if user ended connection.
                {
                    return receiptSuccessful;                                       // Return that user ended connection.
                }
            }
        } while (!success && i < 3);                                                // Allow 3 unsuccessful login attempts.
    }

    else if (strncmp(receiveBuffer, "PASS", 4) == 0)                                // Check if PASS message received from client.
    {
        success = commandPassword(ns, receiveBuffer, password, authroisedLogin, debug); // Handle command.
    }

    else if (strncmp(receiveBuffer, "SYST", 4) == 0)                                // Check if SYST message received from client.
    {
        success = commandSystemInformation(ns, debug);                              // Handle command.
    }

    else if (strncmp(receiveBuffer, "QUIT", 4) == 0)                                // Check if QUIT message received from client.
    {
        success = commandQuit(ns, debug);                                           // Handle command.
    }

    else if (strncmp(receiveBuffer, "PORT", 4) == 0)                                // Check if PORT message received from client.
    {
        success = commandDataPort(ns, sDataActive, receiveBuffer, debug);           // Handle command.
    }

    //technically, LIST is different than NLST,but we make them the same here
    else if ( (strncmp(receiveBuffer, "LIST", 4) == 0) || (strncmp(receiveBuffer, "NLST", 4) == 0))   // Check if LIST or NLST message received from client.
    {
        success = commandList(ns, sDataActive, debug);                              // Handle command.
    }

    else if ( (strncmp(receiveBuffer, "RETR", 4) == 0))                             // Check if RETR message received from client.
    {
        success = commandRetrieve(ns, sDataActive, receiveBuffer, debug);           // Handle command.
    }

    else if ( (strncmp(receiveBuffer, "STOR", 4) == 0))                             // Check if STOR message received from client.
    {
        success = commandStore(ns, sDataActive, receiveBuffer, debug);              // Handle command.
    }

    else                                                                            // Command not known.
    {
        success = commandUnknown(ns, debug);                                        // Tell client command not known.
    }

    return success;                                                                 // Return whether reply sent successfully.
}

// Receives message and saves it in receive buffer, returns false if connection ended.
bool receiveMessage(SOCKET &ns, char receiveBuffer[], bool debug)
{
    int i = 0;                                                                      // Index of character of the receive buffer to look at.
    int bytes = 0;                                                                  // Response of receive function.

    bool messageToRead = true;                                                      // If more message to read this is true.

    while (messageToRead)                                                           // Read each byte of message.
    {
        bytes = recv(ns, &receiveBuffer[i], 1, 0);                                  // Inspect receive buffer byte by byte.

        if ((bytes == SOCKET_ERROR) || (bytes == 0))                                // If nothing in receive buffer client has disconnected.
        {
            messageToRead = false;                                                  // No message, end read loop.
        }
        else if (receiveBuffer[i] == '\n')                                          // Message ends on line-feed.
        {
            receiveBuffer[i] = '\0';                                                // Cap string.
            messageToRead = false;                                                  // All of message read, end read loop.
        }
        else if (receiveBuffer[i] != '\r')                                          // Ignore carriage-return.
        {
            i++;                                                                    // Go to next 
        }
    }

    if ((bytes == SOCKET_ERROR) || (bytes == 0))                                    // Client disconnected.
    {
        return false;                                                               // No message, end client message loop.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<--- " << receiveBuffer << "\\r\\n" << std::endl;
    }

    return true;                                                                    // Client still connected.
}

// Client sent USER command, returns false if fails.
bool commandUserName(SOCKET &ns, char receiveBuffer[], char userName[], bool &authroisedLogin, bool debug)
{
    removeCommand(receiveBuffer, userName);                                         // Remove the command from the received command and save as user name.

    std::cout << "User: \"" << userName << "\" attempting to login." << std::endl;

    bool validUserName = isValidUserName(userName);                                 // Check if user name is valid.

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    if (validUserName)                                                              // Check validity.
    {
        std::cout << "User name valid. Password required." << std::endl;

        authroisedLogin = true;                                                     // Need authorised password to log in.

        sprintf(sendBuffer, "331 Authorised login requested, please specify the password.\r\n");    // Add message to send buffer.
    }
    else                                                                            // User name invalid
    {
        std::cout << "User name unauthorised. Public access only." << std::endl;

        authroisedLogin = false;                                                    // Don't need authorised password to log in.

        sprintf(sendBuffer, "331 Public login requested, please specify email as password.\r\n");               // Add message to send buffer.
    }

    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }
    
    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Return whether a valid user name was entered.
}

// Returns true if valid user name.
bool isValidUserName(const char userName[])
{
    return !strcmp(userName, "nhreyes");                                            // Only valid user name is nhreyes, for testing purposes.
}

// Client sent PASS command, returns false if fails.
bool commandPassword(SOCKET &ns, char receiveBuffer[], char password[], bool authroisedLogin, bool debug)
{
    removeCommand(receiveBuffer, password);                                         // Get the inputted password.

    bool validPassword = isValidPassword(password, authroisedLogin);                // Check if password is email address.

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    if (validPassword)                                                              // Check if valid email address.
    {
        std::cout << "Password valid. User logged in." << std::endl;

        sprintf(sendBuffer, "230 Login successful.\r\n");                           // Add message to send buffer.
    }
    else                                                                            // Invalid email address.
    {
        std::cout << "Password invalid. Login failed." << std::endl;

        sprintf(sendBuffer, "530 Login authentication failed.\r\n");                // Add message to send buffer.
    }

    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return validPassword;                                                           // Return true if connection should end (if login unsuccessful).
}

// Returns true if valid password.
bool isValidPassword(const char password[], bool authroisedLogin)
{
    if (authroisedLogin)                                                            // Check if user name is for authorised user.
    {
        return !strcmp(password, "334");                                            // Only valid password is 334 for user nhreyes, for testing purposes.
    }
    else
    {
        return isEmailAddress(password);                                            // Public users must have email address for password.
    }
}

// Client sent SYST command, returns false if fails.
bool commandSystemInformation(SOCKET &ns, bool debug)
{
    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    std::cout << "System information requested." << std::endl;

    sprintf(sendBuffer,"215 Windows Type: WIN64\r\n");                              // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Client sent QUIT command, returns false if fails.
bool commandQuit(SOCKET &ns, bool debug)
{
    std::cout << "Client has quit the session." << std::endl;

    return false;                                                                   // Return that connection ended.
}

// Client sent PORT command, returns false if fails.
bool commandDataPort(SOCKET &ns, SOCKET &sDataActive, char receiveBuffer[], bool debug)
{
    std::cout << "===================================================" << std::endl;
    std::cout << "\tActive FTP mode, the client is listening..." << std::endl;
    
    char ipBuffer[40];                                                              // Stores decimal representation of client IP.
    char portBuffer[6];                                                             // Stores decimal representation of client port.

    memset(&ipBuffer, 0, 40);                                                       // Ensure blank.
    memset(&portBuffer, 0, 40);                                                     // Ensure blank.

    bool success = getClientIPPort(ns, receiveBuffer, ipBuffer, portBuffer, debug); // Get the IP address and port of the client.
    if (!success)                                                                   // Did not succeed.
    {
        return sendArgumentSyntaxError(ns, debug);                                  // Send error to client.
    }

    struct addrinfo *result = NULL;                                                 // Structure for client's address information.

    success = getClientAddressInfoActive(ns, result, ipBuffer, portBuffer, debug);  // Get the address information for the connection.
    if (!success)                                                                   // Did not succeed.
    {
        freeaddrinfo(result);                                                       // Free memory.
        
        return sendFailedActiveConnection(ns, debug);                               // Send error to client.
    }

    success = allocateDataTransferSocket(sDataActive, result, debug);               // Allocate socket for data transfer.
    if (!success)
    {
        closesocket(sDataActive);                                                   // Close socket.
        freeaddrinfo(result);                                                       // Free memory.
        
        return sendFailedActiveConnection(ns, debug);                               // Send error to client.
    }

    success = connectDataTransferSocket(sDataActive, result, debug);                // Connect to data transfer socket.
    if (!success)
    {
        closesocket(sDataActive);                                                   // Close socket.
        freeaddrinfo(result);                                                       // Free memory.

        return sendFailedActiveConnection(ns, debug);                               // Send error to client.
    }

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer,"200 PORT Command successful.\r\n");                         // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "<<<DEBUG INFO>>>: Connected to client's data connection." << std::endl;
    }

    return success;                                                                 // Connection not ended, command handled.
}

// Gets the client's IP and port number for active connection.
bool getClientIPPort(SOCKET &ns, char receiveBuffer[], char ipBuffer[], char portBuffer[], bool debug)
{
    int activePort[2], activeIP[4];                                                 // Stores port and IP for client transfer.

    int scannedItems = sscanf(receiveBuffer, "PORT %d,%d,%d,%d,%d,%d",              // Get port and IP information from client message.
                              &activeIP[0], &activeIP[1], &activeIP[2], &activeIP[3],
                              &activePort[0], &activePort[1]);
    
    if (scannedItems < 6)                                                           // Check that syntax as expected.
    {
        char sendBuffer[BUFFER_SIZE];                                               // Set up send buffer.
        memset(&sendBuffer, 0, BUFFER_SIZE);                                        // Ensure blank.

        sprintf(sendBuffer,"501 Syntax error in arguments.\r\n");                   // Add message to send buffer.
        int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                    // Send reply to client.

        if (debug)                                                                  // Check if debug on.
        {
            std::cout << "---> " << sendBuffer;
        }

        if (bytes < 0)                                                              // Check if message sent.
        {
            return false;                                                           // Message not sent, return that connection ended.
        }

        return true;                                                                // Failed but don't end connection.
    }

    sprintf(ipBuffer, "%d.%d.%d.%d", activeIP[0], activeIP[1], activeIP[2], activeIP[3]); // Create decimal representation of IP (IPv4 format).
    
    std::cout << "\tClient's IP is " << ipBuffer << std::endl;                      // IPv4 format client address.

    int portDecimal = activePort[0];                                                // The port number as a decimal.
    portDecimal = portDecimal << 8;                                                 // First number is most significant 8 bits.
    portDecimal += activePort[1];                                                   // Second number is least significant 8 bits.

    sprintf(portBuffer, "%hu", portDecimal);                                        // Copy port decimal into port buffer.

    std::cout << "\tClient's Port is " << portBuffer << std::endl;
    printf("===================================================\n");

    return true;
}

// Sends the client a message to say data connection failed.
bool sendArgumentSyntaxError(SOCKET &ns, bool debug)
{
    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer,"501 Syntax error in arguments.\r\n");                       // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;
}

// Gets the servers address information based on arguments.
bool getClientAddressInfoActive(SOCKET &ns, struct addrinfo * &result, const char ipBuffer[], const char portBuffer[], bool debug)
{
    struct addrinfo hints;                                                          // Create address info hint structure.
    memset(&hints, 0, sizeof(struct addrinfo));                                     // Clean up the structure.

    hints.ai_family = AF_INET;                                                      // Set address family as internet (IPv4).
    hints.ai_socktype = SOCK_STREAM;                                                // Set socket type as socket stream (for TCP).
    hints.ai_protocol = 0;                                                          // Set protocol as TCP.

    int iResult;                                                                    // Create return value.
    iResult = getaddrinfo(ipBuffer, portBuffer, &hints, &result);                   // Resolve address information for client connection.
    
    if (iResult != 0)                                                               // Check for expected execution of getaddrinfo.
    {
        std::cout << "getaddrinfo() for client failed: " << iResult << std::endl;

        return false;                                                               // Failed.
    }
    
    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Client address information created." << std::endl;
    }

    return true;                                                                    // Completed without error.
}

// Allocates the socket for data transfer..
bool allocateDataTransferSocket(SOCKET &sDataActive, struct addrinfo *result, bool debug)
{
    sDataActive = socket(result->ai_family, result->ai_socktype, result->ai_protocol);  // Allocate socket.

    if (sDataActive == INVALID_SOCKET)                                              // Check for error in socket allocation.
    {
        std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;

        return false;                                                               // Failed, return false.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Data transfer socket allocated." << std::endl;
    }

    return true;                                                                    // Completed without error.
}

// Bind data transfer socket to result address.
bool connectDataTransferSocket(SOCKET &sDataActive, struct addrinfo *result, bool debug)
{
    int iResult = connect(sDataActive, result->ai_addr, (int) result->ai_addrlen);  // Connect the data transfer socket to the client's IP address and port number.

    if (iResult == SOCKET_ERROR)                                                    // Check if error occurred in socket connection.
    {
        std::cout << "Active connection failed with error: " << WSAGetLastError() << std::endl;
        closesocket(sDataActive);                                                   // Close socket.

        return false;                                                               // Failed, return false.
    }

    if (debug)                                                                      // Check if debug info should be displayed.
    {
        std::cout << "<<<DEBUG INFO>>>: Data transfer socket connected." << std::endl;
    }

    return true;                                                                    // Completed without error.
}

// Sends the client a message to say data connection failed.
bool sendFailedActiveConnection(SOCKET &ns, bool debug)
{
    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer,"425 Something is wrong, can't start active connection.\r\n");   // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;
}

// Client sent LIST command, returns false if fails.
bool commandList(SOCKET &ns, SOCKET &sDataActive, bool debug)
{
    int successLevel = sendFile(ns, sDataActive, "tmpDir.txt", debug);              // Create and send directory listing.
    if (successLevel != 1)                                                          // Check if direcoty listing sent correctly.
    {
        closesocket(sDataActive);                                                   // Close active connection.

        return successLevel;                                                        // Returns 0 if total failure, -1 if just incorrect syntax.
    }

    closesocket(sDataActive);                                                       // Close active connection.

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "226 File transfer complete.\r\n");                         // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send message to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Sends specified file to client.
int sendFile(SOCKET &ns, SOCKET &sDataActive, const char fileName[], bool debug)
{
    if (!strcmp(fileName, "tmpDir.txt"))                                            // Check if LIST call.
    {
        std::cout << "Client has requested the directory listing." << std::endl;

        system("dir > tmpDir.txt");                                                 // Save directory information in temp file.
    }
    else
    {
        std::cout << "Client has requested to retrieve the file: \"" << fileName << "\"." << std::endl;
    }

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    FILE *fIn = fopen(fileName, "r");                                               // Open file.
    if (fIn == NULL)                                                                // Check if valid file.
    {
        std::cout << "The file: \"" << fileName << "\" does not exist." << std::endl;

        sprintf(sendBuffer, "550 File name invalid.\r\n");                          // Add message to send buffer.
        int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                    // Send reply to client.

        if (debug)                                                                  // Check if debug on.
        {
            std::cout << "---> " << sendBuffer;
        }

        if (bytes < 0)                                                              // Check if message sent.
        {
            return 0;                                                               // Message not sent, return that connection ended.
        }

        return -1;                                                                  // Failure, but don't end connection.
    }
    else
    {
        sprintf(sendBuffer, "150 Opening ASCII mode data connection.\r\n");         // Add message to send buffer.
        int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                    // Send reply to client.

        if (debug)                                                                  // Check if debug on.
        {
            std::cout << "---> " << sendBuffer;
        }

        if (bytes < 0)                                                              // Check if message sent.
        {
            if (!strcmp(fileName, "tmpDir.txt"))                                    // Check if LIST call.
            {
                fclose(fIn);                                                        // Close the file.
                system("del tmpDir.txt");                                           // Save directory information in temp file.
            }

            return 0;                                                               // Message not sent, return that connection ended.
        }
    }

    char tempBuffer[80];                                                            // Temporary character buffer.
    memset(&tempBuffer, 0, 80);                                                     // Ensure blank.

    while (!feof(fIn))                                                              // Scan till end of file.
    {
        fgets(tempBuffer, 78, fIn);                                                 // Get data from file into temp buffer.

        char sendBuffer[BUFFER_SIZE];                                               // Set up send buffer.
        memset(&sendBuffer, 0, BUFFER_SIZE);                                        // Ensure blank.

        sprintf(sendBuffer, "%s", tempBuffer);                                      // Add data to send buffer.

        int bytes = send(sDataActive, sendBuffer, strlen(sendBuffer), 0);           // Send over active connection.

        if (bytes < 0)                                                              // Check if error in sending.
        {
            if (!strcmp(fileName, "tmpDir.txt"))                                    // Check if LIST call.
            {
                fclose(fIn);                                                        // Close the file.
                system("del tmpDir.txt");                                           // Save directory information in temp file.
            }

            return 0;                                                               // Return failure.
        }
    }

    fclose(fIn);                                                                    // Close the file.

    if (!strcmp(fileName, "tmpDir.txt"))                                            // Check if LIST call.
    {
        system("del tmpDir.txt");                                                   // Save directory information in temp file.
    }

    std::cout << "File sent successfully."<< std::endl;

    return 1;                                                                       // Return success.
}

// Client sent RETR command, returns flase if fails.
bool commandRetrieve(SOCKET &ns, SOCKET &sDataActive, char receiveBuffer[], bool debug)
{
    char fileName[80];                                                              // Stores the name of the file to transfer.
    memset(&fileName, 0, 80);                                                       // Ensure empty.

    removeCommand(receiveBuffer, fileName);                                         // Get file name from command.

    bool success = sendFile(ns, sDataActive, fileName, debug);                      // Create and send directory listing.
    if (!success)                                                                   // Check if direcoty listing sent correctly.
    {
        closesocket(sDataActive);                                                   // Close active connection.

        return success;
    }

    closesocket(sDataActive);                                                       // Close active connection.

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "226 File transfer complete.\r\n");                         // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send message to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Client sent STORE command, returns flase if fails.
bool commandStore(SOCKET &ns, SOCKET &sDataActive, char receiveBuffer[], bool debug)
{
    char fileName[80];                                                              // Stores the name of the file to transfer.
    memset(&fileName, 0, 80);                                                       // Ensure empty.

    removeCommand(receiveBuffer, fileName);                                         // Get file name from command.

    bool success = saveFile(ns, sDataActive, fileName, debug);                      // Save the file to drive.
    if (!success)                                                                   // Check if direcoty listing sent correctly.
    {
        closesocket(sDataActive);                                                   // Close active connection.

        return success;
    }

    closesocket(sDataActive);                                                       // Close active connection.

    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "226 File transfer complete.\r\n");                         // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send message to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Sends specified file to client.
bool saveFile(SOCKET &ns, SOCKET &sDataActive, const char fileName[], bool debug)
{
    std::cout << "Client has requested to store the file: \"" << fileName << "\"." << std::endl;

    char receiveBuffer[BUFFER_SIZE];                                                // Set up send buffer.
    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.

    memset(&receiveBuffer, 0, BUFFER_SIZE);                                         // Ensure blank.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "150 Opening ASCII mode data connection.\r\n");             // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    char tempBuffer[80];                                                            // Temporary character buffer.
    memset(&tempBuffer, 0, 80);                                                     // Ensure blank.

    std::ofstream fOut;                                                             // Output file stream.
    fOut.open(fileName);                                                            // Open file.

    bool moreFile = true;                                                           // Flag for more file to read.

    while (moreFile)                                                                // Scan till end of file.
    {
        moreFile = receiveFileContents(sDataActive, tempBuffer);                    // Receive file contents.

        fOut << tempBuffer;                                                         // Save buffer to file.
    }

    fOut.close();                                                                   // Close the file.

    std::cout << "File saved successfully."<< std::endl;

    return true;                                                                    // Return success.
}

// Receives message and saves it in receive buffer, returns false if connection ended.
bool receiveFileContents(SOCKET &sDataActive, char receiveBuffer[])
{
    int i = 0;                                                                      // Index of character of the receive buffer to look at.
    int bytes = 0;                                                                  // Response of receive function.

    bool fileToRead = true;                                                         // If more file to read this is true.

    while (fileToRead && i < 79)                                                    // Read each byte of file.
    {
        bytes = recv(sDataActive, &receiveBuffer[i], 1, 0);                         // Inspect receive buffer byte by byte.

        if ((bytes == SOCKET_ERROR) || (bytes == 0))                                // If nothing in receive buffer client has disconnected.
        {
            fileToRead = false;                                                     // No message, end read loop.
        }

        i++;
    }

    receiveBuffer[i] = '\0';                                                        // Cap string.

    if ((bytes == SOCKET_ERROR) || (bytes == 0))                                    // Client disconnected.
    {
        return false;                                                               // No message, end client message loop.
    }

    return true;                                                                    // Client still connected.
}

// Client sent OPTS command, returns false if fails.
bool commandUnknown(SOCKET &ns, bool debug)
{
    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer,"550 unrecognised command.\r\n");                            // Add message to send buffer.
    int bytes = send(ns, sendBuffer, strlen(sendBuffer), 0);                        // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    if (bytes < 0)                                                                  // Check if message sent.
    {
        return false;                                                               // Message not sent, return that connection ended.
    }

    return true;                                                                    // Connection not ended, command handled.
}

// Takes a string with a 4 letter command at beginning and saves an output string with this removed.
void removeCommand(const char inputString[], char outputString[])
{
    unsigned int i = 0;                                                             // Array index.
    unsigned int length = strlen(inputString);                                      // Length of string.

    for (; i + 5 < length; i++)                                                     // Scan over input string.
    {
        outputString[i] = inputString[i + 5];                                       // Copy character to output string.
    }

    outputString[i] = '\0';                                                         // Cap output string.
}

// Check is inputted string is valid email address (only requires an '@' before a '.').
bool isEmailAddress(const char address[])
{
    if (!isAlphabetical(address[0]))                                                // First character must be a-z or A-Z.
    {
        return false;                                                               // Invalid first character.
    }

    int atIndex = -1, dotIndex = -1;                                                // To store the index of @ and . characters.

    unsigned int length = strlen(address);                                          // The length of the address.

    for (unsigned int i = 1; i < length; i++)                                       // Loop over email address.
    {
        const char c = address[i];                                                  // Get the current character from the string.

        if (!isAlphabetical(c) && !isNumerical(c))                                  // Check if not alphanumeric.
        {
            if (c == '@')                                                           // See if character is @ symbol.
            {
                atIndex = i;                                                        // Save index of @ symbol.
            }
            else if (c == '.')                                                      // See if character is . symbol.
            {
                dotIndex = i;                                                       // Save index of . symbol.
            }
            else                                                                    // Invalid character.
            {
                return false;                                                       // Not valid email address.
            }
        }
    }

    return (atIndex != -1 && dotIndex != -1) && (atIndex < dotIndex);               // Return true if both symbols exist and are in correct order.
}

// Returns true if the character is alphabetical.
bool isAlphabetical(const char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// Returns true if the character is a number.
bool isNumerical(const char c)
{
    return (c >= '0' && c <= '9');
}

// Sends client the closing connection method and closes the socket.
void closeClientConnection(SOCKET &ns, bool debug)
{
    char sendBuffer[BUFFER_SIZE];                                                   // Set up send buffer.
    memset(&sendBuffer, 0, BUFFER_SIZE);                                            // Ensure blank.

    sprintf(sendBuffer, "221 FTP server closed the connection.\r\n");               // Create buffer to send to client.
    send(ns, sendBuffer, strlen(sendBuffer), 0);                                    // Send reply to client.

    if (debug)                                                                      // Check if debug on.
    {
        std::cout << "---> " << sendBuffer;
    }

    std::cout << "Disconnected from client." << std::endl;
    
    closesocket(ns);                                                                // Close socket.
}

