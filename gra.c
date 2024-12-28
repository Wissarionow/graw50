#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/un.h>

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define RESET "\x1b[0m"

#define MAX_IP_LENGTH 16  // Długość maksymalna adresu IP w postaci tekstowej

struct sockaddr_in other_addr;
socklen_t addr_len = sizeof(other_addr);

char* getMyIPAddress() {
    char hostName[256];
    if (gethostname(hostName, sizeof(hostName)) == -1) {
        perror("Błąd przy pobieraniu nazwy hosta");
        return NULL;
    }

    struct hostent *hostInfo = gethostbyname(hostName);
    if (hostInfo == NULL) {
        perror("Błąd przy pobieraniu informacji o hoście");
        return NULL;
    }

    struct in_addr **addrList = (struct in_addr **)hostInfo->h_addr_list;
    if (addrList[0] == NULL) {
        perror("Brak dostępnych adresów IP dla hosta");
        return NULL;
    }

    char *ipAddress = malloc(MAX_IP_LENGTH);
    if (ipAddress == NULL) {
        perror("Błąd przy alokacji pamięci");
        return NULL;
    }

    // Konwersja adresu IP do postaci tekstowej
    if (inet_ntop(AF_INET, addrList[0], ipAddress, MAX_IP_LENGTH) == NULL) {
        perror("Błąd przy konwersji adresu IP do postaci tekstowej");
        free(ipAddress);
        return NULL;
    }

    return ipAddress;
}


int compare_dates(struct tm date1, struct tm date2) {
    
    time_t time1 = mktime(&date1);
    time_t time2 = mktime(&date2);

   
    if (time1 < time2) {
        return -1; 
    } else return 1;
}

void error(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void handler(int signum){
    printf("\nKoniec gry\n");
    exit(0);
}

int randnum() {
    srand(time(NULL));
    
    int losowaLiczba = rand() % 10 + 1;

    return losowaLiczba;
}

struct game{
    int num;
    short won; //gra zostala wygrana przez nadawce     
    char player1Name[20];// 0-pierwszy gracz, 1-drugi gracz
    char player2Name[20];// 0-pierwszy gracz, 1-drugi gracz
    int scores[2];
    int time;
    int in_progress;  // 1-w trakcie, 0-zakonczona
};



struct game gameData;//global gamedata structure declaration
char ip[20];
char* PORT;
char nick[20];


void send_game_data(const char *ip, char* port) {
    int sockfd;
    struct addrinfo hints, hints2, *res, *servinfo;

    // Prepare the hints structure
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;  // Use my IP address


    int err = getaddrinfo(NULL, port, &hints, &servinfo);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }


    // Prepare the hints structure
    memset(&hints2, 0, sizeof(hints2));
    hints2.ai_family = AF_INET;
    hints2.ai_socktype = SOCK_DGRAM;


    // Use getaddrinfo() to get addresses
    err = getaddrinfo(ip, port, &hints2, &res);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }


    // Create UDP socket
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sockfd == -1) {
        perror("Error creating socket");
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the IP address and port
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("Error binding socket");
        close(sockfd);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    // Send gameData structure via UDP
    if (sendto(sockfd, &gameData, sizeof(struct game), 0, res->ai_addr, res->ai_addrlen) == -1) {
        perror("Error sending data");
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);
    close(sockfd);
}

void receive_game_data(const char *ip, char* port) {
    printf(BLUE"Oczekiwanie na drugiego gracza\n"RESET);
    int sockfd;
    
    struct addrinfo hints, hints2, *res, *servinfo;
    socklen_t addrLen;

    // Prepare the hints structure
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;  // Use my IP address

    // Use getaddrinfo() to get addresses
    int err = getaddrinfo(NULL, port, &hints, &servinfo);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }
    
    // Prepare the hints structure
    memset(&hints2, 0, sizeof(hints2));
    hints2.ai_family = AF_INET;
    hints2.ai_socktype = SOCK_DGRAM;


    // Use getaddrinfo() to get addresses
    err = getaddrinfo(ip, port, &hints2, &res);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }
    // Create UDP socket
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sockfd == -1) {
        perror("Error creating socket");
        freeaddrinfo(servinfo);
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the IP address and port
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("Error binding socket");
        close(sockfd);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    // Receive gameData structure via UDP
    addrLen = sizeof(other_addr);
    if (recvfrom(sockfd, &gameData, sizeof(gameData), 0, (struct sockaddr*) &other_addr, (socklen_t *)&addr_len) == -1) {
        perror("Error receiving data");
        close(sockfd);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);
    close(sockfd);
}

int i_am_player1=0;
void create_connection(char *ip, char* port){
    //struct game bufor;
    printf(BLUE"Rozpoczecie laczenia z %s\n"RESET, ip);
    send_game_data(ip, port);
   
    int teraz = gameData.time;
    receive_game_data(ip, port);
    srand(time(NULL));
    

    if(teraz<gameData.time){//tu jest problem
        //jestem graczem 1
        printf(YELLOW"Jestem graczem 1\n"RESET);
        strcpy(gameData.player1Name, nick);
        gameData.num = randnum();
        gameData.in_progress = 1;
        i_am_player1=1;
        send_game_data(ip, port);
        receive_game_data(ip, port);
    }else{
        //jestem graczem 
        printf(YELLOW"Jestem graczem 2\n"RESET);
        strcpy(gameData.player2Name, nick);
        //gameData.num = rand()%10+1;
        gameData.in_progress = 1;
        send_game_data(ip, port);
    }
    printf(BLUE"Łączenie zakończne skucesem\n"RESET);

}




int main(int argc,char *argv[]){

    if(argc < 3)
    {error("Za malo argumentow");}
   
    
    signal(SIGINT, handler);
    strcpy(ip , argv[1]);
    PORT = argv[2];
    
    if(argc == 4)
        strcpy(nick, argv[3]);
    else
        strcpy(nick, getMyIPAddress());

    //struct tm *local_time;
    time_t current_time;
   
    strcpy(gameData.player1Name, nick);
    strcpy(gameData.player2Name, "0");
    gameData.time =time(&current_time);
    printf("Gra w 50, wersja \n");
    //printf("twoj adres ip: %s\n", getMyIPAddress());
    printf("Utworzono o %s", ctime(&current_time));
    printf("Wyslano zaproszenie gry do gracza: %s\n", ip);
    create_connection(ip, PORT);
    gameData.num=randnum();

    if(i_am_player1!=1)
        printf("Oczekiwanie na ruch gracza %s\n", gameData.player1Name);
    
   
    fflush(stdout);

    char input[100];
    int liczba;

    if(i_am_player1==1){
        printf("Wylosowana liczba: %d\n", gameData.num);
        printf("Twoj ruch: ");
       
        while(1){
            scanf("%s", input);

            if(strcmp(input, "koniec")==0){
                gameData.in_progress = 0;
                send_game_data(ip, PORT);
                kill(getpid(), SIGINT);
            }
            if(strcmp(input, "wynik")==0){
                 printf("Wynik gry:%s %d do %d %s\n",gameData.player1Name, gameData.scores[0], gameData.scores[1],gameData.player2Name);
                continue;
            }
            liczba = atoi(input);
            if(liczba-gameData.num>10||liczba-gameData.num<0){//zla liczba
                printf("Za duza roznica\n");
                printf("Twoj ruch: ");
                continue;
            }else{//zwykla sytuacja
                sleep(1);
                gameData.num = liczba;
                send_game_data(ip, PORT);
                break;
            }
        }
    }

    
    
  
    while(1){
        receive_game_data(ip, PORT);
        while(1){//kolej gracza
            if(gameData.in_progress==0){
                printf("Drugi gracz oposcil gre\n");
                printf("Wynik gry:%s %d do %d %s\n",gameData.player1Name, gameData.scores[0], gameData.scores[1],gameData.player2Name);
                create_connection(ip, PORT);
            }


            if(gameData.won==1){
                printf("Przegrales\n");
                 printf("Wynik gry:%s %d do %d %s\n",gameData.player1Name, gameData.scores[0], gameData.scores[1],gameData.player2Name);
                gameData.won = 0;
                srand(time(NULL));
                gameData.num=randnum();
            }
            printf("Aktualna liczba: %d\n", gameData.num);
            printf("Podaj liczbe: \n>");
            scanf("%s", input);
            

            if(strcmp(input, "koniec")==0){
                gameData.in_progress = 0;
                send_game_data(ip, PORT);
                kill(getpid(), SIGINT);
            }

            if(strcmp(input, "wynik")==0){
                printf("Wynik gry:%s %d do %d %s\n",gameData.player1Name, gameData.scores[0], gameData.scores[1],gameData.player2Name);
                continue;
            }

            liczba = atoi(input);
            if(liczba-gameData.num>10||liczba-gameData.num<=0){//zla liczba
                printf("Za duza roznica\n");
                printf("Twoj ruch: ");
                continue;
            }else{
                if (liczba==50){//wyglrana lokalnie
                    printf("Wygrales\n");
                    gameData.won = 1;
                    //gameData.in_progress = 0;
                    gameData.num=50;
                    if(i_am_player1==1)
                        gameData.scores[0]++;
                    else 
                        gameData.scores[1]++;
                    send_game_data(ip, PORT);
                }else{//zwykly ruch
                gameData.num = liczba;
                send_game_data(ip, PORT);
                }
                break;
            }
        }

    } 

    return 0;

}
