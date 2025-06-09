/*
 * Liberation Circuit - Multiplayer Public Interface
 * Minimal header for multiplayer integration without circular dependencies
 */

#ifndef S_MULTIPLAYER_PUBLIC_H
#define S_MULTIPLAYER_PUBLIC_H

#ifdef NETWORK_ENABLED

// Forward declarations
typedef enum {
    MP_MENU_MAIN = 0,
    MP_MENU_HOST_SETUP,
    MP_MENU_GAME_BROWSER,
    MP_MENU_JOIN_SETUP,
    MP_MENU_LOBBY,
    MP_MENU_CONNECTING,
    MP_MENU_ERROR,
    MP_MENU_SETTINGS
} multiplayer_menu_state_t;

// Public multiplayer functions
int multiplayer_menu_init(void);
void multiplayer_menu_shutdown(void);
void multiplayer_menu_update(void);
void multiplayer_menu_draw(void);
void multiplayer_menu_handle_input(void);
void multiplayer_menu_set_state(multiplayer_menu_state_t new_state);
void multiplayer_menu_show_error(const char* error_message, uint32_t timeout);

// Network event callbacks (called by network system)
void multiplayer_on_player_joined(uint32_t player_id, const char* player_name);
void multiplayer_on_player_left(uint32_t player_id);
void multiplayer_on_game_data(uint32_t player_id, const void* data, size_t size);
void multiplayer_on_chat(uint32_t player_id, const char* message);
void multiplayer_on_network_error(const char* error_message);

#else // !NETWORK_ENABLED

// Stub implementations when networking is disabled
static inline int multiplayer_menu_init(void) { return 0; }
static inline void multiplayer_menu_shutdown(void) {}
static inline void multiplayer_menu_update(void) {}
static inline void multiplayer_menu_draw(void) {}
static inline void multiplayer_menu_handle_input(void) {}
static inline void multiplayer_menu_set_state(int state) { (void)state; }
static inline void multiplayer_menu_show_error(const char* msg, uint32_t timeout) { (void)msg; (void)timeout; }

#endif // NETWORK_ENABLED

#endif // S_MULTIPLAYER_PUBLIC_H