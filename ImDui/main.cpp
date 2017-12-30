#include "ImDui.h"

// Data
static ID2D1Factory*			g_pD2DFactory		= NULL;		// D2D工厂
static IDWriteFactory*			g_pDWriteFactory	= NULL;		// DWrite工厂
static IWICImagingFactory*		g_pWICFactory		= NULL;		// WIC工厂
static ID2D1HwndRenderTarget*	g_pMainRT			= NULL;		// 呈现器

template<class Interface>
inline void SafeRelease(Interface **ppInterfaceToRelease)
{
	if (*ppInterfaceToRelease != NULL)
	{
		(*ppInterfaceToRelease)->Release();

		(*ppInterfaceToRelease) = NULL;
	}
}

HRESULT CreateDeviceIndependentResources()
{
	HRESULT hr;
	ID2D1GeometrySink *pSink = NULL;

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);

	if (SUCCEEDED(hr))
		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(g_pDWriteFactory), reinterpret_cast<IUnknown **>(&g_pDWriteFactory));

	CoInitialize(NULL);

	if (SUCCEEDED(hr))
		hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_pWICFactory));

	return hr;
}

HRESULT CreateDeviceResources(HWND hWnd)
{
	HRESULT hr = S_OK;

	if (!g_pMainRT)
	{
		RECT rc;
		GetClientRect(hWnd, &rc);
		D2D1_SIZE_U size = D2D1::SizeU(	rc.right - rc.left,	rc.bottom - rc.top);
		hr = g_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &g_pMainRT);
	}

	return hr;
}

void DestroyResources()
{
	SafeRelease(&g_pD2DFactory);
	SafeRelease(&g_pDWriteFactory);
	SafeRelease(&g_pWICFactory);
	SafeRelease(&g_pMainRT);

	CoUninitialize();
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ImDui::Event& events = ImDui::GetEvents();
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		events.MouseDown = true;
		return true;
	case WM_LBUTTONUP:
		events.MouseDown = false;
		return true;
	case WM_MOUSEWHEEL:
		events.MouseWheel = GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1 : -1;
		return true;
	case WM_MOUSEMOVE:
		events.MousePos.x = (signed short)(lParam);
		events.MousePos.y = (signed short)(lParam >> 16);
		return true;
	case WM_SIZE:
		g_pMainRT->Resize(D2D1::SizeU(LOWORD(lParam), HIWORD(lParam)));
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void ShowWindowOptions(bool* open)
{
	static char* options_en[11] = 
	{
		"Window Options",
		"I can eat glass and it doesn't hurt me.",
		"Help",
		"1.Double-click on title bar to collapse window.\n2.Click and drag on lower right corner to resize window.\n3.Click and drag on any empty space to move window.",
		"Window options",
		"no titlebar",
		"no border",
		"no resize",
		"no move",
		"English",
		"fill alpha"
	};

	static char* options_cn[11] = 
	{
		"窗口选项",
		"我能吞下玻璃而不伤身体。",
		"帮助",
		"1.双击标题栏可以折叠窗口。\n2.点击右下角拖动可调整窗口大小。\n3.点击拖动空区域可以移动窗口。",
		"窗口选项",
		"无标题栏",
		"无边框",
		"可调大小",
		"可移动",
		"中文",
		"窗口透明度"
	};

	static bool no_titlebar = false;
	static bool no_border = true;
	static bool no_resize = false;
	static bool no_move = false;
	static bool cn = true;
	static float fill_alpha = 1.f;

	const unsigned int layout_flags = 
		(no_titlebar ? ImDuiWindowFlags_NoTitleBar : 0) | 
		(no_border ? 0 : ImDuiWindowFlags_ShowBorders) |
		(no_resize ? ImDuiWindowFlags_NoResize : 0) | 
		(no_move ? ImDuiWindowFlags_NoMove : 0);
	ImDui::BeginWindow(cn ? options_cn[0] : options_en[0], open, ImFloat2(20, 20 + 200 + 20), ImFloat2(400, 300), fill_alpha, layout_flags);
	ImDui::Text(cn ? options_cn[1] : options_en[1]);
	ImDui::Spacing();

	if (ImDui::Collapse(cn ? options_cn[2] : options_en[2]))
	{
		ImDui::Text(cn ? options_cn[3] : options_en[3]);
	}

	if (ImDui::Collapse(cn ? options_cn[4] : options_en[4]))
	{
		ImDui::CheckBox(cn ? options_cn[5] : options_en[5], &no_titlebar); ImDui::SameLine(100);
		ImDui::CheckBox(cn ? options_cn[6] : options_en[6], &no_border); 
		ImDui::CheckBox(cn ? options_cn[7] : options_en[7], &no_resize);ImDui::SameLine(100);
		ImDui::CheckBox(cn ? options_cn[8] : options_en[8], &no_move); ImDui::SameLine(200);
		ImDui::CheckBox(cn ? options_cn[9] : options_en[9], &cn);
		ImDui::SliderFloat(cn ? options_cn[10] : options_en[10], &fill_alpha, 0.0f, 1.0f);
	}

	ImDui::EndWindow();
}

int main(int, char**)
{
	CreateDeviceIndependentResources();

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, L"ImDui Example", NULL };
	RegisterClassEx(&wc);
	HWND hwnd = CreateWindow(L"ImDui Example", L"ImDui Example", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1080, 640, NULL, NULL, wc.hInstance, NULL);

	if (!SUCCEEDED(CreateDeviceResources(hwnd)))
	{
		DestroyResources();
		UnregisterClass(L"ImDui Example", wc.hInstance);
		return 1;
	}

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	ImDui::InitResources(g_pD2DFactory, g_pDWriteFactory, g_pWICFactory, g_pMainRT);
	ImDui::SetBgImage("iceland.jpg");

	bool show_demo = true;
	bool show_window_options = true;
	bool show_style_editor = true;
	ImFloat4 clear_color = ImFloat4(194 / 255.f, 194 / 255.f, 100 / 255.f, 1.f);

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		ImDui::NewFrame();

		// test codes

		if (show_demo)
		{
			ImDui::BeginWindow("ImDui Demo", &show_demo, ImFloat2(20, 20), ImFloat2(400, 200));
			ImDui::Text("Hello ImDui!");
			ImDui::Button("I am a button.");

			static float val_slider = 0.5;
			ImDui::SliderFloat("slider", &val_slider, 0.0f, 1.0f);

			static bool val1 = false;
			static bool val2 = false;
			ImDui::CheckBox("checkbox1", &val1); ImDui::SameLine(100);
			ImDui::CheckBox("checkbox2", &val2);

			static int e = 0;
			ImDui::RadioButton("radio a", &e, 0); ImDui::SameLine(100);
			ImDui::RadioButton("radio b", &e, 1); ImDui::SameLine(200);
			ImDui::RadioButton("radio c", &e, 2);

			static float col1[3] = { 1.0f,0.0f,0.2f };
			ImDui::ColorEdit3("color editor 1", col1);
			ImDui::EndWindow();
		}

		if (show_window_options)
		{
			ShowWindowOptions(&show_window_options);
		}

		if (show_style_editor)
		{
			ImDui::BeginWindow("Style Editor", &show_style_editor, ImFloat2(1080 - 400 - 20 - 18, 20), ImFloat2(400, 570));
			ImDui::ShowStyleEditor();
			ImDui::EndWindow();
		}

		g_pMainRT->BeginDraw();
		g_pMainRT->Clear(clear_color.ToD2DColorF());

		ImDui::Render();

		g_pMainRT->EndDraw();
	}

	ImDui::Shutdown();
	DestroyResources();
	UnregisterClass(L"ImDui Example", wc.hInstance);

	return 0;
}
