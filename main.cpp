#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // for sleep
#include <fstream>

#include "functions.h"
using namespace std;


bool game_running = true;
bool game_over = false; // this is so the game can keep running in the background while the score screen plays
int since_last_frame = 0;
int WINDOW_H = 800;
int WINDOW_W = WINDOW_H * 4 / 7;
int score = 0;
int wave = 1;
int player_balls_max = 2;
int player_balls_count = player_balls_max;

// texture declaration
SDL_Texture *img_seymour = NULL;
SDL_Texture *img_doubledonk = NULL;


// sfx declaration (initialized in WINMAIN)
Mix_Chunk *sfx_doubledonk = NULL;
Mix_Chunk *sfx_horn = NULL;




vector <Ball> balls; // holds pointers for all balls for reference
vector <Shape> shapes; // holds pointers for all shapes for reference
vector <BallPickup> ballPickups; // holds pointers for all particles for reference
vector <Particle> particles; // holds pointers for all particles for reference
vector <Confetti> confettis; // help me


// normalizes the vector, i.e. giving it a magnitude of 1, I think

Vector2d normalize(const Vector2d &axis){
	Vector2d out = axis;
	double magnitude = sqrt(pow(axis.x,2) + pow(axis.y,2));
	if (magnitude != 0){
		out.x /= magnitude;
		out.y /= magnitude;
	}
	return out;
}


// with a circle, you use the vector pointing from the nearest vertex on the shape to the center of the circle instead

int FindClosestPointOnPolygon(Ball &ball, Shape &shape){
	int result = -1;
	double minDistance = 2000000;

	for(int i = 0; i < shape.sides; i++){
		Vector2d v = shape.vertices[i];
		double distance = sqrt(pow(v.x - ball.x, 2) + pow(v.y - ball.y, 2));

		if(distance < minDistance){
			minDistance = distance;
			result = i;
		}
		delete &v;
	}

	return result;
}

// projects the circle's points onto the axis

void ProjectCircle(Ball &ball, Vector2d axis, double &min, double &max){
	Vector2d directionAndRadius = {axis.x * ball.r,axis.y * ball.r};

	Vector2d p1;
	p1.x = ball.x + directionAndRadius.x;
	p1.y = ball.y + directionAndRadius.y;
	Vector2d p2;
	p2.x = ball.x - directionAndRadius.x;
	p2.y = ball.y - directionAndRadius.y;

	min = p1.x * axis.x + p1.y * axis.y;
	max = p2.x * axis.x + p2.y * axis.y;

	if(min > max){
		// swap the min and max values.
		double tmp = min;
		min = max;
		max = tmp;
	}
}

// projects the polygon's vertices onto the axis

void ProjectVertices(Shape &shape, Vector2d axis, double &min, double &max){
	min = 2000000;
	max = -2000000;

	for(int i = 0; i < shape.sides; i++){
		Vector2d v = shape.vertices[i];
		double proj = v.x * axis.x + v.y * axis.y;
		if(proj < min) { min = proj; }
		if(proj > max) { max = proj; }
		delete &v;
	}
}


// this logic uses Seperating Axis Theorem
// the logic is that you take a line perpendicular to each surface and "project" each vertex onto it and see
// if there's a gap. Do this for ever vertex on each shape and if there is not a single gap, they must be intersecting

bool isIntersecting(Ball &ball, Shape &shape){
	double depth = 2000000;
	Vector2d axis = {0,0};
	double axisDepth = 0;
	double minA, maxA, minB, maxB;
	Vector2d normal = {0,0};

	for (int i = 0; i < shape.sides; i++){
		Vector2d va = shape.vertices[i];
		Vector2d vb = shape.vertices[(i + 1) % shape.sides]; // this logic just wraps back to 0 when the index gets higher than the number of vertices

		Vector2d edge = vb - va;
		axis = {edge.y, -edge.x};
		axis = normalize(axis);

		ProjectVertices(shape, axis, minA, maxA);
		ProjectCircle(ball, axis, minB, maxB);


		if (minA >= maxB || minB >= maxA){
			return false;
		}

		axisDepth = (maxB - minA > maxA - minB) ? maxA - minB: maxB - minA;

		if (axisDepth < depth){
			depth = axisDepth;
			normal = axis;
		}
		delete &va;
		delete &vb;
	}

	int cpIndex = FindClosestPointOnPolygon(ball, shape);
	Vector2d cp = shape.vertices[cpIndex];

	axis.x = cp.x - ball.x;
	axis.y = cp.y - ball.y;
	axis = normalize(axis);

	ProjectVertices(shape, axis, minA, maxA);
	ProjectCircle(ball, axis, minB, maxB);

	if (minA >= maxB || minB >= maxA){
		return false;
	}

	axisDepth = (maxB - minA > maxA - minB) ? maxA - minB: maxB - minA;

	if (axisDepth < depth){
		depth = axisDepth;
		normal = axis;
	}

	Vector2d direction;
	direction.x = shape.x - ball.x;
	direction.y = shape.y - ball.y;

	if (direction.x * normal.x + direction.y * normal.y < 0){
		normal.x *= -1;
		normal.y *= -1;
	}

	delete &cp;

	return true;
}





// get the index of the two vertices of the sides closest to the ball
void getEdgeOfCollision(Ball &ball, Shape &shape, int &index1, int &index2){
	double minDist1 = 2000000;
	double minDist2 = 2000000;

	for(int i = 0; i < shape.sides; i++){
		Vector2d v = shape.vertices[i];
		double distance = sqrt(pow(v.x - ball.x, 2) + pow(v.y - ball.y, 2));

		if(distance < minDist2){
			if (distance < minDist1){
				int tmp = index1;
				index1 = i;
				index2 = tmp;

				double tmpDist = minDist1;
				minDist1 = distance;
				minDist2 = tmpDist;
			}
			else {
				index2 = i;
				minDist2 = distance;
			}
		}

	}
	if (index1 > index2){
		int tmp = index1;
		index1 = index2;
		index2 = tmp;
	}
}


// responsible for making the ball bounce off of shapes

void ballImpactShape(Ball &ball, Shape &shape){
	// gets the edge of impact using the two closest vertices to the ball
	int index1, index2;
	getEdgeOfCollision(ball, shape, index1, index2);

	cout << "vx: " << ball.vx << " vy: " << ball.vy << endl;
	Vector2d edge = {shape.vertices[index2 % shape.sides].x - shape.vertices[index1].x,
			shape.vertices[index2 % shape.sides].y - shape.vertices[index1].y};
	Vector2d axis = {edge.y, -edge.x};
	axis = normalize(axis);
	double dotProduct = axis.x * ball.vx + axis.y * ball.vy;

	ball.vx = ball.vx - 2 * dotProduct * axis.x;
	ball.vy = ball.vy - 2 * dotProduct * axis.y;
	cout << "vx: " << ball.vx << " vy: " << ball.vy << endl;

}



// the intent was to push the ball out of the shape if it ever intersected it
// this function does not work, ignore
void resolveCollision(Ball &ball, Shape &shape){
	int index1, index2;
	getEdgeOfCollision(ball, shape, index1, index2);
	Vector2d edge = {shape.vertices[index2 % shape.sides].x - shape.vertices[index1].x,
			shape.vertices[index2 % shape.sides].y - shape.vertices[index1].y};
	Vector2d axis = {edge.y, -edge.x};
	axis = normalize(axis);

	double edgeProjection = shape.vertices[index1].x * axis.x + shape.vertices[index1].y * axis.y;
	double ballCenter = (ball.x * axis.x + ball.y * axis.y);
	double angle;

	double dist = ballCenter - edgeProjection;
	if (ballCenter > edgeProjection){
		dist = fabs(ball.r - dist);
	}

	angle = atan(dist);
	dist *= 10;
	ball.x += axis.x * dist * cos(angle);
	ball.y += axis.y * dist * sin(angle);

	delete &edge;
	delete &axis;
}



// handler for all things ball movement, collision, and collision interactions

void move(Ball &ball){
	Ball tmpBall = ball;
	tmpBall.x = ball.x;
	tmpBall.y = ball.y;
	double deltax = ball.vx / FPS; // distance is adjusted for fps
	double deltay = ball.vy / FPS;
	ball.vy += G / FPS;

	tmpBall.x += deltax;
	tmpBall.y += deltay;


	// intersection with another ball
	for (unsigned int i = 0; i < balls.size(); i++){
		double dist = sqrt(pow(tmpBall.x - balls.at(i).x, 2) + pow(tmpBall.y - balls.at(i).y, 2));
		if (dist <= tmpBall.r * 1.5){
			double tmpx = ball.vx;
			double tmpy = ball.vy;
			ball.vx = balls.at(i).vx;
			ball.vy = balls.at(i).vy;
			balls.at(i).vx = tmpx;
			balls.at(i).vy = tmpy;

			deltax = ball.vx / FPS;
			deltay = ball.vy / FPS;
		}
	}


	// tests if the ball WILL intersect a shape using tmpBall
	for (unsigned int i = 0; i < shapes.size(); i++){
		if (isIntersecting(tmpBall, shapes.at(i))){
			ballImpactShape(ball, shapes.at(i)); // ball bouncing
			shapes.at(i).hp--; // shape damaging
			if (shapes.at(i).hp <= 0){ // remove shape if hp is 0

				// this handles the double donk and score function
				// destroy a shape with the last ball it was hit by and you get double points and a sound effect
				if (&ball == shapes.at(i).lastHitBy && shapes.at(i).lastHitBy != NULL){
					score += 2;
					Mix_PlayChannel(-1, sfx_doubledonk, 0);
					Particle tmp(1000, img_doubledonk, 100, 100, shapes.at(i).x, shapes.at(i).y - 30);
					particles.push_back(tmp);
				}
				else {
					score++;
				}
				shapes.erase(shapes.begin() + i);
				i--;
			}
			else {
				shapes.at(i).lastHitBy = &ball;
			}
		}
	}


	for (unsigned int i = 0; i < ballPickups.size(); i++){
		double dist = sqrt(pow(tmpBall.x - ballPickups.at(i).x, 2) + pow(tmpBall.y - ballPickups.at(i).y, 2));
		if (dist <= ballPickups.at(i).r){
			Ball tmp;
			tmp.x = tmpBall.x;
			tmp.y = tmpBall.y;
			tmp.vx = tmpBall.vx;
			tmp.vy = tmpBall.vy * -1;
			balls.push_back(tmp);
			ballPickups.erase(ballPickups.begin() + i);
			player_balls_max++;
		}
	}


	// test for collision with the screen borders
	if (ball.x + deltax - ball.r < 0 || ball.x + deltax + ball.r > WINDOW_W){
		ball.vx *= -1 * FRICTION;
		deltax *= -1;
	}
	if (ball.y + deltay - ball.r < 0){
		ball.vy *= -1 * FRICTION;
		deltay *= -1;
	}
	ball.x += deltax;
	ball.y += deltay;
}









void drawCursor(SDL_Renderer *renderer, int x, int y){
	double startx = WINDOW_W / 2;
	double starty = WINDOW_H / 10;
	int dist = WINDOW_H / 100;
	Vector2d direction = {x - startx, y - starty};
	long double angle = atan2(direction.y, direction.x); // this would formally crash the program for some reason
	double tmpx = startx + (WINDOW_H / 20 + dist) * cos(angle);
	double tmpy = starty + (WINDOW_H / 20 + dist) * sin(angle);
	for (int i = 0; i < 7; i++){
		SDL_RenderFillCircle(renderer, tmpx, tmpy, dist);
		tmpx += 4 * dist * cos(angle);
		tmpy += 4 * dist * sin(angle);
	}
}




void launchBall(double angle){
	Ball tmp = {WINDOW_W / 2.0, WINDOW_H / 10.0, 20, 800 * cos(angle), 800 * sin(angle)};
	balls.push_back(tmp);
}



void seedShapes(){
	int count = 0;
	for (unsigned int i = 0; i < shapes.size(); i++){
		count += shapes.at(i).hp;
		shapes.at(i).y -= (WINDOW_H * 0.8 - (WINDOW_W / 4)) / 6;
		shapes.at(i).lastHitBy = NULL;
	}
	for (unsigned int i = 0; i < ballPickups.size(); i++){
		ballPickups.at(i).y -= (WINDOW_H * 0.8 - (WINDOW_W / 4)) / 6;
	}
	while (count < wave + 3){
		Shape tmp;
		tmp.hp = max(wave - (rand() % wave), 1);
		tmp.x = (rand() % 6) * WINDOW_W / 8 + WINDOW_W / 4;
		tmp.y = WINDOW_H - WINDOW_H / 10 - WINDOW_W / 8;
		tmp.sides = rand() % 3 + 3;
		tmp.size = WINDOW_W / 8;
		tmp.rotate = (wave > 3) ? rand() % 3: 0;

		count += tmp.hp;

		shapes.reserve(1);
		shapes.push_back(tmp);
	}
	if (wave % 10 == 0){
		BallPickup newBallPickup;
		newBallPickup.x = (rand() % 6) * WINDOW_W / 8 + WINDOW_W / 4;
		newBallPickup.y = WINDOW_H - WINDOW_H / 10 - WINDOW_W / 8;
		ballPickups.push_back(newBallPickup);
	}
}




// WINMAIN FUNCTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int WinMain(int argc, char *argv[]){
	bool allow_player_input = false;
	bool mouse_keypress = false;
	bool handle_best_score = true;
	int mouseX;
	int mouseY;
	double launch_angle;
	unsigned int ball_launch_timer;
	unsigned int confetti_timer = 0;
	string text; // generic overwriteable text variable for displaying text

	ifstream inFile;
	int bestScore;

	inFile.open("scores.txt");
	inFile >> bestScore;
	inFile.close();






	balls.reserve(20);  // vector::reserve(), my beloved
	shapes.reserve(20);
	ballPickups.reserve(20);


	srand(time(NULL));


	// SDL setup commands
	SDL_Init(SDL_INIT_VIDEO);
	TTF_Init();



	// creates the game window
	SDL_Window *window = SDL_CreateWindow("Falling Balls Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);

	/// this block was intended to make the game fullscreen, but I didn't like how it looked
	/*if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0){
		cout << "Error running SDL_SetWindowFullScreen\n";
	}*/

	// creates the renderer object
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

	// I just copied the calibri.ttf file from C:Windows/Fonts
	TTF_Font *fontBig = TTF_OpenFont("./fonts/calibri.ttf", WINDOW_H / 15);
	TTF_Font *fontMedium = TTF_OpenFont("./fonts/calibri.ttf", WINDOW_H / 20);
	TTF_Font *fontSmall = TTF_OpenFont("./fonts/calibri.ttf", WINDOW_H / 25);



	// TEXTURE INITIALIZATION
	img_seymour = IMG_LoadTexture(renderer, "./textures/seymour.jpg");
	img_doubledonk = IMG_LoadTexture(renderer, "./textures/doubledonk.png");


	// MIXER AND SFX INITIALIZATION
	Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 );
	Mix_Volume(-1, 30);
	sfx_doubledonk = Mix_LoadWAV("./sfx/Doubledonk.wav");
	sfx_horn = Mix_LoadWAV("./sfx/horn.wav");





	// manual declaration of shapes
	// HP, x, y, sides, size, rotation
	//shapes.push_back({3, 300, 500, 4, 100, 0});

	//shapes.push_back({4, 300, 400, 3, 100, 1});

	//shapes.push_back({5, 500, 400, 3, 100, 2});

	//shapes.push_back({9, 400, 700, 5, 100, 1});




	// misc geometry declaration
	SDL_Rect upperBox = {0, 0, WINDOW_W, WINDOW_H / 10};
	SDL_Rect lowerBox = {0, WINDOW_H - WINDOW_H / 10, WINDOW_W, WINDOW_H / 10};
	SDL_Rect scoreBox = {0, WINDOW_H / 3, WINDOW_W, WINDOW_H / 3};
	SDL_Rect screenDarken = {0, 0, WINDOW_W, WINDOW_H};






	// game loop
	while (game_running){

		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer,255, 255, 255, 255); // this color is used for the render functions below





		for (unsigned int i = 0; i < shapes.size(); i++){
			if (shapes.at(i).y < WINDOW_H / 10){
				game_over = true;
			}
		}




		// keyboard input
		SDL_Event event;
		SDL_PollEvent(&event);
		// sometimes when the game crashes, it goes through this block first for some reason
		// I have no clue why
		if (event.type == SDL_QUIT || event.key.keysym.sym == SDLK_ESCAPE){
			cout << "I ended legitimately\n";
			game_running = false;
		}
		if (event.key.keysym.sym == SDLK_BACKSPACE){
			game_over = true;
		}


		// this block handles player input
		if (!game_over){
			// left mouse button pressed
			if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
				mouse_keypress = true;
			}

			// left mouse button released
			if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT){
				if (mouse_keypress && allow_player_input){
					SDL_GetMouseState(&mouseX, &mouseY);
					launch_angle = atan2(mouseY - WINDOW_H / 10, mouseX - WINDOW_W / 2);
					allow_player_input = false;
					launchBall(launch_angle);
					ball_launch_timer = SDL_GetTicks() + 500;
					player_balls_count--;
				}
				mouse_keypress = false;
			}

			// handles the cursor before launching a ball
			if (mouse_keypress && allow_player_input){
				SDL_GetMouseState(&mouseX, &mouseY);
				drawCursor(renderer, mouseX, mouseY);
			}
		}






		// launches ball on a set timer like in the game
		if (SDL_GetTicks() >= ball_launch_timer && player_balls_count > 0 && !allow_player_input){
			ball_launch_timer = SDL_GetTicks() + 500;
			player_balls_count--;
			launchBall(launch_angle);
		}




		// THIS BLOCK EXECUTES WHEN THE PLAYER'S "TURN" ENDS
		if (balls.size() == 0 && !allow_player_input){
			allow_player_input = true;
			player_balls_count = player_balls_max;
			seedShapes();
			wave++;
		}




		// draw other miscellaneous geometry
		SDL_SetRenderDrawColor(renderer,49, 49, 49, 255);
		SDL_RenderFillRect(renderer, &upperBox);
		SDL_RenderFillCircle(renderer, WINDOW_W / 2, WINDOW_H / 10, WINDOW_H / 20);
		SDL_SetRenderDrawColor(renderer,31, 31, 31, 255);
		SDL_RenderFillCircle(renderer, WINDOW_W / 2, WINDOW_H / 10, 25);
		SDL_SetRenderDrawColor(renderer,255, 255, 255, 255);
		if (balls.size() == 0){
			SDL_RenderFillCircle(renderer, WINDOW_W / 2, WINDOW_H / 10, 20);
		}



		// display particles
		for (unsigned int i = 0; i < particles.size(); i++){
			if (SDL_GetTicks() >= particles.at(i).lifetime){
				particles.erase(particles.begin() + i);
			}
			else {
				SDL_Rect tmp = {static_cast<int>(particles.at(i).x), static_cast<int>(particles.at(i).y), static_cast<int>(particles.at(i).w), static_cast<int>(particles.at(i).h)};
				SDL_RenderCopy(renderer, particles.at(i).texture, NULL, &tmp);
			}
		}

		// display balls
		for (unsigned int i = 0; i < balls.size(); i++){
			move(balls.at(i));
			// very lazy, but I didn't want to also have to pass in the ball's index to move() for this to work
			if (balls.at(i).y + balls.at(i).r > WINDOW_H){
				balls.erase(balls.begin() + i);
			}
			else {
				SDL_RenderFillCircle(renderer,balls.at(i).x,balls.at(i).y,balls.at(i).r);
			}
		}

		// display shapes
		for (unsigned int i = 0; i < shapes.size(); i++){
			shapes.at(i).Draw(renderer, fontBig);
		}

		// display ball pickups
		for (unsigned int i = 0; i < ballPickups.size(); i++){
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			SDL_RenderFillCircle(renderer, ballPickups.at(i).x, ballPickups.at(i).y, ballPickups.at(i).r * 1.5);
			SDL_SetRenderDrawColor(renderer, 31, 31, 31, 255);
			SDL_RenderFillCircle(renderer, ballPickups.at(i).x, ballPickups.at(i).y, ballPickups.at(i).r * 1.25);
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			SDL_RenderFillCircle(renderer, ballPickups.at(i).x, ballPickups.at(i).y, ballPickups.at(i).r);
		}




		// render the low bound here, because we want this to cover up the balls
		SDL_SetRenderDrawColor(renderer,49, 49, 49, 255);
		SDL_RenderFillRect(renderer, &lowerBox);



		text = "BEST: " + to_string(bestScore);
		customRenderText(renderer, text, fontMedium, {255,155,0,255}, CORNER, WINDOW_W, WINDOW_H, WINDOW_W, WINDOW_H / 10 - WINDOW_H / 20);

		text = to_string(score);
		customRenderText(renderer, text, fontBig, {255,255,255,255}, CENTER, WINDOW_W, WINDOW_H, WINDOW_W / 2, 0);

		text = to_string(player_balls_max) + "x";
		customRenderText(renderer, text, fontSmall, {255,255,255,255}, CORNER, WINDOW_W, WINDOW_H, WINDOW_W / 2 - WINDOW_H / 10, WINDOW_H / 10 - WINDOW_H / 25);




		if (game_over && handle_best_score){
			if (score > bestScore){
				ofstream outFile("scores.txt", ofstream::trunc);
				outFile << score;
				outFile.close();

				confetti_timer = SDL_GetTicks() + 5000;
				Mix_PlayChannel(-1, sfx_horn, 0);
			}
			handle_best_score = false;
		}


		if (game_over){
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 155);
			SDL_RenderFillRect(renderer, &screenDarken);
			SDL_SetRenderDrawColor(renderer,49, 49, 49, 255);
			SDL_RenderFillRect(renderer, &scoreBox);
			SDL_Rect tmp = {WINDOW_W * 19 / 20 - WINDOW_H * 2 / 9, WINDOW_H / 3 + WINDOW_H / 18, WINDOW_H * 2 / 9, WINDOW_H * 2 / 9};
			SDL_RenderCopy(renderer, img_seymour, NULL, &tmp);
			text = "YOU'RE WINNER !";
			customRenderText(renderer, text, fontMedium, {255,155,0,255}, CENTER, WINDOW_W, WINDOW_H, WINDOW_W / 2, WINDOW_H / 3 + WINDOW_H / 40);
			text = "SCORE: " + to_string(score);
			customRenderText(renderer, text, fontMedium, {255,155,0,255}, CORNER, WINDOW_W, WINDOW_H, WINDOW_W / 20, WINDOW_H / 2 - WINDOW_H / 20);

			if (score > bestScore){
				text = "New best score!";
				customRenderText(renderer, text, fontSmall, {255,155,0,255}, CORNER, WINDOW_W, WINDOW_H, WINDOW_W / 20, WINDOW_H / 2);
			}

			if (SDL_GetTicks() < confetti_timer && SDL_GetTicks() % 3 == 0){
				Confetti tmp(WINDOW_W);
				confettis.push_back(tmp);
			}
			for (unsigned int i = 0; i < confettis.size(); i++){
				SDL_SetRenderDrawColor(renderer, confettis.at(i).r, confettis.at(i).g, confettis.at(i).b, 255);
				SDL_RenderFillCircle(renderer, confettis.at(i).x, confettis.at(i).y, confettis.at(i).radius);
				confettis.at(i).move();
				if (confettis.at(i).y + confettis.at(i).r > WINDOW_H){
					confettis.erase(confettis.begin() + i);
				}
			}

		}



		// the last draw color set is used for the background color when RenderPresent is called
		SDL_SetRenderDrawColor(renderer, 31, 31, 31, 255);
		SDL_RenderPresent(renderer);


		// some code from a tutorial to make sure the time between frames is consistent
		int timeToWait = TARGET_FRAME_TIME - (SDL_GetTicks() - since_last_frame);
		if (timeToWait > 0 && timeToWait <= TARGET_FRAME_TIME){
			SDL_Delay(timeToWait);
		}
		since_last_frame = SDL_GetTicks();

	}


	// game loop over
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	return 0;
}

