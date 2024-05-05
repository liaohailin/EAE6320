// Header Files
//=============

#include "precompiled.h"
#include "Graphics.h"
#include "Time.h"
#include "UserInput.h"
#include "WindowsProgram.h"
#include "Mesh.h"
#include "Material.h"
#include "DebugMenu.h"
#include "Player.h"
#include "../Tools/Network/RakPeerInterface.h"
#include "../Tools/Network/MessageIdentifiers.h"
#include "../Tools/Network/BitStream.h"
#include "../Tools/Network/RakNetTypes.h"  // MessageID

#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <stdio.h>

#ifdef _DEBUG
#define EAE6320_GRAPHICS_SHOULDD3DDEBUGINFOBEENABLED
#endif

#ifdef EAE6320_GRAPHICS_SHOULDD3DDEBUGINFOBEENABLED
#define D3D_DEBUG_INFO
#endif


#include <d3d9.h>
#include <d3dx9shader.h>
#include <XAudio2.h>
#include <XAudio2fx.h>
#include <X3DAudio.h>

#include <string>
#include "../External/Lua/5.2.3/src/lua.hpp"

#define MAX_CLIENTS 10
#define SERVER_PORT 60000
#define INPUTCHANNELS 1
#define OUTPUTCHANNELS 8
// Static Data Initialization
//===========================

namespace
{
	HWND s_mainWindow= NULL;
	IDirect3D9* s_direct3dInterface = NULL;
	IDirect3DDevice9* s_direct3dDevice = NULL;

	// The vertex information that is sent to the display adaptor must match what the vertex shader expects
	struct sVertex
	{
		float x, y, z;//position
		float nx, ny, nz;//normal
		float r,g,b;//color
		float u, v;//coordinate
	};
	D3DVERTEXELEMENT9 s_vertexElements[] = 
	{ 
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		{ 0, 36, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT ,D3DDECLUSAGE_TEXCOORD,0},
		D3DDECL_END()
	};

	IDirect3DVertexDeclaration9* s_vertexDeclaration = NULL;


	//vertex shader constant 
	D3DXHANDLE matrixWorldToViewHandler = NULL;
	D3DXHANDLE matrixViewToScreenHandler = NULL;
	D3DXHANDLE matrixModelToWorldHandler = NULL;

	//fragment shader constant value
	//light
	D3DXHANDLE constantHandlerColor;
	D3DXHANDLE constantHandlerAmbientLight;
	D3DXHANDLE constantHandlerDirectionLight;
	D3DXHANDLE constantHandlerDirectionLightColor;
	//texture
	D3DXHANDLE samplerHandle = NULL;
	IDirect3DTexture9* planeTexture;
	DWORD samplerRegister;
	DWORD samplerRegister2;

	//shader constant set value
	D3DXVECTOR4 ambientLight(0.2f, 0.2f, 0.2f, 1.0f);
	D3DXVECTOR4 directionLight(0, -1, 0, 1);
	D3DXVECTOR4 directionLightColor(1.0, 1.0, 1.0, 1.0);
	//box position
	D3DXVECTOR3 mapModelPos(0.0f, 0.0f, 0.0f);

	//camera info
	D3DXMATRIX cameraRotation;
	D3DXVECTOR3 cameraPosition(0.0f, 0.0f, -10.0f);
	float cameraSpeed = 10;
	D3DXVECTOR3 cameraVelocity(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 cameraRayCastDir(0, 0, 0);
	D3DXVECTOR3 posBefore(0, 0, 0);
	float playerRotValue = 0;

	//perspective view info
	float nearPlane = 10.0f;
	float farPlane = 10000.0f;
	float fieldOfView = D3DXToRadian(60.0f);

	//mesh binary file address
	const char* braceMatPath = "data/braceMat.mat";
	const char* cementWallMatPath = "data/cementWallMat.mat";
	const char* railingMatPath = "data/railingMat.mat";
	const char* wallMatPath = "data/wallMat.mat";
	const char* floorMatPath = "data/floorMat.mat";
	const char* otherObjectMatPath = "data/otherObjectMat.mat";
	const char* playerMatPath = "data/catMat.mat";
	const char* collisionMatPath = "data/collisionMat.mat";
	const char* redMatPath = "data/redMat.mat";
	const char* blueMatPath = "data/blueMat.mat";
	const char* dogMatPath = "data/dogMat.mat";

	const char* braceMeshPath = "data/braceMesh.mesh";
	const char* cementWallMeshPath = "data/cementWallMesh.mesh";
	const char* wallMeshPath = "data/wallMesh.mesh";
	const char* railingMeshPath = "data/railingMesh.mesh";
	const char* floorMeshPath = "data/floorMesh.mesh";
	const char* otherObjectMeshPath = "data/otherObjectMesh.mesh";
	const char* playerMeshPath = "data/catMesh.mesh";
	const char* flagMeshPath = "data/flagMesh.mesh";
	const char* dogMeshPath = "data/dogMesh.mesh";
	const char* collisionMeshPath = "data/sceneCollisionInfo.mesh";
	

	const unsigned int sceneObjNum = 11;
	const unsigned int materialNum = 11;
	Mesh::cMesh* pCTFMeshArray[sceneObjNum];
	Material::cMaterial* pCTFMaterialArray[materialNum];
	//player
	Player::cPlayer* pPlayer;
	//arcball
	ArcBall::cArcBall* arcBall;
	float bindPlayer = -1.0f;
	bool initBindPos = false;
	
	//flag capture
	bool getRedFlag = false;
	bool getBlueFlag = false;
	DebugMenu::cDebugMenu* pFlagCapText;
	float sprintValue = 10.0f;
	int catScore = 0;
	int dogScore = 0;
	//octree collision
	const char* octreeCollisionDataPath = "data/octreeCollisionInfo.col";
	Player::OctreeNode* pSceneOctree;
	char* pBinaryFileEntry;//pointer entry for the binary file
	unsigned int binaryFileIndex=0;//shift for the binary file
	unsigned int fileLength;
	std::vector<float> octreeCollisionTriangleVertices;
	std::vector<unsigned int> octreeCollisionTriangleIndices;
	std::vector<unsigned int>octreeCollideTriangleList;

	//networking
	bool isServer = false;
	enum GameMessages
	{
		ID_KEEP_PASSING_DATA = ID_USER_PACKET_ENUM + 1
	};
	RakNet::RakPeerInterface *peer;
	RakNet::Packet *packet;

	//audio
	IXAudio2* pXAudio2 = NULL;
	IXAudio2MasteringVoice* pMasterVoice = NULL;
	IXAudio2Voice* pVoice = NULL;
	IXAudio2SourceVoice* pSoundEffectSource=NULL;
	IXAudio2SourceVoice* pBackgroundSource = NULL;
	IXAudio2SourceVoice* pWalkSource = NULL; 
	//3daudio
	X3DAUDIO_HANDLE X3DInstance;
	X3DAUDIO_LISTENER Listener = { 0 };
	X3DAUDIO_EMITTER Emitter = { 0 };
	X3DAUDIO_DSP_SETTINGS DSPSettings = { 0 };
	XAUDIO2_DEVICE_DETAILS deviceDetails;
	IXAudio2SubmixVoice* pSubmixVoice = NULL;
	IUnknown* pReverbEffect;

	X3DAUDIO_CONE Listener_DirectionalCone = { X3DAUDIO_PI*5.0f / 6.0f, X3DAUDIO_PI*11.0f / 6.0f, 1.0f, 0.75f, 0.0f, 0.25f, 1.0f, 1.5f };//->InnerAngle/InnerLPF/InnerReverb/InnerVolume/OuterAngle/OuterLPF/OuterReverb/OuterVolume
	X3DAUDIO_CONE Emitter_DirectionalCone = { X3DAUDIO_PI*5.0f / 6.0f, X3DAUDIO_PI*11.0f / 6.0f, 1.0f, 0.75f, 0.0f, 0.25f, 1.0f, 1.5f };

	X3DAUDIO_DISTANCE_CURVE_POINT Emitter_LFE_CurvePoints[3] = { 0.0f, 1.0f, 0.25f, 0.0f, 1.0f, 0.0f };
	X3DAUDIO_DISTANCE_CURVE       Emitter_LFE_Curve = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_LFE_CurvePoints[0], 3 };

	X3DAUDIO_DISTANCE_CURVE_POINT Emitter_Reverb_CurvePoints[3] = { 0.0f, 0.5f, 0.75f, 1.0f, 1.0f, 0.0f };
	X3DAUDIO_DISTANCE_CURVE       Emitter_Reverb_Curve = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_Reverb_CurvePoints[0], 3 };

	float soundEffectVolume = 20.0f;
	float backgroundMusicVolume = 10.0f;
	char* winSoundPath = "data/winn-up.wav";
	char* getFlagSoundPath = "data/gotFlag.wav";
	char* backgroundMusicPath = "data/backgroundMusic.wav";
	char* walkPath = "data/walk.wav";


#ifdef _DEBUG_MENU
	//debug menu
	float debugOpen = -1.0f;
	DebugMenu::cDebugMenu* pDebugMenu;
	
	float drawSphere = -1.0f;
	float drawPossibleCollisionScene = -1.0f;
	float enableOctreeDisplay = -1.0f;
	//debug shapes
	const char* debugBoxMeshPath = "data/debugBoxMesh.mesh";
	const char* debugSphereMeshPath = "data/debugSphereMesh.mesh";
	Mesh::cMesh* pDebugMeshArray[2];
	D3DXVECTOR3 debugScale(10.0f, 10.0f, 10.0f);
	IDirect3DVertexBuffer9* s_debugLineVertexBuffer = NULL;
	//draw the possible collision triangles
	IDirect3DVertexBuffer9* pOctreeCollisdeTriangleVertexBuffer;
	IDirect3DIndexBuffer9* pOctreeCollisdeTriangleIndexBuffer;
	
#endif
}

// Helper Function Declarations
//=============================

namespace
{
	bool CreateDevice( const HWND i_mainWindow );
	bool CreateInterface( const HWND i_mainWindow );
	bool CreateAudio();
	bool CreateVertexDeclaration();
	bool CreateCollisionInfo();
	bool CreateMesh();
	bool CreateMaterial();
	bool CreateFlagCaptureData();
	bool ServerClientChooseScreen();
#ifdef _DEBUG_MENU
	bool CreateDebugMenu();
	bool CreateOctreeCollideTriangleVertexBuffer(void);
	bool RebuildOctreeIndexBufferAccordingToNewTriangleList(void);
	bool CreateDebugLineVertexBuffer(const DWORD i_usage);
	void DrawOctreeBox(Player::OctreeNode* i_currentOctree);
#endif
	//matrix functions
	D3DXMATRIX GetTransform(D3DXVECTOR3& i_position);
	D3DXMATRIX GetTransform(D3DXVECTOR3& i_position, D3DXQUATERNION& i_rotation);
	D3DXMATRIX GetTransform(D3DXVECTOR3& i_position, D3DXVECTOR3& i_scale);
	D3DXMATRIX GetTransform(D3DXVECTOR3& i_position, D3DXVECTOR3& i_scale, D3DXMATRIX& i_rotation);
	D3DXMATRIX GetWorldToViewTransform(D3DXVECTOR3& i_translate);
	void CameraMoveForward(float i_moveAmount);
	void CameraMoveUpward(float i_moveAmount);
	void CameraMoveLeft(float i_moveAmount);
	void PlayerMoveForward(float i_moveAmount);
	void PlayerMoveUpward(float i_moveAmount);
	void PlayerMoveLeft(float i_moveAmount);
	void ApplyMatrix(D3DXMATRIX& matrix, D3DXVECTOR4& i_vector);
	D3DXMATRIX GetViewToScreenTransform();

	//compile shader
	bool LoadAndAllocateShaderProgram(const char* i_path, void*& o_compiledShader, std::string* o_errorMessage);
	//load binary mesh file
	bool LoadBinaryCollisionFile(const char*i_binaryMeshFilePath, char*& i_bufferPointer);

	//camera follow
	void CameraFollowPlayer(D3DXVECTOR3 &i_playerPos, float i_deltaT);
	bool VelocityEqual(D3DXVECTOR3& i_v1, D3DXVECTOR3& i_v2);
	bool PositionEqual(D3DXVECTOR3& i_p1, D3DXVECTOR3& i_p2);
	bool PositionClose(D3DXVECTOR3& i_p1, D3DXVECTOR3& i_p2);
	float Distance(D3DXVECTOR3& i_pos1, D3DXVECTOR3& i_pos2);
	void CameraPointToPlayer();

	//sound
	bool PlayWave(char* i_audioSoucePath, IXAudio2SourceVoice* &pSouce);
}

// Interface
//==========
ArcBall::cArcBall* GetArcBall()
{
	return arcBall;
}
bool Initialize( const HWND i_mainWindow )
{
	// Initialize the interface to the Direct3D9 library
	if ( !CreateInterface( i_mainWindow ) )
	{
		return false;
	}
	// Create an interface to a Direct3D device
	if ( CreateDevice( i_mainWindow ) )
	{
		s_mainWindow = i_mainWindow;
	}
	else
	{
		goto OnError;
	}
	
	if (!CreateVertexDeclaration())
	{
		goto OnError;
	}
	if (!CreateCollisionInfo())
	{
		goto OnError;
	}
	
	
#ifdef _DEBUG_MENU
	if (!CreateDebugMenu())
	{
		goto OnError;
	}
#endif
	if (!ServerClientChooseScreen())
	{
		goto OnError;
	}
	if (!CreateMesh())
	{
		goto OnError;
	}
	if (!CreateMaterial())
	{
		goto OnError;
	}
	if (!CreateFlagCaptureData())
	{
		goto OnError;
	}
	if (!CreateAudio())
	{
		goto OnError;
	}
	return true;

OnError:

	ShutDown();
	return false;
}

void Render()
{
	//input
	D3DXVECTOR3 offset(0.0f, 0.0f,0.0f);
	D3DXVECTOR4 cameraMovement(0.0f,0.0f,0.0f,1.0f);
	{
		// Get the direction
		{
			if (eae6320::UserInput::IsKeyPressed(VK_LEFT))
			{
				offset.x -= 1.0f;
#ifdef _DEBUG_MENU
				if (debugOpen>0.0f)
				{
					if ((int)(pDebugMenu->debugItemIndex)==1)
					cameraSpeed -= 0.5f;

					if ((int)(pDebugMenu->debugItemIndex) == 6)
					{
						soundEffectVolume -= 0.5f;
						if (soundEffectVolume < 0.0f)
							soundEffectVolume = 0.0f;
					}
					if ((int)(pDebugMenu->debugItemIndex) == 7)
					{
						backgroundMusicVolume -= 0.5f;
						if (backgroundMusicVolume < 0.0f)
							backgroundMusicVolume = 0.0f;

						pBackgroundSource->SetVolume(backgroundMusicVolume/20);
					}
				}
#endif
			}
			if (eae6320::UserInput::IsKeyPressed(VK_RIGHT))
			{
				offset.x += 1.0f;
#ifdef _DEBUG_MENU
				if (debugOpen>0.0f)
				{
					if ((int)(pDebugMenu->debugItemIndex) == 1)
					cameraSpeed += 0.5f;

					if ((int)(pDebugMenu->debugItemIndex) == 6)
					{
						soundEffectVolume += 0.5f;
						if (soundEffectVolume > 100.0f)
							soundEffectVolume = 100.0f;
					}
						
					if ((int)(pDebugMenu->debugItemIndex) == 7)
					{
						backgroundMusicVolume += 0.5f;
						if (backgroundMusicVolume > 100.0f)
							backgroundMusicVolume = 100.0f;

						pBackgroundSource->SetVolume(backgroundMusicVolume / 20);
					}
				}
#endif
			}
			if (eae6320::UserInput::IsKeyPressed(VK_UP))
			{
				offset.z += 1.0f;
#ifdef _DEBUG_MENU
				if (debugOpen>0.0f)
				{
					pDebugMenu->debugItemIndex -= 0.1f;
					if (pDebugMenu->debugItemIndex < 0.0f)
						pDebugMenu->debugItemIndex = 0.0f;
				}
#endif
			}
			if (eae6320::UserInput::IsKeyPressed(VK_DOWN))
			{
				offset.z -= 1.0f;
#ifdef _DEBUG_MENU
				if (debugOpen>0.0f)
				{
					pDebugMenu->debugItemIndex += 0.1f;
					if (pDebugMenu->debugItemIndex > 7.0f)
						pDebugMenu->debugItemIndex = 7.0f;
				}
#endif
			}
			
			if (eae6320::UserInput::IsKeyPressed(VK_SPACE))
			{
#ifdef _DEBUG_MENU
				if (debugOpen > 0.0f)
				{
					if ((int)(pDebugMenu->debugItemIndex) == 2)
					{
						cameraPosition.x = 0.0f;
						cameraPosition.y = 0.0f;
						cameraPosition.z = -10.0f;

						arcBall->Reset();
					}
					if ((int)(pDebugMenu->debugItemIndex) == 3)
					{
						if (drawSphere > 0.0f)
						{
							drawSphere -= 0.2f;
							if (drawSphere < 0.0f)
								drawSphere = -1.0f;
						}
						else
						{
							drawSphere += 0.2f;
							if (drawSphere>0.0f)
								drawSphere = 1.0f;
						}
					}
					if ((int)(pDebugMenu->debugItemIndex) == 4)
					{
						if (drawPossibleCollisionScene > 0.0f)
						{
							drawPossibleCollisionScene -= 0.2f;
							if (drawPossibleCollisionScene < 0.0f)
								drawPossibleCollisionScene = -1.0f;
						}
						else
						{
							drawPossibleCollisionScene += 0.2f;
							if (drawPossibleCollisionScene>0.0f)
								drawPossibleCollisionScene = 1.0f;
						}
					}
					if ((int)(pDebugMenu->debugItemIndex) == 5)
					{
						if ( enableOctreeDisplay> 0.0f)
						{
							enableOctreeDisplay -= 0.2f;
							if (enableOctreeDisplay < 0.0f)
								enableOctreeDisplay = -1.0f;
						}
						else
						{
							enableOctreeDisplay += 0.2f;
							if (enableOctreeDisplay>0.0f)
								enableOctreeDisplay = 1.0f;
						}
					}
				}
#endif
			}
			if (eae6320::UserInput::IsKeyPressed(VK_OEM_3))
			{
#ifdef _DEBUG_MENU
				if (debugOpen > 0.0f)
				{
					debugOpen -= 0.2f;
					if (debugOpen < 0.0f)
						debugOpen = -1.0f;
				}
				else
				{
					debugOpen += 0.2f;
					if (debugOpen>0.0f)
						debugOpen = 1.0f;
				}
					

#endif
			}
			if (isServer)
			{
				if (eae6320::UserInput::IsKeyPressed(0x57))//w
				{
					cameraMovement.z = 10.0f*cameraSpeed;
					if (bindPlayer > 0.0f)
					{
						PlayerMoveForward(cameraSpeed / 5);
					}

				}
				if (eae6320::UserInput::IsKeyPressed(0x41))//a
				{
					cameraMovement.x = 10.0f*cameraSpeed;
					if (bindPlayer>0.0f)
						PlayerMoveLeft(cameraSpeed / 5);
				}
				if (eae6320::UserInput::IsKeyPressed(0x53))//s
				{
					cameraMovement.z = -10.0f*cameraSpeed;
					if (bindPlayer>0.0f)
						PlayerMoveForward(-cameraSpeed / 5);
				}
				if (eae6320::UserInput::IsKeyPressed(0x44))//d
				{
					cameraMovement.x = -10.0f*cameraSpeed;
					if (bindPlayer>0.0f)
						PlayerMoveLeft(-cameraSpeed / 5);
				}
				if (eae6320::UserInput::IsKeyPressed(0x57) || eae6320::UserInput::IsKeyPressed(0x41) || eae6320::UserInput::IsKeyPressed(0x53) || eae6320::UserInput::IsKeyPressed(0x44))
				{
					PlayWave(walkPath, pWalkSource);
				}
				if (!eae6320::UserInput::IsKeyPressed(0x57) && !eae6320::UserInput::IsKeyPressed(0x41) && !eae6320::UserInput::IsKeyPressed(0x53) && !eae6320::UserInput::IsKeyPressed(0x44))
				{
					if (pWalkSource)
					{
						pWalkSource->Stop();
					}
				}
				if (eae6320::UserInput::IsKeyPressed(VK_LSHIFT))
				{
					if (sprintValue > 0.0f)
					{
						cameraSpeed = 30;
						sprintValue -= 0.03;
						if (sprintValue < 0.0f)
							sprintValue = 0.0f;
					}
					else cameraSpeed = 10;
				}
				else
				{
					cameraSpeed = 10;
					sprintValue += 0.01;
					if (sprintValue > 10.0f)
						sprintValue = 10.0f;
				}
			}
			else
			{
				if (eae6320::UserInput::IsKeyPressed(0x49))//i
				{
					cameraMovement.z = 10.0f*cameraSpeed;
					if (bindPlayer>0.0f)
						PlayerMoveForward(cameraSpeed / 5);

				}
				if (eae6320::UserInput::IsKeyPressed(0x4a))//j
				{
					cameraMovement.x = 10.0f*cameraSpeed;
					if (bindPlayer>0.0f)
						PlayerMoveLeft(cameraSpeed / 5);
				}
				if (eae6320::UserInput::IsKeyPressed(0x4b))//k
				{
					cameraMovement.z = -10.0f*cameraSpeed;
					if (bindPlayer>0.0f)
						PlayerMoveForward(-cameraSpeed / 5);
				}
				if (eae6320::UserInput::IsKeyPressed(0x4c))//l
				{
					cameraMovement.x = -10.0f*cameraSpeed;
					if (bindPlayer>0.0f)
						PlayerMoveLeft(-cameraSpeed / 5);
				}

				if (eae6320::UserInput::IsKeyPressed(VK_RSHIFT))
				{
					if (sprintValue > 0.0f)
					{
						cameraSpeed = 30;
						sprintValue -= 0.03;
						if (sprintValue < 0.0f)
							sprintValue = 0.0f;
					}
					else cameraSpeed = 10;
				}
				else
				{
					cameraSpeed = 10;
					sprintValue += 0.01;
					if (sprintValue > 10.0f)
						sprintValue = 10.0f;
				}
			}
			if (eae6320::UserInput::IsKeyPressed(0x52))//r
			{
				cameraMovement.y = +10.0f*cameraSpeed;
			}
			if (eae6320::UserInput::IsKeyPressed(0x46))//f
			{
				cameraMovement.y = -10.0f*cameraSpeed;
			}
			if (eae6320::UserInput::IsKeyPressed(0x42))//b: represent binding player
			{
				if (bindPlayer > 0.0f)
				{
					bindPlayer -= 0.2f;
					if (bindPlayer < 0.0f)
						bindPlayer = -1.0f;
				}
				else
				{
					bindPlayer += 0.2f;
					if (bindPlayer>0.0f)
						bindPlayer = 1.0f;
				}
			}

			//control lighting
			if (eae6320::UserInput::IsKeyPressed(0x4a))//j
			{
				directionLight.x += 0.01f;
			}
			if (eae6320::UserInput::IsKeyPressed(0x4c))//l
			{
				directionLight.x -= 0.01f;
			}
			if (eae6320::UserInput::IsKeyPressed(0x49))//i
			{
				directionLight.z -= 0.01f;
			}
			if (eae6320::UserInput::IsKeyPressed(0x4b))//k
			{
				directionLight.z += 0.01f;
			}

		}
		// Get the speed
		float unitsPerSecond = 1.0f;
		// Calculate the offset
		offset *= (unitsPerSecond * eae6320::Time::GetSecondsElapsedThisFrame())*100;
		cameraMovement *= (unitsPerSecond * eae6320::Time::GetSecondsElapsedThisFrame());
		cameraMovement.w = 1.0f;
	}
	
	if (bindPlayer > 0.0f)
	{
		//flag capture
		D3DXVECTOR3 redFlagPos(1500, 200, 1500);
		D3DXVECTOR3 blueFlagPos(-1500, 200, -1500);
		D3DXVECTOR3 flagShift(-10.5444, 648.169, 34.9844);
		//dog go get red flag!
		if (getRedFlag == false && Distance(pCTFMeshArray[9]->meshPosition, redFlagPos) < 200)
		{
			PlayWave(getFlagSoundPath, pSoundEffectSource);
			pSoundEffectSource->SetVolume(soundEffectVolume/20);
			getRedFlag = true;
		}
		//cat go get blue flag!
		if (getBlueFlag == false && Distance(pCTFMeshArray[6]->meshPosition, blueFlagPos) < 200)
		{
			PlayWave(getFlagSoundPath, pSoundEffectSource);
			pSoundEffectSource->SetVolume(soundEffectVolume/20);
			getBlueFlag = true;
		}
		//dog
		if (getRedFlag)
		{
			if (Distance(pCTFMeshArray[6]->meshPosition, pPlayer->pPlayerMesh->meshPosition)<200)
			{
				getRedFlag = false;
				pCTFMeshArray[7]->meshPosition = redFlagPos;
			}
			pCTFMeshArray[7]->meshPosition = flagShift + pCTFMeshArray[9]->meshPosition + D3DXVECTOR3(0, 150, 0);
		}
		else
		{
			pCTFMeshArray[7]->meshPosition = flagShift + redFlagPos;
		}
		//cat
		if (getBlueFlag)
		{
			if (Distance(pCTFMeshArray[9]->meshPosition, pPlayer->pPlayerMesh->meshPosition)<200)
			{
				getBlueFlag = false;
				pCTFMeshArray[8]->meshPosition = blueFlagPos;
			}
			pCTFMeshArray[8]->meshPosition = flagShift + pPlayer->pPlayerMesh->meshPosition + D3DXVECTOR3(0, 150, 0);
		}
		else
		{
			pCTFMeshArray[8]->meshPosition = flagShift + blueFlagPos;
		}

		//dog get flag and back to its area
		if (getRedFlag&&Distance(pCTFMeshArray[9]->meshPosition, blueFlagPos) < 200)
		{
			//player sound effect
			PlayWave(winSoundPath, pSoundEffectSource);
			
			getRedFlag = false;
			getBlueFlag = false;
			// reset the flag position
			pCTFMeshArray[7]->meshPosition = redFlagPos;
			pCTFMeshArray[8]->meshPosition = blueFlagPos;
			//dog score increase
			dogScore++;
		}
		//cat get flag and back to its area
		if (getBlueFlag&&Distance(pCTFMeshArray[6]->meshPosition, redFlagPos) < 200)
		{
			//player sound effect
			PlayWave(winSoundPath, pSoundEffectSource);

			getBlueFlag = false;
			getRedFlag = false;
			//reset blue flag position
			pCTFMeshArray[8]->meshPosition = blueFlagPos;
			pCTFMeshArray[7]->meshPosition = redFlagPos;
			//player score increase
			catScore++;
		}

		
	
		//player camera movment
		CameraFollowPlayer(pPlayer->pPlayerMesh->meshPosition, eae6320::Time::GetSecondsElapsedThisFrame()); 
		/*if (!initBindPos)
		{
			cameraPosition.x = pPlayer->pPlayerMesh->meshPosition.x;
			cameraPosition.y = pPlayer->pPlayerMesh->meshPosition.y;
			cameraPosition.z = pPlayer->pPlayerMesh->meshPosition.z;
			initBindPos = true;
		}
		pPlayer->pPlayerMesh->meshRotationMatrix = *(arcBall->GetRotationMatrix());

		D3DXMATRIX OEZRot;
		D3DXMatrixRotationY(&OEZRot, 3.14);
		pPlayer->pPlayerMesh->meshRotationMatrix *= OEZRot;

		pPlayer->pPlayerMesh->meshRotationMatrix._13 *= -1;
		pPlayer->pPlayerMesh->meshRotationMatrix._31 *= -1;
		CameraMoveForward(cameraMovement.z);
		CameraMoveLeft(cameraMovement.x);
		CameraMoveUpward(cameraMovement.y);
		pPlayer->pPlayerMesh->meshPosition.x = cameraPosition.x;
		pPlayer->pPlayerMesh->meshPosition.y = cameraPosition.y;
		pPlayer->pPlayerMesh->meshPosition.z = cameraPosition.z;*/

		pPlayer->UpdateCollisionSegment(pPlayer->pPlayerMesh->meshPosition, D3DXVECTOR3(50, 50, 50)*0.5);

		octreeCollideTriangleList.clear();
		if (pPlayer->IfCollideWithOctreeScene(pSceneOctree,octreeCollisionTriangleVertices,octreeCollisionTriangleIndices,octreeCollideTriangleList))
		{
			pPlayer->pPlayerMesh->meshPosition = posBefore;
			if (pPlayer->floorFlag)
			{
				//climb the floor
				PlayerMoveUpward((pPlayer->floorHeight * 500)*(eae6320::Time::GetSecondsElapsedThisFrame()));
				/*CameraMoveUpward((pPlayer->floorHeight*400 - cameraPosition.y)*(eae6320::Time::GetSecondsElapsedThisFrame()));
				
				pPlayer->pPlayerMesh->meshPosition.x = cameraPosition.x;
				pPlayer->pPlayerMesh->meshPosition.y = cameraPosition.y;
				pPlayer->pPlayerMesh->meshPosition.z = cameraPosition.z;
*/
			}
			else
			{
				pPlayer->pPlayerMesh->meshPosition = posBefore;
				/*CameraMoveForward(-cameraMovement.z);
				CameraMoveLeft(-cameraMovement.x);

				pPlayer->pPlayerMesh->meshPosition.x = cameraPosition.x;
				pPlayer->pPlayerMesh->meshPosition.y = cameraPosition.y;
				pPlayer->pPlayerMesh->meshPosition.z = cameraPosition.z;*/

				
			}
			pPlayer->UpdateCollisionSegment(pPlayer->pPlayerMesh->meshPosition, D3DXVECTOR3(50, 50, 50)*0.5);
		}
		else
		{
			PlayerMoveUpward((-200)*(eae6320::Time::GetSecondsElapsedThisFrame()));
			pPlayer->UpdateCollisionSegment(pPlayer->pPlayerMesh->meshPosition, D3DXVECTOR3(50, 50, 50));
			if (pPlayer->IfCollideWithOctreeScene(pSceneOctree, octreeCollisionTriangleVertices, octreeCollisionTriangleIndices, octreeCollideTriangleList))
			{
				pPlayer->pPlayerMesh->meshPosition = posBefore;
			}
			pPlayer->UpdateCollisionSegment(pPlayer->pPlayerMesh->meshPosition, D3DXVECTOR3(50, 50, 50));
			/*float fallAmount = (-200)*(eae6320::Time::GetSecondsElapsedThisFrame());
			CameraMoveUpward(fallAmount);
			pPlayer->pPlayerMesh->meshPosition.x = cameraPosition.x;
			pPlayer->pPlayerMesh->meshPosition.y = cameraPosition.y;
			pPlayer->pPlayerMesh->meshPosition.z = cameraPosition.z;
			pPlayer->UpdateCollisionSegment(pPlayer->pPlayerMesh->meshPosition, pPlayer->pPlayerMesh->meshScale);
			if (pPlayer->IfCollideWithOctreeScene(pSceneOctree, octreeCollisionTriangleVertices, octreeCollisionTriangleIndices, octreeCollideTriangleList))
			{
				CameraMoveUpward(-fallAmount);
				pPlayer->pPlayerMesh->meshPosition.x = cameraPosition.x;
				pPlayer->pPlayerMesh->meshPosition.y = cameraPosition.y;
				pPlayer->pPlayerMesh->meshPosition.z = cameraPosition.z;

				pPlayer->UpdateCollisionSegment(pPlayer->pPlayerMesh->meshPosition, pPlayer->pPlayerMesh->meshScale);
			}*/
		}
	}
	else
	{
		initBindPos = false;
		CameraMoveForward(cameraMovement.z);
		CameraMoveLeft(cameraMovement.x);
	}

	//3d audio
	D3DXMATRIX playerRot;
	D3DXMatrixInverse(&playerRot, NULL, &(pPlayer->pPlayerMesh->meshRotationMatrix));

	D3DXVECTOR3 vWorldAheadPlayer;
	D3DXVECTOR3 vLocalAhead = D3DXVECTOR3(0, 0, -1);
	D3DXVec3TransformCoord(&vWorldAheadPlayer, &vLocalAhead, &playerRot);
	vWorldAheadPlayer.x *= -1;

	Emitter.OrientFront = vWorldAheadPlayer;
	Emitter.OrientTop = D3DXVECTOR3(0, 1, 0);
	Emitter.Position = pPlayer->pPlayerMesh->meshPosition;
	Emitter.Velocity = D3DXVECTOR3(cameraSpeed / 5, cameraSpeed / 5, cameraSpeed / 5);

	D3DXMATRIX cameraRot;
	D3DXMatrixInverse(&cameraRot, NULL, arcBall->GetRotationMatrix());

	D3DXVECTOR3 vWorldAheadCamera;
	vLocalAhead = D3DXVECTOR3(0, 0, 1);
	D3DXVec3TransformCoord(&vWorldAheadCamera, &vLocalAhead, &cameraRot);

	Listener.OrientFront = vWorldAheadCamera;
	Listener.OrientTop = D3DXVECTOR3(0, 1, 0);
	Listener.Position = cameraPosition;
	Listener.Velocity = D3DXVECTOR3(cameraSpeed, cameraSpeed, cameraSpeed);

	DWORD dwCalcFlags = X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER
		| X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_LPF_REVERB
		| X3DAUDIO_CALCULATE_REVERB;

	X3DAudioCalculate(X3DInstance, &Listener, &Emitter,dwCalcFlags,&DSPSettings);
	if (pWalkSource)
	{
		pWalkSource->SetFrequencyRatio(DSPSettings.DopplerFactor);
		pWalkSource->SetOutputMatrix(pMasterVoice, INPUTCHANNELS, deviceDetails.OutputFormat.Format.nChannels, DSPSettings.pMatrixCoefficients);
		pWalkSource->SetOutputMatrix(pSubmixVoice, 1, 1, &DSPSettings.ReverbLevel);

		XAUDIO2_FILTER_PARAMETERS FilterParametersDirect = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f *DSPSettings.LPFDirectCoefficient), 1.0f }; // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here
		pWalkSource->SetOutputFilterParameters(pMasterVoice, &FilterParametersDirect);
		XAUDIO2_FILTER_PARAMETERS FilterParametersReverb = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * DSPSettings.LPFReverbCoefficient), 1.0f }; // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here
		pWalkSource->SetOutputFilterParameters(pSubmixVoice, &FilterParametersReverb);
	}

	//network communication
	{
		for (packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
		{

			switch (packet->data[0])
			{
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
				printf("Another client has disconnected.\n");
				break;
			case ID_REMOTE_CONNECTION_LOST:
				printf("Another client has lost the connection.\n");
				break;
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
				printf("Another client has connected.\n");
				break;
			case ID_CONNECTION_REQUEST_ACCEPTED:
			{
												   printf("Our connection request has been accepted.\n");

												   // Use a BitStream to write a custom user message
												   // Bitstreams are easier to use than sending casted structures, and handle endian swapping automatically
												   RakNet::BitStream bsOut;
												   bsOut.Write((RakNet::MessageID)ID_KEEP_PASSING_DATA);
												   //pass dog position to server cat
												   bsOut.Write(pPlayer->pPlayerMesh->meshPosition.x);
												   bsOut.Write(pPlayer->pPlayerMesh->meshPosition.y);
												   bsOut.Write(pPlayer->pPlayerMesh->meshPosition.z);
												   //rotation matrix
												   bsOut.Write(pPlayer->pPlayerMesh->meshRotationMatrix);
												   //pass dog score
												   bsOut.Write(dogScore);

												   peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
			}
				break;
			case ID_NEW_INCOMING_CONNECTION:
				printf("A connection is incoming.\n");
				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				printf("The server is full.\n");
				break;
			case ID_DISCONNECTION_NOTIFICATION:
				if (isServer){
					printf("A client has disconnected.\n");
				}
				else {
					printf("We have been disconnected.\n");
				}
				break;
			case ID_CONNECTION_LOST:
				if (isServer){
					printf("A client lost the connection.\n");
				}
				else {
					printf("Connection lost.\n");
				}
				break;

			case ID_KEEP_PASSING_DATA:
			{
										 if (isServer)
										 {
											 //cat receive infomation 
											 RakNet::RakString rs;
											 float dogX = 0.0f, dogY = 0.0f, dogZ = 0.0f;
											 int getDogScore = 0;
											 RakNet::BitStream bsIn(packet->data, packet->length, false);
											 bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
											 //get dogX, dogY, dogZ
											 bsIn.Read(dogX);
											 bsIn.Read(dogY);
											 bsIn.Read(dogZ);
											 pCTFMeshArray[9]->meshPosition = D3DXVECTOR3(dogX, dogY, dogZ);
											 //rotation matrix
											 bsIn.Read(pCTFMeshArray[9]->meshRotationMatrix);
											 //get score
											 bsIn.Read(getDogScore);
											 dogScore = getDogScore;

											 //and write out information
											 RakNet::BitStream bsOut;
											 bsOut.Write((RakNet::MessageID)ID_KEEP_PASSING_DATA);
											 //write out cat posision and cat score
											 bsOut.Write(pPlayer->pPlayerMesh->meshPosition.x);
											 bsOut.Write(pPlayer->pPlayerMesh->meshPosition.y);
											 bsOut.Write(pPlayer->pPlayerMesh->meshPosition.z);
											 //rotation matrix
											 bsOut.Write(pPlayer->pPlayerMesh->meshRotationMatrix);
											 //score
											 bsOut.Write(catScore);
											 peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
										 }
										 else
										 {
											 //dog receive infomation 
											 RakNet::RakString rs;
											 float catX = 0.0f, catY = 0.0f, catZ = 0.0f;
											 int getCatScore = 0;
											 RakNet::BitStream bsIn(packet->data, packet->length, false);
											 bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
											 //get dogX, dogY, dogZ
											 bsIn.Read(catX);
											 bsIn.Read(catY);
											 bsIn.Read(catZ);
											 pCTFMeshArray[6]->meshPosition = D3DXVECTOR3(catX, catY, catZ);
											 //rotation matrix
											 bsIn.Read(pCTFMeshArray[6]->meshRotationMatrix);
											 //get score
											 bsIn.Read(getCatScore);
											 catScore = getCatScore;

											 //and write out information
											 RakNet::BitStream bsOut;
											 bsOut.Write((RakNet::MessageID)ID_KEEP_PASSING_DATA);
											 //write out cat posision and cat score
											 bsOut.Write(pPlayer->pPlayerMesh->meshPosition.x);
											 bsOut.Write(pPlayer->pPlayerMesh->meshPosition.y);
											 bsOut.Write(pPlayer->pPlayerMesh->meshPosition.z);
											 //rotation matrix
											 bsOut.Write(pPlayer->pPlayerMesh->meshRotationMatrix);
											 //score
											 bsOut.Write(dogScore);
											 peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);

										 }

			}
				break;

			default:
			{
					   printf("Message with identifier %i has arrived.\n", packet->data[0]);
					   break;
			}

			}
		}

	}

	// Every frame an entirely new image will be created.
	// Before drawing anything, then, the previous image will be erased
	// by "clearing" the image buffer (filling it with a solid color)
	// and any other associated buffers (filling them with whatever values make sense)
	{
		const D3DRECT* subRectanglesToClear = NULL;
		const DWORD subRectangleCount = 0;
		const DWORD clearColorAndDepth = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
		const D3DCOLOR colorToClear = D3DCOLOR_XRGB(0, 0, 0);
		const float depthToClear = 1.0f;
		const DWORD noStencilBuffer = 0;
		HRESULT result = s_direct3dDevice->Clear(subRectangleCount, subRectanglesToClear,
			clearColorAndDepth, colorToClear, depthToClear, noStencilBuffer);
		assert(SUCCEEDED(result));
		s_direct3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
	}

	// The actual function calls that draw geometry must be made between paired calls to
	// BeginScene() and EndScene()
	{
		HRESULT result = s_direct3dDevice->BeginScene();
		assert( SUCCEEDED( result ) );
		{
			D3DPERF_BeginEvent(0, L"Mesh Rendering");
			s_direct3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			s_direct3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
			s_direct3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
			s_direct3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);

			int i = 0;
			while (i <sceneObjNum-1)
			{
				//set material
				D3DPERF_BeginEvent(0, L"Set Material per instance");
				result = s_direct3dDevice->SetPixelShader(pCTFMaterialArray[i]->pFragmentShader);
				assert(SUCCEEDED(result));
				D3DPERF_EndEvent();
				//set per view constance
				pCTFMaterialArray[i]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixViewToScreenHandler, &GetViewToScreenTransform());
				//lighting
				pCTFMaterialArray[i]->fragmentShaderconstantsTable->SetVector(s_direct3dDevice, constantHandlerDirectionLight, &directionLight);

				//draw ball mesh
				D3DPERF_BeginEvent(0, L"Draw Mesh");
				{
					// Set the shaders
					{
						pCTFMaterialArray[i]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixModelToWorldHandler, &GetTransform(pCTFMeshArray[i]->meshPosition, pCTFMeshArray[i]->meshScale, pCTFMeshArray[i]->meshRotationMatrix));

						pCTFMaterialArray[i]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixWorldToViewHandler, &GetWorldToViewTransform(cameraPosition));

						result = s_direct3dDevice->SetVertexShader(pCTFMaterialArray[i]->pVertexShader);
						assert(SUCCEEDED(result));
					}
					//set texture
					D3DPERF_BeginEvent(0, L"Set Material per view");
					{
						result = s_direct3dDevice->SetTexture(samplerRegister, pCTFMaterialArray[i]->pTexture);
						assert(SUCCEEDED(result));
						result = s_direct3dDevice->SetTexture(samplerRegister2, pCTFMaterialArray[i]->pTexture);
						assert(SUCCEEDED(result));
					}
					D3DPERF_EndEvent();
					// Bind a specific vertex buffer to the device as a data source
					{
						// There can be multiple streams of data feeding the display adaptor at the same time
						const unsigned int streamIndex = 0;
						// It's possible to start streaming data in the middle of a vertex buffer
						const unsigned int bufferOffset = 0;
						// The "stride" defines how large a single vertex is in the stream of data
						const unsigned int bufferStride = sizeof(sVertex);
						result = s_direct3dDevice->SetStreamSource(streamIndex, pCTFMeshArray[i]->pVertexBuffer, bufferOffset, bufferStride);
						assert(SUCCEEDED(result));
					}
					// Set the indices to use
					{
						HRESULT result = s_direct3dDevice->SetIndices(pCTFMeshArray[i]->pIndexBuffer);
						assert(SUCCEEDED(result));
					}
					// Render objects from the current streams
					{
						// We are using triangles as the "primitive" type,
						// and we have defined the vertex buffer as a triangle list
						// (meaning that every triangle is defined by three vertices)
						const D3DPRIMITIVETYPE primitiveType = D3DPT_TRIANGLELIST;
						// It's possible to start rendering primitives in the middle of the stream
						const unsigned int indexOfFirstVertexToRender = 0;
						const unsigned int indexOfFirstIndexToUse = 0;
						// EAE6320_TODO: How many vertices are in a rectangle,
						// and how many "primitives" (triangles) is it made up of?
						const unsigned int vertexCountToRender = pCTFMeshArray[i]->vertexNum;
						const unsigned int primitiveCountToRender = pCTFMeshArray[i]->indexNum / 3;
						HRESULT result = s_direct3dDevice->DrawIndexedPrimitive(primitiveType, indexOfFirstVertexToRender, indexOfFirstVertexToRender,
							vertexCountToRender, indexOfFirstIndexToUse, primitiveCountToRender);
						assert(SUCCEEDED(result));
					}
				}
				D3DPERF_EndEvent();
				
				i++;
			}
			

			D3DPERF_EndEvent();//end of mesh rendering


			//draw flag capture text
			{
				D3DPERF_BeginEvent(0, L"flag capture text Rendering");

				//sprint
				int sprintGrid = (int)sprintValue;
				std::string sprintText = "Sprint:";
				for (int i = 0; i < sprintGrid; i++)
				{
					sprintText += "I";
				}
				pFlagCapText->pFont->DrawText(NULL, sprintText.c_str(), -1,
					&(pFlagCapText->debugItems[0].rect), DT_LEFT | DT_NOCLIP, 0xFFFFFFFF);

				//cat team( red) score
				std::string catScoreText = "Cat:" + std::to_string(catScore);
				pFlagCapText->pFont->DrawText(NULL, catScoreText.c_str(), -1,
					&(pFlagCapText->debugItems[1].rect), DT_LEFT | DT_NOCLIP, 0xFFFFFFFF);
				//dog team(blue) score
				std::string blueScoreText = "Dog:" + std::to_string(dogScore);
				pFlagCapText->pFont->DrawText(NULL, blueScoreText.c_str(), -1,
					&(pFlagCapText->debugItems[2].rect), DT_LEFT | DT_NOCLIP, 0xFFFFFFFF);

				D3DPERF_EndEvent();
			}
#ifdef _DEBUG_MENU
			if (debugOpen>0)
			{

				D3DPERF_BeginEvent(0, L"Debug Menu Rendering");
				
				//frame rate
				float time = 1/eae6320::Time::GetSecondsElapsedThisFrame();
				std::string timeText = "Frame Rate:"+std::to_string(time);
				pDebugMenu->pFont->DrawText(NULL,timeText.c_str(), -1,
					&(pDebugMenu->debugItems[0].rect), DT_LEFT | DT_NOCLIP, 0xFFFFFFFF);
				//camera speed
				std::string cameraSpeedText = "Camera Fly Speed:" + std::to_string(cameraSpeed);
				pDebugMenu->pFont->DrawText(NULL, cameraSpeedText.c_str(), -1,
					&(pDebugMenu->debugItems[1].rect), DT_LEFT | DT_NOCLIP, 0xFFFFFFFF);
				//reset camera position and rotation
				std::string resetCameraText = "Reset Camera(press space bar to reset)";
				pDebugMenu->pFont->DrawText(NULL, resetCameraText.c_str(), -1,
					&(pDebugMenu->debugItems[2].rect), DT_LEFT | DT_NOCLIP, 0xFFFFFFFF);
				//draw sphere
				std::string enableSphereText;
				int numberToDraw = 2;
				if (drawSphere>0.0f)
				{
					enableSphereText = "Enable/Disable Sphere:";
					enableSphereText += "ON";
					numberToDraw = 2;
				}
				else
				{
					enableSphereText = "Enable/Disable Sphere:";
					enableSphereText += "OFF";
					numberToDraw = 1;
				}
				pDebugMenu->pFont->DrawText(NULL, enableSphereText.c_str(), -1,
					&(pDebugMenu->debugItems[3].rect), DT_LEFT | DT_NOCLIP, 0xFFFFFFFF);
				//enable collision scene dynamic draw
				std::string enablePossibleCollisionSceneText;
				if (drawPossibleCollisionScene>0.0f)
				{
					enablePossibleCollisionSceneText = "Enable/Disable Dynamic Possible Collision Environment:";
					enablePossibleCollisionSceneText += "ON";
				}
				else
				{
					enablePossibleCollisionSceneText = "Enable/Disable Dynamic Possible Collision Environment:";
					enablePossibleCollisionSceneText += "OFF";
				}
				pDebugMenu->pFont->DrawText(NULL, enablePossibleCollisionSceneText.c_str(), -1,
					&(pDebugMenu->debugItems[4].rect), DT_LEFT | DT_NOCLIP, 0xFFFFFFFF);
				//draw octree box intersect with camera ray casting
				std::string octreeDisplayText;
				if (enableOctreeDisplay>0.0f)
				{
					octreeDisplayText = "Enable/Disable Octree Display:";
					octreeDisplayText += "ON";
				}
				else
				{
					octreeDisplayText = "Enable/Disable Octree Display:";
					octreeDisplayText += "OFF";
				}
				pDebugMenu->pFont->DrawText(NULL, octreeDisplayText.c_str(), -1,
					&(pDebugMenu->debugItems[5].rect), DT_LEFT | DT_NOCLIP, 0xFFFFFFFF);

				//sound effect volume
				std::string soundEffectVolumeText = "Sound Effect Volume:" + std::to_string(soundEffectVolume);
				pDebugMenu->pFont->DrawTextA(NULL, soundEffectVolumeText.c_str(), -1,
					&(pDebugMenu->debugItems[6].rect), DT_LEFT | DT_NOCLIP, 0xFFFFFFFF);

				//background music volume
				std::string backgroundMusicVolumeText = "Background Music Volume:" + std::to_string(backgroundMusicVolume);
				pDebugMenu->pFont->DrawTextA(NULL, backgroundMusicVolumeText.c_str(), -1,
					&(pDebugMenu->debugItems[7].rect), DT_LEFT | DT_NOCLIP, 0xFFFFFFFF);

				//current item emphasize
				pDebugMenu->pFont->DrawText(NULL, pDebugMenu->debugItems[(int)(pDebugMenu->debugItemIndex)].text, -1,
					&(pDebugMenu->debugItems[(int)(pDebugMenu->debugItemIndex)].rect), DT_LEFT | DT_NOCLIP, 0xff00ff00);
				
				D3DPERF_EndEvent();

				D3DPERF_BeginEvent(0, L"Debug Object Rendering");
				//draw octree scene
				//draw the collision triangles out
				if (drawPossibleCollisionScene>0.0f)
				{
					//fist rebuild the refreshed triangle vertex and index buffer
					RebuildOctreeIndexBufferAccordingToNewTriangleList();

					//then made draw call
					{

						s_direct3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
						s_direct3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
						s_direct3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

						//set material
						D3DPERF_BeginEvent(0, L"Set Material per instance");
						result = s_direct3dDevice->SetPixelShader(pCTFMaterialArray[materialNum - 1]->pFragmentShader);
						assert(SUCCEEDED(result));
						D3DPERF_EndEvent();
						//set per view constance
						pCTFMaterialArray[materialNum - 1]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixViewToScreenHandler, &GetViewToScreenTransform());
						//lighting
						pCTFMaterialArray[materialNum - 1]->fragmentShaderconstantsTable->SetVector(s_direct3dDevice, constantHandlerDirectionLight, &directionLight);

						D3DPERF_BeginEvent(0, L"Draw collision triangle Mesh");
						{
							// Set the shaders
							{
								pCTFMaterialArray[materialNum - 1]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixModelToWorldHandler,
								&GetTransform(D3DXVECTOR3(0, 0, 0),
								D3DXVECTOR3(1, 1, 1)));

								pCTFMaterialArray[materialNum - 1]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixWorldToViewHandler, &GetWorldToViewTransform(cameraPosition));

								result = s_direct3dDevice->SetVertexShader(pCTFMaterialArray[materialNum - 1]->pVertexShader);
								assert(SUCCEEDED(result));
							}
							//set texture
							{
								D3DXVECTOR4 boxColor(0.9f, 0.5f, 1.0f, 1.0f);
								pCTFMaterialArray[materialNum - 1]->fragmentShaderconstantsTable->SetVector(s_direct3dDevice, constantHandlerColor, &boxColor);
							}
							// Bind a specific vertex buffer to the device as a data source
							{
								// There can be multiple streams of data feeding the display adaptor at the same time
								const unsigned int streamIndex = 0;
								// It's possible to start streaming data in the middle of a vertex buffer
								const unsigned int bufferOffset = 0;
								// The "stride" defines how large a single vertex is in the stream of data
								const unsigned int bufferStride = sizeof(sVertex);
								result = s_direct3dDevice->SetStreamSource(streamIndex, pOctreeCollisdeTriangleVertexBuffer, bufferOffset, bufferStride);
								assert(SUCCEEDED(result));
							}
							// Set the indices to use
							{
								HRESULT result = s_direct3dDevice->SetIndices(pOctreeCollisdeTriangleIndexBuffer);
								assert(SUCCEEDED(result));
							}
							// Render objects from the current streams
							{
								// We are using triangles as the "primitive" type,
								// and we have defined the vertex buffer as a triangle list
								// (meaning that every triangle is defined by three vertices)
								const D3DPRIMITIVETYPE primitiveType = D3DPT_TRIANGLELIST;
								// It's possible to start rendering primitives in the middle of the stream
								const unsigned int indexOfFirstVertexToRender = 0;
								const unsigned int indexOfFirstIndexToUse = 0;
								// EAE6320_TODO: How many vertices are in a rectangle,
								// and how many "primitives" (triangles) is it made up of?
								const unsigned int vertexCountToRender = octreeCollisionTriangleVertices.size();
								const unsigned int primitiveCountToRender = octreeCollideTriangleList.size();
								HRESULT result = s_direct3dDevice->DrawIndexedPrimitive(primitiveType, indexOfFirstVertexToRender, indexOfFirstVertexToRender,
									vertexCountToRender, indexOfFirstIndexToUse, primitiveCountToRender);
								assert(SUCCEEDED(result));
							}
						}
						D3DPERF_EndEvent();
					}
				}
				
				if (enableOctreeDisplay > 0.0f)
				{
					//first draw the ray cast line
					//cameraRayCastDir
					{
						sVertex* vertexData;
						{
							const unsigned int lockEntireBuffer = 0;
							const DWORD useDefaultLockingBehavior = 0;
							const HRESULT result = s_debugLineVertexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
								reinterpret_cast<void**>(&vertexData), useDefaultLockingBehavior);
						}
						vertexData[0].x = cameraPosition.x; vertexData[0].y = cameraPosition.y; vertexData[0].z = cameraPosition.z;
						vertexData[0].r = 0.1f; vertexData[0].g = 0.5f; vertexData[0].b = 0.9f;
						vertexData[1].x = cameraPosition.x + cameraRayCastDir.x * 1000; vertexData[1].y = cameraPosition.y + cameraRayCastDir.y * 1000; vertexData[1].z = cameraPosition.z + cameraRayCastDir.z * 1000;
						vertexData[1].r = 0.1f; vertexData[1].g = 0.5f; vertexData[1].b = 0.9f;
						s_debugLineVertexBuffer->Unlock();

						// Set the position

						for (unsigned int i = 0; i < 10; i++)
						{
							{
								pCTFMaterialArray[materialNum - 1]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixModelToWorldHandler, &GetTransform(D3DXVECTOR3(0, 0, 0), debugScale*0.1));
								pCTFMaterialArray[materialNum - 1]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixWorldToViewHandler, &GetWorldToViewTransform(cameraPosition));

								result = s_direct3dDevice->SetVertexShader(pCTFMaterialArray[materialNum - 1]->pVertexShader);
								assert(SUCCEEDED(result));
							}
							//set texture
									{
							D3DXVECTOR4 boxColor(0.1, 0.5, 0.9, 1.0f);
									
							pCTFMaterialArray[materialNum - 1]->SetColor(boxColor.x, boxColor.y, boxColor.z);
							pCTFMaterialArray[materialNum - 1]->fragmentShaderconstantsTable->SetVector(s_direct3dDevice, constantHandlerColor, &boxColor);
									}
							// Bind a specific vertex buffer to the device as a data source
							{
							// There can be multiple streams of data feeding the display adaptor at the same time
							const unsigned int streamIndex = 0;
							// It's possible to start streaming data in the middle of a vertex buffer
							const unsigned int bufferOffset = 0;
							// The "stride" defines how large a single vertex is in the stream of data
							const unsigned int bufferStride = sizeof(sVertex);
							result = s_direct3dDevice->SetStreamSource(streamIndex, s_debugLineVertexBuffer, bufferOffset, bufferStride);
							assert(SUCCEEDED(result));
							}

							// Render objects from the current streams
							{
								// We are using triangles as the "primitive" type,
								// and we have defined the vertex buffer as a triangle list
								// (meaning that every triangle is defined by three vertices)
								const D3DPRIMITIVETYPE lineList = D3DPT_LINELIST;
								// It's possible to start rendering primitives in the middle of the stream
								const unsigned int indexOfFirstVertexToRender = 0;
								HRESULT result = s_direct3dDevice->DrawPrimitive(lineList, indexOfFirstVertexToRender, 1);
								assert(SUCCEEDED(result));
							}
						}
						
						//then draw the octree box
						DrawOctreeBox(pSceneOctree);
					}
					
				}
				
				pCTFMaterialArray[materialNum - 1]->fragmentShaderconstantsTable->SetVector(s_direct3dDevice, constantHandlerColor, &D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f));
				int i = 0;
				while (i < numberToDraw)
				{
					//set material
					D3DPERF_BeginEvent(0, L"Set Material per instance");
					result = s_direct3dDevice->SetPixelShader(pCTFMaterialArray[0]->pFragmentShader);
					assert(SUCCEEDED(result));
					D3DPERF_EndEvent();
					//set per view constance
					pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixViewToScreenHandler, &GetViewToScreenTransform());
					//lighting
					pCTFMaterialArray[0]->fragmentShaderconstantsTable->SetVector(s_direct3dDevice, constantHandlerDirectionLight, &directionLight);

					D3DPERF_BeginEvent(0, L"Draw Mesh");
					{
						// Set the shaders
						{
							pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixModelToWorldHandler, &GetTransform(D3DXVECTOR3(20.0*i,0.0,100.0),debugScale));
							pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixWorldToViewHandler, &GetWorldToViewTransform(cameraPosition));

							result = s_direct3dDevice->SetVertexShader(pCTFMaterialArray[0]->pVertexShader);
							assert(SUCCEEDED(result));
						}
						//set texture
						D3DPERF_BeginEvent(0, L"Set Material per view");
						{
							result = s_direct3dDevice->SetTexture(samplerRegister, pCTFMaterialArray[1]->pTexture);
							assert(SUCCEEDED(result));
							result = s_direct3dDevice->SetTexture(samplerRegister2, pCTFMaterialArray[1]->pTexture);
							assert(SUCCEEDED(result));
						}
						D3DPERF_EndEvent();
						// Bind a specific vertex buffer to the device as a data source
						{
							// There can be multiple streams of data feeding the display adaptor at the same time
							const unsigned int streamIndex = 0;
							// It's possible to start streaming data in the middle of a vertex buffer
							const unsigned int bufferOffset = 0;
							// The "stride" defines how large a single vertex is in the stream of data
							const unsigned int bufferStride = sizeof(sVertex);
							result = s_direct3dDevice->SetStreamSource(streamIndex, pDebugMeshArray[i]->pVertexBuffer, bufferOffset, bufferStride);
							assert(SUCCEEDED(result));
						}
						// Set the indices to use
						{
							HRESULT result = s_direct3dDevice->SetIndices(pDebugMeshArray[i]->pIndexBuffer);
							assert(SUCCEEDED(result));
						}
						// Render objects from the current streams
						{
							// We are using triangles as the "primitive" type,
							// and we have defined the vertex buffer as a triangle list
							// (meaning that every triangle is defined by three vertices)
							const D3DPRIMITIVETYPE primitiveType = D3DPT_TRIANGLELIST;
							// It's possible to start rendering primitives in the middle of the stream
							const unsigned int indexOfFirstVertexToRender = 0;
							const unsigned int indexOfFirstIndexToUse = 0;
							// EAE6320_TODO: How many vertices are in a rectangle,
							// and how many "primitives" (triangles) is it made up of?
							const unsigned int vertexCountToRender = pDebugMeshArray[i]->vertexNum;
							const unsigned int primitiveCountToRender = pDebugMeshArray[i]->indexNum / 3;
							HRESULT result = s_direct3dDevice->DrawIndexedPrimitive(primitiveType, indexOfFirstVertexToRender, indexOfFirstVertexToRender,
								vertexCountToRender, indexOfFirstIndexToUse, primitiveCountToRender);
							assert(SUCCEEDED(result));
						}
					}
					D3DPERF_EndEvent();

					i++;
				}
		
				{//draw debug line
					//// Set the position
					//{
					//	pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixModelToWorldHandler, &GetTransform(D3DXVECTOR3(-10.0, 0.0, 100.0),debugScale*10));
					//	pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixWorldToViewHandler, &GetWorldToViewTransform(cameraPosition));

					//	result = s_direct3dDevice->SetVertexShader(pCTFMaterialArray[0]->pVertexShader);
					//	assert(SUCCEEDED(result));
					//}
					//
					//// Bind a specific vertex buffer to the device as a data source
					//{
					//	// There can be multiple streams of data feeding the display adaptor at the same time
					//	const unsigned int streamIndex = 0;
					//	// It's possible to start streaming data in the middle of a vertex buffer
					//	const unsigned int bufferOffset = 0;
					//	// The "stride" defines how large a single vertex is in the stream of data
					//	const unsigned int bufferStride = sizeof(sVertex);
					//	result = s_direct3dDevice->SetStreamSource(streamIndex, s_debugLineVertexBuffer, bufferOffset, bufferStride);
					//	assert(SUCCEEDED(result));
					//}

					//// Render objects from the current streams
					//{
					//	// We are using triangles as the "primitive" type,
					//	// and we have defined the vertex buffer as a triangle list
					//	// (meaning that every triangle is defined by three vertices)
					//	const D3DPRIMITIVETYPE lineList = D3DPT_LINELIST;
					//	// It's possible to start rendering primitives in the middle of the stream
					//	const unsigned int indexOfFirstVertexToRender = 0;
					//	HRESULT result = s_direct3dDevice->DrawPrimitive(lineList, indexOfFirstVertexToRender, 1);
					//	assert(SUCCEEDED(result));
					//}

					//forward
					{
						sVertex* vertexData;
						{
							const unsigned int lockEntireBuffer = 0;
							const DWORD useDefaultLockingBehavior = 0;
							const HRESULT result = s_debugLineVertexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
								reinterpret_cast<void**>(&vertexData), useDefaultLockingBehavior);
						}
						vertexData[0].x = 0; vertexData[0].y = 0; vertexData[0].z = 0;
						vertexData[1].x = 0; vertexData[1].y =0; vertexData[1].z = 1;
						s_debugLineVertexBuffer->Unlock();

						// Set the position
						{
							pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixModelToWorldHandler, &GetTransform(pPlayer->forwardSeg.startPoint, pPlayer->pPlayerMesh->meshScale*0.5));
							pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixWorldToViewHandler, &GetWorldToViewTransform(cameraPosition));

							result = s_direct3dDevice->SetVertexShader(pCTFMaterialArray[0]->pVertexShader);
							assert(SUCCEEDED(result));
						}

						// Bind a specific vertex buffer to the device as a data source
						{
							// There can be multiple streams of data feeding the display adaptor at the same time
							const unsigned int streamIndex = 0;
							// It's possible to start streaming data in the middle of a vertex buffer
							const unsigned int bufferOffset = 0;
							// The "stride" defines how large a single vertex is in the stream of data
							const unsigned int bufferStride = sizeof(sVertex);
							result = s_direct3dDevice->SetStreamSource(streamIndex, s_debugLineVertexBuffer, bufferOffset, bufferStride);
							assert(SUCCEEDED(result));
						}

						// Render objects from the current streams
						{
							// We are using triangles as the "primitive" type,
							// and we have defined the vertex buffer as a triangle list
							// (meaning that every triangle is defined by three vertices)
							const D3DPRIMITIVETYPE lineList = D3DPT_LINELIST;
							// It's possible to start rendering primitives in the middle of the stream
							const unsigned int indexOfFirstVertexToRender = 0;
							HRESULT result = s_direct3dDevice->DrawPrimitive(lineList, indexOfFirstVertexToRender, 1);
							assert(SUCCEEDED(result));
						}

					}
					//left
					{
						sVertex* vertexData;
						{
							const unsigned int lockEntireBuffer = 0;
							const DWORD useDefaultLockingBehavior = 0;
							const HRESULT result = s_debugLineVertexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
								reinterpret_cast<void**>(&vertexData), useDefaultLockingBehavior);
						}
						vertexData[0].x = 0; vertexData[0].y = 0; vertexData[0].z = 0;
						vertexData[1].x = -1; vertexData[1].y = 0; vertexData[1].z = 0;
						s_debugLineVertexBuffer->Unlock();

						// Set the position
						{
							pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixModelToWorldHandler, &GetTransform(pPlayer->leftSeg.startPoint, pPlayer->pPlayerMesh->meshScale*0.5));
							pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixWorldToViewHandler, &GetWorldToViewTransform(cameraPosition));

							result = s_direct3dDevice->SetVertexShader(pCTFMaterialArray[0]->pVertexShader);
							assert(SUCCEEDED(result));
						}

						// Bind a specific vertex buffer to the device as a data source
						{
							// There can be multiple streams of data feeding the display adaptor at the same time
							const unsigned int streamIndex = 0;
							// It's possible to start streaming data in the middle of a vertex buffer
							const unsigned int bufferOffset = 0;
							// The "stride" defines how large a single vertex is in the stream of data
							const unsigned int bufferStride = sizeof(sVertex);
							result = s_direct3dDevice->SetStreamSource(streamIndex, s_debugLineVertexBuffer, bufferOffset, bufferStride);
							assert(SUCCEEDED(result));
						}

						// Render objects from the current streams
						{
							// We are using triangles as the "primitive" type,
							// and we have defined the vertex buffer as a triangle list
							// (meaning that every triangle is defined by three vertices)
							const D3DPRIMITIVETYPE lineList = D3DPT_LINELIST;
							// It's possible to start rendering primitives in the middle of the stream
							const unsigned int indexOfFirstVertexToRender = 0;
							HRESULT result = s_direct3dDevice->DrawPrimitive(lineList, indexOfFirstVertexToRender, 1);
							assert(SUCCEEDED(result));
						}

					}

					//down
					{
						sVertex* vertexData;
						{
							const unsigned int lockEntireBuffer = 0;
							const DWORD useDefaultLockingBehavior = 0;
							const HRESULT result = s_debugLineVertexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
								reinterpret_cast<void**>(&vertexData), useDefaultLockingBehavior);
						}
						vertexData[0].x = 0; vertexData[0].y = 0; vertexData[0].z = 0;
						vertexData[1].x = 0; vertexData[1].y = -1; vertexData[1].z = 0;
						s_debugLineVertexBuffer->Unlock();

						// Set the position
						{
							pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixModelToWorldHandler, &GetTransform(pPlayer->leftSeg.startPoint, pPlayer->pPlayerMesh->meshScale*0.5));
							pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixWorldToViewHandler, &GetWorldToViewTransform(cameraPosition));

							result = s_direct3dDevice->SetVertexShader(pCTFMaterialArray[0]->pVertexShader);
							assert(SUCCEEDED(result));
						}

						// Bind a specific vertex buffer to the device as a data source
						{
							// There can be multiple streams of data feeding the display adaptor at the same time
							const unsigned int streamIndex = 0;
							// It's possible to start streaming data in the middle of a vertex buffer
							const unsigned int bufferOffset = 0;
							// The "stride" defines how large a single vertex is in the stream of data
							const unsigned int bufferStride = sizeof(sVertex);
							result = s_direct3dDevice->SetStreamSource(streamIndex, s_debugLineVertexBuffer, bufferOffset, bufferStride);
							assert(SUCCEEDED(result));
						}

						// Render objects from the current streams
						{
							// We are using triangles as the "primitive" type,
							// and we have defined the vertex buffer as a triangle list
							// (meaning that every triangle is defined by three vertices)
							const D3DPRIMITIVETYPE lineList = D3DPT_LINELIST;
							// It's possible to start rendering primitives in the middle of the stream
							const unsigned int indexOfFirstVertexToRender = 0;
							HRESULT result = s_direct3dDevice->DrawPrimitive(lineList, indexOfFirstVertexToRender, 1);
							assert(SUCCEEDED(result));
						}

					}
	
					//backward
					{
						sVertex* vertexData;
						{
							const unsigned int lockEntireBuffer = 0;
							const DWORD useDefaultLockingBehavior = 0;
							const HRESULT result = s_debugLineVertexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
								reinterpret_cast<void**>(&vertexData), useDefaultLockingBehavior);
						}
						vertexData[0].x = 0; vertexData[0].y = 0; vertexData[0].z = 0;
						vertexData[1].x = 0; vertexData[1].y = 0; vertexData[1].z = -1;
						s_debugLineVertexBuffer->Unlock();

						// Set the position
						{
							pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixModelToWorldHandler, &GetTransform(pPlayer->forwardSeg.startPoint, pPlayer->pPlayerMesh->meshScale*0.5));
							pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixWorldToViewHandler, &GetWorldToViewTransform(cameraPosition));

							result = s_direct3dDevice->SetVertexShader(pCTFMaterialArray[0]->pVertexShader);
							assert(SUCCEEDED(result));
						}

						// Bind a specific vertex buffer to the device as a data source
						{
							// There can be multiple streams of data feeding the display adaptor at the same time
							const unsigned int streamIndex = 0;
							// It's possible to start streaming data in the middle of a vertex buffer
							const unsigned int bufferOffset = 0;
							// The "stride" defines how large a single vertex is in the stream of data
							const unsigned int bufferStride = sizeof(sVertex);
							result = s_direct3dDevice->SetStreamSource(streamIndex, s_debugLineVertexBuffer, bufferOffset, bufferStride);
							assert(SUCCEEDED(result));
						}

						// Render objects from the current streams
						{
							// We are using triangles as the "primitive" type,
							// and we have defined the vertex buffer as a triangle list
							// (meaning that every triangle is defined by three vertices)
							const D3DPRIMITIVETYPE lineList = D3DPT_LINELIST;
							// It's possible to start rendering primitives in the middle of the stream
							const unsigned int indexOfFirstVertexToRender = 0;
							HRESULT result = s_direct3dDevice->DrawPrimitive(lineList, indexOfFirstVertexToRender, 1);
							assert(SUCCEEDED(result));
						}

					}
					//right
					{
						sVertex* vertexData;
						{
							const unsigned int lockEntireBuffer = 0;
							const DWORD useDefaultLockingBehavior = 0;
							const HRESULT result = s_debugLineVertexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
								reinterpret_cast<void**>(&vertexData), useDefaultLockingBehavior);
						}
						vertexData[0].x = 0; vertexData[0].y = 0; vertexData[0].z = 0;
						vertexData[1].x = 1; vertexData[1].y = 0; vertexData[1].z = 0;
						s_debugLineVertexBuffer->Unlock();

						// Set the position
						{
							pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixModelToWorldHandler, &GetTransform(pPlayer->leftSeg.startPoint, pPlayer->pPlayerMesh->meshScale*0.5));
							pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixWorldToViewHandler, &GetWorldToViewTransform(cameraPosition));

							result = s_direct3dDevice->SetVertexShader(pCTFMaterialArray[0]->pVertexShader);
							assert(SUCCEEDED(result));
						}

						// Bind a specific vertex buffer to the device as a data source
						{
							// There can be multiple streams of data feeding the display adaptor at the same time
							const unsigned int streamIndex = 0;
							// It's possible to start streaming data in the middle of a vertex buffer
							const unsigned int bufferOffset = 0;
							// The "stride" defines how large a single vertex is in the stream of data
							const unsigned int bufferStride = sizeof(sVertex);
							result = s_direct3dDevice->SetStreamSource(streamIndex, s_debugLineVertexBuffer, bufferOffset, bufferStride);
							assert(SUCCEEDED(result));
						}

						// Render objects from the current streams
						{
							// We are using triangles as the "primitive" type,
							// and we have defined the vertex buffer as a triangle list
							// (meaning that every triangle is defined by three vertices)
							const D3DPRIMITIVETYPE lineList = D3DPT_LINELIST;
							// It's possible to start rendering primitives in the middle of the stream
							const unsigned int indexOfFirstVertexToRender = 0;
							HRESULT result = s_direct3dDevice->DrawPrimitive(lineList, indexOfFirstVertexToRender, 1);
							assert(SUCCEEDED(result));
						}

					}

					//up
					{
						sVertex* vertexData;
						{
							const unsigned int lockEntireBuffer = 0;
							const DWORD useDefaultLockingBehavior = 0;
							const HRESULT result = s_debugLineVertexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
								reinterpret_cast<void**>(&vertexData), useDefaultLockingBehavior);
						}
						vertexData[0].x = 0; vertexData[0].y = 0; vertexData[0].z = 0;
						vertexData[1].x = 0; vertexData[1].y = 1; vertexData[1].z = 0;
						s_debugLineVertexBuffer->Unlock();

						// Set the position
						{
							pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixModelToWorldHandler, &GetTransform(pPlayer->leftSeg.startPoint, pPlayer->pPlayerMesh->meshScale*0.5));
							pCTFMaterialArray[0]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixWorldToViewHandler, &GetWorldToViewTransform(cameraPosition));

							result = s_direct3dDevice->SetVertexShader(pCTFMaterialArray[0]->pVertexShader);
							assert(SUCCEEDED(result));
						}

						// Bind a specific vertex buffer to the device as a data source
						{
							// There can be multiple streams of data feeding the display adaptor at the same time
							const unsigned int streamIndex = 0;
							// It's possible to start streaming data in the middle of a vertex buffer
							const unsigned int bufferOffset = 0;
							// The "stride" defines how large a single vertex is in the stream of data
							const unsigned int bufferStride = sizeof(sVertex);
							result = s_direct3dDevice->SetStreamSource(streamIndex, s_debugLineVertexBuffer, bufferOffset, bufferStride);
							assert(SUCCEEDED(result));
						}

						// Render objects from the current streams
						{
							// We are using triangles as the "primitive" type,
							// and we have defined the vertex buffer as a triangle list
							// (meaning that every triangle is defined by three vertices)
							const D3DPRIMITIVETYPE lineList = D3DPT_LINELIST;
							// It's possible to start rendering primitives in the middle of the stream
							const unsigned int indexOfFirstVertexToRender = 0;
							HRESULT result = s_direct3dDevice->DrawPrimitive(lineList, indexOfFirstVertexToRender, 1);
							assert(SUCCEEDED(result));
						}

					}

				}

				D3DPERF_EndEvent();
			}
#endif
		}
		result = s_direct3dDevice->EndScene();
		assert( SUCCEEDED( result ) );
	}

	// Everything has been drawn to the "back buffer", which is just an image in memory.
	// In order to display it, the contents of the back buffer must be "presented"
	// (to the front buffer)
	{
		const RECT* noSourceRectangle = NULL;
		const RECT* noDestinationRectangle = NULL;
		const HWND useDefaultWindow = NULL;
		const RGNDATA* noDirtyRegion = NULL;
		HRESULT result = s_direct3dDevice->Present( noSourceRectangle, noDestinationRectangle, useDefaultWindow, noDirtyRegion );
		assert( SUCCEEDED( result ) );
	}
	
	//time
	eae6320::Time::OnNewFrame();
}

bool ShutDown()
{
	bool wereThereErrors = false;

	RakNet::RakPeerInterface::DestroyInstance(peer);

	if ( s_direct3dInterface )
	{
		if ( s_direct3dDevice )
		{
			if ( s_vertexDeclaration )
			{
				s_direct3dDevice->SetVertexDeclaration( NULL );
				s_vertexDeclaration->Release();
				s_vertexDeclaration = NULL;
			}
			int i = 0;
			while (i < materialNum)
			{
				if (pCTFMaterialArray[i])
				{
					delete pCTFMaterialArray[i];
					pCTFMaterialArray[i] = NULL;
				}
				if (pCTFMeshArray[i])
				{
					delete pCTFMeshArray[i];
					pCTFMeshArray[i] = NULL;
				}
				i++;
			}
			
			if (pBackgroundSource)
			{
				pBackgroundSource->DestroyVoice();
				pBackgroundSource = NULL;
			}
			if (pSoundEffectSource)
			{
				pSoundEffectSource->DestroyVoice();
				pSoundEffectSource = NULL;
			}
			if (pWalkSource)
			{
				pWalkSource->DestroyVoice();
				pWalkSource = NULL;
			}
			if (pMasterVoice)
			{
				pMasterVoice->DestroyVoice();
				pMasterVoice = NULL;
			}
			if (pXAudio2){
				pXAudio2->Release();
				pXAudio2 = NULL;
			}

#ifdef _DEBUG_MENU
			i = 0;
			while (i < 2)
			{
				if (pDebugMeshArray[i])
				{
					delete pDebugMeshArray[i];
					pDebugMeshArray[i] = NULL;
				}
				i++;
			}
			if (s_debugLineVertexBuffer)
			{
				s_debugLineVertexBuffer->Release();
				s_debugLineVertexBuffer = NULL;
			}
			if (pFlagCapText)
			{
				delete pFlagCapText;
				pFlagCapText = NULL;
			}
			if (pDebugMenu)
			{
				delete pDebugMenu;
				pDebugMenu = NULL;
			}
			if (pOctreeCollisdeTriangleVertexBuffer)
			{
				pOctreeCollisdeTriangleVertexBuffer->Release();
				pOctreeCollisdeTriangleVertexBuffer = NULL;
			}
			if (pOctreeCollisdeTriangleIndexBuffer)
			{
				pOctreeCollisdeTriangleIndexBuffer->Release();
				pOctreeCollisdeTriangleIndexBuffer = NULL;
			}
#endif
			s_direct3dDevice->Release();
			s_direct3dDevice = NULL;
		}

		s_direct3dInterface->Release();
		s_direct3dInterface = NULL;
	}
	s_mainWindow = NULL;

	return !wereThereErrors;
}

// Helper Function Definitions
//============================

namespace
{
	bool CreateDevice( const HWND i_mainWindow )
	{
		const UINT useDefaultDevice = D3DADAPTER_DEFAULT;
		const D3DDEVTYPE useHardwareRendering = D3DDEVTYPE_HAL;
		const DWORD useHardwareVertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
		D3DPRESENT_PARAMETERS presentationParameters = { 0 };
		{
			//jinghui's way to solve full screen problem
			RECT desktop;
			// Get a handle to the desktop window
			const HWND hDesktop = GetDesktopWindow();
			// Get the size of screen to the variable desktop
			GetWindowRect(hDesktop, &desktop);
			int screenWidth = desktop.right;
			int screenHeight = desktop.bottom;

			presentationParameters.BackBufferWidth = screenWidth;
			presentationParameters.BackBufferHeight = screenHeight;
			presentationParameters.BackBufferFormat = D3DFMT_X8R8G8B8;
			presentationParameters.BackBufferCount = 1;
			presentationParameters.MultiSampleType = D3DMULTISAMPLE_NONE;
			presentationParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
			presentationParameters.hDeviceWindow = i_mainWindow;
			presentationParameters.Windowed = !g_shouldRenderFullScreen ;
			presentationParameters.EnableAutoDepthStencil = TRUE;
			presentationParameters.AutoDepthStencilFormat = D3DFMT_D16;
			presentationParameters.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
		}
		HRESULT result = s_direct3dInterface->CreateDevice( useDefaultDevice, useHardwareRendering,
			i_mainWindow, useHardwareVertexProcessing, &presentationParameters, &s_direct3dDevice );
		if ( SUCCEEDED( result ) )
		{
			for (DWORD i = 0; i < 8; ++i)
			{
				s_direct3dDevice->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
				s_direct3dDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
				s_direct3dDevice->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
			}
			return true;
		}
		else
		{
			MessageBox( i_mainWindow, "DirectX failed to create a Direct3D9 device", "No D3D9 Device", MB_OK | MB_ICONERROR );
			return false;
		}
	}

	bool CreateInterface( const HWND i_mainWindow )
	{

		// D3D_SDK_VERSION is #defined by the Direct3D header files,
		// and is just a way to make sure that everything is up-to-date
		s_direct3dInterface = Direct3DCreate9( D3D_SDK_VERSION );
		if ( s_direct3dInterface )
		{
			return true;
		}
		else
		{
			MessageBox( i_mainWindow, "DirectX failed to create a Direct3D9 interface", "No D3D9 Interface", MB_OK | MB_ICONERROR );
			return false;
		}
	}

	//helper function of Create Audio
	HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD & dwChunkSize, DWORD & dwChunkDataPosition)
	{
		HRESULT hr = S_OK;
		if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
			return HRESULT_FROM_WIN32(GetLastError());

		DWORD dwChunkType;
		DWORD dwChunkDataSize;
		DWORD dwRIFFDataSize = 0;
		DWORD dwFileType;
		DWORD bytesRead = 0;
		DWORD dwOffset = 0;

		while (hr == S_OK)
		{
			DWORD dwRead;
			if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
				hr = HRESULT_FROM_WIN32(GetLastError());

			if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
				hr = HRESULT_FROM_WIN32(GetLastError());

			switch (dwChunkType)
			{
			case 'FFIR':
				dwRIFFDataSize = dwChunkDataSize;
				dwChunkDataSize = 4;
				if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
					hr = HRESULT_FROM_WIN32(GetLastError());
				break;

			default:
				if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
					return HRESULT_FROM_WIN32(GetLastError());
			}

			dwOffset += sizeof(DWORD)* 2;

			if (dwChunkType == fourcc)
			{
				dwChunkSize = dwChunkDataSize;
				dwChunkDataPosition = dwOffset;
				return S_OK;
			}

			dwOffset += dwChunkDataSize;

			if (bytesRead >= dwRIFFDataSize) return S_FALSE;

		}

		return S_OK;

	}
	HRESULT ReadChunkData(HANDLE hFile, void * buffer, DWORD buffersize, DWORD bufferoffset)
	{
		HRESULT hr = S_OK;
		if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
			return HRESULT_FROM_WIN32(GetLastError());
		DWORD dwRead;
		if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}
	bool PlayWave(char* i_audioSoucePath, IXAudio2SourceVoice* &pSouce)
	{
		if (pSouce)
		{
			XAUDIO2_VOICE_STATE state;
			pSouce->GetState(&state);
			if (state.BuffersQueued > 0)
			{
				pSouce->Start(0);
				return false;
			}

			pSouce->DestroyVoice();
			pSouce = NULL;
		}
		WAVEFORMATEXTENSIBLE waveFormatX = { 0 };
		XAUDIO2_BUFFER waveBuffer = { 0 };

		HRESULT result;

		HANDLE hFile = CreateFile(
			i_audioSoucePath,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);

		if (INVALID_HANDLE_VALUE == hFile)
			return HRESULT_FROM_WIN32(GetLastError());

		if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
			return HRESULT_FROM_WIN32(GetLastError());

		DWORD dwChunkSize;
		DWORD dwChunkPosition;
		//check the file type, should be fourccWAVE or 'XWMA'
		FindChunk(hFile, 'FFIR', dwChunkSize, dwChunkPosition);
		DWORD filetype;
		ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
		if (filetype != 'EVAW')
			return false;

		//Locate the 'fmt ' chunk, and copy its contents into a WAVEFORMATEXTENSIBLE structure.
		FindChunk(hFile, ' tmf', dwChunkSize, dwChunkPosition);
		ReadChunkData(hFile, &waveFormatX, dwChunkSize, dwChunkPosition);

		//fill out the audio data buffer with the contents of the fourccDATA chunk
		FindChunk(hFile, 'atad', dwChunkSize, dwChunkPosition);
		BYTE * pDataBuffer = new BYTE[dwChunkSize];
		ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);

		waveBuffer.AudioBytes = dwChunkSize;  //buffer containing audio data
		waveBuffer.pAudioData = pDataBuffer;  //size of the audio buffer in bytes
		waveBuffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

		// Play the wave using a source voice that sends to both the submix and mastering voices
		//
		XAUDIO2_SEND_DESCRIPTOR sendDescriptors[2];
		sendDescriptors[0].Flags = XAUDIO2_SEND_USEFILTER; // LPF direct-path
		sendDescriptors[0].pOutputVoice = pMasterVoice;
		sendDescriptors[1].Flags = XAUDIO2_SEND_USEFILTER; // LPF reverb-path -- omit for better performance at the cost of less realistic occlusion
		sendDescriptors[1].pOutputVoice = pSubmixVoice;
		const XAUDIO2_VOICE_SENDS sendList = { 2, sendDescriptors };

		if (FAILED(result = pXAudio2->CreateSourceVoice(&pSouce, (WAVEFORMATEX*)&waveFormatX,0,2.0f,NULL,&sendList)))
			return false;

		if (FAILED(result = pSouce->SubmitSourceBuffer(&waveBuffer)))
			return false;

		
		pSouce->Start(0);

		return true;
	}
	bool CreateAudio()
	{
		
		HRESULT result;
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (FAILED(result = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
			return false;
		if (FAILED(result = pXAudio2->CreateMasteringVoice(&pMasterVoice,XAUDIO2_DEFAULT_CHANNELS)))
			return false;

		//create 3d audio
		//DWORD dwChannelMask;
		
		if (FAILED(result = pXAudio2->GetDeviceDetails(0, &deviceDetails)))
		{
			return false;
		}
		if (deviceDetails.OutputFormat.Format.nChannels > OUTPUTCHANNELS)
		{
			return false;
		}
		//create reverb effect
		if (FAILED(result = XAudio2CreateReverb(&pReverbEffect, 0)))
		{
			return false;
		}
		//create submix voice
		XAUDIO2_EFFECT_DESCRIPTOR effects[] = { { pReverbEffect, TRUE, 1 } };
		XAUDIO2_EFFECT_CHAIN effectChain = { 1, effects };

		if (FAILED(result =pXAudio2->CreateSubmixVoice(&pSubmixVoice, 1,
			deviceDetails.OutputFormat.Format.nSamplesPerSec, 0, 0,
			nullptr, &effectChain)))
		{
			return false;
		}

		//initialize x3daudio
		X3DAudioInitialize(deviceDetails.OutputFormat.dwChannelMask, X3DAUDIO_SPEED_OF_SOUND, X3DInstance);

		{
			
			Listener.Position.x = 0;
			Listener.Position.y = 0;
			Listener.Position.z = 0;

			Listener.OrientFront.x = 0;
			Listener.OrientFront.y = 0;
			Listener.OrientTop.x = 0;
			Listener.OrientTop.z = 0.f;

			Listener.OrientFront.z = 0;
			Listener.OrientTop.y = 1.f;

			Listener.pCone=(X3DAUDIO_CONE*)&Listener_DirectionalCone;

			Emitter.pCone = (X3DAUDIO_CONE*)&Emitter_DirectionalCone;

			Emitter.Position.x = 0;
			Emitter.Position.y = 0;
			Emitter.Position.z = 0;

			Emitter.OrientFront.x = 0;
			Emitter.OrientFront.y = 0;
			Emitter.OrientTop.x = 0;
			Emitter.OrientTop.z = 0.f;

			Emitter.OrientFront.z = 0;
			Emitter.OrientTop.y = 1.f;

			Emitter.ChannelCount = INPUTCHANNELS;
			Emitter.ChannelRadius = 1.0f;
			Emitter.pChannelAzimuths = new FLOAT32[8];
			// Use of Inner radius allows for smoother transitions as
			// a sound travels directly through, above, or below the listener.
			// It also may be used to give elevation cues.
			Emitter.InnerRadius = 2.0f;
			Emitter.InnerRadiusAngle = X3DAUDIO_PI / 4.0f;;

			Emitter.pVolumeCurve = (X3DAUDIO_DISTANCE_CURVE*)&X3DAudioDefault_LinearCurve;
			Emitter.pLFECurve = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_LFE_Curve;
			Emitter.pLPFDirectCurve = nullptr; // use default curve
			Emitter.pLPFReverbCurve = nullptr; // use default curve
			Emitter.pReverbCurve = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_Reverb_Curve;
			Emitter.CurveDistanceScaler = 500.0f;
			Emitter.DopplerScaler = 1.0f;
			//Emitter.CurveDistanceScaler = FLT_MIN;
		}

		FLOAT32 * matrix = new FLOAT32[deviceDetails.OutputFormat.Format.nChannels];
		DSPSettings.SrcChannelCount = INPUTCHANNELS;
		DSPSettings.DstChannelCount = deviceDetails.OutputFormat.Format.nChannels;
		DSPSettings.pMatrixCoefficients = matrix;

		PlayWave(backgroundMusicPath, pBackgroundSource);
		pBackgroundSource->SetVolume(backgroundMusicVolume / 20);
		return true;
	}
	

	bool CreateCollisionInfo()
	{
		if (!LoadBinaryCollisionFile(octreeCollisionDataPath,pBinaryFileEntry))
			return false;

		//load binary file data

		//first load triangle data
		{
			unsigned int triangleVerticesNum;
			memcpy(&triangleVerticesNum, pBinaryFileEntry + binaryFileIndex, sizeof(unsigned int));
			binaryFileIndex += sizeof(unsigned int);
			unsigned int triangleIndicesNum;
			memcpy(&triangleIndicesNum, pBinaryFileEntry + binaryFileIndex, sizeof(unsigned int));
			binaryFileIndex += sizeof(unsigned int);

			for (unsigned int i = 0; i < triangleVerticesNum; i++)
			{
				float triangleVertex;
				memcpy(&triangleVertex, pBinaryFileEntry + binaryFileIndex, sizeof(float));
				binaryFileIndex += sizeof(float);
				octreeCollisionTriangleVertices.push_back(triangleVertex);
			}
			for (unsigned int i = 0; i < triangleIndicesNum; i++)
			{
				unsigned int triangleIndex;
				memcpy(&triangleIndex, pBinaryFileEntry + binaryFileIndex, sizeof(unsigned int));
				binaryFileIndex += sizeof(unsigned int);
				octreeCollisionTriangleIndices.push_back(triangleIndex);
			}
		}
		//next load octree data
		{
			//put data in array
			pSceneOctree = new Player::OctreeNode;
			for (unsigned int i = 0; i < 8; i++)
			{
				pSceneOctree->children[i] = NULL;
			}
			//rebuild the octree according to the data
			//first get the octree max depth
			unsigned int octreeMaxDepth;
			memcpy(&octreeMaxDepth, pBinaryFileEntry + binaryFileIndex, sizeof(unsigned int));
			binaryFileIndex += sizeof(unsigned int);

			while (binaryFileIndex < fileLength)
			{
				//get the info for octree
				Player::OctreeNode* pCurrentOctreeNode = pSceneOctree;
				{
					//node track
					//go to the right node and put the info there
					unsigned int nodeBranch;
					unsigned int depthCounter = 0;
					while (depthCounter < octreeMaxDepth)
					{
						memcpy(&nodeBranch, pBinaryFileEntry + binaryFileIndex, sizeof(unsigned int));
						binaryFileIndex += sizeof(unsigned int);
						depthCounter++;
						if (nodeBranch != 8)
						{
							if (pCurrentOctreeNode->children[nodeBranch] == NULL)
							{
								pCurrentOctreeNode->children[nodeBranch] = new Player::OctreeNode;
								for (unsigned int i = 0; i < 8; i++)
								{
									pCurrentOctreeNode->children[nodeBranch]->children[i] = NULL;
								}
							}
							pCurrentOctreeNode = pCurrentOctreeNode->children[nodeBranch];
						}
					}

					//now that the pointer point to the right octree node, add info to it!
					//iscollision
					memcpy(&(pCurrentOctreeNode->isCollision), pBinaryFileEntry + binaryFileIndex, sizeof(bool));
					binaryFileIndex += sizeof(bool);
					//depth
					memcpy(&(pCurrentOctreeNode->depth), pBinaryFileEntry + binaryFileIndex, sizeof(unsigned int));
					binaryFileIndex += sizeof(unsigned int);//jump depth
					//halfwidth
					memcpy(&(pCurrentOctreeNode->halfWidth), pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += sizeof(float);
					//center
					memcpy(&(pCurrentOctreeNode->centerX), pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += sizeof(float);
					memcpy(&(pCurrentOctreeNode->centerY), pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += sizeof(float);
					memcpy(&(pCurrentOctreeNode->centerZ), pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += sizeof(float);
					//triangle number
					unsigned int triangleNum;
					memcpy(&(triangleNum), pBinaryFileEntry + binaryFileIndex, sizeof(unsigned int));
					binaryFileIndex += sizeof(unsigned int);
					//get triangle list
					for (unsigned int i = 0; i < triangleNum; i++)
					{
						unsigned int currentTriangleIndex;
						memcpy(&currentTriangleIndex, pBinaryFileEntry + binaryFileIndex, sizeof(unsigned int));
						binaryFileIndex += sizeof(unsigned int);
						pCurrentOctreeNode->triangleList.push_back(currentTriangleIndex);
					}
				}

			}
		}
#ifdef _DEBUG_MENU
		CreateOctreeCollideTriangleVertexBuffer();
#endif
		return true;
	}

	bool CreateMesh()
	{
		arcBall = new ArcBall::cArcBall();
		pCTFMeshArray[0] = new Mesh::cMesh(braceMeshPath, s_direct3dDevice);
		pCTFMeshArray[1] = new Mesh::cMesh(cementWallMeshPath, s_direct3dDevice);
		pCTFMeshArray[2] = new Mesh::cMesh(wallMeshPath, s_direct3dDevice);
		pCTFMeshArray[3] = new Mesh::cMesh(railingMeshPath, s_direct3dDevice);
		pCTFMeshArray[4] = new Mesh::cMesh(otherObjectMeshPath, s_direct3dDevice);
		pCTFMeshArray[5] = new Mesh::cMesh(floorMeshPath, s_direct3dDevice);
		pCTFMeshArray[6] = new Mesh::cMesh(playerMeshPath, s_direct3dDevice);
		pCTFMeshArray[7] = new Mesh::cMesh(flagMeshPath, s_direct3dDevice);
		pCTFMeshArray[8] = new Mesh::cMesh(flagMeshPath, s_direct3dDevice);
		pCTFMeshArray[9] = new Mesh::cMesh(dogMeshPath, s_direct3dDevice);
		pCTFMeshArray[10] = new Mesh::cMesh(collisionMeshPath, s_direct3dDevice);

		pCTFMeshArray[7]->meshPosition = D3DXVECTOR3(-10.5444, 648.169, 34.9844) + D3DXVECTOR3(1500, 200, 1500);
		pCTFMeshArray[8]->meshPosition = D3DXVECTOR3(-10.5444, 648.169, 34.9844) + D3DXVECTOR3(-1500, 200, -1500);
		D3DXMatrixRotationY(&pCTFMeshArray[8]->meshRotationMatrix, 3.1415);

		//dog mesh
		pCTFMeshArray[9]->meshScale = D3DXVECTOR3(0.5, 0.5, 0.5);
		pCTFMeshArray[9]->meshPosition = D3DXVECTOR3(0, -200, -850);
		//cat mesh
		pCTFMeshArray[6]->meshScale = D3DXVECTOR3(50, 50, 50);
		pCTFMeshArray[6]->meshPosition = D3DXVECTOR3(0, -200, 850);

		if (isServer)
			pPlayer = new Player::cPlayer(pCTFMeshArray[6]);
		else pPlayer = new Player::cPlayer(pCTFMeshArray[9]);

		D3DXMatrixRotationY(&pPlayer->pPlayerMesh->meshRotationMatrix, playerRotValue);

		pPlayer->UpdateCollisionSegment(pPlayer->pPlayerMesh->meshPosition, D3DXVECTOR3(50, 50, 50));

		// init camera info
		D3DXMatrixIdentity(&cameraRotation);
		return true;
	}
	bool CreateMaterial()
	{
		pCTFMaterialArray[0] = new Material::cMaterial(braceMatPath, s_direct3dDevice, &s_mainWindow);
		pCTFMaterialArray[1] = new Material::cMaterial(cementWallMatPath, s_direct3dDevice, &s_mainWindow);
		pCTFMaterialArray[2] = new Material::cMaterial(wallMatPath, s_direct3dDevice, &s_mainWindow);
		pCTFMaterialArray[3] = new Material::cMaterial(railingMatPath, s_direct3dDevice, &s_mainWindow);
		pCTFMaterialArray[4] = new Material::cMaterial(otherObjectMatPath, s_direct3dDevice, &s_mainWindow);
		pCTFMaterialArray[5] = new Material::cMaterial(floorMatPath, s_direct3dDevice, &s_mainWindow);
		pCTFMaterialArray[6] = new Material::cMaterial(playerMatPath, s_direct3dDevice, &s_mainWindow);
		pCTFMaterialArray[7] = new Material::cMaterial(redMatPath, s_direct3dDevice, &s_mainWindow);
		pCTFMaterialArray[8] = new Material::cMaterial(blueMatPath, s_direct3dDevice, &s_mainWindow);
		pCTFMaterialArray[9] = new Material::cMaterial(dogMatPath, s_direct3dDevice, &s_mainWindow);
		pCTFMaterialArray[10] = new Material::cMaterial(collisionMatPath, s_direct3dDevice, &s_mainWindow);


		for (int i = 0; i < materialNum; i++)
		{
			//set constant value for vertex shader
			matrixModelToWorldHandler = pCTFMaterialArray[i]->vertexShaderConstantsTable->GetConstantByName(NULL, "g_transform_modelToWorld");
			matrixWorldToViewHandler = pCTFMaterialArray[i]->vertexShaderConstantsTable->GetConstantByName(NULL, "g_transform_worldToView");
			matrixViewToScreenHandler = pCTFMaterialArray[i]->vertexShaderConstantsTable->GetConstantByName(NULL, "g_transform_viewToScreen");

			pCTFMaterialArray[i]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixModelToWorldHandler, &GetTransform(mapModelPos));
			pCTFMaterialArray[i]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixWorldToViewHandler, &GetWorldToViewTransform(cameraPosition));
			//pCTFMaterialArray[i]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixViewToScreenHandler, &GetViewToScreenTransform());


			if (!pCTFMaterialArray[i]->useDefualtColor)
			{
				//set constant value for fragment shader
				D3DXHANDLE handlerColor = pCTFMaterialArray[i]->fragmentShaderconstantsTable->GetConstantByName(NULL, "g_colorModifier");
				constantHandlerAmbientLight = pCTFMaterialArray[i]->fragmentShaderconstantsTable->GetConstantByName(NULL, "g_light_ambient");
				constantHandlerDirectionLight = pCTFMaterialArray[i]->fragmentShaderconstantsTable->GetConstantByName(NULL, "g_light_direction");
				constantHandlerDirectionLightColor = pCTFMaterialArray[i]->fragmentShaderconstantsTable->GetConstantByName(NULL, "g_light_direction_color");


				const D3DXVECTOR4 pColor(pCTFMaterialArray[i]->pColorModifier->x, pCTFMaterialArray[i]->pColorModifier->y, pCTFMaterialArray[i]->pColorModifier->z, 1.0f);
				pCTFMaterialArray[i]->fragmentShaderconstantsTable->SetVector(s_direct3dDevice, handlerColor, &pColor);

				pCTFMaterialArray[i]->fragmentShaderconstantsTable->SetVector(s_direct3dDevice, constantHandlerAmbientLight, &ambientLight);

				pCTFMaterialArray[i]->fragmentShaderconstantsTable->SetVector(s_direct3dDevice, constantHandlerDirectionLight, &directionLight);

				pCTFMaterialArray[i]->fragmentShaderconstantsTable->SetVector(s_direct3dDevice, constantHandlerDirectionLightColor, &directionLightColor);


				samplerHandle = pCTFMaterialArray[i]->fragmentShaderconstantsTable->GetConstantByName(NULL, "g_color_sampler");
				if (samplerHandle != NULL)
				{
					samplerRegister = static_cast<DWORD>(pCTFMaterialArray[i]->fragmentShaderconstantsTable->GetSamplerIndex(samplerHandle));
				}
				samplerHandle = pCTFMaterialArray[i]->fragmentShaderconstantsTable->GetConstantByName(NULL, "g_color_sampler2");
				if (samplerHandle != NULL)
				{
					samplerRegister2 = static_cast<DWORD>(pCTFMaterialArray[i]->fragmentShaderconstantsTable->GetSamplerIndex(samplerHandle));
				}
				HRESULT setTexResult = s_direct3dDevice->SetTexture(samplerRegister, pCTFMaterialArray[i]->pTexture);
				{
					if (FAILED(setTexResult))
						return false;
				}
			}
		}
		constantHandlerColor = pCTFMaterialArray[materialNum - 1]->fragmentShaderconstantsTable->GetConstantByName(NULL, "g_colorModifier");
		pCTFMaterialArray[materialNum - 1]->fragmentShaderconstantsTable->SetVector(s_direct3dDevice, constantHandlerColor, &D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f));

		return true;
	}
	

	bool CreateVertexDeclaration()
	{
		// Initialize the vertex format
		HRESULT result = s_direct3dDevice->CreateVertexDeclaration(s_vertexElements, &s_vertexDeclaration);
		if (SUCCEEDED(result))
		{
			result = s_direct3dDevice->SetVertexDeclaration(s_vertexDeclaration);
			if (FAILED(result))
			{
				MessageBox(s_mainWindow, "DirectX failed to set the vertex declaration", "No Vertex Declaration", MB_OK | MB_ICONERROR);
				return false;
			}
		}
		else
		{
			MessageBox(s_mainWindow, "DirectX failed to create a Direct3D9 vertex declaration", "No Vertex Declaration", MB_OK | MB_ICONERROR);
			return false;
		}
		return true;
	}

	bool CreateFlagCaptureData()
	{
		pFlagCapText = new DebugMenu::cDebugMenu(s_direct3dDevice);
		SetRect(&pFlagCapText->debugItems[0].rect, 0, 1000, 100, 200);
		SetRect(&pFlagCapText->debugItems[1].rect, 0, 0, 100, 200);
		SetRect(&pFlagCapText->debugItems[2].rect, 1500, 0, 100, 200);
		return true;
	}
	bool ServerClientChooseScreen()
	{
		{
			const D3DRECT* subRectanglesToClear = NULL;
			const DWORD subRectangleCount = 0;
			const DWORD clearColorAndDepth = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
			const D3DCOLOR colorToClear = D3DCOLOR_XRGB(0, 0, 0);
			const float depthToClear = 1.0f;
			const DWORD noStencilBuffer = 0;
			HRESULT result = s_direct3dDevice->Clear(subRectangleCount, subRectanglesToClear,
				clearColorAndDepth, colorToClear, depthToClear, noStencilBuffer);
			assert(SUCCEEDED(result));
			s_direct3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
		}

		// The actual function calls that draw geometry must be made between paired calls to
		// BeginScene() and EndScene()
		{
			HRESULT result = s_direct3dDevice->BeginScene();
			assert(SUCCEEDED(result));
			{
				D3DPERF_BeginEvent(0, L"Mesh Rendering");
				s_direct3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
				s_direct3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
				s_direct3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
				s_direct3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);

				
				{

					D3DPERF_BeginEvent(0, L"Debug Menu Rendering");

					
					//reset camera position and rotation
					std::string resetCameraText = "Set the Server/Client mode in console window!";
					pDebugMenu->pFont->DrawText(NULL, resetCameraText.c_str(), -1,
						&(pDebugMenu->debugItems[2].rect), DT_LEFT | DT_NOCLIP, 0xFFFFFFFF);

					//camera speed
					D3DPERF_EndEvent();

				}
			}
			result = s_direct3dDevice->EndScene();
			assert(SUCCEEDED(result));
		}

		// Everything has been drawn to the "back buffer", which is just an image in memory.
		// In order to display it, the contents of the back buffer must be "presented"
		// (to the front buffer)
		{
			const RECT* noSourceRectangle = NULL;
			const RECT* noDestinationRectangle = NULL;
			const HWND useDefaultWindow = NULL;
			const RGNDATA* noDirtyRegion = NULL;
			HRESULT result = s_direct3dDevice->Present(noSourceRectangle, noDestinationRectangle, useDefaultWindow, noDirtyRegion);
			assert(SUCCEEDED(result));
		}

		//network test

		char str[512];
		peer = RakNet::RakPeerInterface::GetInstance();

		printf("(C) or (S)erver?\n");
		gets(str);
		if ((str[0] == 'c') || (str[0] == 'C'))
		{
			RakNet::SocketDescriptor sd;
			peer->Startup(1, &sd, 1);
			isServer = false;
		}
		else {
			RakNet::SocketDescriptor sd(SERVER_PORT, 0);
			peer->Startup(MAX_CLIENTS, &sd, 1);
			isServer = true;
		}

		// TODO - Add code body here
		if (isServer)
		{
			printf("Starting the server.\n");
			// We need to let the server accept incoming connections from the clients
			peer->SetMaximumIncomingConnections(MAX_CLIENTS);
		}
		else {
			printf("Enter server IP or hit enter for 127.0.0.1\n");
			gets(str);
			if (str[0] == 0){
				strcpy(str, "127.0.0.1");
			}
			printf("Starting the client.\n");
			peer->Connect(str, SERVER_PORT, 0, 0);

		}

		return true;
	}
#ifdef _DEBUG_MENU
	bool CreateDebugMenu()
	{
		pDebugMenu = new DebugMenu::cDebugMenu(s_direct3dDevice);
		pDebugMeshArray[0] = new Mesh::cMesh(debugBoxMeshPath, s_direct3dDevice);
		pDebugMeshArray[1] = new Mesh::cMesh(debugSphereMeshPath, s_direct3dDevice);
		if (!CreateDebugLineVertexBuffer(D3DUSAGE_WRITEONLY))
			return false;

		return true;
	}
	bool CreateDebugLineVertexBuffer(const DWORD i_usage)
	{

		// Initialize the vertex format
		HRESULT result = s_direct3dDevice->CreateVertexDeclaration(s_vertexElements, &s_vertexDeclaration);
		if (SUCCEEDED(result))
		{
			result = s_direct3dDevice->SetVertexDeclaration(s_vertexDeclaration);
			if (FAILED(result))
			{
				MessageBox(s_mainWindow, "DirectX failed to set the vertex declaration", "No Vertex Declaration", MB_OK | MB_ICONERROR);
				return false;
			}
		}
		else
		{
			MessageBox(s_mainWindow, "DirectX failed to create a Direct3D9 vertex declaration", "No Vertex Declaration", MB_OK | MB_ICONERROR);
			return false;
		}

		// Create a vertex buffer
		{
			// EAE6320_TODO: How many vertices does a rectangle have?
			const unsigned int vertexCount = 2;
			const unsigned int bufferSize = vertexCount * sizeof(sVertex);
			// We will define our own vertex format
			const DWORD useSeparateVertexDeclaration = 0;
			// Place the vertex buffer into memory that Direct3D thinks is the most appropriate
			const D3DPOOL useDefaultPool = D3DPOOL_DEFAULT;
			HANDLE* const notUsed = NULL;

			result = s_direct3dDevice->CreateVertexBuffer(bufferSize, i_usage, useSeparateVertexDeclaration, useDefaultPool,
				&s_debugLineVertexBuffer, notUsed);
			if (FAILED(result))
			{
				MessageBox(s_mainWindow, "DirectX failed to create a vertex buffer", "No Vertex Buffer", MB_OK | MB_ICONERROR);
				return false;
			}
		}
		// Fill the vertex buffer with the rectangle's vertices
		{
			// Before the vertex buffer can be changed it must be "locked"
			sVertex* vertexData;
			{
				const unsigned int lockEntireBuffer = 0;
				const DWORD useDefaultLockingBehavior = 0;
				result = s_debugLineVertexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
					reinterpret_cast<void**>(&vertexData), useDefaultLockingBehavior);
				if (FAILED(result))
				{
					MessageBox(s_mainWindow, "DirectX failed to lock the vertex buffer", "No Vertex Buffer", MB_OK | MB_ICONERROR);
					return false;
				}
			}
			// Fill the buffer
			{
				// EAE6320_TODO: The x and y position of the first vertex
				// can be filled in like this:

				vertexData[0].x = 0.0f; vertexData[0].y = 0.0f; vertexData[0].z = 0.0f;
				vertexData[0].r = 1.0f; vertexData[0].g = 1.0f; vertexData[0].b = 1.0f;
				vertexData[0].nx = 0; vertexData[0].ny = 0; vertexData[0].nz = -1;
				vertexData[0].u = 0; vertexData[0].v = 0;

				vertexData[1].x = 0.0f; vertexData[1].y = 4.0f; vertexData[1].z = -4.0f;
				vertexData[1].r = 1.0f; vertexData[1].g = 1.0f; vertexData[1].b = 1.0f;
				vertexData[1].nx = 0; vertexData[1].ny = 0; vertexData[1].nz = -1;
				vertexData[1].u = 0; vertexData[1].v = 0;


			}
			// The buffer must be "unlocked" before it can be used
			{
				result = s_debugLineVertexBuffer->Unlock();
				if (FAILED(result))
				{
					MessageBox(s_mainWindow, "DirectX failed to unlock the vertex buffer", "No Vertex Buffer", MB_OK | MB_ICONERROR);
					return false;
				}
			}
		}

		return true;
	}
	bool RebuildOctreeIndexBufferAccordingToNewTriangleList(void)
	{
		if (pOctreeCollisdeTriangleIndexBuffer)
		{
			pOctreeCollisdeTriangleIndexBuffer->Release();
			pOctreeCollisdeTriangleIndexBuffer = NULL;
		}

		DWORD usage = 0;
		{
			// Our code will only ever write to the buffer
			usage |= D3DUSAGE_WRITEONLY;
			// The type of vertex processing should match what was specified when the device interface was created with CreateDevice()
			{
				D3DDEVICE_CREATION_PARAMETERS deviceCreationParameters;
				const HRESULT result = s_direct3dDevice->GetCreationParameters(&deviceCreationParameters);
				if (SUCCEEDED(result))
				{
					DWORD vertexProcessingType = deviceCreationParameters.BehaviorFlags &
						(D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_SOFTWARE_VERTEXPROCESSING);
					usage |= (vertexProcessingType != D3DCREATE_SOFTWARE_VERTEXPROCESSING) ?
						0 : D3DUSAGE_SOFTWAREPROCESSING;
				}
				else
				{
					return false;
				}
			}
		}

		// Create an index buffer
		{
			// We'll use 32-bit indices in this class to keep things simple
			D3DFORMAT format = D3DFMT_INDEX32;
			unsigned int bufferLength;
			{
				// EAE6320_TODO: How many triangles in a rectangle?
				const unsigned int verticesPerTriangle = 3;
				const unsigned int triangleNum = octreeCollideTriangleList.size();
				bufferLength = verticesPerTriangle * triangleNum * sizeof(unsigned int);
			}
			D3DPOOL useDefaultPool = D3DPOOL_DEFAULT;
			HANDLE* notUsed = NULL;

			HRESULT result = s_direct3dDevice->CreateIndexBuffer(bufferLength, usage, format, useDefaultPool,
				&pOctreeCollisdeTriangleIndexBuffer, notUsed);
			if (FAILED(result))
			{
				return false;
			}
		}
		// Fill the index buffer with the rectangle's triangles' indices
		{
		// Before the index buffer can be changed it must be "locked"
		unsigned int* indices;
		{
			const unsigned int lockEntireBuffer = 0;
			const DWORD useDefaultLockingBehavior = 0;
			const HRESULT result = pOctreeCollisdeTriangleIndexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
				reinterpret_cast<void**>(&indices), useDefaultLockingBehavior);
			if (FAILED(result))
			{
				return false;
			}
		}
		// Fill the buffer
		{
			// EAE6320_TODO: What should the indices be
			// in order to draw the required number of triangles
			// using a left-handed winding order?
			unsigned int i = 0;
			unsigned int indicesDataIndex = 0;
			while (i < octreeCollideTriangleList.size())
			{

				//triangle for plane first
				indices[indicesDataIndex] = octreeCollisionTriangleIndices[octreeCollideTriangleList[i] * 3];
				indicesDataIndex++;

				indices[indicesDataIndex] = octreeCollisionTriangleIndices[octreeCollideTriangleList[i] * 3 + 1];
				indicesDataIndex++;

				indices[indicesDataIndex] = octreeCollisionTriangleIndices[octreeCollideTriangleList[i] * 3 + 2];
				indicesDataIndex++;

				i++;
			}
			if (indicesDataIndex != octreeCollideTriangleList.size() * 3)
				return false;
		}
		// The buffer must be "unlocked" before it can be used
		{
			const HRESULT result = pOctreeCollisdeTriangleIndexBuffer->Unlock();
			if (FAILED(result))
			{
				return false;
			}
		}
	}

	}
	bool CreateOctreeCollideTriangleVertexBuffer(void)
	{
		if (pOctreeCollisdeTriangleVertexBuffer)
		{
			pOctreeCollisdeTriangleVertexBuffer->Release();
			pOctreeCollisdeTriangleVertexBuffer = NULL;
		}
		DWORD usage = 0;
		{
			// Our code will only ever write to the buffer
			usage |= D3DUSAGE_WRITEONLY;
			// The type of vertex processing should match what was specified when the device interface was created with CreateDevice()
			{
				D3DDEVICE_CREATION_PARAMETERS deviceCreationParameters;
				const HRESULT result = s_direct3dDevice->GetCreationParameters(&deviceCreationParameters);
				if (SUCCEEDED(result))
				{
					DWORD vertexProcessingType = deviceCreationParameters.BehaviorFlags &
						(D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_SOFTWARE_VERTEXPROCESSING);
					usage |= (vertexProcessingType != D3DCREATE_SOFTWARE_VERTEXPROCESSING) ?
						0 : D3DUSAGE_SOFTWAREPROCESSING;
				}
				else
				{
					return false;
				}
			}
		}

		// Initialize the vertex format
		HRESULT result;

		// Create a vertex buffer
		const unsigned int vertexCount = octreeCollisionTriangleVertices.size()/3;
		{
			// EAE6320_TODO: How many vertices does a rectangle have?


			const unsigned int bufferSize = vertexCount * sizeof(sVertex);
			// We will define our own vertex format
			const DWORD useSeparateVertexDeclaration = 0;
			// Place the vertex buffer into memory that Direct3D thinks is the most appropriate
			const D3DPOOL useDefaultPool = D3DPOOL_DEFAULT;
			HANDLE* const notUsed = NULL;

			result = s_direct3dDevice->CreateVertexBuffer(bufferSize, usage, useSeparateVertexDeclaration, useDefaultPool,
				&pOctreeCollisdeTriangleVertexBuffer, notUsed);
			if (FAILED(result))
			{
				return false;
			}
		}
		// Fill the vertex buffer with the rectangle's vertices
		{
			// Before the vertex buffer can be changed it must be "locked"
			sVertex* vertexData;
			{
				const unsigned int lockEntireBuffer = 0;
				const DWORD useDefaultLockingBehavior = 0;
				result = pOctreeCollisdeTriangleVertexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
					reinterpret_cast<void**>(&vertexData), useDefaultLockingBehavior);
				if (FAILED(result))
				{
					return false;
				}
			}
			// Fill the buffer
			{
				unsigned int i = 0;
				unsigned int vertexDataIndex = 0;
				while (i < octreeCollisionTriangleVertices.size()/3)
				{
					//position
					vertexData[vertexDataIndex].x = octreeCollisionTriangleVertices[i * 3];
					vertexData[vertexDataIndex].y = octreeCollisionTriangleVertices[i * 3 + 1];
					vertexData[vertexDataIndex].z = octreeCollisionTriangleVertices[i * 3 + 2];
					i++;
					vertexDataIndex++;
				}
				if (vertexDataIndex != octreeCollideTriangleList.size()/3)
					return false;
			}
			// The buffer must be "unlocked" before it can be used
			{
				result = pOctreeCollisdeTriangleVertexBuffer->Unlock();
				if (FAILED(result))
				{
					return false;
				}
			}
		}
		return true;
	}
	void DrawOctreeBox(Player::OctreeNode* i_currentOctree)
	{
		if (pPlayer->IntersectSegmentOctreeBox(cameraPosition, cameraPosition + cameraRayCastDir * 1000, i_currentOctree->halfWidth, i_currentOctree->centerX, i_currentOctree->centerY, i_currentOctree->centerZ))
		{
			
			
				s_direct3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				s_direct3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				s_direct3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

				//set material
				D3DPERF_BeginEvent(0, L"Set Material per instance");
				HRESULT result = s_direct3dDevice->SetPixelShader(pCTFMaterialArray[materialNum - 1]->pFragmentShader);
				assert(SUCCEEDED(result));
				D3DPERF_EndEvent();
				//set per view constance
				pCTFMaterialArray[materialNum - 1]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixViewToScreenHandler, &GetViewToScreenTransform());
				//lighting
				pCTFMaterialArray[materialNum - 1]->fragmentShaderconstantsTable->SetVector(s_direct3dDevice, constantHandlerDirectionLight, &directionLight);

				D3DPERF_BeginEvent(0, L"Draw Mesh");
				{
					// Set the shaders
					{
						pCTFMaterialArray[materialNum - 1]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixModelToWorldHandler,
						&GetTransform(D3DXVECTOR3(i_currentOctree->centerX, i_currentOctree->centerY, i_currentOctree->centerZ),
						D3DXVECTOR3(i_currentOctree->halfWidth * 2, i_currentOctree->halfWidth * 2, i_currentOctree->halfWidth * 2)));

						pCTFMaterialArray[materialNum - 1]->vertexShaderConstantsTable->SetMatrix(s_direct3dDevice, matrixWorldToViewHandler, &GetWorldToViewTransform(cameraPosition));

						result = s_direct3dDevice->SetVertexShader(pCTFMaterialArray[materialNum - 1]->pVertexShader);
						assert(SUCCEEDED(result));
					}
					//set texture
					{
					D3DXVECTOR4 boxColor(1.0f,1.0f,1.0f,1.0f);
					if (i_currentOctree->depth == 0){ boxColor.x = 0.9; boxColor.y = 0.5; boxColor.z = 0.1; }
					else if (i_currentOctree->depth == 1){ boxColor.x = 0.1; boxColor.y = 0.5; boxColor.z = 0.9; }
					else if (i_currentOctree->depth == 2){ boxColor.x = 0.1; boxColor.y = 0.9; boxColor.z = 0.5; }
					else{ boxColor.x = 0.5; boxColor.y = 0.7; boxColor.z = 0.2; }
						//pCTFMaterialArray[materialNum-1]->SetColor(boxColor.x,boxColor.y,boxColor.z);
					pCTFMaterialArray[materialNum - 1]->fragmentShaderconstantsTable->SetVector(s_direct3dDevice, constantHandlerColor, &boxColor);
					}
					// Bind a specific vertex buffer to the device as a data source
					{
						// There can be multiple streams of data feeding the display adaptor at the same time
						const unsigned int streamIndex = 0;
						// It's possible to start streaming data in the middle of a vertex buffer
						const unsigned int bufferOffset = 0;
						// The "stride" defines how large a single vertex is in the stream of data
						const unsigned int bufferStride = sizeof(sVertex);
						result = s_direct3dDevice->SetStreamSource(streamIndex, pDebugMeshArray[0]->pVertexBuffer, bufferOffset, bufferStride);
						assert(SUCCEEDED(result));
					}
					// Set the indices to use
					{
						HRESULT result = s_direct3dDevice->SetIndices(pDebugMeshArray[0]->pIndexBuffer);
						assert(SUCCEEDED(result));
					}
					// Render objects from the current streams
					{
						// We are using triangles as the "primitive" type,
						// and we have defined the vertex buffer as a triangle list
						// (meaning that every triangle is defined by three vertices)
						const D3DPRIMITIVETYPE primitiveType = D3DPT_TRIANGLELIST;
						// It's possible to start rendering primitives in the middle of the stream
						const unsigned int indexOfFirstVertexToRender = 0;
						const unsigned int indexOfFirstIndexToUse = 0;
						// EAE6320_TODO: How many vertices are in a rectangle,
						// and how many "primitives" (triangles) is it made up of?
						const unsigned int vertexCountToRender = pDebugMeshArray[0]->vertexNum;
						const unsigned int primitiveCountToRender = pDebugMeshArray[0]->indexNum / 3;
						HRESULT result = s_direct3dDevice->DrawIndexedPrimitive(primitiveType, indexOfFirstVertexToRender, indexOfFirstVertexToRender,
							vertexCountToRender, indexOfFirstIndexToUse, primitiveCountToRender);
						assert(SUCCEEDED(result));
					}
				}
				D3DPERF_EndEvent();
			
			for (unsigned int i = 0; i < 8; i++)
			{
				if (i_currentOctree->children[i] != NULL)
				{
					DrawOctreeBox(i_currentOctree->children[i]);
				}
			}
		}
	}
#endif
	D3DXMATRIX GetTransform(D3DXVECTOR3& i_position)
	{
		D3DXMATRIX transform;
		{
			D3DXMatrixTransformation(&transform, NULL, NULL, NULL, NULL,
				NULL, &i_position);
		}
		return transform;
	}
	D3DXMATRIX GetTransform(D3DXVECTOR3& i_position, D3DXVECTOR3& i_scale)
	{
		D3DXMATRIX transform;
		{
			D3DXMatrixTransformation(&transform, NULL, NULL, &i_scale, NULL,
				NULL, &i_position);
		}
		return transform;
	}
	D3DXMATRIX GetTransform(D3DXVECTOR3& i_position, D3DXQUATERNION& i_rotation)
	{
		D3DXMATRIX transform;
		{
			D3DXMatrixTransformation(&transform, NULL, NULL, NULL, NULL,
				&i_rotation, &i_position);
		}
		return transform;
	}
	D3DXMATRIX GetTransform(D3DXVECTOR3& i_position, D3DXVECTOR3& i_scale, D3DXMATRIX& i_rotation)
	{
		D3DXMATRIX transform;
		{
			D3DXQUATERNION rotation;
			D3DXQuaternionRotationMatrix(&rotation, &i_rotation);
			D3DXMatrixTransformation(&transform, NULL, NULL, &i_scale, NULL,
				&rotation, &i_position);
		}
		return transform;
	}
	D3DXMATRIX GetWorldToViewTransform(D3DXVECTOR3& i_position)
	{
		D3DXMATRIX viewMatrix;
		if (bindPlayer > 0.0f)
		{
			D3DXVECTOR3 eyePoint =i_position;
			D3DXVECTOR3 lookAtPoint = pPlayer->pPlayerMesh->meshPosition;

			D3DXVECTOR3 fwdVector = lookAtPoint - eyePoint;
			D3DXVec3Normalize(&fwdVector, &fwdVector);

			D3DXVECTOR3 upVector(0.0f, 1.0f, 0.0f);
			D3DXVECTOR3 sideVector;
			D3DXVec3Cross(&sideVector, &upVector, &fwdVector);
			D3DXVec3Cross(&upVector, &fwdVector, &sideVector);

			D3DXMatrixLookAtLH(&viewMatrix, &eyePoint, &lookAtPoint, &upVector);
		}
		else
		{
			D3DXMATRIX cameraRot = *GetArcBall()->GetRotationMatrix();
			D3DXMatrixInverse(&cameraRot, NULL, &cameraRot);

			D3DXVECTOR3 vWorldUp;
			D3DXVECTOR3 vLocalUp = D3DXVECTOR3(0, 1, 0);
			D3DXVec3TransformCoord(&vWorldUp, &vLocalUp, &cameraRot);

			D3DXVECTOR3 vWorldAhead;
			D3DXVECTOR3 vLocalAhead = D3DXVECTOR3(0, 0, 1);
			D3DXVec3TransformCoord(&vWorldAhead, &vLocalAhead, &cameraRot);
#ifdef _DEBUG_MENU
			cameraRayCastDir = vWorldAhead;
#endif
			D3DXVECTOR3 eyePoint = i_position;
			D3DXVECTOR3 lookAtPoint = eyePoint + vWorldAhead * 10;

			D3DXMatrixLookAtLH(&viewMatrix, &eyePoint, &lookAtPoint, &vWorldUp);
		}
		

		return viewMatrix;
		
	}
	void CameraMoveForward(float i_moveAmount)
	{
		D3DXMATRIX cameraRot;
		D3DXMatrixInverse(&cameraRot, NULL, GetArcBall()->GetRotationMatrix());

		D3DXVECTOR3 vWorldAhead;
		D3DXVECTOR3 vLocalAhead = D3DXVECTOR3(0, 0, 1);
		D3DXVec3TransformCoord(&vWorldAhead, &vLocalAhead, &cameraRot);
#ifdef _DEBUG_MENU
		cameraRayCastDir = vWorldAhead;
#endif
		if (bindPlayer > 0.0f)
			vWorldAhead.y = 0;
		cameraPosition += vWorldAhead*i_moveAmount;
	}
	void CameraMoveUpward(float i_moveAmount)
	{
		D3DXMATRIX cameraRot;
		D3DXMatrixInverse(&cameraRot, NULL, GetArcBall()->GetRotationMatrix());

		D3DXVECTOR3 vWorldUp;
		D3DXVECTOR3 vLocalUp = D3DXVECTOR3(0, 1, 0);
		D3DXVec3TransformCoord(&vWorldUp, &vLocalUp, &cameraRot);
		
		cameraPosition += vWorldUp*i_moveAmount;
	}
	void CameraMoveLeft(float i_moveAmount)
	{
		D3DXMATRIX cameraRot;
		D3DXMatrixInverse(&cameraRot, NULL, GetArcBall()->GetRotationMatrix());

		D3DXVECTOR3 vWorldUp;
		D3DXVECTOR3 vLocalUp = D3DXVECTOR3(0, 1, 0);
		D3DXVec3TransformCoord(&vWorldUp, &vLocalUp, &cameraRot);

		D3DXVECTOR3 vWorldAhead;
		D3DXVECTOR3 vLocalAhead = D3DXVECTOR3(0, 0, 1);
		D3DXVec3TransformCoord(&vWorldAhead, &vLocalAhead, &cameraRot);
#ifdef _DEBUG_MENU
		cameraRayCastDir = vWorldAhead;
#endif
		D3DXVECTOR3 vWorldLeft;
		D3DXVec3Cross(&vWorldLeft, &vWorldAhead, &vWorldUp);

		cameraPosition += vWorldLeft*i_moveAmount;
	}

	void PlayerMoveForward(float i_moveAmount)
	{
		D3DXMATRIX playerRot;
		D3DXMatrixInverse(&playerRot, NULL, &(pPlayer->pPlayerMesh->meshRotationMatrix));

		D3DXVECTOR3 vWorldAhead;
		D3DXVECTOR3 vLocalAhead = D3DXVECTOR3(0, 0, -1);
		D3DXVec3TransformCoord(&vWorldAhead, &vLocalAhead, &playerRot);
		vWorldAhead.x *= -1;
		
		posBefore = pPlayer->pPlayerMesh->meshPosition;
		pPlayer->pPlayerMesh->meshPosition += vWorldAhead*i_moveAmount;
	}
	void PlayerMoveUpward(float i_moveAmount)
	{
		D3DXMATRIX playerRot;
		D3DXMatrixInverse(&playerRot, NULL, &(pPlayer->pPlayerMesh->meshRotationMatrix));

		D3DXVECTOR3 vWorldUp;
		D3DXVECTOR3 vLocalUp = D3DXVECTOR3(0, 1, 0);
		D3DXVec3TransformCoord(&vWorldUp, &vLocalUp, &playerRot);

		posBefore = pPlayer->pPlayerMesh->meshPosition;
		pPlayer->pPlayerMesh->meshPosition += vWorldUp*i_moveAmount;
	}
	void PlayerMoveLeft(float i_moveAmount)
	{
		D3DXMATRIX playerRot;
		D3DXMatrixInverse(&playerRot, NULL, &(pPlayer->pPlayerMesh->meshRotationMatrix));

		D3DXVECTOR3 vWorldUp;
		D3DXVECTOR3 vLocalUp = D3DXVECTOR3(0, 1, 0);
		D3DXVec3TransformCoord(&vWorldUp, &vLocalUp, &playerRot);

		D3DXVECTOR3 vWorldAhead;
		D3DXVECTOR3 vLocalAhead = D3DXVECTOR3(0, 0, -1);
		D3DXVec3TransformCoord(&vWorldAhead, &vLocalAhead, &playerRot);

		D3DXVECTOR3 vWorldLeft;
		D3DXVec3Cross(&vWorldLeft, &vWorldAhead, &vWorldUp);

		//player needs to face to the direction it's moving
		playerRotValue += -1 * eae6320::Time::GetSecondsElapsedThisFrame()*i_moveAmount/abs(i_moveAmount);
		D3DXMatrixRotationAxis(&pPlayer->pPlayerMesh->meshRotationMatrix, &vWorldUp, playerRotValue);
		//vWorldLeft.x *= -1;
		posBefore = pPlayer->pPlayerMesh->meshPosition;
		pPlayer->pPlayerMesh->meshPosition += vWorldLeft*i_moveAmount/2;
	}
	void ApplyMatrix(D3DXMATRIX& matrix, D3DXVECTOR4& i_vector)
	{
		i_vector.x = (matrix._11*i_vector.x + matrix._21*i_vector.y + matrix._31*i_vector.z + matrix._41*i_vector.w);
		i_vector.y = (matrix._12*i_vector.x + matrix._22*i_vector.y + matrix._32*i_vector.z + matrix._42*i_vector.w);
		i_vector.z = (matrix._13*i_vector.x + matrix._23*i_vector.y + matrix._33*i_vector.z + matrix._43*i_vector.w);
	}
	D3DXMATRIX GetViewToScreenTransform()
	{
		D3DXMATRIX transform_viewToScreen;
		{
			const float aspectRatio = static_cast<float>(g_windowWidth) / static_cast<float>(g_windowHeight);
			D3DXMatrixPerspectiveFovLH(&transform_viewToScreen, fieldOfView, aspectRatio, nearPlane, farPlane);
		}
		return transform_viewToScreen;
	}

	bool LoadBinaryCollisionFile(const char*i_binaryMeshFilePath, char*& i_bufferPointer)
	{
		bool wereThereErrors = false;
		std::ifstream file(i_binaryMeshFilePath, std::ifstream::binary);
		if (file)
		{
			file.seekg(0, file.end);
			int length = (int)file.tellg();
			fileLength = length;
			file.seekg(0, file.beg);

			i_bufferPointer = new char[length];

			file.read(i_bufferPointer, length);
			if (!file)
			{
				wereThereErrors = true;
			}


			file.close();
		}
		else
		{
			wereThereErrors = true;
		}

		return !wereThereErrors;
	}
	void CameraFollowPlayer(D3DXVECTOR3 &i_playerPos, float i_deltaT)
	{
		//camera and player should keep a reasonable distance and angle
		//first distance
		{
			//if getting too far, move the camera toweard player
			// move to the back of player, first get the back direction of player
			D3DXVECTOR3 vWorldBack;
			{
				D3DXMATRIX playerRot;
				D3DXMatrixInverse(&playerRot, NULL, &(pPlayer->pPlayerMesh->meshRotationMatrix));

				D3DXVECTOR3 vLocalBack = D3DXVECTOR3(0, 0, 1);
				D3DXVec3TransformCoord(&vWorldBack, &vLocalBack, &playerRot);
			}
			vWorldBack.x *= -1;
			D3DXVec3Normalize(&vWorldBack, &vWorldBack);
			D3DXVECTOR3 cameraDes = i_playerPos + D3DXVECTOR3(0, 100, 0) + vWorldBack * 250;
			D3DXVECTOR3 moveDir=cameraDes-cameraPosition;
			D3DXVec3Normalize(&moveDir, &moveDir);
			D3DXVECTOR3 currentVelocityNormal;
			D3DXVec3Normalize(&currentVelocityNormal, &cameraVelocity);
			D3DXVECTOR3 cameraAcceleration;
			if (VelocityEqual(moveDir, currentVelocityNormal))
			{
				cameraAcceleration = moveDir;
			}
			else
			{
				cameraAcceleration = (moveDir - currentVelocityNormal);
			}
			
			D3DXVECTOR3 deltaS(0, 0, 0);
			deltaS = cameraVelocity*i_deltaT + 0.5*50000*cameraAcceleration*i_deltaT*i_deltaT;
			cameraVelocity += cameraAcceleration*i_deltaT;

			if (PositionClose(cameraPosition, cameraDes))
			{
				cameraAcceleration = D3DXVECTOR3(0, 0, 0);
				cameraVelocity = D3DXVECTOR3(0,0,0);
				cameraPosition = cameraDes;
			}
			else
			{
				//update camera position
				cameraPosition += deltaS;
				
			}
			//CameraPointToPlayer();
		}

	}
	void CameraPointToPlayer()
	{
		D3DXVECTOR3 toPlayer = pPlayer->pPlayerMesh->meshPosition - cameraPosition;
		D3DXVECTOR3 fwdVector;
		D3DXVec3Normalize(&fwdVector, &toPlayer);

		D3DXVECTOR3 upVector(0.0f, 1.0f, 0.0f);
		D3DXVECTOR3 sideVector;
		D3DXVec3Cross(&sideVector, &upVector, &fwdVector);
		D3DXVec3Cross(&upVector, &fwdVector, &sideVector);

		D3DXVec3Normalize(&upVector, &upVector);
		D3DXVec3Normalize(&sideVector, &sideVector);

		D3DXMATRIX rotation(sideVector.x, sideVector.y, sideVector.z, 0.0f,
			upVector.x, upVector.y, upVector.z, 0.0f,
			fwdVector.x, fwdVector.y, fwdVector.z, 0.0f,
			cameraPosition.x, cameraPosition.y, cameraPosition.z, 1.0f);
		cameraRotation = rotation;
	}
	float Distance(D3DXVECTOR3& i_pos1, D3DXVECTOR3& i_pos2)
	{
		return sqrt((i_pos1.x - i_pos2.x)*(i_pos1.x - i_pos2.x) + (i_pos1.y - i_pos2.y)*(i_pos1.y - i_pos2.y) + (i_pos1.z - i_pos2.z)*(i_pos1.z - i_pos2.z));
	}
	bool VelocityEqual(D3DXVECTOR3& i_v1, D3DXVECTOR3& i_v2)
	{
		if (abs(i_v1.x - i_v2.x) < 0.1f&&abs(i_v1.y - i_v2.y) < 0.1f&&abs(i_v1.z - i_v2.z) < 0.1f)
			return true;
		else return false;
	}
	bool PositionEqual(D3DXVECTOR3& i_p1, D3DXVECTOR3& i_p2)
	{
		if (abs(i_p1.x - i_p2.x) < 0.1f&&abs(i_p1.y - i_p2.y) < 0.1f&&abs(i_p1.z - i_p2.z) < 0.1f)
			return true;
		else return false;
	}
	bool PositionClose(D3DXVECTOR3& i_p1, D3DXVECTOR3& i_p2)
	{
		if (abs(i_p1.x - i_p2.x) < 10.0f&&abs(i_p1.y - i_p2.y) < 10.0f&&abs(i_p1.z - i_p2.z) < 10.0f)
			return true;
		else return false;
	}
}
