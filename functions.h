#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib> // for itoa function
#include "SDL2/SDL.h"
#include "SDL2/SDL_video.h"
#include "SDL2/SDL_mouse.h"
#include "SDL2/SDL_ttf.h" // font header
#include "SDL2/SDL_image.h" // images and textures
#include "SDL2/SDL_mixer.h"
#include "SDL_Plotter.h"
#include <cmath>

// If you have a SDL folder already, look for SDL_ttf and SDL_image online, and once you unzip them,
// copy the "bin", "include", and "lib" folders over the existing folders. Windows will just add
// the new stuff into the respective folders.

// Also in eclipse, I had to go into the project's Properties and then under
// C/C++ Build, Settings, MinGW C++ Linker, Libraries, I had to add the names
// "SDL2_ttf" and "SDL2_image" and the file path to the "lib" folder.



// CONSTANTS
const double G = 980;
const double FRICTION = 0.85;
const int FPS = 60;
const int TARGET_FRAME_TIME = 1000 / FPS;






// fills in a circle
int SDL_RenderFillCircle(SDL_Renderer * renderer, int x, int y, int radius);



enum TEXT_ALLIGNMENT {CORNER, CENTER};

void customRenderText(SDL_Renderer*, std::string, TTF_Font*, SDL_Color, TEXT_ALLIGNMENT,
		const int, const int, int, int);



// convenient functions
int min(int, int); // returns smaller of two numbers
int max(int, int); // returns larger of two numbers





// mathematical vector struct used for doing complex vector math
struct Vector2d{
	double x, y, depth;
	Vector2d(){
		x = y = depth = 0.0;
	}
	Vector2d(double, double);
	Vector2d operator+(Vector2d);
	Vector2d operator-(Vector2d);
};



// ball object
class Ball{
public:
	double x, y, r, vx, vy;
	Ball();
	// x, y, radius, x-velocity, y-velocity
	Ball(double,double,double,double,double);
};


class BallPickup{
public:
	double x, y, r;
	BallPickup();
	// x, y, radius, x-velocity, y-velocity
	BallPickup(double,double,double);
};



class Particle{
public:
	unsigned int lifetime;
	SDL_Texture* texture;
	double x, y, w, h, vx, vy;
	bool gravity;
	Particle();
	Particle(unsigned int, SDL_Texture*, int, int, int, int);
	void move();
};

// based off of ball, but with no collision used for decoration
class Confetti{
public:
	double x, y, radius, vx, vy;
	int r, g, b;
	Confetti(const int = 0);
	void move();
};


// shapes the ball hits
class Shape{
public:
	Vector2d vertices[10]; // vertices with a max of ten
	double angle; // angle in radians
	int hp; // hitpoints
	int x; // x coordinate of center
	int y; // y coordinate of center
	int sides; // num of sides, equivalent to number of vertices
	int size; // "radius" used for generating shape
	int rotate; // 0 = no rotation, 1 = counterclockwise, 2 = clockwise
	Ball *lastHitBy; // pointer to the last ball it was hit by for the double donk
	Shape();
	Shape(int, int, int, int, int, int);
	void Draw(SDL_Renderer*, TTF_Font*);
private:
	void setVertices(); // sets vertices, also applies rotation
};




#endif


