#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"
#include "SDL.h"

#define POOL_SIZE 15


struct entity {
	SDL_Rect pos;
	SDL_Texture *sprite;
};

struct item {
	enum : Sint8 {
		NIL = -1,
		COIN = 0,
		CACTUS = 1
	} type;

	union {
		int x;
		struct item *next;
	};
};

struct item_arr {
	struct entity e[2];
	struct item *first;
	struct item pool[POOL_SIZE];
};

static void items_render(SDL_Renderer *, struct item_arr *);

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

static inline void
item_remove(struct item_arr *a, struct item *it)
{
	it->type = NIL;
	it->next = a->first;
	a->first = it;
}

static int
item_create(struct item_arr *a)
{
	if (a->first == NULL)
		return -1;
	
	struct item *i = a->first;
	a->first = i->next;

	i->x = 512;
	i->type = rand() & 1;

	return 0;
}

static void
item_move(struct item_arr *a, float dT, const struct entity *pato)
{
	for (int i = 0; i < POOL_SIZE; i++) {
		struct item *it = &a->pool[i];
		struct entity *e = &a->e[it->type];
		bool col = true;

		if (it->type == NIL)
			continue;

		if (it->x + e->pos.w <= 0) {
			item_remove(a, it);
			continue;
		}

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
				item_remove(a, it);
			else if (it->type == CACTUS)
				exit(0);
		}

		it->x -= (int)(dT * 200);
	}
}


static void
items_render(SDL_Renderer *r, struct item_arr *a)
{
	for (int i = 0; i < POOL_SIZE; i++) {
		if (a->pool[i].type == NIL) {
			continue;
		}

		struct entity *e = &a->e[a->pool[i].type];
		e->pos.x = a->pool[i].x;
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

	items.first = &items.pool[0];
	for (int i = 0; i < POOL_SIZE; i++) {
		items.pool[i].next = &items.pool[i+1];
		items.pool[i].type = NIL;
	}

	srand(time(NULL));

	SDL_Surface *surface = LoadImage("duck.png");
	SDL_Texture *ducks = SDL_CreateTextureFromSurface(renderer, surface);

	surface = LoadImage("coin.png");
	items.e[COIN].sprite = SDL_CreateTextureFromSurface(renderer, surface);

	surface = LoadImage("cactus.png");
	items.e[CACTUS].sprite = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_FreeSurface(surface);

	items.e[COIN].pos.w = 20;
	items.e[COIN].pos.h = 20;
	items.e[COIN].pos.y = 310;

	items.e[CACTUS].pos.w = 20;
	items.e[CACTUS].pos.h = 50;
	items.e[CACTUS].pos.y = size/3*2-50;

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
					if (yvelocity < -200)
						yvelocity = -200;
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
