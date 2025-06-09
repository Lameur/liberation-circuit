
#ifndef H_S_MENU
#define H_S_MENU

void start_menus();

inline void init_w_init() // also called from s_mission.c
{

  w_init.players = 2;
  w_init.core_setting = 2;
  //	w_init.starting_data_setting = 0;
  w_init.game_seed = 0;
  //	w_init.data_wells = 0;

  w_init.size_setting = 2;
  w_init.command_mode = COMMAND_MODE_AUTO;
  fix_w_init_size();

  int i;

  for (i = 0; i < PLAYERS; i++)
  {
	sprintf(w_init.player_name[i], "Player %i", i);
	w_init.starting_data_setting[i] = 0;
	//		w_init.player_starting_data [i] = (w_init.starting_data_setting + 1) * 300; // may be changed by some missions
  }

  // this function doesn't initialise everything - it leaves some things (like player spawn positions) that must be initialised when the game is being started.
}

void fix_w_init_size();
void run_game_from_menu();

#endif
