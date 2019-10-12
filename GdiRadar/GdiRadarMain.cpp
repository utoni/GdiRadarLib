#include "pch.h"
#include <iostream>
#include <vector>

#include <Windows.h>
#include <time.h>

#pragma comment(lib, "Gdi32.lib")

#define SCALE_X(posx) ((int)((float)GameMapWidth * (1000.0f / posx)))
#define SCALE_Y(posy) ((int)((float)GameMapHeight * (1000.0f / posy)))

static HWND myDrawWnd = NULL;
static HINSTANCE hInstance = NULL;
static WNDCLASS wc = { 0 };
static float GameMapWidth = 0;
static float GameMapHeight = 0;

enum entity_color {
	EC_RED
};

struct entity {
	float pos[2];
	float health;
	enum entity_color color;
	const char *name;
};
std::vector<struct entity> entities;


static void draw_entity(HDC hdc, float posx, float posy, float health, enum entity_color color, const char *name)
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
		SetDCBrushColor(hdc, RGB(255, 0, 0));
		break;
	}
	std::cout << GameMapWidth << ", " << SCALE_X(posx) << std::endl;
	Ellipse(hdc, SCALE_X(posx), SCALE_Y(posy), SCALE_X(posx + 5), SCALE_Y(posy + 5));
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	static HBRUSH EnemyBrush = NULL;
	static HBRUSH BackgroundBrush = NULL;
	static COLORREF TextCOLOR = NULL;
	static HFONT HFONT_Hunt = NULL;
	static RECT DC_Dimensions = {};
	static HDC hdc = NULL;

	switch (message)
	{
	case WM_CREATE:
		std::cout << "WM_CREATE\n";
		hdc = GetDC(hwnd);
		EnemyBrush = CreateSolidBrush(RGB(255, 0, 0));
		BackgroundBrush = CreateSolidBrush(RGB(0, 0, 0));
		TextCOLOR = RGB(0, 255, 0);
		SetBkMode(hdc, TRANSPARENT);
		return 0;
	case WM_PAINT:
	{
		std::cout << "WM_PAINT\n";
		PAINTSTRUCT ps;

		BeginPaint(hwnd, &ps);
		for (auto& entity : entities) {
			draw_entity(hdc, entity.pos[0], entity.pos[1], entity.health, entity.color, entity.name);
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
		GetClientRect(hwnd, &DC_Dimensions);
		FillRect(hdc, &DC_Dimensions, BackgroundBrush);
		return 0;
	case WM_DESTROY:
		std::cout << "WM_DESTROY\n";
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wparam, lparam);
}

int main()
{
	static double last_time_called = 0.0;
	double current_time;

	std::cout << "Init\n";

	GameMapWidth = 1000.0f;
	GameMapHeight = 1000.0f;

	hInstance = (HINSTANCE)GetWindowLongW(GetActiveWindow(), -6);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hCursor = LoadCursor(hInstance, IDC_ARROW);
	wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = L"peter";
	wc.style = CS_HREDRAW | CS_VREDRAW;

	UnregisterClassW(L"peter", hInstance);
	if (!RegisterClass(&wc))
	{
		return 1;
	}

	myDrawWnd = CreateWindowW(L"peter",
		L"the window",
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_MAXIMIZEBOX | WS_SIZEBOX,
		50, 50, 640, 480,
		NULL, NULL, hInstance, NULL);
	ShowWindow(myDrawWnd, SW_SHOWNORMAL);
	UpdateWindow(myDrawWnd);

	last_time_called = clock();

	entities.push_back(entity{ 0.0f, 0.0f, 100.0f, EC_RED, "test" });
	entities.push_back(entity{ 1000.0f, 1000.0f, 50.0f, EC_RED, "m0wL" });
	entities.push_back(entity{ 500.0f, 500.0f, 80.0f, EC_RED, "whiteshirt" });

	MSG msg;
	while (GetMessageA(&msg, myDrawWnd, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
