// BitTube - Makapuf 2014 for Bitbox - GPLv3

/* todo :
	using mouse ?

	win/loose anim / sprite
	intro level / more levels
	credits ?
	loose life vs level (continue ? non, reprend au niveau)

	intro (+into bitbox : sprite logo rond fade in/out, ...)
	bonus all squares
	cursor with the form of the brick (NSEW, 1 color)
	music / sfx
	"hurry","well done" (voices)
	bords qui sautent e/w
	one ways, set flow slow/fast, teleports 

	handle tubes cross (&give points)

	gros bonh as sprite + dead / ok 
	heatshrink tileset / intro sprites vers scratch RAM 32k
*/


#include <stdint.h>
#include <stdlib.h> // rand
#include <math.h> // sin

#include <bitbox.h>
#include <blitter.h>

#include "build/screen3.h"
#include "build/tubes3.h"

// 400x300 is 50x38 tiles on screen. 
#define SCREEN_W 64
#define SCREEN_H 40

// real game grid.
#define GRID_W 10 
#define GRID_H 7

#define FLOW_SPEED 30 // vga frames per flow step
#define FLOW_START 10 // seconds before starting 

#define ANIM_SPEED 16

enum GuyFrames {guy_blink,  guy_cry,  guy_look,  guy_nice,  guy_normal,  guy_urgh};

extern const unsigned char build_cursor_spr[];
extern const unsigned char build_logo_spr[];
extern const unsigned char build_bonh_spr[];
extern const unsigned char build_splash_spr[];
extern const unsigned char build_title_spr[];

extern const uint16_t build_screen3_tset[];
extern const uint8_t build_screen3_tmap[][50*38];
extern const uint8_t build_tubes3_tmap[][3*3];

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

#define NB_LEVELS 1
const int level_time[] = {10};

// ---------------------------------------------------------------------------------------
// Globals
enum GameState game_state = State_Logo;
int time_state; // frame when entering the state

int score; // 
int time;  // time left before end of level (level cleared) expressed in elements
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
char *msg;

// positions of elements (grid, score, .. ) on screen in tiles : defined in TMX as(special tiles on background tilemap / tiles ?), computed on load.
int pos_grid, pos_score, pos_time, pos_next, pos_lives, pos_level, pos_timegraph;


enum SFX {
	SFX_START, SFX_EXPLODE, SFX_PUT, SFX_CANNOTPUT, SFX_START_LIQUID, SFX
};


// ---------------------------------------------------------------------------------------
// Code 


// XXX
void sfx(int sfx_id) {} 


void enter_level(int mylevel, char * mymsg)
// Will in fact put the game in pre level - that will enter the level afterwards
{

	if (mylevel>=NB_LEVELS || mylevel <0) {
		message("Level Error : %d\n",mylevel);
		mylevel = NB_LEVELS-1;		
	}

	if (mylevel==0) {
		// game start
		score=0; lives=3;
	}

	// load background (if needed?)
	tmap_blit(tmap, 0, 0, screen3_header, build_screen3_tmap[screen3_screen]); // unshrink before ? Level ? 
	
	// scan positions of special tiles & load grid
	for (int i=0;i<SCREEN_H*SCREEN_W;i++) {		
		switch (vram[i]) {
			case screen3_time : pos_time=i; break;
			case screen3_grid : pos_grid=i; break;
			case screen3_next : pos_next=i; break;
			case screen3_score : pos_score=i; break;
			case screen3_lives : pos_lives=i; break;
			case screen3_level : pos_level=i; break;
			case screen3_start_cursor : pos_timegraph=i; break;
		}
	}

	// load initial grid from vram, at center of 3x3 tile
	for (int i=0;i<GRID_H;i++)
		for (int j=0;j<GRID_W;j++)
			switch(vram[(i*3+1)*SCREEN_W + j*3+1 + pos_grid]) {
				case screen3_source : grid[i][j]=TUBE_SOURCE; flow_x=i;flow_y=j; break;
				case screen3_block  : grid[i][j]=TUBE_BLOCKED; break;
				default : grid[i][j]=TUBE_EMPTY; break;
			}

	cursor_x=1; cursor_y=0;
	
	flow_frame=-60*FLOW_START/FLOW_SPEED; // vga frames per update
	flow_dir=C_W; // Source connectivity is EW so dir is W
	flow_frame_base=2; // after 2 frames on/off

	sprite_guy->y=215; // show him, wheterver frame he was (win, loose, ...)

	explosion_frame=-1;

	sfx(SFX_START);

	// go to prelevel

	msg=mymsg;
	time_state = vga_frame;
	game_state = State_PreLevel;

	time = level_time[mylevel];


}

void static_level()
{
	// wait for keypress
}


void put_grid( void )
// user put tile on grid (from cursor position)
{
	grid[cursor_y][cursor_x] = next_tubes[4];
	sfx(SFX_PUT);
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
	sprite_guy->fr=guy_cry;
	enter_level(level,"  000\n  TRY  \n\n AGAIN\n  00");
}

void win(void) 
{ 
	// XXX animation on tileset, music
	sprite_guy->fr=guy_nice;
	enter_level(level+1, "  GET\n READY \n\n LEVEL\n  00");
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
				explosion_frame = 0;
				explosion_x = cursor_x;
				explosion_y = cursor_y;

				sfx(SFX_EXPLODE);
				break;

			case TUBE_EMPTY : 
				put_grid(); 
				sfx(SFX_PUT); 
				break;

			default : // other : obstacle, source, tube with liquid .. 
				sfx(SFX_CANNOTPUT);
				break;
		}
	}

	old_gamepad = gamepad_buttons[0];
}

void advance_flow(int x,int y, int dir)
{
	// try to advance to this next tile (can fail, we don't know if connectivity of next tile is ok yet)
	score++;
	time--; 
	if (!time) {
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
			sfx(SFX_START_LIQUID);
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

void put_tile(int pos,int gridelt_id, int frame)
{
	// copy a 3x3 tilemap on screen from 3x3 tilemaps
	for (int i=0;i<3;i++)
		for (int j=0;j<3;j++)
			vram[ pos + i + j*SCREEN_W ] = build_tubes3_tmap[grid_maps[gridelt_id]+frame][j*3+i];
}

void put_number(int pos, int n, int digits)
{
	for (int i=digits; i>0;i--) {
		vram[pos+i-1]=screen3_zero + n%10;
		n/=10;
	}
}

void display(void) 
{
	// grid
	for (int j=0;j<GRID_H;j++)
		for (int i=0;i<GRID_W;i++) { 
			int c=grid[j][i]; 
			int frame=0;
			// animated tiles ?
			if (c==TUBE_BLOCKED) 
				frame=(vga_frame/ANIM_SPEED)%4;
			put_tile(pos_grid+j*3*SCREEN_W + i*3,c,frame); 
		}

	// next pieces
	for (int i=0;i<5;i++)
		put_tile(pos_next+3*i*SCREEN_W, next_tubes[i],0);

	// cursor 
	sprite_cursor->x = (pos_grid%SCREEN_W + cursor_x*3) *8;
	sprite_cursor->y = (pos_grid/SCREEN_W + cursor_y*3) *8;
	// XXX frame	sprite_cursor->fr = (vga_frame/32)%4;


	// numerical values, 1 tile per digit, 0 filled.
	put_number(pos_time, time, 2);
	put_number(pos_score, score,5); 
	put_number(pos_lives, lives, 2);
	put_number(pos_level, level, 2);


	// explosion
	if ( explosion_frame >=0 ) {
		put_tile(pos_grid+explosion_y*3*SCREEN_W + explosion_x*3, TUBE_EXPLOSION,explosion_frame); 
		if (vga_frame%8==0) {
			if (explosion_frame<3) 
				explosion_frame++;
			else 
				explosion_frame = -1;
		} 
	} 

	// flow or source 
	if (flow_frame>=0) {
		put_tile(pos_grid+flow_y*3*SCREEN_W + flow_x*3, grid[flow_y][flow_x],flow_frame_base + flow_frame  ); 

	} else { // negative : still waiting
		// make source blink
		put_tile(pos_grid+flow_y*3*SCREEN_W + flow_x*3, grid[flow_y][flow_x],flow_frame % 2 ? 0 : 1 ); // 0 - 1 
		// update time graph
		int nb = -flow_frame*23/(60*FLOW_START/FLOW_SPEED); // XXX compute 23 from pos_timegraph ...
		for (int i=1;i<nb;i++) 
			vram[pos_timegraph+SCREEN_W*i] = screen3_cursor_time;
		for (int i=nb;i<SCREEN_H;i++) 
			vram[pos_timegraph+SCREEN_W*i] = screen3_cursor_time+1;
	}

	// flow : use grid+flow_frame. as soon as entered, is logically a full one
}

void game_init() {
	tmap = tilemap_new (build_screen3_tset,0,0, TMAP_HEADER(SCREEN_W,SCREEN_H,TSET_8,TMAP_U8), vram);
	sprite_cursor = sprite_new(build_cursor_spr, 0,1000,0);
	sprite_logo = 	sprite_new(build_logo_spr, 25,-1000,0);
	sprite_guy = 	sprite_new(build_bonh_spr, 8,1000,0);
	sprite_splash = sprite_new(build_splash_spr, 50,1000,-1);

	game_state = State_Logo;
}

#define LOGO_TIME (2*60)
#define PRELEVEL_TIME (60*4)
#define SPLASH_TIME (2*60)


void print(char *str)
{
	int id=0,x=0,y=0;
	char c;
	while ((c=*str++)) {
		switch(c) {
			case ' ' : id = tubes3_space;  break;
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
			case '\n' : id=0; x=0; y++;   break;
		}
		if (id) {
			tmap_blit(tmap, 15+x*4,13+y*4, tubes3_header, &build_tubes3_tmap[id][0]);
			x++;
		}
	}
}

void game_frame( void ) 
{ 	
	switch(game_state) {
		case State_Logo : 
			tmap_blit(tmap, 0, 0, screen3_header, build_screen3_tmap[screen3_logo]);
			if (vga_frame<LOGO_TIME)
				sprite_logo->y = (150-sprite_logo->h/2) + cosf(30.*vga_frame/(float)LOGO_TIME) * (LOGO_TIME-vga_frame)*4;
			// splash ! 
			if (vga_frame>=LOGO_TIME*3/2) {
				blitter_remove(sprite_logo);
				sprite_logo = sprite_new(build_title_spr, 0,0,0);

				game_state=State_Intro;
				vga_frame=0;
			}
			// XXX anim pour virer BG et logo en accelerant
			break;

		case State_Intro : 
			// fade in sprite intro using palette
			// then splash !
			// press start 
			if (vga_frame>=SPLASH_TIME) 
				sprite_splash->y=25;
			if (vga_frame>=SPLASH_TIME*2) {
				blitter_remove(sprite_logo);
				sprite_splash->y=1000;
				// Now get to prelevel(message, level=0)
				enter_level(0, "  GET\n READY \n\n LEVEL\n  00");
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
			if (vga_frame-time_state >= PRELEVEL_TIME) {
				game_state=State_Level;
				sprite_guy->fr=guy_normal;
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


