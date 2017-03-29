#include "Game.h"
#include "Vertex.h"

#include "WICTextureLoader.h"  // for loading textures

// for test
#include <iostream>
using namespace std;

// For the DirectX Math library
using namespace DirectX;


// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore( 
		hInstance,		   // The application's handle
		"DirectX Game",	   // Text for the window's title bar
		1280,			   // Width of the window's client area
		720,			   // Height of the window's client area
		true)			   // Show extra stats (fps) in title bar?
{
	// Initialize fields
	vertexShader = 0;
	pixelShader = 0;
	//sound = 0;

#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.");
#endif
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	// Release any (and all!) DirectX objects
	// we've made in the Game class

	// Delete our simple shader objects, which
	// will clean up their own internal DirectX stuff
	
	delete m1;
	delete m2;
	delete m3;
	delete conemesh;
	delete cube;
	delete cylinder;
	delete helix;
	delete sphere;
	delete torus;
	delete plane;
	for (int i = 0; i < 12; i++)
		delete E[i];
	//delete Entity_obj;
	delete c;
	delete ma_metal;
	delete ma_concrete;
	delete vertexShader;
	delete pixelShader;
	SRV_Concrete->Release();
	SRV_Metal->Release();
	SampleState->Release();
	
	// release the sound object
	/*if (sound)
	{
		sound->Shutdown();
		delete sound;
	}*/

	//clean up the shadow mapping stuffs
	shadowMapTexture->Release();
	DSV_Shadow->Release();
	SRV_Shadow->Release();
	RS_Shadow->Release();


}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later



	LoadShaders();
	CreateCamera();
	/*CreateSound(hWnd);*/
	// ----load texture----

	CreateWICTextureFromFile(device, context, L"Assets/Textures/Tile.jpg",0, &SRV_Metal);
	CreateWICTextureFromFile(device, context, L"Assets/Textures/DamageConcrete.jpg", 0, &SRV_Concrete);
	// --------------------

	// ----sampler code----

	// First, create a description
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;  // how to handle addresses outside the 0-1 UV range
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;  // D3D11_TEXTURE_ADDRESS_WRAP is a usual value ( wrapping textures)
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // how to handle sampling "between" pixels ;
														//D3D11_FILTER_MIN_MAG_MIP_LINEAR is usual (trilinear filtering)
	sampDesc.MaxAnisotropy = 16;

	// Now, create the sampler from the description
	device->CreateSamplerState(&sampDesc, &SampleState);
	// -------------------------

	// shadow mapping stuff

	// Set up the description of the shadow map texture
	D3D11_TEXTURE2D_DESC shadowMapTexDesc = {};
	shadowMapTexDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowMapTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	// Set up the depth stencil view description --> for using this texture "as a depth buffer"
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;

	// Set up the shader resource view description --> for sampling from the texture in a pixel shader
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

	// using the description above to create the texture, depth stencil view and the shader resource view
	device->CreateTexture2D(&shadowMapTexDesc, NULL, &shadowMapTexture);
	device->CreateDepthStencilView(shadowMapTexture, &descDSV, &DSV_Shadow);
	device->CreateShaderResourceView(shadowMapTexture, &srvDesc, &SRV_Shadow);

	// finishing the shadow mapping depth buffer thing??




	CreateMaterial();
	CreateMeshes();
	CreateEntities(); // the third assignment
	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// shadow mappign stuff-->setting render states
	// set null render target and clear the depth buffer
	context->OMSetRenderTargets(0, 0, DSV_Shadow);  
	context->ClearDepthStencilView(DSV_Shadow, D3D11_CLEAR_DEPTH, 1.0f, 0); // clear the depth buffer
	// Set up the rasterize state
	D3D11_RASTERIZER_DESC rsDesc = {};
	device->CreateRasterizerState(&rsDesc, &RS_Shadow);
	// Set a viewport
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = 1024; // width/height should be equal to shadow map size
	viewport.Height = 1024;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	context->RSSetViewports(1, &viewport);


	/*
	directionalLight.AmbientColor = XMFLOAT4(0.1, 0.1, 0.1, 0.1);
	directionalLight.DiffuseColor = XMFLOAT4(0, 0, 1, 1);  // blue
	directionalLight.Direction = XMFLOAT3(1, -1, 0);
	*/

	directionalLight2.AmbientColor = XMFLOAT4(0.1, 0.1, 0.1, 0.1);
	directionalLight2.DiffuseColor = XMFLOAT4(1, 1, 0, 1);  // yellow
	directionalLight2.Direction = XMFLOAT3(0, -1, 0);

	/*
	pointLight.Color = XMFLOAT4(1,0.1f,0.1f,1);  
	pointLight.Position = XMFLOAT3(0,2, 0);
	XMFLOAT3 cpos;
	XMStoreFloat3(&cpos, c->GetCameraPosition());
	pointLight.CameraPos = cpos;
	// for test: cout << pointLight.CameraPos.x << pointLight.CameraPos.y << pointLight.CameraPos.z << endl;
	*/
	
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files using
// my SimpleShader wrapper for DirectX shader manipulation.
// - SimpleShader provides helpful methods for sending
//   data to individual variables on the GPU
// --------------------------------------------------------
void Game::LoadShaders()
{
	vertexShader = new SimpleVertexShader(device, context);
	if (!vertexShader->LoadShaderFile(L"Debug/VertexShader.cso"))
		vertexShader->LoadShaderFile(L"VertexShader.cso");		

	pixelShader = new SimplePixelShader(device, context);
	if(!pixelShader->LoadShaderFile(L"Debug/PixelShader.cso"))	
		pixelShader->LoadShaderFile(L"PixelShader.cso");

	// You'll notice that the code above attempts to load each
	// compiled shader file (.cso) from two different relative paths.

	// This is because the "working directory" (where relative paths begin)
	// will be different during the following two scenarios:
	//  - Debugging in VS: The "Project Directory" (where your .cpp files are) 
	//  - Run .exe directly: The "Output Directory" (where the .exe & .cso files are)

	// Checking both paths is the easiest way to ensure both 
	// scenarios work correctly, although others exist

	// shadow mapping stuff
	VS_Shadow = new SimpleVertexShader(device, context);
	if (!vertexShader->LoadShaderFile(L"Debug/VS_Shadow.cso"))
		vertexShader->LoadShaderFile(L"VS_Shadow.cso");

	// ??? PS_Shadow = new SimplePixelShader(device, context);  // no pixel shader??
}

void Game::CreateCamera()
{
	c = new Camera();
	c->UpdateProjMatrix(width, height);
}

void Game::CreateMaterial()
{
	// pass in the texture and sampler state you have made above before drawing it
	ma_metal = new Material(pixelShader,vertexShader, SRV_Metal, SampleState);
	ma_concrete = new Material(pixelShader, vertexShader, SRV_Concrete, SampleState);

}


// --------------------------------------------------------
// Initializes the matrices necessary to represent our geometry's 
// transformations and our 3D camera
// --------------------------------------------------------
//void Game::CreateMatrices()
//{
	// Set up world matrix
	// - In an actual game, each object will need one of these and they should
	//   update when/if the object moves (every frame)
	// - You'll notice a "transpose" happening below, which is redundant for
	//   an identity matrix.  This is just to show that HLSL expects a different
	//   matrix (column major vs row major) than the DirectX Math library
	//XMMATRIX W = XMMatrixIdentity();
	//XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(W)); // Transpose for HLSL!
	// Create the View matrix
	// - In an actual game, recreate this matrix every time the camera 
	//    moves (potentially every frame)
	// - We're using the LOOK TO function, which takes the position of the
	//    camera and the direction vector along which to look (as well as "up")
	// - Another option is the LOOK AT function, to look towards a specific
	//    point in 3D space
	//XMVECTOR pos = XMVectorSet(0, 0, -5, 0);
	//XMVECTOR dir = XMVectorSet(0, 0, 1, 0);
	//XMVECTOR up  = XMVectorSet(0, 1, 0, 0);
	//XMMATRIX V   = XMMatrixLookToLH(
	//	pos,     // The position of the "camera"
	//	dir,     // Direction the camera is looking
	//	up);     // "Up" direction in 3D space (prevents roll)
	//XMStoreFloat4x4(&viewMatrix, XMMatrixTranspose(V)); // Transpose for HLSL!
	// Create the Projection matrix
	// - This should match the window's aspect ratio, and also update anytime
	//   the window resizes (which is already happening in OnResize() below)
	//XMMATRIX P = XMMatrixPerspectiveFovLH(
	//	0.25f * 3.1415926535f,		// Field of View Angle
	//	(float)width / height,		// Aspect ratio
	//	0.1f,						// Near clip plane distance
	//	100.0f);					// Far clip plane distance
	//XMStoreFloat4x4(&projectionMatrix, XMMatrixTranspose(P)); // Transpose for HLSL!
//}



// Creates three Mesh objects, with different geometry, in the Game class
void Game::CreateMeshes()
{
	// create the first mesh
	Vertex v1[]=
{ 
	{ XMFLOAT3(+0.0f, +1.0f, +0.0f), XMFLOAT2(+0.0f,+0.0f), XMFLOAT3(+0.0f,+0.0f,-1.0f) },
	{ XMFLOAT3(+1.5f, -1.0f, +0.0f),  XMFLOAT2(+0.0f,+0.0f), XMFLOAT3(+0.0f,+0.0f,-1.0f) },
	{ XMFLOAT3(-1.5f, -1.0f, +0.0f),  XMFLOAT2(+0.0f,+0.0f), XMFLOAT3(+0.0f,+0.0f,-1.0f) },
};
	unsigned int i1[] = { 0, 1, 2 };
	Vertex * x1 = v1;
	unsigned int * y1 = i1;
	m1 = new Mesh(x1, 3, y1, 3, device);

	// create the second mesh
	Vertex v2[] =
	{
		{ XMFLOAT3(+1.0f, +1.0f, +0.0f),  XMFLOAT2(+0.0f,+0.0f),  XMFLOAT3(+0.0f,+0.0f,-1.0f) },
		{ XMFLOAT3(+2.0f, +1.0f, +0.0f),  XMFLOAT2(+0.0f,+0.0f),  XMFLOAT3(+0.0f,+0.0f,-1.0f) },
		{ XMFLOAT3(+1.0f, +2.0f, +0.0f),  XMFLOAT2(+0.0f,+0.0f),  XMFLOAT3(+0.0f,+0.0f,-1.0f) },
	};
	Vertex * x2 = v2;
	unsigned int i2[] = { 2, 1, 0 };
	unsigned int * y2 = i2;
	m2 = new Mesh(x2, 3, y2, 3, device);

	// create the third mesh
	Vertex v3[] =
	{
		{ XMFLOAT3(-1.0f, +1.0f, +0.0f),  XMFLOAT2(+0.0f,+0.0f),  XMFLOAT3(+0.0f,+0.0f,-1.0f) },
		{ XMFLOAT3(-2.0f, +1.0f, +0.0f),  XMFLOAT2(+0.0f,+0.0f),  XMFLOAT3(+0.0f,+0.0f,-1.0f) },
		{ XMFLOAT3(-1.0f, +2.0f, +0.0f),  XMFLOAT2(+0.0f,+0.0f),  XMFLOAT3(+0.0f,+0.0f,-1.0f) },
	};
	Vertex * x3 = v3;	
	unsigned int i3[] = { 0, 1, 2 };
	unsigned int * y3 = i3;
	m3 = new Mesh(x3, 3, y3, 3, device);

	// create new meshes using third mesh constructor
	conemesh = new Mesh("Assets/Models/cone.obj", device);
	cube = new Mesh("Assets/Models/cube.obj", device);
	cylinder = new Mesh("Assets/Models/cylinder.obj", device);
	helix = new Mesh("Assets/Models/helix.obj", device);
	sphere = new Mesh("Assets/Models/sphere.obj", device);
	torus = new Mesh("Assets/Models/torus.obj", device);
	plane = new Mesh("Assets/Models/plane.obj", device);

}

void Game::CreateEntities()
{

	E[0] = new Entity(m1,ma_concrete);
	E[1] = new Entity(m1,ma_metal);
	E[2] = new Entity(m2,ma_metal);
	E[3] = new Entity(m2,ma_metal);
	E[4] = new Entity(m3,ma_metal);
	E[5] = new Entity(conemesh, ma_metal);
	E[6] = new Entity(cube, ma_concrete);
	E[7] = new Entity(cylinder, ma_concrete);
	E[8] = new Entity(helix, ma_metal);
	E[9] = new Entity(sphere, ma_concrete);
	E[10] = new Entity(torus, ma_metal);
	E[11] = new Entity(plane, ma_metal);

}

// create & initialize the sound object
//void Game::CreateSound(HWND hwnd)
//{
//	sound = new Sound;
//	sound->Initialize(hwnd);
//	cout << "sound initialize successfully" << endl;
//}


// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	// Update our projection matrix since the window size changed
	c->UpdateProjMatrix(width, height);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Quit if the escape key is pressed
	if (GetAsyncKeyState(VK_ESCAPE))
		Quit();

	// when getting key input calling camera updating function
	c->Update(deltaTime);

	// ---- assignment 3-----

	float sinTime = (sin(totalTime * 2) + 2.0f) / 10.0f;
	float cosTime = (cos(totalTime * 2)) / 10.0f;

	E[0]->SetRot(XMFLOAT3(totalTime, 0, 0));
	E[0]->SetTrans(XMFLOAT3(-1, 0, 0)); // rotate at x axis

	E[1]->SetTrans(XMFLOAT3(totalTime, 0, 0));  // move off the screen
	E[1]->SetScale (XMFLOAT3(sinTime, sinTime, sinTime)); // scale in a sin wave

	E[2]->SetTrans(XMFLOAT3(3, -3, 0));
	E[2]->SetScale(XMFLOAT3(cosTime, cosTime, cosTime)); // scale in a cos wave

	E[3]->SetTrans(XMFLOAT3(0, 0, -1));
	E[3]->SetRot(XMFLOAT3(0, totalTime, 0)); // rotate at y axis

	E[4]->SetTrans(XMFLOAT3(0, 0, 0));
	E[4]->SetRot(XMFLOAT3(0, 0, totalTime)); // rotate at z axis according
	// ---assignment 3 ------
	

	E[5]->SetTrans(XMFLOAT3(1, 1, 0));
	E[6]->SetRot(XMFLOAT3(0, totalTime, 0));
	E[7]->SetTrans(XMFLOAT3(2, 0, 0));

	E[8]->SetTrans(XMFLOAT3(-2, 1, -2));
	E[8]->SetRot(XMFLOAT3(1, 1, 0));


	E[9]->SetTrans(XMFLOAT3(-2, 0, 0));

	E[10]->SetTrans(XMFLOAT3(0, -1, 0));

	E[11]->SetTrans(XMFLOAT3(0, -2, 0));

}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Background color (Cornflower Blue in this case) for clearing
	const float color[4] = {0.4f, 0.6f, 0.75f, 0.0f};

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV, color);
	context->ClearDepthStencilView(
		depthStencilView, 
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	// Send data to shader variables
	//  - Do this ONCE PER OBJECT you're drawing
	//  - This is actually a complex process of copying data to a local buffer
	//    and then copying that entire buffer to the GPU.  
	//  - The "SimpleShader" class handles all of that for you.

	// load material once
	


	// ---Third Assignment---
	// draw the entity
	for (int i = 0; i<12; i++)
	{

		ID3D11Buffer *  vb = E[i]->GetMesh()->GetVertexBuffer();
		ID3D11Buffer *  ib = E[i]->GetMesh()->GetIndexBuffer();

		UINT stride = sizeof(Vertex);
		UINT offset = 0;

		context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
		context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);

		E[i]->PrepareMaterial(c->GetViewMatrix(), c->GetProjectionMatrix()); // draw each entity
		/*
		pixelShader->SetData(
			"directionalLight", // the name of the (eventual) variable in the shader
			&directionalLight,  // the address of the data to copy
			sizeof(DirectionalLight)  // the size of the data to copy
		);
		*/
		pixelShader->SetData(
			"directionalLight2",
			&directionalLight2,
			sizeof(DirectionalLight)
		);
		/*
		XMFLOAT3 camerapos;
		XMStoreFloat3(&camerapos, c->GetCameraPosition());
		//pixelShader->SetFloat3("cameraPosition", camerapos);

		pixelShader->SetFloat3("PointLightPosition", XMFLOAT3(0, 2, 0));
		pixelShader->SetFloat4("PointLightColor", XMFLOAT4(1, 0.1f, 0.1f, 1));  // red
		pixelShader->SetFloat3("CameraPosition", camerapos);
		*/
		/*
		pixelShader->SetData(
			"pointLight", 
			&pointLight,
			sizeof(PointLight));
		*/
		//XMFLOAT3 camerapos;
		//XMStoreFloat3(&camerapos, c->GetCameraPosition());
		//pixelShader->SetFloat3("cameraPosition", camerapos);

		// for shadow mapping stuffs

		// trying to pass the light's view matrix
		XMVECTOR lightdirection = XMLoadFloat3(&directionalLight2.Direction);
		
		XMMATRIX lightview = 
			XMMatrixLookToLH(XMVectorSet(0, 0, 0, 0),	// light's position /center of your world
			lightdirection,								// light's direction
			XMVectorSet(0, 1, 0, 0));					// Updirection

		XMFLOAT4X4 LightView;
		XMStoreFloat4x4(&LightView, lightview);

		// trying to pass the light's projection matrix
		XMFLOAT4X4 LightProjection;
		XMMATRIX lightprojection = XMMatrixOrthographicLH(100,100,0,100);
		XMStoreFloat4x4(&LightProjection, lightprojection);
		
		VS_Shadow->SetMatrix4x4("world", E[i]->GetMatrix());
		VS_Shadow->SetMatrix4x4("view", LightView);
		VS_Shadow->SetMatrix4x4("projection", LightProjection);

	


		context->DrawIndexed(
			E[i]->GetMesh()->GetIndexCount(),     // The number of indices to use (we could draw a subset if we wanted)
			0,     // Offset to the first index we want to use
			0);    // Offset to add to each index when looking up vertices
	}
	// ---Third Assignment---



	// Present the back buffer to the user
	//  - Puts the final frame we're drawing into the window so the user can see it
	//  - Do this exactly ONCE PER FRAME (always at the very end of the frame)
	swapChain->Present(0, 0);
}


#pragma region Mouse Input

// --------------------------------------------------------
// Helper method for mouse clicking.  We get this information
// from the OS-level messages anyway, so these helpers have
// been created to provide basic mouse input if you want it.
// --------------------------------------------------------
void Game::OnMouseDown(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...

	// Save the previous mouse position, so we have it for the future
	prevMousePos.x = x;
	prevMousePos.y = y;

	// Caputure the mouse so we keep getting mouse move
	// events even if the mouse leaves the window.  we'll be
	// releasing the capture once a mouse button is released
	SetCapture(hWnd);
}

// --------------------------------------------------------
// Helper method for mouse release
// --------------------------------------------------------
void Game::OnMouseUp(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...

	// We don't care about the tracking the cursor outside
	// the window anymore (we're not dragging if the mouse is up)
	ReleaseCapture();
}

// --------------------------------------------------------
// Helper method for mouse movement.  We only get this message
// if the mouse is currently over the window, or if we're 
// currently capturing the mouse.
// --------------------------------------------------------
void Game::OnMouseMove(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...
	// calling camera movement methods here

	if (buttonState & 0x0001)
	{
		float xDiff = (x - prevMousePos.x) * 0.005f;
		float yDiff = (y - prevMousePos.y) * 0.005f;
		c->Rotate(yDiff, xDiff);
	}



	// Save the previous mouse position, so we have it for the future
	prevMousePos.x = x;
	prevMousePos.y = y;
}

// --------------------------------------------------------
// Helper method for mouse wheel scrolling.  
// WheelDelta may be positive or negative, depending 
// on the direction of the scroll
// --------------------------------------------------------
void Game::OnMouseWheel(float wheelDelta, int x, int y)
{
	// Add any custom code here...
}
#pragma endregion