#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"

#include "../CSC8503Common/NavigationGrid.h"
#include "../CSC8503Common/NavigationMesh.h"
#include "../CSC8503Common/PushdownState.h"

#include "../CSC8503Common/BehaviourAction.h"
#include "../CSC8503Common/BehaviourSequence.h"
#include "../CSC8503Common/BehaviourSelector.h"

#include <iomanip>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>


using namespace NCL;
using namespace CSC8503;



TutorialGame::TutorialGame() {
	world = new GameWorld();
	renderer = new GameTechRenderer(*world);
	physics = new PhysicsSystem(*world);

	testStateObject = nullptr;
	upDownStateObject = nullptr;
	player = nullptr;
	enemy = nullptr;
	coinMinerAI = nullptr;
	behavCoin = nullptr;
	apple = nullptr;
	bonusBall = nullptr;
	movingSphere = nullptr;

	forceMagnitude = 10.0f;
	useGravity = false;
	inSelectionMode = false;
	displayPath = false;

	playerScore = 1000;
	enemyScore = 1000;
	timer = 0;
	spawntimer = 0;
	delSpawns = 0;
	enemyAITimer = 0;
	enemytimer = 0;
	behavTimer = 0;
	loseGame = false;
	winGame = false;
	once = false;
	movePlayer = false;
	practiceMode = false;
	testBehavOnce = false;
	goToCoin = false;
	turnOnAI = false;
	getCoin = false;

	enemyState = EnemyState::IDLE;
	spawnedCoins = false;
	restart = false;

	Debug::SetRenderer(renderer);

	InitialiseAssets();
}


/*

Each of the little demo scenarios used in the game uses the same 2 meshes,
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh", &cubeMesh);
	loadFunc("sphere.msh", &sphereMesh);
	loadFunc("Male1.msh", &charMeshA);
	loadFunc("courier.msh", &charMeshB);
	loadFunc("security.msh", &enemyMesh);
	loadFunc("coin.msh", &bonusMesh);
	loadFunc("capsule.msh", &capsuleMesh);

	basicTex = (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	InitWorld();
}

TutorialGame::~TutorialGame() {
	delete cubeMesh;
	delete sphereMesh;
	delete charMeshA;
	delete charMeshB;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;

	delete player; /////////////////
	delete enemy;
}

void TutorialGame::UpdateGame(float dt) {

	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}

	UpdateKeys();


	if (!winGame && !loseGame) {
		if (useGravity) {
			Debug::Print("(G)ravity on", Vector2(5, 95));
			physics->UseGravity(useGravity);
		}
		else {
			Debug::Print("(G)ravity off", Vector2(5, 95));
		}
	}

	if (practiceMode) {
		playerScore = 999999;
	}

	TextOnScreen(dt);
	LoseGame(dt);
	WinGame(dt);


	SelectObject();
	MoveSelectedObject();
	physics->Update(dt);

	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetPosition();
		//Vector3 camPos = objPos + lockedOffset;
		Vector3 camPos = objPos + Vector3(0, 20, 60);;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x);
		world->GetMainCamera()->SetYaw(angles.y);

		//Debug::DrawAxisLines(lockedObject->GetTransform().GetMatrix(), 2.0f);
	}


	if (movePlayer == true) {
		Vector3 objPos = player->GetTransform().GetPosition();
		Quaternion orientation = player->GetTransform().GetOrientation();
		Vector3 camPos = objPos + Vector3(0, 45, 60); //higher z = farther back camera

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler();
		Vector3 orien = orientation.ToEuler();
		world->GetMainCamera()->SetPosition(camPos);
		//world->GetMainCamera()->SetPitch(angles.x + 10);

		//world->GetMainCamera()->SetYaw(angles.y + orien.y); //moving camera with player
		//world->GetMainCamera()->SetYaw(angles.y);

	}


	if (testStateObject) {
		testStateObject->Update(dt);
	}

	if (upDownStateObject) {
		upDownStateObject->Update(dt);
	}


	if (!practiceMode || (practiceMode && turnOnAI == true)) {
		EnemyAI(dt); //enemy only moves in game mode

		behavTimer += dt;
		if (behavTimer < 5) {
			CoinMineMovement(dt);
		}

		if (behavTimer >= 6 && testBehavOnce == false) {
			coinMinerAI->GetPhysicsObject()->ClearForces();
			TestBehaviourTree(dt);
			testBehavOnce = true;
		}
		if (behavTimer >= 9) {
			testBehavOnce = false;
			behavTimer = 0;
		}
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM9)) {
		turnOnAI = !turnOnAI;
	}

	//spinning coins
	for (auto i : bonus) {
		i->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
	}


	FallenOffStage();
	PlaneBonusIntersection();

	SpawnFallingObject(dt);


	world->UpdateWorld(dt);
	renderer->Update(dt);

	Debug::FlushRenderables(dt);
	renderer->Render();

}


void TutorialGame::LoseGame(float dt) {

	if (playerScore <= 0 && once == false) {

		world = new GameWorld();
		renderer = new GameTechRenderer(*world);
		loseGame = true;
		once = true;
		playerScore = 0;

		if (physics->enemy1 == true && physics->collectedBonusBall == true) {
			enemyScore += 1000;
			std::cout << "Enemy gets bonus score of 1000\n";
		}
	}

	if (once == true && loseGame == true) {
		renderer->DrawString("YOU LOSE!", Vector2(40, 20), 50, Vector4(0.9, 0.1, 0.6, 1));
		renderer->DrawString("Your Score: " + std::to_string(playerScore), Vector2(10, 40), 40);
		renderer->DrawString("Player 2 Score: " + std::to_string(enemyScore), Vector2(10, 50), 40);
		renderer->DrawString("R to restart game", Vector2(10, 70), 30);
		renderer->DrawString("Esc to go back to main menu", Vector2(10, 80), 30);
		renderer->Update(dt);
		world->UpdateWorld(dt);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::R)) {
		restart = true;
	}

}

void TutorialGame::WinGame(float dt) {

	float offset = 10.0f;
	if (player->GetTransform().GetPosition().x <= 0 + offset && player->GetTransform().GetPosition().x >= 0 - offset &&
		player->GetTransform().GetPosition().z <= -90 + offset && player->GetTransform().GetPosition().z >= -90 - offset) {

		if (playerScore > 0 && once == false) {
			world = new GameWorld();
			renderer = new GameTechRenderer(*world);
			winGame = true;
			once = true;

			if (physics->player1 == true && physics->collectedBonusBall == true) {
				playerScore += 1000;
				std::cout << "Player gets bonus score of 1000\n";
			}
			if (physics->enemy1 == true && physics->collectedBonusBall == true) {
				enemyScore += 1000;
				std::cout << "Enemy gets bonus score of 1000\n";
			}
		}

		if (once == true && winGame == true) {
			renderer->DrawString("YOU WIN!", Vector2(40, 20), 50, Vector4(0.9, 0.1, 0.6, 1));
			renderer->DrawString("Your Score: " + std::to_string(playerScore), Vector2(10, 40), 40);
			renderer->DrawString("Player 2 Score: " + std::to_string(enemyScore), Vector2(10, 50), 40);
			renderer->DrawString("R to restart game", Vector2(10, 70), 30);
			renderer->DrawString("Esc to go back to main menu", Vector2(10, 80), 30);
			renderer->Update(dt);
			world->UpdateWorld(dt);
		}

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::R)) {
			restart = true;
		}
	}
}



void TutorialGame::PlaneBonusIntersection() {
	Vector3 plane = Vector3(0, -20, 0);
	Vector3 normal = plane.Normalised();
	float r = 1;
	Plane* p = new Plane(plane, r);


	if (p->SphereInPlane(bonusBall->GetTransform().GetPosition(), r)) {
		//std::cout<< p->GetPointOnPlane()<<"\n";
		bonusBall->GetTransform().SetPosition(Vector3(50, 5, 50));
	}


	for (auto i : fallingSpawns) {
		if (i->GetName() == "cube") {
			Vector3 temp = i->GetRenderObject()->GetTransform()->GetScale() / 2;

			if (p->AABBInPlane(i->GetTransform().GetPosition(), temp)) {
				float x = rand() % 30 + 45;
				float zCube = rand() % -15 + -45;
				float y = 60;
				i->GetTransform().SetPosition(Vector3(x, y, zCube));
			}
		}
	}

}

void TutorialGame::FallenOffStage() {

	Vector3 playerPos = player->GetTransform().GetPosition();
	if (playerPos.y <= -20 || playerPos.y >= 200) {
		float x = 0;
		float z = 0;


		if (playerPos.x < -100) {
			x = -85;
		}
		if (playerPos.x > 100) {
			x = 85;
		}
		if (playerPos.x > -100 && playerPos.x < 100) {
			x = playerPos.x;
		}
		if (playerPos.z < -100) {
			z = -85;
		}
		if (playerPos.z > 100) {
			z = 85;
		}
		if (playerPos.z > -100 && playerPos.z < 100) {
			z = playerPos.z;
		}
		player->GetPhysicsObject()->ClearForces();
		player->GetTransform().SetPosition(Vector3(x, 5, z));

	}


	Vector3 enemyPos = enemy->GetTransform().GetPosition();
	if (enemyPos.y <= -20 || enemyPos.y >= 200) {
		float x = 0;
		float z = 0;


		if (enemyPos.x < -100) {
			x = -85;
		}
		if (enemyPos.x > 100) {
			x = 85;
		}
		if (enemyPos.x > -100 && enemyPos.x < 100) {
			x = enemyPos.x;
		}
		if (enemyPos.z < -100) {
			z = -85;
		}
		if (enemyPos.z > 100) {
			z = 85;
		}
		if (enemyPos.z > -100 && enemyPos.z < 100) {
			z = enemyPos.z;
		}
		enemy->GetPhysicsObject()->ClearForces();
		enemy->GetTransform().SetPosition(Vector3(x, 5, z));

	}


	Vector3 minerPos = coinMinerAI->GetTransform().GetPosition();
	if (minerPos.y <= -20 || minerPos.y >= 100) {
		float x = 0;
		float z = 0;

		if (minerPos.x < -100) {
			x = -85;
		}
		if (minerPos.x > 100) {
			x = 85;
		}
		if (minerPos.x > -100 && minerPos.x < 100) {
			x = minerPos.x;
		}
		if (minerPos.z < -100) {
			z = -85;
		}
		if (minerPos.z > 100) {
			z = 85;
		}
		if (minerPos.z > -100 && minerPos.z < 100) {
			z = minerPos.z;
		}
		coinMinerAI->GetPhysicsObject()->ClearForces();
		coinMinerAI->GetTransform().SetPosition(Vector3(x, 5, z));

	}

}


void TutorialGame::TextOnScreen(float dt) {
	timer += dt;
	decreaseScore += dt;
	std::stringstream stream;
	stream << "Timer: " << std::fixed << std::setprecision(1) << timer;
	std::string time = stream.str();

	Debug::Print(time, Vector2(80, 6), 25, Vector4(1, 0.8, 0, 1));


	if (decreaseScore >= 1 && winGame == false && loseGame == false) {

		playerScore -= 10;
		enemyScore -= 10;

		if (playerScore <= 0) {
			playerScore = 0;
		}
		if (enemyScore <= 0) {
			enemyScore = 0;
		}

		decreaseScore = 0;
	}

	if (physics->getPBonus() == true) {
		physics->setPBonus(false);
		playerScore += 25;
		std::cout << "Player +25 score\n";
	}
	if (physics->getEBonus() == true) {
		physics->setEbonus(false);
		enemyScore += 25;
		std::cout << "Enemy +25 score\n";
	}


	std::string pScore = "You: " + std::to_string(playerScore);
	std::string eScore = "Player 2: " + std::to_string(enemyScore);

	Debug::Print(pScore, Vector2(1, 6), 30, Vector4(0, 0.88, 0.01, 1));
	Debug::Print(eScore, Vector2(1, 12), 30, Vector4(1, 0.08, 0, 1));

}


GameObject* TutorialGame::AddJumpPad(const Vector3& position, const Vector3& size) {
	GameObject* jumpPad = new GameObject();

	Vector3 dimensions = Vector3(5, 2, 5);
	AABBVolume* volume = new AABBVolume(size);

	jumpPad->SetBoundingVolume((CollisionVolume*)volume);

	jumpPad->GetTransform()
		.SetPosition(position)
		.SetScale(size * 2);

	jumpPad->SetName("jumppad");

	jumpPad->SetRenderObject(new RenderObject(&jumpPad->GetTransform(), cubeMesh, basicTex, basicShader));
	jumpPad->SetPhysicsObject(new PhysicsObject(&jumpPad->GetTransform(), jumpPad->GetBoundingVolume()));

	jumpPad->GetPhysicsObject()->SetInverseMass(0);
	/////
	jumpPad->GetPhysicsObject()->SetCRes(1.0f);  //higher number more elastic
	jumpPad->GetPhysicsObject()->InitCubeInertia();

	jumpPad->GetPhysicsObject()->SetCollisonType(CollisionType::JUMPPAD);
	jumpPad->GetPhysicsObject()->SetState(ObjectState::STATIC);

	jumpPad->GetRenderObject()->SetColour(Vector4(2, 0.6, 0, 1));
	jumpPad->GetRenderObject()->SetOriColour(Vector4(2, 0.6, 0, 1));

	world->AddGameObject(jumpPad);

	return jumpPad;
}

GameObject* TutorialGame::SlowFloor() {
	GameObject* slowfloor = new GameObject();

	Vector3 dimensions = Vector3(20, 0.03, 50);
	AABBVolume* volume = new AABBVolume(dimensions);

	slowfloor->SetBoundingVolume((CollisionVolume*)volume);

	slowfloor->GetTransform()
		.SetPosition(Vector3(0, 0, -20))
		.SetScale(dimensions * 2);

	slowfloor->SetName("slowfloor");

	slowfloor->SetRenderObject(new RenderObject(&slowfloor->GetTransform(), cubeMesh, basicTex, basicShader));
	slowfloor->SetPhysicsObject(new PhysicsObject(&slowfloor->GetTransform(), slowfloor->GetBoundingVolume()));

	slowfloor->GetPhysicsObject()->SetInverseMass(0);
	slowfloor->GetPhysicsObject()->SetCRes(0.0f);  //higher number more elastic
	slowfloor->GetPhysicsObject()->InitCubeInertia();

	slowfloor->GetPhysicsObject()->SetCollisonType(CollisionType::FLOOR);
	slowfloor->GetPhysicsObject()->SetState(ObjectState::STATIC);

	slowfloor->GetRenderObject()->SetColour(Vector4(0.43, 0.22, 0, 1));
	slowfloor->GetRenderObject()->SetOriColour(Vector4(0.43, 0.22, 0, 1));
	slowfloor->GetRenderObject()->SetDefaultTexture(nullptr);

	world->AddGameObject(slowfloor);

	return slowfloor;
}

GameObject* TutorialGame::FastFloor(const Vector3& position, const Vector3& size) {
	GameObject* fastFloor = new GameObject();

	AABBVolume* volume = new AABBVolume(size);

	fastFloor->SetBoundingVolume((CollisionVolume*)volume);

	fastFloor->GetTransform()
		.SetPosition(position)
		.SetScale(size * 2);

	fastFloor->SetName("fastfloor");

	fastFloor->SetRenderObject(new RenderObject(&fastFloor->GetTransform(), cubeMesh, basicTex, basicShader));
	fastFloor->SetPhysicsObject(new PhysicsObject(&fastFloor->GetTransform(), fastFloor->GetBoundingVolume()));

	fastFloor->GetPhysicsObject()->SetInverseMass(0);
	fastFloor->GetPhysicsObject()->SetCRes(0.0f);  //higher number more elastic
	fastFloor->GetPhysicsObject()->InitCubeInertia();

	fastFloor->GetPhysicsObject()->SetCollisonType(CollisionType::FLOOR);
	fastFloor->GetPhysicsObject()->SetState(ObjectState::STATIC);

	fastFloor->GetRenderObject()->SetColour(Vector4(1.2, 2.16, 2.2, 1));
	fastFloor->GetRenderObject()->SetOriColour(Vector4(1.2, 2.16, 2.2, 1));
	fastFloor->GetRenderObject()->SetDefaultTexture(nullptr);

	world->AddGameObject(fastFloor);

	return fastFloor;
}

GameObject* TutorialGame::FloatingFloor(const Vector3& position, const Vector3& size) {
	GameObject* floatFloor = new GameObject();

	AABBVolume* volume = new AABBVolume(size);

	floatFloor->SetBoundingVolume((CollisionVolume*)volume);

	floatFloor->GetTransform()
		.SetPosition(position)
		.SetScale(size * 2);

	floatFloor->SetName("floor");

	floatFloor->SetRenderObject(new RenderObject(&floatFloor->GetTransform(), cubeMesh, basicTex, basicShader));
	floatFloor->SetPhysicsObject(new PhysicsObject(&floatFloor->GetTransform(), floatFloor->GetBoundingVolume()));

	floatFloor->GetPhysicsObject()->SetInverseMass(0);
	floatFloor->GetPhysicsObject()->SetCRes(0.0f);  //higher number more elastic
	floatFloor->GetPhysicsObject()->InitCubeInertia();

	floatFloor->GetPhysicsObject()->SetCollisonType(CollisionType::FLOOR);
	floatFloor->GetPhysicsObject()->SetState(ObjectState::STATIC);

	floatFloor->GetRenderObject()->SetColour(Vector4(1, 0.71, 0.96, 1));
	floatFloor->GetRenderObject()->SetOriColour(Vector4(1, 0.71, 0.96, 1));

	world->AddGameObject(floatFloor);

	return floatFloor;
}

GameObject* TutorialGame::OBBFloor(const Vector3& position, const Vector3& size) {
	GameObject* obbFloor = new GameObject();

	OBBVolume* volume = new OBBVolume(size);

	obbFloor->SetBoundingVolume((CollisionVolume*)volume);

	obbFloor->GetTransform()
		.SetPosition(position)
		.SetScale(size * 2);

	obbFloor->SetName("obbfloor");

	obbFloor->SetRenderObject(new RenderObject(&obbFloor->GetTransform(), cubeMesh, basicTex, basicShader));
	obbFloor->SetPhysicsObject(new PhysicsObject(&obbFloor->GetTransform(), obbFloor->GetBoundingVolume()));

	obbFloor->GetPhysicsObject()->SetInverseMass(0.0);
	obbFloor->GetPhysicsObject()->SetCRes(0.8f);  //higher number more elastic
	obbFloor->GetPhysicsObject()->InitCubeInertia();
	obbFloor->GetTransform().SetOrientation(Quaternion(0.25, 0, 0, 1));

	obbFloor->GetPhysicsObject()->SetCollisonType(CollisionType::FLOOR);
	obbFloor->GetPhysicsObject()->SetState(ObjectState::DYNAMIC);

	obbFloor->GetRenderObject()->SetColour(Vector4(1, 0.71, 0.96, 1));
	obbFloor->GetRenderObject()->SetOriColour(Vector4(1, 0.71, 0.96, 1));

	world->AddGameObject(obbFloor);

	return obbFloor;
}


void TutorialGame::SpawnFallingObject(float dt) {
	spawntimer += dt;
	delSpawns += dt;

	if (spawntimer >= 5) {
		int random = rand() % 3;
		float x = rand() % 30 + 45;
		float z = -70;
		float zCube = rand() % -15 + -45;
		float y = 60;

		float rad = 0;
		float dimension = 0;
		float halfHeight = 0;
		float radius = 0;

		switch (random) {
		case 0:
			rad = rand() % 2 + 1;
			fallingSpawns.push_back(AddSphereToWorld(Vector3(x, y, z), rad));
			break;
		case 1:
			dimension = rand() % 2 + 1;
			fallingSpawns.push_back(AddCubeToWorld(Vector3(x, y, zCube), Vector3(dimension, dimension, dimension)));
			break;
		case 2:
			halfHeight = rand() % 3 + 2.55;
			radius = rand() % 3 + 0.9;
			fallingSpawns.push_back(AddCapsuleToWorld(Vector3(x, y, z), halfHeight, radius));
			break;
		}

		spawntimer = 0;
	}

	for (auto i : fallingSpawns) {
		if (i->GetName() == "cube") {
			i->GetRenderObject()->SetDefaultTexture(basicTex);
		}
	}


	if (delSpawns >= 60 && !fallingSpawns.empty()) {
		for (auto i : fallingSpawns) {
			if (i == fallingSpawns.front()) {
				i->GetTransform().SetPosition(Vector3(800, 600, 500));
				i->SetIsActive(false);
			}
		}
		fallingSpawns.erase(fallingSpawns.begin());
		delSpawns = 0;
	}
}


void TutorialGame::EnemyAI(float dt) {

	enemytimer += dt;
	float offset = 6;
	Vector3 enemyPos = enemy->GetTransform().GetPosition();
	float force = 70;

	if (physics->slowfloorE == true) {
		force = 50;
	}
	if (physics->fastFloorE == true) {
		force = 250.0f;
	}

	//going for coin
	float coinDistance = 15;
	for (auto i : bonus) {
		if ((enemyPos.x - coinDistance <= i->GetTransform().GetPosition().x && enemyPos.x + coinDistance >= i->GetTransform().GetPosition().x &&
			enemyPos.z - coinDistance <= i->GetTransform().GetPosition().z && enemyPos.z + coinDistance >= i->GetTransform().GetPosition().z)) {
			Vector3 coinPos = i->GetTransform().GetPosition();
			Vector3 direction = Vector3(coinPos.x, 0, coinPos.z) - enemyPos;
			direction.Normalise();
			enemy->GetPhysicsObject()->AddForce(direction * force*3);
			//std::cout << "Going after coin\n";
			getCoin = true;
		}
	}



	//going for bonusBall
	if (physics->enemy1 == false && (enemyPos.x - coinDistance <= bonusBall->GetTransform().GetPosition().x && enemyPos.x + coinDistance >= bonusBall->GetTransform().GetPosition().x &&
		enemyPos.z - coinDistance <= bonusBall->GetTransform().GetPosition().z && enemyPos.z + coinDistance >= bonusBall->GetTransform().GetPosition().z)) {
		Vector3 direction = bonusBall->GetTransform().GetPosition() - enemyPos;
		direction.Normalise();
		enemy->GetPhysicsObject()->AddForce(direction * force);
		std::cout << "Going after bonusBall\n";
		physics->enemy1 = true;
	}


	else {
		if (enemyPos.x < testNodes.back().x + offset && enemyPos.x > testNodes.back().x - offset &&
			enemyPos.z < testNodes.back().z + offset && enemyPos.z > testNodes.back().z - offset && enemytimer < 5) {
			enemyPos.y = 0;

			if (testNodes.size() >= 2) {
				testNodes.pop_back();
			}
			enemytimer = 0;
		}

		if (enemytimer >= 5) { //recalculate path
			testNodes.clear();
			Vector3 tempPos = Vector3(enemyPos.x + 95, enemyPos.y, enemyPos.z + 95);
			NavigationPath path;
			NavigationGrid grid("TestGrid1.txt");
			bool found = grid.FindPath(Vector3(95, 0, 5), tempPos, path);

			Vector3 pos;
			while (path.PopWaypoint(pos)) {
				testNodes.push_back(pos);
			}

			if ((enemytimer >5.5 && physics->slowfloorE == false && getCoin == false) || !found  ) {
			//if ((enemytimer >8  && getCoin == false) || enemytimer > 20||!found  ) {
				testNodes.clear();
				std::cout << "Stuck, teleporting near player!\n";
				enemy->GetTransform().SetPosition(player->GetTransform().GetPosition() + Vector3(5, 0, 5));
				tempPos = Vector3(enemy->GetTransform().GetPosition().x + 95, enemy->GetTransform().GetPosition().y, enemy->GetTransform().GetPosition().z + 95);
				found = grid.FindPath(Vector3(95, 0, 5), tempPos, path);

				while (path.PopWaypoint(pos)) {
					testNodes.push_back(pos);
				}

			}

			if (found) {
				std::cout << "Recalculating Path\n";
				enemy->GetTransform().SetPosition(Vector3(enemyPos.x, 4, enemyPos.z));
				DisplayPathfinding();
			}


			enemytimer = 0;
		}

		Vector3 direction = testNodes.back() - enemyPos;
		direction.Normalise();
		enemy->GetPhysicsObject()->AddForce(direction * force);
		
	}

	getCoin = false;
}

void TutorialGame::CoinMineMovement(float dt) {

	enemyAITimer += dt;
	coinPos;
	Vector3 coinMinerPos = coinMinerAI->GetTransform().GetPosition();
	float offset = 6;
	float force = 75;

	if (physics->slowfloorE == true) {
		force = 45;
	}
	if (physics->fastFloorE == true) {
		force = 250.0f;
	}


	if (goToCoin == true) {
		Vector3 direction;

		direction = coinPos - coinMinerPos;
		direction.Normalise();
		coinMinerAI->GetPhysicsObject()->AddForce(direction * force*2);
		goToCoin = false;
	}

	else {
		if (coinMinerPos.x < testNodeCopy.back().x + offset && coinMinerPos.x > testNodeCopy.back().x - offset &&
			coinMinerPos.z < testNodeCopy.back().z + offset && coinMinerPos.z > testNodeCopy.back().z - offset && enemyAITimer < 5) {
			coinMinerPos.y = 0;

			if (testNodeCopy.size() >= 2) {
				testNodeCopy.pop_back();
			}
			enemyAITimer = 0;
		}

		if ((enemyAITimer >= 4 && physics->slowfloorE == false) || (enemyAITimer >= 4 && physics->slowfloorE == true)) { //recalculate path
			testNodeCopy.clear();
			Vector3 tempPos = Vector3(coinMinerPos.x + 95, coinMinerPos.y, coinMinerPos.z + 95);
			NavigationPath path;
			NavigationGrid grid("TestGrid1.txt");
			bool found = grid.FindPath(Vector3(95, 0, 5), tempPos, path);

			Vector3 pos;
			while (path.PopWaypoint(pos)) {
				testNodeCopy.push_back(pos);
			}

			if (!found || enemyAITimer > 4) {
				testNodeCopy.clear();
				//std::cout << "Stuck, teleporting near player!\n";
				coinMinerAI->GetTransform().SetPosition(player->GetTransform().GetPosition() + Vector3(5, 0, 5));
				tempPos = Vector3(coinMinerAI->GetTransform().GetPosition().x + 95, coinMinerAI->GetTransform().GetPosition().y, coinMinerAI->GetTransform().GetPosition().z + 95);
				found = grid.FindPath(Vector3(95, 0, 5), tempPos, path);

				while (path.PopWaypoint(pos)) {
					testNodeCopy.push_back(pos);
				}

			}

			if (found) {
				//std::cout << "Recalculating Path\n";
				coinMinerAI->GetTransform().SetPosition(Vector3(coinMinerPos.x, 4, coinMinerPos.z));
				DisplayPathfinding();
			}


			enemyAITimer = 0;
		}

		Vector3 direction = testNodeCopy.back() - coinMinerPos;
		direction.Normalise();
		coinMinerAI->GetPhysicsObject()->AddForce(direction * force);
		enemyAITimer = 0;
	}
}


void TutorialGame::UpdateKeys() {

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		selectionObject = nullptr;
		lockedObject = nullptr;
		playerScore = 1000;
		enemyScore = 1000;
		timer = 0;
		loseGame = false;
		winGame = false;
		once = false;
		bonus.clear();
		pingpong.clear();
		fallingSpawns.clear();
		InitWorld(); //We can reset the simulation at any time with F1
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}




	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}

	/// <summary>
	/// //////////////////////////
	/// </summary>

	if (!practiceMode) {
		movePlayer = true;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUMPAD0)) {
		movePlayer = !movePlayer;
		selectionObject = false;
		lockedObject = false;
	}
	if (movePlayer == true) {
		MovePlayer();
	}



	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM0)) {
		displayPath = !displayPath;
	}
	if (displayPath == true) {
		DisplayPathfinding();
	}
}


void TutorialGame::LockedObjectMovement() {
	if (lockedObject->GetName() != "player") {

		lockedObject->GetPhysicsObject()->ClearForces();

		Matrix4 view = world->GetMainCamera()->BuildViewMatrix();
		Matrix4 camWorld = view.Inverse();

		Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

		//forward is more tricky -  camera forward is 'into' the screen...
		//so we can take a guess, and use the cross of straight up, and
		//the right axis, to hopefully get a vector that's good enough!

		Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
		fwdAxis.y = 0.0f;
		fwdAxis.Normalise();

		Vector3 charForward = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);
		Vector3 charForward2 = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);

		float force = 100.0f;

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			lockedObject->GetPhysicsObject()->AddForce(-rightAxis * force);
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			Vector3 worldPos = selectionObject->GetTransform().GetPosition();
			lockedObject->GetPhysicsObject()->AddForce(rightAxis * force);
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			lockedObject->GetPhysicsObject()->AddForce(fwdAxis * force);
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			lockedObject->GetPhysicsObject()->AddForce(-fwdAxis * force);
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD2)) {
			lockedObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD8)) {
			lockedObject->GetPhysicsObject()->AddForce(Vector3(0, 10, 0));
		}
	}

}

void TutorialGame::DebugObjectMovement() {
	//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject && (selectionObject->GetName() != "player")) {

		selectionObject->GetPhysicsObject()->ClearForces();

		//Debug::DrawLine(Vector3(0, 10, 0), Vector3(0, 10, 0.3f * 3), Vector4(1, 0, 0, 1));  //line from initial camera pos to object

		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD2)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD4)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD6)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}


		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 10, 0));
		}
	}

}

void TutorialGame::MovePlayer() {

	player->GetPhysicsObject()->ClearForces();
	//movement
	Matrix4 view = world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld = view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0));
	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();

	Vector3 charForward = player->GetTransform().GetOrientation() * Vector3(0, 0, 1);
	float force = 200;

	if (physics->slowfloorP == true) {
		force = 40.0f;
	}
	if (physics->fastFloorP == true) {
		force = 800.0f;
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::A)) {
		player->GetPhysicsObject()->AddForce(-rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::D)) {
		player->GetPhysicsObject()->AddForce(rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::W)) {
		player->GetPhysicsObject()->AddForce(fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::S)) {
		player->GetPhysicsObject()->AddForce(-fwdAxis * force);
	}


	//rotate player
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM1)) {
		player->GetPhysicsObject()->AddTorque(Vector3(0, 20, 0));
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM3)) {
		player->GetPhysicsObject()->AddTorque(Vector3(0, -20, 0));
	}



	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::SPACE) && physics->jump == true) {

		player->GetPhysicsObject()->AddForce(Vector3(0, 60 * force, 0));
		physics->jump = false;
	}
	if (player->GetPhysicsObject()->GetForce().y > force) { //pulling player down faster
		player->GetPhysicsObject()->AddForce(Vector3(0, -9.8 * force, 0));
	}

	if (physics->jumpforce == true) { //jumping on jumppad
		player->GetPhysicsObject()->AddForce(Vector3(0, force * 150, 0));
		physics->jumpforce = false;
	}



}



void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-20.0f);
	world->GetMainCamera()->SetYaw(360.0f);
	world->GetMainCamera()->SetPosition(Vector3(0, 40, 100));
	lockedObject = nullptr;
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	BridgeConstraintTest();
	BallConstraint();

	InitGameExamples();
	Walls();

	InitBehavTreeExample();

	//InitCubeGridWorld(5,5,10,10,Vector3(5,5,5));
	//InitSphereGridWorld(5, 5, 10, 10, 10);

	InitDefaultFloor();

	TestPathfinding();

	enemy->GetTransform().SetPosition(testNodes.back());

}

void TutorialGame::Walls() {

	Vector3 cubeSize = Vector3(5, 5, 40);
	AddCubeToWorld(Vector3(95, 5, 60), Vector3(5, 5, 40), 0);
	AddCubeToWorld(Vector3(80, 5, 30), Vector3(10, 5, 10), 0);
	AddCubeToWorld(Vector3(55, 5, 85), Vector3(5, 5, 15), 0);
	AddCubeToWorld(Vector3(-20, 5, 95), Vector3(70, 5, 5), 0);
	AddCubeToWorld(Vector3(-95, 5, 75), Vector3(5, 5, 25), 0);

	AddCubeToWorld(Vector3(-95, 5, -30), Vector3(5, 5, 70), 0);
	AddCubeToWorld(Vector3(-55, 5, -95), Vector3(35, 5, 5), 0);
	AddCubeToWorld(Vector3(-15, 5, -90), Vector3(5, 5, 10), 0);
	AddCubeToWorld(Vector3(15, 5, -90), Vector3(5, 5, 10), 0);
	AddCubeToWorld(Vector3(95, 5, -45), Vector3(5, 5, 55), 0);

	AddCubeToWorld(Vector3(85, 5, -5), Vector3(10, 5, 15), 0);
	AddCubeToWorld(Vector3(45, 5, 0), Vector3(5, 5, 10), 0);
	AddCubeToWorld(Vector3(35, 5, -15), Vector3(15, 5, 5), 0);
	AddCubeToWorld(Vector3(25, 5, -45), Vector3(5, 5, 25), 0);
	AddCubeToWorld(Vector3(30, 5, -85), Vector3(10, 5, 5), 0);

	AddCubeToWorld(Vector3(45, 5, -70), Vector3(5, 5, 10), 0);
	AddCubeToWorld(Vector3(60, 5, -65), Vector3(10, 5, 5), 0);
	AddCubeToWorld(Vector3(75, 5, -70), Vector3(5, 5, 10), 0);
	AddCubeToWorld(Vector3(-55, 5, 65), Vector3(25, 5, 5), 0);
	AddCubeToWorld(Vector3(-65, 5, 55), Vector3(15, 5, 5), 0);

	AddCubeToWorld(Vector3(-35, 5, 45), Vector3(5, 5, 15), 0);
	AddCubeToWorld(Vector3(-25, 5, 10), Vector3(5, 5, 30), 0);
	AddCubeToWorld(Vector3(-30, 5, -65), Vector3(10, 5, 5), 0);
	AddCubeToWorld(Vector3(-30, 5, -85), Vector3(10, 5, 5), 0);
	AddCubeToWorld(Vector3(-35, 5, 10), Vector3(5, 5, 10), 0);

	AddCubeToWorld(Vector3(-50, 5, 5), Vector3(10, 5, 5), 0);
	AddCubeToWorld(Vector3(-55, 5, -10), Vector3(5, 5, 10), 0);
	AddCubeToWorld(Vector3(-70, 5, 25), Vector3(20, 5, 5), 0);
	AddCubeToWorld(Vector3(-65, 5, 35), Vector3(15, 5, 5), 0);
	AddCubeToWorld(Vector3(-25, 5, -45), Vector3(5, 5, 15), 0);

	AddCubeToWorld(Vector3(0, 5, -102.5), Vector3(10, 5, 2.5), 0);



}


//tut 8 Constraints and solvers
void TutorialGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(10, 1.25, 5);

	float invCubeMass = 5; // how heavy the middle pieces are
	int numLinks = 6;
	float maxDistance = 17; // constraint distance
	float cubeDistance = 15; // distance between links

	Vector3 startPos = Vector3(0, 20, 35);

	//infinite mass, start and end of bridge
	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);
	start->SetName("floor");
	start->GetPhysicsObject()->SetCRes(0.1);
	start->GetRenderObject()->SetDefaultTexture(basicTex);
	start->GetRenderObject()->SetColour(Vector4(0.98, 0.94, 0.28, 1));
	start->GetRenderObject()->SetOriColour(Vector4(0.98, 0.94, 0.28, 1));

	GameObject* end = AddCubeToWorld(startPos - Vector3(0, 0, (numLinks + 2) * cubeDistance), cubeSize, 0);
	end->SetName("floor");
	end->GetPhysicsObject()->SetCRes(0.1);
	end->GetRenderObject()->SetDefaultTexture(basicTex);
	end->GetRenderObject()->SetColour(Vector4(0.98, 0.94, 0.28, 1));
	end->GetRenderObject()->SetOriColour(Vector4(0.98, 0.94, 0.28, 1));

	GameObject* previous = start;

	for (int i = 0; i < numLinks; ++i) {
		GameObject* block = AddCubeToWorld(startPos - Vector3(0, 0, (i + 1) * cubeDistance), cubeSize, invCubeMass);
		block->SetName("floor");
		block->GetPhysicsObject()->SetCRes(0.1);
		block->GetRenderObject()->SetColour(Vector4(0.98, 0.94, 0.28, 1));
		block->GetRenderObject()->SetOriColour(Vector4(0.98, 0.94, 0.28, 1));
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}

	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance); //take out end if want to make rope
	world->AddConstraint(constraint);

}

void TutorialGame::BallConstraint() {
	Vector3 cubeSize = Vector3(5, 1.25, 10);

	float invMass = 2; // how heavy the middle pieces are
	int numLinks = 1;
	float maxDistance = 40; //length of entire thing
	float cubeDistance = 5; //invisible rope

	Vector3 startPos = Vector3(-75, 50, -10);

	//infinite mass, start and end of bridge
	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);
	start->SetName("floor");
	start->GetPhysicsObject()->SetCRes(0.1);
	start->GetRenderObject()->SetDefaultTexture(basicTex);
	start->GetRenderObject()->SetColour(Vector4(2, 1.4, 1.92, 1));
	start->GetRenderObject()->SetOriColour(Vector4(2, 1.4, 1.92, 1));

	GameObject* previous = start;

	for (int i = 0; i < numLinks; ++i) {
		movingSphere = AddSphereToWorld(startPos - Vector3(0, (i + 1) * cubeDistance, 0), 10, invMass);
		movingSphere->GetPhysicsObject()->SetCRes(0.2);
		movingSphere->GetRenderObject()->SetColour(Vector4(0.5, 0, 2, 1));
		movingSphere->GetRenderObject()->SetOriColour(Vector4(0.5, 0, 2, 1));

		PositionConstraint* constraint = new PositionConstraint(previous, movingSphere, maxDistance);
		world->AddConstraint(constraint);
		previous = movingSphere;
	}

	PositionConstraint* constraint = new PositionConstraint(previous, start, maxDistance); //take out end if want to make rope
	world->AddConstraint(constraint);

}

/*
A single function to add a large immoveable cube to the bottom of our world
*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();

	Vector3 floorSize = Vector3(100, 2, 100);
	AABBVolume* volume = new AABBVolume(floorSize);
	//OBBVolume* volume = new OBBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetName("floor");

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetRenderObject()->SetColour(Vector4(2, 1.4, 1.92, 1));
	floor->GetRenderObject()->SetOriColour(Vector4(2, 1.4, 1.92, 1));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	floor->GetPhysicsObject()->SetCollisonType(CollisionType::FLOOR);
	floor->GetPhysicsObject()->SetState(ObjectState::STATIC);


	world->AddGameObject(floor);

	return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple'
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass) {
	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetName("sphere");


	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	/////
	sphere->GetPhysicsObject()->SetCRes(1.0f);
	sphere->GetPhysicsObject()->InitSphereInertia();

	sphere->GetPhysicsObject()->SetState(ObjectState::DYNAMIC);

	world->AddGameObject(sphere);

	////////////hollow sphere = sphere1 inertia tensor - sphere2 inertia tensor./////////////

	return sphere;
}

GameObject* TutorialGame::AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass) {
	GameObject* capsule = new GameObject();

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius * 0.75);  //////////////////////////////
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius * 2, halfHeight, radius * 2))
		.SetPosition(position);

	capsule->SetName("capsule");

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();
	capsule->GetPhysicsObject()->SetCRes(0.7);

	capsule->GetTransform().SetOrientation(Quaternion(1, 0.5, 0.5, 1)); //horizontal

	capsule->GetPhysicsObject()->SetState(ObjectState::DYNAMIC);

	world->AddGameObject(capsule);

	return capsule;

}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject();

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetName("cube");

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1)); ///////////////////////////////////////////////////////////////////////////////////////////////////
	cube->GetRenderObject()->SetOriColour(Vector4(1, 1, 1, 1));
	cube->GetRenderObject()->SetDefaultTexture(nullptr);

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	/////
	cube->GetPhysicsObject()->SetCRes(0.5f);  //higher number more elastic
	cube->GetPhysicsObject()->InitCubeInertia();
	cube->GetPhysicsObject()->SetState(ObjectState::DYNAMIC);

	world->AddGameObject(cube);

	return cube;
}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	//for (int x = 0; x < numCols; ++x) {
	//	for (int z = 0; z < numRows; ++z) {
	//		Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

	//		if (rand() % 2) {
	//			AddCubeToWorld(position, cubeDims);
	//			//AddCapsuleToWorld(position, 0.85f * 3, 0.3f * 3, 1.0);
	//		}
	//		else {
	//			AddCapsuleToWorld(position, 0.85f*3, 0.3f*3);
	//			//AddSphereToWorld(position, sphereRadius);
	//		}
	//	}
	//}


	testObjects.push_back(AddCubeToWorld(Vector3(70,5,50), cubeDims));
	testObjects.push_back(AddCapsuleToWorld(Vector3(75,5,50) ,0.85f * 3, 0.3f * 3, 1.0));
	testObjects.push_back(AddSphereToWorld(Vector3(80,5,50), sphereRadius));

	for (auto i : testObjects) {
		if (i->GetName() == "cube") {
			i->GetRenderObject()->SetDefaultTexture(basicTex);
		}
	}

}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols + 1; ++x) {
		for (int z = 1; z < numRows + 1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}

void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitGameExamples() {
	AddPlayerToWorld(Vector3(70, 5, 85));
	AddEnemyToWorld(Vector3(85, 5, 85));

	AddBonusBall(Vector3(50, 5, 50));

	AddJumpPad(Vector3(0, 0, 60), Vector3(10, 0.05, 10));
	AddJumpPad(Vector3(60, 0, -85), Vector3(10, 0.05, 5));
	SlowFloor();
	FastFloor(Vector3(-65, 0, 80), Vector3(15, 0.02, 10));
	FastFloor(Vector3(-85, 0, 70), Vector3(5, 0.02, 20));


	OBBFloor(Vector3(60, 35, -70), Vector3(20, 1.25, 10));

	FloatingFloor(Vector3(-65, 20, -35), Vector3(5, 1.25, 5));
	FloatingFloor(Vector3(-75, 20, -65), Vector3(5, 1.25, 5));
	FloatingFloor(Vector3(-55, 20, -55), Vector3(5, 1.25, 5));
	FloatingFloor(Vector3(-35, 20, -45), Vector3(5, 1.25, 5));

	//on top of floating floor
	bonus.push_back(AddBonusToWorld(Vector3(-65, 24, -35)));
	bonus.push_back(AddBonusToWorld(Vector3(-75, 24, -65)));
	bonus.push_back(AddBonusToWorld(Vector3(-55, 24, -55)));
	bonus.push_back(AddBonusToWorld(Vector3(-35, 24, -45)));

	testStateObject = AddStateObjectToWorld(Vector3(-55, 0, 45));
	upDownStateObject = AddUpDownState(Vector3(-45, 0, 45));

	pingpong.push_back(AddSphereToWorld(Vector3(-45, 2, -25), 4, 0));
	pingpong.push_back(AddSphereToWorld(Vector3(-65, 2, -35), 4, 0));
	pingpong.push_back(AddSphereToWorld(Vector3(-85, 2, -45), 4, 0));
	pingpong.push_back(AddSphereToWorld(Vector3(-75, 2, -75), 4, 0));
	pingpong.push_back(AddSphereToWorld(Vector3(-55, 2, -65), 4, 0));

	for (auto i : pingpong) {
		i->GetPhysicsObject()->SetCRes(10);
	}



	//add coins
	bonus.push_back(AddBonusToWorld(Vector3(30, 5, 10)));
	bonus.push_back(AddBonusToWorld(Vector3(0, 5, 80)));
	bonus.push_back(AddBonusToWorld(Vector3(-85, 5, 35)));
	bonus.push_back(AddBonusToWorld(Vector3(-35, 5, 25)));
	bonus.push_back(AddBonusToWorld(Vector3(-75, 5, 15)));

	bonus.push_back(AddBonusToWorld(Vector3(-35, 5, -5)));
	bonus.push_back(AddBonusToWorld(Vector3(-65, 5, -45)));
	bonus.push_back(AddBonusToWorld(Vector3(-45, 5, -85)));
	bonus.push_back(AddBonusToWorld(Vector3(25, 5, -95)));
	bonus.push_back(AddBonusToWorld(Vector3(50, 5, -40)));

	bonus.push_back(AddBonusToWorld(Vector3(80, 5, -40)));
	bonus.push_back(AddBonusToWorld(Vector3(95, 5, 15)));
	bonus.push_back(AddBonusToWorld(Vector3(-10, 5, 20)));
	bonus.push_back(AddBonusToWorld(Vector3(10, 5, -20)));
	bonus.push_back(AddBonusToWorld(Vector3(-10, 5, -50)));

	bonus.push_back(AddBonusToWorld(Vector3(0, 5, -75)));
	bonus.push_back(AddBonusToWorld(Vector3(30, 5, 70)));

	//on bridge
	//bonus.push_back(AddBonusToWorld(Vector3(-5, 25, 35)));
	//bonus.push_back(AddBonusToWorld(Vector3(5, 25, 35)));
	//bonus.push_back(AddBonusToWorld(Vector3(-5, 25, -55)));
	//bonus.push_back(AddBonusToWorld(Vector3(5, 25, -55)));
}


GameObject* TutorialGame::AddPlayerToWorld(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 0.1f;

	///////GameObject* character = new GameObject();
	player = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.85f, 0.3f) * meshSize);
	//CapsuleVolume* volume = new CapsuleVolume(0.85f *meshSize, 0.3f*meshSize);

	player->SetBoundingVolume((CollisionVolume*)volume);

	player->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	player->SetName("player");

	if (rand() % 2) {
		player->SetRenderObject(new RenderObject(&player->GetTransform(), charMeshA, nullptr, basicShader));
	}
	else {
		player->SetRenderObject(new RenderObject(&player->GetTransform(), charMeshB, nullptr, basicShader));
	}
	player->SetPhysicsObject(new PhysicsObject(&player->GetTransform(), player->GetBoundingVolume()));

	player->GetRenderObject()->SetColour(Vector4(0, 1.76, 0.02, 1));
	player->GetRenderObject()->SetOriColour(Vector4(0, 1.76, 0.02, 1));

	player->GetPhysicsObject()->SetInverseMass(inverseMass);
	player->GetPhysicsObject()->InitSphereInertia();

	player->GetPhysicsObject()->SetCRes(0.2);
	player->GetPhysicsObject()->SetCollisonType(CollisionType::PLAYER);
	player->GetPhysicsObject()->SetState(ObjectState::DYNAMIC);

	world->AddGameObject(player);

	return player;
}

GameObject* TutorialGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 0.1f;

	//GameObject* character = new GameObject();
	enemy = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	enemy->SetBoundingVolume((CollisionVolume*)volume);

	enemy->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	enemy->SetName("enemy");

	enemy->SetRenderObject(new RenderObject(&enemy->GetTransform(), enemyMesh, nullptr, basicShader));
	enemy->SetPhysicsObject(new PhysicsObject(&enemy->GetTransform(), enemy->GetBoundingVolume()));

	enemy->GetRenderObject()->SetColour(Vector4(2, 0.16, 0, 1));
	enemy->GetRenderObject()->SetOriColour(Vector4(2, 0.16, 0, 1));

	enemy->GetPhysicsObject()->SetInverseMass(inverseMass);
	enemy->GetPhysicsObject()->InitSphereInertia();

	enemy->GetPhysicsObject()->SetCRes(0.2);
	enemy->GetPhysicsObject()->SetCollisonType(CollisionType::ENEMY);
	enemy->GetPhysicsObject()->SetState(ObjectState::DYNAMIC);

	world->AddGameObject(enemy);

	return enemy;
}

GameObject* TutorialGame::AddCoinMiner(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 0.1f;

	coinMinerAI = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	coinMinerAI->SetBoundingVolume((CollisionVolume*)volume);

	coinMinerAI->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	coinMinerAI->SetName("enemy");

	coinMinerAI->SetRenderObject(new RenderObject(&coinMinerAI->GetTransform(), charMeshA, nullptr, basicShader));
	coinMinerAI->SetPhysicsObject(new PhysicsObject(&coinMinerAI->GetTransform(), coinMinerAI->GetBoundingVolume()));

	coinMinerAI->GetRenderObject()->SetColour(Vector4(0.4, 1.42, 2.5, 1));
	coinMinerAI->GetRenderObject()->SetOriColour(Vector4(0.4, 1.42, 2.5, 1));

	coinMinerAI->GetPhysicsObject()->SetInverseMass(inverseMass);
	coinMinerAI->GetPhysicsObject()->InitSphereInertia();

	coinMinerAI->GetPhysicsObject()->SetCRes(0.2);
	coinMinerAI->GetPhysicsObject()->SetCollisonType(CollisionType::BEHAVAI);
	coinMinerAI->GetPhysicsObject()->SetState(ObjectState::DYNAMIC);

	world->AddGameObject(coinMinerAI);

	return coinMinerAI;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position) {
	//GameObject* apple = new GameObject();
	apple = new GameObject();

	SphereVolume* volume = new SphereVolume(1.25f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	apple->SetName("bonus");

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetRenderObject()->SetColour(Vector4(2, 1.7, 0, 1));
	apple->GetRenderObject()->SetOriColour(Vector4(2, 1.7, 0, 1));

	apple->GetPhysicsObject()->SetInverseMass(0.001f);
	apple->GetPhysicsObject()->InitSphereInertia();

	apple->GetPhysicsObject()->SetCollisonType(CollisionType::BONUS);
	apple->GetPhysicsObject()->SetState(ObjectState::DYNAMIC);

	world->AddGameObject(apple);

	return apple;
}

GameObject* TutorialGame::AddBonusBall(const Vector3& position) {
	//GameObject* apple = new GameObject();
	bonusBall = new GameObject();

	SphereVolume* volume = new SphereVolume(1.25f * 1.5);
	bonusBall->SetBoundingVolume((CollisionVolume*)volume);
	bonusBall->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25) * 1.5)
		.SetPosition(position);


	bonusBall->SetName("bonusBall");

	bonusBall->SetRenderObject(new RenderObject(&bonusBall->GetTransform(), bonusMesh, nullptr, basicShader));
	bonusBall->SetPhysicsObject(new PhysicsObject(&bonusBall->GetTransform(), bonusBall->GetBoundingVolume()));

	bonusBall->GetPhysicsObject()->SetInverseMass(1.0f);
	bonusBall->GetPhysicsObject()->InitSphereInertia();

	bonusBall->GetRenderObject()->SetColour(Vector4(1, 0.1, 0.7, 1));
	bonusBall->GetRenderObject()->SetOriColour(Vector4(1, 0.1, 0.7, 1));

	bonusBall->GetPhysicsObject()->SetCollisonType(CollisionType::BONUS);
	bonusBall->GetPhysicsObject()->SetState(ObjectState::DYNAMIC);


	world->AddGameObject(bonusBall);

	return bonusBall;
}

StateGameObject* TutorialGame::AddStateObjectToWorld(const Vector3& position) {
	StateGameObject* movingCube = new StateGameObject();

	AABBVolume* volume = new AABBVolume(Vector3(5, 5, 5));
	movingCube->SetBoundingVolume((CollisionVolume*)volume);
	movingCube->GetTransform()
		.SetPosition(position)
		.SetScale(Vector3(5, 5, 5) * 2);


	movingCube->SetName("movingObject");

	movingCube->SetRenderObject(new RenderObject(&movingCube->GetTransform(), cubeMesh, nullptr, basicShader));
	movingCube->SetPhysicsObject(new PhysicsObject(&movingCube->GetTransform(), movingCube->GetBoundingVolume()));

	movingCube->GetRenderObject()->SetColour(Vector4(2, 2, 0, 1));
	movingCube->GetRenderObject()->SetOriColour(Vector4(2, 2, 0, 1));

	movingCube->GetPhysicsObject()->SetInverseMass(0.01f);
	movingCube->GetPhysicsObject()->InitSphereInertia();
	movingCube->GetPhysicsObject()->SetCRes(0.0f);

	movingCube->GetPhysicsObject()->SetCollisonType(CollisionType::MOVINGOBJECT);
	movingCube->GetPhysicsObject()->SetState(ObjectState::DYNAMIC);

	world->AddGameObject(movingCube);

	return movingCube;

}

StateGameObjectUD* TutorialGame::AddUpDownState(const Vector3& position) {
	StateGameObjectUD* movingCube = new StateGameObjectUD();

	AABBVolume* volume = new AABBVolume(Vector3(5, 5, 5));
	movingCube->SetBoundingVolume((CollisionVolume*)volume);
	movingCube->GetTransform()
		.SetPosition(position)
		.SetScale(Vector3(5, 5, 5) * 2);


	movingCube->SetName("movingObject");

	movingCube->SetRenderObject(new RenderObject(&movingCube->GetTransform(), cubeMesh, nullptr, basicShader));
	movingCube->SetPhysicsObject(new PhysicsObject(&movingCube->GetTransform(), movingCube->GetBoundingVolume()));

	movingCube->GetRenderObject()->SetColour(Vector4(2, 2, 0, 1));
	movingCube->GetRenderObject()->SetOriColour(Vector4(2, 2, 0, 1));

	movingCube->GetPhysicsObject()->SetInverseMass(0.01f);
	movingCube->GetPhysicsObject()->InitSphereInertia();
	movingCube->GetPhysicsObject()->SetCRes(0.0f);

	movingCube->GetPhysicsObject()->SetCollisonType(CollisionType::MOVINGOBJECT);
	movingCube->GetPhysicsObject()->SetState(ObjectState::DYNAMIC);

	world->AddGameObject(movingCube);

	return movingCube;
}


/*
Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around.
*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		if (!winGame && !loseGame) {
			renderer->DrawString("Press Q to change to camera mode!", Vector2(5, 85));
		}

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {

			if (selectionObject) {	//set colour to deselected;
				//selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject->GetRenderObject()->SetColour(selectionObject->GetRenderObject()->GetOriColour());
				selectionObject = nullptr;
				lockedObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
			RayCollision closestCollision;

			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				selectionObject->GetRenderObject()->SetColour(Vector4(0, 0.3, 1, 1));  //set colour to selected object

				////////////////////				
				Debug::DrawLine(world->GetMainCamera()->GetPosition(), closestCollision.collidedAt, Vector4(1, 0, 0, 1), 5);
				//Debug::DrawLine(world->GetMainCamera()->GetPosition(), selectionObject->GetTransform().GetPosition(), Vector4(1, 0, 0, 1), 3);
				Debug::DrawAxisLines(selectionObject->GetTransform().GetMatrix(), 1.0f, 5);
				////////////////////
				return true;
			}

			else {
				return false;
			}
		}
	}
	else {
		if (!winGame && !loseGame) {
			renderer->DrawString("Press Q to change to select mode!", Vector2(5, 85));
		}
	}

	if (lockedObject && !loseGame && !winGame) {
		renderer->DrawString("Press L to unlock object!", Vector2(5, 80));
		PrintDebugInfo();
	}

	else if (selectionObject && !loseGame && !winGame) {

		renderer->DrawString("Press L to lock selected object object!", Vector2(5, 80));

		////////////////////
		//printing name above selected object

		float screenAspect = (float)1262 / (float)676;
		Matrix4 viewMatrix = world->GetMainCamera()->BuildViewMatrix();
		//world->GetMainCamera()->BuildOrthoCamera(world->GetMainCamera()->GetPosition(), world->GetMainCamera()->GetPitch(), world->GetMainCamera()->GetYaw(), 0, 100, 0, 100, 1, 0);
		Matrix4 projMatrix = world->GetMainCamera()->BuildProjectionMatrix(screenAspect);


		Vector3 objectPos = (Matrix4::Translation(Vector3(0, 4, 0)) * selectionObject->GetTransform().GetPosition());
		Vector4 mvp = (projMatrix * viewMatrix * Vector4(objectPos, 1.0));  //mvp
		float x = mvp.x / mvp.w;
		float y = mvp.y / mvp.w;
		float z = mvp.z / mvp.w;

		objectPos = Vector3(x, y, z);

		float screenX = 0 + 100 * (objectPos.x + 1) / 2;
		float screenY = 0 + 100 * (objectPos.y + 1) / 2;


		//debug info
		//if (selectionObject->GetName() != "player") {
		Debug::Print(selectionObject->GetName() + "(" + std::to_string(selectionObject->GetWorldID()) + ")", Vector2(screenX - 5, 100 - (screenY)+2), 20, Debug::CYAN);
		//}
		PrintDebugInfo();

		/////////////////

	}


	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
		if (selectionObject) {
			if (lockedObject == selectionObject) {
				lockedObject = nullptr;
			}
			else {
				lockedObject = selectionObject;
			}
		}

	}

	return false;
}


void TutorialGame::PrintDebugInfo() {
	string debugInfos[10];

	std::string nameInfo = "Name: " + selectionObject->GetName();
	std::string posInfo = "Position: (" + std::to_string(selectionObject->GetTransform().GetPosition().x) + "," + std::to_string(selectionObject->GetTransform().GetPosition().y) + "," + std::to_string(selectionObject->GetTransform().GetPosition().z) + ')';
	std::string oriInfo = "Orientation: (" + std::to_string(selectionObject->GetTransform().GetOrientation().x) + "," + std::to_string(selectionObject->GetTransform().GetOrientation().y) + "," + std::to_string(selectionObject->GetTransform().GetOrientation().z) + "," + std::to_string(selectionObject->GetTransform().GetOrientation().w) + ')';
	std::string forceInfo = "Force: (" + std::to_string(selectionObject->GetPhysicsObject()->GetForce().x) + "," + std::to_string(selectionObject->GetPhysicsObject()->GetForce().y) + "," + std::to_string(selectionObject->GetPhysicsObject()->GetForce().z) + ')';
	std::string massInfo = "Inverse Mass: " + std::to_string(selectionObject->GetPhysicsObject()->GetInverseMass());
	std::string cresInfo = "Coeff of Res: " + std::to_string(selectionObject->GetPhysicsObject()->GetCRes());
	std::string colVol = "CollisionVolume: " + selectionObject->GetPhysicsObject()->PrintColVol((selectionObject->GetPhysicsObject()->GetCollisionVolume()));
	std::string worldIDInfo = "WorldID: " + std::to_string(selectionObject->GetWorldID());
	std::string colType = "CollisionType: " + selectionObject->GetPhysicsObject()->ToString(selectionObject->GetPhysicsObject()->GetCollisionType());
	std::string isActiveinfo = "IsActive: " + std::to_string(selectionObject->IsActive());
	debugInfos[0] = nameInfo;
	debugInfos[1] = posInfo;
	debugInfos[2] = oriInfo;
	debugInfos[3] = forceInfo;
	debugInfos[4] = massInfo;
	debugInfos[5] = cresInfo;
	debugInfos[6] = colVol;
	debugInfos[7] = worldIDInfo;
	debugInfos[8] = colType;
	debugInfos[9] = isActiveinfo;

	int a = 15;
	for (int i = 0; i < 10; i++) {
		a += 3;
		Debug::Print(debugInfos[i], Vector2(2, a), 16);
	}
}


/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/
void TutorialGame::MoveSelectedObject() {

	if (!winGame && !loseGame) {
		renderer->DrawString("Click Force :" + std::to_string(forceMagnitude), Vector2(35, 5)); // Draw debug text at 10 ,20
	}

	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject) {
		return;	//we haven �t selected anything !
	}

	// Push the selected object !
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {

		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
		RayCollision closestCollision;

		//physics->SetGravity(Vector3(0, -1, 0));    //////////////////////////////////////////////////////

		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				//selectionObject->GetPhysicsObject()->AddForce(ray.GetDirection() * forceMagnitude);  //linear motion

				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);  //angular motion
			}
		}
	}

}


//Pathfinding
void TutorialGame::TestPathfinding() {

	testNodes.clear();
	testNodeCopy.clear();
	NavigationGrid grid("TestGrid1.txt");

	NavigationPath outPath; //making path 

	Vector3 startPos(95, 0, 5); //(0,0,-90)

	Vector3 endPos(180, 0, 180); //(85,0,85)

	bool found = grid.FindPath(startPos, endPos, outPath); //finding path and putting it into outPath

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) { //only pop waypoint if AI is close enough to node
		testNodes.push_back(pos);
	}



	Vector3 startPosMiner(170, 0, 190); //+95
	NavigationPath minerPath; //making path 
	bool foundMiner = grid.FindPath(startPos, startPosMiner, minerPath);

	Vector3 posMiner;
	while (minerPath.PopWaypoint(posMiner)) {
		testNodeCopy.push_back(posMiner);
	}

}

void TutorialGame::DisplayPathfinding() {
	for (int i = 1; i < testNodes.size(); ++i) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];

		a.y = 1;
		b.y = 1;

		Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
	}


	for (int i = 1; i < testNodeCopy.size(); ++i) {
		Vector3 a = testNodeCopy[i - 1];
		Vector3 b = testNodeCopy[i];

		a.y = 2;
		b.y = 2;

		Debug::DrawLine(a, b, Vector4(0, 0, 1, 1));
	}
}


//Behaviour Tree
void TutorialGame::InitBehavTreeExample() {
	AddCoinMiner(Vector3(77, 5, 90));

	//behavCoins.clear();

	//int randomNo = rand() % 2;
	//int genX = 0;
	//int genZ = 0;
	//int tempX = 0;
	//int tempZ = 0;

	//switch (randomNo) {

	//case 0:
	//	spawnedCoins = false;
	//	break;
	//case 1:
	//	genX = rand() % 21 + 70;
	//	genZ = rand() % (-41) + (-50);
	//	tempX = genX;
	//	tempZ = genZ;

	//	behavCoins.push_back(AddBehavCoin("redCoin", Vector4(1, 0, 0, 1), Vector3(genX, 11, genZ)));

	//	genX = rand() % 21 + 70;
	//	genZ = rand() % (-41) + (-50);
	//	if (genX == tempX && genZ == tempZ) {
	//		genX = rand() % 21 + 70;
	//		genZ = rand() % (-41) + (-50);
	//	}
	//	behavCoins.push_back(AddBehavCoin("blueCoin", Vector4(0, 0, 1, 1), Vector3(genX, 5, genZ)));
	//	spawnedCoins = true;

	//	break;
	//}

}

GameObject* TutorialGame::AddBehavCoin(string name, Vector4 colour, Vector3 position) {
	behavCoin = new GameObject();

	SphereVolume* volume = new SphereVolume(1.25f);
	behavCoin->SetBoundingVolume((CollisionVolume*)volume);
	behavCoin->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	behavCoin->SetName(name);

	behavCoin->SetRenderObject(new RenderObject(&behavCoin->GetTransform(), bonusMesh, nullptr, basicShader));
	behavCoin->SetPhysicsObject(new PhysicsObject(&behavCoin->GetTransform(), behavCoin->GetBoundingVolume()));

	behavCoin->GetRenderObject()->SetColour(colour);
	behavCoin->GetRenderObject()->SetOriColour(colour);

	behavCoin->GetPhysicsObject()->SetInverseMass(1.0f);
	behavCoin->GetPhysicsObject()->InitSphereInertia();

	behavCoin->GetPhysicsObject()->SetCollisonType(CollisionType::BONUS);
	behavCoin->GetPhysicsObject()->SetState(ObjectState::DYNAMIC);


	world->AddGameObject(behavCoin);

	return behavCoin;
}

//Behaviour Tree example - AI can only take 1 item and takes closest one
void TutorialGame::TestBehaviourTree(float time) {

	float decider;
	Vector3 coinMinerPos = coinMinerAI->GetTransform().GetPosition();


	BehaviourAction* findCoin = new BehaviourAction("Look for coin", [&](float dt, BehaviourState state)-> BehaviourState {
		if (state == BehaviourState::Initialise) {
			std::cout << "Looking for Coins!\n";
			state = BehaviourState::Ongoing;
		}

		else if (state == BehaviourState::Ongoing) {
			for (auto i : bonus) {
				if (i->IsActive() == true) {
					std::cout << "Sense Coins!\n";
					return BehaviourState::Success;
				}

				else {
					std::cout << "No Coins left!\n";
					return BehaviourState::Failure;
				}
			}

		}
		return state;
		}
	);

	BehaviourAction* decideTakeCoin = new BehaviourAction("Decide to take coin", [&](float dt, BehaviourState state)-> BehaviourState {
		if (state == BehaviourState::Initialise) {
			std::cout << "Deciding if it should take coin!\n";
			decider = rand() % 2;
			state = BehaviourState::Ongoing;
		}

		else if (state == BehaviourState::Ongoing) {
			if (decider == 0) {
				std::cout << "Going for coin!\n";
				return BehaviourState::Success;
			}

			else {
				std::cout << "Don't want coins anymore!\n";
				return BehaviourState::Failure;
			}
		}
		return state;
		}
	);

	BehaviourAction* chooseCoin = new BehaviourAction("Choosing Coin to take", [&](float dt, BehaviourState state)-> BehaviourState {
		if (state == BehaviourState::Initialise) {
			std::cout << "Getting closest coin\n";
			return BehaviourState::Ongoing;
		}

		else if (state == BehaviourState::Ongoing) {

			for (auto i : bonus) { //using raycasting to find coin

				float coinDist = 0;
				Vector3 direction = Vector3(0, 0, 0);

				coinPos = i->GetTransform().GetPosition();

				float coinDistance = 20;



				//float tempLen = coinPos.Length();
				//temp = coinPos;
				////closest coin
				//if (tempLen > coinPos.Length()) {
				//	tempLen = coinPos.Length();
				//	temp = coinPos;
				//}
				//else {
				//	tempLen = tempLen;
				//	temp = temp;
				//}

				coinDist = (coinPos - coinMinerPos).Length();
				direction = coinPos - coinMinerPos;
				//std::cout << temp;

				if (coinMinerPos.x - coinDistance <= coinPos.x && coinMinerPos.x + coinDistance >= coinPos.x &&
					coinMinerPos.z - coinDistance <= coinPos.z && coinMinerPos.z + coinDistance >= coinPos.z &&
					coinPos.y < 20 && coinPos.y > 0) {

					std::cout << coinPos;
					SphereVolume* sphVol = new SphereVolume(1.25f * 1.5);
					RayCollision closestCollision;
					Ray* ray = new Ray(coinMinerPos, direction);

					Debug::DrawLine(coinMinerPos, coinPos, Debug::RED, 5.0f);

					if (CollisionDetection::RaySphereIntersection(*ray, i->GetTransform(), *sphVol, closestCollision)) {
						std::cout << "Coin in sight using ray!\n";
						coinCounter++;
						return BehaviourState::Success;
					}
					if (CollisionDetection::RaySphereIntersection(*ray, i->GetTransform(), *sphVol, closestCollision) == false) {
						std::cout << "Coin in sight!\n";
						coinCounter++;
						return BehaviourState::Success;
					}
					
				}


			}
			std::cout << "No coins here...\n";
			return BehaviourState::Failure;


			/*for (auto i : bonus) {
				float coinDist = (i->GetTransform().GetPosition() - coinMinerPos).Length();
				coinPos = i->GetTransform().GetPosition();
				if (coinDist <= 100) {
					std::cout << "Coin in sight!\n";
					coinCounter++;
					return BehaviourState::Success;
				}

				std::cout << "No coins here...\n";
				return BehaviourState::Failure;

			}*/

		}

		return state;
		}
	);


	//connecting states together by add children
	BehaviourSequence* sequence = new BehaviourSequence("Finding Sequence"); //sequence
	sequence->AddChild(findCoin);
	sequence->AddChild(decideTakeCoin);

	BehaviourSelector* selection = new BehaviourSelector("Choose Selection"); //selector
	selection->AddChild(chooseCoin);

	BehaviourSequence* rootSequence = new BehaviourSequence("Root Sequence"); //root adding sequence and selector
	rootSequence->AddChild(sequence);
	rootSequence->AddChild(selection);


	rootSequence->Reset();
	BehaviourState state = BehaviourState::Ongoing;
	std::cout << "\nGoing to find coins!\n";

	while (state == BehaviourState::Ongoing) {
		state = rootSequence->Execute(time);
	}

	if (state == BehaviourState::Success) {
		goToCoin = true;
		std::cout << "SUCCESS Behaviour Tree - Trying to go for coin!\n";
	}

	else if (state == BehaviourState::Failure) {
		std::cout << "FAIL Behaviour Tree - No coins looted\n";

	}

	std::cout << "Finished behaviour tree job!\n\n";

}




