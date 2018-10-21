/*
1. To manupulate any stage of pipeline, only DeviceContext can be used.
2. Macros starting with D3D_ are common to all DirectX versions. Macros starting with D3D11_ are specific to 11 version
*/

#include <windows.h>
#include<stdio.h> // for file I/O

#include<d3d11.h>
#include<d3dcompiler.h>

#pragma warning(disable:4438)
#include "XNAMath\xnamath.h"
#include "Sphere.h"

#pragma comment (lib,"d3d11.lib")
#pragma comment(lib, "D3dcompiler.lib")


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

ID3D11Buffer *gpID3D11Buffer_VertexBuffer = NULL; //vertex buffer vbo_position
ID3D11Buffer *gpID3D11Buffer_VertexBuffer_normal = NULL;
ID3D11Buffer *gpID3D11Buffer_ConstantBuffer = NULL; //constant buffer 

ID3D11Buffer *gpID3D11Buffer_IndexBuffer = NULL;

float angleRotateRed = 0.0f;
float angleRotateGreen = 90.0f;
float angleRotateBlue = 180.0f;

ID3D11InputLayout *gpID3D11InputLayout = NULL; //

bool gbAnimate = false;
bool gbLight = false;

unsigned int gNumElements;
unsigned int gNumVertices;

float sphere_vertices[1146];
float sphere_normals[1146];
float sphere_textures[764];
short sphere_elements[2280];

Sphere *sphere = new Sphere();

float lightAmbient[] = { 0.0f,0.0f,0.0f,1.0f };
float lightSpecular[] = { 1.0f,1.0f,1.0f,1.0f };

float lightDiffuse_red[] = { 1.0f,1.0f,1.0f,1.0f };
float lightDiffuse_green[] = { 1.0f,1.0f,1.0f,1.0f };
float lightDiffuse_blue[] = { 1.0f,1.0f,1.0f,1.0f };

float lightPosition_red[] = { 0.0f,0.0f,0.0f,0.0f };
float lightPosition_green[] = { 0.0f,0.0f,0.0f,0.0f };
float lightPosition_blue[] = { 0.0f,0.0f,0.0f,0.0f };

float material_ambient[] = { 0.0f,0.0f,0.0f,1.0f };
float material_diffuse[] = { 1.0f,1.0f,1.0f,1.0f };
float material_specular[] = { 1.0f,1.0f,1.0f,1.0f };
float material_shininess = 50.0f;

struct CBUFFER
{
	XMMATRIX model_matrix;
	XMMATRIX view_matrix;
	XMMATRIX projection_matrix;

	XMVECTOR Ld_red_uniform;
	XMVECTOR Ld_green_uniform;
	XMVECTOR Ld_blue_uniform;

	XMVECTOR Ls_uniform;
	XMVECTOR La_uniform;

	XMVECTOR Lp_red_uniform;
	XMVECTOR Lp_green_uniform;
	XMVECTOR Lp_blue_uniform;

	XMVECTOR Kd_uniform;
	XMVECTOR Ka_uniform;
	XMVECTOR Ks_uniform;

	float shininess;

	unsigned int L_KeyPressed_uniform;

};

XMMATRIX gPerspectiveProjectionMatrix;

void updateAngle(void);

bool bIsAKeyPressed = false;
bool bIsLKeyPressed = false;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
	// function declarations
	HRESULT initialize(void);
	void uninitialize(void);
	void display(void);

	// variable declarations
	WNDCLASSEX wndclass;
	HWND hwnd;
	MSG msg;
	TCHAR szClassName[] = TEXT("D3D11:3D Rotating Shapes Colored");
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
		TEXT("D3D11:24 Material with Rotating Lights- PerPixel"),
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

	// initialize D3D
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
			updateAngle();

			if (gbActiveWindow == true)
			{
				if (gbEscapeKeyIsPressed == true)
					bDone = true;
			}
		}
	}

	uninitialize();
	return((int)msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HRESULT resize(int, int);

	void ToggleFullscreen(void);
	void uninitialize(void);
	HRESULT hr;

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
		case 0x41: // for 'A' or 'a'
			if (bIsAKeyPressed == false)
			{


				gbAnimate = true;
				bIsAKeyPressed = true;
			}
			else
			{
				gbAnimate = false;
				bIsAKeyPressed = false;
			}
			break;
		case 0x4C: // for 'L' or 'l'
			if (bIsLKeyPressed == false)
			{
				fopen_s(&gpFile, gszLogFileName, "a+");
				fprintf_s(gpFile, "L key presed, light is on.\n");
				fclose(gpFile);
				gbLight = true;
				bIsLKeyPressed = true;
			}
			else
			{
				fopen_s(&gpFile, gszLogFileName, "a+");
				fprintf_s(gpFile, "L key presed, light is Off.\n");
				fclose(gpFile);
				gbLight = false;
				bIsLKeyPressed = false;
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
	// variable declarations
	MONITORINFO mi;

	// code
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
	void uninitialize(void);
	HRESULT resize(int, int);
	HRESULT LoadD3DTexture(const wchar_t *, ID3D11ShaderResourceView **);


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


	//******************  Vertex shader *********************

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
		"float4x4 model_matrix;" \
		"float4x4 view_matrix;" \
		"float4x4 projection_matrix;" \

		"float4 Ld_red_uniform;"\
		"float4 Ld_green_uniform;"\
		"float4 Ld_blue_uniform;"\

		"float4 Ls_uniform;"\
		"float4 La_uniform;"\

		"float4 Lp_red_uniform;"\
		"float4 Lp_green_uniform;"\
		"float4 Lp_blue_uniform;"\

		"float4 Kd_uniform;"\
		"float4 Ka_uniform;"\
		"float4 Ks_uniform;"\

		"float shininess;"\

		"uint L_KeyPressed_uniform;"\
		"}" \
		"struct out_vertex"\
		"{"\
		"float4 position:SV_POSITION;"\
		"float3 trabsformed_normal:TNORMAL;"\
		"float3 light_direction_red:LDIRECTION_RED;"\
		"float3 light_direction_green:LDIRECTION_GREEN;"\
		"float3 light_direction_blue:LDIRECTION_BLUE;"\
		"float3 viewer_vector:VVECTOR;"\
		"};"\

		"out_vertex main(float4 pos : POSITION, float4 normal : NORMAL)" \
		"{" \
		"out_vertex output;"\
		"if(L_KeyPressed_uniform==1)"\
		"{"\
		"float3 eyeCoordinated=mul((float3x3)model_matrix , (float3)pos);"\
		"eyeCoordinated=mul((float3x3)view_matrix,eyeCoordinated );"\
		"float3 transformed_normals=mul(mul((float3x3)view_matrix,(float3x3)model_matrix),(float3)normal);"\
		"float3 viewer_vector = -eyeCoordinated.xyz;" \

		"float3 light_direction_red = (float3)Lp_red_uniform-eyeCoordinated.xyz;"\
		"float3 light_direction_green = (float3)Lp_green_uniform-eyeCoordinated.xyz;"\
		"float3 light_direction_blue = (float3)Lp_blue_uniform-eyeCoordinated.xyz;"\

		"output.trabsformed_normal=transformed_normals;"\
		"output.viewer_vector=viewer_vector;"\

		"output.light_direction_red=light_direction_red;"\
		"output.light_direction_green=light_direction_green;"\
		"output.light_direction_blue=light_direction_blue;"\

		/*saturate can be used inplace of max function in D3D*/
		"}"\

		"float4x4 localposition = mul(projection_matrix, view_matrix);"
		"localposition =mul(localposition ,model_matrix );"\
		"output.position=mul(localposition,pos);"
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
		"cbuffer ConstantBuffer" \
		"{" \
		"float4x4 model_matrix;" \
		"float4x4 view_matrix;" \
		"float4x4 projection_matrix;" \

		"float4 Ld_red_uniform;"\
		"float4 Ld_green_uniform;"\
		"float4 Ld_blue_uniform;"\

		"float4 La_uniform;"\
		"float4 Ls_uniform;"\

		"float4 Lp_red_uniform;"\
		"float4 Lp_green_uniform;"\
		"float4 Lp_blue_uniform;"\

		"float4 Kd_uniform;"\
		"float4 Ka_uniform;"\
		"float4 Ks_uniform;"\

		"float shininess;"\


		"uint L_KeyPressed_uniform;"\
		"}" \
		"struct out_vertex"\
		"{"\
		"float4 position:SV_POSITION;"\
		"float3 trabsformed_normal:TNORMAL;"\
		"float3 light_direction_red:LDIRECTION_RED;"\
		"float3 light_direction_green:LDIRECTION_GREEN;"\
		"float3 light_direction_blue:LDIRECTION_BLUE;"\
		"float3 viewer_vector:VVECTOR;"\
		"};"\

		"float4 main(out_vertex input) : SV_TARGET" \
		"{" \
		"float4 phong_ads_color;" \

		"if(L_KeyPressed_uniform==1)" \
		"{" \
		"float3 normalized_transformed_normals=normalize(input.trabsformed_normal);" \
		"float3 normalized_light_direction=normalize(input.light_direction_red);" \
		"float3 normalized_viewer_vector=normalize(float3(input.viewer_vector.x,input.viewer_vector.y,input.viewer_vector.z));" \
		"float tn_dot_ld = max(dot(normalized_transformed_normals,normalized_light_direction),0.0);"\
		"float3 ambient = (float3)La_uniform * (float3)Ka_uniform;" \
		"float3 diffuse = (float3)Ld_red_uniform * (float3)Kd_uniform * tn_dot_ld;" \
		"float3 reflection_vector = reflect(-normalized_light_direction, normalized_transformed_normals);" \
		"float3 LsKs_mul=(float3)Ls_uniform * (float3)Ks_uniform;"\
		"float3 RvVv = pow(max(dot(reflection_vector, normalized_viewer_vector), 0.0), shininess); "\
		"float3 specular = (float3)LsKs_mul * (float3) RvVv;" \
		"float3 outColor=ambient + diffuse + specular;"
		"phong_ads_color=float4(outColor,1.0);" \

		" normalized_light_direction=normalize(input.light_direction_green);" \
		" tn_dot_ld = max(dot(normalized_transformed_normals,normalized_light_direction),0.0);"\
		" diffuse = (float3)Ld_green_uniform * (float3)Kd_uniform * tn_dot_ld;" \
		" reflection_vector = reflect(-normalized_light_direction, normalized_transformed_normals);" \
		" LsKs_mul=(float3)Ls_uniform * (float3)Ks_uniform;"\
		" RvVv = pow(max(dot(reflection_vector, normalized_viewer_vector), 0.0), shininess); "\
		" specular = LsKs_mul* RvVv;" \
		"phong_ads_color=phong_ads_color+float4(ambient + diffuse + specular,1.0);" \

		" normalized_light_direction=normalize(input.light_direction_blue);" \
		" tn_dot_ld = max(dot(normalized_transformed_normals,normalized_light_direction),0.0);"\
		" diffuse = (float3)Ld_blue_uniform * (float3)Kd_uniform * tn_dot_ld;" \
		" reflection_vector = reflect(-normalized_light_direction, normalized_transformed_normals);" \
		" LsKs_mul=(float3)Ls_uniform * (float3)Ks_uniform;"\
		" RvVv = pow(max(dot(reflection_vector, normalized_viewer_vector), 0.0), shininess); "\
		" specular = LsKs_mul * RvVv;" \
		"phong_ads_color=phong_ads_color+float4(ambient + diffuse + specular,1.0);" \

		"}" \
		"else" \
		"{" \
		"phong_ads_color=float4(1.0,1.0,1.0,1.0);"\
		"}" \
		"return (phong_ads_color);" \
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
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[2]; //Structure

	ZeroMemory(&inputElementDesc, sizeof(D3D11_INPUT_ELEMENT_DESC)); //clear memory or zero memory

	inputElementDesc[0].SemanticName = "POSITION";
	inputElementDesc[0].SemanticIndex = 0;
	inputElementDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDesc[0].InputSlot = 0;
	inputElementDesc[0].AlignedByteOffset = 0;
	inputElementDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;//Vertex shader is per vertex data. Per pixes is pixel shader
	inputElementDesc[0].InstanceDataStepRate = 0; //Number of instances to draw 

	inputElementDesc[1].SemanticName = "NORMAL";
	inputElementDesc[1].SemanticIndex = 0;
	inputElementDesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDesc[1].InputSlot = 1; //this needs to be 1 as at 0 position we already sent vertices.
	inputElementDesc[1].AlignedByteOffset = 0;
	inputElementDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;//Vertex shader is per vertex data. Per pixes is pixel shader
	inputElementDesc[1].InstanceDataStepRate = 0; //Number of instances to draw 


	hr = gpID3D11Device->CreateInputLayout(inputElementDesc, 2, pID3DBlob_VertexShaderCode->GetBufferPointer(), pID3DBlob_VertexShaderCode->GetBufferSize(), &gpID3D11InputLayout);
	//(inputLaoutArray{position,color,textcord},num of element {only vertecies},VertexShaderCode pointer, size of vertex shader size )
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
	gpID3D11DeviceContext->IASetInputLayout(gpID3D11InputLayout); //Set input layout to device context
	pID3DBlob_VertexShaderCode->Release();
	pID3DBlob_VertexShaderCode = NULL;
	//**************************************************  END Input layout**********************************************************

	//********************************** Triangle Vertices ******************************************
	sphere->getSphereVertexData(sphere_vertices, sphere_normals, sphere_textures, sphere_elements);

	gNumVertices = sphere->getNumberOfSphereVertices();
	gNumElements = sphere->getNumberOfSphereElements();


	D3D11_BUFFER_DESC bufferDesc; // declare buffer ~ vbo. Structure 
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC)); //clear memory or zero memory


	bufferDesc.Usage = D3D11_USAGE_DYNAMIC; //Dynamic draw, GL_STATIC_DRAW or GL_DYNAMIC_DRAW
	bufferDesc.ByteWidth = sizeof(sphere_vertices); //sizeof(float) * sizeof(vertices).
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; //This buffer need to bind to vertex buffer.
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; //allowing GPU to write on to this buffer

	hr = gpID3D11Device->CreateBuffer(&bufferDesc, NULL, &gpID3D11Buffer_VertexBuffer); //Create VBO
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Vertext buffer creation failed.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Vertext buffer creation failed.\n");
		fclose(gpFile);
	}

	//Map resource (vertex buffer) with device context. Map the vertex data to subresource
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	ZeroMemory(&mappedSubresource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	gpID3D11DeviceContext->Map(gpID3D11Buffer_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource); //Map vertex buffer to this resources.
																												//(vertexBiffer, are we sending multiple data{vertex,color,texture},We want to discrard older data, should we send data now ?(or static or dynamic draw , adress of subrespurce )
	memcpy(mappedSubresource.pData, sphere_vertices, sizeof(sphere_vertices)); //copy vertertces to mappedSubResource data in pData
	gpID3D11DeviceContext->Unmap(gpID3D11Buffer_VertexBuffer, 0); //Once copied data to subresources then unmap the vertexBuffer from device context

																  /* If it was static draw then we need to put below call, but as we are in DYNAMIC draw this callis in draw
																  gpID3D11DeviceContext->IASetVertexBuffers(0, 1, &gpID3D11Buffer_VertexBuffer, &stride, &offset);
																  */

																  // we need to put data in cbuffer, below code does it


																  //********************************** Triangle Color ******************************************


	bufferDesc; // declare buffer ~ vbo. Structure 
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC)); //clear memory or zero memory


	bufferDesc.Usage = D3D11_USAGE_DYNAMIC; //Dynamic draw, GL_STATIC_DRAW or GL_DYNAMIC_DRAW
	bufferDesc.ByteWidth = sizeof(sphere_normals); //sizeof(float) * sizeof(vertices).
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; //This buffer need to bind to vertex buffer.
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; //allowing GPU to write on to this buffer

	hr = gpID3D11Device->CreateBuffer(&bufferDesc, NULL, &gpID3D11Buffer_VertexBuffer_normal); //Create VBO
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Vertext buffer creation failed for normal.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Vertext buffer creation success for normal.\n");
		fclose(gpFile);
	}

	//Map resource (vertex buffer) with device context. Map the vertex data to subresource
	mappedSubresource;
	ZeroMemory(&mappedSubresource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	gpID3D11DeviceContext->Map(gpID3D11Buffer_VertexBuffer_normal, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource); //Map vertex buffer to this resources.
																													   //(vertexBiffer, are we sending multiple data{vertex,color,texture},We want to discrard older data, should we send data now ?(or static or dynamic draw , adress of subrespurce )
	memcpy(mappedSubresource.pData, sphere_normals, sizeof(sphere_normals)); //copy vertertces to mappedSubResource data in pData
	gpID3D11DeviceContext->Unmap(gpID3D11Buffer_VertexBuffer_normal, 0); //Once copied data to subresources then unmap the vertexBuffer from device context

																		 /* If it was static draw then we need to put below call, but as we are in DYNAMIC draw this callis in draw
																		 gpID3D11DeviceContext->IASetVertexBuffers(0, 1, &gpID3D11Buffer_VertexBuffer, &stride, &offset);
																		 */


																		 //***************************** Constant Buffer *********************************************


	bufferDesc; // declare buffer ~ vbo. Structure 
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC)); //clear memory or zero memory


	bufferDesc.Usage = D3D11_USAGE_DYNAMIC; //Dynamic draw, GL_STATIC_DRAW or GL_DYNAMIC_DRAW
	bufferDesc.ByteWidth = sizeof(short) * gNumElements; //sizeof(float) * sizeof(vertices).
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER; //This buffer need to bind to vertex buffer.
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; //allowing GPU to write on to this buffer

	hr = gpID3D11Device->CreateBuffer(&bufferDesc, NULL, &gpID3D11Buffer_IndexBuffer); //Create VBO
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Index buffer creation failed for color.\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Index buffer creation success for color.\n");
		fclose(gpFile);
	}

	//Map resource (vertex buffer) with device context. Map the vertex data to subresource
	mappedSubresource;
	ZeroMemory(&mappedSubresource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	gpID3D11DeviceContext->Map(gpID3D11Buffer_IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource); //Map vertex buffer to this resources.
																											   //(vertexBiffer, are we sending multiple data{vertex,color,texture},We want to discrard older data, should we send data now ?(or static or dynamic draw , adress of subrespurce )
	memcpy(mappedSubresource.pData, sphere_elements, sizeof(short)*gNumElements); //copy vertertces to mappedSubResource data in pData
	gpID3D11DeviceContext->Unmap(gpID3D11Buffer_IndexBuffer, 0); //Once copied data to subresources then unmap the vertexBuffer from device context

																 /* If it was static draw then we need to put below call, but as we are in DYNAMIC draw this callis in draw
																 gpID3D11DeviceContext->IASetVertexBuffers(0, 1, &gpID3D11Buffer_VertexBuffer, &stride, &offset);
																 */


																 //***************************** Constant Buffer *********************************************







	D3D11_BUFFER_DESC bufferDesc_ConstantBuffer; //Create one more buffer to bind data with cbuffer {Constant buffer}

	ZeroMemory(&bufferDesc_ConstantBuffer, sizeof(D3D11_BUFFER_DESC));

	bufferDesc_ConstantBuffer.Usage = D3D11_USAGE_DEFAULT; //STATIC or DYNAMIC or DEFAULT
	bufferDesc_ConstantBuffer.ByteWidth = sizeof(CBUFFER);
	bufferDesc_ConstantBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER; //this buffer need to point to cbuffer in shader
	hr = gpID3D11Device->CreateBuffer(&bufferDesc_ConstantBuffer, nullptr, &gpID3D11Buffer_ConstantBuffer); //create buffer

	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Constant buffer creation failed (cbuffer).\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Constant buffer creation success (cbuffer).\n");
		fclose(gpFile);
	}




	//*********************************************************************************************************************																				   
	// d3d clear color ( blue ). Similar to glClearColor()
	gClearColor[0] = 0.0f; //Red
	gClearColor[1] = 0.0f; //Green
	gClearColor[2] = 0.0f; //Blue
	gClearColor[3] = 1.0f; //Alpha


						   /*There are three more things needs to be done in initilize,
						   CreateRenderTargetView (create target view)
						   OMSetRenderTargets
						   RSSetViewports (Create view port)
						   But, all these three steps again and always need to do when window resize, so instead of adding duplicate code again in initilize, we are calling resize (just to reuse the code)
						   */

						   //Initialize orthograpicProjectionMatrix
	gPerspectiveProjectionMatrix = XMMatrixIdentity();


	hr = resize(WIN_WIDTH, WIN_HEIGHT); // calling resize with fixed/initial width and height
	if (FAILED(hr))
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Resize failed from initialize function\n");
		fclose(gpFile);
		return(hr);
	}
	else
	{
		fopen_s(&gpFile, gszLogFileName, "a+");
		fprintf_s(gpFile, "Resize Succeeded from initialize function\n");
		fclose(gpFile);
	}

	return(S_OK);
}

HRESULT resize(int width, int height)
{
	//result variable use for error checking
	HRESULT hr = S_OK;

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

										/*
										set render target view as render target. OMS - Output Mergerer Stage of pipeline

										The output-merger (OM) stage generates the final rendered pixel color using a combination of pipeline state,
										the pixel data generated by the pixel shaders, the contents of the render targets,
										and the contents of the depth/stencil buffers.
										The OM stage is the final step for determining which pixels are visible (with depth-stencil testing)
										and blending the final pixel colors
										*/
	gpID3D11DeviceContext->OMSetRenderTargets(1, &gpID3D11RenderTargetView, NULL);
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

	//gPerspectiveProjectionMatrix = XMMatrixPerspectiveFovLH(45.0f, (float)width / (float)height, 0.1f, 100.0f);
	gPerspectiveProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

	return(hr);
}

void display(void)
{

	D3D11_VIEWPORT d3dViewPort;
	d3dViewPort.Width = (float)WIN_WIDTH/4;
	d3dViewPort.Height = (float)WIN_HEIGHT/4;
	d3dViewPort.MinDepth = 0.0f;
	d3dViewPort.MaxDepth = 1.0f;


	//Clear the render target view and give default color (blue). It is similar to glClearColor)
	gpID3D11DeviceContext->ClearRenderTargetView(gpID3D11RenderTargetView, gClearColor);

	// select which vertex buffer to display/draw from the vertex buffer
	UINT stride = sizeof(float) * 3; //how many stride for vertex buffer from vertexBuffer data.
	UINT offset = 0; //Offset required 

					 //gpID3D11DeviceContext->IASetVertexBuffers(1, 1, &gpID3D11Buffer_ColowBuffer, &stride, &offset); // for color

					 //(start slot ,how many buffers, adress of strides, offset adress)

					 //************************************************** Triangle **************************************************
	gpID3D11DeviceContext->VSSetConstantBuffers(0, 1, &gpID3D11Buffer_ConstantBuffer); //set constant buffer in device contextS
	gpID3D11DeviceContext->PSSetConstantBuffers(0, 1, &gpID3D11Buffer_ConstantBuffer);

	gpID3D11DeviceContext->IASetVertexBuffers(0, 1, &gpID3D11Buffer_VertexBuffer, &stride, &offset);
	gpID3D11DeviceContext->IASetVertexBuffers(1, 1, &gpID3D11Buffer_VertexBuffer_normal, &stride, &offset); //for color
	gpID3D11DeviceContext->IASetIndexBuffer(gpID3D11Buffer_IndexBuffer, DXGI_FORMAT_R16_UINT, 0);



	gpID3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); //GL_TRIANGLES. no triangle fan
																						  // define world and view matrix

																						  //XMMATRIX translationMatrix = XMMatrixTranslation(-1.5f, 0.0f, -6.0f);
																						  //XMMATRIX rotationMatrix = XMMatrixRotationY(-angleTriangle); // +ve = right to left rotation, -ve = left to right rotation
																						  //XMMATRIX worldMatrix = rotationMatrix * translationMatrix; // ORDER IS VERY ... VERY ... VERY ... IMPORTANT

	XMMATRIX modelMatrix = XMMatrixIdentity();
	XMMATRIX viewMatrix = XMMatrixIdentity();
	XMMATRIX translationaMatrix = XMMatrixTranslation(0.5f, 0.0f, 3.0f);
	modelMatrix = modelMatrix*translationaMatrix;
	//Load the actual data in to constant buffer prepared in initialize
	CBUFFER constantBuffer;

	ZeroMemory(&constantBuffer, sizeof(CBUFFER));


		lightPosition_red[0] = (float)cos(angleRotateRed)*100.0f;
		lightPosition_red[1] = 0.0f;
		lightPosition_red[2] = (float)sin(angleRotateRed)*100.0f;;
		lightPosition_red[3] = 100.0f;

		lightPosition_green[0] = 0.0f;
		lightPosition_green[1] = (float)cos(angleRotateGreen)*100.0f;
		lightPosition_green[2] = (float)sin(angleRotateGreen)*100.0f;
		lightPosition_green[3] = 100.0f;

		lightPosition_blue[0] = -(float)cos(angleRotateBlue)*100.0f;
		lightPosition_blue[1] = 0.0f;
		lightPosition_blue[2] = (float)sin(angleRotateBlue)*100.0f;
		lightPosition_blue[3] = 100.0f;



		constantBuffer.La_uniform = XMVectorSet(lightAmbient[0], lightAmbient[1], lightAmbient[2], lightAmbient[3]);
		constantBuffer.Ls_uniform = XMVectorSet(lightSpecular[0], lightSpecular[1], lightSpecular[2], lightSpecular[3]);
		constantBuffer.Lp_red_uniform = XMVectorSet(lightPosition_red[0], lightPosition_red[1], lightPosition_red[2], lightPosition_red[3]);
		constantBuffer.Ld_red_uniform = XMVectorSet(lightDiffuse_red[0], lightDiffuse_red[1], lightDiffuse_red[2], lightDiffuse_red[3]);
		constantBuffer.Lp_green_uniform = XMVectorSet(lightPosition_green[0], lightPosition_green[1], lightPosition_green[2], lightPosition_green[3]);
		constantBuffer.Ld_green_uniform = XMVectorSet(lightDiffuse_green[0], lightDiffuse_green[1], lightDiffuse_green[2], lightDiffuse_green[3]);
		constantBuffer.Lp_red_uniform = XMVectorSet(lightPosition_blue[0], lightPosition_blue[1], lightPosition_blue[2], lightPosition_blue[3]);
		constantBuffer.Ld_red_uniform = XMVectorSet(lightDiffuse_blue[0], lightDiffuse_blue[1], lightDiffuse_blue[2], lightDiffuse_blue[3]);
		constantBuffer.L_KeyPressed_uniform = 1;

		constantBuffer.model_matrix = modelMatrix; //set data in constant buffer, so that we can put it in shader
		constantBuffer.view_matrix = viewMatrix;
		constantBuffer.projection_matrix = gPerspectiveProjectionMatrix;
//***************************************1st *************************************************************************
		d3dViewPort.TopLeftX = 10;
		d3dViewPort.TopLeftY = 10;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.0215, 0.1745, 0.0215, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.07568, 0.61424, 0.07568, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.633, 0.727811, 0.633, 1.0);
		constantBuffer.shininess = 0.6*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 100;
		d3dViewPort.TopLeftY = 10;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.135, 0.2225, 0.1575, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.54, 0.89, 0.63, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.316228, 0.316228, 0.316228, 1.0);
		constantBuffer.shininess = 0.1*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 200;
		d3dViewPort.TopLeftY = 10;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.05375, 0.05, 0.06625, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.18275, 0.17, 0.22525, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.332741, 0.328634, 0.346435, 1.0);
		constantBuffer.shininess = 0.3*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 300;
		d3dViewPort.TopLeftY = 10;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.25, 0.20725, 0.20725, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(1.0, 0.829, 0.829, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.296648, 0.296648, 0.296648, 1.0);
		constantBuffer.shininess = 0.088*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 400;
		d3dViewPort.TopLeftY = 10;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.1745, 0.01175, 0.01175, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.61424, 0.04136, 0.04136, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.727811, 0.626959, 0.626959, 1.0);
		constantBuffer.shininess = 0.6*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 500;
		d3dViewPort.TopLeftY = 10;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.1, 0.18725, 0.1745, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.396, 0.74151, 0.69102, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.297254, 0.30829, 0.306678, 1.0);
		constantBuffer.shininess = 0.1*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);




//**************************** 2nd row

		d3dViewPort.TopLeftX = 10;
		d3dViewPort.TopLeftY = 100;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.329412, 0.223529, 0.027451, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.780392, 0.568627, 0.113725, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.992157, 0.941176, 0.807843, 1.0);
		constantBuffer.shininess = 0.21794872*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);

		d3dViewPort.TopLeftX = 100;
		d3dViewPort.TopLeftY = 100;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.2125, 0.1275, 0.054, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.714, 0.4284, 0.18144, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.393548, 0.271906, 0.166721, 1.0);
		constantBuffer.shininess = 0.2*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 200;
		d3dViewPort.TopLeftY = 100;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.25, 0.25, 0.25, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.4, 0.4, 0.4, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.774597, 0.774597, 0.774597, 1.0);
		constantBuffer.shininess = 0.6*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 300;
		d3dViewPort.TopLeftY = 100;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.19125, 0.0735, 0.0225, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.7038, 0.27048, 0.0828, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.256777, 0.137622, 0.086014, 1.0);
		constantBuffer.shininess = 0.1*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 400;
		d3dViewPort.TopLeftY = 100;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.24725, 0.1995, 0.0745, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.75164, 0.60648, 0.22648, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.628281, 0.555802, 0.366065, 1.0);
		constantBuffer.shininess = 0.4*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 500;
		d3dViewPort.TopLeftY = 100;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.19225, 0.19225, 0.19225, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.50754, 0.50754, 0.50754, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.508273, 0.508273, 0.508273, 1.0);
		constantBuffer.shininess = 0.4*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);

	//**************************** 3rd row


		d3dViewPort.TopLeftX = 10;
		d3dViewPort.TopLeftY = 200;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.0, 0.0, 0.0, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.01, 0.01, 0.01, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.50, 0.50, 0.50, 1.0);
		constantBuffer.shininess = 0.25*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 100;
		d3dViewPort.TopLeftY = 200;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.0, 0.1, 0.06, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.0, 0.50980392, 0.50980392, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.50196078, 0.50196078, 0.50196078, 1.0);
		constantBuffer.shininess = 0.25*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);





		d3dViewPort.TopLeftX = 200;
		d3dViewPort.TopLeftY = 200;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.0, 0.0, 0.0, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.1, 0.35, 0.1, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.45, 0.55, 0.45, 1.0);
		constantBuffer.shininess = 0.25*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 300;
		d3dViewPort.TopLeftY = 200;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.0, 0.0, 0.0, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.5, 0.0, 0.0, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.7, 0.6, 0.6, 1.0);
		constantBuffer.shininess = 0.25*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 400;
		d3dViewPort.TopLeftY = 200;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.0, 0.0, 0.0, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.55, 0.55, 0.55, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.70, 0.70, 0.70, 1.0);
		constantBuffer.shininess = 0.25*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 500;
		d3dViewPort.TopLeftY = 200;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.0, 0.0, 0.0, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.5, 0.5, 0.0, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.60, 0.60, 0.50, 1.0);
		constantBuffer.shininess = 0.25*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		
//**************************** 4th row
		
		d3dViewPort.TopLeftX = 10;
		d3dViewPort.TopLeftY = 300;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.02, 0.02, 0.02, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.01, 0.01, 0.01, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.4, 0.4, 0.4, 1.0);
		constantBuffer.shininess = 0.078125*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 100;
		d3dViewPort.TopLeftY = 300;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.0, 0.05, 0.05, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.4, 0.5, 0.5, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.04, 0.7, 0.7, 1.0);
		constantBuffer.shininess = 0.078125*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 200;
		d3dViewPort.TopLeftY = 300;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.0, 0.05, 0.0, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.4, 0.5, 0.4, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.04, 0.7, 0.04, 1.0);
		constantBuffer.shininess = 0.078125*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);

		
		d3dViewPort.TopLeftX = 300;
		d3dViewPort.TopLeftY = 300;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.05, 0.0, 0.0, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.5, 0.4, 0.4, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.7, 0.04, 0.04, 1.0);
		constantBuffer.shininess = 0.078125*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 400;
		d3dViewPort.TopLeftY = 300;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.05, 0.05, 0.05, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.5, 0.5, 0.5, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.7, 0.7, 0.7, 1.0);
		constantBuffer.shininess = 0.078125*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


		d3dViewPort.TopLeftX = 500;
		d3dViewPort.TopLeftY = 300;
		gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
		constantBuffer.Ka_uniform = XMVectorSet(0.05, 0.05, 0.0, 1.0);
		constantBuffer.Kd_uniform = XMVectorSet(0.5, 0.5, 0.4, 1.0);
		constantBuffer.Ks_uniform = XMVectorSet(0.7, 0.7, 0.04, 1.0);
		constantBuffer.shininess = 0.078125*128.0;
		gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
		gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);


	// switch between front & back buffer
	gpIDXGISwapChain->Present(0, 0); //similar to wglSwapBuffer, glxSwapBuffer
}

void uninitialize(void)
{
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
		fprintf_s(gpFile, "Un initilize successful\n");
		fprintf_s(gpFile, "Log file closed successfully.\n");
		fclose(gpFile);
	}
}

void updateAngle()
{
	angleRotateRed = angleRotateRed + 0.001f;
	if (angleRotateRed > 360.0f)
		angleRotateRed = 0.0f;


	angleRotateGreen = angleRotateGreen + 0.001f;
	if (angleRotateGreen > 360.0f)
		angleRotateGreen = 0.0f;

	angleRotateBlue = angleRotateBlue + 0.001f;
	if (angleRotateBlue > 360.0f)
		angleRotateBlue = 0.0f;
}