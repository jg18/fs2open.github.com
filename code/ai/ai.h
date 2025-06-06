/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/



#ifndef _AI_H
#define _AI_H

#include "ai/aigoals.h"
#include "ai/ai_profiles.h"
#include "globalincs/globals.h"
#include "globalincs/pstypes.h"
#include "globalincs/systemvars.h"
#include "object/waypoint.h"
#include "parse/sexp.h"
#include "physics/physics.h"
#include "ship/ship_flags.h"

class ship_weapon;
class ship_subsys;
class object;
class ship_info;

#define	AI_DEFAULT_CLASS 3  // default AI class for new ships (Fred)

typedef struct ai_flag_name {
	AI::AI_Flags flag;
	char flag_name[TOKEN_LENGTH];
} ai_flag_name;

typedef struct ai_flag_description {
	AI::AI_Flags flag;
	const char *flag_desc;
} ai_flag_description;

extern ai_flag_name Ai_flag_names[];
extern ai_flag_description Ai_flag_descriptions[];
extern const int Num_ai_flag_names;

//	dock_orient_and_approach() modes.
#define	DOA_APPROACH	1		//	Approach the current point on the path (aip->path_cur)
#define	DOA_DOCK		2		//	Dock with goal object.
#define	DOA_UNDOCK_1	3		//	Begin undocking with goal object.  Just move away.
#define	DOA_UNDOCK_2	4		//	Secondary undocking.  Move away.
#define	DOA_UNDOCK_3	5		//	Tertiary undocking.  Move away and orient away.
#define	DOA_DOCK_STAY	6		//	Rigidly maintain position in dock bay.

//	Type values for ai_dock_with_object() dock_type parameter.
#define	AIDO_DOCK		1		//	Set goal of docking with object.
#define	AIDO_DOCK_NOW	2		//	Immediately move into dock position.  For ships that start mission docked.
#define	AIDO_UNDOCK		3		//	Set goal of undocking with object.

//	Flags to ai_turn_towards_vector().
#define	AITTV_FAST					(1<<0)	//	Turn fast, not slowed down based on skill level.
#define AITTV_VIA_SEXP				(1<<1)	//	Goober5000 - via sexp
#define AITTV_IGNORE_BANK			(1<<2)	//	Goober5000 - ignore bank when turning
#define AITTV_SLOW_BANK_ACCEL		(1<<3)  //  Asteroth - used by formation flying
#define AITTV_FORCE_DELTA_BANK		(1<<4)  //  Lafiel - Always use the provided delta bank and override calculated values

#define	KAMIKAZE_HULL_ON_DEATH	-1000.0f	//	Hull strength ship gets set to if it crash-dies.

// individual AI modes aka AI behaviors
#define	AIM_CHASE				0
#define	AIM_EVADE				1
#define	AIM_GET_BEHIND			2		//	This mode is not actually implemented.
#define	AIM_STAY_NEAR			3		//	Stay near another ship.
#define	AIM_STILL				4		//	Sit still.  Don't move.  Hold your breath.  Don't blink.
#define	AIM_GUARD				5		//	Guard an object
#define	AIM_AVOID				6		//	Avoid an object
#define	AIM_WAYPOINTS			7		//	Fly waypoints
#define	AIM_DOCK				8		//	Dock with ship.
#define	AIM_NONE				9		//	Uh, do nothing.
#define	AIM_BIGSHIP				10		//	Like a capital ship, doesn't focus on one ship.
#define	AIM_PATH				11		//	Follow path on ship
#define	AIM_BE_REARMED			12		//	Allow self to be rearmed
#define	AIM_SAFETY				13		//	Seek safety at periphery of battle
#define	AIM_EVADE_WEAPON		14		//	Evade a weapon.
#define	AIM_STRAFE				15		//  Attack a big ship by strafing it
#define	AIM_PLAY_DEAD			16		//	Play dead.  Get it?  Don't move, fire, etc.
#define	AIM_BAY_EMERGE			17		//  Emerging from a fighter bay, following path to do so
#define	AIM_BAY_DEPART			18		//  Departing to a fighter bay, following path to do so
#define	AIM_SENTRYGUN			19		//  AI mode for sentry guns only (floating turrets)
#define	AIM_WARP_OUT			20		//	Commence warp out sequence.  Point in legal direction.  Then call John's code.
#define AIM_FLY_TO_SHIP			21		//  Rendezvous with a ship.  Like stay-near, but non-perpetual.
#define AIM_LUA					22		//  Generic Lua-based AI mode

#define	MAX_AI_BEHAVIORS		23		//	Number of AIM_xxxx types

#define	MAX_WAYPOINTS_PER_LIST	20
#define	MAX_ENEMY_DISTANCE	2500.0f		//	Maximum distance from which a ship will pursue an enemy.

#define MAX_AI_GOALS	5
#define AI_ACTIVE_GOAL_NONE		-1
#define	AI_ACTIVE_GOAL_DYNAMIC	999

typedef struct ai_class {
	char	name[NAME_LENGTH];
	float	ai_accuracy[NUM_SKILL_LEVELS];
	float	ai_evasion[NUM_SKILL_LEVELS];
	float	ai_courage[NUM_SKILL_LEVELS];
	float	ai_patience[NUM_SKILL_LEVELS];

	//SUSHI: These were originally in AI_Profiles, adding the option to override in AI.tbl
	//INT_MIN and FLT_MIN represent the "not set" state for which defaults are used instead.
	float	ai_cmeasure_fire_chance[NUM_SKILL_LEVELS];	
	float	ai_in_range_time[NUM_SKILL_LEVELS];			
	float	ai_link_ammo_levels_maybe[NUM_SKILL_LEVELS];
	float	ai_link_ammo_levels_always[NUM_SKILL_LEVELS];
	float	ai_primary_ammo_burst_mult[NUM_SKILL_LEVELS];
	float	ai_link_energy_levels_maybe[NUM_SKILL_LEVELS];
	float	ai_link_energy_levels_always[NUM_SKILL_LEVELS];
	fix		ai_predict_position_delay[NUM_SKILL_LEVELS];
	float	ai_shield_manage_delay[NUM_SKILL_LEVELS];
	float	ai_ship_fire_delay_scale_friendly[NUM_SKILL_LEVELS];	
	float	ai_ship_fire_delay_scale_hostile[NUM_SKILL_LEVELS];
	float	ai_ship_fire_secondary_delay_scale_friendly[NUM_SKILL_LEVELS];
	float	ai_ship_fire_secondary_delay_scale_hostile[NUM_SKILL_LEVELS];
	float	ai_turn_time_scale[NUM_SKILL_LEVELS];
	float	ai_glide_attack_percent[NUM_SKILL_LEVELS];
	float	ai_circle_strafe_percent[NUM_SKILL_LEVELS];
	float	ai_glide_strafe_percent[NUM_SKILL_LEVELS];
	float	ai_random_sidethrust_percent[NUM_SKILL_LEVELS];
	float	ai_stalemate_time_thresh[NUM_SKILL_LEVELS];
	float	ai_stalemate_dist_thresh[NUM_SKILL_LEVELS];
	int		ai_chance_to_use_missiles_on_plr[NUM_SKILL_LEVELS];
	float	ai_max_aim_update_delay[NUM_SKILL_LEVELS];
	float	ai_turret_max_aim_update_delay[NUM_SKILL_LEVELS];
	flagset<AI::Profile_Flags>		ai_profile_flags;		//Holds the state of flags that are set
	flagset<AI::Profile_Flags>		ai_profile_flags_set;	//Holds which flags are set and which are just left alone

	//SUSHI: These are optional overrides to an AI class to prevent the automatic scaling based on AI class index
	int		ai_aburn_use_factor[NUM_SKILL_LEVELS];		
	float	ai_shockwave_evade_chance[NUM_SKILL_LEVELS];	
	float	ai_get_away_chance[NUM_SKILL_LEVELS];	
	float	ai_secondary_range_mult[NUM_SKILL_LEVELS];
	bool	ai_class_autoscale;		//Defaults to true, but can be turned off in order to disable extra scaling of some AI behaviors
									//based on AI class index
} ai_class;

//	Submode definitions.
//	Note: These need to be renamed to be of the form: AIS_mode_xxxx
// Cyborg17 -- If you add submodes for AI_CHASE, you *have to* add entries to Submode_text[] in aicode.cpp
#define	SM_CONTINUOUS_TURN	1	// takes parm: vector_id {0..3 = right, -right, up, -up}
#define	SM_ATTACK				2
#define	SM_EVADE_SQUIGGLE		3
#define	SM_EVADE_BRAKE			4
#define	SM_EVADE					5
#define	SM_SUPER_ATTACK		6
#define	SM_AVOID					7
#define	SM_GET_BEHIND			8
#define	SM_GET_AWAY				9
#define	SM_EVADE_WEAPON		10		//	Evade incoming weapon
#define	SM_FLY_AWAY				11		//	Fly away from target_objnum
#define	SM_ATTACK_FOREVER		12		//	Engine subsystem destroyed, so attack, never evading, avoiding, etc.
#define	SM_STEALTH_FIND		13		// Stealth ship is "targeted", but not visible, so try to find based on predicted pos
#define	SM_STEALTH_SWEEP		14		// General sweep, looking for stealth after not visible for some time.
#define	SM_BIG_APPROACH		15		// Big ship approaches another
#define	SM_BIG_CIRCLE			16		// Big ship flies circle around other big ship to get good angle to go parallel
#define	SM_BIG_PARALLEL		17		// Big ship flies parallel to another

//SUSHI: Attack submodes (besides those implicitly listed above) 
#define AIS_CHASE_GLIDEATTACK	18	// Ship uses glide to move in a constant direction while pointing and shooting at target
#define AIS_CHASE_CIRCLESTRAFE	19	// Attempt a circle-strafe on the target

//	Submodes for docking behavior
#define	AIS_DOCK_0		20
#define	AIS_DOCK_1		21
#define	AIS_DOCK_2		22
#define	AIS_DOCK_3		23
#define	AIS_DOCK_4		24			//	Only for rearm/repair.
#define	AIS_DOCK_4A		25			//	Only for not rearm/repair.  MK, 7/15/97
#define	AIS_UNDOCK_0	30
#define	AIS_UNDOCK_1	31
#define	AIS_UNDOCK_2	32
#define	AIS_UNDOCK_3	33
#define	AIS_UNDOCK_4	34

//	Submodes for seeking safety.
#define	AISS_1	41				//	Pick a spot to fly to.
#define	AISS_2	42				//	Flying to spot.
#define	AISS_3	43				//  Gotten near spot, fly about there.
#define	AISS_1a	44				//	Pick a new nearby spot because we are endangered, then go to AISS_2.  (Not implemented.)

//	Submodes for Guard behavior
#define	AIS_GUARD_PATROL		101
#define	AIS_GUARD_ATTACK		102
#define	AIS_GUARD_2				103
#define	AIS_GUARD_STATIC		104					//	maintain current relative position to guard object, if possible

// Submodes for strafing big ships behavior (AIM_STRAFE)
#define	AIS_STRAFE_ATTACK		201	// fly towards target and attack
#define	AIS_STRAFE_AVOID		202	// fly evasive vector to avoid incoming fire
#define	AIS_STRAFE_RETREAT1	203	// fly away from attack point
#define	AIS_STRAFE_RETREAT2	204
#define	AIS_STRAFE_POSITION	205	// re-position to resume strafing attack
#define	AIS_STRAFE_GLIDE_ATTACK	206	// SUSHI: Glide strafe atack

#define	WPF_REPEAT				(1 << 0)
#define	WPF_BACKTRACK			(1 << 1)

#define	PD_FORWARD				1
#define	PD_BACKWARD				-1

#define	MIN_TRACKABLE_ASPECT_DOT	0.992f		//	dot of fvec and vec_to_enemy to progress towards aspect lock

//	Submodes for warping out.
#define	AIS_WARP_1				300	//	Make sure there is no obstruction to warping out.
#define	AIS_WARP_2				301
#define	AIS_WARP_3				302
#define	AIS_WARP_4				303
#define	AIS_WARP_5				304
#define AIS_DEPART_TO_BAY		305

//	A node on a path.
//	Contains global location of point.
//	Contains hooks back to original path information.
//	This hook is used to extract information on the point such as whether it is
//	protected by turrets.
typedef struct pnode {
	vec3d	pos;
	int		path_num;			//	path number from polymodel, ie in polymodel, paths[path_num]
	int		path_index;			//	index in original model path of point, ie in model_path, use verts[path_index]
} pnode;

#define	MAX_PATH_POINTS	1000
extern pnode	Path_points[MAX_PATH_POINTS];
extern pnode	*Ppfp;			//	Free pointer in path points.

// Goober5000 (based on the "you can only remember 7 things in short-term memory" assumption)
#define MAX_IGNORE_NEW_OBJECTS	7

#define AI_DYNAMIC_PATH_RECHECK_DELAY 1*1000

typedef struct ai_info {
	flagset<AI::AI_Flags> ai_flags;				//	Special flags for AI behavior.
	int		shipnum;					// Ship using this slot, -1 means none.

	int		mode;
	int		previous_mode;
	int		mode_time;				//	timestamp at which current mode elapses.
	int		target_objnum;			//	object index of current target.
	int		target_signature;		//	Signature of current target.
	int		previous_target_objnum;	//	On 5/19/97, only used for player.

	int		stealth_last_cheat_visible_stamp;	// when within 100m, always update pos and velocity, with error increasing for increasing time from last legal visible
	int		stealth_last_visible_stamp;
	float		stealth_sweep_box_size;
	vec3d	stealth_last_pos;
	vec3d	stealth_velocity;

	float		previous_dot_to_enemy;	//	dot(fvec, vec_to_enemy) last frame
	float		target_time;			//	Amount of time continuously targeting this ship.

	int		enemy_wing;				//	When picking an enemy wing, only allow to be in enemy_wing, unless == -1, in which case don't care.
	int		attacker_objnum;

	int		goal_objnum;			//	mode specific goal.  In DOCK, ship to dock with.
	int		goal_signature;

	int		guard_objnum;			//	Ship to guard.
	int		guard_signature;		//	Signature of ship to guard.
	int		guard_wingnum;			//	Wing to guard.  guard_objnum set to leader.

	int		ignore_objnum;			//	ship to be ignored, based on player order.  UNUSED_OBJNUM if none.  -(wing_num+1) if ignoring wing.
	int		ignore_signature;		//	signature of ship to be ignored

	// Goober5000
	int		ignore_new_objnums[MAX_IGNORE_NEW_OBJECTS];
	int		ignore_new_signatures[MAX_IGNORE_NEW_OBJECTS];

	int		ai_class;				//	Class.  Might be override of default.

	//	Probably become obsolete, to be replaced by path_start, path_cur, etc.
	int		wp_list_index;			// index of waypoint list being followed
	int		wp_index;				// index of waypoint in list
	int		wp_flags;				//	waypoint flags, see WPF_xxxx
	int		waypoint_speed_cap;		// -1 no cap, otherwise cap - changed to int by Goober5000

	//	Path following information
	int		path_start;				//	Index into global array, start of path.
	int		path_cur;				//	Index into global array, current location in path.
	int		path_length;			//	Number of links in this path.
	int		path_dir;				//	PD_FORWARD, PD_BACKWARD
	int		path_flags;				//	loop, backtrack, whatever else.
	int		path_objnum;			//	Object of interest.  It's model contains the path.
	int		path_goal_obj_hash;	//	Hash value of goal object when global path created.
	fix		path_next_create_time;	//	Next time at which we'll create a global path.
	vec3d	path_create_pos;		//	Object's position at time of global path creation.
	matrix	path_create_orient;	//	Object's orientation at time of global path creation.
	int		mp_index;				//	Model path index.  Index in polymodel:model_paths
	fix		path_next_check_time;	//	Last time checked to see if would collide with model.
	int		path_goal_dist;		// minimum distance to first path point to consider path reached
	int		path_subsystem_next_check;	// timestamp to next check if subsystem is still visible
	vec3d	path_depart_rvec;		//Rotational orientation associated with the path

	int		submode;
	int		previous_submode;		// previous submode, get it?
	float		best_dot_to_enemy;	//	best dot product to enemy in last BEST_DOT_TIME seconds
	float		best_dot_from_enemy;	// best dot product for enemy to player in last BEST_DOT_TIME seconds
	fix		best_dot_to_time;		// time at which best dot occurred
	fix		best_dot_from_time;	// time at which best dot occurred
	fix		submode_start_time;	// time at which we entered the current submode
	int		submode_parm0;			//	parameter specific to current submode
	int		submode_parm1;			//	SUSHI: Another optional parameter
	float	submode_float0;			//	Goober5000: a float parameter
	fix		next_predict_pos_time;			//	Next time to predict position.

	int		next_dynamic_path_check_time;	//ETP: Time of next path check in ai_new_maybe_reposition_attack_subsys
	vec3d	last_dynamic_path_goal;			//ETP: Remebers last target location in ai_new_maybe_reposition_attack_subsys

	//SUSHI: like last_predicted_enemy_pos, but for aiming (which currently ignores predicted position)
	//Unlike the predicted position stuff, also takes into account velocity
	//Only used against small ships
	fix		next_aim_pos_time;
	vec3d	last_aim_enemy_pos;		// Normally this is the position of the enemy a turret is targeting but in the F3 Lab, this is used to pass raw target coordinates to the turret firing methods. 
	vec3d	last_aim_enemy_vel;

	ai_goal	goals[MAX_AI_GOALS];
	int		active_goal;			//	index of active goal, -1 if none, AI_ACTIVE_GOAL_DYNAMIC if dynamic (runtime-created) goal
	int		goal_check_time;		// timer used for processing goals for this ai object

	vec3d	last_predicted_enemy_pos;		//	Where he thought enemy was last time.
	float	time_enemy_in_range;				//	Amount of time enemy continuously in "sight", near crosshair.
	float	time_enemy_near;					//	SUSHI: amount of time enemy continuously "near" the player
	fix		last_attack_time;					//	Missiontime of last time this ship attacked its enemy.
	fix		last_hit_time;						//	Missiontime of last time this ship was hit by anyone.
	int		last_hit_quadrant;				//	Shield section of last hit.
	fix		last_hit_target_time;			//	Missiontime of last time this ship successfully hit target.
	int		hitter_objnum;						//	Object index of ship that hit this ship last time.
	int		hitter_signature;					//	Signature of hitter.  Prevents stupidity if hitter gets killed.
	fix		resume_goal_time;					//	Time at which to resume interrupted goal, if nothing else intervenes.
	float		prev_accel;							//	Acceleration last frame.
	float		prev_dot_to_goal;					//	dot of fvec to goal last frame, used to see if making progress towards goal.
	vec3d	goal_point;							//	Used in AIM_SAFETY, AIM_STILL and in circling.
	vec3d	prev_goal_point;					//	Previous location of goal point, used at least for evading.
	
	//Values copied from the AI class
	float	ai_accuracy, ai_evasion, ai_courage, ai_patience;
	int		ai_aburn_use_factor;		
	float	ai_shockwave_evade_chance;	
	float	ai_get_away_chance;	
	float	ai_secondary_range_mult;
	bool	ai_class_autoscale;

	//SUSHI: These were originally in AI_Profiles, adding the option to override in AI.tbl
	float	ai_cmeasure_fire_chance;
	float	ai_in_range_time;
	float	ai_link_ammo_levels_maybe;
	float	ai_link_ammo_levels_always;
	float	ai_primary_ammo_burst_mult;
	float	ai_link_energy_levels_maybe;
	float	ai_link_energy_levels_always;
	fix		ai_predict_position_delay;
	float	ai_shield_manage_delay;	
	float	ai_ship_fire_delay_scale_friendly;
	float	ai_ship_fire_delay_scale_hostile;
	float	ai_ship_fire_secondary_delay_scale_friendly;
	float	ai_ship_fire_secondary_delay_scale_hostile;
	float	ai_turn_time_scale;
	float	ai_glide_attack_percent;
	float	ai_circle_strafe_percent;
	float	ai_glide_strafe_percent;
	float	ai_random_sidethrust_percent;
	float	ai_stalemate_time_thresh;
	float	ai_stalemate_dist_thresh;
	int		ai_chance_to_use_missiles_on_plr;
	float	ai_max_aim_update_delay;
	float	ai_turret_max_aim_update_delay;
	flagset<AI::Profile_Flags> ai_profile_flags;	//Holds AI_Profiles flags (possibly overriden by AI class) that actually apply to AI

	ship_subsys*	targeted_subsys;			// Targeted subobject on current target.  NULL if none;
	ship_subsys*	last_subsys_target;		// last known subsystem target
	int				targeted_subsys_parent;	//	Parent objnum of subobject, not necessarily targeted

	float		aspect_locked_time;				//	Time towards acquiring lock for current_target

	// Goober5000
	int		support_ship_objnum;			// objnum of support ship docking with us, or (if we're a support ship) object we're docking to
	int		support_ship_signature;			// signature of the same

	int		danger_weapon_objnum;			//	Closest objnum of weapon fired at this ship.
	int		danger_weapon_signature;		//	Signature of object danger_weapon_objnum.

	vec3d	guard_vec;							//	vector to object being guarded, only used in AIS_GUARD_STATIC submode
	int		nearest_locked_object;			//	Nearest locked object.
	float		nearest_locked_distance;		//	Distance to nearest locked object.

	float		current_target_distance;		// Distance of current target from player
	int		current_target_is_locked;		// Flag to indicate whether the current target is locked for missile fire
	int		current_target_dist_trend;		// Tracks whether distance to target is increasing or decreasing
	int		current_target_speed_trend;	// Tracks whether speed of target is increasing or decreasing

	float		last_dist;							// last frame's distance between player and target
	float		last_speed;							// last frame's target speed
	int		last_secondary_index;			// needed for secondary weapon change check
	int		last_target;

	int		rearm_first_missile;				// flag to show that reloading of missilies hasn't begun yet
	int		rearm_first_ballistic_primary;		// flag to show that reloading of ballistic primaries hasn't begun yet
	int		rearm_release_delay;				// timestamp used to delay separation of ships after rearm complete

	fix		afterburner_stop_time;			//	Missiontime to turn off afterburner
	int		last_objsig_hit;					// The object number signature of the ship last hit by this ship
	int		ignore_expire_timestamp;		//	Timestamp at which temporary ignore (AIF_TEMPORARY_IGNORE) expires.
	int		warp_out_timestamp;				//	Timestamp at which this ship is to warp out.
	int		next_rearm_request_timestamp;	//	Timestamp at which ship might next request rearm.
	int		primary_select_timestamp;		//	When to next select a primary weapon.
	int		secondary_select_timestamp;	//	When to next select a secondary weapon.

	int		scan_for_enemy_timestamp;		// When to next look for enemy fighters if sitting still while pounding
														// on a bigship.   SCAN_FIGHTERS_INTERVAL is defined in AiBig.h
	int		choose_enemy_timestamp;			//	Time at which it is next legal to choose a new enemy (does not apply 
														// to special situations, like getting hit by a weapon)
	int		force_warp_time;					//	time at which to give up avoiding a ship and just warp out

	int		shockwave_object;					//	Object index of missile that will generate a shockwave.  We will try to avoid.

	int		shield_manage_timestamp;		//	Time at which to next manage shield.
	int		self_destruct_timestamp;		//	Time at which to self-destruct, probably due to being disabled.
	int		ok_to_target_timestamp;			//	Time at which this ship can dynamically target.

	int		kamikaze_damage;					// some damage value used to produce a shockwave from a kamikaze ship
	vec3d	big_attack_point;					//	Global point this ship is attacking on a big ship.
	vec3d	big_attack_surface_normal;		// Surface normal at ship at big_attack_point;
	int		pick_big_attack_point_timestamp; //	timestamp at which to pick a new point to attack on a big ship.

	//	Note: These three avoid_XX terms are shared between the code that avoids small (only player now) and large ships
	//	The bits in ai_flags determine which is occurring.  AIF_AVOID_SMALL_SHIP, AIF_AVOID_BIG_SHIP
	int		avoid_ship_num;					//	object index of small ship to avoid
	vec3d	avoid_goal_point;					//	point to aim at when avoiding a ship
	fix		avoid_check_timestamp;			//	timestamp at which to next check for having to avoid ship

	vec3d	big_collision_normal;			// Global normal of collision with big ship.  Helps find direction to fly away from big ship.  Set for each collision.
	vec3d	big_recover_1_direction;		//	A relative direction to fly towards when recovering from collision with a big ship, stage 1.
	vec3d	big_recover_2_pos;				//	Global point to fly towards when recovering from collision with a big ship, stage 2.
	int		big_recover_timestamp;			//	timestamp at which it's OK to re-enter stage 1.

	int		abort_rearm_timestamp;			//	time at which this rearm should be aborted in a multiplayer game.

	// artillery targeting info
	int		artillery_objnum;					// object currently being targeted for artillery lock/attack
	int		artillery_sig;						// artillery object signature
	float		artillery_lock_time;				// how long we've been locked onto this guy
	vec3d	artillery_lock_pos;				// base position of the lock point on (in model's frame of reference)
	float		lethality;							// measure of how dangerous ship is to enemy BIG|HUGE ships (likelyhood of targeting)

	flagset<AI::Maneuver_Override_Flags>	ai_override_flags;			// flags for marking ai overrides from sexp or lua systems
	control_info	ai_override_ci;		// ai override control info
	int		ai_override_lat_timestamp;		// mark for when to end the current lateral maneuver override
	int		ai_override_rot_timestamp;		// mark for when to end the current rotational maneuver override

	int form_obj_slotnum;               // for flying in formation object mode, the position in the formation

	int multilock_check_timestamp;		// when to check for multilock next
	SCP_vector<std::pair<int, ship_subsys*>> ai_missile_locks_firing;  // a list of missile locks (locked objnum, locked subsys) the ai is currently firing
	
	ai_lua_parameters lua_ai_target;
} ai_info;

int ai_maybe_autoscale(int absolute_index = -1);
int ai_get_autoscale_index(int absolute_index = -1);

// Goober5000
typedef struct {
	vec3d docker_point;
	vec3d dockee_point;
	int dock_mode;
	int submodel;
	vec3d submodel_pos;
	float submodel_r;
	float submodel_w;
} rotating_dockpoint_info;

#define	MAX_AI_INFO	 MAX_SHIPS

// SUBSYS_PATH_DIST is used as the distance that a subsystem path should terminate from the actual
// subsystem.  We don't want to rely on the model path points for this, since we may need to be 
// changed.
#define SUBSYS_PATH_DIST	500.0f

// Friendly damage defines
#define MAX_BURST_DAMAGE	20.0f		// max damage that can be done in BURST_DURATION
#define BURST_DURATION		500	// decay time over which Player->damage_this_burst falls from MAX_BURST_DAMAGE to 0

extern int Mission_all_attack;	//	!0 means all teams attack all teams.

extern ai_info Ai_info[];
extern ai_info *Player_ai;

extern ai_class *Ai_classes;
extern char** Ai_class_names;

extern int Num_ai_classes;
extern int Ai_firing_enabled;

extern const char *Skill_level_names(int skill_level, int translate = 1);

extern control_info AI_ci;

// need access to following data in AiBig.cpp
extern object	*Pl_objp;
extern object	*En_objp;
extern float	AI_frametime;


// Return index of free AI slot.
// Return 0 if no free slot.
int ai_get_slot(int shipnum);

// Releases an AI slot to be used by someone else.
void ai_free_slot(int ai_index);

// call to init one ai object.. you can pass all sorts of
// stuff by adding new paramters.
void ai_object_init(object * obj, int ai_index);

// Called once a frame
void ai_process( object * obj, int ai_index, float frametime );

const char *ai_get_goal_target_name(const char *name, int *index);
void ai_clear_goal_target_names();

extern void init_ai_system(void);
extern void ai_attack_object(object *attacker, object *attacked, int ship_info_index = -1);
extern void ai_evade_object(object *evader, object *evaded);
extern void ai_ignore_object(object *ignorer, object *ignored, int ignore_new);
extern void ai_ignore_wing(object *ignorer, int wingnum);
extern void ai_dock_with_object(object *docker, int docker_index, object *dockee, int dockee_index, int dock_type);
extern void ai_stay_still(object *still_objp, vec3d *view_pos);
extern void ai_do_default_behavior(object *obj);
extern void ai_start_waypoints(object *objp, int wl_index, int wp_flags, int start_index, bool force = false);
extern void ai_ship_hit(object *objp_ship, object *hit_objp, const vec3d *hit_normal);
extern void ai_ship_destroy(int shipnum);
extern vec3d ai_get_acc_limit(vec3d* vel_limit, const object* objp);
extern void ai_turn_towards_vector(const vec3d *dest, object *objp, const vec3d *slide_vec, const vec3d *rel_pos, float bank_override, int flags, const vec3d *rvec = nullptr, const vec3d* turnrate_mod = nullptr);
extern void init_ai_object(int objnum);
extern void ai_init(void);				//	Call this one to parse ai.tbl.
extern void ai_level_init(void);		//	Call before each level to reset AI

extern int ai_set_attack_subsystem(object *objp, int subnum);
extern int ai_issue_rearm_request(object *requester_objp);		//	Object requests rearm/repair.
extern int ai_abort_rearm_request(object *requester_objp);		//	Object aborts rearm/repair.
extern void ai_do_repair_frame(object *objp, ai_info *aip, float frametime);		//	Repair a ship object, player or AI.
extern void ai_update_danger_weapon(int objnum, int weapon_objnum);

// called externally from MissionParse.cpp to position ships in wings upon arrival into the
// mission.
extern void get_absolute_wing_pos(vec3d *result_pos, object *leader_objp, int wingnum, int wing_index, bool formation_object_flag, bool autopilot = false);

//	Interface from goals code to AI.  Set ship to guard.  *objp guards *other_objp
extern void ai_set_guard_object(object *objp, object *other_objp);
extern void ai_set_guard_wing(object *objp, int wingnum);
extern void ai_warp_out(object *objp, vec3d *vp);
extern void ai_attack_wing(object *attacker, int wingnum);
extern void ai_deathroll_start(object *ship_obj);
extern int set_target_objnum(ai_info *aip, int objnum);
extern void ai_form_on_wing(object *objp, object *goal_objp);
extern void ai_do_stay_near(object *objp, object *other_obj, float dist, int additional_data);
extern ship_subsys *set_targeted_subsys(ai_info *aip, ship_subsys *new_subsys, int parent_objnum);
extern void ai_rearm_repair( object *objp, int docker_index, object *goal_objp, int dockee_index );
extern void ai_add_rearm_goal( object *requester_objp, object *support_objp );
extern void create_model_path(object *pl_objp, object *mobjp, int path_num, int subsys_path=0);
extern int ai_find_goal_index( ai_goal* aigp, int mode, int submode = -1, int priority = -1 );

// Goober5000
extern void ai_do_safety(object *objp);

// used to get path info for fighter bay emerging and departing
int ai_acquire_emerge_path(object *pl_objp, int parent_objnum, int path_mask);
int ai_acquire_depart_path(object *pl_objp, int parent_objnum, int path_mask);

// used by AiBig.cpp
extern void ai_set_positions(object *pl_objp, object *en_objp, ai_info *aip, vec3d *player_pos, vec3d *enemy_pos);
extern void accelerate_ship(ai_info *aip, float accel);
extern void turn_away_from_point(object *objp, vec3d *point, float bank_override);
extern float ai_endangered_by_weapon(ai_info *aip);
extern void update_aspect_lock_information(ai_info *aip, vec3d *vec_to_enemy, float dist_to_enemy, float enemy_radius);
extern void ai_chase_ct();
extern void ai_find_path(object *pl_objp, int objnum, int path_num, int exit_flag, int subsys_path=0);
extern float ai_path();
extern void evade_weapon();
extern int might_collide_with_ship(object *obj1, object *obj2, float dot_to_enemy, float dist_to_enemy, float duration);
extern int ai_fire_primary_weapon(object *objp);	//changed to return weather it fired-Bobboau
extern int ai_fire_secondary_weapon(object *objp);
extern float ai_get_weapon_dist(ship_weapon *swp);
extern void turn_towards_point(object *objp, vec3d *point, vec3d *slide_vec, float bank_override, matrix* target_orient = nullptr, int flags = 0);
extern bool ai_willing_to_afterburn_hard(ai_info* aip);
extern void ai_afterburn_hard(object* objp, ai_info* aip);
extern int ai_maybe_fire_afterburner(object *objp, ai_info *aip);
extern void set_predicted_enemy_pos(vec3d *predicted_enemy_pos, object *pobjp, vec3d *enemy_pos, vec3d *enemy_vel, ai_info *aip);

extern int is_instructor(object *objp);
extern int find_enemy(int objnum, float range, int max_attackers, int ship_info_index = -1);

float ai_get_weapon_speed(const ship_weapon *swp);
void set_predicted_enemy_pos_turret(vec3d *predicted_enemy_pos, const vec3d *gun_pos, const object *pobjp, const vec3d *enemy_pos, const vec3d *enemy_vel, float weapon_speed, float time_enemy_in_range);

// function to change rearm status for ai ships (called from sexpression code)
extern void ai_set_good_rearm_time( int team, int time );
extern void ai_set_bad_rearm_time(int team, int time);
extern void ai_good_secondary_time( int team, int weapon_index, int num_weapons, const char *shipname );

extern void ai_do_objects_docked_stuff(object *docker, int docker_point, object *dockee, int dockee_point, bool update_clients = true);
extern void ai_do_objects_undocked_stuff( object *docker, object *dockee );
extern void ai_do_objects_repairing_stuff( object *repaired_obj, object *repair_obj, int how );

// Goober5000
//	Move to a position relative to a dock bay using thrusters.
extern float dock_orient_and_approach(object *docker_objp, int docker_index, object *dockee_objp, int dockee_index, int dock_mode, rotating_dockpoint_info *rdinfo = NULL);

void ai_set_mode_warp_out(object *objp, ai_info *aip);

// prototyped by Goober5000
int get_nearest_objnum(int objnum, int enemy_team_mask, int enemy_wing, float range, int max_attackers, int ship_info_index);

// moved to header file by Goober5000
void ai_announce_ship_dying(object *dying_objp);

//Moved declaration here for player ship -WMC
void ai_process_subobjects(int objnum);

//SUSHI: Setting ai_info stuff from both ai class and ai profile
void init_aip_from_class_and_profile(ai_info *aip, ai_class *aicp, ai_profile_t *profile);

void ai_update_aim(ai_info *aip);

//SUSHI: Random evasive sidethrust
void do_random_sidethrust(ai_info *aip, ship_info *sip);

void ai_formation_object_recalculate_slotnums(int form_objnum, int exiting_objnum = -1);


bool test_line_of_sight(const vec3d* from, const vec3d* to, SCP_unordered_set<int>&& excluded_object_ids = {}, float threshold = 10.0f, bool test_for_shields = false, bool test_for_hull = true, float* first_intersect_dist = nullptr, object** first_intersect_obj = nullptr);

#endif
