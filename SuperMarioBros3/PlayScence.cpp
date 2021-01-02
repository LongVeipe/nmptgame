#include <iostream>
#include <fstream>

#include "PlayScence.h"
#include "Utils.h"
#include "Textures.h"
#include "Sprites.h"
#include "Portal.h"
#include "Coin.h"
#include "Bullet.h"
#include "Reward_LevelUp.h"
#include "Plant_Fire.h"
#include "Plant_Normal.h"
#include "Koopa_Small.h"
#include "QuestionBox.h"

using namespace std;

CPlayScene::CPlayScene(int id, LPCWSTR filePath):CScene(id, filePath)
{
	key_handler = new CPlayScenceKeyHandler(this);
	map = nullptr;
	player = nullptr;
}

/*
	Load scene resources from scene file (textures, sprites, animations and objects)
	See scene1.txt, scene2.txt for detail format specification
*/

#define SCENE_SECTION_UNKNOWN -1
#define SCENE_SECTION_TEXTURES 2
#define SCENE_SECTION_SPRITES 3
#define SCENE_SECTION_ANIMATIONS 4
#define SCENE_SECTION_ANIMATION_SETS	5
#define SCENE_SECTION_OBJECTS	6
#define SCENE_SECTION_MAP	7

#define OBJECT_TYPE_MARIO			0
#define OBJECT_TYPE_BRICK			1
#define OBJECT_TYPE_QUESTION_BOX	2
#define OBJECT_TYPE_GOOMBA			3
#define OBJECT_TYPE_KOOPA_SMALL		4
#define OBJECT_TYPE_COIN			5
#define OBJECT_TYPE_PLANT_FIRE		6
#define OBJECT_TYPE_PLANT_NORMAL	7

#define OBJECT_TYPE_PORTAL	50

#define MAX_SCENE_LINE 1024


void CPlayScene::_ParseSection_TEXTURES(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() < 5) return; // skip invalid lines

	int texID = atoi(tokens[0].c_str());
	wstring path = ToWSTR(tokens[1]);
	int R = atoi(tokens[2].c_str());
	int G = atoi(tokens[3].c_str());
	int B = atoi(tokens[4].c_str());

	CTextures::GetInstance()->Add(texID, path.c_str(), D3DCOLOR_XRGB(R, G, B));
}

void CPlayScene::_ParseSection_MAP(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() < 9) return; // skip invalid lines

	int idMap = atoi(tokens[0].c_str());
	int tileWidth = atoi(tokens[1].c_str()); 
	int tileHeight = atoi(tokens[2].c_str());
	int tRTileSet = atoi(tokens[3].c_str());
	int tCTileSet = atoi(tokens[4].c_str());
	int tRMap = atoi(tokens[5].c_str());
	int tCMap = atoi(tokens[6].c_str());
	int totalTiles = atoi(tokens[7].c_str());
	wstring MatrixPath = ToWSTR(tokens[8]);

	this->map = new Map(idMap, tileWidth, tileHeight, tRTileSet, tCTileSet, tRMap, tCMap, totalTiles);
	map->LoadMatrix(MatrixPath.c_str());
	map->CreateTilesFromTileSet();
}

void CPlayScene::_ParseSection_SPRITES(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() < 6) return; // skip invalid lines

	int ID = atoi(tokens[0].c_str());
	int l = atoi(tokens[1].c_str());
	int t = atoi(tokens[2].c_str());
	int r = atoi(tokens[3].c_str());
	int b = atoi(tokens[4].c_str());
	int texID = atoi(tokens[5].c_str());

	LPDIRECT3DTEXTURE9 tex = CTextures::GetInstance()->Get(texID);
	if (tex == NULL)
	{
		DebugOut(L"[ERROR] Texture ID %d not found!\n", texID);
		return; 
	}

	CSprites::GetInstance()->Add(ID, l, t, r, b, tex);
}

void CPlayScene::_ParseSection_ANIMATIONS(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() < 3) return; // skip invalid lines - an animation must at least has 1 frame and 1 frame time

	//DebugOut(L"--> %s\n",ToWSTR(line).c_str());

	LPANIMATION ani = new CAnimation();

	int ani_id = atoi(tokens[0].c_str());
	for (int i = 1; i < tokens.size(); i += 2)	// why i+=2 ?  sprite_id | frame_time  
	{
		int sprite_id = atoi(tokens[i].c_str());
		int frame_time = atoi(tokens[i+1].c_str());
		ani->Add(sprite_id, frame_time);
	}

	CAnimations::GetInstance()->Add(ani_id, ani);
}

void CPlayScene::_ParseSection_ANIMATION_SETS(string line)
{
	vector<string> tokens = split(line);

	if (tokens.size() < 2) return; // skip invalid lines - an animation set must at least id and one animation id

	int ani_set_id = atoi(tokens[0].c_str());

	LPANIMATION_SET s = new CAnimationSet();

	CAnimations *animations = CAnimations::GetInstance();

	for (int i = 1; i < tokens.size(); i++)
	{
		int ani_id = atoi(tokens[i].c_str());
		
		LPANIMATION ani = animations->Get(ani_id);
		s->push_back(ani);
	}

	CAnimationSets::GetInstance()->Add(ani_set_id, s);
}

/*
	Parse a line in section [OBJECTS] 
*/
void CPlayScene::_ParseSection_OBJECTS(string line)
{
	vector<string> tokens = split(line);

	//DebugOut(L"--> %s\n",ToWSTR(line).c_str());

	if (tokens.size() < 3) return; // skip invalid lines - an object set must have at least id, x, y

	int object_type = atoi(tokens[0].c_str());
	float x = atof(tokens[1].c_str());
	float y = atof(tokens[2].c_str());

	int ani_set_id = atoi(tokens[3].c_str());

	CAnimationSets * animation_sets = CAnimationSets::GetInstance();

	CGameObject *obj = NULL;

	switch (object_type)
	{
	case OBJECT_TYPE_MARIO:
		if (player!=NULL) 
		{
			DebugOut(L"[ERROR] MARIO object was created before!\n");
			return;
		}
		obj = new CMario(x,y); 
		player = (CMario*)obj;  

		DebugOut(L"[INFO] Player object created!\n");
		break;
	//case OBJECT_TYPE_GOOMBA: obj = new CGoomba(); break;
	case OBJECT_TYPE_BRICK:
	{
		int type = atoi(tokens[4].c_str());
		obj = new CBrick(x, y, type);
		break;
	}
	case OBJECT_TYPE_QUESTION_BOX:
	{
		int type = atoi(tokens[4].c_str());
		obj = new CQuestionBox(x, y, type);
		break;
	}
	case OBJECT_TYPE_KOOPA_SMALL:
	{
		float typeKoopa = atof(tokens[4].c_str());
		obj = new CKoopa_Small(x, y, typeKoopa);
		break;
	}
	case OBJECT_TYPE_GOOMBA:
	{
		float typeGoomba = atof(tokens[4].c_str());
		obj = new CGoomba(x, y, typeGoomba);
		break;
	}
	case OBJECT_TYPE_PLANT_FIRE:
	{
		float limit_y = atof(tokens[4].c_str());
		int type = atoi(tokens[5].c_str());
		obj = new CPlant_Fire(x, y, limit_y, type);
		break;
	}
	case OBJECT_TYPE_PLANT_NORMAL:
	{
		float limit_y = atof(tokens[4].c_str());
		int type = atoi(tokens[5].c_str());
		obj = new CPlant_Normal(x, y, limit_y, type);
		break;
	}
	case OBJECT_TYPE_COIN: 
	{
		obj = new CCoin(x, y);
		break;
	}
	case OBJECT_TYPE_PORTAL:
	{
		float r = atof(tokens[4].c_str());
		float b = atof(tokens[5].c_str());
		int scene_id = atoi(tokens[6].c_str());
		obj = new CPortal(x, y, r, b, scene_id);
	}
		break;
	default:
		DebugOut(L"[ERR] Invalid object type: %d\n", object_type);
		return;
	}

	// General object setup
	obj->SetPosition(x, y);

	LPANIMATION_SET ani_set = animation_sets->Get(ani_set_id);

	obj->SetAnimationSet(ani_set);
	objects.push_back(obj);
}

void CPlayScene::Load()
{
	DebugOut(L"[INFO] Start loading scene resources from : %s \n", sceneFilePath);

	ifstream f;
	f.open(sceneFilePath);

	// current resource section flag
	int section = SCENE_SECTION_UNKNOWN;					

	char str[MAX_SCENE_LINE];
	while (f.getline(str, MAX_SCENE_LINE))
	{
		string line(str);

		if (line[0] == '#') continue;	// skip comment lines	

		if (line == "[TEXTURES]") { section = SCENE_SECTION_TEXTURES; continue; }
		if (line == "[MAP]") { section = SCENE_SECTION_MAP; continue; }
		if (line == "[SPRITES]") { section = SCENE_SECTION_SPRITES; continue; }
		if (line == "[ANIMATIONS]") { section = SCENE_SECTION_ANIMATIONS; continue; }
		if (line == "[ANIMATION_SETS]") { section = SCENE_SECTION_ANIMATION_SETS; continue; }
		if (line == "[OBJECTS]") { section = SCENE_SECTION_OBJECTS; continue; }
		if (line[0] == '[') { section = SCENE_SECTION_UNKNOWN; continue; }	

		//
		// data section
		//
		switch (section)
		{ 
			case SCENE_SECTION_TEXTURES: _ParseSection_TEXTURES(line); break;
			case SCENE_SECTION_SPRITES: _ParseSection_SPRITES(line); break;
			case SCENE_SECTION_ANIMATIONS: _ParseSection_ANIMATIONS(line); break;
			case SCENE_SECTION_ANIMATION_SETS: _ParseSection_ANIMATION_SETS(line); break;
			case SCENE_SECTION_OBJECTS: _ParseSection_OBJECTS(line); break;
			case SCENE_SECTION_MAP: _ParseSection_MAP(line); break;
		}
	}

	f.close();

	CTextures::GetInstance()->Add(ID_TEX_BBOX, L"textures\\bbox.png", D3DCOLOR_XRGB(255, 255, 255));

	DebugOut(L"[INFO] Done loading scene resources %s\n", sceneFilePath);
}

void CPlayScene::Update(DWORD dt)
{
	// We know that Mario is the first object in the list hence we won't add him into the colliable object list
	// TO-DO: This is a "dirty" way, need a more organized way 

	vector<LPGAMEOBJECT> coObjects;
	for (size_t i = 1; i < objects.size(); i++)
	{
		coObjects.push_back(objects[i]);
	}

	for (size_t i = 0; i < objects.size(); i++)
	{
		/*if (dynamic_cast<CGoomba*>(objects[i]))
		{
			CGoomba* goomba = dynamic_cast<CGoomba*>(objects[i]);
			if ((goomba->GetState() == GOOMBA_STATE_DIE_X || goomba->GetState() == GOOMBA_STATE_DIE_Y ) && !goomba->IsInCamera())
			{
				objects.erase(objects.begin() + i);
				coObjects.erase(coObjects.begin() + i);
				delete goomba;
				goomba = nullptr;
			}
			else
				goomba->Update(dt, &coObjects);
		}
		else*/
			objects[i]->Update(dt, &coObjects);
	}

	// skip the rest if scene was already unloaded (Mario::Update might trigger PlayScene::Unload)
	if (player == NULL) return; 


	// Update camera to follow mario
	float cx, cy;
	player->GetPosition(cx, cy);
	CGame *game = CGame::GetInstance();
	//cx
	if (map != nullptr && (cx > map->GetMapWidth() - game->GetScreenWidth() / 2))
		cx = map->GetMapWidth() - game->GetScreenWidth();
	else if (cx < game->GetScreenWidth() / 2)
		cx = 0;
	else
		cx -= game->GetScreenWidth() / 2;
	//cy
	if (map != nullptr && (cy > map->GetMapHeight() - game->GetScreenHeight() / 2))
		cy = map->GetMapHeight() - game->GetScreenHeight();
	else if (cy < game->GetScreenHeight() / 2)
		cy = 0;
	else
		cy -= game->GetScreenHeight() / 2;
	if(!player->IsSwingTail)
		CGame::GetInstance()->SetCamPos(round(cx), round(cy));
}

void CPlayScene::Render()
{
	if (map)
		this->map->Render();
	for (int i = 0; i < objects.size(); i++)
	{
		objects[i]->Render();
	}
	
}

/*
	Unload current scene
*/
void CPlayScene::Unload()
{
	for (int i = 0; i < objects.size(); i++)
		delete objects[i];
	
	objects.clear();
	player = NULL;

	delete map;
	map = nullptr;



	DebugOut(L"[INFO] Scene %s unloaded! \n", sceneFilePath);
}

void CPlayScenceKeyHandler::OnKeyDown(int KeyCode)
{
	//DebugOut(L"[INFO] KeyDown: %d\n", KeyCode);

	CMario *mario = ((CPlayScene*)scence)->GetPlayer();
	switch (KeyCode)
	{
	case DIK_SPACE:
		if (mario->GetLevel() == MARIO_LEVEL_RACCOON)
		{
			if (mario->IsRaccoonReadyFly())
			{
				mario->RaccoonStartFlyHigh();
			}
			else if (mario->IsRaccoonCanFlyHigh)
			{
				mario->SetState(MARIO_STATE_JUMP);
			}
			else if (mario->IsFalling && !mario->IsRaccoonCanFlyHigh)
			{
 				if (!mario->IsFlying)
					mario->SlowFall();
			}


		}
		if (mario->IsTouchingGround == true)
		{
			mario->SetJumpStack(0);
			mario->IsReadyJump = true;
			mario->SetState(MARIO_STATE_JUMP);
			mario->UpJumpStack();
			mario->IsTouchingGround = false;
			
		}
		break;
	case DIK_Q: 
		mario->Reset();
		break;
	case DIK_C:
		if (mario->GetLevel() == MARIO_LEVEL_FIRE)
			mario->StartThrowFire();
		else if (mario->GetLevel() == MARIO_LEVEL_RACCOON)
			mario->StartSwingTail();
		break;
	case DIK_DOWN:
		mario->SetState(MARIO_STATE_BEND_DOWN);
		break;
	case DIK_Z:
		int newLevel = mario->GetLevel() + 1;
		if (newLevel > MARIO_MAX_LEVEL)
			newLevel = 1;
		float cur_x, cur_y;
		mario->GetPosition(cur_x, cur_y);
		mario->SetPosition(cur_x, cur_y - 16);
		mario->SetLevel(newLevel);
		break;
	}
}
void CPlayScenceKeyHandler::OnKeyUp(int KeyCode)
{

	CMario* mario = ((CPlayScene*)scence)->GetPlayer();
	switch (KeyCode)
	{
	case DIK_DOWN:
		mario->SetState(MARIO_STATE_IDLE);
		break;
	case DIK_SPACE:
		mario->SetJumpStack(MARIO_MAX_JUMPIMG_STACKS);
		mario->IsReadyJump = false;
		break;
	case DIK_RIGHT:
		mario->SetState(MARIO_STATE_IDLE);
		break;
	case DIK_LEFT:
		mario->SetState(MARIO_STATE_IDLE);
		break;
	case DIK_B:
		mario->IsReadyHolding = false;
		mario->IsHolding = false;
		break;
	}
}

void CPlayScenceKeyHandler::KeyState(BYTE *states)
{
	CGame *game = CGame::GetInstance();
	CMario *mario = ((CPlayScene*)scence)->GetPlayer();

	// disable control key when Mario die 
	
	if (mario->GetState() == MARIO_STATE_DIE) return;
	
	if (game->IsKeyDown(DIK_RIGHT))
	{
		mario->SetState(MARIO_STATE_WALKING_RIGHT);

		if (game->IsKeyDown(DIK_SPACE))
		{
			if (mario->IsReadyJump == true && mario->GetJumpStack() < MARIO_MAX_JUMPIMG_STACKS)
			{

				mario->SetState(MARIO_STATE_JUMP);
				mario->UpJumpStack();
			}
			mario->downImminent();
		}
		else if (game->IsKeyDown(DIK_D))
		{
			mario->upImminent();
		}
		else
			mario->downImminent();
	}
	else if (game->IsKeyDown(DIK_LEFT))
	{
		mario->SetState(MARIO_STATE_WALKING_LEFT);
		if (game->IsKeyDown(DIK_SPACE))
		{
			if (mario->IsReadyJump == true && mario->GetJumpStack() < MARIO_MAX_JUMPIMG_STACKS)
			{

				mario->SetState(MARIO_STATE_JUMP);
				mario->UpJumpStack();
			}
			mario->downImminent();
		}
		else if (game->IsKeyDown(DIK_D))
		{
			mario->upImminent();
		}
		else
		{
			mario->downImminent();
		}
	}
	else if(game->IsKeyDown(DIK_SPACE))
	{ 
		if (mario->IsReadyJump == true && mario->GetJumpStack() < MARIO_MAX_JUMPIMG_STACKS)
		{
			mario->SetState(MARIO_STATE_JUMP);
			mario->UpJumpStack();
			if(game->IsKeyDown(DIK_RIGHT))
				mario->SetState(MARIO_STATE_WALKING_RIGHT);
			else if(game->IsKeyDown(DIK_LEFT))
				mario->SetState(MARIO_STATE_WALKING_LEFT);
			mario->downImminent();
		}
	}
	else 
	{ 
		if(mario->IsTouchingGround)
			mario->SetState(MARIO_STATE_IDLE);
		mario->downImminent();
	}
	if (game->IsKeyDown(DIK_B))
	{
		mario->IsReadyHolding = true;
	}
	
		
}