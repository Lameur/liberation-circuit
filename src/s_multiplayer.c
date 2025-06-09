/*
 * Liberation Circuit - Multiplayer Menu Implementation
 * Basic LAN multiplayer menu system
 */

#ifdef NETWORK_ENABLED

#include "m_config.h"
#include "n_network.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

// Forward declarations to avoid circular dependencies
struct game_struct
{
  int phase;
};

struct fontstruct
{
  ALLEGRO_FONT *fnt;
};

struct settingsstruct
{
  int option[32]; // Assuming OPTIONS is around 32
};

// External declarations
extern struct game_struct game;
extern struct fontstruct font[16]; // Assuming FONTS is around 16
extern struct settingsstruct settings;

// Constants from other headers
#define GAME_PHASE_MENU 6
#define GAME_PHASE_MULTIPLAYER 7
#define OPTION_WINDOW_W 0
#define OPTION_WINDOW_H 1
#define FONT_SQUARE 0

// Multiplayer menu states
typedef enum
{
  MP_MENU_MAIN = 0,
  MP_MENU_HOST_SETUP,
  MP_MENU_GAME_BROWSER,
  MP_MENU_JOIN_SETUP,
  MP_MENU_LOBBY,
  MP_MENU_CONNECTING,
  MP_MENU_ERROR,
  MP_MENU_SETTINGS
} multiplayer_menu_state_t;

// UI element types
typedef struct
{
  char text[256];
  char placeholder[64];
  int x, y, w, h;
  int active;
  int selected;
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

  // Host game setup
  char host_game_name[64];
  char host_player_name[32];
  int host_port;
  int host_max_players;

  // Join game setup
  char join_hostname[256];
  char join_player_name[32];
  int join_port;

  // Game browser
  game_browser_entry_t browser_games[16];
  int browser_game_count;
  int browser_selected_game;
  uint32_t browser_last_refresh;
  int browser_auto_refresh;

  // Lobby
  lobby_player_entry_t lobby_players[8];
  int lobby_player_count;
  int lobby_ready;

  // Status messages
  char status_message[256];
  uint32_t status_timeout;
  ALLEGRO_COLOR status_color;
  char error_message[512];
  uint32_t error_timeout;

  // Settings
  char default_player_name[32];

} multiplayer_menu_context_t;

// Color definitions
#define MP_COLOR_BACKGROUND al_map_rgb(20, 20, 30)
#define MP_COLOR_TEXT al_map_rgb(255, 255, 255)
#define MP_COLOR_TEXT_DIM al_map_rgb(180, 180, 180)
#define MP_COLOR_ERROR al_map_rgb(255, 100, 100)
#define MP_COLOR_SUCCESS al_map_rgb(100, 255, 100)
#define MP_COLOR_WARNING al_map_rgb(255, 255, 100)
#define MP_COLOR_HOST al_map_rgb(255, 200, 100)
#define MP_COLOR_READY al_map_rgb(100, 255, 150)
#define MP_COLOR_NOT_READY al_map_rgb(255, 150, 100)

// Configuration constants
#define MP_REFRESH_INTERVAL 2000

// Function declarations
static void multiplayer_draw_main_menu(void);
static void multiplayer_draw_host_menu(void);
static void multiplayer_draw_join_menu(void);
static void multiplayer_draw_browser_menu(void);
static void multiplayer_draw_lobby_menu(void);
static void multiplayer_handle_main_input(void);
static void multiplayer_handle_host_input(void);
static void multiplayer_handle_join_input(void);
static void multiplayer_handle_browser_input(void);
static void multiplayer_handle_lobby_input(void);
static void multiplayer_init_default_settings(void);
static void multiplayer_show_status(const char *message, ALLEGRO_COLOR color, uint32_t timeout);
static int multiplayer_key_pressed(int allegro_key);

// Function implementations for multiplayer header interface
void multiplayer_menu_set_state(multiplayer_menu_state_t new_state);
void multiplayer_menu_return_to_previous(void);
void multiplayer_menu_show_error(const char *error_message, uint32_t timeout);
void multiplayer_draw_background(void);
void multiplayer_draw_title(const char *title);
void multiplayer_draw_status(void);
void multiplayer_draw_connection_status(int x, int y);
void multiplayer_host_start_game(void);
void multiplayer_join_connect(void);
void multiplayer_browser_refresh(void);
void multiplayer_browser_join_selected(void);
void multiplayer_lobby_toggle_ready(void);
void multiplayer_lobby_leave(void);
void multiplayer_on_player_joined(uint32_t player_id, const char *player_name);
void multiplayer_on_player_left(uint32_t player_id);
void multiplayer_on_game_data(uint32_t player_id, const void *data, size_t size);
void multiplayer_on_chat(uint32_t player_id, const char *message);
void multiplayer_on_network_error(const char *error_message);

// Global multiplayer menu context
multiplayer_menu_context_t g_mp_menu;

// Local function prototypes
static void multiplayer_draw_main_menu(void);
static void multiplayer_draw_host_menu(void);
static void multiplayer_draw_join_menu(void);
static void multiplayer_draw_browser_menu(void);
static void multiplayer_draw_lobby_menu(void);
static void multiplayer_handle_main_input(void);
static void multiplayer_handle_host_input(void);
static void multiplayer_handle_join_input(void);
static void multiplayer_handle_browser_input(void);
static void multiplayer_handle_lobby_input(void);
static void multiplayer_init_default_settings(void);

// Initialize multiplayer menu system
int multiplayer_menu_init(void)
{
  // Clear multiplayer menu context
  memset(&g_mp_menu, 0, sizeof(multiplayer_menu_context_t));

  // Set initial state
  g_mp_menu.current_state = MP_MENU_MAIN;
  g_mp_menu.previous_state = MP_MENU_MAIN;

  // Initialize default settings
  multiplayer_init_default_settings();

  // Set up network callbacks
  network_callbacks_t callbacks = {0};
  callbacks.on_player_joined = multiplayer_on_player_joined;
  callbacks.on_player_left = multiplayer_on_player_left;
  callbacks.on_game_data = multiplayer_on_game_data;
  callbacks.on_chat = multiplayer_on_chat;
  callbacks.on_error = multiplayer_on_network_error;
  network_set_callbacks(&callbacks);

  return 1;
}

// Shutdown multiplayer menu system
void multiplayer_menu_shutdown(void)
{
  // Disconnect from any active network session
  network_disconnect();

  // Clear context
  memset(&g_mp_menu, 0, sizeof(multiplayer_menu_context_t));
}

// Update multiplayer menu
void multiplayer_menu_update(void)
{
  // Update network
  network_update();

  // Update browser if in browser mode
  if (g_mp_menu.current_state == MP_MENU_GAME_BROWSER && g_mp_menu.browser_auto_refresh)
  {
	uint32_t current_time = al_get_time() * 1000;
	if (current_time - g_mp_menu.browser_last_refresh > MP_REFRESH_INTERVAL)
	{
	  multiplayer_browser_refresh();
	  g_mp_menu.browser_last_refresh = current_time;
	}
  }

  // Clear expired status messages
  uint32_t current_time = al_get_time() * 1000;
  if (g_mp_menu.status_timeout > 0 && current_time > g_mp_menu.status_timeout)
  {
	g_mp_menu.status_message[0] = '\0';
	g_mp_menu.status_timeout = 0;
  }

  if (g_mp_menu.error_timeout > 0 && current_time > g_mp_menu.error_timeout)
  {
	g_mp_menu.error_message[0] = '\0';
	g_mp_menu.error_timeout = 0;
  }
}

// Draw multiplayer menu
void multiplayer_menu_draw(void)
{
  multiplayer_draw_background();

  switch (g_mp_menu.current_state)
  {
  case MP_MENU_MAIN:
	multiplayer_draw_main_menu();
	break;
  case MP_MENU_HOST_SETUP:
	multiplayer_draw_host_menu();
	break;
  case MP_MENU_JOIN_SETUP:
	multiplayer_draw_join_menu();
	break;
  case MP_MENU_GAME_BROWSER:
	multiplayer_draw_browser_menu();
	break;
  case MP_MENU_LOBBY:
	multiplayer_draw_lobby_menu();
	break;
  case MP_MENU_CONNECTING:
	multiplayer_draw_join_menu(); // Show join menu while connecting
	break;
  case MP_MENU_ERROR:
	multiplayer_draw_main_menu(); // Show main menu with error
	break;
  case MP_MENU_SETTINGS:
	multiplayer_draw_main_menu(); // Settings not implemented yet
	break;
  default:
	multiplayer_draw_main_menu();
	break;
  }

  // Draw status messages
  multiplayer_draw_status();
}

// Handle multiplayer input
void multiplayer_menu_handle_input(void)
{
  switch (g_mp_menu.current_state)
  {
  case MP_MENU_MAIN:
	multiplayer_handle_main_input();
	break;
  case MP_MENU_HOST_SETUP:
	multiplayer_handle_host_input();
	break;
  case MP_MENU_JOIN_SETUP:
	multiplayer_handle_join_input();
	break;
  case MP_MENU_GAME_BROWSER:
	multiplayer_handle_browser_input();
	break;
  case MP_MENU_LOBBY:
	multiplayer_handle_lobby_input();
	break;
  case MP_MENU_CONNECTING:
	multiplayer_handle_join_input(); // Handle input while connecting
	break;
  case MP_MENU_ERROR:
  case MP_MENU_SETTINGS:
	multiplayer_handle_main_input(); // Fallback to main input
	break;
  }
}

// Initialize default settings
static void multiplayer_init_default_settings(void)
{
  strcpy(g_mp_menu.host_game_name, "Liberation Circuit Game");
  strcpy(g_mp_menu.host_player_name, "Player");
  strcpy(g_mp_menu.join_player_name, "Player");
  strcpy(g_mp_menu.join_hostname, "127.0.0.1");

  g_mp_menu.host_port = NETWORK_DEFAULT_PORT;
  g_mp_menu.join_port = NETWORK_DEFAULT_PORT;
  g_mp_menu.host_max_players = 4;
  g_mp_menu.browser_auto_refresh = 1;

  strcpy(g_mp_menu.default_player_name, "Player");
}

// Draw background
void multiplayer_draw_background(void)
{
  al_clear_to_color(MP_COLOR_BACKGROUND);
}

// Draw main multiplayer menu
static void multiplayer_draw_main_menu(void)
{
  int center_x = settings.option[OPTION_WINDOW_W] / 2;
  int start_y = 200;
  int line_height = 40;

  // Title
  multiplayer_draw_title("MULTIPLAYER");

  // Menu options
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, start_y, ALLEGRO_ALIGN_CENTRE, "HOST GAME");
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, start_y + line_height, ALLEGRO_ALIGN_CENTRE, "JOIN GAME");
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, start_y + line_height * 2, ALLEGRO_ALIGN_CENTRE, "BROWSE GAMES");
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, start_y + line_height * 4, ALLEGRO_ALIGN_CENTRE, "BACK TO MAIN MENU");

  // Network status
  multiplayer_draw_connection_status(center_x - 200, start_y + line_height * 6);
}

// Draw host game menu
static void multiplayer_draw_host_menu(void)
{
  int center_x = settings.option[OPTION_WINDOW_W] / 2;
  int start_y = 150;
  int line_height = 35;

  multiplayer_draw_title("HOST GAME");

  // Game settings
  al_draw_textf(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x - 150, start_y, ALLEGRO_ALIGN_LEFT, "Game Name: %s", g_mp_menu.host_game_name);
  al_draw_textf(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x - 150, start_y + line_height, ALLEGRO_ALIGN_LEFT, "Player Name: %s", g_mp_menu.host_player_name);
  al_draw_textf(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x - 150, start_y + line_height * 2, ALLEGRO_ALIGN_LEFT, "Port: %d", g_mp_menu.host_port);
  al_draw_textf(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x - 150, start_y + line_height * 3, ALLEGRO_ALIGN_LEFT, "Max Players: %d", g_mp_menu.host_max_players);

  // Buttons
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, start_y + line_height * 5, ALLEGRO_ALIGN_CENTRE, "START HOSTING");
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, start_y + line_height * 6, ALLEGRO_ALIGN_CENTRE, "BACK");
}

// Draw join game menu
static void multiplayer_draw_join_menu(void)
{
  int center_x = settings.option[OPTION_WINDOW_W] / 2;
  int start_y = 150;
  int line_height = 35;

  multiplayer_draw_title("JOIN GAME");

  // Connection settings
  al_draw_textf(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x - 150, start_y, ALLEGRO_ALIGN_LEFT, "Host IP: %s", g_mp_menu.join_hostname);
  al_draw_textf(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x - 150, start_y + line_height, ALLEGRO_ALIGN_LEFT, "Port: %d", g_mp_menu.join_port);
  al_draw_textf(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x - 150, start_y + line_height * 2, ALLEGRO_ALIGN_LEFT, "Player Name: %s", g_mp_menu.join_player_name);

  // Buttons
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, start_y + line_height * 4, ALLEGRO_ALIGN_CENTRE, "CONNECT");
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, start_y + line_height * 5, ALLEGRO_ALIGN_CENTRE, "BACK");
}

// Draw game browser menu
static void multiplayer_draw_browser_menu(void)
{
  int center_x = settings.option[OPTION_WINDOW_W] / 2;
  int start_y = 120;
  int line_height = 25;

  multiplayer_draw_title("BROWSE GAMES");

  // Game list header
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x - 200, start_y, ALLEGRO_ALIGN_LEFT, "Game Name");
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, start_y, ALLEGRO_ALIGN_LEFT, "Host");
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x + 100, start_y, ALLEGRO_ALIGN_LEFT, "Players");

  // Game list
  for (int i = 0; i < g_mp_menu.browser_game_count && i < 10; i++)
  {
	game_browser_entry_t *entry = &g_mp_menu.browser_games[i];
	int y = start_y + line_height * (i + 2);

	ALLEGRO_COLOR color = (i == g_mp_menu.browser_selected_game) ? MP_COLOR_WARNING : MP_COLOR_TEXT;

	al_draw_text(font[FONT_SQUARE].fnt, color, center_x - 200, y, ALLEGRO_ALIGN_LEFT, entry->game_info.game_name);
	al_draw_text(font[FONT_SQUARE].fnt, color, center_x, y, ALLEGRO_ALIGN_LEFT, entry->game_info.host_name);
	al_draw_textf(font[FONT_SQUARE].fnt, color, center_x + 100, y, ALLEGRO_ALIGN_LEFT, "%d/%d",
				  entry->game_info.current_players, entry->game_info.max_players);
  }

  // Buttons
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, start_y + line_height * 14, ALLEGRO_ALIGN_CENTRE, "REFRESH");
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, start_y + line_height * 15, ALLEGRO_ALIGN_CENTRE, "JOIN SELECTED");
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, start_y + line_height * 16, ALLEGRO_ALIGN_CENTRE, "BACK");
}

// Draw lobby menu
static void multiplayer_draw_lobby_menu(void)
{
  int center_x = settings.option[OPTION_WINDOW_W] / 2;
  int start_y = 120;
  int line_height = 25;

  multiplayer_draw_title("GAME LOBBY");

  // Player list
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x - 150, start_y, ALLEGRO_ALIGN_LEFT, "Players:");

  for (int i = 0; i < g_mp_menu.lobby_player_count; i++)
  {
	lobby_player_entry_t *entry = &g_mp_menu.lobby_players[i];
	int y = start_y + line_height * (i + 1);

	ALLEGRO_COLOR color = entry->ready ? MP_COLOR_READY : MP_COLOR_NOT_READY;
	if (entry->host)
	  color = MP_COLOR_HOST;

	al_draw_textf(font[FONT_SQUARE].fnt, color, center_x - 150, y, ALLEGRO_ALIGN_LEFT, "%s %s %s",
				  entry->player.name,
				  entry->host ? "(HOST)" : "",
				  entry->ready ? "[READY]" : "[NOT READY]");
  }

  // Buttons
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, start_y + line_height * 10, ALLEGRO_ALIGN_CENTRE, "TOGGLE READY");
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, start_y + line_height * 11, ALLEGRO_ALIGN_CENTRE, "LEAVE GAME");
}

// Draw title
void multiplayer_draw_title(const char *title)
{
  int center_x = settings.option[OPTION_WINDOW_W] / 2;
  al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_TEXT, center_x, 80, ALLEGRO_ALIGN_CENTRE, title);
}

// Draw status messages
void multiplayer_draw_status(void)
{
  int center_x = settings.option[OPTION_WINDOW_W] / 2;
  int status_y = settings.option[OPTION_WINDOW_H] - 80;

  if (strlen(g_mp_menu.status_message) > 0)
  {
	al_draw_text(font[FONT_SQUARE].fnt, g_mp_menu.status_color, center_x, status_y, ALLEGRO_ALIGN_CENTRE, g_mp_menu.status_message);
  }

  if (strlen(g_mp_menu.error_message) > 0)
  {
	al_draw_text(font[FONT_SQUARE].fnt, MP_COLOR_ERROR, center_x, status_y - 25, ALLEGRO_ALIGN_CENTRE, g_mp_menu.error_message);
  }
}

// Draw connection status
void multiplayer_draw_connection_status(int x, int y)
{
  const char *state_str = network_get_state_string(network_get_player_count() >= 0 ? NETWORK_STATE_CONNECTED : NETWORK_STATE_DISCONNECTED);
  al_draw_textf(font[FONT_SQUARE].fnt, MP_COLOR_TEXT_DIM, x, y, ALLEGRO_ALIGN_LEFT, "Network: %s", state_str);
}

// Set menu state
void multiplayer_menu_set_state(multiplayer_menu_state_t new_state)
{
  g_mp_menu.previous_state = g_mp_menu.current_state;
  g_mp_menu.current_state = new_state;
}

// Return to previous state
void multiplayer_menu_return_to_previous(void)
{
  multiplayer_menu_state_t temp = g_mp_menu.current_state;
  g_mp_menu.current_state = g_mp_menu.previous_state;
  g_mp_menu.previous_state = temp;
}

// Show status message
static void multiplayer_show_status(const char *message, ALLEGRO_COLOR color, uint32_t timeout)
{
  strncpy(g_mp_menu.status_message, message, sizeof(g_mp_menu.status_message) - 1);
  g_mp_menu.status_message[sizeof(g_mp_menu.status_message) - 1] = '\0';
  g_mp_menu.status_color = color;
  g_mp_menu.status_timeout = al_get_time() * 1000 + timeout;
}

// Show error message
void multiplayer_menu_show_error(const char *error_message, uint32_t timeout)
{
  strncpy(g_mp_menu.error_message, error_message, sizeof(g_mp_menu.error_message) - 1);
  g_mp_menu.error_message[sizeof(g_mp_menu.error_message) - 1] = '\0';
  g_mp_menu.error_timeout = al_get_time() * 1000 + timeout;
}

// Simplified input checking function
static int multiplayer_key_pressed(int allegro_key)
{
  ALLEGRO_KEYBOARD_STATE keystate;
  al_get_keyboard_state(&keystate);
  return al_key_down(&keystate, allegro_key);
}

// Basic input handlers (simplified for now)
static void multiplayer_handle_main_input(void)
{
  static int last_key_state[4] = {0, 0, 0, 0}; // Track previous key states

  int key1 = multiplayer_key_pressed(ALLEGRO_KEY_1);
  int key2 = multiplayer_key_pressed(ALLEGRO_KEY_2);
  int key3 = multiplayer_key_pressed(ALLEGRO_KEY_3);
  int keyesc = multiplayer_key_pressed(ALLEGRO_KEY_ESCAPE);

  // Check for key press (was up, now down)
  if (key1 && !last_key_state[0])
  {
	multiplayer_menu_set_state(MP_MENU_HOST_SETUP);
  }
  else if (key2 && !last_key_state[1])
  {
	multiplayer_menu_set_state(MP_MENU_JOIN_SETUP);
  }
  else if (key3 && !last_key_state[2])
  {
	multiplayer_menu_set_state(MP_MENU_GAME_BROWSER);
	multiplayer_browser_refresh();
  }
  else if (keyesc && !last_key_state[3])
  {
	// Return to main game menu (this should be handled by the calling menu system)
	game.phase = GAME_PHASE_MENU;
  }

  // Update key states
  last_key_state[0] = key1;
  last_key_state[1] = key2;
  last_key_state[2] = key3;
  last_key_state[3] = keyesc;
}

static void multiplayer_handle_host_input(void)
{
  static int last_enter = 0, last_escape = 0;

  int enter_key = multiplayer_key_pressed(ALLEGRO_KEY_ENTER);
  int escape_key = multiplayer_key_pressed(ALLEGRO_KEY_ESCAPE);

  if (enter_key && !last_enter)
  {
	multiplayer_host_start_game();
  }
  else if (escape_key && !last_escape)
  {
	multiplayer_menu_set_state(MP_MENU_MAIN);
  }

  last_enter = enter_key;
  last_escape = escape_key;
}

static void multiplayer_handle_join_input(void)
{
  static int last_enter = 0, last_escape = 0;

  int enter_key = multiplayer_key_pressed(ALLEGRO_KEY_ENTER);
  int escape_key = multiplayer_key_pressed(ALLEGRO_KEY_ESCAPE);

  if (enter_key && !last_enter)
  {
	multiplayer_join_connect();
  }
  else if (escape_key && !last_escape)
  {
	multiplayer_menu_set_state(MP_MENU_MAIN);
  }

  last_enter = enter_key;
  last_escape = escape_key;
}

static void multiplayer_handle_browser_input(void)
{
  static int last_up = 0, last_down = 0, last_enter = 0, last_r = 0, last_escape = 0;

  int up_key = multiplayer_key_pressed(ALLEGRO_KEY_UP);
  int down_key = multiplayer_key_pressed(ALLEGRO_KEY_DOWN);
  int enter_key = multiplayer_key_pressed(ALLEGRO_KEY_ENTER);
  int r_key = multiplayer_key_pressed(ALLEGRO_KEY_R);
  int escape_key = multiplayer_key_pressed(ALLEGRO_KEY_ESCAPE);

  if (up_key && !last_up && g_mp_menu.browser_selected_game > 0)
  {
	g_mp_menu.browser_selected_game--;
  }
  else if (down_key && !last_down && g_mp_menu.browser_selected_game < g_mp_menu.browser_game_count - 1)
  {
	g_mp_menu.browser_selected_game++;
  }
  else if (enter_key && !last_enter)
  {
	multiplayer_browser_join_selected();
  }
  else if (r_key && !last_r)
  {
	multiplayer_browser_refresh();
  }
  else if (escape_key && !last_escape)
  {
	multiplayer_menu_set_state(MP_MENU_MAIN);
  }

  last_up = up_key;
  last_down = down_key;
  last_enter = enter_key;
  last_r = r_key;
  last_escape = escape_key;
}

static void multiplayer_handle_lobby_input(void)
{
  static int last_space = 0, last_escape = 0;

  int space_key = multiplayer_key_pressed(ALLEGRO_KEY_SPACE);
  int escape_key = multiplayer_key_pressed(ALLEGRO_KEY_ESCAPE);

  if (space_key && !last_space)
  {
	multiplayer_lobby_toggle_ready();
  }
  else if (escape_key && !last_escape)
  {
	multiplayer_lobby_leave();
  }

  last_space = space_key;
  last_escape = escape_key;
}

// Host game functionality
void multiplayer_host_start_game(void)
{
  if (network_host_game(g_mp_menu.host_game_name, g_mp_menu.host_port))
  {
	multiplayer_show_status("Hosting game...", MP_COLOR_SUCCESS, 3000);
	multiplayer_menu_set_state(MP_MENU_LOBBY);

	// Add local player to lobby
	g_mp_menu.lobby_player_count = 1;
	strcpy(g_mp_menu.lobby_players[0].player.name, g_mp_menu.host_player_name);
	g_mp_menu.lobby_players[0].host = 1;
	g_mp_menu.lobby_players[0].ready = 1;
  }
  else
  {
	multiplayer_menu_show_error("Failed to host game", 5000);
  }
}

// Join game functionality
void multiplayer_join_connect(void)
{
  if (network_join_game(g_mp_menu.join_hostname, g_mp_menu.join_port, g_mp_menu.join_player_name))
  {
	multiplayer_show_status("Connecting...", MP_COLOR_WARNING, 3000);
	multiplayer_menu_set_state(MP_MENU_CONNECTING);
  }
  else
  {
	multiplayer_menu_show_error("Failed to connect", 5000);
  }
}

// Browser functionality
void multiplayer_browser_refresh(void)
{
  network_start_discovery();
  g_mp_menu.browser_game_count = network_get_discovered_games(NULL, 0);

  if (g_mp_menu.browser_game_count > 0)
  {
	game_info_t games[16];
	int count = network_get_discovered_games(games, 16);

	for (int i = 0; i < count && i < 16; i++)
	{
	  g_mp_menu.browser_games[i].game_info = games[i];
	  g_mp_menu.browser_games[i].ping = 0; // TODO: implement ping
	  g_mp_menu.browser_games[i].selected = 0;
	}
	g_mp_menu.browser_game_count = count;
  }

  multiplayer_show_status("Refreshed game list", MP_COLOR_SUCCESS, 2000);
}

void multiplayer_browser_join_selected(void)
{
  if (g_mp_menu.browser_selected_game < g_mp_menu.browser_game_count)
  {
	game_browser_entry_t *entry = &g_mp_menu.browser_games[g_mp_menu.browser_selected_game];

	// Set join parameters from selected game
	snprintf(g_mp_menu.join_hostname, sizeof(g_mp_menu.join_hostname), "%u.%u.%u.%u",
			 (entry->game_info.host_ip >> 0) & 0xFF,
			 (entry->game_info.host_ip >> 8) & 0xFF,
			 (entry->game_info.host_ip >> 16) & 0xFF,
			 (entry->game_info.host_ip >> 24) & 0xFF);
	g_mp_menu.join_port = entry->game_info.host_port;

	multiplayer_join_connect();
  }
}

// Lobby functionality
void multiplayer_lobby_toggle_ready(void)
{
  g_mp_menu.lobby_ready = !g_mp_menu.lobby_ready;
  // TODO: Send ready status to other players
}

void multiplayer_lobby_leave(void)
{
  network_disconnect();
  multiplayer_menu_set_state(MP_MENU_MAIN);
  multiplayer_show_status("Left game", MP_COLOR_WARNING, 2000);
}

// Network event callbacks
void multiplayer_on_player_joined(uint32_t player_id, const char *player_name)
{
  if (g_mp_menu.lobby_player_count < NETWORK_MAX_PLAYERS)
  {
	lobby_player_entry_t *entry = &g_mp_menu.lobby_players[g_mp_menu.lobby_player_count];
	entry->player.player_id = player_id;
	strncpy(entry->player.name, player_name, sizeof(entry->player.name) - 1);
	entry->player.name[sizeof(entry->player.name) - 1] = '\0';
	entry->ready = 0;
	entry->host = 0;
	g_mp_menu.lobby_player_count++;

	char message[128];
	snprintf(message, sizeof(message), "%s joined", player_name);
	multiplayer_show_status(message, MP_COLOR_SUCCESS, 3000);
  }
}

void multiplayer_on_player_left(uint32_t player_id)
{
  for (int i = 0; i < g_mp_menu.lobby_player_count; i++)
  {
	if (g_mp_menu.lobby_players[i].player.player_id == player_id)
	{
	  char message[128];
	  snprintf(message, sizeof(message), "%s left", g_mp_menu.lobby_players[i].player.name);

	  // Remove player from list
	  for (int j = i; j < g_mp_menu.lobby_player_count - 1; j++)
	  {
		g_mp_menu.lobby_players[j] = g_mp_menu.lobby_players[j + 1];
	  }
	  g_mp_menu.lobby_player_count--;

	  multiplayer_show_status(message, MP_COLOR_WARNING, 3000);
	  break;
	}
  }
}

void multiplayer_on_game_data(uint32_t player_id, const void *data, size_t size)
{
  // TODO: Handle game data
}

void multiplayer_on_chat(uint32_t player_id, const char *message)
{
  // TODO: Handle chat messages
}

void multiplayer_on_network_error(const char *error_message)
{
  multiplayer_menu_show_error(error_message, 5000);
}

// Public interface functions for menu integration

#endif // NETWORK_ENABLED