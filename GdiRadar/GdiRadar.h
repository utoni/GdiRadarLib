#pragma once

#include <time.h>
#include <Windows.h>


struct gdi_radar_config {
	LPCWSTR className;
	LPCWSTR windowName;
	clock_t minimumUpdateTime;
	size_t reservedEntities;
};

struct gdi_radar_context;


struct gdi_radar_context * const
	gdi_radar_configure(struct gdi_radar_config const * const cfg,
		HINSTANCE hInst);
bool gdi_radar_init(struct gdi_radar_context * const ctx);


enum entity_color {
	EC_RED
};

struct entity {
	float pos[2];
	float health;
	enum entity_color color;
	const char *name;
};


void gdi_radar_add_entity(struct gdi_radar_context * const ctx,
	struct entity const * const ent);
void gdi_radar_clear_entities(struct gdi_radar_context * const ctx);
void gdi_radar_process_window_events_blocking(struct gdi_radar_context * const ctx);
void gdi_radar_process_window_events_nonblocking(struct gdi_radar_context * const ctx);