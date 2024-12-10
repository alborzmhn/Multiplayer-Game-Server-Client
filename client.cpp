#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <poll.h>
#include <vector>
#include <signal.h>

typedef struct pollfd pollfd;
using namespace std;

const char* THE_MATCH_FACTS_ARE_AS_BELOW = "The match facts are as below : \n";
const char* SUCCESSFULLY_JOINED = "You successfully join the room\n";
const char* THE_MATCH_ENDED = "The match is ended\n";
const char* YOUR_TIME_IS_ENDED = "Your time has been ended\n";
const char* CHOOSE_AN_ACTION = "Choose an action between Rock(1), Paper(2), Scissors(3)\n";
const char* GAME_STARTED = "Game is now started\n";

typedef struct pollfd pollfd;

#define STDIN 0
#define STDOUT 1
#define BUFFER_SIZE 1024

char buffer[BUFFER_SIZE];

void recv_msg_from_server(int server_fd){
    memset(buffer, 0, BUFFER_SIZE);
    recv(server_fd, buffer, BUFFER_SIZE, 0);
    write(1, buffer, strlen(buffer));
}

void recv_msg_from_broadcast(int broadcast_fd){
    memset(buffer, 0, BUFFER_SIZE);
    recvfrom(broadcast_fd, buffer, BUFFER_SIZE, 0, nullptr, nullptr);
    write(1, buffer, strlen(buffer));
}

void send_msg_to_server(int server_fd){
    memset(buffer, 0, BUFFER_SIZE);
    read(STDIN, buffer, BUFFER_SIZE);
    send(server_fd, buffer, strlen(buffer), 0);
}

int connect_user_to_server(int port, char* ipaddr, int opt){
    int room_fd;
    struct sockaddr_in room_addr;
    room_addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, ipaddr, &(room_addr.sin_addr)) == -1)
        perror("FAILED: Input ipv4 address invalid");

    if((room_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        perror("FAILED: Socket was not created");

    if(setsockopt(room_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        perror("FAILED: Making socket reusable failed");

    if(setsockopt(room_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
        perror("FAILED: Making socket reusable port failed");

    memset(room_addr.sin_zero, 0, sizeof(room_addr.sin_zero));
    
    room_addr.sin_port = port;

    if(connect(room_fd, (sockaddr*)(&room_addr), sizeof(room_addr)))
        perror("FAILED: Connect");

    return room_fd;
}

int connect_user_to_broadcast(char* ipaddr, int opt, int broadcast){
    int broadcast_fd;
    struct sockaddr_in bc_addr;

    if((broadcast_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        perror("FAILED: Socket was not created");

    if(setsockopt(broadcast_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1)
        perror("FAILED: Making socket reusable failed");

    if(setsockopt(broadcast_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
        perror("FAILED: Making socket reusable port failed");

    bc_addr.sin_family = AF_INET;
    bc_addr.sin_port = htons(8080);
    bc_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    if(bind(broadcast_fd, (struct sockaddr*)(&bc_addr), sizeof(bc_addr)) == -1)
        perror("FAILED: Bind unsuccessfull");

    return broadcast_fd;

}

void alarm_handler(int sig){
    write(1, YOUR_TIME_IS_ENDED, strlen(YOUR_TIME_IS_ENDED));
}

void set_alarm_for_choosing(int room_fd){
    memset(buffer, 0, BUFFER_SIZE);
    signal(SIGALRM, alarm_handler);
    siginterrupt(SIGALRM, 1);
    alarm(10);
    int read_ret = read(STDIN, buffer, BUFFER_SIZE);
    alarm(0);
    if(read_ret == -1){
        send(room_fd, "-1\n", 2, 0);  
    }                     
    else{
        send(room_fd, buffer, strlen(buffer), 0);
    }
}

int main(int argc, char* argv[])
{
    if(argc != 3)
        perror("Invalid Arguments");

    char* ipaddr = argv[1];
    int server_fd, opt = 1;
    vector<pollfd> pfds;
    server_fd = connect_user_to_server(htons(strtol(argv[2],NULL,10)), ipaddr, opt);
    pfds.push_back(pollfd{STDIN, POLLIN, 0}); 
    pfds.push_back(pollfd{server_fd, POLLIN, 0}); 
    int room_fd = -1;
    int room_port;
    int broadcast = 1;
    int broadcast_fd = connect_user_to_broadcast(ipaddr, opt, broadcast);
    pfds.push_back(pollfd{broadcast_fd, POLLIN, 0}); 

    while(1)
    {
        if(poll(pfds.data(), (nfds_t)(pfds.size()), -1) == -1)
            perror("FAILED: Poll");
        
        for(size_t i = 0; i < pfds.size(); ++i)
        {
            if(pfds[i].revents & POLLIN)
            {
                if(pfds[i].fd == broadcast_fd){
                    recv_msg_from_broadcast(broadcast_fd);
                    if(((string)buffer).substr(0, strlen(THE_MATCH_FACTS_ARE_AS_BELOW)) == THE_MATCH_FACTS_ARE_AS_BELOW){
                        exit(0);
                    }
                }
                else if(pfds[i].fd == room_fd){
                    recv_msg_from_server(room_fd);
                    // if(((string)buffer).substr(strlen(buffer) - strlen(THE_MATCH_ENDED)) == THE_MATCH_ENDED){
                    //     //is_in_room = 0;
                    //     //close(room_fd);
                    //     //room_fd = -1;
                    // }
                    if(((string)buffer).substr(0, strlen(GAME_STARTED)) == GAME_STARTED){
                        set_alarm_for_choosing(room_fd);
                    }
                }
                else if(pfds[i].fd == server_fd){
                    recv_msg_from_server(server_fd);
                    if(((string)buffer).substr(0, strlen(SUCCESSFULLY_JOINED)) == SUCCESSFULLY_JOINED){
                        //is_in_room = 1;
                        room_port = stoi(((string)buffer).substr(strlen(SUCCESSFULLY_JOINED)).c_str());
                        room_fd = connect_user_to_server(room_port, ipaddr, opt);
                        pfds.push_back(pollfd{room_fd, POLLIN, 0}); 
                    }
                }
                else{
                    send_msg_to_server(server_fd);
                }
            }
        }
    }
}

