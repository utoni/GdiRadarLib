#include "stdafx.h"
#include "GdiRadar.h"

#include <iostream>
#include <vector>

#pragma comment(lib, "Gdi32.lib")

#ifdef _VERBOSE
#define DBG(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#define INVALID_MAP_VALUE ((UINT64)0)


struct gdi_radar_drawing
{
	HBRUSH BlueBrush, BlackBrush, RedBrush;
	HPEN DefaultPen;
	COLORREF TextCOLOR;
	RECT DC_Dimensions;
	HDC hdc;
	UINT64 GameMapWindowWidth;
	UINT64 GameMapWindowHeight;
	bool StickToBottom;
};

struct gdi_radar_context
{
	PWSTR className;
	ATOM classAtom;
	PWSTR windowName;
	HWND myDrawWnd;
	WNDCLASSW wc;

	double minimumUpdateTime;
	UINT64 maximumRedrawFails;
	clock_t lastTimeUpdated;
	UINT64 GameMapWidth;
	UINT64 GameMapHeight;
	size_t reservedEntities;

	struct gdi_radar_drawing drawing;
	std::vector<struct entity> entities;
};


static void draw_entity(struct gdi_radar_context * const ctx, struct entity * const ent)
{
#if 0
	RECT healthRect = { posx - 10, posy - 10, posx + 10, posy - 5 };
	FillRect(hdc, &rect, color);

	RECT textRect = { posx, posy, posx + 10, posy - 5 };
	DrawText(hdc, TEXT("Michael Morrison"), -1, &rect,
		DT_SINGLELINE | DT_CENTER | DT_VCENTER);
#endif

	switch (ent->color) {
	case EC_BLUE:
		SelectObject(ctx->drawing.hdc, ctx->drawing.BlueBrush);
		break;
	case EC_BLACK:
		SelectObject(ctx->drawing.hdc, ctx->drawing.BlackBrush);
		break;
	case EC_RED:
		SelectObject(ctx->drawing.hdc, ctx->drawing.RedBrush);
		break;
	}

	float frealx = ent->pos[0] * ((float)ctx->drawing.GameMapWindowWidth / ctx->GameMapWidth);
	float frealy = ent->pos[1] * ((float)ctx->drawing.GameMapWindowHeight / ctx->GameMapHeight);
	int realx = (int)frealx;
	int realy = (int)frealy;
	Ellipse(ctx->drawing.hdc, realx - 5, realy - 5, realx + 5, realy + 5);
}

static void CalcGameToWindowDimensions(struct gdi_radar_context * const ctx)
{
	float aspectRatio = (float)ctx->GameMapWidth / ctx->GameMapHeight;
	if (ctx->drawing.StickToBottom) {
		float newWidth = (float)ctx->drawing.DC_Dimensions.bottom / aspectRatio;
		ctx->drawing.GameMapWindowHeight = ctx->drawing.DC_Dimensions.bottom - 1;
		ctx->drawing.GameMapWindowWidth = (UINT64)newWidth;
	}
	else {
		float newHeight = (float)ctx->drawing.DC_Dimensions.right / aspectRatio;
		ctx->drawing.GameMapWindowHeight = (UINT64)newHeight;
		ctx->drawing.GameMapWindowWidth = ctx->drawing.DC_Dimensions.right - 1;
	}
}

static void CleanupWindowMemory(struct gdi_radar_context * const ctx)
{
	struct gdi_radar_drawing * const drawing = &ctx->drawing;

	DeleteObject(drawing->BlueBrush);
	DeleteObject(drawing->BlackBrush);
	DeleteObject(drawing->RedBrush);
	DeleteObject(drawing->DefaultPen);
	DeleteDC(drawing->hdc);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct gdi_radar_context * wnd_ctx = NULL;

	if (message == WM_CREATE) {
		LONG_PTR pParent = (LONG_PTR)((LPCREATESTRUCTW)lparam)->lpCreateParams;
		SetWindowLongPtrW(hwnd, -21, pParent);
		wnd_ctx = (struct gdi_radar_context *)pParent;
	}
	else {
		wnd_ctx = (struct gdi_radar_context *)GetWindowLongPtrW(hwnd, -21);
	}

	if (!wnd_ctx)
	{
		DBG("%s\n", "WndProc: ctx NULL!");
		return DefWindowProc(hwnd, message, wparam, lparam);
	}

	struct gdi_radar_drawing * const drawing = &wnd_ctx->drawing;
	if (!drawing)
	{
		DBG("%s\n", "WndProc: drawing NULL!");
		return DefWindowProc(hwnd, message, wparam, lparam);
	}

	switch (message)
	{
	case WM_CREATE:
		DBG("%s\n", "WM_CREATE");
		drawing->hdc = GetDC(hwnd);
		drawing->BlueBrush = CreateSolidBrush(RGB(0, 0, 255));
		drawing->BlackBrush = CreateSolidBrush(RGB(0, 0, 0));
		drawing->RedBrush = CreateSolidBrush(RGB(255, 0, 0));
		drawing->DefaultPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 0));
		drawing->TextCOLOR = RGB(0, 255, 0);
		SetBkMode(drawing->hdc, TRANSPARENT);
		return 0;
	case WM_DESTROY:
		DBG("%s\n", "WM_DESTROY");
		CleanupWindowMemory(wnd_ctx);
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		DBG("%s\n", "WM_PAINT");
		PAINTSTRUCT ps;

		BeginPaint(hwnd, &ps);

		SelectObject(drawing->hdc, drawing->DefaultPen);
		POINT lines[] = { { 0,0 }, { (LONG)drawing->GameMapWindowWidth, 0 },
			{ (LONG)drawing->GameMapWindowWidth,  (LONG)drawing->GameMapWindowHeight },
			{ 0, (LONG)drawing->GameMapWindowHeight }, { 0,0 } };
		Polyline(drawing->hdc, lines, 5);

		for (size_t i = 0; i < wnd_ctx->entities.size(); ++i) {
			draw_entity(wnd_ctx, &wnd_ctx->entities.at(i));
		}

		EndPaint(hwnd, &ps);

		wnd_ctx->lastTimeUpdated = clock();
		break;
	}

	case WM_LBUTTONDOWN:
		DBG("%s\n", "WM_LBUTTONDOWN");
		break;
	case WM_NCLBUTTONDOWN:
		DBG("%s\n", "WM_NCLBUTTONDOWN");
		break;
	case WM_CHAR:
		DBG("%s\n", "WM_CHAR");
		break;
	case WM_MOVE:
		DBG("%s\n", "WM_MOVE");
		break;
	case WM_SIZE:
		DBG("%s\n", "WM_SIZE");
		GetClientRect(hwnd, &drawing->DC_Dimensions);
		CalcGameToWindowDimensions(wnd_ctx);
		break;
	}

	return DefWindowProcW(hwnd, message, wparam, lparam);
}

struct gdi_radar_context * const
	gdi_radar_configure(struct gdi_radar_config const * const cfg,
		HINSTANCE hInst)
{
	struct gdi_radar_context * result = new gdi_radar_context;
	if (!result)
	{
		return NULL;
	}
	ZeroMemory(result, sizeof(*result));

	/* config params */
	result->className = _wcsdup(cfg->className);
	result->windowName = _wcsdup(cfg->windowName);
	result->minimumUpdateTime = cfg->minimumUpdateTime;
	result->maximumRedrawFails = cfg->maximumRedrawFails;
	result->reservedEntities = cfg->reservedEntities;
	result->entities.reserve(result->reservedEntities);

	/* other */
	result->wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	result->wc.hCursor = LoadCursor(hInst, IDC_ARROW);
	result->wc.hIcon = LoadIcon(hInst, IDI_APPLICATION);
	result->wc.hInstance = hInst;
	result->wc.lpfnWndProc = WndProc;
	result->wc.lpszClassName = result->className;
	result->wc.style = CS_HREDRAW | CS_VREDRAW;
	result->GameMapWidth = INVALID_MAP_VALUE;
	result->GameMapHeight = INVALID_MAP_VALUE;

	return result;
}

bool gdi_radar_init(struct gdi_radar_context * const ctx)
{
	if (!ctx)
	{
		return false;
	}

	if (ctx->GameMapWidth == INVALID_MAP_VALUE ||
		ctx->GameMapHeight == INVALID_MAP_VALUE)
	{
		DBG("%s\n", "Invalid game map dimensions!");
		return false;
	}

	UnregisterClassW(ctx->className, ctx->wc.hInstance);
	ctx->classAtom = RegisterClassW(&ctx->wc);
	if (!ctx->classAtom)
	{
		DBG("Register window class failed with 0x%X!\n", GetLastError());
		return false;
	}

	ctx->myDrawWnd = CreateWindowW(ctx->className, ctx->windowName,
		WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_EX_LAYERED | WS_VISIBLE |
		WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_MAXIMIZEBOX | WS_SIZEBOX,
		50, 50, 640, 480,
		NULL, NULL, ctx->wc.hInstance, ctx);
	if (!ctx->myDrawWnd)
	{
		DBG("%s\n", "Create window failed!");
		return false;
	}
	if (!ShowWindow(ctx->myDrawWnd, SW_SHOWNORMAL))
	{
		DBG("%s\n", "Show window failed!");
	}
	if (!UpdateWindow(ctx->myDrawWnd))
	{
		DBG("%s\n", "Update window failed!");
		return false;
	}

	ctx->lastTimeUpdated = clock();
	return true;
}

static void gdi_radar_check_entity_bounds(struct gdi_radar_context * const ctx,
	struct entity * const ent)
{
	if (ent->pos[0] < 0)
	{
		ent->pos[0] = 0;
	}
	if (ent->pos[0] > ctx->GameMapWidth)
	{
		ent->pos[0] = (int)ctx->GameMapWidth;
	}
	if (ent->pos[1] < 0)
	{
		ent->pos[1] = 0;
	}
	if (ent->pos[1] > ctx->GameMapHeight)
	{
		ent->pos[1] = (int)ctx->GameMapHeight;
	}
}

void gdi_radar_add_entity(struct gdi_radar_context * const ctx,
	struct entity * const ent)
{
	if (!ctx)
	{
		return;
	}

	gdi_radar_check_entity_bounds(ctx, ent);
	ctx->entities.push_back(*ent);
}

void gdi_radar_set_entity(struct gdi_radar_context * const ctx, size_t i,
	struct entity * const ent)
{
	if (!ctx)
	{
		return;
	}

	struct entity& found_ent = ctx->entities.at(i);
	gdi_radar_check_entity_bounds(ctx, ent);
	found_ent = *ent;
}

void gdi_radar_clear_entities(struct gdi_radar_context * const ctx)
{
	if (!ctx)
	{
		return;
	}

	ctx->entities.clear();
}

bool gdi_radar_redraw_if_necessary(struct gdi_radar_context * const ctx)
{
	clock_t end;
	double cpu_time_used;

	if (!ctx)
	{
		return false;
	}

	end = clock();
	cpu_time_used = ((double)(end - ctx->lastTimeUpdated)) / CLOCKS_PER_SEC;
#ifdef _DEBUG
	DBG("Time past after last update: %lf\n", cpu_time_used);
#endif

	if (cpu_time_used > ctx->minimumUpdateTime) {
		if (cpu_time_used > ctx->minimumUpdateTime * ctx->maximumRedrawFails) {
			DBG("ERROR: Redraw failed for the last %llu times!\n",
				ctx->maximumRedrawFails);
			return false;
		}
		RedrawWindow(ctx->myDrawWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
	}

	return true;
}

void gdi_radar_set_game_dimensions(struct gdi_radar_context * const ctx,
	UINT64 GameMapWidth, UINT64 GameMapHeight, bool StickToBottom)
{
	if (!ctx)
	{
		return;
	}

	ctx->GameMapWidth = GameMapWidth;
	ctx->GameMapHeight = GameMapHeight;
	ctx->drawing.StickToBottom = StickToBottom;
}

static inline LRESULT
gdi_process_events(struct gdi_radar_context * const ctx, MSG * const msg)
{
	TranslateMessage(msg);
	return DispatchMessageW(msg);
}

LRESULT gdi_radar_process_window_events_blocking(struct gdi_radar_context * const ctx)
{
	MSG msg;

	if (!ctx)
	{
		return 0;
	}

	while (GetMessageW(&msg, ctx->myDrawWnd, 0, 0))
	{
		gdi_process_events(ctx, &msg);
	}
	return 1;
}

LRESULT gdi_radar_process_window_events_nonblocking(struct gdi_radar_context * const ctx)
{
	MSG msg;

	if (!ctx)
	{
		return 0;
	}

	while (PeekMessageW(&msg, ctx->myDrawWnd, 0, 0, PM_REMOVE))
	{
		gdi_process_events(ctx, &msg);
	}
	return 1;
}

void gdi_radar_close_and_cleanup(struct gdi_radar_context ** const ctx)
{
	if (!ctx || !*ctx)
	{
		return;
	}

	CloseWindow((*ctx)->myDrawWnd);
	DestroyWindow((*ctx)->myDrawWnd);
	CleanupWindowMemory(*ctx);

	*ctx = NULL;
}