#include "scene.h"
#include "game.h"
#include "animation.h"
#include "direct3d.h"
#include "keyboard.h"
#include "texture.h"
#include "title.h"

SCENE scene = SCENE_ANM_LOGO;

void Init(void)
{
	switch (scene)
	{
	case SCENE_TITLE:
		Title_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	case SCENE_GAME:
		Game_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	case SCENE_RESULT:
		break;
	case SCENE_ANM_LOGO:
		Animation_Logo_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	case SCENE_ANM_OP:
		Animation_Op_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	case SCENE_ANM_WIN:
		Animation_Win_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	case SCENE_ANM_LOSE:
		Animation_Lose_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	default:
		break;
	}
}

void Update(void)
{
	switch (scene)
	{
	case SCENE_TITLE:
		Title_Update();
		break;
	case SCENE_GAME:
		Game_Update();
		break;
	case SCENE_RESULT:
		break;
	case SCENE_ANM_LOGO:
		Animation_Logo_Update();
		break;
	case SCENE_ANM_OP:
		Animation_Op_Update();
		break;
	case SCENE_ANM_WIN:
		Animation_Win_Update();
		break;
	case SCENE_ANM_LOSE:
		Animation_Lose_Update();
		break;
	default:
		break;
	}
}

void Draw(void)
{
	switch (scene)
	{
	case SCENE_TITLE:
		Title_Draw();
		break;
	case SCENE_GAME:
		Game_Draw();
		break;
	case SCENE_RESULT:
		break;
	case SCENE_ANM_LOGO:
		Animation_Logo_Draw();
		break;
	case SCENE_ANM_OP:
		Animation_Op_Draw();
		break;
	case SCENE_ANM_WIN:
		Animation_Win_Draw();
		break;
	case SCENE_ANM_LOSE:
		Animation_Lose_Draw();
		break;
	default:
		break;
	}
}

void Finalize(void)
{
	switch (scene)
	{
	case SCENE_TITLE:
		Title_Finalize();
		break;
	case SCENE_GAME:
		Game_Finalize();
		break;
	case SCENE_RESULT:
		break;
	case SCENE_ANM_LOGO:
		Animation_Logo_Finalize();
		break;
	case SCENE_ANM_OP:
		Animation_Op_Finalize();
		break;
	case SCENE_ANM_WIN:
		Animation_Win_Finalize();
		break;
	case SCENE_ANM_LOSE:
		Animation_Lose_Finalize();
		break;
	default:
		break;
	}
}

void SetScene(SCENE id)
{
	Finalize();

	scene = id;

	Init();
}

SCENE GetScene(void)
{
	return scene;
}
