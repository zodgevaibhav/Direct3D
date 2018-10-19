/*
1. To manupulate any stage of pipeline, only DeviceContext can be used.
2. Macros starting with D3D_ are common to all DirectX versions. Macros starting with D3D11_ are specific to 11 version
*/
#include <windows.h>
#include<stdio.h> // for file I/O

#include <d3d11.h> // d3d11 header files
#include <d3dcompiler.h>

#include "WICTextureLoader.h" // To load texture from file, build using project DirectXTK from GIT. DirectX::CreateWICTextureFromFile

// to suppress the warning "C4838: conversion from 'unsigned int' to 'INT' requires a narrowing conversion"
// due to '.inl' files included in xnamath.h file
#pragma warning( disable: 4838 )
#include "XNAMath\xnamath.h"

#pragma comment (lib,"d3d11.lib")
#pragma comment (lib,"D3dcompiler.lib")
#pragma comment (lib,"DirectXTK.lib") // for DirectX::CreateWICTextureFromFile

#define WIN_WIDTH 800
#define WIN_HEIGHT 600

// global function declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// global varibale declarations
FILE *gpFile = NULL;
char gszLogFileName[] = "Log.txt";

HWND ghwnd = NULL;

DWORD dwStyle;
WINDOWPLACEMENT wpPrev = { sizeof(WINDOWPLACEMENT) };

bool gbActiveWindow = false;
bool gbEscapeKeyIsPressed = false;
bool gbFullscreen = false;

float gClearColor[4]; // RGBA
IDXGISwapChain *gpIDXGISwapChain = NULL;
ID3D11Device *gpID3D11Device = NULL;
ID3D11DeviceContext *gpID3D11DeviceContext = NULL;
ID3D11RenderTargetView *gpID3D11RenderTargetView = NULL;

ID3D11VertexShader *gpID3D11VertexShader = NULL; //vertex shader Object
ID3D11PixelShader *gpID3D11PixelShader = NULL; //fragment shader Object



ID3D11Buffer *gpID3D11Buffer_VertexBuffer_Position_Cube = NULL;
ID3D11Buffer *gpID3D11Buffer_VertexBuffer_Texture_Cube = NULL;
ID3D11Buffer *gpID3D11Buffer_VertexBuffer_quads_color = NULL;


ID3D11InputLayout *gpID3D11InputLayout = NULL; //same as attributes in OPenGL
ID3D11Buffer *gpID3D11Buffer_ConstantBuffer = NULL; //same as Uniform in openGL

ID3D11RasterizerState *gpID3D11RasterizerState = NULL; //holds a description for rasterizer state
													   /*The rasterizer - state interface holds a description for rasterizer state that you can bind to the rasterizer stage.
													   The rasterization stage converts vector information (composed of shapes or primitives) into a raster image (composed of pixels) for the purpose of displaying real-time 3D graphics.
													   During rasterization, each primitive is converted into pixels, while interpolating per-vertex values across each primitive.
													   */
ID3D11DepthStencilView *gpID3D11DepthStencilView = NULL; //accesses a texture resource during depth-stencil testing

ID3D11ShaderResourceView *gpID3D11ShaderResourceView_Texture_Cube = NULL;
ID3D11SamplerState *gpID3D11SamplerState_Texture_Cube = NULL;

// a struct to represent constant buffer(i.e Uniform in openGL) ( names in host code and shader code need not to be same )
struct CBUFFER
{
	XMMATRIX WorldViewProjectionMatrix;
};

XMMATRIX gPerspectiveProjectionMatrix;

float angleQuads = 0;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
	// function declarations
	HRESULT initialize(void);
	void update(void);
	void display(void);
	void uninitialize(void);

	// variable declarations
	WNDCLASSEX wndclass;
	HWND hwnd;
	MSG msg;
	TCHAR szClassName[] = TEXT("Direct3D11");
	bool bDone = false;

	// code
	// create log file
	if (fopen_s(&gpFile, gszLogFileName, "w") != 0)
	{
		MessageBox(NULL, TEXT("Log File Can Not Be Created\nExitting ..."), TEXT("Error"), MB_OK | MB_TOPMOST | MB_ICONSTOP);
		exit(0);
	}
	else
	{
		fprintf_s(gpFile, "Log File Is Successfully Opened.\n");
		fclose(gpFile);
	}

	// initialize WNDCLASSEX structure
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.lpfnWndProc = WndProc;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.lpszClassName = szClassName;
	wndclass.lpszMenuName = NULL;
	wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// register WNDCLASSEX structure
	RegisterClassEx(&wndclass);

	// create window
	hwnd = CreateWindow(szClassName,
		TEXT("D3D:Interleaved - Light/Texure"),
		WS_OVERLAPPEDWINDOW,
		100,
		100,
		WIN_WIDTH,
		WIN_HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL);

	ghwnd = hwnd;

	ShowWindow(hwnd, iCmdShow);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);

	// initialize
	HRESULT hr;
	hr = initialize();
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "initialize() Failed. Exitting Now ...\n");
		fclose(gpFile);
		DestroyWindow(hwnd);
		hwnd = NULL;
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "initialize() Succeeded.\n");
		fclose(gpFile);
	}

	// message loop
	while (bDone == false)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				bDone = true;
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			display();
			update();

			if (gbActiveWindow == true)
			{
				if (gbEscapeKeyIsPressed == true)
					bDone = true;
			}
		}
	}

	// clean-up
	uninitialize();

	return((int)msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	// function declarations
	HRESULT resize(int, int);
	void ToggleFullscreen(void);
	void uninitialize(void);

	// variable declaratrions
	HRESULT hr;

	// code
	switch (iMsg)
	{
	case WM_ACTIVATE:
		if (HIWORD(wParam) == 0) // if 0, window is active
			gbActiveWindow = true;
		else // if non-zero, window is not active
			gbActiveWindow = false;
		break;
	case WM_ERASEBKGND:
		return(0);
	case WM_SIZE:
		if (gpID3D11DeviceContext)
		{
			hr = resize(LOWORD(lParam), HIWORD(lParam));
			if (FAILED(hr))
			{
				fopen_s(&gpFile, gszLogFileName, "a+");
				fprintf_s(gpFile, "resize() Failed.\n");
				fclose(gpFile);
				return(hr);
			}
			else
			{
				fopen_s(&gpFile, gszLogFileName, "a+");
				fprintf_s(gpFile, "resize() Succeeded.\n");
				fclose(gpFile);
			}
		}
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE: // case 27
			if (gbEscapeKeyIsPressed == false)
				gbEscapeKeyIsPressed = true;
			break;
		case 0x46: // 'F' or 'f'
			if (gbFullscreen == false)
			{
				ToggleFullscreen();
				gbFullscreen = true;
			}
			else
			{
				ToggleFullscreen();
				gbFullscreen = false;
			}
			break;
		default:
			break;
		}
		break;
	case WM_LBUTTONDOWN:
		break;
	case WM_CLOSE:
		uninitialize();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}
	return(DefWindowProc(hwnd, iMsg, wParam, lParam));
}

void ToggleFullscreen(void)
{
	MONITORINFO mi;

	if (gbFullscreen == false)
	{
		dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
		if (dwStyle & WS_OVERLAPPEDWINDOW)
		{
			mi = { sizeof(MONITORINFO) };
			if (GetWindowPlacement(ghwnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(ghwnd, MONITORINFOF_PRIMARY), &mi))
			{
				SetWindowLong(ghwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(ghwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOZORDER | SWP_FRAMECHANGED);
			}
		}
		ShowCursor(FALSE);
	}
	else
	{
		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd, &wpPrev);
		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);

		ShowCursor(TRUE);
	}
}

HRESULT initialize(void)
{
	// function declarations
	HRESULT LoadD3DTexture(const wchar_t *, ID3D11ShaderResourceView **);
	void uninitialize(void);
	HRESULT resize(int, int);

	HRESULT hr;
	D3D_DRIVER_TYPE d3dDriverType; //this local temp variable will used in loop for storing driver type temporary.
	D3D_DRIVER_TYPE d3dDriverTypes[] = { D3D_DRIVER_TYPE_HARDWARE,D3D_DRIVER_TYPE_WARP,D3D_DRIVER_TYPE_REFERENCE, }; //Driver types we want to use. Generaly HARDWARE type should be used for D3D programs
	D3D_FEATURE_LEVEL d3dFeatureLevel_required = D3D_FEATURE_LEVEL_11_0; // Requred D3D feature level we want i.e 11. 
	D3D_FEATURE_LEVEL d3dFeatureLevel_acquired = D3D_FEATURE_LEVEL_10_0; // If mentioned required D3D feature level not found/available then give minimum D3D feature level i.e 10
																		 /* To handle driversity of video cards with new and older machines, featuer level concept introduces in D3D11.
																		 FEATURE LEVEL IS THE WELL DEFINED SET OF GPU FUNCTIONALITY
																		 Ref.-https://docs.microsoft.com/en-us/windows/desktop/direct3d11/overviews-direct3d-11-devices-downlevel-intro */

	UINT createDeviceFlags = 0;
	UINT numDriverTypes = 0;
	UINT numFeatureLevels = 1; // How many feature level we will be asking while creating DeviceAndSwapChain

							   /*To get the driver we need to run loop for all the driver types we are requesting in array and to run through loop need to calculate size of array*/
	numDriverTypes = sizeof(d3dDriverTypes) / sizeof(d3dDriverTypes[0]); // this is the general way to find size of array


	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	ZeroMemory((void *)&dxgiSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	dxgiSwapChainDesc.BufferCount = 1;
	dxgiSwapChainDesc.BufferDesc.Width = WIN_WIDTH;
	dxgiSwapChainDesc.BufferDesc.Height = WIN_HEIGHT;
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.OutputWindow = ghwnd;
	dxgiSwapChainDesc.SampleDesc.Count = 1;
	dxgiSwapChainDesc.SampleDesc.Quality = 0;
	dxgiSwapChainDesc.Windowed = TRUE;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++) //Get driver which is available. If hardware found then give me hardware, else check for WRAP available else give me REFERENCE
	{
		d3dDriverType = d3dDriverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(//Creates a device that represents the display adapter and a swap chain used for rendering.
			NULL,                          // Pointer to video adapter to use when creating device. Pass NULL to use default or primary adapter (whom monitor is attached).
			d3dDriverType,                 // Which driver Type is required {HARDWARE, WRAP or REFERENCE ?}
			NULL,                          // Handle (HMODULE)to the custom created software rasterizer/renderer (handle to DLL), it should be non NULL if driver type is D3D_DRIVER_TYPE_SOFTWARE, else it should be NULL
			createDeviceFlags,             // Why to use the device we are creating ? RENDERING, COMPUTE, DEBUG
			&d3dFeatureLevel_required,     // What feature level we want ? provide Highest feature level we want
			numFeatureLevels,              // How many feature level we want ? generaly there are 6 feature levels we can get. We are asking to give 1 feature level.
			D3D11_SDK_VERSION,             // SDK version we are going to use
			&dxgiSwapChainDesc,            // Swap chain descriptor created above. We are sending pointer variable.
			&gpIDXGISwapChain,             // Swap chain OUT parameter
			&gpID3D11Device,               // Pointer to device created by this functions.
			&d3dFeatureLevel_acquired,     // Pointer to acquired feature level by this function
			&gpID3D11DeviceContext);       // Device context
		if (SUCCEEDED(hr)) //if found required device then break
			break;
	}

	//Code copied from sir
	if (FAILED(hr)) //if HR failed after loop
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "D3D11CreateDeviceAndSwapChain() Failed.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "D3D11CreateDeviceAndSwapChain() Succeeded.\n");
		fprintf_s(gpFile, "The Chosen Driver Is Of ");
		if (d3dDriverType == D3D_DRIVER_TYPE_HARDWARE)
		{
			fprintf_s(gpFile, "Hardware Type.\n");
		}
		else if (d3dDriverType == D3D_DRIVER_TYPE_WARP)
		{
			fprintf_s(gpFile, "Warp Type.\n");
		}
		else if (d3dDriverType == D3D_DRIVER_TYPE_REFERENCE)
		{
			fprintf_s(gpFile, "Reference Type.\n");
		}
		else
		{
			fprintf_s(gpFile, "Unknown Type.\n");
		}

		fprintf_s(gpFile, "The Supported Highest Feature Level Is ");
		if (d3dFeatureLevel_acquired == D3D_FEATURE_LEVEL_11_0)
		{
			fprintf_s(gpFile, "11.0\n");
		}
		else if (d3dFeatureLevel_acquired == D3D_FEATURE_LEVEL_10_1)
		{
			fprintf_s(gpFile, "10.1\n");
		}
		else if (d3dFeatureLevel_acquired == D3D_FEATURE_LEVEL_10_0)
		{
			fprintf_s(gpFile, "10.0\n");
		}
		else
		{
			fprintf_s(gpFile, "Unknown.\n");
		}

		fclose(gpFile);
	}

	//**************************************** Shader programs ******************************************************
	/*
	Common 6 Steps to in Shader:
	1. Write the shader source code
	2. Declare variable to hold compiled shaders byte code and error code if any
	3. Compile the shader
	4. Error checking of shader compilation
	5. Create the shader
	6. Set the shader at appropriate stage in pipe line
	*/


	//*******************************************  Vertex shader *******************************************

	/*Attributes in GLSL are similar to cbuffer(Constant buffer) in HLSL
	float4x4 -> mat4
	float4 -> vec4
	POSITION -> vPosition, POSITION is inbuilt symantic (attribute in glsl)
	:SV_POSITION indicate that, the return value of main function is System value like gl_position in glsl
	mul function is used to multiply the matrics
	to return multiple values we need to create struct
	*/
	//Step 1: Write shader source code
	const char *vertexShaderSourceCode =
		"cbuffer ConstantBuffer" \
		"{" \
		"float4x4 worldViewProjectionMatrix;" \
		"}" \
		"struct vertex_output" \
		"{" \
		"float4 position : SV_POSITION;" \
		"float2 texcoord : TEXCOORD;" \
		"float4 out_color : OUT_COLOR;" \
		"};" \
		"vertex_output main(float4 pos : POSITION, float2 texcoord : TEXCOORD, float4 color : VCOLOR)" \
		"{" \
		"vertex_output output;" \
		"output.position = mul(worldViewProjectionMatrix, pos);" \
		"output.texcoord = texcoord;" \
		"output.out_color=color;"
		"return(output);" \
		"}";

	//Step 2: Declare two variables
	ID3DBlob *pID3DBlob_VertexShaderCode = NULL;
	ID3DBlob *pID3DBlob_Error = NULL;

	//Step 3: Compile the shader
	/*Shader compiler exe name is fxe.exe called as Effect-Compiler tool */
	hr = D3DCompile(vertexShaderSourceCode, // Vertex shader source code
		lstrlenA(vertexShaderSourceCode) + 1, //length of shader source code. 'A' character indicate we want to use ANCI function. +1 is created to accomodate null terminated character at the end of string
		"VS", //name of Macro, which indicates that while compiling create VertexShader
		NULL, // struct of D3D_SHADER_MACRO, if we want to use external shader
		D3D_COMPILE_STANDARD_FILE_INCLUDE, //Include standerd file while compiling the shader program
		"main", //name of entry point function. It is not necessory to use name as 'main'
		"vs_5_0", //shader feature level. Curently level 5 is latest
		0, //Compiler constant. Ex. debug, optimization, validation
		0, // Effect constant. Ex. effect debug, effect optimize, effect validation
		&pID3DBlob_VertexShaderCode, //Compilerd source code in out parameter
		&pID3DBlob_Error  //if any error, will be returned in output parameter
		);

	// Step 4: Error checking
	if (FAILED(hr))
	{
		if (pID3DBlob_Error != NULL)
		{
			fopen_s(&gpFile, gszLogFileName, "a+");
			fprintf_s(gpFile, "Vertex shader compilation failed: %s.\n", (char *)pID3DBlob_Error->GetBufferPointer());
			fclose(gpFile);
			pID3DBlob_Error->Release();
			pID3DBlob_Error = NULL;
			return(hr);
		}
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Vertex shader compilation successful.\n");
		fclose(gpFile);
	}


	//Step 5: Create the shader
	/*
	Shader created on device
	Compiled source code sent as paramter

	*/
	hr = gpID3D11Device->CreateVertexShader(pID3DBlob_VertexShaderCode->GetBufferPointer(),
		pID3DBlob_VertexShaderCode->GetBufferSize(),
		NULL,
		&gpID3D11VertexShader); //Out parameter where verted shader object will be stored

	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Vertex shader object creation failed.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Vertex shader object creation success.\n");
		fclose(gpFile);
	}

	//Step 6: Attacher shader to pipeline
	gpID3D11DeviceContext->VSSetShader(gpID3D11VertexShader, 0, 0);

	//********************************* Pixel (Fragment) shader *************************************************************
	//Step 1: Write shader source code
	const char *pixelShaderSourceCode =
		"Texture2D myTexture2D;" \
		"SamplerState mySamplerState;" \
		"float4 main(float4 position : SV_POSITION, float2 texcoord : TEXCOORD, float4 outColor : OUT_COLOR) : SV_TARGET" \
		"{" \
		"float4 color = myTexture2D.Sample(mySamplerState, texcoord);" \
		"return(color+outColor);" \
		"}";

	//Step 2: declare two variable
	ID3DBlob *pID3DBlob_PixelShaderCode = NULL;
	pID3DBlob_Error = NULL; //re initialize the error hold variable

							//Step 3: Compile shader
	hr = D3DCompile(pixelShaderSourceCode,
		lstrlenA(pixelShaderSourceCode) + 1,
		"PS", //Shader name Pixel Shader
		NULL,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main",
		"ps_5_0",
		0,
		0,
		&pID3DBlob_PixelShaderCode,
		&pID3DBlob_Error);

	//Step 4: Error check
	if (FAILED(hr))
	{
		if (pID3DBlob_Error != NULL)
		{
			fopen_s(&gpFile, gszLogFileName, "a+");
			fprintf_s(gpFile, "Pixel shader compilation failed: %s.\n", (char *)pID3DBlob_Error->GetBufferPointer());
			fclose(gpFile);
			pID3DBlob_Error->Release();
			pID3DBlob_Error = NULL;
			return(hr);
		}
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Vertex shader compilation success.\n");
		fclose(gpFile);
	}

	//Step 5: Create shader object
	hr = gpID3D11Device->CreatePixelShader(pID3DBlob_PixelShaderCode->GetBufferPointer(),
		pID3DBlob_PixelShaderCode->GetBufferSize(),
		NULL,
		&gpID3D11PixelShader);
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "pixel shader object creation failed.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "pixel shader object creation success.\n");
		fclose(gpFile);
	}

	//Step 6: Attach shader to pipeline
	gpID3D11DeviceContext->PSSetShader(gpID3D11PixelShader, 0, 0);

	// Release pixel shader source code as it is blob
	pID3DBlob_PixelShaderCode->Release(); // We are releasing as we will not be passing any attribute in below code. Else we could not release.. VertexShader is not released as we need it.
	pID3DBlob_PixelShaderCode = NULL;

	//********************************************* Input layout {glBindAtrribLocation}
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[3]; //Structure
	ZeroMemory(&inputElementDesc, sizeof(D3D11_INPUT_ELEMENT_DESC)); //clear memory or zero memory

	inputElementDesc[0].SemanticName = "POSITION";
	inputElementDesc[0].SemanticIndex = 0;
	inputElementDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDesc[0].InputSlot = 0; // 0 for vertex position
	inputElementDesc[0].AlignedByteOffset = 0;
	inputElementDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA; //Vertex shader is per vertex data. Per pixes is pixel shader
	inputElementDesc[0].InstanceDataStepRate = 0; //Number of instances to draw 

	inputElementDesc[1].SemanticName = "TEXCOORD";
	inputElementDesc[1].SemanticIndex = 0;
	inputElementDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDesc[1].InputSlot = 1; //for textcords //this needs to be 1 as at 0 position we already sent vertices.
	inputElementDesc[1].AlignedByteOffset = 0;
	inputElementDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA; //Vertex shader is per vertex data. Per pixes is pixel shader
	inputElementDesc[1].InstanceDataStepRate = 0;

	inputElementDesc[2].SemanticName = "VCOLOR";
	inputElementDesc[2].SemanticIndex = 0;
	inputElementDesc[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDesc[2].InputSlot = 2; //for textcords //this needs to be 1 as at 0 position we already sent vertices.
	inputElementDesc[2].AlignedByteOffset = 0;
	inputElementDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA; //Vertex shader is per vertex data. Per pixes is pixel shader
	inputElementDesc[2].InstanceDataStepRate = 0;



	hr = gpID3D11Device->CreateInputLayout(inputElementDesc, _ARRAYSIZE(inputElementDesc), pID3DBlob_VertexShaderCode->GetBufferPointer(), pID3DBlob_VertexShaderCode->GetBufferSize(), &gpID3D11InputLayout);
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Input layout creation failed.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Input layout creation success.\n");
		fclose(gpFile);
	}
	gpID3D11DeviceContext->IASetInputLayout(gpID3D11InputLayout);
	pID3DBlob_VertexShaderCode->Release(); // NOTE : ID3DBlob obtained for vertex shader must be released after creating input layout
	pID3DBlob_VertexShaderCode = NULL;

	//**************************************************  END Input layout**********************************************************


	//********************************** cube Vertices ******************************************
	float cube_vertices[] =
	{
		// SIDE 1 ( TOP )
		// triangle 1
		-1.0f, +1.0f, +1.0f,
		+1.0f, +1.0f, +1.0f,
		-1.0f, +1.0f, -1.0f,
		// triangle 2
		-1.0f, +1.0f, -1.0f,
		+1.0f, +1.0f, +1.0f,
		+1.0f, +1.0f, -1.0f,

		// SIDE 2 ( BOTTOM )
		// triangle 1
		+1.0f, -1.0f, -1.0f,
		+1.0f, -1.0f, +1.0f,
		-1.0f, -1.0f, -1.0f,
		// triangle 2
		-1.0f, -1.0f, -1.0f,
		+1.0f, -1.0f, +1.0f,
		-1.0f, -1.0f, +1.0f,

		// SIDE 3 ( FRONT )
		// triangle 1
		-1.0f, +1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		// triangle 2
		-1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		+1.0f, -1.0f, -1.0f,

		// SIDE 4 ( BACK )
		// triangle 1
		+1.0f, -1.0f, +1.0f,
		+1.0f, +1.0f, +1.0f,
		-1.0f, -1.0f, +1.0f,
		// triangle 2
		-1.0f, -1.0f, +1.0f,
		+1.0f, +1.0f, +1.0f,
		-1.0f, +1.0f, +1.0f,

		// SIDE 5 ( LEFT )
		// triangle 1
		-1.0f, +1.0f, +1.0f,
		-1.0f, +1.0f, -1.0f,
		-1.0f, -1.0f, +1.0f,
		// triangle 2
		-1.0f, -1.0f, +1.0f,
		-1.0f, +1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,

		// SIDE 6 ( RIGHT )
		// triangle 1
		+1.0f, -1.0f, -1.0f,
		+1.0f, +1.0f, -1.0f,
		+1.0f, -1.0f, +1.0f,
		// triangle 2
		+1.0f, -1.0f, +1.0f,
		+1.0f, +1.0f, -1.0f,
		+1.0f, +1.0f, +1.0f,
	};

	//********************************** cube textcords ******************************************
	float cube_texcoords[] =
	{
		// SIDE 1 ( TOP )
		// triangle 1
		+0.0f, +0.0f,
		+0.0f, +1.0f,
		+1.0f, +0.0f,
		// triangle 2
		+1.0f, +0.0f,
		+0.0f, +1.0f,
		+1.0f, +1.0f,

		// SIDE 2 ( BOTTOM )
		// triangle 1
		+0.0f, +0.0f,
		+0.0f, +1.0f,
		+1.0f, +0.0f,
		// triangle 2
		+1.0f, +0.0f,
		+0.0f, +1.0f,
		+1.0f, +1.0f,

		// SIDE 3 ( FRONT )
		// triangle 1
		+0.0f, +0.0f,
		+0.0f, +1.0f,
		+1.0f, +0.0f,
		// triangle 2
		+1.0f, +0.0f,
		+0.0f, +1.0f,
		+1.0f, +1.0f,

		// SIDE 4 ( BACK )
		// triangle 1
		+0.0f, +0.0f,
		+0.0f, +1.0f,
		+1.0f, +0.0f,
		// triangle 2
		+1.0f, +0.0f,
		+0.0f, +1.0f,
		+1.0f, +1.0f,

		// SIDE 5 ( LEFT )
		// triangle 1
		+0.0f, +0.0f,
		+0.0f, +1.0f,
		+1.0f, +0.0f,
		// triangle 2
		+1.0f, +0.0f,
		+0.0f, +1.0f,
		+1.0f, +1.0f,

		// SIDE 6 ( RIGHT )
		// triangle 1
		+0.0f, +0.0f,
		+0.0f, +1.0f,
		+1.0f, +0.0f,
		// triangle 2
		+1.0f, +0.0f,
		+0.0f, +1.0f,
		+1.0f, +1.0f,
	};

	// PYRAMID
	// create vertex buffer
	D3D11_BUFFER_DESC bufferDesc; // declare buffer ~ vbo. Structure 

	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(float) * _ARRAYSIZE(cube_vertices); // size is sizeof(float) * 3 because there are 3 rows in vertices array
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;     //This buffer need to bind to vertex buffer.
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;  // allow CPU to write into this buffer
	hr = gpID3D11Device->CreateBuffer(&bufferDesc, NULL, &gpID3D11Buffer_VertexBuffer_Position_Cube);
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "ID3D11Device::CreateBuffer() Failed For Vertex Buffer For Cube Position.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "ID3D11Device::CreateBuffer() Succeeded For Vertex Buffer For Cube Position.\n");
		fclose(gpFile);
	}

	// copy vertices into above buffer
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	ZeroMemory(&mappedSubresource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	gpID3D11DeviceContext->Map(gpID3D11Buffer_VertexBuffer_Position_Cube, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubresource); // map buffer
	memcpy(mappedSubresource.pData, cube_vertices, sizeof(float)*_ARRAYSIZE(cube_vertices));
	gpID3D11DeviceContext->Unmap(gpID3D11Buffer_VertexBuffer_Position_Cube, NULL);

	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(float) * _ARRAYSIZE(cube_texcoords);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;     //This buffer need to bind to vertex buffer.
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;  // allow CPU to write into this buffer
	hr = gpID3D11Device->CreateBuffer(&bufferDesc, NULL, &gpID3D11Buffer_VertexBuffer_Texture_Cube);
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Vertex Buffer creation failed for Cube Texture.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Vertex Buffer creation successfulull for Cube Texture.\n");
		fclose(gpFile);
	}

	// copy vertex texcoords into above buffer
	ZeroMemory(&mappedSubresource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	gpID3D11DeviceContext->Map(gpID3D11Buffer_VertexBuffer_Texture_Cube, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubresource); // map buffer
	memcpy(mappedSubresource.pData, cube_texcoords, sizeof(cube_texcoords));
	gpID3D11DeviceContext->Unmap(gpID3D11Buffer_VertexBuffer_Texture_Cube, NULL);


	float colorQuads[] = //winding orer is left hand (anti clock wise) winding order for DirectX. OpenGL winding order was right hand
	{
		1.0f, 0.0f, 0.0f, //RED
		1.0f, 0.0f, 0.0f, //RED
		1.0f, 0.0f, 0.0f, //RED

		1.0f, 0.0f, 0.0f, //RED
		1.0f, 0.0f, 0.0f, //RED
		1.0f, 0.0f, 0.0f, //RED

		1.0f, 0.0f, 1.0f, //MAGENTA
		1.0f, 0.0f, 1.0f, //MAGENTA
		1.0f, 0.0f, 1.0f, //MAGENTA

		1.0f, 0.0f, 1.0f, //MAGENTA
		1.0f, 0.0f, 1.0f, //MAGENTA
		1.0f, 0.0f, 1.0f, //MAGENTA

		0.0f, 1.0f, 1.0f, //CYAN
		0.0f, 1.0f, 1.0f, //CYAN
		0.0f, 1.0f, 1.0f, //CYAN

		0.0f, 1.0f, 1.0f, //CYAN
		0.0f, 1.0f, 1.0f, //CYAN
		0.0f, 1.0f, 1.0f, //CYAN

		0.0f, 0.0f, 1.0f, //BLUE
		0.0f, 0.0f, 1.0f, //BLUE
		0.0f, 0.0f, 1.0f, //BLUE

		0.0f, 0.0f, 1.0f, //BLUE
		0.0f, 0.0f, 1.0f, //BLUE
		0.0f, 0.0f, 1.0f, //BLUE

		0.0f, 1.0f, 0.0f, //GREEN
		0.0f, 1.0f, 0.0f, //GREEN
		0.0f, 1.0f, 0.0f, //GREEN

		0.0f, 1.0f, 0.0f, //GREEN
		0.0f, 1.0f, 0.0f, //GREEN
		0.0f, 1.0f, 0.0f, //GREEN

		1.0f, 1.0f, 0.0f, //YELLOW
		1.0f, 1.0f, 0.0f, //YELLOW
		1.0f, 1.0f, 0.0f, //YELLOW

		1.0f, 1.0f, 0.0f, //YELLOW
		1.0f, 1.0f, 0.0f, //YELLOW
		1.0f, 1.0f, 0.0f, //YELLOW

	};


	bufferDesc; // declare buffer ~ vbo. Structure 
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC)); //clear memory or zero memory


	bufferDesc.Usage = D3D11_USAGE_DYNAMIC; //Dynamic draw, GL_STATIC_DRAW or GL_DYNAMIC_DRAW
	bufferDesc.ByteWidth = sizeof(float) * _ARRAYSIZE(colorQuads); //sizeof(float) * sizeof(vertices).
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; //This buffer need to bind to vertex buffer.
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; //allowing GPU to write on to this buffer

	hr = gpID3D11Device->CreateBuffer(&bufferDesc, NULL, &gpID3D11Buffer_VertexBuffer_quads_color); //Create VBO
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Vertext buffer creation failed for color.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Vertext buffer creation success for color.\n");
		fclose(gpFile);
	}

	//Map resource (vertex buffer) with device context. Map the vertex data to subresource
	mappedSubresource;
	ZeroMemory(&mappedSubresource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	gpID3D11DeviceContext->Map(gpID3D11Buffer_VertexBuffer_quads_color, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource); //Map vertex buffer to this resources.
																															//(vertexBiffer, are we sending multiple data{vertex,color,texture},We want to discrard older data, should we send data now ?(or static or dynamic draw , adress of subrespurce )
	memcpy(mappedSubresource.pData, colorQuads, sizeof(float) * _ARRAYSIZE(colorQuads)); //copy vertertces to mappedSubResource data in pData
	gpID3D11DeviceContext->Unmap(gpID3D11Buffer_VertexBuffer_quads_color, 0); //Once copied data to subresources then unmap the vertexBuffer from device context

																			  /* If it was static draw then we need to put below call, but as we are in DYNAMIC draw this callis in draw
																			  gpID3D11DeviceContext->IASetVertexBuffers(0, 1, &gpID3D11Buffer_VertexBuffer, &stride, &offset);
																			  */




																			  //*************************************************************************************************************************	




																			  //**********************************************************************************************
																			  // define and set the constant buffer
	D3D11_BUFFER_DESC bufferDesc_ConstantBuffer;
	ZeroMemory(&bufferDesc_ConstantBuffer, sizeof(D3D11_BUFFER_DESC));
	bufferDesc_ConstantBuffer.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc_ConstantBuffer.ByteWidth = sizeof(CBUFFER); // see the declaration of 'CBUFFER' above [ XMMATRIX means float4x4 means 4*4 = 16 float values, each float is 4, so 16*4 = 64 ]
	bufferDesc_ConstantBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hr = gpID3D11Device->CreateBuffer(&bufferDesc_ConstantBuffer, nullptr, &gpID3D11Buffer_ConstantBuffer);
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Constant Buffer creation failed.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Constant Buffer creation succeed.\n");
		fclose(gpFile);
	}
	gpID3D11DeviceContext->VSSetConstantBuffers(0, 1, &gpID3D11Buffer_ConstantBuffer);

	//**********  Copied code from Sir - need more deep dive
	// set rasterization state 
	// In D3D, backface culling is by default 'ON'
	// Means backface of geometry will not be visible.
	// this causes our 2D triangles backface to vanish on rotation.
	// so set culling 'OFF'
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory((void *)&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.CullMode = D3D11_CULL_NONE; // this allows both sides of 2D geometry visible on rotation
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.ScissorEnable = FALSE;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	hr = gpID3D11Device->CreateRasterizerState(&rasterizerDesc, &gpID3D11RasterizerState);
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "ID3D11Device::CreateRasterizerState() Failed For Culling.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "ID3D11Device::CreateRasterizerState() Succeeded For Culling.\n");
		fclose(gpFile);
	}
	gpID3D11DeviceContext->RSSetState(gpID3D11RasterizerState);

	// PYRAMID
	// create texture resource and texture view
	// CUBE
	// create texture resource and texture view
	hr = LoadD3DTexture(L"Vijay_Kundali.bmp", &gpID3D11ShaderResourceView_Texture_Cube);
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "LoadD3DTexture() Failed For Cube Texture.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "LoadD3DTexture() Succeeded For Cube Texture.\n");
		fclose(gpFile);
	}

	// Create the sample state
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	hr = gpID3D11Device->CreateSamplerState(&samplerDesc, &gpID3D11SamplerState_Texture_Cube);
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "ID3D11Device::CreateSamplerState() Failed For Cube Texture.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "ID3D11Device::CreateSamplerState() Succeeded For Cube Texture.\n");
		fclose(gpFile);
	}

	// d3d clear color
	// black
	gClearColor[0] = 0.0f;
	gClearColor[1] = 0.0f;
	gClearColor[2] = 0.0f;
	gClearColor[3] = 1.0f;

	// set projection matrix to identity matrix
	gPerspectiveProjectionMatrix = XMMatrixIdentity();

	// call resize for first time
	hr = resize(WIN_WIDTH, WIN_HEIGHT);
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "resize() Failed.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "resize() Succeeded.\n");
		fclose(gpFile);
	}

	return(S_OK);
}

HRESULT LoadD3DTexture(const wchar_t *textureFileName, ID3D11ShaderResourceView **ppID3D11ShaderResourceView)
{
	// code
	HRESULT hr;

	// create texture
	hr = DirectX::CreateWICTextureFromFile(gpID3D11Device, gpID3D11DeviceContext, textureFileName, NULL, ppID3D11ShaderResourceView);
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "CreateWICTextureFromFile() Failed For Texture Resource.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "CreateWICTextureFromFile() Succeeded For Texture Resource.\n");
		fclose(gpFile);
	}

	return(hr);
}

HRESULT resize(int width, int height)
{
	//result variable use for error checking
	HRESULT hr = S_OK;

	if (gpID3D11DepthStencilView)
	{
		gpID3D11DepthStencilView->Release();
		gpID3D11DepthStencilView = NULL;
	}

	/*
	If already target view is present then release it. It is required as we alway need to create render target view if window size changed.
	We checked it becasue for if this call from initialize then renderTargetView will not present.
	*/
	if (gpID3D11RenderTargetView)
	{
		gpID3D11RenderTargetView->Release();
		gpID3D11RenderTargetView = NULL;
	}

	//After resize the window then swapChainBuffer also need to change. Below call resize the buffer
	gpIDXGISwapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	// How many buffer to resize, width, height, DXGI format, SWAP_CHAIN_FLAG (Rendered, debug ??) 0 if best suitable method 

	// We need one more buffer to use as back buffer (for double buffering).
	ID3D11Texture2D *pID3D11Texture2D_BackBuffer; //We need buffer which can have color, so we are using Textuer2D type
	gpIDXGISwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pID3D11Texture2D_BackBuffer); //LPVOID* is equavalent to void**
																									  //Give buffer at 0th index, iid(GUID) of type ID3D11TextureType, and point buffer to pID3D11Texture2D_BackBuffer variable

																									  // Convert above back buffer to render target view. RTV is used by device so call the function on device
	hr = gpID3D11Device->CreateRenderTargetView(pID3D11Texture2D_BackBuffer, NULL, &gpID3D11RenderTargetView);
	//(backBuffer, pointer to render target view additional description {NULL}, and outParameter for renderTargetView

	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Create render target view failed in Resize function.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Create render target view succeeed in Resize function.\n");
		fclose(gpFile);
	}
	pID3D11Texture2D_BackBuffer->Release(); //Release back buffer as it not of use as we have created Render Target view using this buffer.
	pID3D11Texture2D_BackBuffer = NULL; //Explicit nullification


										// create depth stencil buffer ( or zbuffer )
	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(D3D11_TEXTURE2D_DESC));
	textureDesc.Width = (UINT)width;
	textureDesc.Height = (UINT)height;
	textureDesc.ArraySize = 1;
	textureDesc.MipLevels = 1;
	textureDesc.SampleDesc.Count = 1; // in real world, this can be up to 4
	textureDesc.SampleDesc.Quality = 0; // if above is 4, then it is 1
	textureDesc.Format = DXGI_FORMAT_D32_FLOAT;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	ID3D11Texture2D *pID3D11Texture2D_DepthBuffer;
	gpID3D11Device->CreateTexture2D(&textureDesc, NULL, &pID3D11Texture2D_DepthBuffer);

	// create depth stencil view from above depth stencil buffer
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	hr = gpID3D11Device->CreateDepthStencilView(pID3D11Texture2D_DepthBuffer, &depthStencilViewDesc, &gpID3D11DepthStencilView);
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "ID3D11Device::CreateDepthStencilView() Failed.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "ID3D11Device::CreateDepthStencilView() Succeeded.\n");
		fclose(gpFile);
	}
	pID3D11Texture2D_DepthBuffer->Release();
	pID3D11Texture2D_DepthBuffer = NULL;

	/*
	set render target view as render target. OMS - Output Mergerer Stage of pipeline

	The output-merger (OM) stage generates the final rendered pixel color using a combination of pipeline state,
	the pixel data generated by the pixel shaders, the contents of the render targets,
	and the contents of the depth/stencil buffers.
	The OM stage is the final step for determining which pixels are visible (with depth-stencil testing)
	and blending the final pixel colors
	*/
	gpID3D11DeviceContext->OMSetRenderTargets(1, &gpID3D11RenderTargetView, gpID3D11DepthStencilView);
	// How many Target to set '1', RenderTarget? , DepthStencil? 'NULL' as no depth as of now

	// CreateViewPort and set it to device context. Similar to glViewPort()
	D3D11_VIEWPORT d3dViewPort;
	d3dViewPort.TopLeftX = 0;
	d3dViewPort.TopLeftY = 0;
	d3dViewPort.Width = (float)width;
	d3dViewPort.Height = (float)height;
	d3dViewPort.MinDepth = 0.0f;
	d3dViewPort.MaxDepth = 1.0f;
	gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);

	// set perspective matrix
	// 'LH' indicates 'Left Handed' co-ordinate system that Direct3D has 
	gPerspectiveProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

	return(hr);
}

void display(void)
{
	//Clear the render target view and give default color (blue). It is similar to glClearColor)
	gpID3D11DeviceContext->ClearRenderTargetView(gpID3D11RenderTargetView, gClearColor);

	// clear the depth/stencil view to 1.0
	gpID3D11DeviceContext->ClearDepthStencilView(gpID3D11DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	//************************************************** QUAD **************************************************
	// select which vertex buffer to display
	UINT stride = sizeof(float) * 3; // 3 is for x, y, z
	UINT offset = 0;//Offset required 



	stride = sizeof(float) * 3; // 3 is for x, y, z
	offset = 0;
	gpID3D11DeviceContext->IASetVertexBuffers(0, 1, &gpID3D11Buffer_VertexBuffer_Position_Cube, &stride, &offset);

	gpID3D11DeviceContext->IASetVertexBuffers(2, 1, &gpID3D11Buffer_VertexBuffer_quads_color, &stride, &offset);

	stride = sizeof(float) * 2; // 2 is for u, v
	offset = 0;
	gpID3D11DeviceContext->IASetVertexBuffers(1, 1, &gpID3D11Buffer_VertexBuffer_Texture_Cube, &stride, &offset); // 1st param '1' is for 1st slot used for 'color'

																												  // bind texture and sampler as pixel shader resource
	gpID3D11DeviceContext->PSSetShaderResources(0, 1, &gpID3D11ShaderResourceView_Texture_Cube);
	gpID3D11DeviceContext->PSSetSamplers(0, 1, &gpID3D11SamplerState_Texture_Cube);

	// select geometry primtive
	gpID3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// translation is concerned with world matrix transformation
	XMMATRIX translationMatrix = XMMatrixTranslation(0.0f, 0.0f, 6.0f);

	XMMATRIX r1 = XMMatrixRotationX(angleQuads); // +ve = right to left rotation, -ve = left to right rotation
	XMMATRIX r2 = XMMatrixRotationY(angleQuads); // +ve = right to left rotation, -ve = left to right rotation
	XMMATRIX r3 = XMMatrixRotationZ(angleQuads); // +ve = right to left rotation, -ve = left to right rotation
	XMMATRIX  rotationMatrix = r1 * r2 * r3;

	XMMATRIX scaleMatrix = XMMatrixScaling(0.75f, 0.75f, 0.75f);
	XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix; // ORDER IS VERY ... VERY ... VERY ... IMPORTANT

	XMMATRIX viewMatrix = XMMatrixIdentity();

	// final WorldViewProjection matrix
	XMMATRIX wvpMatrix = worldMatrix * viewMatrix * gPerspectiveProjectionMatrix;
	CBUFFER constantBuffer;
	// load the data into the constant buffer
	constantBuffer.WorldViewProjectionMatrix = wvpMatrix;
	gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);

	// draw vertex buffer to render target
	// cube has 6 surfaces, each with 2 triangles, each with 3 vertices, 
	// so total 36 vertices in our "vertices" array in D3DInitialize()
	gpID3D11DeviceContext->Draw(6, 0); // 6 vertices from 0 to 6 
	gpID3D11DeviceContext->Draw(6, 6); // 6 vertices from 6 to 12
	gpID3D11DeviceContext->Draw(6, 12); // 6 vertices from 12 to 18
	gpID3D11DeviceContext->Draw(6, 18); // 6 vertices from 18 to 24
	gpID3D11DeviceContext->Draw(6, 24); // 6 vertices from 24 to 30
	gpID3D11DeviceContext->Draw(6, 30); // 6 vertices from 30 to 36

										// switch between frot & back buffers
	gpIDXGISwapChain->Present(0, 0);
}

void update(void)
{
	// code

	angleQuads = angleQuads - 0.002f;
	if (angleQuads <= 0.0f)
		angleQuads = angleQuads + 360.0f;
}

void uninitialize(void)
{
	// code
	if (gpID3D11SamplerState_Texture_Cube)
	{
		gpID3D11SamplerState_Texture_Cube->Release();
		gpID3D11SamplerState_Texture_Cube = NULL;
	}

	if (gpID3D11ShaderResourceView_Texture_Cube)
	{
		gpID3D11ShaderResourceView_Texture_Cube->Release();
		gpID3D11ShaderResourceView_Texture_Cube = NULL;
	}

	if (gpID3D11RasterizerState)
	{
		gpID3D11RasterizerState->Release();
		gpID3D11RasterizerState = NULL;
	}

	if (gpID3D11Buffer_ConstantBuffer)
	{
		gpID3D11Buffer_ConstantBuffer->Release();
		gpID3D11Buffer_ConstantBuffer = NULL;
	}

	if (gpID3D11InputLayout)
	{
		gpID3D11InputLayout->Release();
		gpID3D11InputLayout = NULL;
	}

	if (gpID3D11Buffer_VertexBuffer_Position_Cube)
	{
		gpID3D11Buffer_VertexBuffer_Position_Cube->Release();
		gpID3D11Buffer_VertexBuffer_Position_Cube = NULL;
	}

	if (gpID3D11Buffer_VertexBuffer_Texture_Cube)
	{
		gpID3D11Buffer_VertexBuffer_Texture_Cube->Release();
		gpID3D11Buffer_VertexBuffer_Texture_Cube = NULL;
	}

	if (gpID3D11PixelShader)
	{
		gpID3D11PixelShader->Release();
		gpID3D11PixelShader = NULL;
	}

	if (gpID3D11VertexShader)
	{
		gpID3D11VertexShader->Release();
		gpID3D11VertexShader = NULL;
	}

	if (gpID3D11DepthStencilView)
	{
		gpID3D11DepthStencilView->Release();
		gpID3D11DepthStencilView = NULL;
	}

	if (gpID3D11RenderTargetView)
	{
		gpID3D11RenderTargetView->Release();
		gpID3D11RenderTargetView = NULL;
	}

	if (gpIDXGISwapChain)
	{
		gpIDXGISwapChain->Release();
		gpIDXGISwapChain = NULL;
	}

	if (gpID3D11DeviceContext)
	{
		gpID3D11DeviceContext->Release();
		gpID3D11DeviceContext = NULL;
	}

	if (gpID3D11Device)
	{
		gpID3D11Device->Release();
		gpID3D11Device = NULL;
	}

	if (gpFile)
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "uninitialize() Succeeded\n");
		fprintf_s(gpFile, "Log File Is Successfully Closed.\n");
		fclose(gpFile);
	}
}
