//*************************************************************************************************************
#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "GdiPlus.lib")

#ifdef _DEBUG
#	pragma comment(lib, "d3dx10d.lib")
#else
#	pragma comment(lib, "d3dx10.lib")
#endif

#include <Windows.h>
#include <GdiPlus.h>
#include <d3d10.h>
#include <d3dx10.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <iostream>

#define TITLE			"Shader tutorial 13: Stencil shadow volume (DX10)"
#define MYERROR(x)		{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)	{ if( (x) ) { (x)->Release(); (x) = NULL; } }

DXGI_SWAP_CHAIN_DESC	swapchaindesc;
DXGI_MODE_DESC			displaymode;
ID3D10Device*			device;
ID3D10RenderTargetView*	rendertargetview	= NULL;
ID3D10DepthStencilView*	depthstencilview	= NULL;
ID3D10Texture2D*		depthstencil		= NULL;
IDXGISwapChain*			swapchain			= NULL;;

HWND					hwnd				= NULL;
ULONG_PTR				gdiplustoken		= 0;
RECT					workarea;
long					screenwidth			= 800;
long					screenheight		= 600;

short					mousex, mousedx		= 0;
short					mousey, mousedy		= 0;
short					mousedown			= 0;

HRESULT InitScene();

void UninitScene();
void Update(float delta);
void Render(float alpha, float elapsedtime);
void KeyPress(WPARAM wparam);

HRESULT InitDirect3D(HWND hwnd)
{
	displaymode.Format	= DXGI_FORMAT_R8G8B8A8_UNORM;
	displaymode.Width	= screenwidth;
	displaymode.Height	= screenheight;

	displaymode.RefreshRate.Numerator = 60;
	displaymode.RefreshRate.Denominator = 1;

	ID3D10Texture2D*				backbuffer = 0;
	D3D10_TEXTURE2D_DESC			depthdesc;
	D3D10_DEPTH_STENCIL_VIEW_DESC	dsv;
	D3D10_VIEWPORT					vp;
	UINT							flags = 0;
	HRESULT							hr;

	memset(&swapchaindesc, 0, sizeof(DXGI_SWAP_CHAIN_DESC));
	memset(&depthdesc, 0, sizeof(D3D10_TEXTURE2D_DESC));
	memset(&dsv, 0, sizeof(D3D10_DEPTH_STENCIL_VIEW_DESC));

	swapchaindesc.BufferDesc			= displaymode;
	swapchaindesc.BufferDesc.Width		= screenwidth;
	swapchaindesc.BufferDesc.Height		= screenheight;
	swapchaindesc.BufferCount			= 1;
	swapchaindesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchaindesc.OutputWindow			= hwnd;
	swapchaindesc.SampleDesc.Count		= 1;
	swapchaindesc.SampleDesc.Quality	= 0;
	swapchaindesc.Windowed				= true;

#ifdef _DEBUG
	flags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

	hr = D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, flags,
		D3D10_SDK_VERSION, &swapchaindesc, &swapchain, &device);

	if( FAILED(hr) )
		return hr;

	hr = swapchain->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)&backbuffer);

	if( FAILED(hr) )
		return hr;

	hr = device->CreateRenderTargetView(backbuffer, NULL, &rendertargetview);

	if( FAILED(hr) )
		return hr;

	backbuffer->Release();

	depthdesc.Width					= screenwidth;
	depthdesc.Height				= screenheight;
	depthdesc.MipLevels				= 1;
	depthdesc.ArraySize				= 1;
	depthdesc.Format				= DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthdesc.SampleDesc.Count		= 1;
	depthdesc.SampleDesc.Quality	= 0;
	depthdesc.Usage					= D3D10_USAGE_DEFAULT;
	depthdesc.BindFlags				= D3D10_BIND_DEPTH_STENCIL;

	hr = device->CreateTexture2D(&depthdesc, NULL, &depthstencil);

	if( FAILED(hr) )
		return hr;

	dsv.Format				= depthdesc.Format;
	dsv.ViewDimension		= D3D10_DSV_DIMENSION_TEXTURE2D;
	dsv.Texture2D.MipSlice	= 0;

	hr = device->CreateDepthStencilView(depthstencil, &dsv, &depthstencilview);

	if( FAILED(hr) )
		return hr;

	device->OMSetRenderTargets(1, &rendertargetview, depthstencilview);

	vp.Width		= screenwidth;
	vp.Height		= screenheight;
	vp.MinDepth		= 0.0f;
	vp.MaxDepth		= 1.0f;
	vp.TopLeftX		= 0;
	vp.TopLeftY		= 0;

	device->RSSetViewports(1, &vp);

	return S_OK;
}
//*************************************************************************************************************
static void ReadString(FILE* f, char* buff)
{
	size_t ind = 0;
	char ch = fgetc(f);

	while( ch != '\n' )
	{
		buff[ind] = ch;
		ch = fgetc(f);
		++ind;
	}

	buff[ind] = '\0';
}
//*************************************************************************************************************
HRESULT LoadMeshFromQM(LPCTSTR file, DWORD options, ID3DX10Mesh** mesh)
{
	static const char* usages[] =
	{
		"POSITION",
		"POSITIONT",
		"COLOR",
		"BLENDWEIGHT",
		"BLENDINDICES",
		"NORMAL",
		"TEXCOORD",
		"TANGENT",
		"BINORMAL",
		"PSIZE",
		"TESSFACTOR"
	};

	static const DXGI_FORMAT types[] =
	{
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R8G8B8A8_UNORM
	};

	static const unsigned short elemsizes[6] =
	{
		1, 2, 3, 4, 4, 4
	};

	static const unsigned short elemstrides[6] =
	{
		4, 4, 4, 4, 1, 1
	};

	D3D10_INPUT_ELEMENT_DESC*	decl;
	D3DX10_ATTRIBUTE_RANGE*		table;
	ID3DX10MeshBuffer*			vertexbuffer;
	ID3DX10MeshBuffer*			indexbuffer;
	ID3DX10MeshBuffer*			attribbuffer;
	SIZE_T						datasize;

	D3DXCOLOR		color;
	HRESULT			hr;
	FILE*			infile = 0;
	unsigned int	unused;
	unsigned int	version;
	unsigned int	numindices;
	unsigned int	numvertices;
	unsigned int	vstride;
	unsigned int	istride;
	unsigned int	numsubsets;
	unsigned int	numelems;
	unsigned short	tmp16;
	unsigned char	tmp8;
	void*			data;
	char			buff[256];

#ifdef _MSC_VER
	fopen_s(&infile, file, "rb");
#else
	infile = fopen(file, "rb")
#endif

	if( !infile )
		return E_FAIL;

	fread(&unused, 4, 1, infile);
	fread(&numindices, 4, 1, infile);
	fread(&istride, 4, 1, infile);
	fread(&numsubsets, 4, 1, infile);

	version = unused >> 16;

	fread(&numvertices, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	fread(&unused, 4, 1, infile);

	table = new D3DX10_ATTRIBUTE_RANGE[numsubsets];

	// vertex declaration
	fread(&numelems, 4, 1, infile);
	decl = new D3D10_INPUT_ELEMENT_DESC[numelems];

	vstride = 0;

	for( unsigned int i = 0; i < numelems; ++i )
	{
		fread(&tmp16, 2, 1, infile);
		decl[i].InputSlot = tmp16;
		decl[i].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;

		fread(&tmp8, 1, 1, infile);
		decl[i].SemanticName = usages[tmp8];

		fread(&tmp8, 1, 1, infile);
		decl[i].Format = types[tmp8];

		unused = elemsizes[tmp8] * elemstrides[tmp8];

		fread(&tmp8, 1, 1, infile);
		decl[i].SemanticIndex = tmp8;

		//decl[i].AlignedByteOffset = D3D10_APPEND_ALIGNED_ELEMENT;
		decl[i].AlignedByteOffset = vstride;
		decl[i].InstanceDataStepRate = 0;

		vstride += unused;
	}

	// create mesh
	if( istride == 4 )
		options |= D3DX10_MESH_32_BIT;

	hr = D3DX10CreateMesh(device, decl, numelems, "POSITION", numvertices, numindices / 3, options, mesh);

	if( FAILED(hr) )
		goto _fail;

	(*mesh)->GetVertexBuffer(0, &vertexbuffer);
	(*mesh)->GetIndexBuffer(&indexbuffer);

	// TODO: SetVertexData(), SetIndexData() ?
	vertexbuffer->Map(&data, &datasize);
	fread(data, vstride, numvertices, infile);
	vertexbuffer->Unmap();
	vertexbuffer->Release();

	indexbuffer->Map(&data, &datasize);
	fread(data, istride, numindices, infile);
	indexbuffer->Unmap();
	indexbuffer->Release();

	if( version > 1 )
	{
		fread(&unused, 4, 1, infile);

		if( unused > 0 )
			fseek(infile, 8 * unused, SEEK_CUR);
	}

	for( unsigned int i = 0; i < numsubsets; ++i )
	{
		D3DX10_ATTRIBUTE_RANGE& subset = table[i];

		subset.AttribId = i;

		fread(&subset.FaceStart, 4, 1, infile);
		fread(&subset.VertexStart, 4, 1, infile);
		fread(&subset.VertexCount, 4, 1, infile);
		fread(&subset.FaceCount, 4, 1, infile);

		subset.FaceCount /= 3;
		subset.FaceStart /= 3;

		fseek(infile, 6 * sizeof(float), SEEK_CUR);

		ReadString(infile, buff);
		ReadString(infile, buff);

		bool hasmaterial = (buff[1] != ',');

		if( hasmaterial )
		{
			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			//mat.MatD3D.Ambient = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			//mat.MatD3D.Diffuse = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			//mat.MatD3D.Specular = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			//mat.MatD3D.Emissive = (D3DCOLORVALUE&)color;

			fread(&color.r, sizeof(float), 1, infile);	// mat.MatD3D.Power
			fread(&color.a, sizeof(float), 1, infile);	// mat.MatD3D.Diffuse.a

			fread(&unused, 4, 1, infile);
			ReadString(infile, buff);

			//if( buff[1] != ',' )
			//{
			//	unused = strlen(buff);

			//	mat.pTextureFilename = new char[unused + 1];
			//	memcpy(mat.pTextureFilename, buff, unused);
			//	mat.pTextureFilename[unused] = 0;
			//}

			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
			ReadString(infile, buff);
		}
		else
		{
			//color = D3DXCOLOR(1, 1, 1, 1);

			//memcpy(&mat.MatD3D.Ambient, &color, 4 * sizeof(float));
			//memcpy(&mat.MatD3D.Diffuse, &color, 4 * sizeof(float));
			//memcpy(&mat.MatD3D.Specular, &color, 4 * sizeof(float));

			//color = D3DXCOLOR(0, 0, 0, 1);
			//memcpy(&mat.MatD3D.Emissive, &color, 4 * sizeof(float));

			//mat.MatD3D.Power = 80.0f;
		}

		ReadString(infile, buff);

		//if( buff[1] != ','  && mat.pTextureFilename == 0 )
		//{
		//	unused = strlen(buff);

		//	mat.pTextureFilename = new char[unused + 1];
		//	memcpy(mat.pTextureFilename, buff, unused);
		//	mat.pTextureFilename[unused] = 0;
		//}

		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
		ReadString(infile, buff);
	}

	// attribute buffer
	(*mesh)->SetAttributeTable(table, numsubsets);
	(*mesh)->GetAttributeBuffer(&attribbuffer);

	attribbuffer->Map(&data, &datasize);

	for( unsigned int i = 0; i < numsubsets; ++i )
	{
		const D3DX10_ATTRIBUTE_RANGE& subset = table[i];

		for( DWORD j = 0; j < subset.FaceCount; ++j )
			*((DWORD*)data + subset.FaceStart + j) = subset.AttribId;
	}

	attribbuffer->Unmap();
	attribbuffer->Release();

	(*mesh)->CommitToDevice();

_fail:
	delete[] decl;
	delete[] table;

	fclose(infile);
	return hr;
}
//*************************************************************************************************************
HRESULT RenderText(const std::string& str, ID3D10Texture2D* tex, DWORD width, DWORD height)
{
	if( tex == 0 )
		return E_FAIL;

	if( gdiplustoken == 0 )
	{
		Gdiplus::GdiplusStartupInput gdiplustartup;
		Gdiplus::GdiplusStartup(&gdiplustoken, &gdiplustartup, NULL);
	}

	Gdiplus::Color				outline(0xff000000);
	Gdiplus::Color				fill(0xffffffff);

	Gdiplus::Bitmap*			bitmap;
	Gdiplus::Graphics*			graphics;
	Gdiplus::GraphicsPath		path;
	Gdiplus::FontFamily			family(L"Arial");
	Gdiplus::StringFormat		format;
	Gdiplus::Pen				pen(outline, 3);
	Gdiplus::SolidBrush			brush(fill);
	std::wstring				wstr(str.begin(), str.end());

	//format.SetAlignment(Gdiplus::StringAlignmentFar);

	bitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
	graphics = new Gdiplus::Graphics(bitmap);

	// render text
	graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	graphics->SetPageUnit(Gdiplus::UnitPixel);

	path.AddString(wstr.c_str(), wstr.length(), &family, Gdiplus::FontStyleBold, 25, Gdiplus::Point(0, 0), &format);
	pen.SetLineJoin(Gdiplus::LineJoinRound);

	graphics->DrawPath(&pen, &path);
	graphics->FillPath(&brush, &path);

	// copy to texture
	Gdiplus::Rect rc(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
	Gdiplus::BitmapData data;

	D3D10_TEXTURE2D_DESC desc;
	D3D10_MAPPED_TEXTURE2D mapped;
	UINT subres;

	tex->GetDesc(&desc);
	subres = D3D10CalcSubresource(0, 0, desc.MipLevels);

	memset(&data, 0, sizeof(Gdiplus::BitmapData));

	bitmap->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);
	tex->Map(subres, D3D10_MAP_WRITE_DISCARD, 0, &mapped);
	
	memcpy(mapped.pData, data.Scan0, width * height * 4);

	tex->Unmap(subres);
	bitmap->UnlockBits(&data);

	delete graphics;
	delete bitmap;

	return S_OK;
}
//*************************************************************************************************************
LRESULT WINAPI WndProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_KEYUP:
		switch(wParam)
		{
		case VK_ESCAPE:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		default:
			KeyPress(wParam);
			break;
		}
		break;

	case WM_MOUSEMOVE: {
		short x = (short)(lParam & 0xffff);
		short y = (short)((lParam >> 16) & 0xffff);

		mousedx += x - mousex;
		mousedy += y - mousey;

		mousex = x;
		mousey = y;
		} break;

	case WM_LBUTTONDOWN:
		mousedown = 1;
		break;

	case WM_RBUTTONDOWN:
		mousedown = 2;
		break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		mousedown = 0;
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}
//*************************************************************************************************************
void Adjust(tagRECT& out, long& width, long& height, DWORD style, DWORD exstyle, bool menu = false)
{
	long w = workarea.right - workarea.left;
	long h = workarea.bottom - workarea.top;

	out.left = (w - width) / 2;
	out.top = (h - height) / 2;
	out.right = (w + width) / 2;
	out.bottom = (h + height) / 2;

	AdjustWindowRectEx(&out, style, false, 0);

	long windowwidth = out.right - out.left;
	long windowheight = out.bottom - out.top;

	long dw = windowwidth - width;
	long dh = windowheight - height;

	if( windowheight > h )
	{
		float ratio = (float)width / (float)height;
		float realw = (float)(h - dh) * ratio + 0.5f;

		windowheight = h;
		windowwidth = (long)floor(realw) + dw;
	}

	if( windowwidth > w )
	{
		float ratio = (float)height / (float)width;
		float realh = (float)(w - dw) * ratio + 0.5f;

		windowwidth = w;
		windowheight = (long)floor(realh) + dh;
	}

	out.left = workarea.left + (w - windowwidth) / 2;
	out.top = workarea.top + (h - windowheight) / 2;
	out.right = workarea.left + (w + windowwidth) / 2;
	out.bottom = workarea.top + (h + windowheight) / 2;

	width = windowwidth - dw;
	height = windowheight - dh;
}
//*************************************************************************************************************
int main(int argc, char* argv[])
{
	LARGE_INTEGER qwTicksPerSec = { 0, 0 };
	LARGE_INTEGER qwTime;
	LONGLONG tickspersec;
	double last, current;
	double delta, accum = 0;

	// ablak osztály
	WNDCLASSEX wc =
	{
		sizeof(WNDCLASSEX),
		CS_CLASSDC,
		(WNDPROC)WndProc,
		0L,
		0L,
		GetModuleHandle(NULL),
		NULL, NULL, NULL, NULL, "TestClass", NULL
	};

	RegisterClassEx(&wc);
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workarea, 0);

	RECT rect = { 0, 0, screenwidth, screenheight };
	DWORD style = WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

	// ablakos mód
	style |= WS_SYSMENU|WS_BORDER|WS_CAPTION;
	Adjust(rect, screenwidth, screenheight, style, 0);

	hwnd = CreateWindowA("TestClass", TITLE, style,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, wc.hInstance, NULL);

	if( !hwnd )
	{
		MYERROR("Could not create window");
		goto _end;
	}

	if( FAILED(InitDirect3D(hwnd)) )
	{
		MYERROR("Failed to initialize Direct3D");
		goto _end;
	}
	
	if( FAILED(InitScene()) )
	{
		MYERROR("Failed to initialize scene");
		goto _end;
	}

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	POINT p;
	GetCursorPos(&p);
	ScreenToClient(hwnd, &p);

	// timer
	QueryPerformanceFrequency(&qwTicksPerSec);
	tickspersec = qwTicksPerSec.QuadPart;

	QueryPerformanceCounter(&qwTime);
	last = (double)qwTime.QuadPart / (double)tickspersec;

	while( msg.message != WM_QUIT )
	{
		QueryPerformanceCounter(&qwTime);

		current = (double)qwTime.QuadPart / (double)tickspersec;
		delta = (current - last);

		last = current;
		accum += delta;

		mousedx = mousedy = 0;

		while( accum > 0.0333f )
		{
			accum -= 0.0333f;

			while( PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) )
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			Update(0.0333f);
		}

		if( msg.message != WM_QUIT )
			Render((float)accum / 0.0333f, (float)delta);
	}

_end:
	UninitScene();

	if( device )
		device->ClearState();

	SAFE_RELEASE(depthstencil);
	SAFE_RELEASE(depthstencilview);
	SAFE_RELEASE(rendertargetview);
	SAFE_RELEASE(swapchain);

	if( device )
	{
		ULONG rc = device->Release();

		if( rc > 0 )
		{
			if( rc > 0 )
				MYERROR("You forgot to release something");
		}
	}

	if( gdiplustoken )
		Gdiplus::GdiplusShutdown(gdiplustoken);

	UnregisterClass("TestClass", wc.hInstance);
	_CrtDumpMemoryLeaks();

#ifdef _DEBUG
	system("pause");
#endif

	return 0;
}
//*************************************************************************************************************
