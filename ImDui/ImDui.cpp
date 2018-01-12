#include "imdui.h"

#pragma warning (disable: 4996)

// Macros

#define MAX_LEN 2560

//-----------------------------------------------------------------------------
// Global
//-----------------------------------------------------------------------------

template<typename T, UINT maxElements>
class RingBuffer
{
public:
	RingBuffer() : m_start(0), m_count(0) {}

	void Add(T element)
	{
		m_elements[(m_start + m_count) % maxElements] = element;

		if (m_count < maxElements)
			m_count++;
		else
			m_start = (m_start + 1) % maxElements;
	}

	T GetFirst() const
	{
		assert(m_count > 0);
		return m_elements[m_start];
	}

	T GetLast() const
	{
		assert(m_count > 0);
		return m_elements[(m_start + m_count - 1) % maxElements];
	}

	T GetCount() const { return m_count; }

	void Reset() { m_start = 0; m_count = 0; }

private:
	UINT m_start;
	UINT m_count;
	T m_elements[maxElements];
};

enum GuiStyleColor_
{
	Color_Text,
	Color_Border,
	Color_WindowBg,
	Color_WidgetBg,
	Color_WidgetActive,
	Color_TitleBar,
	Color_TitleBarCollapsed,
	Color_Slider,
	Color_SliderActive,
	Color_Button,
	Color_ButtonHovered,
	Color_ButtonActive,
	Color_Collapse,
	Color_CollapseHovered,
	Color_CollapseActive,
	Color_ResizeGrip,
	Color_ResizeGripHovered,
	Color_ResizeGripActive,
	Color_TooltipBg,
	Color_COUNT,
};

static ImFloat4 ImHexToRGBA(ImUint rgb)
{
	static const ImUint sc_redShift = 16;
	static const ImUint sc_greenShift = 8;
	static const ImUint sc_blueShift = 0;

	static const ImUint sc_redMask = 0xff << sc_redShift;
	static const ImUint sc_greenMask = 0xff << sc_greenShift;
	static const ImUint sc_blueMask = 0xff << sc_blueShift;

	ImFloat4 c;
	c.x = static_cast<float>((rgb & sc_redMask) >> sc_redShift) / 255.f;
	c.y = static_cast<float>((rgb & sc_greenMask) >> sc_greenShift) / 255.f;
	c.z = static_cast<float>((rgb & sc_blueMask) >> sc_blueShift) / 255.f;
	c.w = 1.f;

	return c;
}

static bool IsHideText(const char* text)
{
	const char* text_end = text;
	while (*text_end != '\0' && (text_end[0] != '#' || text_end[1] != '#'))
		text_end++;
	return text_end == text;
}

//-----------------------------------------------------------------------------
// ImDui
//-----------------------------------------------------------------------------

namespace ImDui
{
	struct D2DRender;
	struct Window;

	int				Min(int lhs, int rhs) { return lhs < rhs ? lhs : rhs; }
	int				Max(int lhs, int rhs) { return lhs >= rhs ? lhs : rhs; }
	float			Min(float lhs, float rhs) { return lhs < rhs ? lhs : rhs; }
	float			Max(float lhs, float rhs) { return lhs >= rhs ? lhs : rhs; }
	ImFloat2		Min(const ImFloat2& lhs, const ImFloat2& rhs) { return ImFloat2(Min(lhs.x, rhs.x), Min(lhs.y, rhs.y)); }
	ImFloat2		Max(const ImFloat2& lhs, const ImFloat2& rhs) { return ImFloat2(Max(lhs.x, rhs.x), Max(lhs.y, rhs.y)); }
	float			Clamp(float f, float mn, float mx) { return (f < mn) ? mn : (f > mx) ? mx : f; }
	ImFloat2		Clamp(const ImFloat2& f, const ImFloat2& mn, ImFloat2 mx) { return ImFloat2(Clamp(f.x, mn.x, mx.x), Clamp(f.y, mn.y, mx.y)); }
	float			Saturate(float f) { return (f < 0.0f) ? 0.0f : (f > 1.0f) ? 1.0f : f; }
	float			Lerp(float a, float b, float t) { return a + (b - a) * t; }
	ImFloat2		Lerp(const ImFloat2& a, const ImFloat2& b, float t) { return a + (b - a) * t; }
	ImFloat2		Lerp(const ImFloat2& a, const ImFloat2& b, const ImFloat2& t) { return ImFloat2(a.x + (b.x - a.x) * t.x, a.y + (b.y - a.y) * t.y); }

	float			Distance(const ImFloat2& lhs) { return sqrt(lhs.x*lhs.x + lhs.y*lhs.y); }

	bool			PtInRect(ImFloat2 pt, ImFloat4 rt);

	void			OutLog(const char * pszFormat, ...);
	void			OutWarning(const char * pszFormat, ...);
	void			OutError(const char * pszFormat, ...);

	size_t			FormatString(char* buf, size_t buf_size, const char* fmt, ...);
	size_t			FormatStringV(char* buf, size_t buf_size, const char* fmt, va_list args);

	std::wstring	ATOW(const std::string& str);
	std::string		WTOA(const std::wstring& str);

	bool			WidgetMouseEvent(ImFloat4 bb, const ImUint id, bool* out_hovered = NULL, bool* out_held = NULL, bool repeat = false);
	bool			WindowCloseButton(bool* open = NULL);
	void			DrawWidgetFrame(ImFloat4 rect, ImUint fill_col, bool border = true);
	void			DrawCollapseState(ImFloat2 pos, float offset, float height, bool open, float scale = 1.0f);
	void			DrawWindowState(bool collapse);
	void			CalculateFramesPerSecond();
	Window*			GetWindow(const char* name);

	void			ItemSize(ImFloat2 size, ImFloat2* adjust_start_offset = NULL);
	void			ItemSize(const ImFloat4& aabb, ImFloat2* adjust_start_offset = NULL);

	struct GuiStyle
	{
		wchar_t*		FontName;
		float			FontSize;
		float			StrokeWidth;
		float			DefaultWindowAlpha;
		ImFloat2		WindowPadding;
		ImFloat2		WindowMinSize;
		ImFloat2		FramePadding;
		ImFloat2		ItemSpacing;
		ImFloat2		ItemInnerSpacing;
		float			WindowRounding;
		float			TitleBarHeight;
		ImFloat2		ResizeGripSize;
		ImFloat4		Colors[Color_COUNT];

		GuiStyle();
	};

	struct Storage
	{
		std::unordered_map<ImUint, int> Data;

		void	Clear();
		int		GetValue(ImUint key, int default_val = 0);
		void	SetValue(ImUint key, int val);
		void	SetAllInt(int val);
	};

	struct LayoutData
	{
		ImFloat2			CursorPos;
		ImFloat2			CursorPosPrevLine;
		ImFloat2			CursorStartPos;
		float				CurrentLineHeight;
		float				PrevLineHeight;
		std::vector<float>	ItemWidth;

		LayoutData()
		{
			CursorPos = CursorPosPrevLine = CursorStartPos = ImFloat2(0.0f, 0.0f);
			CurrentLineHeight = PrevLineHeight = 0.0f;
		}
	};

	struct Window
	{
		char*				Name;
		ImFloat4			Rect;
		ImDuiWindowFlags	Flags;
		float				Alpha;
		bool				Visible;
		bool				Collapse;
		float				ItemWidthDefault;
		LayoutData			Layout;
		Storage				StateStorage;
		ImStringUintMap		IDMap;
		ID2D1BitmapRenderTarget* CRT;

		void Resize(ImFloat2 size);
		ImUint GetID(const char* str);

		Window(const char* name, ImFloat2 default_pos, ImFloat2 default_size);
		~Window();

	private:

		static ImUint _idseed;
	};

	struct GUIState
	{
		float					FPS;
		Event					Events;
		GuiStyle				Styles;
		ImUint					HoveredId;
		ImUint					ActiveId;
		Window*					CurrentWindow;
		Window*					RenderWindow;
		Window*					HoveredWindow;
		std::vector<Window*>	Windows;
		char					StrToolTip[1024];
		std::string				BgImage;

		D2DRender*				Render;
	};

	//////////////////////////////////////////////////////////////////////////
	static GUIState s_state;
	static RingBuffer<LONGLONG, 10>	s_times;
	static LARGE_INTEGER s_frequency;
	//////////////////////////////////////////////////////////////////////////

	template<class Interface>
	inline void	SafeRelease(Interface **ppInterfaceToRelease)
	{
		if (*ppInterfaceToRelease != NULL)
		{
			(*ppInterfaceToRelease)->Release();
			(*ppInterfaceToRelease) = NULL;
		}
	}

	struct D2DRender
	{
		enum TEXT_ALIGNMENT_MODE
		{
			MODE_LEFT,
			MODE_CENTER,
			MODE_RIGHT,
		};

		D2DRender()
		{
			_penWidth		= s_state.Styles.StrokeWidth;
			_pD2DFactory	= NULL;
			_pDWriteFactory	= NULL;
			_pWICFactory	= NULL;
			_pMainRT		= NULL;
			_pCommonBrush	= NULL;
			_pTextFormat	= NULL;
		}

		~D2DRender()
		{
			for (auto itor = _mapBitmaps.begin(); itor != _mapBitmaps.end(); ++itor)
				SafeRelease(&itor->second);
			_mapBitmaps.clear();

			SafeRelease(&_pCommonBrush);
			SafeRelease(&_pTextFormat);
		}

		bool Init(ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory, IWICImagingFactory* pWICFactory, ID2D1HwndRenderTarget* pMainRT)
		{
			_pD2DFactory = pD2DFactory;
			_pDWriteFactory = pDWriteFactory;
			_pWICFactory = pWICFactory;
			_pMainRT = pMainRT;

			_pMainRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

			HRESULT hr = S_OK;
			if (SUCCEEDED(hr))
			{
				hr = _pDWriteFactory->CreateTextFormat(
					s_state.Styles.FontName,
					NULL,
					DWRITE_FONT_WEIGHT_REGULAR,
					DWRITE_FONT_STYLE_NORMAL,
					DWRITE_FONT_STRETCH_NORMAL,
					s_state.Styles.FontSize,
					L"en-us",
					&_pTextFormat);

				_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
				_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
				_pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
			}

			if (SUCCEEDED(hr))
				hr = _pMainRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Orange), &_pCommonBrush);

			return false;
		}

		void Resize(float w, float h) { _pMainRT->Resize(D2D1::SizeU(w, h)); }

		ID2D1RenderTarget* GetMainRT() { return _pMainRT; }

		ID2D1RenderTarget* ChooseRT(ID2D1RenderTarget* pRT) { return pRT == NULL ? _pMainRT : pRT; }

		void BeginDraw(ID2D1RenderTarget* pRT)
		{
			pRT->BeginDraw();
			pRT->Clear();
		}

		void EndDraw(ID2D1RenderTarget* pRT)
		{
			pRT->EndDraw();
		}

		void PushClipRect(ID2D1RenderTarget* pRT, ImFloat4 rt)
		{
			ID2D1RenderTarget* pRenderTarget = ChooseRT(pRT);
			pRenderTarget->PushAxisAlignedClip(rt.ToD2DRectF(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		}

		void PopClipRect(ID2D1RenderTarget* pRT)
		{
			ID2D1RenderTarget* pRenderTarget = ChooseRT(pRT);
			pRenderTarget->PopAxisAlignedClip();
		}

		void SetPenWidth(float width)
		{
			_penWidth = width;
		}

		void DrawPoint(ID2D1RenderTarget* pRT, ImFloat4 color, const ImFloat2& pt)
		{
			ID2D1RenderTarget* pRenderTarget = ChooseRT(pRT);
			_pCommonBrush->SetColor(color.ToD2DColorF());
			pRenderTarget->DrawRectangle(D2D1::RectF(pt.x, pt.y, pt.x, pt.y), _pCommonBrush, _penWidth);
		}

		void DrawLine(ID2D1RenderTarget* pRT, ImFloat4 color, const ImFloat2& pt1, const ImFloat2& pt2)
		{
			ID2D1RenderTarget* pRenderTarget = ChooseRT(pRT);
			_pCommonBrush->SetColor(color.ToD2DColorF());
			pRenderTarget->DrawLine(D2D1::Point2F(pt1.x, pt1.y), D2D1::Point2F(pt2.x, pt2.y), _pCommonBrush, _penWidth);
		}

		void DrawRect(ID2D1RenderTarget* pRT, ImFloat4 color, ImFloat4 rt, bool isFilled = false)
		{
			ID2D1RenderTarget* pRenderTarget = ChooseRT(pRT);
			_pCommonBrush->SetColor(color.ToD2DColorF());

			if (isFilled)
				pRenderTarget->FillRectangle(rt.ToD2DRectF(), _pCommonBrush);
			else
				pRenderTarget->DrawRectangle(rt.ToD2DRectF(), _pCommonBrush, _penWidth);
		}

		void DrawRoundedRect(ID2D1RenderTarget* pRT, ImFloat4 color, ImFloat4 rt, float radiusX, float radiusY, bool isFilled = false)
		{
			ID2D1RenderTarget* pRenderTarget = ChooseRT(pRT);
			_pCommonBrush->SetColor(color.ToD2DColorF());

			D2D1_ROUNDED_RECT roundRect = D2D1::RoundedRect(rt.ToD2DRectF(), radiusX, radiusY);
			if (isFilled)
				pRenderTarget->FillRoundedRectangle(roundRect, _pCommonBrush);
			else
				pRenderTarget->DrawRoundedRectangle(roundRect, _pCommonBrush, _penWidth);
		}

		void DrawEllipse(ID2D1RenderTarget* pRT, ImFloat4 color, float x, float y, float radiusX, float radiusY, bool isFilled = false)
		{
			ID2D1RenderTarget* pRenderTarget = ChooseRT(pRT);
			_pCommonBrush->SetColor(color.ToD2DColorF());

			D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radiusX, radiusY);
			if (isFilled)
				pRenderTarget->FillEllipse(ellipse, _pCommonBrush);
			else
				pRenderTarget->DrawEllipse(ellipse, _pCommonBrush, _penWidth);
		}

		void DrawTriangle(ID2D1RenderTarget* pRT, ImFloat4 color, const ImFloat2 &pt1, const ImFloat2 &pt2, const ImFloat2 &pt3, bool isFilled = false)
		{
			ImFloat2 arrayPoint[3];
			arrayPoint[0] = pt1;
			arrayPoint[1] = pt2;
			arrayPoint[2] = pt3;
			DrawPolygon(pRT, color, arrayPoint, 3, isFilled);
		}

		void DrawPolygon(ID2D1RenderTarget* pRT, ImFloat4 color, ImFloat2* array, ImUint count, bool isFilled = false)
		{
			ID2D1RenderTarget* pRenderTarget = ChooseRT(pRT);
			_pCommonBrush->SetColor(color.ToD2DColorF());

			D2D1_POINT_2F* D2DPoints = new D2D1_POINT_2F[count];
			if (D2DPoints != NULL)
			{
				for (ImUint i = 0; i < count; i++)
					D2DPoints[i] = D2D1::Point2F(array[i].x, array[i].y);

				ID2D1PathGeometry* pGeometry = NULL;
				HRESULT hr = _pD2DFactory->CreatePathGeometry(&pGeometry);
				if (SUCCEEDED(hr))
				{
					ID2D1GeometrySink *pSink = NULL;
					hr = pGeometry->Open(&pSink);
					if (SUCCEEDED(hr))
					{
						pSink->BeginFigure(D2DPoints[0], D2D1_FIGURE_BEGIN_FILLED);
						pSink->AddLines(D2DPoints, count);
						pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
					}
					pSink->Close();
				}

				if (isFilled)
					pRenderTarget->FillGeometry(pGeometry, _pCommonBrush);
				else
					pRenderTarget->DrawGeometry(pGeometry, _pCommonBrush, _penWidth);

				SafeRelease(&pGeometry);
				delete[] D2DPoints;
			}
		}

		void DrawPolygonalLine(ID2D1RenderTarget* pRT, ImFloat4 color, ImFloat2* array, ImUint count)
		{
			ID2D1RenderTarget* pRenderTarget = ChooseRT(pRT);
			_pCommonBrush->SetColor(color.ToD2DColorF());

			ID2D1PathGeometry* pGeometry = NULL;
			D2D1_POINT_2F* D2DPoints = new D2D1_POINT_2F[count];
			if (D2DPoints != NULL)
			{
				for (ImUint i = 0; i < count; i++)
					D2DPoints[i] = D2D1::Point2F(array[i].x, array[i].y);

				HRESULT hr = _pD2DFactory->CreatePathGeometry(&pGeometry);
				if (SUCCEEDED(hr))
				{
					ID2D1GeometrySink *pSink = NULL;
					hr = pGeometry->Open(&pSink);
					if (SUCCEEDED(hr))
					{
						pSink->BeginFigure(D2DPoints[0], D2D1_FIGURE_BEGIN_HOLLOW);
						pSink->AddLines(D2DPoints, count);
						pSink->EndFigure(D2D1_FIGURE_END_OPEN);
					}
					pSink->Close();
				}

				pRenderTarget->DrawGeometry(pGeometry, _pCommonBrush, _penWidth);

				SafeRelease(&pGeometry);
				delete[] D2DPoints;
			}
		}

		void DrawText(ID2D1RenderTarget* pRT, ImFloat4 color, std::string txt, const ImFloat4& rt, TEXT_ALIGNMENT_MODE mode = D2DRender::MODE_CENTER)
		{
			ID2D1RenderTarget* pRenderTarget = ChooseRT(pRT);
			_pCommonBrush->SetColor(color.ToD2DColorF());

			switch (mode)
			{
			case D2DRender::MODE_LEFT:
				_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
				break;
			case D2DRender::MODE_CENTER:
				_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
				break;
			case D2DRender::MODE_RIGHT:
				_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
				break;
			default:
				break;
			}

			pRenderTarget->DrawText(ATOW(txt).c_str(), ATOW(txt).length(), _pTextFormat,rt. ToD2DRectF(), _pCommonBrush);
		}

		ImFloat2 GetTextSize(std::string txt)
		{
			ImFloat2 size;
			IDWriteTextLayout* textLayout = NULL;

			HRESULT hr = _pDWriteFactory->CreateTextLayout(ATOW(txt).c_str(), ATOW(txt).length(), _pTextFormat, 0,	0, &textLayout);
			if (textLayout != NULL)
			{
				DWRITE_TEXT_METRICS textMetrics;
				textLayout->GetMetrics(&textMetrics);
				size.x = ceil(textMetrics.widthIncludingTrailingWhitespace);
				size.y = ceil(textMetrics.height);
			}
			SafeRelease(&textLayout);
			return size;
		}

		void DrawImage(ID2D1RenderTarget* pRT, std::string image, float x = 0, float y = 0, float w = 0, float h = 0)
		{
			// find image object
			assert(!image.empty());
			ID2D1Bitmap* pBitmap = NULL;

			auto iter = _mapBitmaps.find(image);
			if (iter == _mapBitmaps.end())
			{
				_mapBitmaps[image] = LoadImage(image, w, h);
				assert(_mapBitmaps[image] != NULL);
			}				

			pBitmap = _mapBitmaps[image];

			// draw image object
			ID2D1RenderTarget* pRenderTarget = ChooseRT(pRT);

			if (w == 0 || h == 0)
			{
				w = pBitmap->GetSize().width;
				h = pBitmap->GetSize().height;
			}
			pRenderTarget->DrawBitmap(pBitmap, D2D1::RectF(x, y, x + w, y + h));
		}

		ID2D1Bitmap* LoadImage(std::string image, UINT width, UINT height)
		{
			ID2D1Bitmap* pBitmap = nullptr;
			IWICBitmapDecoder *pDecoder = nullptr;
			IWICBitmapFrameDecode *pSource = nullptr;
			IWICStream *pStream = nullptr;
			IWICFormatConverter *pConverter = nullptr;
			IWICBitmapScaler *pScaler = nullptr;

			HRESULT hr = _pWICFactory->CreateDecoderFromFilename(
				ATOW(image).c_str(),
				nullptr,
				GENERIC_READ,
				WICDecodeMetadataCacheOnLoad,
				&pDecoder);

			if (SUCCEEDED(hr))
				hr = pDecoder->GetFrame(0, &pSource);

			if (SUCCEEDED(hr))
				hr = _pWICFactory->CreateFormatConverter(&pConverter);

			if (SUCCEEDED(hr))
			{
				if (width != 0 || height != 0)
				{
					UINT originalWidth, originalHeight;
					hr = pSource->GetSize(&originalWidth, &originalHeight);
					if (SUCCEEDED(hr))
					{
						if (width == 0)
						{
							FLOAT scalar = static_cast<FLOAT>(height) / static_cast<FLOAT>(originalHeight);
							width = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
						}
						else if (height == 0)
						{
							FLOAT scalar = static_cast<FLOAT>(width) / static_cast<FLOAT>(originalWidth);
							height = static_cast<UINT>(scalar * static_cast<FLOAT>(originalHeight));
						}

						hr = _pWICFactory->CreateBitmapScaler(&pScaler);
						if (SUCCEEDED(hr))
							hr = pScaler->Initialize(pSource, width, height, WICBitmapInterpolationModeCubic);
						if (SUCCEEDED(hr))
							hr = pConverter->Initialize(pScaler, GUID_WICPixelFormat32bppPBGRA, 
								WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeMedianCut);
					}
				}
				else
				{
					hr = pConverter->Initialize(
						pSource,
						GUID_WICPixelFormat32bppPBGRA,
						WICBitmapDitherTypeNone,
						nullptr,
						0.f,
						WICBitmapPaletteTypeMedianCut);
				}
			}

			if (SUCCEEDED(hr))
				hr = _pMainRT->CreateBitmapFromWicBitmap(pConverter, nullptr, &pBitmap);

			SafeRelease(&pDecoder);
			SafeRelease(&pSource);
			SafeRelease(&pStream);
			SafeRelease(&pConverter);
			SafeRelease(&pScaler);

			return pBitmap;
		}

	private:

		ImFloat4				_bgColor;
		float					_penWidth;

		ID2D1Factory*			_pD2DFactory;
		IDWriteFactory*			_pDWriteFactory;
		IWICImagingFactory*		_pWICFactory;
		ID2D1HwndRenderTarget*	_pMainRT;

		IDWriteTextFormat*		_pTextFormat;
		ID2D1SolidColorBrush*	_pCommonBrush;
		std::unordered_map<std::string, ID2D1Bitmap*> _mapBitmaps;
	};

	//////////////////////////////////////////////////////////////////////////

	Window* GetWindow(const char* name)
	{
		for (size_t i = 0; i != s_state.Windows.size(); i++)
			if (strcmp(s_state.Windows[i]->Name, name) == 0)
				return s_state.Windows[i];
		return NULL;
	}

	void InitResources(ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory, IWICImagingFactory* pWICFactory, ID2D1HwndRenderTarget* pMainRT)
	{
		QueryPerformanceFrequency(&s_frequency);

		s_state.CurrentWindow = NULL;
		s_state.RenderWindow = NULL;
		s_state.HoveredWindow = NULL;
		memset(s_state.StrToolTip, 0, sizeof(s_state.StrToolTip));

		s_state.Render = new D2DRender;
		s_state.Render->Init(pD2DFactory, pDWriteFactory, pWICFactory, pMainRT);
	}

	void ClearResources()
	{
		for (size_t i = 0; i < s_state.Windows.size(); i++)
			delete s_state.Windows[i];
		s_state.Windows.clear();

		delete s_state.Render;
		s_state.Render = NULL;
	}

	Event& GetEvents()
	{
		return s_state.Events;
	}

	float GetFPS()
	{
		return s_state.FPS;
	}

	void CalculateFramesPerSecond()
	{
		LARGE_INTEGER time;
		QueryPerformanceCounter(&time);
		s_times.Add(time.QuadPart);

		static LONGLONG sc_lastTimeStatusShown = 0;
		if (s_times.GetCount() > 0 && s_times.GetLast() > sc_lastTimeStatusShown + 1000000)
		{
			sc_lastTimeStatusShown = s_times.GetLast();
			if (s_times.GetCount() > 0)
				s_state.FPS = (s_times.GetCount() - 1) * s_frequency.QuadPart / static_cast<float>((s_times.GetLast() - s_times.GetFirst()));
		}
	}

	void NewFrame()
	{
		s_state.HoveredId = 0;
		s_state.StrToolTip[0] = '\0';

		CalculateFramesPerSecond();

		// update event states
		s_state.Events.MouseDelta = s_state.Events.MousePos - s_state.Events.MousePosPrev;
		s_state.Events.MousePosPrev = s_state.Events.MousePos;
		s_state.Events.MouseDownTime = s_state.Events.MouseDown ? (s_state.Events.MouseDownTime < 0.0f ? 0.0f : s_state.Events.MouseDownTime + 1 / 60.f) : -1.0f;
		s_state.Events.MouseClicked = (s_state.Events.MouseDownTime == 0.0f);
		s_state.Events.MouseDoubleClicked = false;
		if (s_state.Events.MouseClicked)
		{
			if (0 - s_state.Events.MouseClickedTime < s_state.Events.MouseDoubleClickTime)
			{
				if (Distance(s_state.Events.MousePos - s_state.Events.MouseClickedPos) < s_state.Events.MouseDoubleClickMaxDist)
					s_state.Events.MouseDoubleClicked = true;
				s_state.Events.MouseClickedTime = -FLT_MAX;
			}
			else
			{
				s_state.Events.MouseClickedTime = 0;
				s_state.Events.MouseClickedPos = s_state.Events.MousePos;
			}
		}

		for (int i = (int)s_state.Windows.size() - 1; i >= 0; i--)
		{
			if (PtInRect(s_state.Events.MousePos, s_state.Windows[i]->Rect))
			{
				s_state.HoveredWindow = s_state.Windows[i];
				break;
			}
		}

		if (s_state.Events.MouseClicked)
		{
			for (int i = (int)s_state.Windows.size() - 1; i >=0; i--)
			{
				if (PtInRect(s_state.Events.MousePos, s_state.Windows[i]->Rect))
				{
					s_state.Windows.push_back(s_state.Windows[i]);
					s_state.Windows.erase(s_state.Windows.begin() + i);

					break;
				}
			}
		}
	}

	void SetBgImage(std::string image, bool is_resized)
	{
		s_state.BgImage = image;
	}

	void Render()
	{
		// image bg
		if (!s_state.BgImage.empty())
		{
			s_state.Render->DrawImage(NULL, s_state.BgImage, 0, 0, s_state.Render->GetMainRT()->GetSize().width, s_state.Render->GetMainRT()->GetSize().height);
		}

		// windows
		for (size_t i = 0; i < s_state.Windows.size(); i++)
		{
			if (s_state.Windows[i]->Visible)
			{
				ID2D1Bitmap* pBitmap = NULL;
				s_state.Windows[i]->CRT->GetBitmap(&pBitmap);
				s_state.Render->GetMainRT()->DrawBitmap(pBitmap, s_state.Windows[i]->Rect.ToD2DRectF(), s_state.Windows[i]->Alpha);
				SafeRelease(&pBitmap);
			}
			s_state.Windows[i]->Visible = false;
		}

		// tooltip
		if (s_state.StrToolTip[0])
		{
			const ImFloat2 text_size = s_state.Render->GetTextSize(s_state.StrToolTip);
			ImFloat2 pos = s_state.Events.MousePos + ImFloat2(32, 16);
			ImFloat4 bb(pos - s_state.Styles.FramePadding * 2, text_size + s_state.Styles.FramePadding * 2);

			s_state.Render->DrawRoundedRect(NULL, s_state.Styles.Colors[Color_TooltipBg], bb, 5, 5, true);
			s_state.Render->DrawText(NULL, s_state.Styles.Colors[Color_Text], s_state.StrToolTip, bb);
		}
	}

	void PushItemWidth(float width)
	{
		Window* window = s_state.RenderWindow;
		window->Layout.ItemWidth.push_back(width > 0.0f ? width : window->ItemWidthDefault);
	}

	void PopItemWidth()
	{
		Window* window = s_state.RenderWindow;
		window->Layout.ItemWidth.pop_back();
	}

	void Shutdown()
	{
		ClearResources();
	}

	bool WindowCloseButton(bool* open)
	{
		Window* window = s_state.RenderWindow;

		const ImUint id = window->GetID("##CLOSE");

		const float title_bar_height = s_state.Styles.TitleBarHeight;
		const ImFloat4 bb(window->Rect.z - 20 + 2, 2, title_bar_height - 4, title_bar_height - 4);

		bool hovered, held;
		bool pressed = WidgetMouseEvent(bb, id, &hovered, &held, false);

		// Render
		const GuiStyle& style = s_state.Styles;
		const ImFloat4 col = style.Colors[(held && hovered) ? Color_ButtonActive : hovered ? Color_ButtonHovered : Color_Button];
		s_state.Render->DrawRect(window->CRT, col, bb, true);

		s_state.Render->DrawLine(window->CRT, style.Colors[Color_Text], ImFloat2(bb.x + 4, bb.y + 4), ImFloat2(bb.x + bb.z - 4, bb.y + bb.w - 4));
		s_state.Render->DrawLine(window->CRT, style.Colors[Color_Text], ImFloat2(bb.x + bb.z - 4, bb.y + 4), ImFloat2(bb.x + 4, bb.y + bb.w - 4));

		if (open != NULL && pressed)
			*open = !*open;

		return pressed;
	}

	void DrawWindowState(bool collapse)
	{
		float collapse_radius = 6;
		ImFloat2 collapse_state_center(s_state.Styles.TitleBarHeight / 2, s_state.Styles.TitleBarHeight / 2);
		ImFloat4 collapse_state_rect(s_state.Styles.TitleBarHeight / 2 - collapse_radius, s_state.Styles.TitleBarHeight / 2 - collapse_radius, collapse_radius * 2, collapse_radius * 2);

		s_state.Render->DrawRect(s_state.RenderWindow->CRT, s_state.Styles.Colors[Color_Text], collapse_state_rect, true);
		s_state.Render->DrawLine(s_state.RenderWindow->CRT, s_state.Styles.Colors[Color_WindowBg], collapse_state_center - ImFloat2(4, 0), collapse_state_center + ImFloat2(4, 0));

		if (collapse)
			s_state.Render->DrawLine(s_state.RenderWindow->CRT, s_state.Styles.Colors[Color_WindowBg], collapse_state_center - ImFloat2(0, 4), collapse_state_center + ImFloat2(0, 4));
	}

	void DrawCollapseState(ImFloat2 pos, float offset, float height, bool open, float scale)
	{
		float length = height * 0.8f;

		ImFloat2 a, b, c;
		if (open)
		{
			a = ImFloat2(offset, length / 2);
			b = ImFloat2(offset + length / 2, length / 2 + 8);
			c = ImFloat2(offset + length, length / 2);

			a = a + pos + ImFloat2(0, height * 0.1f) - ImFloat2(length * 0.5f - 8 * 0.5f, 8 * 0.5f);
			b = b + pos + ImFloat2(0, height * 0.1f) - ImFloat2(length * 0.5f - 8 * 0.5f, 8 * 0.5f);
			c = c + pos + ImFloat2(0, height * 0.1f) - ImFloat2(length * 0.5f - 8 * 0.5f, 8 * 0.5f);
		}
		else
		{
			a = ImFloat2(offset, 0);
			b = ImFloat2(offset + 8, length / 2);
			c = ImFloat2(offset, length);

			a = a + pos + ImFloat2(0, height * 0.1f);
			b = b + pos + ImFloat2(0, height * 0.1f);
			c = c + pos + ImFloat2(0, height * 0.1f);
		}

		ImFloat2 pt_arr[3] = { a, b, c };
		s_state.Render->DrawPolygonalLine(s_state.RenderWindow->CRT, s_state.Styles.Colors[Color_Text], pt_arr, 3);
	}

	void DrawWidgetFrame(ImFloat4 rect, ImUint fill_col, bool border)
	{
		Window* window = s_state.RenderWindow;
		s_state.Render->DrawRect(window->CRT, s_state.Styles.Colors[fill_col], rect, true);
		if (window->Flags & ImDuiWindowFlags_ShowBorders)
		{
			D2D1_ANTIALIAS_MODE old_mode = window->CRT->GetAntialiasMode();
			window->CRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
			s_state.Render->DrawRect(window->CRT, s_state.Styles.Colors[Color_Border], rect, false);
			window->CRT->SetAntialiasMode(old_mode);
		}
	}

	bool BeginWindow(const char* name, bool* p_open, ImFloat2 pos, ImFloat2 size, float fill_alpha, ImDuiWindowFlags flags)
	{
		Window* window = GetWindow(name);
		if (!window)
		{
			ImFloat2 posReal;
			if (pos.x == 0 && pos.y == 0)
				posReal = ImFloat2(60, 60);
			else
				posReal = pos;

			ImFloat2 sizeReal;
			if (size.x == 0 && size.y == 0)
				sizeReal = ImFloat2(250, 250);
			else
				sizeReal = size;

			window = new Window(name, posReal, sizeReal);
			s_state.Windows.push_back(window);
		}

		window->Flags = (ImDuiWindowFlags)flags;

		if (fill_alpha < 0.0f)
			fill_alpha = s_state.Styles.DefaultWindowAlpha;

		window->Alpha = fill_alpha;
		window->Visible = true;
		window->ItemWidthDefault = (float)(int)(window->Rect.z > 0.0f ? window->Rect.z * 0.65f : 250.0f);
		s_state.RenderWindow = window;

		// window collapse
		if (!(window->Flags & ImDuiWindowFlags_NoTitleBar))
		{
			if (s_state.Events.MouseDoubleClicked && PtInRect(s_state.Events.MousePos, ImFloat4(window->Rect.x, window->Rect.y, window->Rect.z, s_state.Styles.TitleBarHeight)))
			{
				window->Collapse = !window->Collapse;
			}
		}
		else
		{
			window->Collapse = false;
		}

		// move window
		const ImUint move_id = window->GetID("#MOVE");
		if (s_state.ActiveId == move_id)
		{
			if (s_state.Events.MouseDown)
			{
				if (!(window->Flags & ImDuiWindowFlags_NoMove))
				{
					window->Rect.x += s_state.Events.MouseDelta.x;
					window->Rect.y += s_state.Events.MouseDelta.y;
				}
			}
			else
			{
				s_state.ActiveId = 0;
			}
		}

		// resize grip
		ImFloat4 resize_col;
		if (!window->Collapse && !(window->Flags & ImDuiWindowFlags_NoResize))
		{
			const ImFloat4 resize_rect(ImFloat2(window->Rect.z, window->Rect.w) - s_state.Styles.ResizeGripSize, s_state.Styles.ResizeGripSize);
			const ImUint resize_id = window->GetID("##RESIZE");

			bool hovered, held;
			WidgetMouseEvent(resize_rect, resize_id, &hovered, &held, false);
			resize_col = s_state.Styles.Colors[held ? Color_ResizeGripActive : hovered ? Color_ResizeGripHovered : Color_ResizeGrip];
			if (held)
			{
				ImFloat2 tmpSize(window->Rect.z, window->Rect.w);
				ImFloat2 realSize = Max(tmpSize + s_state.Events.MouseDelta, s_state.Styles.WindowMinSize);
				window->Rect.z = realSize.x;
				window->Rect.w = realSize.y;
				window->Resize(realSize);
			}
		}

		// Setup drawing context
		window->Layout.CursorStartPos = ImFloat2(s_state.Styles.WindowPadding.x, 
			((window->Flags & ImDuiWindowFlags_NoTitleBar) ? 0 : s_state.Styles.TitleBarHeight) + s_state.Styles.WindowPadding.y);
		window->Layout.CursorPos = window->Layout.CursorStartPos;
		window->Layout.CursorPosPrevLine = window->Layout.CursorPos;
		window->Layout.CurrentLineHeight = window->Layout.PrevLineHeight = 0.0f;
		window->Layout.ItemWidth.resize(0);
		window->Layout.ItemWidth.push_back(window->ItemWidthDefault);

		// Draw

		float x = 0;
		float y = 0;
		float w = window->Rect.z;
		float h = window->Rect.w;

		ImFloat4 rect_title_bar(x + 0, y + 0, w, s_state.Styles.TitleBarHeight);
		ImFloat4 rect_title_text(x + 20, y + 0, w, s_state.Styles.TitleBarHeight);
		ImFloat4 rect_window_bg(x, y, w, h);

		s_state.Render->BeginDraw(window->CRT);

		if (window->Collapse)
		{
			s_state.Render->DrawRect(window->CRT, s_state.Styles.Colors[Color_TitleBarCollapsed], rect_title_bar, true);
			if (window->Flags & ImDuiWindowFlags_ShowBorders)
				s_state.Render->DrawRect(window->CRT, s_state.Styles.Colors[Color_Border], rect_title_bar, false);
		}
		else
		{
			s_state.Render->DrawRect(window->CRT, s_state.Styles.Colors[Color_WindowBg], rect_window_bg, true);
			s_state.Render->DrawTriangle(window->CRT, resize_col, ImFloat2(w - s_state.Styles.ResizeGripSize.x, h), ImFloat2(w, h), ImFloat2(w, h - s_state.Styles.ResizeGripSize.y), true);
			if (!(window->Flags & ImDuiWindowFlags_NoTitleBar))
				s_state.Render->DrawRect(window->CRT, s_state.Styles.Colors[Color_TitleBar], rect_title_bar, true);

			if (window->Flags & ImDuiWindowFlags_ShowBorders)
				s_state.Render->DrawRect(window->CRT, s_state.Styles.Colors[Color_Border], rect_window_bg, false);
		}

		// title bar
		if (!(window->Flags & ImDuiWindowFlags_NoTitleBar))
		{
			DrawWindowState(window->Collapse);
			s_state.Render->DrawText(window->CRT, s_state.Styles.Colors[Color_Text], name, rect_title_text, D2DRender::MODE_LEFT);
			if (p_open)
				WindowCloseButton(p_open);
		}

		return true;
	}

	void EndWindow()
	{
		Window* window = s_state.RenderWindow;

		s_state.Render->EndDraw(window->CRT);

		if (s_state.ActiveId == 0 && s_state.HoveredId == 0 && PtInRect(s_state.Events.MousePos, window->Rect) && s_state.Events.MouseClicked)
			s_state.ActiveId = window->GetID("#MOVE");
		s_state.RenderWindow = NULL;
	}

	//////////////////////////////////////////////////////////////////////////

	void ItemSize(ImFloat2 size, ImFloat2* adjust_start_offset)
	{
		Window* window = s_state.RenderWindow;
		if (window->Collapse)
			return;

		const float line_height = Max(window->Layout.CurrentLineHeight, size.y);
		if (adjust_start_offset)
			adjust_start_offset->y = adjust_start_offset->y + (line_height - size.y) * 0.5f;

		// Always align ourselves on pixel boundaries
		window->Layout.CursorPosPrevLine = ImFloat2(window->Layout.CursorPos.x + size.x, window->Layout.CursorPos.y);
		window->Layout.CursorPos = ImFloat2(s_state.Styles.WindowPadding.x, window->Layout.CursorPos.y + line_height + s_state.Styles.ItemSpacing.y);

		window->Layout.PrevLineHeight = line_height;
		window->Layout.CurrentLineHeight = 0.0f;
	}

	void ItemSize(const ImFloat4& aabb, ImFloat2* adjust_start_offset)
	{
		ItemSize(ImFloat2(aabb.z, aabb.w), adjust_start_offset);
	}

	void SameLine(int column_x, int spacing_w)
	{
		Window* window = s_state.RenderWindow;
		if (window->Collapse)
			return;

		float x, y;
		if (column_x != 0)
		{
			if (spacing_w < 0) spacing_w = 0;
			x = (float)column_x + (float)spacing_w;
			y = window->Layout.CursorPosPrevLine.y;
		}
		else
		{
			if (spacing_w < 0) spacing_w = (int)s_state.Styles.ItemSpacing.x;
			x = window->Layout.CursorPosPrevLine.x + (float)spacing_w;
			y = window->Layout.CursorPosPrevLine.y;
		}
		window->Layout.CurrentLineHeight = window->Layout.PrevLineHeight;
		window->Layout.CursorPos = ImFloat2(x, y);
	}

	void Spacing()
	{
		Window* window = s_state.RenderWindow;
		if (window->Collapse)
			return;

		ItemSize(ImFloat2(0, 0));
	}

	//////////////////////////////////////////////////////////////////////////

	void TextV(const char* fmt, va_list args)
	{
		Window* window = s_state.RenderWindow;
		if (window->Collapse)
			return;

		static char buf[1024];
		FormatStringV(buf, ARRAYSIZE(buf), fmt, args);

		const ImFloat2 fontsize = s_state.Render->GetTextSize(buf);
		const ImFloat4 fontrt(window->Layout.CursorPos, fontsize);
		ItemSize(fontrt);
	
		s_state.Render->DrawText(window->CRT, s_state.Styles.Colors[Color_Text], buf, fontrt, D2DRender::MODE_LEFT);
	}

	void Text(const char* label, ...)
	{
		va_list args;
		va_start(args, label);
		TextV(label, args);
		va_end(args);
	}

	bool WidgetMouseEvent(ImFloat4 bb, const ImUint id, bool* out_hovered, bool* out_held, bool repeat)
	{
		Window* window = s_state.RenderWindow;

		bb.x += window->Rect.x;
		bb.y += window->Rect.y;

		const bool hovered = (s_state.HoveredWindow == window) && PtInRect(s_state.Events.MousePos, bb);
		bool pressed = false;
		if (hovered)
		{
			s_state.HoveredId = id;
			if (s_state.Events.MouseClicked)
				s_state.ActiveId = id;
			else if (repeat && s_state.ActiveId)
				pressed = true;
		}

		bool held = false;
		if (s_state.ActiveId == id)
		{
			if (s_state.Events.MouseDown)
			{
				held = true;
			}
			else
			{
				if (hovered)
					pressed = true;
				s_state.ActiveId = 0;
			}
		}

		if (out_hovered) *out_hovered = hovered;
		if (out_held) *out_held = held;

		return pressed;
	}

	bool Button(const char* label, ImFloat2 size)
	{
		Window* window = s_state.RenderWindow;
		if (window->Collapse)
			return false;

		const ImUint id = window->GetID(label);
		ImFloat2 text_size = s_state.Render->GetTextSize(label);

		if (size.x == 0.0f)
			size.x = text_size.x;
		if (size.y == 0.0f)
			size.y = text_size.y;

		ImFloat4 boundRect(window->Layout.CursorPos, size + s_state.Styles.FramePadding * 2);
		ItemSize(boundRect);

		bool hovered, held;
		bool pressed = WidgetMouseEvent(boundRect, id, &hovered, &held, false);

		DrawWidgetFrame(boundRect,(hovered && held) ? Color_ButtonActive : hovered ? Color_ButtonHovered : Color_Button);
		s_state.Render->DrawText(window->CRT, s_state.Styles.Colors[Color_Text], label, boundRect);

		return pressed;
	}

	void CheckBox(const char* label, bool* v)
	{
		Window* window = s_state.RenderWindow;
		if (window->Collapse)
			return;

		const GuiStyle& style = s_state.Styles;
		const unsigned int id = window->GetID(label);

		const ImFloat2 text_size = s_state.Render->GetTextSize(label);
		ImFloat4 check_bb(window->Layout.CursorPos.x, window->Layout.CursorPos.y, text_size.y + style.FramePadding.y * 2, text_size.y + style.FramePadding.y * 2);
		ItemSize(check_bb);
		SameLine(0, style.ItemInnerSpacing.x);

		ImFloat4 text_bb(window->Layout.CursorPos.x, window->Layout.CursorPos.y + style.FramePadding.y, text_size.x, style.FramePadding.y + text_size.y);
		ItemSize(text_bb);

		DrawWidgetFrame(check_bb, Color_WidgetBg);

		ImFloat4 hovered_rect(check_bb.x + window->Rect.x, check_bb.y + window->Rect.y, check_bb.z, check_bb.w);
		const bool hovered = (s_state.HoveredWindow == window) && PtInRect(s_state.Events.MousePos, hovered_rect);
		const bool pressed = hovered && s_state.Events.MouseClicked;
		if (hovered)
			s_state.HoveredId = id;
		if (pressed)
		{
			*v = !(*v);
			s_state.ActiveId = 0;
		}

		if (*v)
		{
			ImFloat4 fillRect;
			fillRect.x = check_bb.x + 4;
			fillRect.y = check_bb.y + 4;
			fillRect.z = check_bb.z - 4 * 2;
			fillRect.w = check_bb.w - 4 * 2;
			s_state.Render->DrawRect(window->CRT, style.Colors[Color_WidgetActive], fillRect, true);
		}

		if (!IsHideText(label))
		{
			s_state.Render->DrawText(window->CRT, s_state.Styles.Colors[Color_Text], label, text_bb);
		}
	}

	bool RadioButton(const char* label, bool active)
	{
		Window* window = s_state.RenderWindow;
		if (window->Collapse)
			return false;

		const GuiStyle& style = s_state.Styles;
		const unsigned int id = window->GetID(label);

		ImFloat2 text_size = s_state.Render->GetTextSize(label);
		ImFloat4 check_bb(window->Layout.CursorPos.x, window->Layout.CursorPos.y, text_size.y + style.FramePadding.y * 2 - 1, text_size.y + style.FramePadding.y * 2 - 1);
		ItemSize(check_bb);
		SameLine(0, style.ItemInnerSpacing.x);

		ImFloat4 text_bb(window->Layout.CursorPos.x, window->Layout.CursorPos.y + style.FramePadding.y, text_size.x, style.FramePadding.y + text_size.y);
		ItemSize(text_bb);

		ImFloat2 center(check_bb.x + check_bb.z / 2, check_bb.y + check_bb.w / 2);
		center.x = (float)(int)center.x + 0.5f;
		center.y = (float)(int)center.y + 0.5f;
		const float radius = check_bb.w * 0.5f;

		ImFloat4 hovered_rect(check_bb.x + window->Rect.x, check_bb.y + window->Rect.y, check_bb.z, check_bb.w);
		const bool hovered = (s_state.HoveredWindow == window) && PtInRect(s_state.Events.MousePos, hovered_rect);
		const bool pressed = hovered && s_state.Events.MouseClicked;
		if (hovered)
			s_state.HoveredId = id;

		s_state.Render->DrawEllipse(window->CRT, style.Colors[Color_WidgetBg], center.x, center.y, radius, radius, true);
		if (active)
			s_state.Render->DrawEllipse(window->CRT, style.Colors[Color_WidgetActive], center.x, center.y, radius - 4, radius - 4, true);

		if (window->Flags & ImDuiWindowFlags_ShowBorders)
			s_state.Render->DrawEllipse(window->CRT, style.Colors[Color_Border], center.x, center.y, radius, radius, false);

		if (!IsHideText(label))
			s_state.Render->DrawText(window->CRT, s_state.Styles.Colors[Color_Text], label, text_bb);
		
		return pressed;
	}

	bool RadioButton(const char* label, int* v, int v_button)
	{
		const bool pressed = ImDui::RadioButton(label, *v == v_button);
		if (pressed)
			*v = v_button;

		return pressed;
	}

	bool Collapse(const char* label, const char* str_id, const bool display_frame, const bool default_open)
	{
		Window* window = s_state.RenderWindow;
		if (window->Collapse)
			return false;

		const GuiStyle& style = s_state.Styles;

		assert(str_id != NULL || label != NULL);
		if (str_id == NULL)	str_id = label;
		if (label == NULL) label = str_id;
		const unsigned int id = window->GetID(str_id);

		bool opened;
		opened = window->StateStorage.GetValue(id, default_open) != 0;

		const ImFloat2 text_size = s_state.Render->GetTextSize(label);
		const ImFloat2 pos_min = window->Layout.CursorPos;
		const ImFloat2 pos_max(window->Rect.z - style.WindowPadding.x * 2, window->Rect.w);
		ImFloat4 bb = ImFloat4(pos_min.x, pos_min.y, pos_max.x, text_size.y);
		if (display_frame)
		{
			bb.x -= style.FramePadding.x * 0.5f;
			bb.z += style.FramePadding.x;
			bb.w += style.FramePadding.y * 2;
		}

		ImFloat4 label_box;
		label_box.x = bb.x + style.FontSize + 10;
		label_box.y = bb.y;
		label_box.z = bb.z - style.FontSize - 10;
		label_box.w = bb.w;

		const ImFloat4 text_bb(bb.x, bb.y, style.FontSize + style.FramePadding.x * 2 * 2 + text_size.x, text_size.y);
		ItemSize(ImFloat2(text_bb.z, bb.w));

		bool hovered, held;
		bool pressed = WidgetMouseEvent(display_frame ? bb : text_bb, id, &hovered, &held);
		if (pressed)
		{
			opened = !opened;
			window->StateStorage.SetValue(id, opened);
		}

		// Render
		const ImFloat4 col = style.Colors[(held && hovered) ? Color_CollapseActive : hovered ? Color_CollapseHovered : Color_Collapse];
		if (display_frame)
		{
			DrawWidgetFrame(bb, (held && hovered) ? Color_CollapseActive : hovered ? Color_CollapseHovered : Color_Collapse);
			DrawCollapseState(pos_min, bb.z - 50, bb.w, opened);
			s_state.Render->DrawText(window->CRT, style.Colors[Color_Text], label, label_box, ImDui::D2DRender::MODE_LEFT);
		}
		else
		{
			if ((held && hovered) || hovered)
				s_state.Render->DrawRect(window->CRT, col, bb, true);
			DrawCollapseState(pos_min, bb.z - 50, bb.w, opened);
			s_state.Render->DrawText(window->CRT, style.Colors[Color_Text], label, label_box, ImDui::D2DRender::MODE_LEFT);
		}

		return opened;
	}

	bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* display_format, float power)
	{
		Window* window = s_state.RenderWindow;
		if (window->Collapse)
			return false;

		const GuiStyle& style = s_state.Styles;
		const unsigned int id = window->GetID(label);
		const float w = window->Layout.ItemWidth.back();

		if (!display_format)
			display_format = "%.3f";

		// Dodgily parse display precision back from the display format
		int decimal_precision = 3;
		if (const char* p = strchr(display_format, '%'))
		{
			p++;
			while (*p >= '0' && *p <= '9')
				p++;
			if (*p == '.')
			{
				decimal_precision = atoi(p + 1);
				if (decimal_precision < 0 || decimal_precision > 10)
					decimal_precision = 3;
			}
		}

		ImFloat2 text_size;
		if (!IsHideText(label))
			text_size = s_state.Render->GetTextSize(label);
		else
			text_size = s_state.Render->GetTextSize("");

		const ImFloat4 frame_bb(window->Layout.CursorPos.x, window->Layout.CursorPos.y, w + style.FramePadding.x*2.0f, text_size.y + style.FramePadding.y*2.0f);
		const ImFloat4 slider_bb(frame_bb.x + s_state.Styles.FramePadding.x, frame_bb.y + s_state.Styles.FramePadding.y, frame_bb.z - s_state.Styles.FramePadding.x * 2, frame_bb.w - s_state.Styles.FramePadding.y * 2);
		const ImFloat4 bb(frame_bb.x, frame_bb.y, frame_bb.z + style.ItemInnerSpacing.x + text_size.x, frame_bb.w);

		const bool is_unbound = v_min == -FLT_MAX || v_min == FLT_MAX || v_max == -FLT_MAX || v_max == FLT_MAX;

		const float grab_size_in_units = 1.0f;															// In 'v' units. Probably needs to be parametrized, based on a 'v_step' value? decimal precision?
		float grab_size_in_pixels;
		if (decimal_precision > 0 || is_unbound)
			grab_size_in_pixels = 10.0f;
		else
			grab_size_in_pixels = Max(grab_size_in_units * (w / (v_max - v_min + 1.0f)), 8.0f);				// Integer sliders
		const float slider_effective_w = slider_bb.z - grab_size_in_pixels;
		const float slider_effective_x1 = slider_bb.x + grab_size_in_pixels*0.5f;
		const float slider_effective_x2 = slider_bb.x + slider_bb.z - grab_size_in_pixels*0.5f;

		// For logarithmic sliders that cross over sign boundary we want the exponential increase to be symetric around 0.0
		float linear_zero_pos = 0.0f;	// 0.0->1.0f
		if (!is_unbound)
		{
			if (v_min * v_max < 0.0f)
			{
				// Different sign
				const float linear_dist_min_to_0 = powf(abs(0.0f - v_min), 1.0f / power);
				const float linear_dist_max_to_0 = powf(abs(v_max - 0.0f), 1.0f / power);
				linear_zero_pos = linear_dist_min_to_0 / (linear_dist_min_to_0 + linear_dist_max_to_0);
			}
			else
			{
				// Same sign
				linear_zero_pos = v_min < 0.0f ? 1.0f : 0.0f;
			}
		}

		const bool hovered = (s_state.HoveredWindow == window) && 
			PtInRect(s_state.Events.MousePos, ImFloat4(slider_bb.x + window->Rect.x, slider_bb.y + window->Rect.y, slider_bb.z, slider_bb.w));
		if (hovered)
			s_state.HoveredId = id;
		if (hovered && s_state.Events.MouseClicked)
			s_state.ActiveId = id;

		bool value_changed = false;

		ItemSize(bb);
		DrawWidgetFrame(frame_bb, Color_WidgetBg);

		if (s_state.ActiveId == id)
		{
			if (s_state.Events.MouseDown)
			{
				if (!is_unbound)
				{
					const float normalized_pos = Clamp((s_state.Events.MousePos.x - window->Rect.x - slider_effective_x1) / slider_effective_w, 0.0f, 1.0f);

					// Linear slider
					//float new_value = ImLerp(v_min, v_max, normalized_pos);

					// Account for logarithmic scale on both sides of the zero
					float new_value;
					if (normalized_pos < linear_zero_pos)
					{
						// Negative: rescale to the negative range before powering
						float a = 1.0f - (normalized_pos / linear_zero_pos);
						a = powf(a, power);
						new_value = Lerp(Min(v_max, 0.f), v_min, a);
					}
					else
					{
						// Positive: rescale to the positive range before powering
						float a = normalized_pos;
						if (abs(linear_zero_pos - 1.0f) > 1.e-6)
							a = (a - linear_zero_pos) / (1.0f - linear_zero_pos);
						a = powf(a, power);
						new_value = Lerp(Max(v_min, 0.0f), v_max, a);
					}

					// Round past decimal precision
					//  0: 1
					//  1: 0.1
					//  2: 0.01
					//  etc..
					// So when our value is 1.99999 with a precision of 0.001 we'll end up rounding to 2.0
					const float min_step = 1.0f / powf(10.0f, (float)decimal_precision);
					const float remainder = fmodf(new_value, min_step);
					if (remainder <= min_step*0.5f)
						new_value -= remainder;
					else
						new_value += (min_step - remainder);

					if (*v != new_value)
					{
						*v = new_value;
						value_changed = true;
					}
				}
			}
			else
			{
				s_state.ActiveId = 0;
			}
		}

		if (!is_unbound)
		{
			// Calculate slider grab positioning
			float grab_t;
			float v_clamped = Clamp(*v, v_min, v_max);
			if (v_clamped < 0.0f)
			{
				float f = 1.0f - (v_clamped - v_min) / (Min(0.0f, v_max) - v_min);
				grab_t = (1.0f - powf(f, 1.0f / power)) * linear_zero_pos;
			}
			else
			{
				float f = (v_clamped - Max(0.0f, v_min)) / (v_max - Max(0.0f, v_min));
				grab_t = linear_zero_pos + powf(f, 1.0f / power) * (1.0f - linear_zero_pos);
			}

			// Draw
			const float grab_x = Lerp(slider_effective_x1, slider_effective_x2, grab_t);
			const ImFloat4 grab_bb(grab_x - grab_size_in_pixels*0.5f, frame_bb.y + 2.0f, grab_size_in_pixels, frame_bb.w - 2.0f - 1.0f);
			s_state.Render->DrawRect(window->CRT, style.Colors[s_state.ActiveId == id ? Color_SliderActive : Color_Slider], grab_bb, true);
		}

		char value_buf[64];
		FormatString(value_buf, ARRAYSIZE(value_buf), display_format, *v);

		if (!IsHideText(value_buf))
		{
			const ImFloat2 value_buf_size = s_state.Render->GetTextSize(value_buf);
			ImFloat4 value_buf_box(slider_bb.x + slider_bb.z / 2 - value_buf_size.x*0.5f, frame_bb.y + style.FramePadding.y, value_buf_size.x, value_buf_size.y);
			s_state.Render->DrawText(window->CRT, style.Colors[Color_Text], value_buf, value_buf_box);
		}

		if (!IsHideText(label))
		{
			const ImFloat2 label_size = s_state.Render->GetTextSize(label);
			ImFloat4 label_box(frame_bb.x + frame_bb.z + style.ItemInnerSpacing.x + style.FramePadding.x, slider_bb.y, label_size.x, label_size.y);
			s_state.Render->DrawText(window->CRT, style.Colors[Color_Text], label, label_box);
		}

		return value_changed;
	}

	bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* display_format)
	{
		if (!display_format)
			display_format = "%.0f";
		float v_f = (float)*v;
		bool changed = ImDui::SliderFloat(label, &v_f, (float)v_min, (float)v_max, display_format, 1.0f);
		*v = (int)v_f;
		return changed;
	}

	bool ColorButton(const ImFloat4& col, bool small_height, bool outline_border)
	{
		Window* window = s_state.RenderWindow;
		if (window->Collapse)
			return false;

		const GuiStyle& style = s_state.Styles;

		const float square_size = s_state.Render->GetTextSize("").y;
		const ImFloat4 bb(window->Layout.CursorPos.x, window->Layout.CursorPos.y, square_size + style.FramePadding.x * 2, square_size + (small_height ? 0 : style.FramePadding.y * 2));
		ItemSize(bb);

		const bool hovered = (s_state.HoveredWindow == window) && 
			PtInRect(s_state.Events.MousePos, ImFloat4(bb.x + window->Rect.x, bb.y + window->Rect.y, bb.z, bb.w));
		const bool pressed = hovered && s_state.Events.MouseClicked;

		if (outline_border)
		{
			s_state.Render->DrawRect(window->CRT, style.Colors[Color_WidgetBg], bb, true);
			s_state.Render->DrawRect(window->CRT, col, ImFloat4(bb.x + 1, bb.y + 1, bb.z - 2, bb.w - 2), true);
		}
		else
		{
			s_state.Render->DrawRect(window->CRT, col, bb, true);
		}

		if (hovered)
		{
			int ix = (int)(col.x * 255.0f + 0.5f);
			int iy = (int)(col.y * 255.0f + 0.5f);
			int iz = (int)(col.z * 255.0f + 0.5f);
			int iw = (int)(col.w * 255.0f + 0.5f);
			ImDui::ToolTip("Color:\n(%.2f,%.2f,%.2f,%.2f)\n#%02X%02X%02X%02X", col.x, col.y, col.z, col.w, ix, iy, iz, iw);
		}

		return pressed;
	}

	bool ColorEdit3(const char* label, float col[3])
	{
		float col4[4];
		col4[0] = col[0];
		col4[1] = col[1];
		col4[2] = col[2];
		col4[3] = 1.0f;
		bool value_changed = ImDui::ColorEdit4(label, col4, false);
		col[0] = col4[0];
		col[1] = col4[1];
		col[2] = col4[2];
		return value_changed;
	}

	bool ColorEdit4(const char* label, float col[4], bool alpha)
	{
		Window* window = s_state.RenderWindow;
		if (window->Collapse)
			return false;

		const GuiStyle& style = s_state.Styles;
		const unsigned int id = window->GetID(label);
		const float w_full = window->Layout.ItemWidth.back();
		const float square_sz = (style.FontSize + style.FramePadding.x * 2.0f);

		const ImFloat2 text_size = s_state.Render->GetTextSize(label);

		float fx = col[0];
		float fy = col[1];
		float fz = col[2];
		float fw = col[3];
		const ImFloat4 col_display(fx, fy, fz, 1.0f);

		int ix = (int)(fx * 255.0f + 0.5f);
		int iy = (int)(fy * 255.0f + 0.5f);
		int iz = (int)(fz * 255.0f + 0.5f);
		int iw = (int)(fw * 255.0f + 0.5f);

		int components = alpha ? 4 : 3;
		bool value_changed = false;

		{
			// 0: RGB 0..255
			// 1: HSV 0.255 Sliders
			const float w_items_all = w_full - (square_sz + style.ItemInnerSpacing.x);
			const float w_item_one = Max(1.0f, (float)(int)((w_items_all - (style.FramePadding.x*2.0f + style.ItemInnerSpacing.x) * (components - 1)) / (float)components));
			const float w_item_last = Max(1.0f, (float)(int)(w_items_all - (w_item_one + style.FramePadding.x*2.0f + style.ItemInnerSpacing.x) * (components - 1)));

			ImDui::PushItemWidth(w_item_one);

			char label_slider[1024] = { 0 };
			sprintf(label_slider, "##%s_%s", label, "X");
			value_changed |= ImDui::SliderInt(label_slider, &ix, 0, 255, "R:%3.0f");
			ImDui::SameLine(0, 0);
			sprintf(label_slider, "##%s_%s", label, "Y");
			value_changed |= ImDui::SliderInt(label_slider, &iy, 0, 255, "G:%3.0f");
			ImDui::SameLine(0, 0);
			if (alpha)
			{
				sprintf(label_slider, "##%s_%s", label, "Z");
				value_changed |= ImDui::SliderInt(label_slider, &iz, 0, 255, "B:%3.0f");
				ImDui::SameLine(0, 0);
				ImDui::PushItemWidth(w_item_last);
				sprintf(label_slider, "##%s_%s", label, "W");
				value_changed |= ImDui::SliderInt(label_slider, &iw, 0, 255, "A:%3.0f");
			}
			else
			{
				ImDui::PushItemWidth(w_item_last);
				sprintf(label_slider, "##%s_%s", label, "Z");
				value_changed |= ImDui::SliderInt(label_slider, &iz, 0, 255, "B:%3.0f");
			}

			ImDui::PopItemWidth();
			ImDui::PopItemWidth();
		}

		ImDui::SameLine(0, 0);
		ImDui::ColorButton(col_display);

		if (!IsHideText(label))
		{
			ImDui::SameLine();
			s_state.Render->DrawText(window->CRT, style.Colors[Color_Text], label, ImFloat4(window->Layout.CursorPos.x, style.FramePadding.y + window->Layout.CursorPos.y, text_size.x, text_size.y));
			ItemSize(text_size);
		}

		// Convert back
		fx = ix / 255.0f;
		fy = iy / 255.0f;
		fz = iz / 255.0f;
		fw = iw / 255.0f;

		if (value_changed)
		{
			col[0] = fx;
			col[1] = fy;
			col[2] = fz;
			if (alpha)
				col[3] = fw;
		}

		return value_changed;
	}

	const char* GetStyleColorName(ImUint idx)
	{
		// Create with regexp: ImGuiCol_{.*}, --> case ImGuiCol_\1: return "\1";
		switch (idx)
		{
			case Color_Text				: return "Color_Text";
			case Color_Border			: return "Color_Border";
			case Color_WindowBg			: return "Color_WindowBg";
			case Color_WidgetBg			: return "Color_WidgetBg";
			case Color_WidgetActive		: return "Color_WidgetActive";
			case Color_TitleBar			: return "Color_TitleBar";
			case Color_TitleBarCollapsed: return "Color_TitleBarCollapsed";
			case Color_Slider			: return "Color_Slider";
			case Color_SliderActive		: return "Color_SliderActive";
			case Color_Button			: return "Color_Button";
			case Color_ButtonHovered	: return "Color_ButtonHovered";
			case Color_ButtonActive		: return "Color_ButtonActive";
			case Color_Collapse			: return "Color_Collapse";
			case Color_CollapseHovered	: return "Color_CollapseHovered";
			case Color_CollapseActive	: return "Color_CollapseActive";
			case Color_ResizeGrip		: return "Color_ResizeGrip";
			case Color_ResizeGripHovered: return "Color_ResizeGripHovered";
			case Color_ResizeGripActive	: return "Color_ResizeGripActive";
			case Color_TooltipBg		: return "Color_TooltipBg";
		}
		assert(0);
		return "Unknown";
	}

	void ShowStyleEditor()
	{
		GuiStyle& style = s_state.Styles;
		const GuiStyle def;

		if (ImDui::Button("Revert Style"))
			s_state.Styles = def;

		for (size_t i = 0; i < Color_COUNT; i++)
		{
			ImDui::ColorEdit4(GetStyleColorName(i), (float*)&style.Colors[i], true);
			if (memcmp(&style.Colors[i], &def.Colors[i], sizeof(ImFloat4)) != 0)
			{
				ImDui::SameLine(); if (ImDui::Button("Revert")) style.Colors[i] = def.Colors[i];
			}
		}
	}

	void ToolTip(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		FormatStringV(s_state.StrToolTip, ARRAYSIZE(s_state.StrToolTip), fmt, args);
		va_end(args);
	}

	//////////////////////////////////////////////////////////////////////////
	// definitions of some structure
	//////////////////////////////////////////////////////////////////////////

	// GuiStyle

	GuiStyle::GuiStyle()
	{
		FontName				= L"Arial";
		FontSize				= 12.f;
		StrokeWidth				= 1.f;

		DefaultWindowAlpha		= 1.0f;
		WindowPadding			= ImFloat2(8, 8);
		WindowMinSize			= ImFloat2(48, 48);
		FramePadding			= ImFloat2(5, 4);
		ItemSpacing				= ImFloat2(10, 5);
		ItemInnerSpacing		= ImFloat2(5, 5);
		TitleBarHeight			= 20.f;
		ResizeGripSize			= ImFloat2(30, 30);

		Colors[Color_Text]				= ImHexToRGBA(0x000000);
		Colors[Color_Border]			= ImHexToRGBA(0xFF00FF);
		Colors[Color_WindowBg]			= ImHexToRGBA(0xDCDCDC);
		Colors[Color_WidgetBg]			= ImHexToRGBA(0xFFFFFF);
		Colors[Color_WidgetActive]		= ImHexToRGBA(0x4296FA);
		Colors[Color_TitleBar]			= ImHexToRGBA(0x7BB4F7);
		Colors[Color_TitleBarCollapsed]	= ImHexToRGBA(0xA0C8FF);
		Colors[Color_Slider]			= ImHexToRGBA(0x64B4FF);
		Colors[Color_SliderActive]		= ImHexToRGBA(0x4296FA);
		Colors[Color_Button]			= ImHexToRGBA(0x9FA8DA);
		Colors[Color_ButtonHovered]		= ImHexToRGBA(0xB2B4D2);
		Colors[Color_ButtonActive]		= ImHexToRGBA(0x8B94C8);
		Colors[Color_Collapse]			= ImHexToRGBA(0x9FA8DA);
		Colors[Color_CollapseHovered]	= ImHexToRGBA(0xB2B4D2);
		Colors[Color_CollapseActive]	= ImHexToRGBA(0x8B94C8);
		Colors[Color_ResizeGrip]		= ImHexToRGBA(0x95A5A6);
		Colors[Color_ResizeGripHovered]	= ImHexToRGBA(0xBDC3C7);
		Colors[Color_ResizeGripActive]	= ImHexToRGBA(0xABB7B7);
		Colors[Color_TooltipBg]			= ImHexToRGBA(0xD8FFC4);
	}

	// Event

	Event::Event()
	{
		memset(this, 0, sizeof(*this));

		MousePos = ImFloat2(-1, -1);
		MousePosPrev = ImFloat2(-1, -1);
		MouseDoubleClickTime = 0.30f;
		MouseDoubleClickMaxDist = 6.0f;
	}

	// Storage

	void Storage::Clear()
	{
		Data.clear();
	}

	int Storage::GetValue(ImUint key, int default_val)
	{
		std::unordered_map<ImUint, int>::iterator iter = Data.find(key);
		if (iter != Data.end())
			return iter->second;
		else
			return default_val;
	}

	void Storage::SetValue(ImUint key, int val)
	{
		std::unordered_map<ImUint, int>::iterator iter = Data.find(key);
		if (iter != Data.end())
			iter->second = val;
		else
			Data[key] = val;
	}

	void Storage::SetAllInt(int v)
	{
		for (auto iter = Data.begin(); iter != Data.end(); iter++)
			iter->second = v;
	}

	// Window

	ImUint Window::_idseed = 1;

	Window::Window(const char* name, ImFloat2 default_pos, ImFloat2 default_size)
		: CRT(NULL)
		, Rect(default_pos.x, default_pos.y, default_size.x, default_size.y)
		, Alpha(1.f)
		, Visible(true)
		, Collapse(false)
		, ItemWidthDefault(0.f)
	{
		Name = strdup(name);
		Resize(default_size);
	}

	Window::~Window()
	{
		SafeRelease(&CRT);

		free(Name);
		Name = NULL;
	}

	void Window::Resize(ImFloat2 size)
	{
		SafeRelease(&CRT);

		HRESULT hr = s_state.Render->GetMainRT()->CreateCompatibleRenderTarget(D2D1::SizeF(size.x, size.y), &CRT);
		assert(hr == S_OK);

		//CRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
	}

	ImUint Window::GetID(const char* str)
	{
		ImStringUintMap::iterator iter = IDMap.find(str);
		if (iter != IDMap.end())
			return IDMap[str];

		ImUint id = _idseed++;
		IDMap[str] = id;
		return id;
	}


	//////////////////////////////////////////////////////////////////////////
	// util
	//////////////////////////////////////////////////////////////////////////

	size_t FormatString(char* buf, size_t buf_size, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		int w = vsnprintf(buf, buf_size, fmt, args);
		va_end(args);
		buf[buf_size - 1] = 0;
		if (w == -1) w = buf_size;
		return w;
	}

	size_t FormatStringV(char* buf, size_t buf_size, const char* fmt, va_list args)
	{
		int w = vsnprintf(buf, buf_size, fmt, args);
		buf[buf_size - 1] = 0;
		if (w == -1) w = buf_size;
		return w;
	}

	bool PtInRect(ImFloat2 pt, ImFloat4 rt)
	{
		if (pt.x < rt.x || pt.y < rt.y || pt.x >= rt.x + rt.z || pt.y >= rt.y + rt.w)
			return false;
		return true;
	}

	void OutLog(const char * pszFormat, ...)
	{
		char szBuf[MAX_LEN];
		va_list ap;
		va_start(ap, pszFormat);
		vsnprintf_s(szBuf, MAX_LEN, MAX_LEN, pszFormat, ap);
		va_end(ap);
		printf("[LOG] %s\n",szBuf);
	}

	void OutWarning(const char * pszFormat, ...)
	{
		char szBuf[MAX_LEN];
		va_list ap;
		va_start(ap, pszFormat);
		vsnprintf_s(szBuf, MAX_LEN, MAX_LEN, pszFormat, ap);
		va_end(ap);
		printf("[WARNING] %s\n", szBuf);
	}

	void OutError(const char * pszFormat, ...)
	{
		char szBuf[MAX_LEN];
		va_list ap;
		va_start(ap, pszFormat);
		vsnprintf_s(szBuf, MAX_LEN, MAX_LEN, pszFormat, ap);
		va_end(ap);
		printf("[ERROR] %s\n", szBuf);
	}

	std::wstring ATOW(const std::string& src)
	{
		std::wstring ret;
		if (!src.empty())
		{
			int nNum = MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, NULL, 0);
			if (nNum)
			{
				WCHAR* wideChaUtilStr = new WCHAR[nNum + 1];
				wideChaUtilStr[0] = 0;
				nNum = MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, wideChaUtilStr, nNum + 1);
				ret = wideChaUtilStr;
				delete[] wideChaUtilStr;
			}
			else
			{
				OutLog("Wrong convert to WideChar code:0x%x", GetLastError());
			}
		}
		return ret;
	}

	std::string WTOA(const std::wstring& src)
	{
		std::string ret;
		if (!src.empty())
		{
			int nNum = WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, NULL, 0, NULL, FALSE);
			if (nNum)
			{
				char* utf8String = new char[nNum + 1];
				utf8String[0] = 0;
				nNum = WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, utf8String, nNum + 1, NULL, FALSE);
				ret = utf8String;
				delete[] utf8String;
			}
			else
			{
				OutLog("Wrong convert to Utf8 code:0x%x", GetLastError());
			}
		}

		return ret;
	}
}