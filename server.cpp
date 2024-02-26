#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <mutex>



#define MAXPLAYERS 100
#define MAXINTREBARI 100
using namespace std;

static int playerCT = 0;
int questionCT = 0;
sqlite3 *db;
bool gameStarted = false;
int currentQuestion = 0;
std::mutex winnersMutex;
int maxScore = 0;
int winner=0;


struct Player
{
    int number;
    char name[256];
    bool isAdmin;
    bool islog;
    int answer;
    int punctaj;
};


//struct pentru intrebari
struct Question
{
    int id;
    char text[256];
    char raspuns1[256];
    char raspuns2[256];
    char raspuns3[256];
    int raspunsCorect;
};

Question intrebari[MAXINTREBARI];
Player players[MAXPLAYERS];

int CheckSQL(int err, const char *msg)
{
    if(err !=SQLITE_OK)
    {
        fprintf(stderr, "Error: %s (%d)\n", msg, err);
        exit(EXIT_FAILURE);
    }
    return err;
}

void InitDataBase()
{
    char *err;
    char *sql_intrebari = "CREATE TABLE IF NOT EXISTS intrebari (id INTEGER PRIMARY KEY, text TEXT NOT NULL, raspuns1 TEXT NOT NULL, raspuns2 TEXT NOT NULL, raspuns3 TEXT NOT NULL, raspuns_corect INTEGER NOT NULL);";

    CheckSQL(sqlite3_open("baza_date.db", &db), "Failed to open database");
    CheckSQL(sqlite3_exec(db, sql_intrebari, 0, 0, &err), "Failed to create table ");
}

int callBack(void *data, int argc, char **argv, char **col)
{
    struct Question *intrebari = (struct Question *)data;

    intrebari[questionCT].id = atoi(argv[0]);
    snprintf(intrebari[questionCT].text, sizeof(intrebari[questionCT].text), "%s", argv[1]);
    snprintf(intrebari[questionCT].raspuns1, sizeof(intrebari[questionCT].raspuns1), "%s", argv[2]);
    snprintf(intrebari[questionCT].raspuns2, sizeof(intrebari[questionCT].raspuns2), "%s", argv[3]);
    snprintf(intrebari[questionCT].raspuns3, sizeof(intrebari[questionCT].raspuns3), "%s", argv[4]);
    intrebari[questionCT].raspunsCorect = atoi(argv[5]);
    questionCT++;

    return 0;
}

void LoadIntrebari()
{
    char *err;
    char *selectIntrebari = "SELECT * FROM intrebari;";
    questionCT = 0;

    CheckSQL(sqlite3_exec(db, selectIntrebari, callBack, intrebari, &err), "Failed to load questions from database");
}





void SendQuestions(int timer, int clientSocket, int playerNumber)
{
    
    int playersFinished = 0;

    for(int i=0; i<questionCT; i++)
    {
        this_thread::sleep_for(chrono::seconds(5));

        string questionMessage = "Intrebarea " + to_string(i + 1) + "): " + intrebari[i].text + "\n1. " + intrebari[i].raspuns1 + "\n2. " + intrebari[i].raspuns2 + "\n3. " + intrebari[i].raspuns3 + "\nRaspunsul tau: ";

        send(clientSocket, questionMessage.c_str(), questionMessage.length(), 0);
        auto start = std::chrono::steady_clock::now();

        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        ssize_t bitiCititi = recv(clientSocket, buffer, sizeof(buffer)-1, 0);

        if(bitiCititi <= 0)
        {
            cout<<"Jucatorul cu numarul "<<players[playerNumber].number<<" s-a deconectat!";
            
            break;
        }

        int playerAnswer;
        if(sscanf(buffer, "%d", &playerAnswer)==1 && playerAnswer>=1 && playerAnswer<=3)
        {
            auto end = std::chrono::steady_clock::now();
            auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(end-start).count();

            if(elapsed_time <= 5)
            {
                if(playerAnswer==intrebari[i].raspunsCorect)
                {
                    send(clientSocket, "Raspuns corect! Ai castigat un punct!", strlen("Raspuns corect! Ai castigat un punct!"), 0);
                    players[playerNumber].punctaj+=1;
                    if(players[playerNumber].punctaj > maxScore)
                    {
                        winner=playerNumber;
                        maxScore=players[playerNumber].punctaj;
                    }
                        
                }
                else
                {
                    send(clientSocket, "Raspuns gresit!", strlen("Raspuns gresit!"), 0);
                }
            }
            else
            {
                send(clientSocket, "Ai raspuns prea tarziu!", strlen("Ai raspuns prea tarziu!"), 0);
            }
            
        }
        else
        {
            send(clientSocket, "Raspuns invalid! Alege un numar de la 1 la 3!", strlen("Raspuns invalid! Alege un numar de la 1 la 3!"), 0);
        }

        
        if(i==questionCT-1)
        {
            playersFinished++;
            if(playersFinished == playerCT-1)
            {
                std::lock_guard<std::mutex> lock(winnersMutex);
                string msg = "\nCastigatorul este jucatorul " + to_string(winner) + " cu " + to_string(maxScore) + " de puncte!";
                send(clientSocket, msg.c_str(), msg.length(), 0);
            }
        }
    }


}



//conexiunea cu clientul
void HandleClient(int clientSocket, sqlite3 *db)
{
    
    
    playerCT++;
    int currentPlayer = playerCT;
    cout<<"Jucatorul cu numarul "<<currentPlayer<<" s-a conectat."<<endl;
    players[currentPlayer].islog=false;

    if(currentPlayer==1)
        players[currentPlayer].isAdmin = true;
    
    else
        players[currentPlayer].isAdmin = false;

     

    send(clientSocket, "Bine ai venit! introdu 'login name' pentru a te loga!", strlen("Bine ai venit! introdu 'login name' pentru a te loga!"), 0);
    while(1)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        int activity = select(clientSocket + 1, &readfds, NULL, NULL, &timeout);

        if(activity==-1)
        {
            perror("Eroare la select");
            break;
        }
        if(activity==0)
            continue;

        if(FD_ISSET(clientSocket, &readfds))
        {
            
            
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            ssize_t bitiCititi = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            if(bitiCititi <= 0)
            {
                cout<<"Jucatorul cu numarul "<<currentPlayer<<" s-a deconectat"<<endl;
               
                break;
            }

            //procesare mesaj de la client
            cout<<"Comanda primita de la jucatorul "<<currentPlayer<<": "<<buffer<<"\n";

            if(!players[currentPlayer].islog && gameStarted==false)
            {
                if(strncmp(buffer, "login", 5)==0)
                {
                    char playerName[256];
                    sscanf(buffer, "login %s", playerName);
                    if(strlen(playerName) > 0)
                    {
                        bool nameUsed=false;
                        for (int i=1; i<=playerCT; i++)
                        {
                            if(i != currentPlayer && players[i].islog && strcmp(players[i].name, playerName)==0)
                            {
                                nameUsed = true;
                                break;
                            }
                        }

                        if(!nameUsed)
                        {
                            strncpy(players[currentPlayer].name, playerName, sizeof(players[currentPlayer].name));
                            players[currentPlayer].islog=true;
                            send(clientSocket, "Autentificare reusita!", strlen("Autentificare reusita!"), 0);
                            
                        }
                        else
                        {
                            send(clientSocket, "Numele e deja folosit!", strlen("Numele e deja folosit!"), 0);
                        }
                    }
                    else
                        send(clientSocket, "Numele nu poate fi gol!", strlen("Numele nu poate fi gol"), 0);
                
                }
                else
                {
                    send(clientSocket, "Comanda invalida! Trebuie sa te autentifici!", strlen("Comanda invalida! Trebuie sa te autentifici!"), 0);
                }
            }
            else if(players[currentPlayer].islog==true)
            {
                //alte optiuni pt jucatorii autentificati
                if(strncmp(buffer, "start", 5)==0 && players[currentPlayer].isAdmin)
                {
                    cout<<"Adminul a dat start la joc\n";
                    
                    gameStarted=true;                    
                }
                else if(strncmp(buffer, "start", 5)==0 && players[currentPlayer].isAdmin==false)
                {
                    send(clientSocket, "Nu esti admin!", strlen("Nu esti admin!"), 0);               
                }
                if(gameStarted==true)
                {                            
                    SendQuestions(5, clientSocket,currentPlayer);     
                     gameStarted=false;                    
                }             
                
            }
            else if(players[currentPlayer].islog==false && gameStarted==true)
            {
                send(clientSocket, "Un joc este deja in desfasurare! Trebuie sa astepti!", strlen("Un joc este deja in desfasurare! Trebuie sa astepti!"), 0);
                playerCT--;
            }
            
        }


    }
    
    --playerCT;
    

    //inchidem conexiunea cu clientul
    close(clientSocket);
}


int main()
{

    InitDataBase();
    LoadIntrebari();

    //initializam serverul
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1)
    {
        perror("Eroare la crearea socketului serverului");
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(12345);

    bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr));
    listen(serverSocket, 2);

    printf("Serverul asculta la portul 12345\n");
    fflush(stdout);


    while(1)
    {
        
            sockaddr_in clientAddr{};
            socklen_t clientAddrLen = sizeof(clientAddr);

            //acceptam conexiunea de la un client
            int clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen);
            if(clientSocket < 0)
            {
                perror("Eroare la acceptarea conexiunii cu clientul");
                continue;
            }

            //cream un fir de executie pentru a gestiona clientul
            thread(HandleClient, clientSocket, db).detach();
       
    }
    

    sqlite3_close(db);

    return 0;
}

