/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/ 



#include "asteroid/asteroid.h"
#include "cmdline/cmdline.h"
#include "debris/debris.h"
#include "freespace.h"
#include "gamesnd/gamesnd.h"
#include "graphics/matrix.h"
#include "hud/hudbrackets.h"
#include "hud/hudtargetbox.h"
#include "iff_defs/iff_defs.h"
#include "io/timer.h"
#include "jumpnode/jumpnode.h"
#include "localization/localize.h"
#include "mission/missionparse.h"
#include "model/model.h"
#include "network/multi.h"
#include "object/object.h"
#include "object/objectdock.h"
#include "parse/parselo.h"
#include "playerman/player.h"
#include "ship/ship.h"
#include "ship/subsysdamage.h"
#include "species_defs/species_defs.h"
#include "weapon/emp.h"
#include "weapon/weapon.h"

#ifndef NDEBUG
#include "hud/hudets.h"
#endif


extern fov_t View_zoom;

int Target_window_coords[GR_NUM_RESOLUTIONS][4] =
{
	{ // GR_640
		8, 362, 131, 112
	},
	{ // GR_1024
		8, 629, 131, 112
	}
};

object *Enemy_attacker = NULL;

static int Target_static_next;
static int Target_static_playing;
sound_handle Target_static_looping = sound_handle::invalid();

bool Target_display_cargo;
char Cargo_string[256] = "";

#ifndef NDEBUG
extern int Show_target_debug_info;
extern int Show_target_weapons;
#endif

// used to print out + or - after target distance and speed
const  char* modifiers[] = {
//XSTR:OFF
"+",
"-",
""
//XSTR:ON
};

#define NUM_TBOX_COORDS			11	// keep up to date
#define TBOX_BACKGROUND			0
#define TBOX_NAME					1
#define TBOX_CLASS				2
#define TBOX_DIST					3
#define TBOX_SPEED				4
#define TBOX_CARGO				5
#define TBOX_HULL					6
#define TBOX_EXTRA				7
#define TBOX_EXTRA_ORDERS		8
#define TBOX_EXTRA_TIME			9
#define TBOX_EXTRA_DOCK			10

// cargo scanning extents
int Cargo_scan_coords[GR_NUM_RESOLUTIONS][4] = {
	{ // GR_640
		7, 364, 130, 109
	},
	{ // GR_1024
		7, 635, 130, 109
	}
};

// first element is time flashing expires
int Targetbox_flash_timers[NUM_TBOX_FLASH_TIMERS];

int CurrentWire = 0;
int Targetbox_wire = 0;
int Targetbox_shader_effect = -1;
color Targetbox_color;
bool Targetbox_color_override = false;
bool Lock_targetbox_mode = false;

// Different target states.  This drives the text display right below the hull integrity on the targetbox.
#define TS_DIS		0
#define TS_OK		1
#define TS_DMG		2
#define TS_CRT		3

static int Current_ts; // holds current target status
static int Last_ts;	// holds last target status.

/**
 * @note Cut down long subsystem names to a more manageable length
 */
void hud_targetbox_truncate_subsys_name(char *outstr)
{	
	if (Lcl_gr && !Disable_built_in_translations) {
		if ( strstr(outstr, "communication") )	{
			strcpy(outstr, "Komm");
		} else if ( !stricmp(outstr, "weapons") ) {
			strcpy(outstr, "Waffen");
		} else if ( strstr(outstr, "engine") || strstr(outstr, "Engine")) {
			strcpy(outstr, "Antrieb");
		} else if ( !stricmp(outstr, "sensors") ) {
			strcpy(outstr, "Sensoren");
		} else if ( strstr(outstr, "navigat") ) {
			strcpy(outstr, "Nav");
		} else if ( strstr(outstr, "fighterbay") || strstr(outstr, "Fighterbay") ) {
			strcpy(outstr, "J\x84gerhangar");
		} else if ( strstr(outstr, "missile") ) {
			strcpy(outstr, "Raketenwerfer");
		} else if ( strstr(outstr, "laser") || strstr(outstr, "turret") ) {
			strcpy(outstr, "Gesch\x81tzturm");
		} else if ( strstr(outstr, "Command Tower") || strstr(outstr, "Bridge") ) {
			strcpy(outstr, "Br\x81""cke");
		} else if ( strstr(outstr, "Barracks") ) {
			strcpy(outstr, "Quartiere");
		} else if ( strstr(outstr, "Reactor") ) {
			strcpy(outstr, "Reaktor");
		} else if ( strstr(outstr, "RadarDish") ) {
			strcpy(outstr, "Radarantenne");
		} else if (!stricmp(outstr, "Gas Collector")) {
			strcpy(outstr, "Sammler");
		} 
	} else if (Lcl_fr && !Disable_built_in_translations) {	
		if ( strstr(outstr, "communication") )	{
			strcpy(outstr, "comm");
		} else if ( !stricmp(outstr, "weapons") ) {
			strcpy(outstr, "armes");
		} else if ( strstr(outstr, "engine") ) {
			strcpy(outstr, "moteur");
		} else if ( !stricmp(outstr, "sensors") ) {
			strcpy(outstr, "detecteurs");
		} else if ( strstr(outstr, "navi") ) {
			strcpy(outstr, "nav");
		} else if ( strstr(outstr, "missile") ) {
			strcpy(outstr, "lanceur de missiles");
		} else if ( strstr(outstr, "fighter") ) {
			strcpy(outstr, "baie de chasse");
		} else if ( strstr(outstr, "laser") || strstr(outstr, "turret") || strstr(outstr, "missile") ) {
			strcpy(outstr, "tourelle");
		} 
	} else if (Lcl_pl && !Disable_built_in_translations) {	
		if ( strstr(outstr, "communication") )	{
			strcpy(outstr, "komunikacja");
		} else if ( !stricmp(outstr, "weapons") ) {
			strcpy(outstr, "uzbrojenie");
		} else if ( strstr(outstr, "engine") || strstr(outstr, "Engine")) {
			strcpy(outstr, "silnik");
		} else if ( !stricmp(outstr, "sensors") ) {
			strcpy(outstr, "sensory");
		} else if ( strstr(outstr, "navigat") ) {
			strcpy(outstr, "nawigacja");
		} else if ( strstr(outstr, "fighterbay") || strstr(outstr, "Fighterbay") ) {
			strcpy(outstr, "dok my\x9Cliw.");
		} else if ( strstr(outstr, "missile") ) {
			strcpy(outstr, "wie\xBF. rakiet.");
		} else if ( strstr(outstr, "laser") || strstr(outstr, "turret") ) {
			strcpy(outstr, "wie\xBFyczka");
		} else if ( strstr(outstr, "Command Tower") || strstr(outstr, "Bridge") ) {
			strcpy(outstr, "mostek");
		} else if ( strstr(outstr, "Barracks") ) {
			strcpy(outstr, "koszary");
		} else if ( strstr(outstr, "Reactor") ) {
			strcpy(outstr, "reaktor");
		} else if ( strstr(outstr, "RadarDish") || strstr(outstr, "Radar Dish") ) {
			strcpy(outstr, "antena radaru");
		} else if (!stricmp(outstr, "Gas Collector")) {
			strcpy(outstr, "zbieracz gazu");
		} 
	} else {
		if (strstr(outstr, XSTR("communication", 333)))	{
			strcpy(outstr, XSTR("comm", 334));
		} else if (strstr(outstr, XSTR("navigation", 335)))	{
			strcpy(outstr, XSTR("nav", 336));
		} else if (strstr(outstr, "gas collector")) {
			strcpy(outstr, "collector");
		}
	}
}

HudGaugeTargetBox::HudGaugeTargetBox():
	HudGauge(HUD_OBJECT_TARGET_MONITOR, HUD_TARGET_MONITOR, false, false, (VM_EXTERNAL | VM_DEAD_VIEW | VM_WARP_CHASE | VM_PADLOCK_ANY), 255, 255, 255), 
	Monitor_mask(-1),
	Use_subsys_name_offsets(false), 
	Use_subsys_integrity_offsets(false),
	Use_disabled_status_offsets(false)
{
}

void HudGaugeTargetBox::initViewportOffsets(int x, int y)
{
	Viewport_offsets[0] = x;
	Viewport_offsets[1] = y;
}

void HudGaugeTargetBox::initViewportSize(int w, int h)
{
	Viewport_w = w;
	Viewport_h = h;
}

void HudGaugeTargetBox::initIntegrityOffsets(int x, int y)
{
	Integrity_bar_offsets[0] = x;
	Integrity_bar_offsets[1] = y;
}

void HudGaugeTargetBox::initIntegrityHeight(int h)
{
	integrity_bar_h = h;
}

void HudGaugeTargetBox::initStatusOffsets(int x, int y)
{
	Status_offsets[0] = x;
	Status_offsets[1] = y;
}

void HudGaugeTargetBox::initNameOffsets(int x, int y)
{
	Name_offsets[0] = x;
	Name_offsets[1] = y;
}

void HudGaugeTargetBox::initClassOffsets(int x, int y)
{
	Class_offsets[0] = x;
	Class_offsets[1] = y;
}

void HudGaugeTargetBox::initDistOffsets(int x, int y)
{
	Dist_offsets[0] = x;
	Dist_offsets[1] = y;
}

void HudGaugeTargetBox::initSpeedOffsets(int x, int y)
{
	Speed_offsets[0] = x;
	Speed_offsets[1] = y;
}

void HudGaugeTargetBox::initCargoStringOffsets(int x, int y)
{
	Cargo_string_offsets[0] = x;
	Cargo_string_offsets[1] = y;
}

void HudGaugeTargetBox::initHullOffsets(int x, int y)
{
	Hull_offsets[0] = x;
	Hull_offsets[1] = y;
}

void HudGaugeTargetBox::initCargoScanType(CargoScanType scantype)
{
	Cargo_scan_type = scantype;
}

void HudGaugeTargetBox::initCargoScanStartOffsets(int x, int y)
{
	Cargo_scan_start_offsets[0] = x;
	Cargo_scan_start_offsets[1] = y;
}

void HudGaugeTargetBox::initCargoScanSize(int w, int h)
{
	Cargo_scan_w = w;
	Cargo_scan_h = h;
}

void HudGaugeTargetBox::initSubsysNameOffsets(int x, int y, bool activate)
{
	Subsys_name_offsets[0] = x;
	Subsys_name_offsets[1] = y;
	Use_subsys_name_offsets = activate;
}

void HudGaugeTargetBox::initSubsysIntegrityOffsets(int x, int y, bool activate)
{
	Subsys_integrity_offsets[0] = x;
	Subsys_integrity_offsets[1] = y;
	Use_subsys_integrity_offsets = activate;
}

void HudGaugeTargetBox::initDisabledStatusOffsets(int x, int y, bool activate)
{
	Disabled_status_offsets[0] = x;
	Disabled_status_offsets[1] = y;
	Use_disabled_status_offsets = activate;
}

void HudGaugeTargetBox::initDesaturate(bool desaturate)
{
	Desaturated = desaturate;
}

void HudGaugeTargetBox::initGaugeWireframe(int wireframe)
{
	GaugeWireframe = wireframe;
}

void HudGaugeTargetBox::initGaugeWirecolor(color wirecolor)
{
	GaugeWirecolor = wirecolor;
}

void HudGaugeTargetBox::initGaugeWirecolorOverride(bool wirecoloroverride)
{
	GaugeWirecolorOverride = wirecoloroverride;
}

void HudGaugeTargetBox::initBitmaps(char *fname_monitor, char *fname_monitor_mask, char *fname_integrity, char *fname_static)
{
	Monitor_frame.first_frame = bm_load_animation(fname_monitor, &Monitor_frame.num_frames);
	if ( Monitor_frame.first_frame < 0 ) {
		Warning(LOCATION,"Cannot load hud ani: %s\n", fname_monitor);
	}

	Integrity_bar.first_frame = bm_load_animation(fname_integrity, &Integrity_bar.num_frames);
	if ( Integrity_bar.first_frame < 0 ) {
		Warning(LOCATION,"Cannot load hud ani: %s\n", fname_integrity);
	}

	if ( strlen(fname_monitor_mask) > 0 ) {
		Monitor_mask = bm_load_animation(fname_monitor_mask);

		if ( Monitor_mask < 0 ) {
			Warning(LOCATION, "Cannot load bitmap hud mask: %s\n", fname_monitor_mask);
		}
	}

	strcpy_s(static_fname, fname_static);
}

void HudGaugeTargetBox::initialize()
{
	hud_anim_init(&Monitor_static, position[0] + Viewport_offsets[0], position[1] + Viewport_offsets[1], NOX(static_fname));

	for(int i = 0; i < NUM_TBOX_FLASH_TIMERS; i++) {
		initFlashTimer(i);
	}

	CurrentWire = GaugeWireframe;

	HudGauge::initialize();
}

void HudGaugeTargetBox::initFlashTimer(int index)
{
	Next_flash_timers[index] = 1;
	flash_flags &= ~(1<<index);
}

void HudGaugeTargetBox::render(float frametime, bool config)
{
	object	*target_objp = nullptr;

	if (!config && Player_ai->target_objnum == -1)
		return;
	
	if (!config && Target_static_playing ) 
		return;

	if (!config) {
		target_objp = &Objects[Player_ai->target_objnum];
	}

	int x = position[0];
	int y = position[1];
	float scale = 1.0;

	if (config) {
		std::tie(x, y, scale) = hud_config_convert_coord_sys(position[0], position[1], base_w, base_h);
		int bmw, bmh;
		bm_get_info(Monitor_frame.first_frame, &bmw, &bmh);
		hud_config_set_mouse_coords(gauge_config_id, x, x + fl2i(bmw * scale), y, y + fl2i(bmh * scale));
	}

	setGaugeColor(HUD_C_NONE, config);

	// blit the background frame
	if (Monitor_frame.first_frame >= 0)
		renderBitmap(Monitor_frame.first_frame, x, y, scale, config);

	if ( Monitor_mask >= 0 ) {
		// render the alpha mask
		gr_alpha_mask_set(1, 0.5f);
		gr_stencil_clear();
		gr_stencil_set(GR_STENCIL_WRITE);
		gr_set_color_buffer(0);

		renderBitmapColor(Monitor_mask, x, y, scale, config);

		gr_set_color_buffer(1);
		gr_stencil_set(GR_STENCIL_NONE);
		gr_alpha_mask_set(0, 1.0f);
	}

	if (!config && target_objp == nullptr) {
		return;
	}

	if (!config) {
		switch (target_objp->type) {
			case OBJ_SHIP:
				renderTargetShip(target_objp);
				break;

			case OBJ_DEBRIS:
				renderTargetDebris(target_objp);
				break;

			case OBJ_WEAPON:
				renderTargetWeapon(target_objp);
				break;

			case OBJ_ASTEROID:
				renderTargetAsteroid(target_objp);
				break;

			case OBJ_JUMP_NODE:
				renderTargetJumpNode(target_objp);
				break;

			default:
				hud_cease_targeting();
				break;
		} // end switch
	} else {
		renderTargetShip(target_objp, config);
	}

	if (!config && Target_static_playing ) {
		setGaugeColor(HUD_C_NONE, config);
		gr_set_screen_scale(base_w, base_h);
		hud_anim_render(&Monitor_static, frametime, 1);
		gr_reset_screen_scale();
	} else {
		showTargetData(frametime, config);
	}

	if(config || Target_display_cargo) {
		// Print out what the cargo is
		if (!config) {
			if (maybeFlashSexp() == 1) {
				setGaugeColor(HUD_C_BRIGHT);
			} else {
				maybeFlashElement(TBOX_FLASH_CARGO);
			}
		}

		renderString(x + fl2i(Cargo_string_offsets[0] * scale), y + fl2i(Cargo_string_offsets[1] * scale), EG_TBOX_CARGO, config ? "cargo: <unknown>" : Cargo_string, scale, config);
	}
}

void HudGaugeTargetBox::renderTargetForeground(bool config)
{
	setGaugeColor(HUD_C_NONE, config);

	int x = position[0];
	int y = position[1];
	float scale = 1.0;

	if (config) {
		std::tie(x, y, scale) = hud_config_convert_coord_sys(position[0], position[1], base_w, base_h);
	}
	
	if (Monitor_frame.first_frame + 1 >= 0)
		renderBitmap(Monitor_frame.first_frame+1, x, y, scale, config);	
}

/**
 * Draw the integrity bar that is on the right of the target monitor
 */
void HudGaugeTargetBox::renderTargetIntegrity(int disabled,int force_obj_num, bool config)
{
	if ( Integrity_bar.first_frame < 0 ) 
		return;

	int x = position[0];
	int y = position[1];
	float scale = 1.0;

	if (config) {
		std::tie(x, y, scale) = hud_config_convert_coord_sys(position[0], position[1], base_w, base_h);
	}

	if (!config && disabled ) {
		renderBitmap(Integrity_bar.first_frame, x + fl2i(Integrity_bar_offsets[0] * scale), y + fl2i(Integrity_bar_offsets[1] * scale));
		return;
	}

	if (!config) {
		if (force_obj_num == -1) {
			Assert(Player_ai->target_objnum >= 0);
		}
	}

	int clip_h = fl2i((1 - (config ? 1.0 : Pl_target_integrity)) * integrity_bar_h);

	int status = Current_ts;
	if (config) {
		status = TS_OK;
	}

	// print out status of ship
	char buf[16];
	switch(status) {
		case TS_DIS:
			strcpy_s(buf,XSTR( "dis", 344));
			break;
		case TS_OK:
			strcpy_s(buf,XSTR( "ok", 345));
			break;
		case TS_DMG:
			strcpy_s(buf,XSTR( "dmg", 346));
			break;
		case TS_CRT:
			strcpy_s(buf,XSTR( "crt", 347));
			break;
		default:
			UNREACHABLE("Unhandled target integrity status %d in HudGaugeTargetBox::renderTargetIntegrity!", status);
			buf[0] = '\0';
			break;
	}

	if (!config) {
		maybeFlashElement(TBOX_FLASH_STATUS);
	}
	
	// finally print out the status of this ship
	renderString(x + fl2i(Status_offsets[0] * scale), y + fl2i(Status_offsets[1] * scale), EG_TBOX_INTEG, buf, scale, config);	

	setGaugeColor(HUD_C_NONE, config);

	int w, h;
	bm_get_info(Integrity_bar.first_frame,&w,&h);
	
	if ( clip_h > 0 ) {
		// draw the dark portion
		renderBitmapEx(Integrity_bar.first_frame, x + fl2i(Integrity_bar_offsets[0] * scale), y + fl2i(Integrity_bar_offsets[1] * scale), w, clip_h,0,0, scale, config);		
	}

	if (clip_h <= integrity_bar_h) {
		// draw the bright portion
		renderBitmapEx(Integrity_bar.first_frame + 1,
			x + fl2i(Integrity_bar_offsets[0] * scale),
			y + fl2i(Integrity_bar_offsets[1] * scale) + fl2i(clip_h * scale),
			w,
			h - clip_h,
			0,
			clip_h,
			scale,
			config);		
	}
}

void HudGaugeTargetBox::renderTargetSetup(vec3d *camera_eye, matrix *camera_orient, fov_t zoom)
{
	// JAS: g3_start_frame uses clip_width and clip_height to determine the
	// size to render to.  Normally, you would set this by using gr_set_clip,
	// but because of the hacked in hud jittering, I couldn't.  So come talk
	// to me before modifying or reusing the following code. Thanks.

	int clip_width = Viewport_w;
	int clip_height = Viewport_h;

	gr_screen.clip_width = clip_width;
	gr_screen.clip_height = clip_height;
	g3_start_frame(1);		// Turn on zbuffering
	hud_save_restore_camera_data(1);
	g3_set_view_matrix( camera_eye, camera_orient, zoom);	
	model_set_detail_level(1);		// use medium detail level

	setClip(position[0] + Viewport_offsets[0], position[1] + Viewport_offsets[1], Viewport_w, Viewport_h);

	// account for gauge RTT with cockpit here --wookieejedi
	float clip_aspect;
	if (gr_screen.rendering_to_texture != -1) {
		clip_aspect = (i2fl(clip_width) / i2fl(clip_height));
	} else {
		clip_aspect = gr_screen.clip_aspect;
	}

	gr_set_proj_matrix(Proj_fov, clip_aspect, Min_draw_distance, Max_draw_distance);
	gr_set_view_matrix(&Eye_position, &Eye_matrix);
}

void HudGaugeTargetBox::renderTargetShip(object *target_objp, bool config)
{
	ship		*target_shipp = nullptr;
	ship_info	*target_sip = nullptr;
	
	if (!config) {
		target_shipp = &Ships[target_objp->instance];
		target_sip = &Ship_info[target_shipp->ship_info_index];
	}

	// Config doesn't render a ship.. but it'd be cool if it did. Maybe some other day.
	if (!config) {
		uint64_t flags = 0;
		if (Detail.targetview_model) {
			// take the forward orientation to be the vector from the player to the current target
			vec3d orient_vec;
			vm_vec_normalized_dir(&orient_vec, &target_objp->pos, &Player_obj->pos);

			float factor = -target_sip->closeup_pos_targetbox.xyz.z;

			matrix camera_orient = IDENTITY_MATRIX;

			// use the player's up vector, and construct the viewers orientation matrix
			vec3d tempv;
			object_get_eye(&tempv, &camera_orient, Player_obj, false);

			auto up_vector = &camera_orient.vec.uvec;
			vm_vector_2_matrix_norm(&camera_orient, &orient_vec, up_vector, nullptr);

			// normalize the vector from the player to the current target, and scale by a factor to calculate
			// the objects position
			vec3d obj_pos = ZERO_VECTOR;
			vm_vec_copy_scale(&obj_pos, &orient_vec, factor);

			// RT, changed scaling here
			vec3d camera_eye = ZERO_VECTOR;
			renderTargetSetup(&camera_eye, &camera_orient, target_sip->closeup_zoom_targetbox);

			// IMPORTANT NOTE! Code handling the case 'missile_view == TRUE' in rendering section of
			// renderTargetWeapon()
			//                 is largely copied over from renderTargetShip(). To keep the codes similar please update
			//                 both if and when needed
			model_render_params render_info;
			render_info.set_object_number(OBJ_INDEX(target_objp));

			color thisColor = GaugeWirecolor;
			bool thisOverride = GaugeWirecolorOverride;

			if (target_sip->uses_team_colors) {
				render_info.set_team_color(target_shipp->team_name,
					target_shipp->secondary_team_name,
					target_shipp->team_change_timestamp,
					target_shipp->team_change_time);
			}

			switch (CurrentWire) {
			case 0:
				flags |= MR_NO_LIGHTING;

				break;
			case 1:
				if (thisOverride) {
					render_info.set_color(thisColor);
				} else {
					if (ship_is_tagged(target_objp))
						render_info.set_color(*iff_get_color(IFF_COLOR_TAGGED, 1));
					else
						render_info.set_color(
							*iff_get_color_by_team_and_object(target_shipp->team, Player_ship->team, 1, target_objp));
				}

				flags = MR_SHOW_OUTLINE_HTL | MR_NO_POLYS | MR_NO_LIGHTING | MR_NO_TEXTURING;

				break;
			case 2:
				break;
			case 3:
				if (thisOverride) {
					render_info.set_color(thisColor);
				} else {
					if (ship_is_tagged(target_objp))
						render_info.set_color(*iff_get_color(IFF_COLOR_TAGGED, 1));
					else
						render_info.set_color(
							*iff_get_color_by_team_and_object(target_shipp->team, Player_ship->team, 1, target_objp));
				}

				flags |= MR_NO_LIGHTING | MR_NO_TEXTURING;

				break;
			}

			if (target_sip->hud_target_lod >= 0) {
				render_info.set_detail_level_lock(target_sip->hud_target_lod);
			}

			if (Targetbox_shader_effect > -1) {
				render_info.set_animated_effect(Targetbox_shader_effect, 0.0f);
			}

			if (Monitor_mask >= 0) {
				gr_stencil_set(GR_STENCIL_READ);
			}

			if (Desaturated) {
				flags |= MR_DESATURATED;
				render_info.set_color(gauge_color);
			}

			if (!Glowpoint_override)
				Glowpoint_override = true;

			// set glowmap flag here since model_render (etc) require an objnum to handle glowmaps
			// if we did pass the objnum, we'd also have thrusters drawn in the targetbox
			if (target_shipp->flags[Ship::Ship_Flags::Glowmaps_disabled]) {
				flags |= MR_NO_GLOWMAPS;
			}

			render_info.set_flags(flags | MR_AUTOCENTER | MR_NO_FOGGING);

			// maybe render a special hud-target-only model
			if (target_sip->model_num_hud >= 0) {
				model_render_immediate(&render_info, target_sip->model_num_hud, &target_objp->orient, &obj_pos);
			} else {
				render_info.set_replacement_textures(
					model_get_instance(target_shipp->model_instance_num)->texture_replace);

				model_render_immediate(&render_info, target_sip->model_num, &target_objp->orient, &obj_pos);
			}

			Glowpoint_override = false;

			if (Monitor_mask >= 0) {
				gr_stencil_set(GR_STENCIL_NONE);
			}

			int sx = 0;
			int sy = 0;
			// check if subsystem target has changed
			if (Player_ai->targeted_subsys == Player_ai->last_subsys_target) {
				vec3d save_pos;

				if (gr_screen.rendering_to_texture != -1) {
					gr_set_screen_scale(canvas_w, canvas_h, -1, -1, target_w, target_h, target_w, target_h, true);
				} else {
					gr_set_screen_scale(base_w, base_h);
				}

				save_pos = target_objp->pos;
				target_objp->pos = obj_pos;
				int subsys_in_view = hud_targetbox_subsystem_in_view(target_objp, &sx, &sy);
				target_objp->pos = save_pos;

				if (subsys_in_view != -1) {

					// AL 29-3-98: If subsystem is destroyed, draw gray brackets
					// Goober5000 - hm, caught a tricky bug for destroyable fighterbays
					if ((Player_ai->targeted_subsys->current_hits <= 0) &&
						ship_subsys_takes_damage(Player_ai->targeted_subsys)) {
						gr_set_color_fast(iff_get_color(IFF_COLOR_MESSAGE, 1));
					} else {
						hud_set_iff_color(target_objp, 1);
					}

					graphics::line_draw_list line_draw_list;
					if (subsys_in_view) {
						draw_brackets_square_quick(&line_draw_list, sx - 10, sy - 10, sx + 10, sy + 10);
					} else {
						draw_brackets_diamond_quick(&line_draw_list, sx - 10, sy - 10, sx + 10, sy + 10);
					}
					line_draw_list.flush();
				}
			}
			renderTargetClose();
		}
	}
	renderTargetForeground(config);
	renderTargetIntegrity(0,config ? -1 : OBJ_INDEX(target_objp), config);

	setGaugeColor(HUD_C_NONE, config);

	renderTargetShipInfo(target_objp, config);
	if (!config) {
		maybeRenderCargoScan(target_sip, Player_ai->targeted_subsys);
	}
}

/**
 * @note formerly hud_render_target_debris(object *target_objp) (Swifty)
 */
void HudGaugeTargetBox::renderTargetDebris(object *target_objp)
{
	vec3d	obj_pos = ZERO_VECTOR;
	vec3d	camera_eye = ZERO_VECTOR;
	matrix	camera_orient = IDENTITY_MATRIX;
	debris	*debrisp;
	float		factor;	
	uint64_t flags = 0;

	debrisp = &Debris[target_objp->instance];

	if ( Detail.targetview_model )	{
		// take the forward orientation to be the vector from the player to the current target
		vec3d orient_vec;
		vm_vec_normalized_dir(&orient_vec, &target_objp->pos, &Player_obj->pos);

		factor = 2*target_objp->radius;

		// use the player's up vector, and construct the viewer's orientation matrix
		vec3d tempv;
		object_get_eye(&tempv, &camera_orient, Player_obj, false);

		auto up_vector = &camera_orient.vec.uvec;
		vm_vector_2_matrix_norm(&camera_orient, &orient_vec, up_vector, nullptr);

		// normalize the vector from the player to the current target, and scale by a factor to calculate
		// the objects position
		vm_vec_copy_scale(&obj_pos,&orient_vec,factor);

		renderTargetSetup(&camera_eye, &camera_orient, 0.5f);
		model_clear_instance(debrisp->model_num);

		model_render_params render_info;

		if (debrisp->model_instance_num >= 0)
			render_info.set_replacement_textures(model_get_instance(debrisp->model_instance_num)->texture_replace);

		color thisColor = GaugeWirecolor;
		bool thisOverride = GaugeWirecolorOverride;

		switch (CurrentWire) {
			case 0:
				flags |= MR_NO_LIGHTING;

				break;
			case 1:
				if (thisOverride) {
					render_info.set_color(thisColor);
				} else {
					render_info.set_color(255, 255, 255);
				}

				flags = MR_SHOW_OUTLINE_HTL | MR_NO_POLYS | MR_NO_LIGHTING | MR_NO_TEXTURING;

				break;
			case 2:
				break;
			case 3:
				if (thisOverride) {
					render_info.set_color(thisColor);
				} else {
					render_info.set_color(255, 255, 255);
				}

				flags |= MR_NO_LIGHTING | MR_NO_TEXTURING;

				break;
		}

		if(Targetbox_shader_effect > -1) {
			render_info.set_animated_effect(Targetbox_shader_effect, 0.0f);
		}

		if ( Monitor_mask >= 0 ) {
			gr_stencil_set(GR_STENCIL_READ);
		}

		if ( Desaturated ) {
			flags |= MR_DESATURATED;
			render_info.set_color(gauge_color);
		}

		render_info.set_flags(flags | MR_NO_FOGGING);

		auto pm = model_get(debrisp->model_num);
		auto pmi = debrisp->model_instance_num < 0 ? nullptr : model_get_instance(debrisp->model_instance_num);

		// This calls the colour that doesn't get reset
		submodel_render_immediate( &render_info, pm, pmi, debrisp->submodel_num, &target_objp->orient, &obj_pos);

		if ( Monitor_mask >= 0 ) {
			gr_stencil_set(GR_STENCIL_NONE);
		}

		renderTargetClose();
	}
	renderTargetForeground();
	renderTargetIntegrity(1);

	// print out ship class that debris came from
	const char *printable_ship_class;
	if (debrisp->parent_alt_name >= 0)
		printable_ship_class = mission_parse_lookup_alt_index(debrisp->parent_alt_name);
	else
		printable_ship_class = Ship_info[debrisp->ship_info_index].get_display_name();
	
	renderString(position[0] + Class_offsets[0], position[1] + Class_offsets[1], EG_TBOX_CLASS, printable_ship_class);	
	renderString(position[0] + Name_offsets[0], position[1] + Name_offsets[1], EG_TBOX_NAME, XSTR("debris", 348));	
}

/**
 * @note Formerly hud_render_target_weapon(object *target_objp)
 */
void HudGaugeTargetBox::renderTargetWeapon(object *target_objp)
{
	vec3d		obj_pos = ZERO_VECTOR;
	vec3d		camera_eye = ZERO_VECTOR;
	matrix		camera_orient = IDENTITY_MATRIX;
	weapon_info	*target_wip = NULL;
	weapon		*wp = NULL;
	object		*viewer_obj, *viewed_obj;
	std::shared_ptr<model_texture_replace> replacement_textures = nullptr;
	int			target_team, is_homing, is_player_missile, missile_view, viewed_model_num, hud_target_lod, w, h;
	uint64_t flags = 0;

	target_team = obj_team(target_objp);

	wp = &Weapons[target_objp->instance];
	target_wip = &Weapon_info[wp->weapon_info_index];

	if (target_wip->model_num == -1)
		return;

	is_homing = FALSE;
	if ( target_wip->is_homing() && weapon_has_homing_object(wp) )
		is_homing = TRUE;

	is_player_missile = FALSE;
	if ( target_objp->parent_sig == Player_obj->signature ) {
		is_player_missile = TRUE;
	}

	if ( Detail.targetview_model )	{
		ship *homing_shipp = NULL;
		ship_info *homing_sip = NULL;

		viewer_obj			= Player_obj;
		viewed_obj			= target_objp;
		missile_view		= FALSE;
		viewed_model_num	= target_wip->model_num;
		hud_target_lod		= target_wip->hud_target_lod;
		// adding a check here to make sure the homing object is a ship, technically it could be some other object just as well
		if ( is_homing && is_player_missile && (wp->homing_object->type == OBJ_SHIP)) {
			homing_shipp = &Ships[wp->homing_object->instance];
			homing_sip = &Ship_info[homing_shipp->ship_info_index];

			viewer_obj			= target_objp;
			viewed_obj			= wp->homing_object;
			missile_view		= TRUE;
			viewed_model_num	= homing_sip->model_num;
			hud_target_lod		= homing_sip->hud_target_lod;
		}

		int pmi_id = object_get_model_instance_num(viewed_obj);
		if (pmi_id >= 0)
			replacement_textures = model_get_instance(pmi_id)->texture_replace;

		// take the forward orientation to be the vector from the player to the current target
		vec3d orient_vec;
		if (missile_view == FALSE)
			vm_vec_normalized_dir(&orient_vec, &viewed_obj->pos, &viewer_obj->pos);
		else
			vm_vec_normalized_dir(&orient_vec, &wp->homing_pos, &viewer_obj->pos);

		// use the viewer's up vector, and construct the viewer's orientation matrix
		vec3d tempv;
		object_get_eye(&tempv, &camera_orient, Player_obj, false);

		auto up_vector = &camera_orient.vec.uvec;
		vm_vector_2_matrix_norm(&camera_orient, &orient_vec, up_vector, nullptr);

		// normalize the vector from the viewer to the viwed target, and scale by a factor to calculate
		// the objects position
		if (missile_view == FALSE) {
			float factor = 2*target_objp->radius;
			// small radius missiles need a bigger factor otherwise they are rendered larger than the targetbox
			if (factor < 8.0f) {
				factor = 8.0f;
			}
			vm_vec_copy_scale(&obj_pos,&orient_vec,factor);
		} else {
			vm_vec_sub(&obj_pos, &viewed_obj->pos, &viewer_obj->pos);
		}

		renderTargetSetup(&camera_eye, &camera_orient, View_zoom * (1.0f/3.0f));
		model_clear_instance(viewed_model_num);
		
		model_render_params render_info;

		color thisColor = GaugeWirecolor;
		bool thisOverride = GaugeWirecolorOverride;

		// IMPORTANT NOTE! Code handling the rendering when 'missile_view == TRUE' is largely copied over from
		//                 renderTargetShip(). To keep the codes similar please update both if and when needed
		if (missile_view == FALSE) {
			switch (CurrentWire) {
				case 0:
					flags |= MR_NO_LIGHTING;

					break;
				case 1:
					if (thisOverride) {
						render_info.set_color(thisColor);
					} else {
						render_info.set_color(
							*iff_get_color_by_team_and_object(target_team, Player_ship->team, 0, target_objp));
					}

					flags = MR_SHOW_OUTLINE_HTL | MR_NO_POLYS | MR_NO_LIGHTING | MR_NO_TEXTURING;

					break;
				case 2:
					break;
				case 3:
					if (thisOverride) {
						render_info.set_color(thisColor);
					} else {
						render_info.set_color(*iff_get_color_by_team_and_object(target_team, Player_ship->team, 0, target_objp));
					}

					flags |= MR_NO_LIGHTING | MR_NO_TEXTURING;

					break;
			}
		} else {
			render_info.set_object_number(OBJ_INDEX(viewed_obj));

			switch (CurrentWire) {
				case 0:
					flags |= MR_NO_LIGHTING;

					break;
				case 1:
					if (thisOverride) {
						render_info.set_color(thisColor);
					} else {
						if (ship_is_tagged(viewed_obj))
							render_info.set_color(*iff_get_color(IFF_COLOR_TAGGED, 1));
						else
							render_info.set_color(*iff_get_color_by_team_and_object(homing_shipp->team, Player_ship->team, 1, viewed_obj));

						if (homing_sip->uses_team_colors) {
							render_info.set_team_color(homing_shipp->team_name, homing_shipp->secondary_team_name, homing_shipp->team_change_timestamp, homing_shipp->team_change_time);
						}
					}

					flags = MR_SHOW_OUTLINE_HTL | MR_NO_POLYS | MR_NO_LIGHTING | MR_NO_TEXTURING;

					break;
				case 2:
					break;
				case 3:
					if (thisOverride) {
						render_info.set_color(thisColor);
					} else {
						if (ship_is_tagged(viewed_obj))
							render_info.set_color(*iff_get_color(IFF_COLOR_TAGGED, 1));
						else
							render_info.set_color(*iff_get_color_by_team_and_object(homing_shipp->team, Player_ship->team, 1, viewed_obj));
					}

					flags |= MR_NO_LIGHTING | MR_NO_TEXTURING;

					break;
			}
		}

		if (hud_target_lod >= 0) {
			render_info.set_detail_level_lock(hud_target_lod);
		}

		if(Targetbox_shader_effect > -1) {
			render_info.set_animated_effect(Targetbox_shader_effect, 0.0f);
		}

		if ( Monitor_mask >= 0 ) {
			gr_stencil_set(GR_STENCIL_READ);
		}

		if ( Desaturated ) {
			flags |= MR_DESATURATED;
			render_info.set_color(gauge_color);
		}

		if (missile_view == TRUE) {
			if (!Glowpoint_override)
				Glowpoint_override = true;

			// set glowmap flag here since model_render (etc) require an objnum to handle glowmaps
			// if we did pass the objnum, we'd also have thrusters drawn in the targetbox
			if (homing_shipp->flags[Ship::Ship_Flags::Glowmaps_disabled]) {
				flags |= MR_NO_GLOWMAPS;
			}
		}
		
		if (missile_view == FALSE ) {
			render_info.set_flags(flags | MR_AUTOCENTER | MR_IS_MISSILE | MR_NO_FOGGING);
			render_info.set_replacement_textures(replacement_textures);

			model_render_immediate( &render_info, viewed_model_num, &viewed_obj->orient, &obj_pos );
		} else {
			// maybe render a special hud-target-only model
			// autocentering is bad in this one
			if(homing_sip->model_num_hud >= 0){
				render_info.set_flags(flags | MR_NO_FOGGING);

				model_render_immediate( &render_info, homing_sip->model_num_hud, &viewed_obj->orient, &obj_pos);
			} else {
				render_info.set_flags(flags | MR_NO_FOGGING);

				model_render_immediate( &render_info, homing_sip->model_num, &viewed_obj->orient, &obj_pos );
			}
		}

		if (missile_view == TRUE) {
			Glowpoint_override = false;
		}

		if ( Monitor_mask >= 0 ) {
			gr_stencil_set(GR_STENCIL_NONE);
		}

		renderTargetClose();
	}
	renderTargetForeground(); 

	renderTargetIntegrity(1);
	setGaugeColor();

	// print out the weapon class name
	auto weapon_name = target_wip->get_display_name();
	gr_get_string_size(&w,&h,weapon_name);

	renderString(position[0] + Name_offsets[0], position[1] + Name_offsets[1], EG_TBOX_NAME, weapon_name);

	// If a homing weapon, show time to impact
	if ( is_homing ) {
		float dist, speed;
		char			outstr[100];				// temp buffer

		speed = vm_vec_mag(&target_objp->phys_info.vel);

		// do the extra math only if it won't lead to null vec issues
		if(!(IS_VEC_NULL_SQ_SAFE(&target_objp->phys_info.vel))){
			vec3d unit_vec, component_vec;

			// in other words substract the magnitude of the target's velocity vectors parallel component from the speed of the weapon
			vm_vec_copy_normalize(&unit_vec, &target_objp->phys_info.vel);
			speed -= vm_vec_projection_parallel(&component_vec, &wp->homing_object->phys_info.vel, &unit_vec);
		}

		dist = vm_vec_dist(&target_objp->pos, &wp->homing_pos);
		
		if ( speed > 0 ) {
			sprintf(outstr, XSTR("impact: %.1f sec", 1596), dist/speed);
		} else {
			strcpy_s(outstr, XSTR( "unknown", 349));
		}

		renderString(position[0] + Class_offsets[0], position[1] + Class_offsets[1], EG_TBOX_CLASS, outstr);		
	}
}

/**
 * @note Formerly hud_render_target_asteroid(object *target_objp)
 */
void HudGaugeTargetBox::renderTargetAsteroid(object *target_objp)
{
	vec3d		obj_pos = ZERO_VECTOR;
	vec3d		camera_eye = ZERO_VECTOR;
	matrix		camera_orient = IDENTITY_MATRIX;
	asteroid		*asteroidp;
	float			time_to_impact, factor;	
	int			pof;

	uint64_t flags = 0;									//draw flags for wireframe
	asteroidp = &Asteroids[target_objp->instance];

	pof = asteroidp->asteroid_subtype;
	
	time_to_impact = asteroid_time_to_impact(target_objp);

	if ( Detail.targetview_model )	{
		// take the forward orientation to be the vector from the player to the current target
		vec3d orient_vec;
		vm_vec_normalized_dir(&orient_vec, &target_objp->pos, &Player_obj->pos);

		factor = 2*target_objp->radius;

		// use the player's up vector, and construct the viewer's orientation matrix
		vec3d tempv;
		object_get_eye(&tempv, &camera_orient, Player_obj, false);

		auto up_vector = &camera_orient.vec.uvec;
		vm_vector_2_matrix_norm(&camera_orient, &orient_vec, up_vector, nullptr);

		// normalize the vector from the player to the current target, and scale by a factor to calculate
		// the objects position
		vm_vec_copy_scale(&obj_pos,&orient_vec,factor);

		renderTargetSetup(&camera_eye, &camera_orient, 0.5f);
		model_clear_instance(Asteroid_info[asteroidp->asteroid_type].subtypes[pof].model_number);
		
		model_render_params render_info;

		if (asteroidp->model_instance_num >= 0)
			render_info.set_replacement_textures(model_get_instance(asteroidp->model_instance_num)->texture_replace);

		color thisColor = GaugeWirecolor;
		bool thisOverride = GaugeWirecolorOverride;

		switch (CurrentWire) {
			case 0:
				flags |= MR_NO_LIGHTING;

				break;
			case 1:
				if (thisOverride) {
					render_info.set_color(thisColor);
				} else {
					if (time_to_impact >= 0)
						render_info.set_color(255, 255, 255);
					else
						render_info.set_color(64, 64, 0);
				}

				flags = MR_SHOW_OUTLINE_HTL | MR_NO_POLYS | MR_NO_LIGHTING | MR_NO_TEXTURING;

				break;
			case 2:
				break;
			case 3:
				if (thisOverride) {
					render_info.set_color(thisColor);
				} else {
					if (time_to_impact >= 0)
						render_info.set_color(255, 255, 255);
					else
						render_info.set_color(64, 64, 0);
				}

				flags |= MR_NO_LIGHTING | MR_NO_TEXTURING;

				break;
		}

		if(Targetbox_shader_effect > -1) {
			render_info.set_animated_effect(Targetbox_shader_effect, 0.0f);
		}

		if ( Monitor_mask >= 0 ) {
			gr_stencil_set(GR_STENCIL_READ);
		}

		if ( Desaturated ) {
			flags |= MR_DESATURATED;
			render_info.set_color(gauge_color);
		}

		render_info.set_flags(flags | MR_NO_FOGGING);

		model_render_immediate( &render_info, Asteroid_info[asteroidp->asteroid_type].subtypes[pof].model_number, &target_objp->orient, &obj_pos );

		if ( Monitor_mask >= 0 ) {
			gr_stencil_set(GR_STENCIL_NONE);
		}

		renderTargetClose();
	}
	renderTargetForeground();
	renderTargetIntegrity(1);
	setGaugeColor();

	// hud print type of Asteroid (debris)
	char hud_name[64];
	switch (asteroidp->asteroid_type) {
		case ASTEROID_TYPE_SMALL:
		case ASTEROID_TYPE_MEDIUM:
		case ASTEROID_TYPE_LARGE:
			strcpy_s(hud_name, XSTR("asteroid", 431));
			break;

		default:
			strcpy_s(hud_name, Asteroid_info[asteroidp->asteroid_type].display_name.c_str());
			break;
	}

	renderString(position[0] + Name_offsets[0], position[1] + Name_offsets[1], EG_TBOX_NAME, hud_name);
	

	if ( time_to_impact >= 0.0f ) {
		renderPrintfWithGauge(position[0] + Class_offsets[0], position[1] + Class_offsets[1], EG_TBOX_CLASS, 1.0f, false, XSTR("impact: %.1f sec", 1596), time_to_impact);	
	}
}

/**
 * Render a jump node on the target monitor
 * @note Formerly hud_render_target_jump_node(object *target_objp)
 */
void HudGaugeTargetBox::renderTargetJumpNode(object *target_objp)
{
	char			outstr[256];
	vec3d		obj_pos = ZERO_VECTOR;
	vec3d		camera_eye = ZERO_VECTOR;
	matrix		camera_orient = IDENTITY_MATRIX;
	float			factor, dist;
	int			hx = 0, hy = 0, w, h;
	SCP_list<CJumpNode>::iterator jnp;
	
	for (jnp = Jump_nodes.begin(); jnp != Jump_nodes.end(); ++jnp) {
		if(jnp->GetSCPObject() != target_objp)
			continue;
	
		if ( jnp->IsHidden() ) {
			set_target_objnum( Player_ai, -1 );
			return;
		}

		if ( Detail.targetview_model )	{
			// take the forward orientation to be the vector from the player to the current target
			vec3d orient_vec;
			vm_vec_normalized_dir(&orient_vec, &target_objp->pos, &Player_obj->pos);

			factor = target_objp->radius*4.0f;

			// use the player's up vector, and construct the viewer's orientation matrix
			vec3d tempv;
			object_get_eye(&tempv, &camera_orient, Player_obj, false);

			auto up_vector = &camera_orient.vec.uvec;
			vm_vector_2_matrix_norm(&camera_orient, &orient_vec, up_vector, nullptr);

			// normalize the vector from the player to the current target, and scale by a factor to calculate
			// the objects position
			vm_vec_copy_scale(&obj_pos,&orient_vec,factor);

			renderTargetSetup(&camera_eye, &camera_orient, 0.5f);

			if ( Monitor_mask >= 0 ) {
				gr_stencil_set(GR_STENCIL_READ);
			}

			jnp->Render( &obj_pos );

			if ( Monitor_mask >= 0 ) {
				gr_stencil_set(GR_STENCIL_NONE);
			}

			renderTargetClose();
		}

		renderTargetForeground();
		renderTargetIntegrity(1);
		setGaugeColor();

		renderString(position[0] + Name_offsets[0], position[1] + Name_offsets[1], EG_TBOX_NAME, jnp->GetDisplayName());

		dist = Player_ai->current_target_distance;
		if ( Hud_unit_multiplier > 0.0f ) {	// use a different displayed distance scale
			dist = dist * Hud_unit_multiplier;
		}

		sprintf(outstr,XSTR( "d: %.0f", 340), dist);
		hud_num_make_mono(outstr, font_num);
		gr_get_string_size(&w,&h,outstr);
	
		renderPrintfWithGauge(position[0] + Dist_offsets[0]+hx, position[1] + Dist_offsets[1]+hy, EG_TBOX_DIST, 1.0f, false, "%s", outstr);
	}
}

/**
 * Toggle through the valid targetbox modes
 *
 * @note 0==standard
 * @note 1==wireframe only
 * @note 2==standard with lighting
 */
void hud_targetbox_switch_wireframe_mode()
{
	CurrentWire++;
		if (CurrentWire==3)
			CurrentWire=0;
}

/**
 * Init a specific targetbox timer
 */
void hud_targetbox_init_flash_timer(int index)
{
	Targetbox_flash_timers[index] = 1;
}

/**
 * Init the timers used to flash different parts of the targetbox.
 *
 * @note This needs to get called whenever the current target changes.
 * @note Need to call initFlashTimers for any TargetBox gauges and call initDockFlashTimer() for Extra Target Info gauges (Switfty)
 */
void hud_targetbox_init_flash()
{
	for(int i = 0; i < NUM_TBOX_FLASH_TIMERS; i++) {
		hud_targetbox_init_flash_timer(i);
	}

	Last_ts = -1;
	Current_ts = -1;
}

int HudGaugeTargetBox::maybeFlashElement(int index, int flash_fast)
{
	int draw_bright=0;

	setGaugeColor();
	if ( !timestamp_elapsed(Targetbox_flash_timers[index]) ) {
		if ( timestamp_elapsed(Next_flash_timers[index]) ) {
			if ( flash_fast ) {
				Next_flash_timers[index] = timestamp(fl2i(TBOX_FLASH_INTERVAL/2.0f));
			} else {
				Next_flash_timers[index] = timestamp(TBOX_FLASH_INTERVAL);
			}
			flash_flags ^= (1<<index);	// toggle between default and bright frames
		}

		if ( flash_flags & (1<<index) ) {
			setGaugeColor(HUD_C_BRIGHT);
			draw_bright=1;
		} else {			
			setGaugeColor(HUD_C_DIM);
		}
	}

	return draw_bright;
}

void HudGaugeTargetBox::renderTargetClose()
{
	gr_end_view_matrix();
	gr_end_proj_matrix();

	g3_end_frame();
	hud_save_restore_camera_data(0);
	resetClip();
}

/**
 * Get the shield and hull percentages for a given ship object
 *
 * @param objp		Pointer to ship object that you want strength values for
 * @param shields	OUTPUT parameter:	percentage value of shields (0->1.0)
 * @param integrity OUTPUT parameter: percentage value of integrity (0->1.0)
 */
void hud_get_target_strength(object *objp, float *shields, float *integrity)
{
	*shields = get_shield_pct(objp);
	*integrity = get_hull_pct(objp);
}

HudGaugeExtraTargetData::HudGaugeExtraTargetData():
HudGauge(HUD_OBJECT_EXTRA_TARGET_DATA, HUD_TARGET_MONITOR_EXTRA_DATA, false, false, (VM_EXTERNAL | VM_DEAD_VIEW | VM_WARP_CHASE | VM_PADLOCK_ANY), 255, 255, 255)
{
	initDockFlashTimer();
}

void HudGaugeExtraTargetData::initialize()
{
	initDockFlashTimer();

	HudGauge::initialize();
}

void HudGaugeExtraTargetData::initBracketOffsets(int x, int y)
{
	bracket_offsets[0] = x;
	bracket_offsets[1] = y;
}

void HudGaugeExtraTargetData::initDockOffsets(int x, int y)
{
	dock_offsets[0] = x;
	dock_offsets[1] = y;
}

void HudGaugeExtraTargetData::initDockMaxWidth(int width)
{
	dock_max_w = width;
}

void HudGaugeExtraTargetData::initTimeOffsets(int x, int y)
{
	time_offsets[0] = x;
	time_offsets[1] = y;
}

void HudGaugeExtraTargetData::initOrderOffsets(int x, int y)
{
	order_offsets[0] = x;
	order_offsets[1] = y;
}

void HudGaugeExtraTargetData::initOrderMaxWidth(int width)
{
	order_max_w = width;
}

void HudGaugeExtraTargetData::initBitmaps(char *fname)
{
	bracket.first_frame = bm_load_animation(fname, &bracket.num_frames);
	if ( bracket.first_frame < 0 ) {
		Warning(LOCATION,"Cannot load hud ani: %s\n", fname);
	}
}

void HudGaugeExtraTargetData::pageIn()
{
	bm_page_in_aabitmap( bracket.first_frame, bracket.num_frames );
}

/**
 * @note Formerly hud_targetbox_show_extra_ship_info(target_shipp, target_objp) (Swifty)
 */
void HudGaugeExtraTargetData::render(float  /*frametime*/, bool config)
{
	if(!config && !canRender())
		return;

	if (!config && Player_ai->target_objnum == -1)
		return;
	
	if (!config && Target_static_playing ) 
		return;

	object	*target_objp = nullptr;
	ship* target_shipp = nullptr;

	if (!config) {
		target_objp = &Objects[Player_ai->target_objnum];

		// only render if this the current target is type OBJ_SHIP
		if (target_objp->type != OBJ_SHIP)
			return;

		target_shipp = &Ships[target_objp->instance];
	}

	int x = position[0];
	int y = position[1];
	float scale = 1.0;

	if (config) {
		std::tie(x, y, scale) = hud_config_convert_coord_sys(position[0], position[1], base_w, base_h);
		int bmw, bmh;
		bm_get_info(bracket.first_frame, &bmw, &bmh);
		hud_config_set_mouse_coords(gauge_config_id, x, x + fl2i(order_max_w * scale), y, y + fl2i(bmh * scale));
	}

	setGaugeColor(HUD_C_NONE, config);

	bool extra_data_shown = false;

	bool not_training = !(The_mission.game_type & MISSION_TYPE_TRAINING);
	if (config || not_training) {
		bool has_orders = false;

		// Print out current orders if the targeted ship is friendly
		// AL 12-26-97: only show orders and time to target for friendly ships
		// Backslash: actually let's consult the IFF table.  Maybe we want to show orders for certain teams, or hide orders for friendlies
		if (config || (((Player_ship->team == target_shipp->team) ||
					((Iff_info[target_shipp->team].flags & IFFF_ORDERS_SHOWN) && !(Iff_info[target_shipp->team].flags & IFFF_ORDERS_HIDDEN)))
				&& Ship_info[target_shipp->ship_info_index].is_flyable()) ) {
			extra_data_shown = true;
			SCP_string orders;
			if (!config) {
				orders = ship_return_orders(target_shipp);
			}
			if (!orders.empty()) {
				char outstr[256];
				strcpy_s(outstr, orders.c_str());
				font::force_fit_string(outstr, 255, fl2i(order_max_w * scale));
				orders = outstr;
				has_orders = true;
			} else {
				orders = XSTR("no orders", 337);
			}

			renderString(x + fl2i(order_offsets[0] * scale), y + fl2i(order_offsets[1] * scale), EG_TBOX_EXTRA1, orders.c_str(), scale, config);
		}

		if (!config && has_orders ) {
			char outstr[256];
			strcpy_s(outstr, XSTR( "time to: ", 338));
			char tmpbuf[256];
			if ( ship_return_time_to_goal(tmpbuf, target_shipp) ) {
				strcat_s(outstr, tmpbuf);
				
				renderString(x + fl2i(time_offsets[0] * scale), y + fl2i(time_offsets[1] * scale), EG_TBOX_EXTRA2, outstr, scale, config);				
			}
		}
	}

	if (!config && Player_ai->last_target != Player_ai->target_objnum) {
		endFlashDock();
	}

	// Print out dock status
	if (!config && object_is_docked(target_objp) )
	{
		startFlashDock(2000);
		// count the objects directly docked to me
		int dock_count = dock_count_direct_docked_objects(target_objp);

		char outstr[256];
		// docked to only one object
		if (dock_count == 1)
		{
			sprintf(outstr, XSTR("Docked: %s", 339), Ships[dock_get_first_docked_object(target_objp)->instance].get_display_name());
		}
		// docked to multiple objects
		else
		{
			sprintf(outstr, XSTR("Docked: %d objects", 1623), dock_count);
		}

		font::force_fit_string(outstr, 255, fl2i(dock_max_w * scale));
		maybeFlashDock();
			
		renderString(x + fl2i(dock_offsets[0] * scale), y + fl2i(dock_offsets[1] * scale), EG_TBOX_EXTRA3, outstr, scale, config);			
		extra_data_shown=true;
	}

	if ( extra_data_shown && bracket.first_frame >= 0) {
		renderBitmap(bracket.first_frame, x + fl2i(bracket_offsets[0] * scale), y + fl2i(bracket_offsets[1] * scale), scale, config);		
	}
}

void HudGaugeExtraTargetData::initDockFlashTimer()
{
	flash_timer[0] = 1;
	flash_timer[1] = 1;
	flash_flags = false;
}

void HudGaugeExtraTargetData::startFlashDock(int duration)
{
	flash_timer[0] = timestamp(duration);
}

int HudGaugeExtraTargetData::maybeFlashDock(int flash_fast)
{
	int draw_bright=0;

	setGaugeColor();
	if ( !timestamp_elapsed(flash_timer[0]) ) {
		if ( timestamp_elapsed(flash_timer[1]) ) {
			if ( flash_fast ) {
				flash_timer[1] = timestamp(fl2i(TBOX_FLASH_INTERVAL/2.0f));
			} else {
				flash_timer[1] = timestamp(TBOX_FLASH_INTERVAL);
			}

			// toggle between default and bright frames
			if(flash_flags)
				flash_flags = false;
			else
				flash_flags = true;
		}

		if (flash_flags) {
			setGaugeColor(HUD_C_BRIGHT);
			draw_bright=1;
		} else {			
			setGaugeColor(HUD_C_DIM);
		}
	}

	return draw_bright;
}

void HudGaugeExtraTargetData::endFlashDock()
{
	flash_timer[0] = timestamp(0);
}

void HudGaugeTargetBox::renderTargetShipInfo(object *target_objp, bool config)
{
	int x = position[0];
	int y = position[1];
	float scale = 1.0;

	if (config) {
		std::tie(x, y, scale) = hud_config_convert_coord_sys(position[0], position[1], base_w, base_h);
	}

	ship* target_shipp = nullptr;

	char outstr_class[NAME_LENGTH];
	char outstr_name[NAME_LENGTH * 2 + 3];
	if (!config) {
		Assert(target_objp); // Goober5000
		Assert(target_objp->type == OBJ_SHIP);
		target_shipp = &Ships[target_objp->instance];

		// set up colors
		if (HudGauge::maybeFlashSexp() == 1) {
			hud_set_iff_color(target_objp, 1);
		} else {
			// Print out ship name, with wing name if it exists
			if (maybeFlashElement(TBOX_FLASH_NAME)) {
				hud_set_iff_color(target_objp, 1);
			} else {
				hud_set_iff_color(target_objp);
			}
		}

		// set up lines
		hud_stuff_ship_name(outstr_name, target_shipp);
		hud_stuff_ship_class(outstr_class, target_shipp);

		// maybe concatenate the callsign
		if (*outstr_name)
		{
			char outstr_callsign[NAME_LENGTH];

			hud_stuff_ship_callsign(outstr_callsign, target_shipp);
			if (*outstr_callsign)
				sprintf(&outstr_name[strlen(outstr_name)], " (%s)", outstr_callsign);
		}
		// maybe substitute the callsign
		else
		{
			hud_stuff_ship_callsign(outstr_name, target_shipp);
		}
	} else {
		strcpy(outstr_name, "Target Ship");
		strcpy(outstr_class, "Fighter Class");
	}

	// print lines based on current coords
	renderString(x + fl2i(Name_offsets[0] * scale), y + fl2i(Name_offsets[1] * scale), EG_TBOX_NAME, outstr_name, scale, config);	
	renderString(x + fl2i(Class_offsets[0] * scale), y + fl2i(Class_offsets[1] * scale), EG_TBOX_CLASS, outstr_class, scale, config);

	// ----------

	float ship_integrity = 1.0f;
	float shield_strength = 1.0f;
	if (!config) {
		hud_get_target_strength(target_objp, &shield_strength, &ship_integrity);
	}

	// convert to values of 0->100
	shield_strength *= 100.0f;
	ship_integrity *= 100.0f;

	int screen_integrity = fl2i(std::lround(ship_integrity));
	if ( screen_integrity == 0 ) {
		if ( ship_integrity > 0 ) {
			screen_integrity = 1;
		}
	}
	// Print out right-justified integrity
	char outstr[NAME_LENGTH];
	sprintf(outstr, XSTR( "%d%%", 341), screen_integrity);
	int w, h;
	gr_get_string_size(&w,&h,outstr, scale);

	if (!config) {
		if (HudGauge::maybeFlashSexp() == 1) {
			setGaugeColor(HUD_C_BRIGHT);
		} else {
			maybeFlashElement(TBOX_FLASH_HULL);
		}
	}

	renderPrintfWithGauge(x + fl2i(Hull_offsets[0] * scale)-w, y + fl2i(Hull_offsets[1] * scale), EG_TBOX_HULL, scale, config, "%s", outstr);	
	setGaugeColor(HUD_C_NONE, config);

	// Config version doesn't do any of the stuff below
	if (config) {
		return;
	}

	// print out the targeted sub-system and % integrity
	if (Player_ai->targeted_subsys != nullptr) {
		shield_strength = Player_ai->targeted_subsys->current_hits/Player_ai->targeted_subsys->max_hits * 100.0f;
		screen_integrity = fl2i(std::lround(shield_strength));

		if ( screen_integrity < 0 ) {
			screen_integrity = 0;
		}

		if ( screen_integrity == 0 ) {
			if ( shield_strength > 0 ) {
				screen_integrity = 1;
			}
		}

		maybeFlashElement(TBOX_FLASH_SUBSYS);

		sprintf(outstr, "%s", ship_subsys_get_name_on_hud(Player_ai->targeted_subsys));

		char *p_line;
		// hence pipe shall be the linebreak
		char linebreak[2] = "|";
		int n_linebreaks = 0;
		p_line = strpbrk(outstr,linebreak);
		
		// figure out how many linebreaks we actually have
		while (p_line != nullptr) {
			n_linebreaks++;
			p_line = strpbrk(p_line+1,linebreak);
		}

		int subsys_name_pos_x;
		int subsys_name_pos_y;

		if ( Use_subsys_name_offsets ) {
			subsys_name_pos_x = position[0] + Subsys_name_offsets[0];
			subsys_name_pos_y = position[1] + Subsys_name_offsets[1];
		} else {
			subsys_name_pos_x = position[0] + Viewport_offsets[0] + 2;
			subsys_name_pos_y = position[1] + Viewport_offsets[1] + Viewport_h;
		}

		if (n_linebreaks) {
			p_line = strtok(outstr,linebreak);
			while (p_line != nullptr) {
				renderPrintf(subsys_name_pos_x, subsys_name_pos_y-h-((h+1)*n_linebreaks), 1.0, false, "%s", p_line);
				p_line = strtok(nullptr,linebreak);
				n_linebreaks--;
			}
		} else {
			hud_targetbox_truncate_subsys_name(outstr);
			renderPrintf(subsys_name_pos_x, subsys_name_pos_y - h, 1.0, false, "%s", outstr);
		}

		int subsys_integrity_pos_x;
		int subsys_integrity_pos_y;

		if ( Use_subsys_integrity_offsets ) {
			subsys_integrity_pos_x = position[0] + Subsys_integrity_offsets[0];
			subsys_integrity_pos_y = position[1] + Subsys_integrity_offsets[1];
		} else {
			subsys_integrity_pos_x = position[0] + Viewport_offsets[0] + Viewport_w - 1;
			subsys_integrity_pos_y = position[1] + Viewport_offsets[1] + Viewport_h;
		}

		// AL 23-3-98: Fighter bays are a special case.  Player cannot destroy them, so don't
		//					show the subsystem strength
		// Goober5000: don't display any strength if we can't destroy this subsystem - but sometimes
		// fighterbays can be destroyed
		if ( ship_subsys_takes_damage(Player_ai->targeted_subsys) )
		{
			sprintf(outstr,XSTR( "%d%%", 341),screen_integrity);
			gr_get_string_size(&w,&h,outstr);
			renderPrintf(subsys_integrity_pos_x - w, subsys_integrity_pos_y - h, 1.0, false, "%s", outstr);
		}

		setGaugeColor();
	}

	// print out 'disabled' on the monitor if the target is disabled
	if ( (target_shipp->flags[Ship::Ship_Flags::Disabled]) || (ship_subsys_disrupted(target_shipp, SUBSYSTEM_ENGINE)) ) {
		if ( target_shipp->flags[Ship::Ship_Flags::Disabled] ) {
			strcpy_s(outstr, XSTR( "DISABLED", 342));
		} else {
			strcpy_s(outstr, XSTR( "DISRUPTED", 343));
		}
		gr_get_string_size(&w,&h,outstr);

		int disabled_status_pos_x;
		int disabled_status_pos_y;

		if ( Use_disabled_status_offsets ) {
			disabled_status_pos_x = position[0] + Disabled_status_offsets[0];
			disabled_status_pos_y = position[1] + Disabled_status_offsets[1];
		} else {
			disabled_status_pos_x = position[0] + Viewport_offsets[0] + Viewport_w/2 - w/2 - 1;
			disabled_status_pos_y = position[1] + Viewport_offsets[1] + Viewport_h - 2*h;
		}

		renderPrintf(disabled_status_pos_x, disabled_status_pos_y, 1.0, false, "%s", outstr);
	}
}

/**
 * Determine if the subsystem is in line-of sight, without taking into account whether the player ship is
 * facing the subsystem
 */
int hud_targetbox_subsystem_in_view(object *target_objp, int *sx, int *sy)
{
	ship_subsys	*subsys;
	vec3d		subobj_pos;
	vertex		subobj_vertex;
	int			rval = -1;
	polymodel	*pm;

	subsys = Player_ai->targeted_subsys;
	if (subsys != NULL ) {
		get_subsystem_pos(&subobj_pos, target_objp, subsys);

		// is it subsystem in view
		if ( Player->subsys_in_view == -1 ) {
			rval = ship_subsystem_in_sight(target_objp, subsys, &View_position, &subobj_pos, false) ? 1 : 0;
		} else {
			rval =  Player->subsys_in_view;
		}

		// get screen coords, adjusting for autocenter
		Assert(target_objp->type == OBJ_SHIP);
		if (target_objp->type == OBJ_SHIP) {
			pm = model_get(Ship_info[Ships[target_objp->instance].ship_info_index].model_num);
			if (pm->flags & PM_FLAG_AUTOCEN) {
				vec3d temp, delta;
				vm_vec_copy_scale(&temp, &pm->autocenter, -1.0f);
				vm_vec_unrotate(&delta, &temp, &target_objp->orient);
				vm_vec_add2(&subobj_pos, &delta);
			}
		}

		g3_rotate_vertex(&subobj_vertex, &subobj_pos);
		g3_project_vertex(&subobj_vertex);
		*sx = (int) subobj_vertex.screen.xyw.x;
		*sy = (int) subobj_vertex.screen.xyw.y;
	}

	return rval;
}

void hud_cargo_scan_update(object *targetp, float frametime)
{
	// update cargo inspection status
	Cargo_string[0] = 0;
	if ( targetp->type == OBJ_SHIP ) {
		Target_display_cargo = player_inspect_cargo(frametime, Cargo_string);
		if ( Target_display_cargo ) {
			if ( Player->cargo_inspect_time > 0 ) {
				hud_targetbox_start_flash(TBOX_FLASH_CARGO);
			}
		}
	}
}

void hud_update_cargo_scan_sound()
{
	if ( Player->cargo_inspect_time <= 0  ) {
		player_stop_cargo_scan_sound();
		return;
	}
	player_maybe_start_cargo_scan_sound();

}

/**
 * If the player is scanning for cargo, draw some cool scanning lines on the target monitor
 */
void HudGaugeTargetBox::maybeRenderCargoScan(ship_info *target_sip, ship_subsys *target_subsys)
{
	int x1, y1, x2, y2;
	float scan_time;				// time required to scan ship

	if ( Player->cargo_inspect_time <= 0  ) {
		return;
	}

	if (target_subsys && target_subsys->system_info->scan_time > 0)
		scan_time = i2fl(target_subsys->system_info->scan_time);
	else
		scan_time = i2fl(target_sip->scan_time);
	scan_time *= Ship_info[Player_ship->ship_info_index].scanning_time_multiplier;

	setGaugeColor(HUD_C_BRIGHT);

	int left = position[0] + Cargo_scan_start_offsets[0];
	int right = left + Cargo_scan_w;
	int top = position[1] + Cargo_scan_start_offsets[1];
	int bot = top + Cargo_scan_h;

	float t = i2fl(Player->cargo_inspect_time) / scan_time;

	if (Cargo_scan_type == CargoScanType::DEFAULT || Cargo_scan_type == CargoScanType::DUAL_SCAN_LINES) {
		// draw horizontal scan line
		x1 = left;
		y1 = fl2i(0.5f + top + (t * Cargo_scan_h));
		x2 = x1 + Cargo_scan_w;

		renderLine(x1, y1, x2, y1);

		// RT Changed this to be optional
		if (Cargo_scan_type == CargoScanType::DUAL_SCAN_LINES) {
			// added 2nd horizontal scan line - phreak
			y1 = fl2i(bot - (t * Cargo_scan_h));
			renderLine(x1, y1, x2, y1);
		}

		// draw vertical scan line
		x1 = fl2i(0.5f + left + (t * Cargo_scan_w));
		y1 = top;
		y2 = bot;

		renderLine(x1, y1 - 3, x1, y2 - 1);

		// RT Changed this to be optional
		if (Cargo_scan_type == CargoScanType::DUAL_SCAN_LINES) {
			// added 2nd vertical scan line - phreak
			x1 = fl2i(0.5f + right - (t * Cargo_scan_w));
			renderLine(x1, y1 - 3, x1, y2 - 1);
		}
	} else if (Cargo_scan_type == CargoScanType::DISCO_SCAN_LINES) {	
		// by popular demand, this, which was made as a joke, was added - Asteroth
		for (int i = 0; i < 4; i++) {
			float arr[2] = { 1 / 0.75f, 1 / 0.4f };
			float tmod;
			if (i < 2) {
				tmod = powf(t, arr[i]);
			} else {
				tmod = 1 - powf(1 - t, arr[i - 2]);
			}

			if (tmod < 0.5f) {
				y2 = fl2i(0.5f + bot - 2.f * tmod * Cargo_scan_h);
				renderLine(left, bot, right, y2);
			} else {
				x2 = fl2i(0.5f + right - (2.f * tmod - 1) * Cargo_scan_w);
				renderLine(left, bot, x2, top);
			}

			if (tmod < 0.5f) {
				x2 = fl2i(0.5f + right - 2.f * tmod * Cargo_scan_w);
				renderLine(right, bot, x2, top);
			} else {
				y2 = fl2i(0.5f + top + (2.f * tmod - 1) * Cargo_scan_h);
				renderLine(right, bot, left, y2);
			}

			if (tmod < 0.5f) {
				y2 = fl2i(0.5f + top + 2.f * tmod * Cargo_scan_h);
				renderLine(right, top, left, y2);
			} else {
				x2 = fl2i(0.5f + left + (2.f * tmod - 1) * Cargo_scan_w);
				renderLine(right, top, x2, bot);
			}

			if (tmod < 0.5f) {
				x2 = fl2i(0.5f + left + 2.f * tmod * Cargo_scan_w);
				renderLine(left, top, x2, bot);
			} else {
				y2 = fl2i(0.5f + bot - (2.f * tmod - 1) * Cargo_scan_h);
				renderLine(left, top, right, y2);
			}
		}
	}
}

void HudGaugeTargetBox::showTargetData(float  /*frametime*/, bool config)
{
	int x = position[0];
	int y = position[1];
	float scale = 1.0;

	if (config) {
		std::tie(x, y, scale) = hud_config_convert_coord_sys(position[0], position[1], base_w, base_h);
	}

	setGaugeColor(HUD_C_NONE, config);

	float current_target_distance;
	object* target_objp = nullptr;
	if (!config) {
		target_objp = &Objects[Player_ai->target_objnum];

		current_target_distance = Player_ai->current_target_distance;
	} else {
		current_target_distance = 100.0f;
	}

	float displayed_target_distance;
	if ( Hud_unit_multiplier > 0.0f ) {	// use a different displayed distance scale
		displayed_target_distance = current_target_distance * Hud_unit_multiplier;
	} else {
		displayed_target_distance = current_target_distance;
	}

	ship* shipp = nullptr;
	debris* debrisp = nullptr;
	__UNUSED ship_info* sip = nullptr;

	// This switch seems to only be related to the debug stuff and maybe should be moved?
	bool is_ship = false;
	if (!config) {
		switch (Objects[Player_ai->target_objnum].type) {
		case OBJ_SHIP:
			shipp = &Ships[target_objp->instance];
			sip = &Ship_info[shipp->ship_info_index];
			is_ship = true;
			break;

		case OBJ_DEBRIS:
			debrisp = &Debris[target_objp->instance];
			sip = &Ship_info[debrisp->ship_info_index];
			break;

		case OBJ_WEAPON:
			sip = NULL;
			break;

		case OBJ_ASTEROID:
			sip = NULL;
			break;

		case OBJ_JUMP_NODE:
			return;

		default:
			Int3(); // can't happen
			break;
		}
	}

	// print out the target distance and speed
	char outstr[256];
	sprintf(outstr,XSTR( "d: %.0f%s", 350), displayed_target_distance, config ? "m" : modifiers[Player_ai->current_target_dist_trend]);

	hud_num_make_mono(outstr, font_num);

	int w, h;
	gr_get_string_size(&w,&h,outstr, scale);

	renderString(x + fl2i(Dist_offsets[0] * scale), y + fl2i(Dist_offsets[1] * scale), EG_TBOX_DIST, outstr, scale, config);	

	float displayed_target_speed, current_target_speed;
	if (!config) {
		Assertion(target_objp != nullptr, "Target is invalid, get a coder!");

		// 7/28/99 DKA: Do not use vec_mag_quick -- the error is too big
		current_target_speed = vm_vec_mag(&target_objp->phys_info.vel);
		if (current_target_speed < 0.1f) {
			current_target_speed = 0.0f;
		}
		// if the speed is 0, determine if we are docked with something -- if so, get the docked velocity
		if ((current_target_speed == 0.0f) && is_ship) {
			current_target_speed = dock_calc_docked_fspeed(&Objects[shipp->objnum]);

			if (current_target_speed < 0.1f) {
				current_target_speed = 0.0f;
			}
		}
	} else {
		current_target_speed = 50;
	}

	if ( Hud_speed_multiplier > 0.0f ) {	// use a different displayed speed scale
		displayed_target_speed = current_target_speed * Hud_speed_multiplier;
	} else {
		displayed_target_speed = current_target_speed;
	}

	sprintf(outstr,
		XSTR("s: %.0f%s", 351),
		displayed_target_speed,
		config ? "m/s" : ((displayed_target_speed > 1) ? modifiers[Player_ai->current_target_speed_trend] : ""));

	hud_num_make_mono(outstr, font_num);

	renderString(x + static_cast<int>(Speed_offsets[0] * scale), y + static_cast<int>(Speed_offsets[1] * scale), EG_TBOX_SPEED, outstr, scale, config);

	// config does no debug stuff, even in debug
	if (config) {
		return;
	}

	//
	// output target info for debug purposes only, this will be removed later
	//

#ifndef NDEBUG
	//XSTR:OFF
	char outstr2[256];	
	if ( Show_target_debug_info && (is_ship == 1) ) {
		int sx, sy, dy;
		sx = gr_screen.center_offset_x + 5;
		dy = gr_get_font_height() + 1;
		sy = gr_screen.center_offset_y + 300 - 7*dy;

		gr_set_color_fast(&HUD_color_debug);

		if ( shipp->ai_index >= 0 ) {
			ai_info	*aip = &Ai_info[shipp->ai_index];

			sprintf(outstr,"AI: %s",Ai_behavior_names[aip->mode]);

			switch (aip->mode) {
			case AIM_CHASE:
				Assert(aip->submode <= SM_BIG_PARALLEL);	//	Must be <= largest chase submode value.
				sprintf(outstr2," / %s",Submode_text[aip->submode]);
				strcat_s(outstr,outstr2);
				break;
			case AIM_STRAFE:
				Assert(aip->submode <= AIS_STRAFE_POSITION);	//	Must be <= largest chase submode value.
				sprintf(outstr2," / %s",Strafe_submode_text[aip->submode-AIS_STRAFE_ATTACK]);
				strcat_s(outstr,outstr2);
				break;
			case AIM_WAYPOINTS:
				break;
			default:
				break;
			}

			gr_printf_no_resize(sx, sy, "%s", outstr);
			sy += dy;

			gr_printf_no_resize(sx, sy, "Max speed = %d, (%d%%)", (int)target_objp->phys_info.max_vel.xyz.z, (int) (100.0f * vm_vec_mag(&target_objp->phys_info.vel)/target_objp->phys_info.max_vel.xyz.z));
			sy += dy;
			
			// data can be found in target montior
			if (aip->target_objnum != -1) {
				const char *target_str;
				float	dot, dist;
				vec3d	v2t;

				if (aip->target_objnum == OBJ_INDEX(Player_obj))
					target_str = "Player!";
				else
					target_str = Ships[Objects[aip->target_objnum].instance].get_display_name();

				gr_printf_no_resize(sx, sy, "Targ: %s", target_str);
				sy += dy;

				dist = vm_vec_dist_quick(&Objects[Player_ai->target_objnum].pos, &Objects[aip->target_objnum].pos);
				vm_vec_normalized_dir(&v2t,&Objects[aip->target_objnum].pos, &Objects[Player_ai->target_objnum].pos);

				dot = vm_vec_dot(&v2t, &Objects[Player_ai->target_objnum].orient.vec.fvec);

				// data can be found in target monitor
				gr_printf_no_resize(sx, sy, "Targ dot: %3.2f", dot);
				sy += dy;
				gr_printf_no_resize(sx, sy, "Targ dst: %3.2f", dist);
				sy += dy;

				if ( aip->targeted_subsys != NULL ) {
					sprintf(outstr, "Subsys: %s", aip->targeted_subsys->system_info->subobj_name);
					gr_printf_no_resize(sx, sy, "%s", outstr);
				}
				sy += dy;
			}

			// print out energy transfer information on the ship
			sy = gr_screen.center_offset_y + 70;

			sprintf(outstr,"MAX G/E: %.0f/%.0f",shipp->weapon_energy, target_objp->phys_info.max_vel.xyz.z);
			gr_printf_no_resize(sx, sy, "%s", outstr);
			sy += dy;
			 
			sprintf(outstr,"G/S/E: %.2f/%.2f/%.2f",Energy_levels[shipp->weapon_recharge_index],Energy_levels[shipp->shield_recharge_index],Energy_levels[shipp->engine_recharge_index]);
			gr_printf_no_resize(sx, sy, "%s", outstr);
			sy += dy;

			//	Show information about attacker.
			{
				int	found = 0;

				if (Enemy_attacker != NULL)
					if (Enemy_attacker->type == OBJ_SHIP) {
						ship		*eshipp;
						ai_info	*eaip;
						float		dot, dist;
						vec3d	v2t;

						eshipp = &Ships[Enemy_attacker->instance];
						eaip = &Ai_info[eshipp->ai_index];

						if (eaip->target_objnum == OBJ_INDEX(Player_obj)) {
							found = 1;
							dist = vm_vec_dist_quick(&Enemy_attacker->pos, &Player_obj->pos);
							vm_vec_normalized_dir(&v2t,&Objects[eaip->target_objnum].pos, &Enemy_attacker->pos);

							dot = vm_vec_dot(&v2t, &Enemy_attacker->orient.vec.fvec);

							gr_printf_no_resize(sx, sy, "#%i: %s", OBJ_INDEX(Enemy_attacker), Ships[Enemy_attacker->instance].get_display_name());
							sy += dy;
							gr_printf_no_resize(sx, sy, "Targ dist: %5.1f", dist);
							sy += dy;
							gr_printf_no_resize(sx, sy, "Targ dot: %3.2f", dot);
							sy += dy;
						}
					}

				if (Player_ai->target_objnum == Enemy_attacker - Objects)
					found = 0;

				if (!found) {
					int	i;

					Enemy_attacker = NULL;
					for (i=0; i<MAX_OBJECTS; i++)
						if (Objects[i].type == OBJ_SHIP) {
							int	enemy;

							if (i != Player_ai->target_objnum) {
								enemy = Ai_info[Ships[Objects[i].instance].ai_index].target_objnum;

								if (enemy == Player_obj-Objects) {
									Enemy_attacker = &Objects[i];
									break;
								}
							}
						}
				}
			}

			// Show target size
			// hud_target_w
			gr_printf_no_resize(sx, sy, "Targ size: %dx%d", Hud_target_w, Hud_target_h );
			sy += dy;

			polymodel *pm = model_get(sip->model_num);
			gr_printf_no_resize(sx, sy, "POF:%s", pm->filename );
			sy += dy;

			gr_printf_no_resize(sx, sy, "Mass: %.2f\n", pm->mass);
			sy += dy;
		}
	}

	// display the weapons for the target on the HUD.  Include ammo counts.
	if ( Show_target_weapons && (is_ship == 1) ) {
		int sx, sy, dy, i;
		ship_weapon *swp;

		swp = &shipp->weapons;
		sx = gr_screen.center_offset_x + 400;
		sy = gr_screen.center_offset_y + 100;
		dy = gr_get_font_height();

		sprintf(outstr,"Num primaries: %d", swp->num_primary_banks);
		gr_printf_no_resize(sx,sy,"%s", outstr);
		sy += dy;
		for ( i = 0; i < swp->num_primary_banks; i++ ) {
			sprintf(outstr,"%d. %s", i+1, Weapon_info[swp->primary_bank_weapons[i]].get_display_name());
			gr_printf_no_resize(sx,sy,"%s", outstr);
			sy += dy;
		}

		sy += dy;
		sprintf(outstr,"Num secondaries: %d", swp->num_secondary_banks);
		gr_printf_no_resize(sx,sy,"%s", outstr);
		sy += dy;
		for ( i = 0; i < swp->num_secondary_banks; i++ ) {
			sprintf(outstr,"%d. %s", i+1, Weapon_info[swp->secondary_bank_weapons[i]].get_display_name());
			gr_printf_no_resize(sx,sy,"%s", outstr);
			sy += dy;
		}
	}
	//XSTR:ON

#endif
}

/**
 * Called at the start of each level
 */
void hud_init_target_static()
{
	Target_static_next = 0;
	Target_static_playing = 0;
	Sensor_static_forced = false;
}

/**
 * Determine if we should draw static on top of the target box
 */
void hud_update_target_static()
{
	float	sensors_str;

	// on lowest skill level, don't show static on target monitor
	if ( (Game_skill_level == 0) && !Sensor_static_forced ) 
		return;

	// if multiplayer observer, don't show static
	if((Game_mode & GM_MULTIPLAYER) && (Net_player->flags & NETINFO_FLAG_OBSERVER))
		return;

	sensors_str = ship_get_subsystem_strength( Player_ship, SUBSYSTEM_SENSORS );

	if ( ship_subsys_disrupted(Player_ship, SUBSYSTEM_SENSORS) ) {
		sensors_str = SENSOR_STR_TARGET_NO_EFFECTS-1;
	}

	if ( (sensors_str > SENSOR_STR_TARGET_NO_EFFECTS) && !Sensor_static_forced ) {
		Target_static_playing = 0;
		Target_static_next = 0;
	} else {
		if ( Target_static_next == 0 )
			Target_static_next = 1;
	}

	if ( timestamp_elapsed(Target_static_next) ) {
		Target_static_playing ^= 1;
		Target_static_next = timestamp_rand(50, 750);
	}

	if ( Target_static_playing ) {
		if (!Target_static_looping.isValid()) {
			Target_static_looping = snd_play_looping(gamesnd_get_game_sound(GameSounds::STATIC));
		}
	} else {
		if (Target_static_looping.isValid()) {
			snd_stop(Target_static_looping);
			Target_static_looping = sound_handle::invalid();
		}
	}
}

/**
 * Updates the HUD status description of a particular ship
 * 
 * Checks for disabled or ships with disrupted engines, as well as damage levels
 * of the target ship. If status has changed, then the HUD will flash
 *
 * @param targetp Instance of the ship target -- note the targetp->instance cannot be negative
 *
 */
void hud_update_ship_status(object *targetp)
{
    Assert( targetp != NULL );
    Assert( (targetp->instance >= 0) && (targetp->instance < MAX_SHIPS) );
    
    if ( (targetp->instance >= 0) && (targetp->instance < MAX_SHIPS) ) {
    	// print out status of ship for the targetbox
		if ( (Ships[targetp->instance].flags[Ship::Ship_Flags::Disabled]) || (ship_subsys_disrupted(&Ships[targetp->instance], SUBSYSTEM_ENGINE)) ) {
			Current_ts = TS_DIS;
		} else {
			if ( Pl_target_integrity > 0.9 ) {
				Current_ts = TS_OK;
			} else if ( Pl_target_integrity > 0.2 ) {
				Current_ts = TS_DMG;
			} else {
				Current_ts = TS_CRT;
			}
		}

		if ( Last_ts != -1 && Current_ts != Last_ts ) {
			hud_targetbox_start_flash(TBOX_FLASH_STATUS);
		}
    }

	Last_ts = Current_ts;
}

/**
 * Start the targetbox item flashing for duration ms
 *
 * @param index		TBOX_FLASH_ define
 * @param duration	optional param (default value TBOX_FLASH_DURATION), how long to flash in ms
 */
void hud_targetbox_start_flash(int index, int duration)
{
	Targetbox_flash_timers[index] = timestamp(duration);
}

/**
 * Stop flashing a specific targetbox item
 */
void hud_targetbox_end_flash(int index)
{
	Targetbox_flash_timers[index] = timestamp(0);
}

void HudGaugeTargetBox::pageIn()
{
	bm_page_in_aabitmap( Monitor_frame.first_frame, Monitor_frame.num_frames);

	bm_page_in_aabitmap( Integrity_bar.first_frame, Integrity_bar.num_frames );
}
