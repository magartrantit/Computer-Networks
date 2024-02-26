#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

int main()
{

    //initializare client
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSocket == -1)
    {
        perror("Eroare la crearea socketului");
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr));

    //conectare la server
    if(connect(clientSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1)
    {
        perror("Eroare la conectarea cu serverul");
        return 1;
    }

    
    while(1)
    {
        //exemplu de primire a unui mesaj de la server
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        //mesaj de la server
        cout<<buffer<<endl;

        //exemplu de trimitere a unui mesaj catre server
        
        string mesaj;
        getline(cin, mesaj);
        
        if(mesaj == "exit")
            break;

        send(clientSocket, mesaj.c_str(), mesaj.length(), 0);

    }
    

    //inchiderea conexiunii cu serverul
    close(clientSocket);

    return 0;
}