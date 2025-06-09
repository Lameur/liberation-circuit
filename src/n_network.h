/*
 * Liberation Circuit - Network Module Header
 * Cross-platform networking support for LAN multiplayer
 */

#ifndef N_NETWORK_H
#define N_NETWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Platform-specific networking includes
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    
    typedef SOCKET socket_t;
    typedef int socklen_t;
    #define SOCKET_ERROR_CODE WSAGetLastError()
    #define CLOSE_SOCKET(s) closesocket(s)
    #define SOCKET_WOULD_BLOCK WSAEWOULDBLOCK
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
    
#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <netdb.h>
    
    typedef int socket_t;
    #define SOCKET_ERROR_CODE errno
    #define CLOSE_SOCKET(s) close(s)
    #define SOCKET_WOULD_BLOCK EWOULDBLOCK
    #define INVALID_SOCKET_VALUE -1
    
#endif

// Network configuration
#define NETWORK_MAX_PLAYERS 8
#define NETWORK_DEFAULT_PORT 7777
#define NETWORK_BROADCAST_PORT 7778
#define NETWORK_BUFFER_SIZE 4096
#define NETWORK_MAX_MESSAGE_SIZE 1024
#define NETWORK_DISCOVERY_INTERVAL 1000  // milliseconds
#define NETWORK_TIMEOUT 5000             // milliseconds
#define NETWORK_MAX_HOSTNAME 256
#define NETWORK_MAX_GAME_NAME 64
#define NETWORK_PROTOCOL_VERSION 1

// Network states
typedef enum {
    NETWORK_STATE_DISCONNECTED = 0,
    NETWORK_STATE_HOSTING,
    NETWORK_STATE_CONNECTING,
    NETWORK_STATE_CONNECTED,
    NETWORK_STATE_ERROR
} network_state_t;

// Message types
typedef enum {
    MSG_DISCOVERY_REQUEST = 1,
    MSG_DISCOVERY_RESPONSE,
    MSG_JOIN_REQUEST,
    MSG_JOIN_RESPONSE,
    MSG_PLAYER_LIST,
    MSG_GAME_START,
    MSG_GAME_DATA,
    MSG_PLAYER_DISCONNECT,
    MSG_PING,
    MSG_PONG,
    MSG_CHAT,
    MSG_GAME_STATE_SYNC,
    MSG_TURN_DATA,
    MSG_ERROR
} message_type_t;

// Network message header
typedef struct {
    uint32_t magic;           // Protocol magic number
    uint16_t version;         // Protocol version
    uint16_t type;           // Message type
    uint32_t size;           // Payload size
    uint32_t sequence;       // Sequence number
    uint32_t timestamp;      // Timestamp
} __attribute__((packed)) message_header_t;

// Player information
typedef struct {
    uint32_t player_id;
    char name[32];
    char hostname[NETWORK_MAX_HOSTNAME];
    uint32_t ip_address;
    uint16_t port;
    uint32_t last_ping;
    int connected;
} network_player_t;

// Game discovery information
typedef struct {
    char game_name[NETWORK_MAX_GAME_NAME];
    char host_name[32];
    uint32_t host_ip;
    uint16_t host_port;
    uint8_t current_players;
    uint8_t max_players;
    uint32_t game_id;
    uint32_t last_seen;
} game_info_t;

// Network context
typedef struct {
    network_state_t state;
    
    // Socket information
    socket_t server_socket;
    socket_t broadcast_socket;
    uint16_t local_port;
    
    // Player management
    network_player_t players[NETWORK_MAX_PLAYERS];
    int player_count;
    uint32_t local_player_id;
    
    // Game information
    char game_name[NETWORK_MAX_GAME_NAME];
    uint32_t game_id;
    int is_host;
    
    // Discovery
    game_info_t discovered_games[16];
    int discovered_game_count;
    uint32_t last_discovery_time;
    
    // Message handling
    uint32_t next_sequence;
    char receive_buffer[NETWORK_BUFFER_SIZE];
    char send_buffer[NETWORK_BUFFER_SIZE];
    
    // Statistics
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t messages_sent;
    uint32_t messages_received;
    uint32_t errors;
    
} network_context_t;

// Network callback types
typedef void (*network_player_joined_callback_t)(uint32_t player_id, const char* player_name);
typedef void (*network_player_left_callback_t)(uint32_t player_id);
typedef void (*network_game_data_callback_t)(uint32_t player_id, const void* data, size_t size);
typedef void (*network_chat_callback_t)(uint32_t player_id, const char* message);
typedef void (*network_error_callback_t)(const char* error_message);

// Network callbacks structure
typedef struct {
    network_player_joined_callback_t on_player_joined;
    network_player_left_callback_t on_player_left;
    network_game_data_callback_t on_game_data;
    network_chat_callback_t on_chat;
    network_error_callback_t on_error;
} network_callbacks_t;

// Global network context
extern network_context_t g_network;
extern network_callbacks_t g_network_callbacks;

// Core network functions
int network_init(void);
void network_shutdown(void);
void network_update(void);

// Host/Join functions
int network_host_game(const char* game_name, uint16_t port);
int network_join_game(const char* hostname, uint16_t port, const char* player_name);
void network_disconnect(void);

// Discovery functions
int network_start_discovery(void);
void network_stop_discovery(void);
int network_get_discovered_games(game_info_t* games, int max_games);

// Messaging functions
int network_send_to_player(uint32_t player_id, message_type_t type, const void* data, size_t size);
int network_send_to_all(message_type_t type, const void* data, size_t size);
int network_broadcast_discovery(void);

// Player management
int network_get_player_count(void);
network_player_t* network_get_player(uint32_t player_id);
network_player_t* network_get_all_players(void);
uint32_t network_get_local_player_id(void);

// Game state synchronization
int network_send_game_state(const void* game_state, size_t size);
int network_send_turn_data(const void* turn_data, size_t size);

// Chat functions
int network_send_chat(const char* message);

// Utility functions
const char* network_get_state_string(network_state_t state);
const char* network_get_error_string(int error_code);
uint32_t network_get_local_ip(void);
void network_set_callbacks(const network_callbacks_t* callbacks);

// Statistics
void network_get_statistics(uint32_t* bytes_sent, uint32_t* bytes_received, 
                           uint32_t* messages_sent, uint32_t* messages_received, 
                           uint32_t* errors);

// Internal helper functions (not for external use)
int _network_create_socket(socket_t* sock, int type);
int _network_set_nonblocking(socket_t sock);
int _network_bind_socket(socket_t sock, uint16_t port);
int _network_send_message(socket_t sock, struct sockaddr_in* addr, 
                         message_type_t type, const void* data, size_t size);
int _network_receive_message(socket_t sock, struct sockaddr_in* from_addr, 
                           message_type_t* type, void* data, size_t* size);
void _network_handle_message(struct sockaddr_in* from_addr, message_type_t type, 
                           const void* data, size_t size);
uint32_t _network_generate_player_id(void);
uint32_t _network_generate_game_id(void);

#endif // N_NETWORK_H