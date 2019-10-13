#include "pch.h"
#include "GdiRadar.h"

#include <iostream>
#include <vector>

#pragma comment(lib, "Gdi32.lib")

#define INVALID_MAP_VALUE ((UINT64)0)
#define SCALE_X(posx) ((int)((float)GameMapWidth * (1000.0f / posx)))
#define SCALE_Y(posy) ((int)((float)GameMapHeight * (1000.0f / posy)))


struct gdi_radar_drawing
{
	HBRUSH EnemyBrush;
	COLORREF TextCOLOR;
	RECT DC_Dimensions;
	HDC hdc;
};

struct gdi_radar_context
{
	PWSTR className;
	ATOM classAtom;
	PWSTR windowName;
	HWND myDrawWnd;
	WNDCLASSW wc;

	clock_t minimumUpdateTime;
	clock_t lastTimeUpdated;
	UINT64 GameMapWidth;
	UINT64 GameMapHeight;
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
		drawing->TextCOLOR = RGB(0, 255, 0);
		SetBkMode(drawing->hdc, TRANSPARENT);
		return 0;
	case WM_DESTROY:
		std::cout << "WM_DESTROY\n";
		DeleteObject(drawing->EnemyBrush);
		DeleteDC(drawing->hdc);
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		std::cout << "WM_PAINT\n";
		PAINTSTRUCT ps;

		BeginPaint(hwnd, &ps);
		for (auto& entity : wnd_ctx->entities) {
			draw_entity(wnd_ctx, entity.pos[0], entity.pos[1], entity.health, entity.color, entity.name);
		}
		EndPaint(hwnd, &ps);
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
	result->minimumUpdateTime = (clock_t)cfg->minimumUpdateTime;
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

void gdi_radar_add_entity(struct gdi_radar_context * const ctx,
	struct entity const * const ent)
{
	ctx->entities.push_back(*ent);
}

void gdi_radar_clear_entities(struct gdi_radar_context * const ctx)
{
	ctx->entities.clear();
}

void gdi_radar_set_game_dimensions(struct gdi_radar_context * const ctx,
	UINT64 GameMapWidth, UINT64 GameMapHeight)
{
	ctx->GameMapWidth = GameMapWidth;
	ctx->GameMapHeight = GameMapHeight;
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