#pragma once

#include <time.h>
#include <Windows.h>


struct gdi_radar_config {
	LPCWSTR className;
	LPCWSTR windowName;
	double minimumUpdateTime;
	UINT64 maximumRedrawFails;
	size_t reservedEntities;
};

struct gdi_radar_context;


static HINSTANCE gdi_radar_get_fake_hinstance()
{
	LONG_PTR hi = GetWindowLongW(GetActiveWindow(), -6);
	return (HINSTANCE)hi;
}
struct gdi_radar_context * const
	gdi_radar_configure(struct gdi_radar_config const * const cfg,
		HINSTANCE hInst);
bool gdi_radar_init(struct gdi_radar_context * const ctx);


enum entity_color {
	EC_RED
};

struct entity {
	int pos[2];
	float health;
	enum entity_color color;
	const char *name;
};


void gdi_radar_add_entity(struct gdi_radar_context * const ctx,
	struct entity * const ent);
void gdi_radar_set_entity(struct gdi_radar_context * const ctx, size_t i,
	struct entity * const ent);
void gdi_radar_clear_entities(struct gdi_radar_context * const ctx);
bool gdi_radar_redraw_if_necessary(struct gdi_radar_context * const ctx);
void gdi_radar_set_game_dimensions(struct gdi_radar_context * const ctx,
	UINT64 GameMapWidth, UINT64 GameMapHeight, bool StickToBottom = true);
static void gdi_radar_set_game_dimensions(struct gdi_radar_context * const ctx,
	float GameMapWidth, float GameMapHeight, bool StickToBottom = true)
{
	gdi_radar_set_game_dimensions(ctx,
		(UINT64)GameMapWidth, (UINT64)GameMapHeight, StickToBottom);
}
LRESULT gdi_radar_process_window_events_blocking(struct gdi_radar_context * const ctx);
LRESULT gdi_radar_process_window_events_nonblocking(struct gdi_radar_context * const ctx);