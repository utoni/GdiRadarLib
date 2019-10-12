#include "pch.h"
#include "GdiRadar.h"

#include <iostream>
#include <vector>

#define SCALE_X(posx) ((int)((float)GameMapWidth * (1000.0f / posx)))
#define SCALE_Y(posy) ((int)((float)GameMapHeight * (1000.0f / posy)))


struct gdi_radar_drawing
{
	HBRUSH EnemyBrush;
	HBRUSH BackgroundBrush;
	COLORREF TextCOLOR;
	HFONT HFONT_Hunt;
	RECT DC_Dimensions;
	HDC hdc;
};

struct gdi_radar_context
{
	PWSTR className;
	PWSTR windowName;
	HWND myDrawWnd;
	HINSTANCE hInstance;
	WNDCLASS wc;

	clock_t minimumUpdateTime;
	clock_t lastTimeUpdated;
	float GameMapWidth;
	float GameMapHeight;
	size_t reservedEntities;

	struct gdi_radar_drawing drawing;
	std::vector<struct entity> entities;
};


static void draw_entity(struct gdi_radar_context * const ctx, float posx, float posy, float health, enum entity_color color, const char *name)
{
#if 0
	RECT healthRect = { posx - 10, posy - 10, posx + 10, posy - 5 };
	FillRect(hdc, &rect, color);

	RECT textRect = { posx, posy, posx + 10, posy - 5 };
	DrawText(hdc, TEXT("Michael Morrison"), -1, &rect,
		DT_SINGLELINE | DT_CENTER | DT_VCENTER);
#endif

	switch (color) {
	case EC_RED:
		SetDCBrushColor(ctx->drawing.hdc, RGB(255, 0, 0));
		break;
	}
	Ellipse(ctx->drawing.hdc, posx, posy, posx + 5, posy + 5);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct gdi_radar_context * const ctx = (gdi_radar_context * const)lparam;

	if (!ctx)
	{
		return DefWindowProc(hwnd, message, wparam, lparam);
	}

	struct gdi_radar_drawing * const drawing = &ctx->drawing;
	if (!drawing)
	{
		return DefWindowProc(hwnd, message, wparam, lparam);
	}

	switch (message)
	{
	case WM_CREATE:
		std::cout << "WM_CREATE\n";
		drawing->hdc = GetDC(hwnd);
		drawing->EnemyBrush = CreateSolidBrush(RGB(255, 0, 0));
		drawing->BackgroundBrush = CreateSolidBrush(RGB(0, 0, 0));
		drawing->TextCOLOR = RGB(0, 255, 0);
		SetBkMode(drawing->hdc, TRANSPARENT);
		return 0;
	case WM_DESTROY:
		std::cout << "WM_DESTROY\n";
		DeleteObject(drawing->EnemyBrush);
		DeleteObject(drawing->BackgroundBrush);
		DeleteDC(drawing->hdc);
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		std::cout << "WM_PAINT\n";
		PAINTSTRUCT ps;

		BeginPaint(hwnd, &ps);
		for (auto& entity : ctx->entities) {
			draw_entity(ctx, entity.pos[0], entity.pos[1], entity.health, entity.color, entity.name);
		}
		EndPaint(hwnd, &ps);
		return 0;
	}
	break;

	case WM_LBUTTONDOWN:
		std::cout << "WM_LBUTTONDOWN\n";
		return 0;
	case WM_NCLBUTTONDOWN:
		std::cout << "WM_NCLBUTTONDOWN\n";
		break;
	case WM_CHAR:
		std::cout << "WM_CHAR\n";
		return 0;
	case WM_MOVE:
		std::cout << "WM_MOVE\n";
		return 0;
	case WM_SIZE:
		std::cout << "WM_SIZE\n";
		GetClientRect(hwnd, &drawing->DC_Dimensions);
		FillRect(drawing->hdc, &drawing->DC_Dimensions, drawing->BackgroundBrush);
		return 0;
	}

	return DefWindowProc(hwnd, message, wparam, lparam);
}

struct gdi_radar_context * const
	gdi_radar_configure(struct gdi_radar_config const * const cfg,
		HINSTANCE hInst)
{
	struct gdi_radar_context * result = new gdi_radar_context;

	/* config params */
	result->className = _wcsdup(cfg->className);
	result->windowName = _wcsdup(cfg->windowName);
	result->minimumUpdateTime = cfg->minimumUpdateTime;
	result->reservedEntities = cfg->reservedEntities;
	result->entities.reserve(result->reservedEntities);

	/* other */
	result->hInstance = hInst;
	result->wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	result->wc.hCursor = LoadCursor(result->hInstance, IDC_ARROW);
	result->wc.hIcon = LoadIcon(result->hInstance, IDI_APPLICATION);
	result->wc.hInstance = result->hInstance;
	result->wc.lpfnWndProc = WndProc;
	result->wc.lpszClassName = result->className;
	result->wc.style = CS_HREDRAW | CS_VREDRAW;

	return result;
}

bool gdi_radar_init(struct gdi_radar_context * const ctx)
{
	UnregisterClassW(ctx->className, ctx->hInstance);
	if (!RegisterClass(&ctx->wc))
	{
		return false;
	}

	ctx->myDrawWnd = CreateWindowW(ctx->className, ctx->windowName,
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_MAXIMIZEBOX | WS_SIZEBOX,
		50, 50, 640, 480,
		NULL, NULL, ctx->hInstance, ctx);
	if (!ctx->myDrawWnd)
	{
		return false;
	}
	if (!ShowWindow(ctx->myDrawWnd, SW_SHOWNORMAL))
	{
		return false;
	}
	if (!UpdateWindow(ctx->myDrawWnd))
	{
		return false;
	}

	ctx->lastTimeUpdated = clock();
	return true;
}

void gdi_radar_add_entity(struct gdi_radar_context * const ctx,
	struct entity const * const ent)
{
	ctx->entities.push_back(*ent);
}

void gdi_radar_clear_entities(struct gdi_radar_context * const ctx)
{
	ctx->entities.clear();
}

static inline LRESULT
gdi_process_events(struct gdi_radar_context * const ctx, MSG * const msg)
{
	if (!TranslateMessage(msg))
	{
		return NULL;
	}
	return DispatchMessageW(msg);
}

void gdi_radar_process_window_events_blocking(struct gdi_radar_context * const ctx)
{
	MSG msg;
	while (GetMessageW(&msg, ctx->myDrawWnd, 0, 0))
	{
		gdi_process_events(ctx, &msg);
	}
}

void gdi_radar_process_window_events_nonblocking(struct gdi_radar_context * const ctx)
{
	MSG msg;
	while (PeekMessageW(&msg, ctx->myDrawWnd, 0, 0, PM_REMOVE))
	{
		gdi_process_events(ctx, &msg);
	}
}