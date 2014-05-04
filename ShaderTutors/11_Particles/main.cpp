//*************************************************************************************************************
#ifdef _DEBUG
#	if _MSC_VER >= 1700
#		pragma comment(lib, "vorbis_vc110d.lib")
#	elif _MSC_VER == 1600
#		pragma comment(lib, "vorbis_vc100d.lib")
#	endif
#else
#	if _MSC_VER >= 1700
#		pragma comment(lib, "vorbis_vc110.lib")
#	elif _MSC_VER == 1600
#		pragma comment(lib, "vorbis_vc100.lib")
#	endif
#endif

#include <d3dx9.h>
#include <iostream>
#include <string>
#include <ctime>

#include "../common/common.h"
#include "../common/particlesystem.h"
#include "../common/audiostreamer.h"
#include "../common/animatedmesh.h"
#include "../common/dxext.h"

#define HELP_TEXT			"If you don't like metal, then it's time to...\n...REEVALUATE YOUR LIFE BITCH!!!!!\n\n(Tip: the dwarfs in the fire\ndidn't like metal either...)"
#define NUM_DWARFS			6

// helper macros
#define TITLE				"Shader tutorial 11: Particles & audio streaming"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

// external variables
extern long		screenwidth;
extern long		screenheight;
extern short	mousedx;
extern short	mousedy;
extern short	mousedown;

extern LPDIRECT3DDEVICE9 device;
extern HWND hwnd;

// tutorial variables
LPD3DXMESH						skymesh			= NULL;
LPD3DXEFFECT					skyeffect		= NULL;
LPD3DXEFFECT					effect			= NULL;
LPD3DXMESH						mesh			= NULL;
LPDIRECT3DCUBETEXTURE9			skytex			= NULL;
LPDIRECT3DTEXTURE9				texture1		= NULL;
LPDIRECT3DTEXTURE9				texture2		= NULL;
LPDIRECT3DTEXTURE9				text			= NULL;
LPDIRECT3DVERTEXDECLARATION9	quaddecl		= NULL;
D3DXMATRIX						dwarfmatrices[NUM_DWARFS];

IXAudio2*						xaudio2			= NULL;
IXAudio2MasteringVoice*			masteringvoice	= NULL;
Sound*							firesound		= NULL;
Sound*							music			= NULL;

AudioStreamer					streamer;
Thread							worker;
ParticleSystem					system1;
AnimatedMesh					dwarfs[NUM_DWARFS];
state<D3DXVECTOR2>				cameraangle;

float textvertices[36] =
{
	9.5f,			9.5f,	0, 1,	0, 0,
	521.5f,			9.5f,	0, 1,	1, 0,
	9.5f,	512.0f + 9.5f,	0, 1,	0, 1,

	9.5f,	512.0f + 9.5f,	0, 1,	0, 1,
	521.5f,			9.5f,	0, 1,	1, 0,
	521.5f,	512.0f + 9.5f,	0, 1,	1, 1
};

static HRESULT InitXAudio2()
{
	HRESULT hr;

	// this is important!!!
	CoInitializeEx(0, COINIT_MULTITHREADED);

	if( FAILED(hr = XAudio2Create(&xaudio2, XAUDIO2_DEBUG_ENGINE)) )
	{
		MYERROR("Could not create XAudio2 object");
		return E_FAIL;
	}

	if( FAILED(hr = xaudio2->CreateMasteringVoice(&masteringvoice)) )
	{
		MYERROR("Could not create mastering voice");
		return E_FAIL;
	}

	return S_OK;
}

static void DwarfSkin(int id, char weapon, char shield, char head, char torso, char legs, char lpads, char rpads)
{
#define ENABLE_IF_OK(var, str) \
	if( var != 1 ) dwarfs[id].EnableFrame(str##"1", false); \
	if( var != 2 ) dwarfs[id].EnableFrame(str##"2", false); \
	if( var != 3 ) dwarfs[id].EnableFrame(str##"3", false);
// end

	ENABLE_IF_OK(weapon, "LOD0_attachment_weapon");
	ENABLE_IF_OK(shield, "LOD0_attachment_shield");
	ENABLE_IF_OK(head, "LOD0_attachment_head");
	ENABLE_IF_OK(torso, "LOD0_attachment_torso");
	ENABLE_IF_OK(legs, "LOD0_attachment_legs");
	ENABLE_IF_OK(lpads, "LOD0_attachment_Lpads");
	ENABLE_IF_OK(rpads, "LOD0_attachment_Rpads");

	dwarfs[id].EnableFrame("Rshoulder", false);
}

HRESULT InitScene()
{
	HRESULT hr;

	SetWindowText(hwnd, TITLE);

	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	MYVALID(device->CreateVertexDeclaration(elem, &quaddecl));

	MYVALID(D3DXLoadMeshFromX("../media/meshes/sky.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skymesh));
	MYVALID(D3DXLoadMeshFromX("../media/meshes/box.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/fire.png", &texture1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/stones.jpg", &texture2));
	MYVALID(D3DXCreateCubeTextureFromFileA(device, "../media/textures/sky4.dds", &skytex));
	MYVALID(device->CreateTexture(512, 512, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &text, NULL));

	MYVALID(DXCreateEffect("../media/shaders/skinning.fx", device, &effect));
	MYVALID(DXCreateEffect("../media/shaders/sky.fx", device, &skyeffect));

	system1.Initialize(device, 500);
	system1.ParticleTexture = texture1;

	// load dwarfs
	dwarfs[0].Effect = effect;
	dwarfs[0].Method = SM_Shader;
	dwarfs[0].Path = "../media/meshes/dwarf/";
	
	MYVALID(dwarfs[0].Load(device, "../media/meshes/dwarf/dwarf.X"));

	dwarfs[0].Clone(dwarfs[1]);
	dwarfs[0].Clone(dwarfs[2]);
	dwarfs[0].Clone(dwarfs[3]);
	dwarfs[0].Clone(dwarfs[4]);
	dwarfs[0].Clone(dwarfs[5]);

	D3DXVECTOR3 scale(0.1f, 0.1f, 0.1f);
	D3DXVECTOR3 trans;
	D3DXQUATERNION rot;

	// dwarf 0
	trans = D3DXVECTOR3(-1, 0, 1);

	D3DXQuaternionRotationYawPitchRoll(&rot, -0.785f, 0, 0);
	D3DXMatrixTransformation(&dwarfmatrices[0], NULL, NULL, &scale, NULL, &rot, &trans);

	// dwarf 1
	trans = D3DXVECTOR3(-1, 0, -1);

	D3DXQuaternionRotationYawPitchRoll(&rot, -2.356f, 0, 0);
	D3DXMatrixTransformation(&dwarfmatrices[1], NULL, NULL, &scale, NULL, &rot, &trans);

	// dwarf 2
	trans = D3DXVECTOR3(1, 0, -1);

	D3DXQuaternionRotationYawPitchRoll(&rot, 2.356f, 0, 0);
	D3DXMatrixTransformation(&dwarfmatrices[2], NULL, NULL, &scale, NULL, &rot, &trans);

	// dwarf 3
	trans = D3DXVECTOR3(1, 0, 1);

	D3DXQuaternionRotationYawPitchRoll(&rot, 0.785f, 0, 0);
	D3DXMatrixTransformation(&dwarfmatrices[3], NULL, NULL, &scale, NULL, &rot, &trans);

	// dwarf 4
	trans = D3DXVECTOR3(-0.2f, 0, -0.2f);

	D3DXQuaternionRotationYawPitchRoll(&rot, -1.57f, 0, 0);
	D3DXMatrixTransformation(&dwarfmatrices[4], NULL, NULL, &scale, NULL, &rot, &trans);

	// dwarf 5
	trans = D3DXVECTOR3(0.2f, 0, 0);

	D3DXQuaternionRotationYawPitchRoll(&rot, -1.57f, 0, 0);
	D3DXMatrixTransformation(&dwarfmatrices[5], NULL, NULL, &scale, NULL, &rot, &trans);

	// skins
	DwarfSkin(0, 1, 1, 3, 1, 1, 3, 0);
	DwarfSkin(1, 2, 0, 1, 3, 3, 1, 1);
	DwarfSkin(2, 3, 2, 2, 2, 1, 2, 3);
	DwarfSkin(3, 1, 3, 2, 1, 3, 0, 2);
	DwarfSkin(4, 1, 1, 3, 3, 2, 2, 1);
	DwarfSkin(5, 0, 0, 1, 2, 3, 0, 1);

	// 0, 1, 2 - dead
	// 3, 5 - stand
	// 4 - jump
	// 6 - cheer with weapon
	// 7 - cheer with one hand
	// 8 - cheer with both hands
	dwarfs[0].SetAnimation(6);
	dwarfs[1].SetAnimation(8);
	dwarfs[2].SetAnimation(7);
	dwarfs[3].SetAnimation(4);
	dwarfs[4].SetAnimation(2);
	dwarfs[5].SetAnimation(1);

	// other
	device->SetRenderState(D3DRS_LIGHTING, false);
	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

	DXRenderText(HELP_TEXT, text, 512, 512);
	cameraangle = D3DXVECTOR2(0.785f, 0.785f);

	// sound
	if( SUCCEEDED(InitXAudio2()) )
	{
		firesound = streamer.LoadSound(xaudio2, "../media/sound/fire.ogg");

		// create streaming thread and load music
		worker.Attach<AudioStreamer>(&streamer, &AudioStreamer::Update);
		worker.Start();

		music = streamer.LoadSoundStream(xaudio2, "../media/sound/painkiller.ogg");
	}

	if( music )
	{
		music->GetVoice()->SetVolume(4);
		music->Play();
	}

	if( firesound )
	{
		firesound->GetVoice()->SetVolume(0.7f);
		firesound->Play();
	}

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	streamer.Destroy();
	WaitForSingleObject(worker.GetHandle(), 3000);

	worker.Close();

	if( masteringvoice )
		masteringvoice->DestroyVoice();

	if( xaudio2 )
		xaudio2->Release();

	// don't do this, ever!!!
	system1.~ParticleSystem();

	for( int i = 0; i < NUM_DWARFS; ++i )
		dwarfs[i].~AnimatedMesh();

	SAFE_RELEASE(skyeffect);
	SAFE_RELEASE(skymesh);
	SAFE_RELEASE(skytex);
	SAFE_RELEASE(text);
	SAFE_RELEASE(quaddecl);
	SAFE_RELEASE(mesh);
	SAFE_RELEASE(effect);
	SAFE_RELEASE(texture1);
	SAFE_RELEASE(texture2);

	DXKillAnyRogueObject();
	//CoUninitialize();
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
}
//*************************************************************************************************************
void Update(float delta)
{
	D3DXVECTOR2 velocity(mousedx, mousedy);

	cameraangle.prev = cameraangle.curr;

	if( mousedown == 1 )
		cameraangle.curr += velocity * 0.004f;

	// clamp to [-pi, pi]
	if( cameraangle.curr.y >= 1.5f )
		cameraangle.curr.y = 1.5f;

	if( cameraangle.curr.y <= -1.5f )
		cameraangle.curr.y = -1.5f;

	system1.Update();
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	D3DXMATRIX		world, view, proj;
	D3DXMATRIX		skyworld, viewproj;
	D3DXVECTOR3		eye(0, 0, -5);
	D3DXVECTOR3		look(0, 0, 0);
	D3DXVECTOR3		up(0, 1, 0);
	D3DXVECTOR2		orient = cameraangle.smooth(alpha);

	D3DXMatrixRotationYawPitchRoll(&view, orient.x, orient.y, 0);
	D3DXVec3TransformCoord(&eye, &eye, &view);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)screenwidth / (float)screenheight, 0.1f, 50);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixMultiply(&viewproj, &view, &proj);

	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);
	device->SetTransform(D3DTS_VIEW, &view);
	device->SetTransform(D3DTS_PROJECTION, &proj);

	for( int i = 0; i < NUM_DWARFS; ++i )
		dwarfs[i].Update(elapsedtime, &dwarfmatrices[i]);

	effect->SetMatrix("matViewProj", &viewproj);

	if( SUCCEEDED(device->BeginScene()) )
	{
		// render sky
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

		D3DXMatrixScaling(&skyworld, 20, 20, 20);
		skyeffect->SetMatrix("matWorld", &skyworld);

		D3DXMatrixIdentity(&skyworld);
		skyeffect->SetMatrix("matWorldSky", &skyworld);

		skyeffect->SetMatrix("matViewProj", &viewproj);
		skyeffect->SetVector("eyePos", (D3DXVECTOR4*)&eye);

		skyeffect->Begin(0, 0);
		skyeffect->BeginPass(0);
		{
			device->SetTexture(0, skytex);
			skymesh->DrawSubset(0);
		}
		skyeffect->EndPass();
		skyeffect->End();

		device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

		// render ground
		D3DXMatrixScaling(&world, 5, 0.1f, 5);
		world._42 = -0.05f;

		device->SetTransform(D3DTS_WORLD, &world);
		device->SetTexture(0, texture2);
		mesh->DrawSubset(0);

		// dwarfs
		for( int i = 0; i < NUM_DWARFS; ++i )
			dwarfs[i].Draw();

		// fire
		D3DXMatrixTranslation(&world, 0, 0.25f, 0);
		device->SetTransform(D3DTS_WORLD, &world);

		system1.Draw(world, view);

		// render text
		device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		device->SetTexture(0, text);
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, textvertices, 6 * sizeof(float));

		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		device->SetRenderState(D3DRS_ZENABLE, TRUE);

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
