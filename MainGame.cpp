#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

int DISPLAY_WIDTH = 1280;
int DISPLAY_HEIGHT = 720;
int DISPLAY_SCALE = 1;

enum Agent8State
{
	STATE_APPEAR = 0,
	STATE_ATTACHED,
	STATE_FLY,
	STATE_DEAD,
};

struct GameState
{
	int score = 0;
	Agent8State agentState = STATE_APPEAR;
	float gemCollisionTimer = 0;
	float textTimer = 0;
	int gemCount = 1;
	int attachFirstAsteroid = 0;
	int level = 0;
	int attachedID = -1;
	float meteorDirection = 0;
	float agent8Direction = 0;
	float asteroidDirection = 0;
	bool agent8LeftOrRight = 0;
	bool levelComplete = 1;
};

GameState gameState;
enum GameObjectType
{
	TYPE_NULL = -1,
	TYPE_AGENT8,
	TYPE_GEM,
	TYPE_ASTEROID,
	TYPE_BROKEN,
	TYPE_DESTROYED,
	TYPE_PARTICLE,
	TYPE_PARTICLE2,
	TYPE_METEOR,
};

void HandlePlayerControls();
void Particles(GameObject& obj_ref);
void Particles2(GameObject& obj_ref);
void WrapFlight(GameObject& obj_ref);
void UpdateAsteroid();
void UpdateAgent8();
void BreakAsteroid(Vector2D position);
void UpdateGem();
void UpdateMeteor();
void RestartLevel();

void MainGameEntry(PLAY_IGNORE_COMMAND_LINE)
{
	Play::CreateManager(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE);
	Play::CentreAllSpriteOrigins();
	Play::MoveSpriteOrigin("spr_agent8_left_strip7", -50, 0);
	Play::MoveSpriteOrigin("spr_agent8_right_strip7", -50, 0);
	Play::MoveSpriteOrigin("spr_agent8_fly", -50, 0);
	Play::MoveSpriteOrigin("spr_meteor_strip2", 50, 0);
	Play::MoveSpriteOrigin("spr_asteroid_strip_2", 17, 0);
	Play::LoadBackground("Data\\Backgrounds\\spr_background.png");
	Play::CreateGameObject(TYPE_AGENT8, { 50, 400 }, 60, "spr_agent8_right_strip7");
	Play::StartAudioLoop("music");
}

bool MainGameUpdate(float elapsedTime)
{
	gameState.gemCollisionTimer += elapsedTime;
	gameState.textTimer += elapsedTime;
	Play::DrawBackground();
	Play::DrawFontText("132px", "REMAINING GEMS: " + std::to_string(gameState.gemCount - gameState.score), { DISPLAY_WIDTH / 2, 50 }, Play::CENTRE);
	Play::DrawFontText("64px", "ARROW KEYS TO ROTATE AND SPACE TO LAUNCH", { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 20 }, Play::CENTRE);
	if (gameState.levelComplete)
		RestartLevel();
	HandlePlayerControls();
	UpdateAsteroid();
	UpdateAgent8();
	UpdateMeteor();
	UpdateGem();
	Particles(Play::GetGameObjectByType(TYPE_AGENT8));
	Particles2(Play::GetGameObjectByType(TYPE_BROKEN));

	Play::PresentDrawingBuffer();
	return Play::KeyDown(VK_ESCAPE);
}

void RestartLevel()
{
	for (int id : Play::CollectGameObjectIDsByType(TYPE_ASTEROID))
	{
		Play::DestroyGameObject(id);
	}
	for (int id : Play::CollectGameObjectIDsByType(TYPE_METEOR))
	{
		Play::DestroyGameObject(id);
	}
	for (int id : Play::CollectGameObjectIDsByType(TYPE_BROKEN))
	{
		Play::DestroyGameObject(id);
	}
	for (int id : Play::CollectGameObjectIDsByType(TYPE_GEM))
	{
		Play::DestroyGameObject(id);
	}

	gameState.level++;
	gameState.gemCount += 2;
	gameState.score = 0;
	gameState.attachFirstAsteroid = 0;

	for (int n = 0; n < gameState.gemCount; n++)
	{
		int id = Play::CreateGameObject(TYPE_ASTEROID, Vector2D(Play::RandomRoll(DISPLAY_WIDTH), Play::RandomRoll(DISPLAY_HEIGHT)), 60, "spr_asteroid");
		gameState.attachFirstAsteroid += 1;
		GameObject& obj_asteroid = Play::GetGameObject(id);
		if (gameState.attachFirstAsteroid < 2)
		{
			gameState.attachedID = id;
		}
		gameState.asteroidDirection = Play::RandomRoll(2 * PLAY_PI);
		Play::SetGameObjectDirection(obj_asteroid, 1, gameState.asteroidDirection);
		obj_asteroid.rotation = gameState.asteroidDirection;
		Play::SetSprite(obj_asteroid, "spr_asteroid_strip_2", 0.2f);
	}

	for (int n = 0; n < gameState.level; n++)
	{
		int id = Play::CreateGameObject(TYPE_METEOR, Vector2D(Play::RandomRoll(DISPLAY_WIDTH), Play::RandomRoll(DISPLAY_HEIGHT)), 40, "spr_meteor");
		GameObject& obj_meteor = Play::GetGameObject(id);
		gameState.meteorDirection = Play::RandomRoll(2 * PLAY_PI);
		Play::SetGameObjectDirection(obj_meteor, 3, gameState.meteorDirection);
		obj_meteor.rotation = gameState.meteorDirection;
		Play::SetSprite(obj_meteor, "spr_meteor_strip2", 0.2f);
	}

	gameState.levelComplete = false;
}

int MainGameExit(void)
{
	Play::DestroyManager();
	return PLAY_OK;
}

void UpdateAgent8()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	switch (gameState.agentState)
	{
	case STATE_APPEAR:
		gameState.textTimer = 0;
		obj_agent8.pos = Play::GetGameObject(gameState.attachedID).pos;
		break;
	case STATE_ATTACHED:
		obj_agent8.rotation = gameState.agent8Direction - PLAY_PI;
		obj_agent8.velocity = { 0,0 };
		break;
	case STATE_FLY:

		Play::SetSprite(obj_agent8, "spr_agent8_fly", 0);
		Play::SetGameObjectDirection(obj_agent8, 3, gameState.agent8Direction);
		if (gameState.gemCount - gameState.score == 0)
		{
			gameState.levelComplete = 1;
			gameState.agentState = STATE_APPEAR;
		}
		break;
	case STATE_DEAD:
		Play::SetSprite(obj_agent8, "spr_agent8_dead_strip2", 0.2f);

		if (!Play::IsVisible(obj_agent8))
		{
			Play::DrawFontText("132px", "PRESS SPACE TO PLAY AGAIN", { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 }, Play::CENTRE);
			Play::SetSprite(obj_agent8, "spr_agent8_fly", 0);
			if (Play::KeyPressed(VK_SPACE))
			{
				gameState.gemCount = 1;
				gameState.level = 0;
				Play::StartAudioLoop("music");
				gameState.levelComplete = 1;
				gameState.agentState = STATE_APPEAR;
			}
		}

		break;
	}

	if (gameState.textTimer < 2)
	{
		Play::DrawFontText("132px", "ROUND " + std::to_string(gameState.level), { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 }, Play::CENTRE);
	}

	if (gameState.agentState != STATE_DEAD)
		WrapFlight(obj_agent8);
	Play::UpdateGameObject(obj_agent8);
	Play::DrawObjectRotated(obj_agent8);
}

void UpdateAsteroid()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vAsteroids = Play::CollectGameObjectIDsByType(TYPE_ASTEROID);

	for (int id : vAsteroids)
	{
		GameObject& obj_asteroid = Play::GetGameObject(id);
		Play::SetSprite(obj_asteroid, "spr_asteroid_strip_2", 0.2f);

		if (Play::IsColliding(obj_asteroid, obj_agent8) && gameState.agentState != STATE_ATTACHED && gameState.agentState != STATE_DEAD)
		{
			gameState.attachedID = id;

			gameState.agentState = STATE_ATTACHED;
			obj_agent8.pos = obj_asteroid.pos;
		}
		if (gameState.agentState == STATE_ATTACHED && id == gameState.attachedID)
		{
			obj_agent8.pos = obj_asteroid.pos;
		}

		WrapFlight(obj_asteroid);

		Play::UpdateGameObject(obj_asteroid);
		Play::DrawObjectRotated(obj_asteroid);
	}

	std::vector<int> vBroken = Play::CollectGameObjectIDsByType(TYPE_BROKEN);

	for (int id : vBroken)
	{
		GameObject& obj_broken = Play::GetGameObject(id);
		Particles2(obj_broken);
		Play::UpdateGameObject(obj_broken);
		Play::DrawObject(obj_broken);

		if (!Play::IsVisible(obj_broken))
			Play::DestroyGameObject(id);
	}
}

void HandlePlayerControls()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	if (gameState.agentState == STATE_FLY)
	{
		if (Play::KeyDown(VK_RIGHT))
		{
			gameState.agent8Direction += 0.02;
			obj_agent8.rotation = gameState.agent8Direction;
		}
		if (Play::KeyDown(VK_LEFT))
		{
			gameState.agent8Direction -= 0.02;
			obj_agent8.rotation = gameState.agent8Direction;
		}
	}
	else if (gameState.agentState == STATE_ATTACHED)
	{
		if (Play::KeyDown(VK_RIGHT))
		{
			gameState.agent8Direction += 0.04;
			obj_agent8.rotation = gameState.agent8Direction - PLAY_PI;
			Play::SetSprite(obj_agent8, "spr_agent8_right_strip7", 0.333f);
			gameState.agent8LeftOrRight = 0;
		}
		else if (Play::KeyDown(VK_LEFT))
		{
			gameState.agent8Direction -= 0.04;
			obj_agent8.rotation = gameState.agent8Direction - PLAY_PI;
			Play::SetSprite(obj_agent8, "spr_agent8_left_strip7", 0.333f);
			gameState.agent8LeftOrRight = 1;
		}
		else
		{
			if (gameState.agent8LeftOrRight)
			{
				Play::SetSprite(obj_agent8, "spr_agent8_left_strip7", 0);
			}
			else
			{
				Play::SetSprite(obj_agent8, "spr_agent8_right_strip7", 0);
			}
		}

		if (Play::IsVisible(obj_agent8))
		{
			if ((obj_agent8.pos.x < DISPLAY_WIDTH && obj_agent8.pos.y < DISPLAY_HEIGHT) && (obj_agent8.pos.x > 0 && obj_agent8.pos.y > 0))
			{
				if (Play::KeyPressed(VK_SPACE))
				{
					Play::PlayAudio("explode");

					gameState.agent8Direction -= PLAY_PI;
					BreakAsteroid(Play::GetGameObject(gameState.attachedID).pos);
					Play::DestroyGameObject(gameState.attachedID);
					gameState.agentState = STATE_FLY;
				}
			}
		}
	}

	Play::UpdateGameObject(obj_agent8);
}

void Particles(GameObject& obj_ref)
{
	if (obj_ref.velocity != Vector2D(0, 0))
	{
		static int spawnCap = 0;
		if (spawnCap++ % 2 == 0)
		{
			int id = Play::CreateGameObject(TYPE_PARTICLE, obj_ref.pos + Vector2D(5, 5) * Play::RandomRollRange(-1, 1), 50, "particle");
		}
	}
	std::vector<int> vParticles = Play::CollectGameObjectIDsByType(TYPE_PARTICLE);

	for (int id : vParticles)
	{
		GameObject& obj_particle = Play::GetGameObject(id);
		if (obj_particle.scale > 0.0f)
		{
			obj_particle.scale -= 0.02f;
			Play::UpdateGameObject(obj_particle);
			Play::DrawObjectRotated(obj_particle);
		}
		else if (obj_particle.scale <= 0.0f)
		{
			obj_particle.scale = 1.0f;
			Play::DestroyGameObject(id);
		}
	}
}

void Particles2(GameObject& obj_ref)
{
	if (obj_ref.velocity != Vector2D(0, 0))
	{
		static int spawnCap = 0;
		if (spawnCap++ % 2 == 0)
		{
			int id = Play::CreateGameObject(TYPE_PARTICLE2, obj_ref.pos + Vector2D(5, 5) * Play::RandomRollRange(-1, 1), 50, "particle");
		}
	}
	std::vector<int> vParticles = Play::CollectGameObjectIDsByType(TYPE_PARTICLE2);
	for (int id : vParticles)
	{
		GameObject& obj_particle = Play::GetGameObject(id);
		if (obj_particle.scale > 0.0f)
		{
			obj_particle.scale -= 0.01f;
			Play::UpdateGameObject(obj_particle);
			Play::DrawObjectRotated(obj_particle);
		}
		if (obj_particle.scale <= 0.0f)
		{
			obj_particle.scale = 1.0f;
			Play::DestroyGameObject(id);
		}
	}
}

void BreakAsteroid(Vector2D position)
{
	for (int i = 0; i < 3; i++)
	{
		int id = Play::CreateGameObject(TYPE_BROKEN, position, 0, "spr_asteroid_pieces_strip3");
		GameObject& obj_broken = Play::GetGameObject(id);
		Play::SetGameObjectDirection(obj_broken, 10, 0 - ((2 * PLAY_PI) / 3) * i);
		obj_broken.frame = i;
	}

	int id = Play::CreateGameObject(TYPE_GEM, position, 25, "spr_gem");
	GameObject& obj_gem = Play::GetGameObject(id);
	gameState.gemCollisionTimer = 0;
}

void UpdateGem()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vGem = Play::CollectGameObjectIDsByType(TYPE_GEM);

	for (int id : vGem)
	{
		GameObject& obj_gem = Play::GetGameObject(id);
		Play::UpdateGameObject(obj_gem);
		Play::DrawObject(obj_gem);

		if (Play::IsColliding(obj_gem, obj_agent8) && gameState.gemCollisionTimer >= 0.3 && gameState.agentState != STATE_DEAD)
		{
			Play::PlayAudio("reward");
			gameState.score += 1;
			Play::DestroyGameObject(id);
		}
	}
}

void UpdateMeteor()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vMeteors = Play::CollectGameObjectIDsByType(TYPE_METEOR);

	for (int id : vMeteors)
	{
		GameObject& obj_meteor = Play::GetGameObject(id);
		Play::SetSprite(obj_meteor, "spr_meteor_strip2", 0.2f);
		if (Play::IsColliding(obj_meteor, obj_agent8) && gameState.agentState == STATE_FLY)
		{
			Play::StopAudioLoop("music");
			Play::PlayAudio("combust");
			gameState.agentState = STATE_DEAD;
		}

		WrapFlight(obj_meteor);

		Play::UpdateGameObject(obj_meteor);
		Play::DrawObjectRotated(obj_meteor);
	}
}

void WrapFlight(GameObject& obj_ref)
{
	if (Play::IsLeavingDisplayArea(obj_ref, Play::HORIZONTAL))
	{
		if (!Play::IsVisible(obj_ref))
			obj_ref.pos.x = DISPLAY_WIDTH - obj_ref.pos.x;
	}
	if (Play::IsLeavingDisplayArea(obj_ref, Play::VERTICAL))
	{
		if (!Play::IsVisible(obj_ref))
			obj_ref.pos.y = DISPLAY_HEIGHT - obj_ref.pos.y;
	}
}