#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <poll.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <fcntl.h>
using namespace std;
typedef struct pollfd pollfd;

#define STDOUT 1
#define BUFFER_SIZE 1024
#define STDIN 0

string THE_MATCH_FACTS_ARE_AS_BELOW = "The match facts are as below : \n";
const char* ROOMS_LAUNCHED = "Rooms launched!\n";
const char* THE_MATCH_ENDED = "The match is ended\n";
const char* SERVER_LAUNCHED = "Server Launched!\n";
const char* NEW_CONNECTION = "New Connection!\n";
const char* WELCOME = "What is your name?\n";
const char* CHOOSE_A_ROOM = "Choose a room to play\n";
const char* THIS_ROOM_NOT_EXIST = "This room does not exist!!\n";
const char* THE_ROOM_IS_NOT_EMPTY = "This room is not empty, choose another one\n";
const char* SUCCESSFULLY_JOINED = "You successfully join the room\n";
const char* WAIT_FOR_OTHER_PLAYER = "Wait for the other player...\n";
const char* GAME_STARTED = "Game is now started\n";
const char* CHOOSE_AN_ACTION = "Choose an action between Rock(1), Paper(2), Scissors(3)\n";
const char* GAME_IS_EQUAL = "The game is equal\n";
const char* END_GAME = "end_game\n";

char buffer[BUFFER_SIZE];

struct USER{
    char name[BUFFER_SIZE];
    int fd;
    int is_introduced;
    int room_fd;
    int wins;
};

class ROOM
{
public:
    int fd;
    USER player1;
    USER player2;
    int num_of_players;
    struct sockaddr_in addr;
    int choice_player1;
    int choice_player2;
    int port;
    int is_match_equal;
    int winner;

    ROOM (int port_num, char* ipaddr, int opt){

        addr.sin_family = AF_INET;
        if(inet_pton(AF_INET, ipaddr, &(addr.sin_addr)) == -1)
            perror("FAILED: Input ipv4 address invalid");

        if((fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
            perror("FAILED: Socket was not created");

        if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
            perror("FAILED: Making socket reusable failed");

        if(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
            perror("FAILED: Making socket reusable port failed");

        memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
        
        addr.sin_port = port_num;

        if(bind(fd, (const struct sockaddr*)(&addr), sizeof(addr)) == -1)
            perror("FAILED: Bind unsuccessfull");

        if(listen(fd, 20) == -1)
            perror("FAILED: Listen unsuccessfull");

        port = port_num;

        choice_player1 = 0;
        choice_player2 = 0;
        num_of_players = 0;
    }

    void add_player(char* name){
        if(num_of_players == 0){
            strcpy(player1.name, name);
            player1.is_introduced = 1;
            player1.room_fd = fd;
        }
        else if(num_of_players == 1){
            strcpy(player2.name, name);
            player2.is_introduced = 1;
            player2.room_fd = fd;
        }
    }

    int accept_user_to_room(vector<USER> &users_in_rooms, vector<pollfd> &pfds){
        struct sockaddr_in new_addr;
        socklen_t new_size = sizeof(new_addr);
        int new_fd = accept(fd, (struct sockaddr*)(&new_addr), &new_size);
        if(num_of_players == 0){
            player1.fd = new_fd;
            send(player1.fd, WAIT_FOR_OTHER_PLAYER, strlen(WAIT_FOR_OTHER_PLAYER), 0);
        }
        else if(num_of_players == 1){
            player2.fd = new_fd;
            send_choosing_menu();
        }
        num_of_players ++;
        USER new_user;
        new_user.fd = new_fd;
        new_user.room_fd = fd;
        pfds.push_back(pollfd{new_fd, POLLIN, 0});
        users_in_rooms.push_back(new_user);
    }

    void send_choosing_menu(){
        send(player1.fd, GAME_STARTED, strlen(GAME_STARTED), 0);
        send(player2.fd, GAME_STARTED, strlen(GAME_STARTED), 0);
        send(player1.fd, CHOOSE_AN_ACTION, strlen(CHOOSE_AN_ACTION), 0);
        send(player2.fd, CHOOSE_AN_ACTION, strlen(CHOOSE_AN_ACTION), 0);
    }

    int store_players_choice(int fd, int broadcast_fd, sockaddr_in bc_addr){

        if(fd == player1.fd){
            choice_player1 = stoi(buffer);
        }
        else if(fd == player2.fd){
            choice_player2 = stoi(buffer);
        }
        if(choice_player1 && choice_player2){
            start_the_game(broadcast_fd, bc_addr);
            return 1;
        }
        return 0;
    }

    void choose_the_winner(){
        if(choice_player1 == -1 && choice_player2 == -1){
            winner = 0;
            is_match_equal = 1;
        }
        else if(choice_player1 == -1){
            winner = 2;
            is_match_equal = 0;
        }
        else if(choice_player2 == -1){
            winner = 1;
            is_match_equal = 0;
        }
        else if(choice_player1 == 1 && choice_player2 == 3) {
            winner = 1;
            is_match_equal = 0;
        }
        else if(choice_player2 == 1 && choice_player1 == 3){
            winner = 2;
            is_match_equal = 0;
        }
        else if(choice_player1 > choice_player2){
            winner = 1;
            is_match_equal = 0;
        }
        else if(choice_player1 < choice_player2){
            winner = 2;
            is_match_equal = 0;
        }
        else{
            winner = 0;
            is_match_equal = 1;
        }
    }

    void send_the_match_result(int broadcast_fd, sockaddr_in bc_addr){
        string result = "MATCH RESULT in ROOM NUMBER : " + to_string(fd) + "\n";
        if(is_match_equal){
            result += GAME_IS_EQUAL;
            // send(player1.fd, GAME_IS_EQUAL, strlen(GAME_IS_EQUAL), 0);
            // send(player2.fd, GAME_IS_EQUAL, strlen(GAME_IS_EQUAL), 0);
        }
        else if(winner == 1){
            result += "THE WINNER IS : " + (string)player1.name + "THE LOSER IS : " + player2.name;
            // send(player1.fd, result.c_str(), strlen(result.c_str()), 0);
            // send(player2.fd, result.c_str(), strlen(result.c_str()), 0);
        }
        else if(winner == 2){
            result += "THE WINNER IS : " + (string)player2.name + "THE LOSER IS : " + player1.name;
            // send(player1.fd, result.c_str(), strlen(result.c_str()), 0);
            // send(player2.fd, result.c_str(), strlen(result.c_str()), 0);
        }

        int a = sendto(broadcast_fd, result.c_str(), strlen(result.c_str()), 0, (const struct sockaddr *)&bc_addr, sizeof(bc_addr));
        // send(player1.fd, THE_MATCH_ENDED, strlen(THE_MATCH_ENDED), 0);
        // send(player2.fd, THE_MATCH_ENDED, strlen(THE_MATCH_ENDED), 0);

    }

    void reset_the_room(){
        choice_player1 = 0;
        choice_player2 = 0;
        num_of_players = 0;
    }

    void start_the_game(int broadcast_fd, sockaddr_in bc_addr){
        choose_the_winner();
        send_the_match_result(broadcast_fd, bc_addr);
        reset_the_room();

    }

    int is_empty(){
        if(num_of_players < 2)
            return 1;
        return 0;
    }

};

int find_user_index_by_fd(int fd,vector<USER> users){
    for(int i = 0; i < users.size(); i++){
        if(users[i].fd == fd){
            return i;
        } 
    }
    return -1;
}

void send_available_rooms(vector<ROOM> rooms, int user_fd){
    string result = "Choose a room to play\n";

    for(int i = 0; i < rooms.size(); i++){
        if(rooms[i].num_of_players == 1){
            result.append("room number " + to_string(i + 1) + " is available. players in room : 1\n");
        }
        if(rooms[i].num_of_players == 0){
            result.append("room number " + to_string(i + 1) + " is available. players in room : 0\n");
        }
    }
    const char* temp = result.c_str();
    send(user_fd, temp, strlen(temp), 0);
}

int is_in_rooms(int fd, int num_of_rooms)
{
    if(fd > 3 && fd <= 3 + num_of_rooms){
        return 1;
    }
    return 0;
}

void accept_user_to_server(vector<USER> &users, vector<pollfd> &pfds, int server_fd){
    struct sockaddr_in new_addr;
    socklen_t new_size = sizeof(new_addr);
    int new_fd = accept(server_fd, (struct sockaddr*)(&new_addr), &new_size);

    write(1, NEW_CONNECTION, strlen(NEW_CONNECTION));
    pfds.push_back(pollfd{new_fd, POLLIN, 0});

    USER new_user;
    new_user.fd = new_fd;
    new_user.is_introduced = 0;
    new_user.wins = 0;
    users.push_back(new_user);
    send(new_fd, WELCOME, strlen(WELCOME), 0);

}

int is_player_choice_available(int choice, vector<ROOM> rooms, int num_of_rooms, int fd){
    if(choice >= 0 && choice < num_of_rooms){
        if(rooms[choice].is_empty()){
            return 1;
        }
        else{
            send(fd, THE_ROOM_IS_NOT_EMPTY, strlen(THE_ROOM_IS_NOT_EMPTY), 0);
            send_available_rooms(rooms, fd);
            return 0;
        }
    }
    else{
        send(fd, THIS_ROOM_NOT_EXIST, strlen(THIS_ROOM_NOT_EXIST), 0);
        send_available_rooms(rooms, fd);
        return 0;
    }
}

void enter_player_to_game(USER &user, char* buffer, vector<ROOM> rooms){
    strcpy(user.name, buffer);
    user.is_introduced = 1;
    send_available_rooms(rooms, user.fd);
}

void update_users(int winner, vector<USER> &users, vector<ROOM> rooms, char player1_name[BUFFER_SIZE], char player2_name[BUFFER_SIZE]){
    for(int i = 0; i < users.size(); i++){
        if(strcmp(users[i].name, player1_name) == 0){
            if(winner == 1){
                users[i].wins ++;
            }
            send_available_rooms(rooms, users[i].fd);
        }
        else if(strcmp(users[i].name, player2_name) == 0){
            if(winner == 2){
                users[i].wins ++;
            }
            send_available_rooms(rooms, users[i].fd);
        }
    }
}

void connect_client_to_chosen_room(USER &user, ROOM &room){
    room.add_player(user.name);
    user.room_fd = room.fd;
    send(user.fd, SUCCESSFULLY_JOINED, strlen(SUCCESSFULLY_JOINED), 0);
    const char* room_port = to_string(room.port).c_str();
    send(user.fd, room_port, strlen(room_port), 0);
}

void print_result(vector<USER> users, int broadcast_fd, sockaddr_in bc_addr){
    string result = THE_MATCH_FACTS_ARE_AS_BELOW;
    for(int i = 0; i < users.size(); i++){
        result.append(to_string(users[i].wins) + " wins for : " + (string)users[i].name);
    }
    int a = sendto(broadcast_fd, result.c_str(), strlen(result.c_str()), 0, (const struct sockaddr *)&bc_addr, sizeof(bc_addr));
}

void creating_rooms(vector<pollfd> &pfds, vector<ROOM> &rooms, int num_of_rooms, char* ipaddr, int opt){
    int port = 2000;
    for(int i = 0; i < num_of_rooms; i++){
        ROOM temp(port, ipaddr, opt);
        rooms.push_back(temp);
        pfds.push_back(pollfd{temp.fd, POLLIN, 0});
        port++;
    }
    write(1, ROOMS_LAUNCHED, strlen(ROOMS_LAUNCHED));
}

int check_is_end_game(){
    memset(buffer, 0, BUFFER_SIZE);
    read(STDIN, buffer, BUFFER_SIZE);
    if(strcmp(buffer, END_GAME) == 0){
        return 1;
    }
    return 0;
}

int create_broadcast(vector<pollfd> &pfds, char* ipaddr, int opt, int broadcast, sockaddr_in &bc_addr){
    int broadcast_fd;
    if((broadcast_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        perror("FAILED: Socket was not created");

    if(setsockopt(broadcast_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1)
        perror("FAILED: Making socket reusable failed");

    if(setsockopt(broadcast_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
        perror("FAILED: Making socket reusable port failed");

    bc_addr.sin_family = AF_INET;
    bc_addr.sin_port = htons(8080);
    bc_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    if(bind(broadcast_fd, (const struct sockaddr*)(&bc_addr), sizeof(bc_addr)) == -1)
        perror("FAILED: Bind unsuccessfull");

    pfds.push_back(pollfd{broadcast_fd, POLLIN, 0});
    return broadcast_fd;

}

int main(int argc, char* argv[])
{

    if(argc != 4)
        perror("Invalid Arguments");

    int broadcast = 1;
    vector<USER> users;
    vector<USER> users_in_rooms;
    vector<ROOM> rooms;
    int num_of_rooms = stoi(argv[3]);
    char* ipaddr = argv[1];
    char player1_name[BUFFER_SIZE], player2_name[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    struct sockaddr_in bc_addr;
    int server_fd, opt = 1;
    std::vector<pollfd> pfds;
    pfds.push_back(pollfd{STDIN, POLLIN, 0});


    server_addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, ipaddr, &(server_addr.sin_addr)) == -1)
        perror("FAILED: Input ipv4 address invalid");

    if((server_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        perror("FAILED: Socket was not created");

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        perror("FAILED: Making socket reusable failed");

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
        perror("FAILED: Making socket reusable port failed");

    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));
    
    server_addr.sin_port = htons(strtol(argv[2], NULL, 10));

    if(bind(server_fd, (const struct sockaddr*)(&server_addr), sizeof(server_addr)) == -1)
        perror("FAILED: Bind unsuccessfull");

    if(listen(server_fd, 20) == -1)
        perror("FAILED: Listen unsuccessfull");

    write(1, SERVER_LAUNCHED, strlen(SERVER_LAUNCHED));
    pfds.push_back(pollfd{server_fd, POLLIN, 0});

    creating_rooms(pfds, rooms, num_of_rooms, ipaddr, opt);
    int broadcast_fd = create_broadcast(pfds, ipaddr, opt, broadcast, bc_addr);

    while(1)
    {
        if(poll(pfds.data(), (nfds_t)(pfds.size()), -1) == -1)
            perror("FAILED: Poll");
        
        for(size_t i = 0; i < pfds.size(); ++i)
        {
            if(pfds[i].revents & POLLIN)
            {
                if(pfds[i].fd == STDIN){
                    if(check_is_end_game()){
                        print_result(users, broadcast_fd, bc_addr); 
                        exit(0);
                    }  
                }
                else if(pfds[i].fd == broadcast_fd){
                    memset(buffer, 0, BUFFER_SIZE);
                    recvfrom(broadcast_fd, buffer, BUFFER_SIZE, 0, nullptr, nullptr);

                }
                else if(pfds[i].fd == server_fd) // new user
                {
                    accept_user_to_server(users, pfds, server_fd);
                }
                else if(is_in_rooms(pfds[i].fd, num_of_rooms)) //connecting client to room
                {
                    rooms[pfds[i].fd - 4].accept_user_to_room(users_in_rooms, pfds);
                }
                else // message from user
                {
                    memset(buffer, 0, BUFFER_SIZE);
                    recv(pfds[i].fd, buffer, BUFFER_SIZE, 0);
                    int index_user_room;
                    int index_user = find_user_index_by_fd(pfds[i].fd, users);
                    if((index_user_room = find_user_index_by_fd(pfds[i].fd, users_in_rooms)) != - 1){
                        if(rooms[users_in_rooms[index_user_room].room_fd - 4].store_players_choice(pfds[i].fd, broadcast_fd, bc_addr)){
                            strcpy(player1_name, rooms[users_in_rooms[index_user_room].room_fd - 4].player1.name);
                            strcpy(player2_name, rooms[users_in_rooms[index_user_room].room_fd - 4].player2.name);
                            int winner = rooms[users_in_rooms[index_user_room].room_fd - 4].winner;
                            update_users(winner, users, rooms, player1_name, player2_name);
                            // users_in_rooms.erase(users_in_rooms.begin() + find_user_index_by_fd(player1_fd, users_in_rooms));
                            // users_in_rooms.erase(users_in_rooms.begin() + find_user_index_by_fd(player1_fd, users_in_rooms));
                        }
                    }
                    else if(users[index_user].is_introduced == 0){
                        enter_player_to_game(users[index_user], buffer, rooms);
                    }
                    else if(is_player_choice_available(stoi(buffer) - 1, rooms, num_of_rooms, pfds[i].fd)){
                        connect_client_to_chosen_room(users[index_user], rooms[stoi(buffer) - 1]);
                    }
                }
            }
        }
    }
}
