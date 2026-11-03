// SPDX-License-Identifier: GPL-3.0-or-later
// Raw declarations copied from d3-historical commit 84e25b0b411284bad57b34f50fd976f1a8a2367f.
// Copyright (C) 2024 Parallax Software. Ptr models 32-bit Windows pointers on a 64-bit test host.
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <string>
using uint=uint32_t; using ubyte=uint8_t; using sbyte=int8_t;using ushort=uint16_t;using Ptr=uint32_t;
struct vector {float x,y,z;};struct matrix {vector a,b,c;};
struct Inventory {Ptr root;int count;Ptr pos;};
#define MAX_PLAYER_WEAPONS 21
#define CALLSIGN_LEN 19
#define TRACKER_ID_LEN 10
#define MAX_WB_TURRETS 8
typedef struct {
	int	index;					//the index of the current primary or secondary weapon
	float firing_time; 			//how long the current weapon has been firing
	int	sound_handle;			//the handle for the sound the firing weapon is making
} player_weapon;

//The structure for a player.  Most of this data will be for multiplayer
typedef struct {

	// positional data for player starts
	int start_index;
	vector	start_pos;							//where the player starts
	int		start_roomnum;
	matrix	start_orient;

	int		startpos_flags;					// these flags apply to the start position (used for teams)

	int		ship_index;							// the index into the Ships array that this player is flying

	// Who am I data
	char		callsign[CALLSIGN_LEN+1];		// The callsign of this player, for net purposes.

	// Game data
	uint		flags;								// Powerup flags, see above...
	int		score;								// The player's current score
	float		damage_magnitude;					// for shield effects
	float		edrain_magnitude;					// for energy drain effects
	float		invul_magnitude;					// for invulnerability effects
	float		energy;								// Amount of energy remaining.
	ubyte		lives;								// Lives remaining, 0 = game over.
	sbyte		level;								// Current level player is playing. (must be signed for secret levels)
	sbyte		starting_level;					// What level the player started on.
	ubyte		keys;									// Which keys the player has
	short	 	killer_objnum;						// Who killed me.... (-1 if no one)
	float		invulnerable_time;				// Time left invulnerable
	float		last_hit_wall_sound_time;		// Last time we played a hit wall sound
	float		last_homing_warning_sound_time;  // Obvious  :)  
	float		last_thrust_time;						// Last time the player thrusted
	float		last_afterburner_time;			// Last time the player used the afterburner
	short		objnum;								// The object number of this player
	sbyte		team;									// The team number this guy is on

	//The current auto-waypoint for this player
	int		current_auto_waypoint_room;	//the most recent auto-waypoint, or -1

	// Statistics...
	//	int		score;								// Current score.
	float		time_level;							// Level time played
	float		time_total;							// Game time played (high word = seconds)

	int		num_hits_level;					// used for accuracy calculation.
	int		num_discharges_level;

	short		num_kills_level;					// Number of kills this level
	short		friendly_kills_level;			// Number of friendly kills this level
 	short		num_kills_total;					// Number of kills total

	// Player weapon info
	uint		weapon_flags;							// Mask of currently owned weapons
	ushort	weapon_ammo[MAX_PLAYER_WEAPONS];	// Ammo for each weapon

	//Weapons
	player_weapon weapon[2];					// Info on the player weapons
	ubyte		laser_level;						// Current level of the laser.
	
	// lighting
	float light_dist;
	float r,g,b;

	float ballspeed;
	ubyte num_balls;
	float ball_r[3],ball_g[3],ball_b[3];

	// Room tracking
	int oldroom;
	
	// Inventory
	Inventory inventory;

	// CounterMeasures Inventory
	Inventory counter_measures;

	// Last time the player fired a weapon
	float last_fire_weapon_time;

	// Afterburner stuff
	float afterburner_mag;			// How big the thrust is for the afterburner
	float thrust_mag;
	int afterburner_sound_handle;
	float afterburn_time_left;

	int thruster_sound_handle;
	int thruster_sound_state;

	// For small views and external cameras
	int small_left_obj,small_right_obj,small_dll_obj;


	// Multiplayer stuff
	ubyte multiplayer_flags;
	ubyte last_multiplayer_flags;
	float last_guided_time;

	char tracker_id[TRACKER_ID_LEN];
	int kills;
	int deaths;
	int suicides;
	float rank;
	float lateral_thrust;		//total lateral movement over the whole game
	float rotational_thrust;	//total rotational movement over the whole game
	unsigned int time_in_game; //seconds in game
	Ptr guided_obj,user_timeout_obj;

	float zoom_distance;


	// Scalar values
	float movement_scalar;
	float damage_scalar;
	float armor_scalar;
	float turn_scalar;
	float weapon_recharge_scalar;
	float weapon_speed_scalar;

	// Observer stuff
	int piggy_objnum;
	int piggy_sig;

	// Custom texture stuff
	int custom_texture_handle;

	// Ship permissions (1 bit per ship)
	int ship_permissions;

	// For invul hit effect
	vector invul_vector;

	//Used to disable various controller input from scripting
	unsigned int controller_bitflags;

	// Marker stuff
	short num_markers;

	short		num_deaths_level;					// Number of kills this level
	short		num_deaths_total;					// Number of kills total


} player;
typedef struct 
{
	int flags; // see above
	float light_distance;
	float red_light1,green_light1,blue_light1;
	float red_light2,green_light2,blue_light2;
	float time_interval;
	float	flicker_distance;
	float directional_dot;
	int timebits;
	ubyte angle;
	ubyte lighting_render_type;
} light_info;

typedef struct
{
	int type_flags;		// see EF_FLAGS above
	
	float alpha;			// alpha value
	float deform_range;	// how many units to deform when drawing
	float cloak_time;		// how much time left cloaked
	float deform_time;	// how much time left deforming
	float color_time;		// how much time left colored
	float r,g,b;		

	// For fading in/out
	float fade_time;
	float fade_max_time;

	float	damage_time;
	float damage_per_second;	// how much damage this object takes per second
	float last_damage_time;		//	last time this object took damage
	int	damage_handle;			// the object handle of the owner of the damage

	float volume_change_time;
	vector volume_old_pos;
	int volume_old_room;

	// For powerups only
	float last_object_hit_time;
	int last_object_hit;		

	// For specular lighting
	vector spec_pos;
	float spec_mag;
	float spec_r,spec_g,spec_b;
	
	// For dynamic volume lighting
	ubyte dynamic_this_frame;
	float dynamic_red,dynamic_green,dynamic_blue;

	// For liquid object
	float liquid_time_left;
	ubyte liquid_mag;
	
	// For freeze objects
	float freeze_scalar;

	// For attach objects
	int attach_line_handle;			// handle to the object that this object is tethered to

	//Sound for special effects
	int sound_handle;

	//For spark effect
	float	spark_delay;		//delay between sparks
	float	spark_timer;		//how long until next spark
	float spark_time_left;	//how long until sparking stops

} effect_info_s;

// Describes the next animation state for a robot
typedef struct custom_anim
{
	float  server_time;
	ushort server_anim_frame;

	unsigned short anim_start_frame;
	unsigned short anim_end_frame;
	float  anim_time;
	float  max_speed;

	short	anim_sound_index;
	ubyte flags;
	char next_anim_type;
} custom_anim;


typedef struct dynamic_wb_info
{
	// Dynamic Data
	float last_fire_time;
	unsigned char cur_firing_mask;
	
	float norm_turret_angle[MAX_WB_TURRETS];
	float turret_next_think_time[MAX_WB_TURRETS];
	ubyte turret_direction[MAX_WB_TURRETS];

	ubyte wb_anim_mask;
	float wb_anim_frame;

	vector cur_target;

	char upgrade_level;  // For multi-level weapons ( 0 to MAX_WB_UPGRADES-1)

	int flags;

} dynamic_wb_info;
static_assert(sizeof(player)==496,"historical player ABI");
static_assert(offsetof(player,flags)==84 && offsetof(player,energy)==104 && offsetof(player,inventory)==308,"player prefix");
static_assert(offsetof(player,afterburn_time_left)==348 && offsetof(player,guided_obj)==420 && offsetof(player,movement_scalar)==432,"player suffix");
static_assert(sizeof(effect_info_s)==164 && sizeof(dynamic_wb_info)==108 && sizeof(custom_anim)==24 && sizeof(light_info)==52,"object ABI");
static std::vector<uint8_t> out;
template<class T> void write(T value) {
  const auto* bytes=reinterpret_cast<const uint8_t*>(&value);
  out.insert(out.end(),bytes,bytes+sizeof(T));
}
void zeros(size_t n,uint8_t value=0) {out.insert(out.end(),n,value);}
void z(const char* text) {out.insert(out.end(),text,text+std::strlen(text)+1);}
void u8(uint8_t x){write(x);} void i16(int16_t x){write(x);} void i32(int32_t x){write(x);} void f32(float x){write(x);}
void object_record(int index,int version,bool effects) {
  i32(0xBADB0B);u8(index==0?4:2);u8(0);u8(0); // player or robot, lighting, light_info absent
  i32(0x1000+index);u8(0);i32(0);zeros(24); // handle, dummy, room, positions
  f32(1);zeros(12);f32(1);zeros(12);f32(1); // 3x3 identity
  u8(3);u8('A');u8('B');u8('C');i16(0);i32(0x01000000); // name, id, flags
  u8(index==0?4:1);u8(1);u8(1);i16(0);f32(5.5f);f32(index==0?87.25f:20.0f);
  u8(255);u8(0);u8(0);f32(0);f32(0);f32(0);i32(-1);i32(-1);i32(-1);
  i32(2);if(version>=2)i32(0);else {i32(-1);i32(-1);} // n_attach and children allocation
  u8(0);i16(0);f32(0);zeros(24);zeros(20); // attach data and impacts
  u8(4);u8('t');u8('e');u8('s');u8('t');u8(0);i16(0); // custom names, position counter
  i16(128);zeros(128,0x31);i16(64);zeros(64,0x32); // movement/control opaque unions
  u8(index==0?0:1);if(index!=0){i16(16);zeros(16,0x19);} // optional AI
  i16(160);zeros(160,0x33);f32(5.5f);custom_anim anim{};write(anim);
  u8(index==0?21:1);
  for(int i=0;i<(index==0?21:1);i++){dynamic_wb_info wb{};wb.last_fire_time=2.25f;wb.flags=0x40;write(wb);}
  u8(effects?1:0);
  if(effects){i16(sizeof(effect_info_s));effect_info_s fx{};fx.alpha=0.625f;fx.type_flags=256;fx.freeze_scalar=1;fx.sound_handle=-1;write(fx);}
}
int main(int argc,char**argv) {
  if(argc<2)return 2;
  int version=argc>2?std::atoi(argv[2]):2;
  bool effect=argc<=3 || std::atoi(argv[3])!=0;
  bool guided=argc>4 && std::atoi(argv[4])!=0;
  bool snapshot=argc>5 && std::atoi(argv[5])!=0;
  char description[32]="Synthetic parser fixture";for(auto c:description)u8(c);i16(version);u8(snapshot?1:0);
  if(snapshot){u8(0);u8(0);u8(122);z("test.ogf");u8(1);zeros(9);i16(2);i16(2);u8(32);u8(40);u8(4);i16(0x7fff);}
  i16(4);z("Afterburner");z("Converter");z("QuadLaser");z("Robot");
  i16(1);z("ship.pof");i16(0);i16(1);z("Pyro-GL");i16(0);i16(0); // remaining five xlate tables
  i16(-1);z("");i16(1);z("fixture.mn3");i32(0x55);f32(123.5f);i32(100);i32(-1);i16(0);
  i32(4);zeros(4);i32(1);i32(0);i32(-1); // weather and doors
  i16(1);i16(400);u8(1);zeros(399); // map
  u8(0);u8(0);i16(0); // unused rooms, zero triggers
  i32(0);i16(64);for(int i=0;i<64;i++){i16(1);u8(0);} // marker text
  i16(1);i32(1500);i32(0);for(int i=0;i<1500;i++)i32(-1);
  i32(0);i32(0);i32(400);i32(0);for(int i=0;i<400;i++)i32(-1);
  object_record(0,version,effect);object_record(1,version,true);
  player pl{};pl.start_index=0;pl.ship_index=0;std::strcpy(pl.callsign,"Testpilot");pl.flags=4096;pl.score=25;
  pl.energy=100.125f;pl.lives=3;pl.level=1;pl.starting_level=1;pl.killer_objnum=-1;pl.objnum=0;pl.team=-1;
  pl.weapon_flags=(1u<<31)|(1u<<20)|(1u<<10)|1u;pl.weapon_ammo[1]=120;pl.weapon_ammo[10]=4;
  pl.weapon[0].index=0;pl.weapon[1].index=10;pl.inventory.count=1;pl.inventory.root=1234;pl.inventory.pos=1234;
  pl.counter_measures.count=0;pl.afterburn_time_left=3.125f;pl.guided_obj=guided?1234:0;
  pl.movement_scalar=pl.damage_scalar=pl.armor_scalar=pl.turn_scalar=pl.weapon_recharge_scalar=pl.weapon_speed_scalar=1.0f;
  pl.controller_bitflags=0x1234;pl.ship_permissions=7;
  i16(sizeof(player));write(pl);if(guided)i32(0x1001);
  i32(1);i32(7);i32(7);i32(1);i32(1);i32(0x20);i32(1);z("Converter description");z("Converter icon");z("Converter");i32(131);i32(0);
  i32(0);i32(0); // countermeasures
  i32(0); // matcens, then an opaque tail that must survive byte for byte
  z("OPAQUE-OSIRIS-TAIL");for(int i=0;i<257;i++)u8((i*73)%256);
  std::ofstream file(argv[1],std::ios::binary);file.write(reinterpret_cast<const char*>(out.data()),out.size());
  std::printf("%s: player=%zu fx=%zu wb=%zu, %zu bytes\n",argv[1],sizeof(player),sizeof(effect_info_s),sizeof(dynamic_wb_info),out.size());
  return file?0:3;
}
