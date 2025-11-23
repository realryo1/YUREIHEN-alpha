#include "scene.h"
#include "game.h"
#include "direct3d.h"
#include "keyboard.h"
#include "texture.h"

SCENE scene = SCENE_GAME;

void Init(void)
{
	switch (scene)
	{
	case SCENE_TITLE:
		break;
	case SCENE_GAME:
		Game_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
		break;
	case SCENE_RESULT:
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
		break;
	case SCENE_GAME:
		Game_Update();
		break;
	case SCENE_RESULT:
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
		break;
	case SCENE_GAME:
		Game_Draw();
		break;
	case SCENE_RESULT:
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
		break;
	case SCENE_GAME:
		Game_Finalize();
		break;
	case SCENE_RESULT:
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

