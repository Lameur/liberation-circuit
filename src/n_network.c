/*
 * Liberation Circuit - Network Module Implementation
 * Cross-platform networking support for LAN multiplayer
 */

#include <allegro5/allegro.h>
#include "n_network.h"
#include "m_config.h"
#include "g_header.h"
#include "i_error.h"

#include <time.h>

// Global network context and callbacks
network_context_t g_network;
network_callbacks_t g_network_callbacks;

// Protocol magic number
#define NETWORK_MAGIC 0x4C494243  // "LIBC" in hex

// Network initialization flag
static int network_initialized = 0;

// Platform-specific initialization
static int platform_network_init(void)
{
#ifdef _WIN32
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        return 0;
    }
#endif
    return 1;
}

// Platform-specific cleanup
static void platform_network_cleanup(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

// Get current timestamp in milliseconds
static uint32_t get_timestamp(void)
{
    return (uint32_t)(time(NULL) * 1000);
}

// Initialize network subsystem
int network_init(void)
{
    if (network_initialized) {
        return 1;
    }
    
    // Initialize platform-specific networking
    if (!platform_network_init()) {
        return 0;
    }
    
    // Initialize network context
    memset(&g_network, 0, sizeof(network_context_t));
    memset(&g_network_callbacks, 0, sizeof(network_callbacks_t));
    
    g_network.state = NETWORK_STATE_DISCONNECTED;
    g_network.server_socket = INVALID_SOCKET_VALUE;
    g_network.broadcast_socket = INVALID_SOCKET_VALUE;
    g_network.local_port = NETWORK_DEFAULT_PORT;
    g_network.next_sequence = 1;
    g_network.game_id = _network_generate_game_id();
    g_network.local_player_id = _network_generate_player_id();
    
    strcpy(g_network.game_name, "Liberation Circuit Game");
    
    network_initialized = 1;
    return 1;
}

// Shutdown network subsystem
void network_shutdown(void)
{
    if (!network_initialized) {
        return;
    }
    
    network_disconnect();
    platform_network_cleanup();
    network_initialized = 0;
}

// Create and configure a socket
int _network_create_socket(socket_t* sock, int type)
{
    *sock = socket(AF_INET, type, 0);
    if (*sock == INVALID_SOCKET_VALUE) {
        return 0;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
    if (type == SOCK_DGRAM) {
        // Enable broadcast for UDP sockets
        setsockopt(*sock, SOL_SOCKET, SO_BROADCAST, (const char*)&opt, sizeof(opt));
    }
    
    return _network_set_nonblocking(*sock);
}

// Set socket to non-blocking mode
int _network_set_nonblocking(socket_t sock)
{
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) return 0;
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

// Bind socket to port
int _network_bind_socket(socket_t sock, uint16_t port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    return bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0;
}

// Send a network message
int _network_send_message(socket_t sock, struct sockaddr_in* addr, 
                         message_type_t type, const void* data, size_t size)
{
    if (size > NETWORK_MAX_MESSAGE_SIZE) {
        return 0;
    }
    
    message_header_t header;
    header.magic = htonl(NETWORK_MAGIC);
    header.version = htons(NETWORK_PROTOCOL_VERSION);
    header.type = htons(type);
    header.size = htonl((uint32_t)size);
    header.sequence = htonl(g_network.next_sequence++);
    header.timestamp = htonl(get_timestamp());
    
    // Prepare message buffer
    char message[sizeof(message_header_t) + NETWORK_MAX_MESSAGE_SIZE];
    memcpy(message, &header, sizeof(message_header_t));
    if (data && size > 0) {
        memcpy(message + sizeof(message_header_t), data, size);
    }
    
    size_t total_size = sizeof(message_header_t) + size;
    ssize_t sent = sendto(sock, message, total_size, 0, 
                         (struct sockaddr*)addr, sizeof(struct sockaddr_in));
    
    if (sent == total_size) {
        g_network.bytes_sent += sent;
        g_network.messages_sent++;
        return 1;
    }
    
    g_network.errors++;
    return 0;
}

// Receive a network message
int _network_receive_message(socket_t sock, struct sockaddr_in* from_addr, 
                           message_type_t* type, void* data, size_t* size)
{
    socklen_t addr_len = sizeof(struct sockaddr_in);
    ssize_t received = recvfrom(sock, g_network.receive_buffer, NETWORK_BUFFER_SIZE, 0,
                               (struct sockaddr*)from_addr, &addr_len);
    
    if (received <= 0) {
        int error = SOCKET_ERROR_CODE;
        if (error == SOCKET_WOULD_BLOCK) {
            return 0; // No data available
        }
        g_network.errors++;
        return -1; // Real error
    }
    
    if (received < sizeof(message_header_t)) {
        g_network.errors++;
        return -1; // Invalid message
    }
    
    message_header_t* header = (message_header_t*)g_network.receive_buffer;
    
    // Validate header
    if (ntohl(header->magic) != NETWORK_MAGIC) {
        g_network.errors++;
        return -1;
    }
    
    if (ntohs(header->version) != NETWORK_PROTOCOL_VERSION) {
        g_network.errors++;
        return -1;
    }
    
    *type = (message_type_t)ntohs(header->type);
    size_t payload_size = ntohl(header->size);
    
    if (payload_size > NETWORK_MAX_MESSAGE_SIZE || 
        received != sizeof(message_header_t) + payload_size) {
        g_network.errors++;
        return -1;
    }
    
    if (data && size) {
        *size = payload_size;
        if (payload_size > 0) {
            memcpy(data, g_network.receive_buffer + sizeof(message_header_t), payload_size);
        }
    }
    
    g_network.bytes_received += received;
    g_network.messages_received++;
    return 1;
}

// Generate unique player ID
uint32_t _network_generate_player_id(void)
{
    static uint32_t counter = 1;
    return (get_timestamp() & 0xFFFFFF00) | (counter++ & 0xFF);
}

// Generate unique game ID
uint32_t _network_generate_game_id(void)
{
    return get_timestamp();
}

// Get local IP address
uint32_t network_get_local_ip(void)
{
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        return INADDR_LOOPBACK;
    }
    
    struct hostent* host_entry = gethostbyname(hostname);
    if (host_entry == NULL) {
        return INADDR_LOOPBACK;
    }
    
    return ((struct in_addr*)host_entry->h_addr_list[0])->s_addr;
}

// Host a new game
int network_host_game(const char* game_name, uint16_t port)
{
    if (g_network.state != NETWORK_STATE_DISCONNECTED) {
        return 0;
    }
    
    // Create server socket
    if (!_network_create_socket(&g_network.server_socket, SOCK_DGRAM)) {
        return 0;
    }
    
    if (!_network_bind_socket(g_network.server_socket, port)) {
        CLOSE_SOCKET(g_network.server_socket);
        g_network.server_socket = INVALID_SOCKET_VALUE;
        return 0;
    }
    
    // Create broadcast socket for discovery
    if (!_network_create_socket(&g_network.broadcast_socket, SOCK_DGRAM)) {
        CLOSE_SOCKET(g_network.server_socket);
        g_network.server_socket = INVALID_SOCKET_VALUE;
        return 0;
    }
    
    if (!_network_bind_socket(g_network.broadcast_socket, NETWORK_BROADCAST_PORT)) {
        CLOSE_SOCKET(g_network.server_socket);
        CLOSE_SOCKET(g_network.broadcast_socket);
        g_network.server_socket = INVALID_SOCKET_VALUE;
        g_network.broadcast_socket = INVALID_SOCKET_VALUE;
        return 0;
    }
    
    g_network.local_port = port;
    g_network.is_host = 1;
    g_network.state = NETWORK_STATE_HOSTING;
    g_network.player_count = 0;
    
    if (game_name) {
        strncpy(g_network.game_name, game_name, NETWORK_MAX_GAME_NAME - 1);
        g_network.game_name[NETWORK_MAX_GAME_NAME - 1] = '\0';
    }
    
    return 1;
}

// Join an existing game
int network_join_game(const char* hostname, uint16_t port, const char* player_name)
{
    if (g_network.state != NETWORK_STATE_DISCONNECTED) {
        return 0;
    }
    
    // Create client socket
    if (!_network_create_socket(&g_network.server_socket, SOCK_DGRAM)) {
        return 0;
    }
    
    // Resolve hostname
    struct hostent* host_entry = gethostbyname(hostname);
    if (host_entry == NULL) {
        CLOSE_SOCKET(g_network.server_socket);
        g_network.server_socket = INVALID_SOCKET_VALUE;
        return 0;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr = *((struct in_addr*)host_entry->h_addr);
    server_addr.sin_port = htons(port);
    
    // Send join request
    char join_data[64];
    if (player_name) {
        strncpy(join_data, player_name, sizeof(join_data) - 1);
        join_data[sizeof(join_data) - 1] = '\0';
    } else {
        strcpy(join_data, "Player");
    }
    
    if (!_network_send_message(g_network.server_socket, &server_addr, 
                              MSG_JOIN_REQUEST, join_data, strlen(join_data) + 1)) {
        CLOSE_SOCKET(g_network.server_socket);
        g_network.server_socket = INVALID_SOCKET_VALUE;
        return 0;
    }
    
    g_network.is_host = 0;
    g_network.state = NETWORK_STATE_CONNECTING;
    
    return 1;
}

// Disconnect from game
void network_disconnect(void)
{
    if (g_network.state == NETWORK_STATE_DISCONNECTED) {
        return;
    }
    
    // Send disconnect message to all players
    if (g_network.state == NETWORK_STATE_CONNECTED || g_network.state == NETWORK_STATE_HOSTING) {
        network_send_to_all(MSG_PLAYER_DISCONNECT, NULL, 0);
    }
    
    // Close sockets
    if (g_network.server_socket != INVALID_SOCKET_VALUE) {
        CLOSE_SOCKET(g_network.server_socket);
        g_network.server_socket = INVALID_SOCKET_VALUE;
    }
    
    if (g_network.broadcast_socket != INVALID_SOCKET_VALUE) {
        CLOSE_SOCKET(g_network.broadcast_socket);
        g_network.broadcast_socket = INVALID_SOCKET_VALUE;
    }
    
    // Reset state
    g_network.state = NETWORK_STATE_DISCONNECTED;
    g_network.player_count = 0;
    g_network.is_host = 0;
    memset(g_network.players, 0, sizeof(g_network.players));
}

// Start game discovery
int network_start_discovery(void)
{
    if (!_network_create_socket(&g_network.broadcast_socket, SOCK_DGRAM)) {
        return 0;
    }
    
    g_network.discovered_game_count = 0;
    g_network.last_discovery_time = get_timestamp();
    
    return network_broadcast_discovery();
}

// Stop game discovery
void network_stop_discovery(void)
{
    if (g_network.broadcast_socket != INVALID_SOCKET_VALUE) {
        CLOSE_SOCKET(g_network.broadcast_socket);
        g_network.broadcast_socket = INVALID_SOCKET_VALUE;
    }
}

// Broadcast discovery request
int network_broadcast_discovery(void)
{
    if (g_network.broadcast_socket == INVALID_SOCKET_VALUE) {
        return 0;
    }
    
    struct sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;
    broadcast_addr.sin_port = htons(NETWORK_BROADCAST_PORT);
    
    return _network_send_message(g_network.broadcast_socket, &broadcast_addr,
                                MSG_DISCOVERY_REQUEST, NULL, 0);
}

// Handle incoming network messages
void _network_handle_message(struct sockaddr_in* from_addr, message_type_t type, 
                           const void* data, size_t size)
{
    switch (type) {
        case MSG_DISCOVERY_REQUEST:
            if (g_network.is_host && g_network.state == NETWORK_STATE_HOSTING) {
                // Respond with game information
                game_info_t game_info;
                memset(&game_info, 0, sizeof(game_info));
                strcpy(game_info.game_name, g_network.game_name);
                strcpy(game_info.host_name, "Host");
                game_info.host_ip = network_get_local_ip();
                game_info.host_port = g_network.local_port;
                game_info.current_players = g_network.player_count;
                game_info.max_players = NETWORK_MAX_PLAYERS;
                game_info.game_id = g_network.game_id;
                
                _network_send_message(g_network.broadcast_socket, from_addr,
                                    MSG_DISCOVERY_RESPONSE, &game_info, sizeof(game_info));
            }
            break;
            
        case MSG_DISCOVERY_RESPONSE:
            if (size == sizeof(game_info_t) && g_network.discovered_game_count < 16) {
                game_info_t* game_info = (game_info_t*)data;
                game_info->last_seen = get_timestamp();
                
                // Check if game already exists in list
                int found = 0;
                for (int i = 0; i < g_network.discovered_game_count; i++) {
                    if (g_network.discovered_games[i].game_id == game_info->game_id) {
                        g_network.discovered_games[i] = *game_info;
                        found = 1;
                        break;
                    }
                }
                
                if (!found) {
                    g_network.discovered_games[g_network.discovered_game_count++] = *game_info;
                }
            }
            break;
            
        case MSG_JOIN_REQUEST:
            if (g_network.is_host && g_network.player_count < NETWORK_MAX_PLAYERS) {
                // Add new player
                network_player_t* player = &g_network.players[g_network.player_count];
                player->player_id = _network_generate_player_id();
                player->ip_address = from_addr->sin_addr.s_addr;
                player->port = ntohs(from_addr->sin_port);
                player->connected = 1;
                player->last_ping = get_timestamp();
                
                if (data && size > 0) {
                    strncpy(player->name, (const char*)data, sizeof(player->name) - 1);
                    player->name[sizeof(player->name) - 1] = '\0';
                } else {
                    sprintf(player->name, "Player%d", g_network.player_count + 1);
                }
                
                inet_ntop(AF_INET, &from_addr->sin_addr, player->hostname, NETWORK_MAX_HOSTNAME);
                
                g_network.player_count++;
                
                // Send join response
                _network_send_message(g_network.server_socket, from_addr,
                                    MSG_JOIN_RESPONSE, &player->player_id, sizeof(uint32_t));
                
                // Notify callback
                if (g_network_callbacks.on_player_joined) {
                    g_network_callbacks.on_player_joined(player->player_id, player->name);
                }
            }
            break;
            
        case MSG_JOIN_RESPONSE:
            if (g_network.state == NETWORK_STATE_CONNECTING && size == sizeof(uint32_t)) {
                g_network.local_player_id = *(uint32_t*)data;
                g_network.state = NETWORK_STATE_CONNECTED;
            }
            break;
            
        case MSG_GAME_DATA:
            if (g_network_callbacks.on_game_data) {
                // Find sender player ID
                uint32_t sender_id = 0;
                for (int i = 0; i < g_network.player_count; i++) {
                    if (g_network.players[i].ip_address == from_addr->sin_addr.s_addr &&
                        g_network.players[i].port == ntohs(from_addr->sin_port)) {
                        sender_id = g_network.players[i].player_id;
                        break;
                    }
                }
                g_network_callbacks.on_game_data(sender_id, data, size);
            }
            break;
            
        case MSG_CHAT:
            if (g_network_callbacks.on_chat && data && size > 0) {
                // Find sender player ID
                uint32_t sender_id = 0;
                for (int i = 0; i < g_network.player_count; i++) {
                    if (g_network.players[i].ip_address == from_addr->sin_addr.s_addr &&
                        g_network.players[i].port == ntohs(from_addr->sin_port)) {
                        sender_id = g_network.players[i].player_id;
                        break;
                    }
                }
                g_network_callbacks.on_chat(sender_id, (const char*)data);
            }
            break;
            
        case MSG_PLAYER_DISCONNECT:
            // Find and remove player
            for (int i = 0; i < g_network.player_count; i++) {
                if (g_network.players[i].ip_address == from_addr->sin_addr.s_addr &&
                    g_network.players[i].port == ntohs(from_addr->sin_port)) {
                    
                    if (g_network_callbacks.on_player_left) {
                        g_network_callbacks.on_player_left(g_network.players[i].player_id);
                    }
                    
                    // Remove player from list
                    for (int j = i; j < g_network.player_count - 1; j++) {
                        g_network.players[j] = g_network.players[j + 1];
                    }
                    g_network.player_count--;
                    break;
                }
            }
            break;
    }
}

// Main network update function
void network_update(void)
{
    if (!network_initialized || g_network.state == NETWORK_STATE_DISCONNECTED) {
        return;
    }
    
    // Process incoming messages
    struct sockaddr_in from_addr;
    message_type_t type;
    char message_data[NETWORK_MAX_MESSAGE_SIZE];
    size_t size;
    
    // Check server socket
    if (g_network.server_socket != INVALID_SOCKET_VALUE) {
        int result = _network_receive_message(g_network.server_socket, &from_addr, &type, message_data, &size);
        if (result > 0) {
            _network_handle_message(&from_addr, type, message_data, size);
        }
    }
    
    // Check broadcast socket
    if (g_network.broadcast_socket != INVALID_SOCKET_VALUE) {
        int result = _network_receive_message(g_network.broadcast_socket, &from_addr, &type, message_data, &size);
        if (result > 0) {
            _network_handle_message(&from_addr, type, message_data, size);
        }
    }
    
    // Send periodic discovery requests
    if (g_network.broadcast_socket != INVALID_SOCKET_VALUE && 
        !g_network.is_host && 
        get_timestamp() - g_network.last_discovery_time > NETWORK_DISCOVERY_INTERVAL) {
        network_broadcast_discovery();
        g_network.last_discovery_time = get_timestamp();
    }
}

// Send message to specific player
int network_send_to_player(uint32_t player_id, message_type_t type, const void* data, size_t size)
{
    if (g_network.server_socket == INVALID_SOCKET_VALUE) {
        return 0;
    }
    
    // Find player
    for (int i = 0; i < g_network.player_count; i++) {
        if (g_network.players[i].player_id == player_id) {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = g_network.players[i].ip_address;
            addr.sin_port = htons(g_network.players[i].port);
            
            return _network_send_message(g_network.server_socket, &addr, type, data, size);
        }
    }
    
    return 0;
}

// Send message to all players
int network_send_to_all(message_type_t type, const void* data, size_t size)
{
    if (g_network.server_socket == INVALID_SOCKET_VALUE) {
        return 0;
    }
    
    int success_count = 0;
    for (int i = 0; i < g_network.player_count; i++) {
        if (g_network.players[i].connected) {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = g_network.players[i].ip_address;
            addr.sin_port = htons(g_network.players[i].port);
            
            if (_network_send_message(g_network.server_socket, &addr, type, data, size)) {
                success_count++;
            }
        }
    }
    
    return success_count;
}

// Get discovered games
int network_get_discovered_games(game_info_t* games, int max_games)
{
    int count = g_network.discovered_game_count;
    if (count > max_games) {
        count = max_games;
    }
    
    if (games) {
        memcpy(games, g_network.discovered_games, count * sizeof(game_info_t));
    }
    
    return count;
}

// Send chat message
int network_send_chat(const char* message)
{
    if (!message) {
        return 0;
    }
    
    return network_send_to_all(MSG_CHAT, message, strlen(message) + 1);
}

// Send game state
int network_send_game_state(const void* game_state, size_t size)
{
    return network_send_to_all(MSG_GAME_STATE_SYNC, game_state, size);
}

// Send turn data
int network_send_turn_data(const void* turn_data, size_t size)
{
    return network_send_to_all(MSG_TURN_DATA, turn_data, size);
}

// Get network state as string
const char* network_get_state_string(network_state_t state)
{
    switch (state) {
        case NETWORK_STATE_DISCONNECTED: return "Disconnected";
        case NETWORK_STATE_HOSTING: return "Hosting";
        case NETWORK_STATE_CONNECTING: return "Connecting";
        case NETWORK_STATE_CONNECTED: return "Connected";
        case NETWORK_STATE_ERROR: return "Error";
        default: return "Unknown";
    }
}

// Get error string
const char* network_get_error_string(int error_code)
{
#ifdef _WIN32
    static char error_buffer[256];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 error_buffer, sizeof(error_buffer), NULL);
    return error_buffer;
#else
    return strerror(error_code);
#endif
}

// Set network callbacks
void network_set_callbacks(const network_callbacks_t* callbacks)
{
    if (callbacks) {
        g_network_callbacks = *callbacks;
    } else {
        memset(&g_network_callbacks, 0, sizeof(network_callbacks_t));
    }
}

// Get player count
int network_get_player_count(void)
{
    return g_network.player_count;
}

// Get specific player
network_player_t* network_get_player(uint32_t player_id)
{
    for (int i = 0; i < g_network.player_count; i++) {
        if (g_network.players[i].player_id == player_id) {
            return &g_network.players[i];
        }
    }
    return NULL;
}

// Get all players
network_player_t* network_get_all_players(void)
{
    return g_network.players;
}

// Get local player ID
uint32_t network_get_local_player_id(void)
{
    return g_network.local_player_id;
}

// Get network statistics
void network_get_statistics(uint32_t* bytes_sent, uint32_t* bytes_received, 
                           uint32_t* messages_sent, uint32_t* messages_received, 
                           uint32_t* errors)
{
    if (bytes_sent) *bytes_sent = g_network.bytes_sent;
    if (bytes_received) *bytes_received = g_network.bytes_received;
    if (messages_sent) *messages_sent = g_network.messages_sent;
    if (messages_received) *messages_received = g_network.messages_received;
    if (errors) *errors = g_network.errors;
}