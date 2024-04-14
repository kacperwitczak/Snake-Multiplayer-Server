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
#define FOOD_SIZE 40
#define COLORS_SIZE 11

char colors[COLORS_SIZE] = { 'G', 'Y', 'P', 'E', 'S', 'V', 'T', 'O', 'C', 'M', 'B' };  // Available colors for the snakes

// Structure representing a client
typedef struct {
    int socket_fd;                // Client socket file descriptor
    struct sockaddr_in address;   // Client network address
    short is_used;                // Flag to check if the client slot is in use
} Client;

// Array to hold client data
Client clients[MAX_CLIENTS];
int num_clients = 0;  // Current number of connected clients

typedef struct {
    int x;
    int y;
} Point;

// Function to generate a random point within the board
Point random_point(int board_size) {
    Point p;
    p.x = rand() % board_size;

    
        
          
    

        
        Expand All
    
    @@ -42,75 +53,63 @@ Point random_point(int board_size) {
  
    p.y = rand() % board_size;
    return p;
}

typedef struct {
    Point snake[SNAKE_SIZE]; // Array of points to represent the snake's position
    int size;                // Current size of the snake
    char direction;          // Current movement direction of the snake
    short is_alive;          // Status if the snake is alive
    int size_to_add;         // Additional size to add to the snake (when eating food)
    char color;              // Color of the snake
} Snake;

typedef struct {
    Point food;      // Position of the food
    short is_eaten;  // Flag to check if the food has been eaten
} Food;

typedef struct {
    pthread_mutex_t snake_mutex;     // Mutex for thread-safe snake operations
    Snake snakes[MAX_CLIENTS];       // Array of snakes
    Food foods[FOOD_SIZE];           // Array of foods
    char board[BOARD_SIZE][BOARD_SIZE]; // The game board
    int board_size;                  // Size of the game board
    int snake_size;                  // Number of snakes
    int food_size;                   // Number of food items
} Game;

// Function to draw the entire board state
void draw_board(Game *g) {
    // Clear the board
    for (int i = 0; i < g->board_size; i++) {
        for (int j = 0; j < g->board_size; j++) {
            g->board[i][j] = ' ';
        }
    }

    // Draw snakes on the board
    for (int i = 0; i < g->snake_size; i++) {
        if (g->snakes[i].is_alive) {
            // Draw snake head with a specific color
            g->board[g->snakes[i].snake[0].y][g->snakes[i].snake[0].x] = g->snakes[i].color;
            // Draw the rest of the snake body
            for (int j = 1; j < g->snakes[i].size; j++) {
                g->board[g->snakes[i].snake[j].y][g->snakes[i].snake[j].x] = 'B';
            }
        }
    }

    // Draw food on the board
    for (int i = 0; i < g->food_size; i++) {
        if (!g->foods[i].is_eaten) {
            g->board[g->foods[i].food.y][g->foods[i].food.x] = 'F';
        }
    }
}

// Initialize the game environment
void init_game(Game *g, int board_size, int snake_size, int food_size) {
    // Initialize mutex for snakes
    if (pthread_mutex_init(&g->snake_mutex, NULL) != 0) {
        printf("\n Mutex init failed\n");
        exit(EXIT_FAILURE);
    }
    g->board_size = board_size;
    g->snake_size = snake_size;
    g->food_size = food_size;

    // Initialize snakes as not alive
    for (int i = 0; i < snake_size; i++) {
        g->snakes[i].is_alive = 0;
    }

    // Place food randomly on the board
    for (int i = 0; i < food_size; i++) {
        g->foods[i].food = random_point(board_size);
        g->foods[i].is_eaten = 0;

    
        
          
    

        
        Expand All
    
    @@ -119,18 +118,16 @@ void init_game(Game *g, int board_size, int snake_size, int food_size) {
  
    }
    draw_board(g);
}

// Clean up the game resources
void destroy_game(Game *g) {
    pthread_mutex_destroy(&g->snake_mutex);
}

// Generate a random direction for new snakes
char random_direction() {
    char directions[4] = {'U', 'D', 'L', 'R'};
    return directions[rand() % 4];
}

// Initialize a new snake in the game
void init_snake(Snake *s, int board_size) {
    s->size = 1;
    s->snake[0] = random_point(board_size);

    
        
          
    

        
        Expand All
    
    @@ -139,40 +136,36 @@ void init_snake(Snake *s, int board_size) {
  
    s->direction = random_direction();
    s->is_alive = 1;
    s->color = colors[rand() % COLORS_SIZE];
}

// Move a snake in its current direction
void move_snake(Snake *s) {
    // Add size to snake if needed
    if (s->size_to_add > 0 && s->size < SNAKE_SIZE - 1) {
        s->size += 1;
        s->size_to_add = 0;
    }

    // Calculate new head position based on the direction
    Point new_head = s->snake[0];
    switch (s->direction) {
        case 'U':
            new_head.y = (new_head.y - 1) % BOARD_SIZE;  // Wrap around the board
            break;
        case 'D':
            new_head.y = (new_head.y + 1) % BOARD_SIZE;
            break;
        case 'L':
            new_head.x = (new_head.x - 1) % BOARD_SIZE;
            break;
        case 'R':
            new_head.x = (new_head.x + 1) % BOARD_SIZE;
            break;
    }

    // Move the snake body
    for (int i = s->size - 1; i > 0; i--) {
        s->snake[i] = s->snake[i - 1];
    }
    s->snake[0] = new_head;
}

// Change the direction of a snake if it's not directly opposite
void change_direction(Snake *s, char new_direction) {
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

// Check for collisions in the game
void check_collision(Game *g) {
    for (int i = 0; i < g->snake_size; i++) {
        if (g->snakes[i].is_alive) {
            // Check for wall collisions
            if (g->snakes[i].snake[0].x < 0 || g->snakes[i].snake[0].x >= g->board_size ||
                g->snakes[i].snake[0].y < 0 || g->snakes[i].snake[0].y >= g->board_size) {
                g->snakes[i].is_alive = 0;  // Snake dies
            }

            // Check for collisions with other snakes
            for (int j = 0; j < g->snake_size; j++) {
                if (i != j && g->snakes[j].is_alive) {
                    for (int k = 0; k < g->snakes[j].size; k++) {
                        if (g->snakes[i].snake[0].x == g->snakes[j].snake[k].x &&
                            g->snakes[i].snake[0].y == g->snakes[j].snake[k].y) {
                            g->snakes[i].is_alive = 0;  // Snake dies
                        }
                    }
                }
            }

            // Check for collisions with food
            for (int j = 0; j < g->food_size; j++) {
                if (g->snakes[i].snake[0].x == g->foods[j].food.x &&
                    g->snakes[i].snake[0].y == g->foods[j].food.y) {
                    g->foods[j].is_eaten = 1;
                    g->snakes[i].size_to_add = 1;  // Snake grows
                }
            }

            // Reinitialize snake if it died
            if (g->snakes[i].is_alive == 0) {
                init_snake(&g->snakes[i], g->board_size);
            }
        }
    }
}

// Update the food positions in the game
void update_food(Game *g) {
    for (int i = 0; i < g->food_size; i++) {
        if (g->foods[i].is_eaten) {
            g->foods[i].food = random_point(g->board_size);
            g->foods[i].is_eaten = 0;
        }
    }
}

// Register a new player in the game
void register_new_player(int client_id, Game *g) {
    pthread_mutex_lock(&g->snake_mutex);
    init_snake(&g->snakes[client_id], g->board_size);
    pthread_mutex_unlock(&g->snake_mutex);
}

// Unregister a player from the game
void unregister_player(int client_id, Game *g) {
    pthread_mutex_lock(&g->snake_mutex);
    g->snakes[client_id].is_alive = 0;
    pthread_mutex_unlock(&g->snake_mutex);
}

// Set a new move direction for a player's snake
void set_new_move(int client_id, char move, Game *g) {
    pthread_mutex_lock(&g->snake_mutex);
    change_direction(&g->snakes[client_id], move);
    pthread_mutex_unlock(&g->snake_mutex);
}

// Mutex for synchronizing access to client data
pthread_mutex_t mutex_clients = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int client_id;
    Game *g;
} Client_Register;

// Thread function to handle client communication
void *client_handler(void *arg) {
    Client_Register client_reg = *((Client_Register *)arg);
    int client_id = client_reg.client_id;
    Game *client_game_handle = client_reg.g;
    Client c;
    char buffer[CLIENT_BUFFER_SIZE];
    pthread_mutex_lock(&mutex_clients);
    c = clients[client_id];
    pthread_mutex_unlock(&mutex_clients);
    int size = BOARD_SIZE;
    int net_size = htonl(size);
    send(c.socket_fd, &net_size, sizeof(net_size), 0);

    // Register new player
    register_new_player(client_id, client_game_handle);

    while (1) {
        // Receive message from client
        int bytes_received = recv(c.socket_fd, buffer, CLIENT_BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            // If error or client closed connection, remove client
            unregister_player(client_id, client_game_handle);

            pthread_mutex_lock(&mutex_clients);
            close(c.socket_fd);
            printf("Client at address %s and id %d disconnected\n", inet_ntoa(c.address.sin_addr), client_id);

            // Remove client from the list
            clients[client_id].is_used = 0;
            num_clients--;
            pthread_mutex_unlock(&mutex_clients);

            pthread_exit(NULL);
        }
        // Process message from client
        printf("Message from client: %c\n", buffer[0]);
        set_new_move(client_id, buffer[0], client_game_handle);
        memset(buffer, 0, CLIENT_BUFFER_SIZE);
    }
}

// Thread function to handle game logic
void *game_logic(void *arg) {
    Game *g = (Game *)arg;
    while (1) {
        // Send board to all clients
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

    if (pthread_mutex_init(&mutex_clients, NULL) != 0) {
        printf("\n Mutex init failed\n");
        return 1;
    }

    Game g;
    init_game(&g, BOARD_SIZE, MAX_CLIENTS, FOOD_SIZE);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].is_used = 0; // Set all clients as not in use
    }

    // Create server socket
    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd == -1) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_port = htons(8888); // Example port 8888

    // Bind address to socket
    if (bind(server_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error binding address to socket");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_socket_fd, 5) == -1) {
        perror("Error listening on socket");
        exit(EXIT_FAILURE);
    }

    // Create thread for handling game logic
    pthread_t game_thread;
    if (pthread_create(&game_thread, NULL, game_logic, &g) != 0) {
        perror("Error creating game thread");
        exit(EXIT_FAILURE);
    }

    printf("Server running at address: %s:%d\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

    // Main server loop
    while (1) {
        // Accept incoming connections
        struct sockaddr_in client_address;
        socklen_t client_addr_len = sizeof(client_address);
        int client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_address, &client_addr_len);
        if (client_socket_fd == -1) {
            perror("Error accepting connection");
            continue;
        }

        // Add client to the list
        if (num_clients < MAX_CLIENTS) {
            // Add client to the list
            pthread_mutex_lock(&mutex_clients);
            int client_id = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {

    
        
          
    

        
        Expand All
    
    @@ -413,22 +404,22 @@ int main() {
  
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
                perror("Error creating client thread");
                close(client_socket_fd);
            }

            printf("New client connected, address: %s and id: %d \n", inet_ntoa(client_address.sin_addr), client_id);

            num_clients++;
            pthread_mutex_unlock(&mutex_clients);
        } else {
            //TODO Send client information about connection refusal
            printf("Connection refused - maximum number of clients reached\n");
            close(client_socket_fd);
        }
    }

    // Close server socket
    close(server_socket_fd);
    destroy_game(&g);
    pthread_mutex_destroy(&mutex_clients);

    
          
            
    

          
          Expand Down
    
    
  
    return 0;
}
