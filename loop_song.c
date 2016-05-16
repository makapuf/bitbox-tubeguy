#include "sampler.h"
// file  sounds/loop.mid
// 2 tracks, 960 ticks/beat (PPQ)
// Track name : intro-Piste_1-1
// Track name : Piste_1-1
struct NoteEvent track_piste_1_1[] = {
    {.tick=   0, .note=0x3c, .vel=0x40}, // ch 1 : note=60 C6 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=  12, .note=0x3c, .vel=0}, // note off 
    {.tick=   0, .note=0x81, .vel=0}, // note off 
    {.tick=   0, .note=0x25, .vel=0x2f}, // ch 1 : note=37 C#4 velocity=47
    {.tick=   3, .note=0x25, .vel=0}, // note off 
    {.tick=   3, .note=0x25, .vel=0x2f}, // ch 1 : note=37 C#4 velocity=47
    {.tick=   3, .note=0x24, .vel=0}, // note off 
    {.tick=   0, .note=0x81, .vel=0}, // note off 
    {.tick=   3, .note=0x40, .vel=0x2f}, // ch 1 : note=64 E6 velocity=47
    {.tick=   0, .note=0x91, .vel=0x2e}, // ch 1 : note=145 C#13 velocity=46
    {.tick=  12, .note=0x40, .vel=0}, // note off 
    {.tick=   0, .note=0x81, .vel=0}, // note off 
    {.tick=   0, .note=0x48, .vel=0x40}, // ch 1 : note=72 C7 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=  12, .note=0x48, .vel=0}, // note off 
    {.tick=  12, .note=0x81, .vel=0}, // note off 
    {.tick=   0, .note=0x48, .vel=0x40}, // ch 1 : note=72 C7 velocity=64
    {.tick=   9, .note=0x48, .vel=0}, // note off 
    {.tick=   3, .note=0x43, .vel=0x40}, // ch 1 : note=67 G6 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=   9, .note=0x43, .vel=0}, // note off 
    {.tick=   3, .note=0x45, .vel=0x40}, // ch 1 : note=69 A6 velocity=64
    {.tick=   9, .note=0x45, .vel=0}, // note off 
    {.tick=   3, .note=0x81, .vel=0}, // note off 
    {.tick=   0, .note=0x40, .vel=0x40}, // ch 1 : note=64 E6 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=  12, .note=0x40, .vel=0}, // note off 
    {.tick=   0, .note=0x81, .vel=0}, // note off 
    {.tick=  12, .note=0x81, .vel=0}, // note off 
    {.tick=   0, .note=0x40, .vel=0x40}, // ch 1 : note=64 E6 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=  12, .note=0x40, .vel=0}, // note off 
    {.tick=   0, .note=0x81, .vel=0}, // note off 
    {.tick=   0, .note=0x48, .vel=0x40}, // ch 1 : note=72 C7 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=  12, .note=0x48, .vel=0}, // note off 
    {.tick=  12, .note=0x48, .vel=0x40}, // ch 1 : note=72 C7 velocity=64
    {.tick=   3, .note=0x24, .vel=0}, // note off 
    {.tick=   6, .note=0x81, .vel=0}, // note off 
    {.tick=   3, .note=0x43, .vel=0x40}, // ch 1 : note=67 G6 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=   9, .note=0x43, .vel=0}, // note off 
    {.tick=   3, .note=0x45, .vel=0x40}, // ch 1 : note=69 A6 velocity=64
    {.tick=   9, .note=0x45, .vel=0}, // note off 
    {.tick=   3, .note=0x81, .vel=0}, // note off 
    {.tick=   0, .note=0x43, .vel=0x40}, // ch 1 : note=67 G6 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=  12, .note=0x43, .vel=0}, // note off 
    {.tick=   0, .note=0x81, .vel=0}, // note off 
    {.tick=  12, .note=0x81, .vel=0}, // note off 
    {.tick=   0, .note=0x43, .vel=0x1f}, // ch 1 : note=67 G6 velocity=31
    {.tick=   0, .note=0x91, .vel=0x34}, // ch 1 : note=145 C#13 velocity=52
    {.tick=  12, .note=0x43, .vel=0}, // note off 
    {.tick=   0, .note=0x81, .vel=0}, // note off 
    {.tick=   0, .note=0x4b, .vel=0x40}, // ch 1 : note=75 D#7 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=  12, .note=0x4b, .vel=0}, // note off 
    {.tick=  12, .note=0x81, .vel=0}, // note off 
    {.tick=   0, .note=0x4b, .vel=0x40}, // ch 1 : note=75 D#7 velocity=64
    {.tick=   9, .note=0x4b, .vel=0}, // note off 
    {.tick=   3, .note=0x46, .vel=0x40}, // ch 1 : note=70 A#6 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=   9, .note=0x46, .vel=0}, // note off 
    {.tick=  15, .note=0x81, .vel=0}, // note off 
    {.tick=   0, .note=0x43, .vel=0x40}, // ch 1 : note=67 G6 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=  12, .note=0x43, .vel=0}, // note off 
    {.tick=   0, .note=0x81, .vel=0}, // note off 
    {.tick=  12, .note=0x81, .vel=0}, // note off 
    {.tick=   0, .note=0x43, .vel=0x40}, // ch 1 : note=67 G6 velocity=64
    {.tick=  12, .note=0x43, .vel=0}, // note off 
    {.tick=   0, .note=0x4b, .vel=0x40}, // ch 1 : note=75 D#7 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=  12, .note=0x4b, .vel=0}, // note off 
    {.tick=  12, .note=0x4b, .vel=0x40}, // ch 1 : note=75 D#7 velocity=64
    {.tick=   3, .note=0x28, .vel=0}, // note off 
    {.tick=   6, .note=0x81, .vel=0}, // note off 
    {.tick=   3, .note=0x46, .vel=0x40}, // ch 1 : note=70 A#6 velocity=64
    {.tick=   0, .note=0x91, .vel=0x40}, // ch 1 : note=145 C#13 velocity=64
    {.tick=   9, .note=0x46, .vel=0}, // note off 
    {.tick=   3, .note=0x48, .vel=0x40}, // ch 1 : note=72 C7 velocity=64
    {.tick=   9, .note=0x48, .vel=0}, // note off 
    {.tick=   3, .note=0x81, .vel=0}, // note off 
};
const int track_piste_1_1_len = 88; // 0 kb
// format 1,ntracks 2,resolution 960, 88 events
