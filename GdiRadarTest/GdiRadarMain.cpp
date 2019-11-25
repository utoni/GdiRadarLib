#include "pch.h"
#include <GdiRadar.h>

#include <iostream>
#include <vector>

#include <Windows.h>
#include <time.h>


int main()
{
	gdi_radar_config cfg = {};
	gdi_radar_context * ctx;

	cfg.className = L"GdiRadarClass";
	cfg.windowName = L"GdiRadarWindow";
	cfg.minimumUpdateTime = 0.25f;
	cfg.maximumRedrawFails = 5;
	cfg.reservedEntities = 16;
	cfg.drawAngles = true;

	std::cout << "Init\n";

	ctx = gdi_radar_configure(&cfg, gdi_radar_get_fake_hinstance());
	if (!ctx)
	{
		std::cout << "Radar configure failed!\n";
	}
	gdi_radar_set_game_dimensions(ctx, 1000.0f, 1000.0f);
	if (!gdi_radar_init(ctx))
	{
		std::cout << "Radar initialize failed\n";
	}

	entity e1{ 0, 0, 0.0f, 0, entity_color::EC_RED, "test" };
	gdi_radar_add_entity(ctx, &e1);
	entity e2{ 1000, 1000, 0.0f, 0, entity_color::EC_RED, "whiteshirt" };
	gdi_radar_add_entity(ctx, &e2);
	entity e3{ 500, 500, 0.0f, 60, entity_color::EC_BLUE, "localplayer" };
	gdi_radar_add_entity(ctx, &e3);
	entity e4{ 50, 800, 0.0f, 0, entity_color::EC_RED, "lowlife" };
	gdi_radar_add_entity(ctx, &e4);
	entity e5{ 250, 100, 0.0f, 0, entity_color::EC_BLACK, "dead" };
	gdi_radar_add_entity(ctx, &e5);

#if 0
	gdi_radar_process_window_events_blocking(ctx);
#else
	do {
		gdi_radar_redraw_if_necessary(ctx);
		Sleep(200);

		e3.pos[0]++;
		e3.angle -= 0.1f;
		gdi_radar_set_entity(ctx, 2, &e3);
		e4.pos[0]++;
		e4.pos[1]++;
		e4.angle += 0.1f;
		gdi_radar_set_entity(ctx, 3, &e4);
	} while (gdi_radar_process_window_events_nonblocking(ctx));
#endif

	return 0;
}
