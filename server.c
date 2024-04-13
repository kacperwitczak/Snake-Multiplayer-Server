#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define MAX_CLIENTS 10

#define BUFFER_SIZE 1024

#define CLIENT_BUFFER_SIZE 1

#define SNAKE_SIZE 10

#define BOARD_SIZE 50

#define DELAY 100000

#define FOOD_SIZE 13

#define COLORS_SIZE 3

char colors[COLORS_SIZE] = { 'G', 'Y', 'P' };





// Struktura reprezentująca klienta
typedef struct {
    int socket_fd;
    struct sockaddr_in address;
    short is_used; // czy klient jest aktualnie używany
} Client;


// Lista klientów
Client clients[MAX_CLIENTS];
int num_clients = 0;

typedef struct {
    int x;
    int y;
} Point;

Point random_point(int board_size) {
    Point p;
    p.x = rand() % board_size;
    p.y = rand() % board_size;
    return p;
}

typedef struct {
    //head: snake[0]
    Point snake[SNAKE_SIZE];
    int size;
    char direction;
    short is_alive;
    int size_to_add;
    char color;
} Snake;

typedef struct {
    Point food;
    short is_eaten;
} Food;

typedef struct {
    pthread_mutex_t snake_mutex;
    Snake snakes[MAX_CLIENTS];
    Food foods[FOOD_SIZE];
    char board[BOARD_SIZE][BOARD_SIZE];
    int board_size;
    int snake_size;
    int food_size;
} Game;

void draw_board(Game *g) {
    for (int i = 0; i < g->board_size; i++) {
        for (int j = 0; j < g->board_size; j++) {
            g->board[i][j] = ' ';
        }
    }
    for (int i = 0; i < g->snake_size; i++) {
        if (g->snakes[i].is_alive) {
            g->board[g->snakes[i].snake[0].y][g->snakes[i].snake[0].x] = g->snakes[i].color;
            for (int j = 1; j < g->snakes[i].size; j++) {
                g->board[g->snakes[i].snake[j].y][g->snakes[i].snake[j].x] = 'B';
            }
        }
    }
    for (int i = 0; i < g->food_size; i++) {
        if (!g->foods[i].is_eaten) {
            g->board[g->foods[i].food.y][g->foods[i].food.x] = 'F';
        }
    }
}

void init_game(Game *g, int board_size, int snake_size, int food_size) {
    g->board_size = board_size;
    g->snake_size = snake_size;
    g->food_size = food_size;
    for (int i = 0; i < snake_size; i++) {
        g->snakes[i].is_alive = 0;
    }
    for (int i = 0; i < food_size; i++) {
        g->foods[i].food = random_point(board_size);
        g->foods[i].is_eaten = 0;
    }

    draw_board(g);
}

char random_direction() {
    char directions[4] = {'U', 'D', 'L', 'R'};
    return directions[rand() % 4];
}


void init_snake(Snake *s, int board_size, char col) {
    s->size = 1;
    s->snake[0] = random_point(board_size);
    s->direction = random_direction();
    s->is_alive = 1;
    s->color = col;
}

void move_snake(Snake *s) {
    if (s->size_to_add > 0 && s->size < SNAKE_SIZE-1) {
        s->size+=1;
        s->size_to_add = 0;
    }


    Point new_head = s->snake[0];
    switch (s->direction) {
        case 'U':
            new_head.y = (new_head.y - 1);
            break;
        case 'D':
            new_head.y = (new_head.y + 1);
            break;
        case 'L':
            new_head.x = (new_head.x - 1);
            break;
        case 'R':
            new_head.x = (new_head.x + 1);
            break;
    }
    for (int i = s->size - 1; i > 0; i--) {
        s->snake[i] = s->snake[i - 1];
    }
    s->snake[0] = new_head;
}

void change_direction(Snake *s, char new_direction) {
    //when snake goes up it can't go down and vice versa
    if (new_direction == 'U' && s->direction != 'D') {
        s->direction = new_direction;
    } else if (new_direction == 'D' && s->direction != 'U') {
        s->direction = new_direction;
    } else if (new_direction == 'L' && s->direction != 'R') {
        s->direction = new_direction;
    } else if (new_direction == 'R' && s->direction != 'L') {
        s->direction = new_direction;
    }
}

void check_collision(Game *g) {
    for (int i = 0; i < g->snake_size; i++) {
        if (g->snakes[i].is_alive) {
            /*
            for (int j = 1; j < g->snake[i].size; j++) {
                if (g->snake[i].snake[0].x == g->snake[i].snake[j].x && g->snake[i].snake[0].y == g->snake[i].snake[j].y) {
                    g->snake[i].is_alive = 0;
                }
            }*/
            if (g->snakes[i].snake[0].x < 0 || g->snakes[i].snake[0].x >= g->board_size || g->snakes[i].snake[0].y < 0 || g->snakes[i].snake[0].y >= g->board_size) {
                g->snakes[i].is_alive = 0;
            }

            for (int j = 0; j < g->snake_size; j++) {
                if (i != j && g->snakes[j].is_alive) {
                    for (int k = 0; k < g->snakes[j].size; k++) {
                        if (g->snakes[i].snake[0].x == g->snakes[j].snake[k].x && g->snakes[i].snake[0].y == g->snakes[j].snake[k].y) {
                            g->snakes[i].is_alive = 0;
                        }
                    }
                }
            }

            for (int j = 0; j < g->food_size; j++) {
                if (g->snakes[i].snake[0].x == g->foods[j].food.x && g->snakes[i].snake[0].y == g->foods[j].food.y) {
                    g->foods[j].is_eaten = 1;
                    g->snakes[i].size_to_add = 1;
                }
            }

            //snake died mid his move
            if (g->snakes[i].is_alive == 0) {
                init_snake(&g->snakes[i], g->board_size, g->snakes[i].color); //rebirth of snake
            }
        }
    }
}

void update_food(Game *g) {
    for (int i = 0; i < g->food_size; i++) {
        if (g->foods[i].is_eaten) {
            g->foods[i].food = random_point(g->board_size);
            g->foods[i].is_eaten = 0;
        }
    }
}

void register_new_player(int client_id, Game *g) {
    pthread_mutex_lock(&g->snake_mutex);
    char color = colors[client_id % COLORS_SIZE];
    init_snake(&g->snakes[client_id], g->board_size, color);
    pthread_mutex_unlock(&g->snake_mutex);
}

void unregister_player(int client_id, Game *g) {
    pthread_mutex_lock(&g->snake_mutex);
    g->snakes[client_id].is_alive = 0;
    pthread_mutex_unlock(&g->snake_mutex);
}

void set_new_move(int client_id, char move, Game *g) {
    pthread_mutex_lock(&g->snake_mutex);
    change_direction(&g->snakes[client_id], move);
    pthread_mutex_unlock(&g->snake_mutex);
}

// Mutex do synchronizacji dostępu do współdzielonych danych
pthread_mutex_t mutex_clients = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int client_id;
    Game *g;
}  Client_Register;

// Wątek obsługujący komunikację z klientem
void *client_handler(void *arg) {
    Client_Register client_reg = *((Client_Register *)arg);
    
    int client_id = client_reg.client_id;
    Game *client_game_handle = client_reg.g;
    Client c;
    char buffer[CLIENT_BUFFER_SIZE];

    pthread_mutex_lock(&mutex_clients);
    c = clients[client_id];
    pthread_mutex_unlock(&mutex_clients);

    //zarejestrowanie nowego gracza
    register_new_player(client_id, client_game_handle);

    int size = BOARD_SIZE;
    send(c.socket_fd, &size, sizeof(int), 0);

    while (1) {
        // Odbierz wiadomość od klienta
        int bytes_received = recv(c.socket_fd, buffer, CLIENT_BUFFER_SIZE, 0); //blocking call
        if (bytes_received <= 0) {
            // Jeśli błąd lub klient zamknął połączenie, usuń klienta z listy
            //unregister player
            unregister_player(client_id, client_game_handle);

            pthread_mutex_lock(&mutex_clients);
            close(c.socket_fd);
            printf("Klient o adresie %s i id %d odlaczony\n", inet_ntoa(c.address.sin_addr), client_id);

            // Usuń klienta z listy
            clients[client_id].is_used = 0;
            num_clients--;
            pthread_mutex_unlock(&mutex_clients);


            pthread_exit(NULL);
        }
        // Przetwórz wiadomość od klienta (możesz dodać własną logikę)
        printf("Wiadomosc od klienta: %c\n", buffer[0]);
        set_new_move(client_id, buffer[0], client_game_handle);
        memset(buffer, 0, CLIENT_BUFFER_SIZE);
    }
}

// Wątek obsługujący logikę gry
void *game_logic(void *arg) {
    Game *g = (Game *)arg;
    while (1) {
        // Wyslij plansze do wszystkich klientow
        pthread_mutex_lock(&mutex_clients);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].is_used) {
                send(clients[i].socket_fd, (const char *)g->board, sizeof(g->board), 0);
            }
        }
        pthread_mutex_unlock(&mutex_clients);

        pthread_mutex_lock(&g->snake_mutex);
        for (int i = 0; i < g->snake_size; i++) {
            if (g->snakes[i].is_alive) {
                move_snake(&g->snakes[i]);
            }
        }
        check_collision(g);
        update_food(g);
        draw_board(g);
        pthread_mutex_unlock(&g->snake_mutex);



        usleep(DELAY);
    }
}

int main() {
    srand(time(NULL));
    Game g;
    init_game(&g, BOARD_SIZE, MAX_CLIENTS, FOOD_SIZE);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].is_used = 0; // Ustaw wszystkich klientów jako nieużywanych
    }
    // Utwórz gniazdo serwera
    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd == -1) {
        perror("Blad podczas tworzenia gniazda serwera");
        exit(EXIT_FAILURE);
    }

    // Skonfiguruj adres serwera
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_port = htons(8888); // Przykładowy port 8888

    // Przypisz adres serwera do gniazda
    if (bind(server_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Blad podczas przypisywania adresu do gniazda");
        exit(EXIT_FAILURE);
    }

    // Nasłuchuj połączeń
    if (listen(server_socket_fd, 5) == -1) {
        perror("Blad podczas nasluchiwania na gniezdzie");
        exit(EXIT_FAILURE);
    }

    // Utwórz wątek obsługujący logikę gry
    pthread_t game_thread;
    if (pthread_create(&game_thread, NULL, game_logic, &g) != 0) {
        perror("Blad podczas tworzenia watku gry");
        exit(EXIT_FAILURE);
    }

    printf("Server running at address: %s:%d\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

    // Główna pętla serwera
    while (1) {
        // Akceptuj przychodzące połączenia
        struct sockaddr_in client_address;
        socklen_t client_addr_len = sizeof(client_address);
        int client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_address, &client_addr_len);
        if (client_socket_fd == -1) {
            perror("Blad podczas akceptowania polaczenia");
            continue;
        }

        // Dodaj klienta do listy
        if (num_clients < MAX_CLIENTS) {
            //dodawanie klienta do listy
            pthread_mutex_lock(&mutex_clients);
            int client_id = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].is_used == 0) {
                    clients[i].socket_fd = client_socket_fd;
                    clients[i].address = client_address;
                    clients[i].is_used = 1;
                    client_id = i;
                    break;
                }
            }
            Client_Register *arg = malloc(sizeof(*arg));
            arg->client_id = client_id;
            arg->g = &g;

            pthread_t client_thread;
            if (pthread_create(&client_thread, NULL, client_handler, arg) != 0) {
                perror("Blad podczas tworzenia watku dla klienta");
                close(client_socket_fd);
            }

            printf("Nowy klient dolaczyl, adres: %s i id: %d \n", inet_ntoa(client_address.sin_addr), client_id);

            num_clients++;
            pthread_mutex_unlock(&mutex_clients);
        } else {
            //TODO Wyslij klientowi informacje o odmowie polaczenia
            printf("Odmowa polaczenia - maksymalna liczba klientow osiagnieta\n");
            close(client_socket_fd);
        }
    }

    // Zamknij gniazdo serwera
    close(server_socket_fd);

    return 0;
}
