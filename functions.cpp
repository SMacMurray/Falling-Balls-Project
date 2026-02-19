#include "functions.h"




void customRenderText(SDL_Renderer *renderer, std::string str, TTF_Font *font, SDL_Color color, TEXT_ALLIGNMENT allignment,
		const int WINDOW_W, const int WINDOW_H, int x, int y){

	char *text = &str[0];
	SDL_Surface *textSurface = TTF_RenderText_Solid(font, text, color);
	SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
	SDL_Rect *tmp = new SDL_Rect;
	tmp->w = textSurface->w;
	tmp->h = textSurface->h;
	switch(allignment){
	case CORNER:
		tmp->x = x;
		tmp->y = y;
		break;
	case CENTER:
		tmp->x = x - textSurface->w / 2;
		tmp->y = y - textSurface->h / 2;
		break;
	}

	// forces the text to display in bounds
	if (tmp->x < 0){
		tmp->x = 0;
	}
	if (tmp->x + textSurface->w > WINDOW_W){
		tmp->x = WINDOW_W - textSurface->w;
	}
	if (tmp->y < 0){
		tmp->y = 0;
	}
	if (tmp->y + textSurface->h > WINDOW_H){
		tmp->y = WINDOW_H - textSurface->h;
	}

	SDL_RenderCopy(renderer, textTexture, NULL, tmp);

	// SDL objects need special functions to remove them to prevent memory leaks
	// also delete tmp since it was created with the new keyword
	delete tmp;
	SDL_FreeSurface(textSurface);
	SDL_DestroyTexture(textTexture);
}





int min(int a, int b){
	int out = b;
	if (a <= b){
		out = a;
	}
	return out;
}

int max(int a, int b){
	int out = b;
	if (a >= b){
		out = a;
	}
	return out;
}




Vector2d::Vector2d(double x, double y){
	this->x = x;
	this->y = y;
	this->depth = 0.0;
}

Vector2d Vector2d::operator+(Vector2d rhs){
	Vector2d out;
	out.x = x + rhs.x;
	out.y = y + rhs.y;
	return out;
}

Vector2d Vector2d::operator-(Vector2d rhs){
	Vector2d out;
	out.x = x - rhs.x;
	out.y = y - rhs.y;
	return out;
}






Ball::Ball(){
	x = 0;
	y = 0;
	r = 20;
	vx = 0.0;
	vy = 0.0;
}

Ball::Ball(double x, double y, double r, double vx, double vy){
	this->x = x;
	this->y = y;
	this->r = r;
	this->vx = vx;
	this->vy = vy;
}





BallPickup::BallPickup(){
	x = 0;
	y = 0;
	r = 20;
}

BallPickup::BallPickup(double x, double y, double r){
	this->x = x;
	this->y = y;
	this->r = r;
}







void Shape::setVertices(){
	if (rotate == 1){
		angle += M_PI / 5 / FPS;
	}
	else if (rotate == 2){
		angle -= M_PI / 5 / FPS;
	}

	if (angle > 2 * M_PI){
		angle -= 2 * M_PI;
	}
	else if (angle < -2 * M_PI){
		angle += 2 * M_PI;
	}

	double tmp_angle = angle;
	double angle_interval = 2 * M_PI / sides;
	for (int i = 0; i < sides; i++){
		vertices[i].x = x + size * cos(tmp_angle);
		vertices[i].y = y + size * sin(tmp_angle);
		tmp_angle += angle_interval;
	}
}


Shape::Shape(){
	angle = 0;
	sides = 3;
	size = 100;
	hp = 1;
	x = 300;
	y = 300;
	rotate = 0;
	lastHitBy = NULL;
	setVertices();

}

Shape::Shape(int hp, int x, int y, int n, int s, int rotate){
	angle = 180.0;
	sides = n;
	size = s;
	this->hp = hp;
	this->x = x;
	this->y = y;
	this->rotate = rotate;
	lastHitBy = NULL;

	setVertices();

}



void Shape::Draw(SDL_Renderer *renderer, TTF_Font *font){
	setVertices();
	int minx = 2000000;
	int maxx = -2000000;
	int miny = 2000000;
	int maxy = -2000000;
	for (int i = 0; i < sides; i++){
		if (vertices[i].x < minx){
			minx = vertices[i].x;
		}
		if (vertices[i].x > maxx){
			maxx = vertices[i].x;
		}
		if (vertices[i].y < miny){
			miny = vertices[i].y;
		}
		if (vertices[i].y > maxy){
			maxy = vertices[i].y;
		}
	}


	int r = 255;
	int g = 0;
	int b = 0;
	g = (hp - 1) * 51;
	if (g >= 255){
		r = (hp - 5) * -51 + 255;
	}
	if (r <= 0){
		b = (hp - 10) * 51;
	}

	SDL_SetRenderDrawColor(renderer, max(r, 0), min(g, 255), min(b, 255), 255);
	std::vector <int> intersections;
	for (int i = miny; i <= maxy; i++){
		for (int j = 0; j < sides; j++){
			// get bounds of the line segment
			int tmpmin = vertices[j].y;
			int tmpmax = vertices[(j + 1) % sides].y;
			// swap them if necessary
			if (tmpmin > tmpmax){
				int tmp = tmpmin;
				tmpmin = tmpmax;
				tmpmax = tmp;
			}
			// only proceed if the y value (i) is between the line segment end points
			if (i > tmpmin && i < tmpmax){
				double slope = (vertices[(j + 1) % sides].y - vertices[j].y) / (vertices[(j + 1) % sides].x - vertices[j].x);
				if (slope != 0){
					intersections.push_back((i +  slope * vertices[j].x - vertices[j].y) / slope);
				}
			}
			else if (i == static_cast<int>(vertices[j].y)){
				intersections.push_back(static_cast<int>(vertices[j].x));
			}
		}
		if (intersections.size() == 2){
			SDL_RenderDrawLine(renderer, intersections.at(0), i, intersections.at(1), i);
		}
		intersections.clear();
	}



	// display the shape's hp using SDL_TTF, WIP
	char text[10];
	itoa(hp, text, 10);
	SDL_Surface *textSurface = TTF_RenderText_Solid(font, text, {0, 0, 0, 255});
	SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
	SDL_Rect tmp = {x - textSurface->w / 2, y - textSurface->h /2, textSurface->w, textSurface->h}; // rectangle where the text is drawn
	SDL_RenderCopy(renderer, textTexture, NULL, &tmp);

	// SDL objects need special functions to remove them to prevent memory leaks
	SDL_FreeSurface(textSurface);
	SDL_DestroyTexture(textTexture);
}


Particle::Particle(){
	this->lifetime = 0;
	this->texture = NULL;
	this->w = 0;
	this->h = 0;
	this->x = 0;
	this->y = 0;
	this->gravity = false;
	this->vx = 0;
	this->vy = 0;
}

Particle::Particle(unsigned int lifetime, SDL_Texture *texture, int w, int h, int x, int y){
	this->lifetime = SDL_GetTicks() + lifetime;
	this->texture = texture;
	this->w = w;
	this->h = h;
	this->x = x - w / 2;
	this->y = y - h / 2;
	this->gravity = false;
	this->vx = 0;
	this->vy = 0;
}






Confetti::Confetti(const int WINDOW_W){
	int randomColor = rand() % 7;
	double angle = static_cast<double>(((rand()%(static_cast<int>(2 * M_PI + 1))) - M_PI));
	vx = cos(angle) * -200;
	vy = sin(angle) * -50;
	x = WINDOW_W / 2;
	y = 0;
	radius = 20;
	switch (randomColor){
	case 0:
		r = 0;
		g = 0;
		b = 255;
		break;
	case 1:
		r = 0;
		g = 255;
		b = 0;
		break;
	case 2:
		r = 0;
		g = 255;
		b = 255;
		break;
	case 3:
		r = 255;
		g = 0;
		b = 0;
		break;
	case 4:
		r = 255;
		g = 0;
		b = 255;
		break;
	case 5:
		r = 255;
		g = 255;
		b = 0;
		break;
	case 6:
		r = 255;
		g = 255;
		b = 255;
		break;
	}
}

void Confetti::move(){
	double deltax = vx / FPS; // distance is adjusted for fps
	double deltay = vy / FPS;
	vy += G / FPS;
	x += deltax;
	y += deltay;
}



// found this online
int SDL_RenderFillCircle(SDL_Renderer * renderer, int x, int y, int radius){
	int offsetx, offsety, d;
	int status;


	offsetx = 0;
	offsety = radius;
	d = radius -1;
	status = 0;

	while (offsety >= offsetx) {

		status += SDL_RenderDrawLine(renderer, x - offsety, y + offsetx,
				x + offsety, y + offsetx);
		status += SDL_RenderDrawLine(renderer, x - offsetx, y + offsety,
				x + offsetx, y + offsety);
		status += SDL_RenderDrawLine(renderer, x - offsetx, y - offsety,
				x + offsetx, y - offsety);
		status += SDL_RenderDrawLine(renderer, x - offsety, y - offsetx,
				x + offsety, y - offsetx);

		if (status < 0) {
			status = -1;
			break;
		}

		if (d >= 2*offsetx) {
			d -= 2*offsetx + 1;
			offsetx +=1;
		}
		else if (d < 2 * (radius - offsety)) {
			d += 2 * offsety - 1;
			offsety -= 1;
		}
		else {
			d += 2 * (offsety - offsetx - 1);
			offsety -= 1;
			offsetx += 1;
		}
	}

	return status;
}
