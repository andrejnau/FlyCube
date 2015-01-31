#pragma once

#include <testscene.h>
#include <scenebase.h>

enum class SceneType
{
	TestScene
};

class GetScene
{
public:
	static SceneBase& get(SceneType type)
	{
		switch (type)
		{
		case SceneType::TestScene:
		default:
			return TestScene::Instance();
			break;
		}
	}
private:
	GetScene() = delete;
	GetScene(const GetScene&) = delete;
	GetScene& operator=(const GetScene&) = delete;
};