/*
 * Liberation Circuit - Multiplayer Menu Interface Header
 * UI for LAN multiplayer game hosting and joining
 */

#ifndef S_MULTIPLAYER_H
#define S_MULTIPLAYER_H

#include "i_header.h"
#include "n_network.h"
#include "s_menu.h"

// Multiplayer menu states
typedef enum
{
  MP_MENU_MAIN = 0,		// Main multiplayer menu
  MP_MENU_HOST_SETUP,	// Host game setup
  MP_MENU_GAME_BROWSER, // Browse for games
  MP_MENU_JOIN_SETUP,	// Join game setup (enter IP)
  MP_MENU_LOBBY,		// Game lobby (waiting for players)
  MP_MENU_CONNECTING,	// Connecting to game
  MP_MENU_ERROR,		// Error display
  MP_MENU_SETTINGS		// Network settings
} multiplayer_menu_state_t;

// UI element types for multiplayer menus
typedef enum
{
  MP_UI_BUTTON = 0,
  MP_UI_TEXT_INPUT,
  MP_UI_LIST,
  MP_UI_LABEL,
  MP_UI_GAME_ENTRY,
  MP_UI_PLAYER_ENTRY,
  MP_UI_PROGRESS_BAR,
  MP_UI_CHECKBOX
} multiplayer_ui_element_type_t;

// Multiplayer UI element structure
typedef struct
{
  multiplayer_ui_element_type_t type;
  int x, y, w, h;
  int active;
  int selected;
  char text[256];
  char placeholder[64];
  void *data;
  void (*callback)(void *element, void *userdata);
  void *userdata;
} multiplayer_ui_element_t;

// Game browser entry
typedef struct
{
  game_info_t game_info;
  int ping;
  int selected;
  uint32_t last_update;
} game_browser_entry_t;

// Player list entry for lobby
typedef struct
{
  network_player_t player;
  int ready;
  int host;
  ALLEGRO_COLOR name_color;
} lobby_player_entry_t;

// Multiplayer menu context
typedef struct
{
  multiplayer_menu_state_t current_state;
  multiplayer_menu_state_t previous_state;

  // UI elements
  multiplayer_ui_element_t elements[32];
  int element_count;
  int selected_element;
  int scroll_offset;

  // Host game setup
  char host_game_name[NETWORK_MAX_GAME_NAME];
  char host_player_name[32];
  int host_port;
  int host_max_players;
  int host_password_protected;
  char host_password[32];

  // Join game setup
  char join_hostname[NETWORK_MAX_HOSTNAME];
  char join_player_name[32];
  int join_port;
  char join_password[32];

  // Game browser
  game_browser_entry_t browser_games[16];
  int browser_game_count;
  int browser_selected_game;
  uint32_t browser_last_refresh;
  int browser_auto_refresh;

  // Lobby
  lobby_player_entry_t lobby_players[NETWORK_MAX_PLAYERS];
  int lobby_player_count;
  int lobby_ready;
  int lobby_chat_visible;
  char lobby_chat_input[256];
  char lobby_chat_messages[16][256];
  int lobby_chat_message_count;
  int lobby_chat_scroll;

  // Connection status
  char status_message[256];
  uint32_t status_timeout;
  ALLEGRO_COLOR status_color;

  // Error handling
  char error_message[512];
  uint32_t error_timeout;

  // Settings
  int enable_upnp;
  int auto_discovery;
  int show_lan_only;
  char default_player_name[32];

  // Animation and effects
  float fade_alpha;
  uint32_t last_update_time;
  int transition_active;

} multiplayer_menu_context_t;

// Global multiplayer menu context
extern multiplayer_menu_context_t g_mp_menu;

// Core multiplayer menu functions
int multiplayer_menu_init(void);
void multiplayer_menu_shutdown(void);
void multiplayer_menu_update(void);
void multiplayer_menu_draw(void);
void multiplayer_menu_handle_input(void);

// State management
void multiplayer_menu_set_state(multiplayer_menu_state_t new_state);
void multiplayer_menu_return_to_previous(void);
void multiplayer_menu_show_error(const char *error_message, uint32_t timeout);
void multiplayer_menu_clear_error(void);

// UI element management
void multiplayer_ui_clear_elements(void);
int multiplayer_ui_add_button(int x, int y, int w, int h, const char *text,
							  void (*callback)(void *, void *), void *userdata);
int multiplayer_ui_add_text_input(int x, int y, int w, int h, char *buffer,
								  int max_length, const char *placeholder);
int multiplayer_ui_add_label(int x, int y, const char *text, ALLEGRO_COLOR color);
int multiplayer_ui_add_list(int x, int y, int w, int h, void *data, int item_count,
							void (*draw_item)(int index, int x, int y, int w, int h, void *data));
void multiplayer_ui_update_elements(void);
void multiplayer_ui_draw_elements(void);
void multiplayer_ui_handle_mouse(int mouse_x, int mouse_y, int mouse_buttons);
void multiplayer_ui_handle_keyboard(int key, int unichar);

// Host game functions
void multiplayer_host_init_ui(void);
void multiplayer_host_start_game(void);
void multiplayer_host_update(void);
void multiplayer_host_draw(void);

// Game browser functions
void multiplayer_browser_init_ui(void);
void multiplayer_browser_refresh(void);
void multiplayer_browser_update(void);
void multiplayer_browser_draw(void);
void multiplayer_browser_join_selected(void);

// Join game functions
void multiplayer_join_init_ui(void);
void multiplayer_join_connect(void);
void multiplayer_join_update(void);
void multiplayer_join_draw(void);

// Lobby functions
void multiplayer_lobby_init_ui(void);
void multiplayer_lobby_update(void);
void multiplayer_lobby_draw(void);
void multiplayer_lobby_toggle_ready(void);
void multiplayer_lobby_send_chat(const char *message);
void multiplayer_lobby_add_chat_message(const char *player_name, const char *message);
void multiplayer_lobby_leave(void);

// Settings functions
void multiplayer_settings_init_ui(void);
void multiplayer_settings_save(void);
void multiplayer_settings_load(void);
void multiplayer_settings_update(void);
void multiplayer_settings_draw(void);

// Network event callbacks
void multiplayer_on_player_joined(uint32_t player_id, const char *player_name);
void multiplayer_on_player_left(uint32_t player_id);
void multiplayer_on_game_data(uint32_t player_id, const void *data, size_t size);
void multiplayer_on_chat(uint32_t player_id, const char *message);
void multiplayer_on_network_error(const char *error_message);

// Utility functions
void multiplayer_format_ping(int ping_ms, char *buffer, size_t buffer_size);
void multiplayer_format_player_count(int current, int max, char *buffer, size_t buffer_size);
void multiplayer_get_local_player_name(char *buffer, size_t buffer_size);
void multiplayer_validate_player_name(char *name);
void multiplayer_validate_game_name(char *name);
int multiplayer_validate_hostname(const char *hostname);
int multiplayer_validate_port(int port);

// Drawing helper functions
void multiplayer_draw_background(void);
void multiplayer_draw_title(const char *title);
void multiplayer_draw_status(void);
void multiplayer_draw_game_entry(int x, int y, int w, int h,
								 const game_browser_entry_t *entry, int selected);
void multiplayer_draw_player_entry(int x, int y, int w, int h,
								   const lobby_player_entry_t *entry);
void multiplayer_draw_chat_box(int x, int y, int w, int h);
void multiplayer_draw_connection_status(int x, int y);
void multiplayer_draw_network_stats(int x, int y);

// Button callback functions
void mp_button_host_game(void *element, void *userdata);
void mp_button_join_game(void *element, void *userdata);
void mp_button_browse_games(void *element, void *userdata);
void mp_button_settings(void *element, void *userdata);
void mp_button_back(void *element, void *userdata);
void mp_button_start_hosting(void *element, void *userdata);
void mp_button_refresh_games(void *element, void *userdata);
void mp_button_connect_to_game(void *element, void *userdata);
void mp_button_toggle_ready(void *element, void *userdata);
void mp_button_leave_lobby(void *element, void *userdata);
void mp_button_send_chat(void *element, void *userdata);

// Configuration constants
#define MP_UI_MARGIN 20
#define MP_UI_BUTTON_HEIGHT 30
#define MP_UI_INPUT_HEIGHT 25
#define MP_UI_LINE_SPACING 35
#define MP_UI_SECTION_SPACING 50
#define MP_REFRESH_INTERVAL 2000
#define MP_STATUS_TIMEOUT 3000
#define MP_ERROR_TIMEOUT 5000
#define MP_CHAT_MAX_MESSAGES 16
#define MP_FADE_SPEED 0.05f

// Color definitions
#define MP_COLOR_BACKGROUND al_map_rgb(20, 20, 30)
#define MP_COLOR_PANEL al_map_rgba(40, 40, 60, 200)
#define MP_COLOR_BUTTON al_map_rgb(60, 60, 100)
#define MP_COLOR_BUTTON_HOVER al_map_rgb(80, 80, 120)
#define MP_COLOR_BUTTON_ACTIVE al_map_rgb(100, 100, 140)
#define MP_COLOR_TEXT al_map_rgb(255, 255, 255)
#define MP_COLOR_TEXT_DIM al_map_rgb(180, 180, 180)
#define MP_COLOR_ERROR al_map_rgb(255, 100, 100)
#define MP_COLOR_SUCCESS al_map_rgb(100, 255, 100)
#define MP_COLOR_WARNING al_map_rgb(255, 255, 100)
#define MP_COLOR_HOST al_map_rgb(255, 200, 100)
#define MP_COLOR_READY al_map_rgb(100, 255, 150)
#define MP_COLOR_NOT_READY al_map_rgb(255, 150, 100)

#endif // S_MULTIPLAYER_H