#include "stdafx.h"
#include "GdiRadar.h"

#include <iostream>
#include <vector>

#pragma comment(lib, "Gdi32.lib")

#define INVALID_MAP_VALUE ((UINT64)0)


struct gdi_radar_drawing
{
	HBRUSH EnemyBrush;
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
	case EC_RED:
		SelectObject(ctx->drawing.hdc, ctx->drawing.EnemyBrush);
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
		std::cout << "WndProc: ctx NULL!\n";
		return DefWindowProc(hwnd, message, wparam, lparam);
	}

	struct gdi_radar_drawing * const drawing = &wnd_ctx->drawing;
	if (!drawing)
	{
		std::cout << "WndProc: drawing NULL!\n";
		return DefWindowProc(hwnd, message, wparam, lparam);
	}

	switch (message)
	{
	case WM_CREATE:
		std::cout << "WM_CREATE\n";
		drawing->hdc = GetDC(hwnd);
		drawing->EnemyBrush = CreateSolidBrush(RGB(255, 0, 0));
		drawing->DefaultPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 0));
		drawing->TextCOLOR = RGB(0, 255, 0);
		SetBkMode(drawing->hdc, TRANSPARENT);
		return 0;
	case WM_DESTROY:
		std::cout << "WM_DESTROY\n";
		DeleteObject(drawing->EnemyBrush);
		DeleteObject(drawing->DefaultPen);
		DeleteDC(drawing->hdc);
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		std::cout << "WM_PAINT\n";
		PAINTSTRUCT ps;

		BeginPaint(hwnd, &ps);

		SelectObject(drawing->hdc, drawing->DefaultPen);
		POINT lines[] = { { 0,0 }, { (LONG)drawing->GameMapWindowWidth, 0 },
			{ (LONG)drawing->GameMapWindowWidth,  (LONG)drawing->GameMapWindowHeight },
			{ 0, (LONG)drawing->GameMapWindowHeight }, { 0,0 } };
		Polyline(drawing->hdc, lines, 5);
		for (auto& entity : wnd_ctx->entities) {
			draw_entity(wnd_ctx, &entity);
		}
		EndPaint(hwnd, &ps);

		wnd_ctx->lastTimeUpdated = clock();
		break;
	}

	case WM_LBUTTONDOWN:
		std::cout << "WM_LBUTTONDOWN\n";
		break;
	case WM_NCLBUTTONDOWN:
		std::cout << "WM_NCLBUTTONDOWN\n";
		break;
	case WM_CHAR:
		std::cout << "WM_CHAR\n";
		break;
	case WM_MOVE:
		std::cout << "WM_MOVE\n";
		break;
	case WM_SIZE:
		std::cout << "WM_SIZE\n";
		GetClientRect(hwnd, &drawing->DC_Dimensions);
		CalcGameToWindowDimensions(wnd_ctx);
		break;
	}

	//std::cout << "Default window proc for message 0x" << std::hex << message << std::endl;
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
	if (ctx->GameMapWidth == INVALID_MAP_VALUE ||
		ctx->GameMapHeight == INVALID_MAP_VALUE)
	{
		std::cout << "Invalid game map dimensions!\n";
		return false;
	}

	UnregisterClassW(ctx->className, ctx->wc.hInstance);
	ctx->classAtom = RegisterClassW(&ctx->wc);
	if (!ctx->classAtom)
	{
		std::cout << "Register window class failed with 0x"
			<< std::hex << GetLastError() << "!\n";
		return false;
	}

	ctx->myDrawWnd = CreateWindowW(ctx->className, ctx->windowName,
		WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_EX_LAYERED | WS_VISIBLE |
		WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_MAXIMIZEBOX | WS_SIZEBOX,
		50, 50, 640, 480,
		NULL, NULL, ctx->wc.hInstance, ctx);
	if (!ctx->myDrawWnd)
	{
		std::cout << "Create window failed!\n";
		return false;
	}
	if (!ShowWindow(ctx->myDrawWnd, SW_SHOWNORMAL))
	{
		std::cout << "Show window failed!\n";
	}
	if (!UpdateWindow(ctx->myDrawWnd))
	{
		std::cout << "Update window failed!\n";
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
	gdi_radar_check_entity_bounds(ctx, ent);
	ctx->entities.push_back(*ent);
}

void gdi_radar_set_entity(struct gdi_radar_context * const ctx, size_t i,
	struct entity * const ent)
{
	struct entity& found_ent = ctx->entities.at(i);
	gdi_radar_check_entity_bounds(ctx, ent);
	found_ent = *ent;
}

void gdi_radar_clear_entities(struct gdi_radar_context * const ctx)
{
	ctx->entities.clear();
}

bool gdi_radar_redraw_if_necessary(struct gdi_radar_context * const ctx)
{
	clock_t end;
	double cpu_time_used;

	end = clock();
	cpu_time_used = ((double)(end - ctx->lastTimeUpdated)) / CLOCKS_PER_SEC;
	std::cout << "Time past after last update: " << cpu_time_used << std::endl;

	if (cpu_time_used > ctx->minimumUpdateTime) {
		if (cpu_time_used > ctx->minimumUpdateTime * ctx->maximumRedrawFails) {
			std::cout << "ERROR: Redraw failed for the last "
				<< ctx->maximumRedrawFails << " times!\n";
			return false;
		}
		RedrawWindow(ctx->myDrawWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
	}

	return true;
}

void gdi_radar_set_game_dimensions(struct gdi_radar_context * const ctx,
	UINT64 GameMapWidth, UINT64 GameMapHeight, bool StickToBottom)
{
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
	LRESULT result = 0;
	MSG msg;
	while (GetMessageW(&msg, ctx->myDrawWnd, 0, 0))
	{
		result = gdi_process_events(ctx, &msg);
	}
	return result;
}

LRESULT gdi_radar_process_window_events_nonblocking(struct gdi_radar_context * const ctx)
{
	LRESULT result = 0;
	MSG msg;
	while (PeekMessageW(&msg, ctx->myDrawWnd, 0, 0, PM_REMOVE))
	{
		result = gdi_process_events(ctx, &msg);
	}
	return result;
}