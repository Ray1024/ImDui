// Name		: ImDui
// Version	: v0.1
// File		: ImDui.h
// Author	: Xiaolei Guo(Ray1024)
// Date		: 2017-12-9

#ifndef __IMDUI_H__
#define __IMDUI_H__

#include <Windows.h>
#include <assert.h>
#include <ctime>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <functional>

#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>

#pragma comment(lib, "D2D1.lib")
#pragma comment(lib, "DWrite.lib")

typedef unsigned int ImDuiWindowFlags;
typedef unsigned int ImUint;
typedef std::unordered_map<std::string, ImUint> ImStringUintMap;

struct ImFloat2
{
	float x, y;
	ImFloat2() { x = y = 0.0f; }
	ImFloat2(float _x, float _y) { x = _x; y = _y; }

	ImFloat2 operator*(const float rhs) { return ImFloat2(x*rhs, y*rhs); }
	ImFloat2 operator/(const float rhs) { return ImFloat2(x / rhs, y / rhs); }
	ImFloat2 operator+(const ImFloat2& rhs) const { return ImFloat2(x + rhs.x, y + rhs.y); }
	ImFloat2 operator-(const ImFloat2& rhs) const { return ImFloat2(x - rhs.x, y - rhs.y); }
	ImFloat2 operator*(const ImFloat2 rhs) const { return ImFloat2(x*rhs.x, y*rhs.y); }
	ImFloat2 operator/(const ImFloat2 rhs) const { return ImFloat2(x / rhs.x, y / rhs.y); }
	ImFloat2& operator+=(const ImFloat2& rhs) { x += rhs.x; y += rhs.y; return *this; }
	ImFloat2& operator-=(const ImFloat2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
	ImFloat2& operator*=(const float rhs) { x *= rhs; y *= rhs; return *this; }
	ImFloat2& operator/=(const float rhs) { x /= rhs; y /= rhs; return *this; }

	D2D1_POINT_2F ToD2DPointF() const { return D2D1::Point2F(x, y); }
	D2D1_SIZE_F ToD2DSizeF() const { return D2D1::SizeF(x, y); }
};

struct ImFloat4
{
	float x, y, z, w;
	ImFloat4() { x = y = z = w = 0.0f; }
	ImFloat4(float _x, float _y, float _z, float _w) { x = _x; y = _y; z = _z; w = _w; }
	ImFloat4(const ImFloat2& _pos, const ImFloat2& _size) { x = _pos.x; y = _pos.y; z = _size.x; w = _size.y; }

	D2D1_RECT_F ToD2DRectF() const { return D2D1::RectF(x, y, x + z, y + w); }
	D2D_COLOR_F ToD2DColorF() const { return D2D1::ColorF(x, y, z, w); }
};

// Flags for ImDui::BeginWindow()
enum ImDuiWindowFlags_
{
	// Default: 0
	ImDuiWindowFlags_ShowBorders			= 1 << 0,
	ImDuiWindowFlags_NoTitleBar				= 1 << 1,
	ImDuiWindowFlags_NoResize				= 1 << 2,
	ImDuiWindowFlags_NoMove					= 1 << 3,
	ImDuiWindowFlags_NoScrollbar			= 1 << 4,
};

namespace ImDui
{
	struct Event
	{
		float		MouseDoubleClickTime;
		float		MouseDoubleClickMaxDist;

		ImFloat2	MousePos;
		bool		MouseDown;
		int			MouseWheel;

		bool		WantCaptureMouse;

		ImFloat2	MousePosPrev;
		ImFloat2	MouseDelta;
		bool		MouseClicked;
		ImFloat2	MouseClickedPos;
		float		MouseClickedTime;
		bool		MouseDoubleClicked;
		float		MouseDownTime;

		Event();
	};

	// Main
	void	InitResources(ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory, IWICImagingFactory* pWICFactory, ID2D1HwndRenderTarget* pMainRT);
	void	ClearResources();
	Event&	GetEvents();
	void	NewFrame();
	void	SetBgImage(std::string image, bool is_resized = true);
	void	Render();
	void	Shutdown();
	float	GetFPS();
	void	ShowStyleEditor();

	bool	BeginWindow(const char* name, bool* p_open, ImFloat2 pos = ImFloat2(), ImFloat2 size = ImFloat2(), float fill_alpha = -1.0f, ImDuiWindowFlags flags = 0);
	void	EndWindow();
	void	PushItemWidth(float width);
	void	PopItemWidth();

	// layout
	void	SameLine(int column_x = 0, int spacing_w = -1);
	void	Spacing();

	// widgets
	void	Text(const char* label, ...);
	bool	Button(const char* label, ImFloat2 size = ImFloat2(0, 0));
	void	CheckBox(const char* label, bool* v);
	bool	RadioButton(const char* label, bool active);
	bool	RadioButton(const char* label, int* v, int v_button);
	bool	Collapse(const char* label, const char* str_id = NULL, const bool display_frame = true, const bool default_open = false);
	bool	SliderFloat(const char* label, float* v, float v_min, float v_max, const char* display_format = "%.3f", float power = 1.0f);
	bool	SliderInt(const char* label, int* v, int v_min, int v_max, const char* display_format = "%.0f");
	bool	ColorButton(const ImFloat4& col, bool small_height = false, bool outline_border = true);
	bool	ColorEdit3(const char* label, float col[3]);
	bool	ColorEdit4(const char* label, float col[4], bool show_alpha = true);
	void	ToolTip(const char* fmt, ...);
}

#endif //__IMDUI_H__