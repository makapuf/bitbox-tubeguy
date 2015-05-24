// BitTube - Makapuf 2014 for Bitbox - GPLv3

/* todo :
	win/loose anim, intro 
	music / sfx

	one ways, set flow slow/fast, teleports 

	gros bonh as sprite (RLE?)

*/

#include <stdint.h>
#include <stdlib.h> // rand

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

extern const unsigned char build_cursor_spr[];
extern const uint16_t build_screen3_tset[];
extern const uint8_t build_screen3_tmap[][50*38];
extern const uint8_t build_tubes3_tmap[][3*3];


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
#define FIRST_LEVEL 0

// ---------------------------------------------------------------------------------------
// Globals

int score; // 
int time;  // time left before end of level (level cleared) expressed in elements
int lives;

uint8_t vram[SCREEN_W*SCREEN_H]; // graphical tilemap, in RAM.
// the grid is the logical state of the map, made of grid elements. 
// It's not the tilemap, which is its graphical representation (+animation state, borders ...)
uint8_t grid[GRID_H][GRID_W]; 
uint8_t next_tubes[5] = { TUBE_NS, TUBE_NW, TUBE_NE, TUBE_EW, TUBE_SE }; // next incoming tubes, as grid elements.

int level; // or menu 
int cursor_x, cursor_y; // on grid.

int flow_x, flow_y, flow_frame; // current flow position & anim frame if flow frame is <0, not started now
int flow_dir;  // dir : where the flow entered, can be one of C_NSEW. if zero, did not start.  

int explosion_frame, explosion_x, explosion_y; // explosion frame. <0 means none, position on grid
object *tmap, *sprite_cursor;

// positions of elements (grid, score, .. ) on screen in tiles : defined in TMX as(special tiles on background tilemap / tiles ?), computed on load.
int pos_grid, pos_score, pos_time, pos_next, pos_lives, pos_level;

enum SFX {
	SFX_START, SFX_EXPLODE, SFX_PUT, SFX_CANNOTPUT, SFX_START_LIQUID, SFX
};


// ---------------------------------------------------------------------------------------
// Code 


// XXX
void sfx(int sfx_id) {} 


/* todo : game_init, score, lives, snd .. */
void enter_level( int next_level)
{
	if (next_level>=NB_LEVELS) 
		message("Level Error\n");

	if (next_level==FIRST_LEVEL) {
		// game start
		score=0; lives=3;
	}

	// load background (if needed?)
	tmap_blit(tmap, 0, 0, screen3_header, &build_screen3_tmap[next_level][0]); // unshrink before
	
	// scan positions of special tiles & load grid
	for (int i=0;i<SCREEN_H*SCREEN_W;i++) {		
		switch (vram[i]) {
			case screen3_time : pos_time=i; break;
			case screen3_grid : pos_grid=i; break;
			case screen3_next : pos_next=i; break;
			case screen3_score : pos_score=i; break;
			case screen3_lives : pos_lives=i; break;
			case screen3_level : pos_level=i; break;
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


	explosion_frame=-1;

	level = next_level;
	sfx(SFX_START);
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
	enter_level(0); // back to title
}

void win(void) 
{ 
	// XXX animation on tileset, music
	enter_level(level+1); // back to title
}


void user(void) {
	kbd_emulate_gamepad();

	// cursor move
	if (GAMEPAD_PRESSED(0,up)) {
		cursor_y = cursor_y>0 ? cursor_y-1 : GRID_H-1;
	} else if (GAMEPAD_PRESSED(0,down)) {
		cursor_y = cursor_y<GRID_H-1 ? cursor_y+1 : 0;
	}

	if (GAMEPAD_PRESSED(0,left)) {
		cursor_x = cursor_x>0 ? cursor_x-1 : GRID_W-1;
	} else if (GAMEPAD_PRESSED(0,right)) {
		cursor_x = cursor_x<GRID_W-1 ? cursor_x+1 : 0;
	}
	// SFX move ?

	if (GAMEPAD_PRESSED(0,A) && explosion_frame <0 ) {
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
}

void advance_flow(int x,int y, int dir)
{
	// try to advance to this next tile (can fail, we don't know if connectivity of next tile is ok yet)
	score++;
	time--; 
	if (!time) 
		win();

	// search tube 
	int i=0;
	for (;i<NB_CONNECTIVITY;i++)
		if ((grid[y][x] == tube_connectivity[i][0]) && (tube_connectivity[i][1] & dir)) { // concerned ?
				// ok move flow there
				flow_x = x;
				flow_y = y;
				flow_frame = 0;
				flow_dir = dir;				
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
		if (flow_frame<8) {
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
			// animated tiles ?

			put_tile(pos_grid+j*3*SCREEN_W + i*3, grid[j][i],0); 
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

	// debug 
	put_number(pos_score+30, flow_frame>0 ? flow_frame : -flow_frame,3); /// score 

	// animated positionned tiles : explosion, flow, blocks (all), source
	if ( explosion_frame >=0 ) {
		put_tile(pos_grid+explosion_y*3*SCREEN_W + explosion_x*3, TUBE_EXPLOSION,explosion_frame); 
		if (vga_frame%16==0) {
			if (explosion_frame<3) 
				explosion_frame++;
			else 
				explosion_frame = -1;
		} 
	} 

	// flow - or source !
	if (flow_frame>=0) {
		put_tile(pos_grid+flow_y*3*SCREEN_W + flow_x*3, grid[flow_y][flow_x],flow_frame ); // 2-6 pour source, 0-8-15 pour le reste selon sens ..
	} else {
		put_tile(pos_grid+flow_y*3*SCREEN_W + flow_x*3, grid[flow_y][flow_x],flow_frame % 2 ? 0 : 1 ); // 0 - 1 
	}
	

	// flow : use grid+flow_frame. as soon as entered, is logically a full one
	
}

void game_init() {
	tmap = tilemap_new (build_screen3_tset,0,0, TMAP_HEADER(SCREEN_W,SCREEN_H,TSET_8,TMAP_U8), vram);
	sprite_cursor = sprite_new(build_cursor_spr, 0,1000,0);
	enter_level(0);
}


void game_frame( void ) 
{ 	
	// pause ?
	if (0) {
		display();
		static_level();
	} else { 
		// real level
		if ((vga_frame & 3 ) == 0 ) user();
		if ((vga_frame % FLOW_SPEED ) == 0 ) flow();
		display(); 
	}
}


