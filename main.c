#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"
#include "SDL.h"

#define SIZE 15

struct entity {
	SDL_Rect pos;
	SDL_Texture *sprite;
};

struct item_arr {
	Sint8 first, last;
	struct item {
		int x;
		enum : Uint8 {COIN, CACTUS, NIL} type;
	} arr[SIZE];
	struct entity es[2];
};

static void items_render(SDL_Renderer *r, struct item_arr *q);

static inline void
entity_render(SDL_Renderer *r, const struct entity *e)
{
	SDL_RenderCopy(
		r,
		e->sprite,
		NULL,
		&e->pos
	);
}

static int
item_remove(struct item_arr *q);

static void
item_create(struct item_arr *q)
{
	int i;

	i = (q->last + 1) % SIZE;
	if (i == q->first)
		return;

	q->arr[q->last].x = 512;
	if (rand() % 2)
		q->arr[q->last].type = COIN;
	else
		q->arr[q->last].type = CACTUS;

	q->last = i;
}

static void
item_move(struct item_arr *q, float dT, struct entity *pato)
{
	if (q->first == q->last)
		return;

	if (q->arr[q->first].type == NIL || q->arr[q->first].x + 20 <= 0)
		item_remove(q);

	for (int i = q->first; i != q->last; i = (i+1) % SIZE) {
		struct item *it= &q->arr[i];
		struct entity *e= &q->es[it->type];
		bool col = true;

		if (it->type == NIL)
			continue;

		if(e->pos.y + e->pos.h <= pato->pos.y)
			col = false;

		if(e->pos.y >= pato->pos.y + pato->pos.h)
			col = false;

		if(it->x + e->pos.w <= pato->pos.x)
			col = false;

		if(it->x >= pato->pos.x + pato->pos.w)
			col = false;

		if (col) {
			if (it->type == COIN)
				it->type = NIL;
			else if (it->type == CACTUS)
				exit(0);
		}

		q->arr[i].x -= (int)(dT * 200);
	}
}


static int
item_remove(struct item_arr *q)
{
	if (q->first == q->last)
		return -1;

	q->first = (q->first + 1) % SIZE;

	return 0;
}

static void
items_render(SDL_Renderer *r, struct item_arr *q)
{
	if (q->first == q->last)
		return;

	for (int i = q->first; i != q->last; i = (i+1) % SIZE) {
		if (q->arr[i].type == NIL) {
			continue;
		}

		struct entity *e = &q->es[q->arr[i].type];
		e->pos.x = q->arr[i].x;
		entity_render(r, e);
	}
}

static SDL_Surface *
LoadImage(char *filename)
{
	int width, height, channels;
	unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);

	int pitch;
	pitch = width * channels;
	pitch = (pitch + 3) & ~3;

	Sint32 red, green, blue, alpha;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	red = 0x000000FF;
	green = 0x0000FF00;
	blue = 0x00FF0000;
	alpha = (channels == 4) ? 0xFF000000 : 0;
#else
	int s = (channels == 4) ? 0 : 8;
	red = 0xFF000000 >> s;
	green = 0x00FF0000 >> s;
	blue = 0x0000FF00 >> s;
	alpha = 0x000000FF >> s;
#endif

	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
		data, width, height, channels*8, pitch, red, green, blue, alpha
	);

	if (!surface)
		return NULL;

	return surface;
}

int
main(int argc, char *argv[])
{
	struct item_arr items;
	items.first = 0;
	items.last = 0;
	int size = 512;
	float yvelocity = 0.0;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR, "SDL_Init()", SDL_GetError(), 0
		);
		SDL_Log("SDL_Init(): %s", SDL_GetError());
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow(
		"Duck",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		size,
		size,
		SDL_WINDOW_SHOWN
	);

	if (window == NULL) {
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR, "SDL_CreateWindow()", SDL_GetError(), 0
		);
		SDL_Log("SDL_CreateWindow(): %s", SDL_GetError());
		return 1;
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

	if (renderer == NULL) {
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR, "SDL_CreateRenderer()", SDL_GetError(), window
		);
		SDL_Log("SDL_CreateRenderer(): %s", SDL_GetError());
		return 1;
	}

	srand(time(NULL));

	SDL_Surface *surface = LoadImage("duck.png");
	SDL_Texture *ducks = SDL_CreateTextureFromSurface(renderer, surface);

	surface = LoadImage("coin.png");
	items.es[COIN].sprite = SDL_CreateTextureFromSurface(renderer, surface);

	surface = LoadImage("cactus.png");
	items.es[CACTUS].sprite = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_FreeSurface(surface);

	items.es[COIN].pos.w = 20;
	items.es[COIN].pos.h = 20;
	items.es[COIN].pos.y = 310;

	items.es[CACTUS].pos.w = 20;
	items.es[CACTUS].pos.h = 50;
	items.es[CACTUS].pos.y = size/3*2-50;

	struct entity pato = {
		.pos = {
			.x = 50,
			.y = size/3*2,
			.w = 50,
			.h = 50
		},
		.sprite = ducks
	};

	SDL_Rect ground = {
		.x = 0,
		.y = size/3*2,
		.w = size,
		.h = size/3+2
	};

	SDL_Event e;
	bool quit = false;
	Uint32 last_time = SDL_GetTicks();
	Uint32 last_spawn = last_time;
	while(!quit) {
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				switch (e.key.keysym.sym) {
				case 'q':
				    quit = true;
				    break;
				case SDLK_UP: case 'w':
					if (pato.pos.y + pato.pos.h >= ground.y)
				    yvelocity = -400;
				}
				break;
			case SDL_KEYUP:
				switch (e.key.keysym.sym) {
				case SDLK_UP: case 'w':
					if (yvelocity < -150)
				    yvelocity += 150;
				}
			}
		}
		Uint32 time = SDL_GetTicks();
		float dT = (time - last_time) / 1000.0f;
		last_time = time;
			
		yvelocity += dT * 750.0f;
		pato.pos.y += yvelocity * dT;

		if (pato.pos.y + pato.pos.h >= ground.y) {
			yvelocity = 0;
			pato.pos.y = ground.y - pato.pos.h;
		}

		if ((time - last_spawn) / 1000.0f >= 2) {
			last_spawn = time;
			item_create(&items);			
		}

		item_move(&items, dT, &pato);

		SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
		SDL_RenderFillRect(renderer, &ground);

		entity_render(renderer, &pato);

		items_render(renderer, &items);
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
