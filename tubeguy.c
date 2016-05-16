// BitTube - Makapuf 2015 for Bitbox - GPLv3

/* todo :
	allow using mouse ?

	change guy if next tube pos is not ready !

	win/loose anim / sprite
	intro level / more levels

	score !

	bonus X

	cursor with the form of the brick (NSEW, glowing - made of 3 subs ?)

	music / sfx
	"hurry" (voices)
	bords qui sautent e/w
	one ways, set flow slow/fast, teleports

	handle tubes cross (&give points)

*/


#include <stdint.h>
#include <stdlib.h> // rand
#include <math.h> // sin
#include <string.h>

#include <bitbox.h>
#include <blitter.h>

#include "sampler.h" // uses sampler

#include "build/screen3.h"
#include "build/tubes3.h"
#include "build/levels.h"

#include "build/binaries.h"

// 400x300 is 50x38 tiles on screen.
#define SCREEN_W 64
#define SCREEN_H 38

// real game grid.
#define GRID_W 10
#define GRID_H 7

#define FLOW_SPEED 30 // vga frames per flow step
#define FLOW_START 10 // seconds before starting flow

#define ANIM_SPEED 16

enum GuyFrames {guy_blink,  guy_cry,  guy_look,  guy_nice,  guy_normal,  guy_urgh};

enum GameState { State_Logo, State_Intro, State_Level, State_PreLevel, State_PostLevel, State_Pause };

enum Directions {C_E=1, C_W=2, C_S=4, C_N=8};


/* possible elements on the grid.
the grid is an array of logical elements (which will be made of several tiles)

 */
enum GridElt {
	TUBE_EMPTY,
	TUBE_EXPLOSION,
	TUBE_BLOCKED, // prevents putting pipes there. Globally animated.

	// Can be filled. Fulls : add +FULL to the IDs
	TUBE_SOURCE,
	TUBE_NS,
	TUBE_NW,
	TUBE_NE,
	TUBE_EW,
	TUBE_SE,
	TUBE_SW,

	TUBE_CROSS, // empty cross
	TUBE_CROSS_HORIZONTAL, // cross _already filled vertically_
	TUBE_CROSS_VERTICAL,
	TUBE_EW_FIXED, // horizontal fixed (cannot be destroyed)
	TUBE_NS_FIXED, // vertical fixed (cannot be destroyed)

};
#define FULL 16 // add this to the tile ID to mean "full", by example TUBE_EW+FULL is horizontal full. Cross is special cased

// grid_elt to grid_map
// first map if animated
const uint8_t grid_maps[] = {
	tubes3_empty,
	tubes3_explo,
	tubes3_blocked,

	tubes3_source,
	tubes3_ns,
	tubes3_nw,
	tubes3_ne,
	tubes3_ew,
	tubes3_se,
	tubes3_sw,

	tubes3_cross,
	tubes3_cross_hori,
	tubes3_cross_verti,
	tubes3_ew_fixed,
	tubes3_ns_fixed,
	// tiles +16 are FULL
	0,
	0,
	0,
	0,
	tubes3_source8,
	tubes3_ns8,
	tubes3_nw8,
	tubes3_ne8,
	tubes3_ew8,
	tubes3_se8,
	tubes3_sw8,
	tubes3_cross_hori8,
};


// connectivity vectors
#define NB_CONNECTIVITY 13
const uint8_t tube_connectivity[NB_CONNECTIVITY][3] = {
	// gridelt id , connectivity, grid element when full
	{TUBE_SOURCE, C_W|C_E, TUBE_SOURCE+FULL},
	{TUBE_NS, C_N|C_S, TUBE_NS+FULL},
	{TUBE_NS_FIXED, C_N|C_S, TUBE_NS+FULL}, // cannot be (re)moved but is like a NS
	{TUBE_EW, C_E|C_W, TUBE_EW+FULL},
	{TUBE_EW_FIXED, C_E|C_W, TUBE_EW+FULL },
	{TUBE_SE, C_S|C_E, TUBE_SE+FULL},
	{TUBE_SW, C_S|C_W, TUBE_SW+FULL},
	{TUBE_NW, C_N|C_W, TUBE_NW+FULL},
	{TUBE_NE, C_N|C_E, TUBE_NE+FULL},
	// Cross : special case
	{TUBE_CROSS, C_N|C_S, TUBE_CROSS_VERTICAL},
	{TUBE_CROSS, C_W|C_E, TUBE_CROSS_HORIZONTAL},
	{TUBE_CROSS_VERTICAL, C_S|C_N, TUBE_CROSS+FULL},
	{TUBE_CROSS_HORIZONTAL, C_W|C_E, TUBE_CROSS+FULL},
};

#define NB_LEVELS 4
const int level_length[] = {10,15,15,15};

// play a sample at 16kHz
#define PLAY(sndfile) play_sample((const int8_t *)build_##sndfile##_snd,build_##sndfile##_snd_len,256*11025/BITBOX_SAMPLERATE, -1, 120,120)

// ---------------------------------------------------------------------------------------
// Globals
enum GameState game_state = State_Logo;
int time_state; // frame when entering the state

int score;
int flow_length;  // flow_length left before end of level (level cleared) expressed in elements
int lives;

uint8_t vram[SCREEN_W*SCREEN_H]; // graphical tilemap, in RAM.
// the grid is the logical state of the map, made of grid elements.
// It's not the tilemap, which is its graphical representation (+animation state, borders ...)
uint8_t grid[GRID_H][GRID_W];
uint8_t next_tubes[5] = { TUBE_SW, TUBE_NW, TUBE_NE, TUBE_EW, TUBE_SE }; // next incoming tubes, as grid elements.

int level; // or menu
int cursor_x, cursor_y; // on grid.

int flow_x, flow_y, flow_frame; // current flow position & anim frame if flow frame is <0, not started now
int flow_dir;  // dir : where the flow entered, can be one of C_NSEW. if zero, did not start.
int flow_frame_base; // in the current flow steps, which is the one (depends on direction)

int explosion_frame, explosion_x, explosion_y; // explosion frame. <0 means none, position on grid
object *tmap, *sprite_cursor, *sprite_logo, *sprite_guy, *sprite_splash;
char msg[50];

// positions of elements (grid, score, .. ) on screen in tiles : defined in TMX as(special tiles on background tilemap / tiles ?), computed on load.
int pos_grid, pos_score, pos_time, pos_next, pos_lives, pos_level, pos_timegraph;



extern const int track_piste_1_1_len;
extern const struct NoteEvent track_piste_1_1[];

// ---------------------------------------------------------------------------------------
// Code


void enter_level(int mylevel, char * mymsg)
// Will in fact put the game in pre level - that will enter the level afterwards

{

	if (mylevel>=NB_LEVELS || mylevel <0) {
		message("Level Error : %d\n",mylevel);
		level = NB_LEVELS-1;
	} else {
		level=mylevel;
	}

	if (level==0) {
		// game start
		score=0; lives=3;
	}

	// load background (if needed?)
	tmap_blitlayer(tmap, 0, 0, screen3_header, build_screen3_tmap,screen3_screen);

	// scan positions of special tiles & load grid
	for (int i=0;i<SCREEN_H*SCREEN_W;i++) {
		switch (vram[i]) {
			case screen3_time  : pos_time=i; break;
			case screen3_grid  : pos_grid=i; break;
			case screen3_next  : pos_next=i; break;
			case screen3_score : pos_score=i; break;
			case screen3_lives : pos_lives=i; break;
			case screen3_level : pos_level=i; break;
			case screen3_start_cursor : pos_timegraph=i; break;
		}
	}

	// load initial grid from level, at center of 3x3 tile
	for (int i=0;i<GRID_H;i++) {
		for (int j=0;j<GRID_W;j++) {
			switch(build_levels_tmap[(level*GRID_H+i)*GRID_W+j]) {
				case screen3_source :
					grid[i][j]=TUBE_SOURCE;
					flow_x=j;flow_y=i;
					break;
				case screen3_block  :
					grid[i][j]=TUBE_BLOCKED;
					break;
				// XXX add screen3_fixed_ns &co
				default :
					grid[i][j]=TUBE_EMPTY;
					break;
			}
		}
	}

	cursor_x=1; cursor_y=0;

	flow_frame=-60*FLOW_START/FLOW_SPEED; // vga frames per update
	flow_dir=C_W; // Source connectivity is EW so dir is W
	flow_frame_base=2; // after 2 frames on/off

	sprite_guy->y=14; // show him, wheterver frame he was (win, loose, ...)

	explosion_frame=-1;

	strcpy(msg,mymsg);
	strcat(msg,"\n  00");
	int n=strlen(msg);
	msg[n-1]+=level%10;
	msg[n-2]+=level/10;


	time_state = vga_frame;
	game_state = State_PreLevel;

	flow_length = level_length[level];


}



void put_grid( void )
// user put tile on grid (from cursor position)
{
	grid[cursor_y][cursor_x] = next_tubes[4];
	PLAY(clang);
	for (int i=3;i>=0;i--)
		next_tubes[i+1] = next_tubes[i];
	// create new one
	next_tubes[0] = (rand()%7) + TUBE_NS ;
}

void loose(void)
{
	// XXX animation on tileset, music
	// check lives, continues.
	// if not lives, back to title
	PLAY(splat);

	sprite_guy->fr=guy_cry;
	sprite_splash->y=25;
	enter_level(level,"  000\n  TRY  \n\n AGAIN");
}

void win(void)
{
	// XXX animation on tileset
	PLAY(welldone);
	sprite_guy->fr=guy_nice;
	enter_level(level+1, "  GET\n READY \n\n LEVEL");
}

// still pressed or just pressed
#define BUTTON(but) ((vga_frame-moved>=16 && gamepad_buttons[0] & gamepad_##but) || \
			        (vga_frame != moved   && gamepad_pressed    & gamepad_##but))


void user(void) {
	static uint16_t old_gamepad;
	uint16_t gamepad_pressed;
	static int moved;

	kbd_emulate_gamepad();

	gamepad_pressed = gamepad_buttons[0] & ~old_gamepad;

	// cursor move
	if (BUTTON(up) && cursor_y>0) {
		moved=vga_frame;
		cursor_y--;
	} else if (BUTTON(down) && cursor_y<GRID_H-1) {
		moved=vga_frame;
		cursor_y++;
	}

	if (BUTTON(left) && cursor_x>0) {
		moved=vga_frame;
		cursor_x--;
	} else if (BUTTON(right) && cursor_x<GRID_W-1) {
		moved=vga_frame;
		cursor_x++;
	}
	// SFX move ?

	// can put a new tube if pressed buttons and not on explosion nor flow.
	if ( gamepad_pressed & gamepad_A && explosion_frame <0 && !(cursor_x==flow_x && cursor_y==flow_y)) {
		switch (grid[cursor_y][cursor_x]) {

			// existing, empty pipes.
			case TUBE_NS :
			case TUBE_EW :
			case TUBE_SE :
			case TUBE_SW :
			case TUBE_NW :
			case TUBE_NE :
			case TUBE_CROSS :
				put_grid(); // put element on grid

				// start explosion
				PLAY(explosion);
				explosion_frame = 0;
				explosion_x = cursor_x;
				explosion_y = cursor_y;

				break;

			case TUBE_EMPTY :
				put_grid();
				break;

			default : // other : obstacle, source, tube with liquid ..
				break;
		}
	}

	old_gamepad = gamepad_buttons[0];
}

void advance_flow(int x,int y, int dir)
{
	// try to advance to this next tile (can fail, we don't know if connectivity of next tile is ok yet)
	score++;
	flow_length--;
	if (!flow_length) {
		win();
		return;
	}

	// search tube
	int i=0;
	for (;i<NB_CONNECTIVITY;i++)
		if ((grid[y][x] == tube_connectivity[i][0]) && (tube_connectivity[i][1] & dir)) { // concerned ?
				// ok move flow there
				flow_x = x;
				flow_y = y;
				flow_frame = 0;
				flow_dir = dir;
				// if first of the two letters, 1st frames, else 8 last ones.
				flow_frame_base = dir < (tube_connectivity[i][1]^dir) ? 1 : 8;
				break;
			}

	if (i==NB_CONNECTIVITY) // did not find it ie the current tile
		loose();
}

void flow(void)
// make the liquid flow
{
	if (flow_frame<0) {
		flow_frame++; // not now
		if (flow_frame==0) // just started flowing now
			// init flow from source (search in grid)
			PLAY(tada);
	}
	else {
		if (flow_frame<6) {
			flow_frame++; // just animate
		} else {
			// finish this tile
			uint8_t out_dir=0; // default = did not find an output
			for (int i=0;i<NB_CONNECTIVITY;i++)
				if ((grid[flow_y][flow_x] == tube_connectivity[i][0]) && (tube_connectivity[i][1] & flow_dir)) {
					grid[flow_y][flow_x] = tube_connectivity[i][2];
					out_dir = tube_connectivity[i][1]^flow_dir;
					break;
				}

			switch (out_dir) {
				case 0 : // didnt find anything
					loose();
					break;
				case C_N :
					if (flow_y>0)
						advance_flow(flow_x, flow_y-1, C_S);
					else
						loose();
					break;
				case C_S :
					if (flow_y<7)
						advance_flow(flow_x, flow_y+1, C_N);
					else
						loose();
					break;
				case C_W :
					if (flow_x>0)
						advance_flow(flow_x-1, flow_y, C_E);
					else
						loose();
					break;
				case C_E :
					if (flow_x<9)
						advance_flow(flow_x+1, flow_y, C_W);
					else
						loose();
					break;
			}

		}
	}

}

void put_tile(int x,int y ,int gridelt_id, int frame)
{
	tmap_blitlayer(tmap,x,y,tubes3_header,build_tubes3_tmap,grid_maps[gridelt_id]+frame);
	// copy a 3x3 tilemap on screen from 3x3 tilemaps
	/*
	for (int i=0;i<3;i++)
		for (int j=0;j<3;j++)
			vram[ pos + i + j*SCREEN_W ] = build_tubes3_tmap[grid_maps[gridelt_id]+frame][j*3+i];
			*/
}

void put_number(int pos, int n, int digits)
{
	for (int i=digits; i>0;i--) {
		vram[pos+i-1]=screen3_zero + n%10;
		n/=10;
	}
}
const uint8_t cursor_frames[8] = {0,1,2,3,3,2,1,0};
void display(void)
{
	int pos_gridx = pos_grid%SCREEN_W;
	int pos_gridy = pos_grid/SCREEN_W;

	// grid
	for (int j=0;j<GRID_H;j++)
		for (int i=0;i<GRID_W;i++) {
			int c=grid[j][i];
			int frame=0;
			// animated tiles ?
			if (c==TUBE_BLOCKED)
				frame=(vga_frame/ANIM_SPEED)%4;
			put_tile(pos_gridx+i*3,pos_gridy+j*3,c,frame);
		}

	// next pieces
	for (int i=0;i<5;i++)
		put_tile(pos_next%SCREEN_W,pos_next/SCREEN_W+3*i, next_tubes[i],0);

	// cursor
	sprite_cursor->x = (pos_gridx + cursor_x*3) *8;
	sprite_cursor->y = (pos_gridy + cursor_y*3) *8;
	sprite_cursor->fr = cursor_frames[(vga_frame/8)%8];


	// numerical values, 1 tile per digit, 0 filled.
	put_number(pos_time,  flow_length,  2);
	put_number(pos_score, score, 5);
	put_number(pos_lives, lives, 2);
	put_number(pos_level, level, 2);


	// explosion
	if ( explosion_frame >=0 ) {
		put_tile(pos_gridx+explosion_x*3,pos_gridy+explosion_y*3, TUBE_EXPLOSION,explosion_frame);
		if (vga_frame%8==0) {
			if (explosion_frame<3)
				explosion_frame++;
			else
				explosion_frame = -1;
		}
	}

	// flow or source
	if (flow_frame>=0) {
		put_tile(pos_gridx+ flow_x*3,pos_gridy+ flow_y*3, grid[flow_y][flow_x],flow_frame_base + flow_frame  );

	} else { // negative : still waiting
		// make source blink
		put_tile(pos_gridx+ flow_x*3, pos_gridy+flow_y*3, grid[flow_y][flow_x],flow_frame % 2 ? 0 : 1 );

		// update time graph
		const int nblines = SCREEN_H - pos_timegraph/SCREEN_W;
		int nb = -flow_frame*nblines/(60*FLOW_START/FLOW_SPEED);
		for (int i=1;i<nb-2;i++)
			vram[pos_timegraph+SCREEN_W*i] = screen3_cursor_time;
		for (int i=nb;i<nblines;i++) {
			vram[pos_timegraph+SCREEN_W*i] = screen3_cursor_time+1;
		}
	}

	// guy face
	// if normal, from time to time, blink or look at grid
	switch (sprite_guy->fr) {
		case guy_normal :
			if (vga_frame%64==0 && rand()%3==0)
				sprite_guy->fr = rand()%2 ? guy_look : guy_blink;
			break;
		case guy_look:
		case guy_blink:
			if (vga_frame % 32 == 0)
				sprite_guy->fr = guy_normal;
			break;
	}

	// flow : use grid+flow_frame. as soon as entered, is logically a full one
}

void game_init() {
	tmap = tilemap_new ((uint16_t*)build_screen3_tset,0,0, TMAP_HEADER(SCREEN_W,SCREEN_H,TSET_8,TMAP_U8), vram);
	sprite_cursor = sprite_new(build_cursor_spr, 0,1000,0);
	sprite_logo = 	sprite_new(build_logo_spr, 25,-1000,0);
	sprite_guy = 	sprite_new(build_bonh_spr, 61,1000,0);
	sprite_splash = sprite_new(build_splash_spr, 50,1000,-1);

	game_state = State_Logo;
}

#define LOGO_TIME (2*60)
#define PRELEVEL_TIME (60*6)
#define SPLASH_TIME (2*60)


void print(char *str)
{
	int id=0,x=0,y=0;
	char c;
	while ((c=*str++)) {
		switch(c) {
			case '0' : id = tubes3_number0;break;
			case '1' : id = tubes3_number1;break;
			case '2' : id = tubes3_number2;break;
			case '3' : id = tubes3_number3;break;
			case '4' : id = tubes3_number4;break;
			case '5' : id = tubes3_number5;break;
			case '6' : id = tubes3_number6;break;
			case '7' : id = tubes3_number7;break;
			case '8' : id = tubes3_number8;break;
			case '9' : id = tubes3_number9;break;
			case 'A' : id = tubes3_letterA;break;
			case 'D' : id = tubes3_letterD;break;
			case 'I' : id = tubes3_letterI;break;
			case 'E' : id = tubes3_letterE;break;
			case 'L' : id = tubes3_letterL;break;
			case 'N' : id = tubes3_letterN;break;
			case 'G' : id = tubes3_letterG;break;
			case 'R' : id = tubes3_letterR;break;
			case 'T' : id = tubes3_letterT;break;
			case 'V' : id = tubes3_letterV;break;
			case 'Y' : id = tubes3_letterY;break;
			case '\n': id=0; x=0; y++;   break;
			case ' ' : id = 0;x++;  break;
		}
		if (id) {
			tmap_blitlayer(tmap, 15+x*4,13+y*4, tubes3_header, build_tubes3_tmap,id);
			x++;
		}
	}
}

void game_frame( void )
{
	switch(game_state) {
		case State_Logo :
			tmap_blitlayer(tmap, 0, 0, screen3_header, build_screen3_tmap,screen3_logo);
			if (vga_frame<LOGO_TIME)
				sprite_logo->y = (150-sprite_logo->h/2) + cosf(30.*vga_frame/(float)LOGO_TIME) * (LOGO_TIME-vga_frame)*4;
			// logo !
			if (vga_frame>=LOGO_TIME*3/2) {
				blitter_remove(sprite_logo);
				sprite_logo = sprite_new(build_title_spr, 0,0,0);

				game_state=State_Intro;
				vga_frame=0;
			}
			// XXX anim pour virer BG et logo en accelerant
			break;

		case State_Intro :
			if (vga_frame==SPLASH_TIME) {
				sprite_splash->y=25;
				PLAY(splat);
			}
			if (vga_frame>=SPLASH_TIME*2) {
				blitter_remove(sprite_logo);
				sprite_splash->y=1000;
				// Now get to prelevel(message, level=0)
				enter_level(0, "  GET\n READY \n\n LEVEL");
			}
			break;

		case State_PreLevel :
			display();
			// blink get ready, level + level
			if (vga_frame & (1<<5)) {
				print (msg);
			} else {
				print ("     \n       \n\n      \n    ");
			}
			if (vga_frame-time_state >= PRELEVEL_TIME/3)
				sprite_splash->y+=2; // hide splash

			if (vga_frame-time_state >= PRELEVEL_TIME) { // now really start the game
				game_state=State_Level;
				sprite_guy->fr=guy_normal;

				PLAY(tada);

				play_track (track_piste_1_1_len, 140, track_piste_1_1,
					(const int8_t *)build_clang_snd, -1,
					build_clang_snd_len,8000);

			}

			break;

		case State_Level :
			if ((vga_frame & 3 ) == 0 ) user();
			if ((vga_frame % FLOW_SPEED ) == 0 ) flow();
			display();
			break;


		default :
			break;
	}
}

