#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"

#include "../CSC8503Common/NavigationGrid.h"
#include "../CSC8503Common/NavigationMesh.h"
#include "../CSC8503Common/PushdownState.h"

#include <iomanip>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>


using namespace NCL;
using namespace CSC8503;



bool displayPath = false;


TutorialGame::TutorialGame() {
	world = new GameWorld();
	renderer = new GameTechRenderer(*world);
	physics = new PhysicsSystem(*world);

	testStateObject = nullptr;
	player = nullptr;
	enemy = nullptr;

	forceMagnitude = 10.0f;
	useGravity = false;
	inSelectionMode = false;

	playerScore = 1000;
	enemyScore = 1000;
	timer = 0;
	loseGame = false;
	winGame = false;
	once = false;
	movePlayer = false;
	enemytimer = 0;
	practiceMode = false;

	enemyState = EnemyState::IDLE;

	Debug::SetRenderer(renderer);

	InitialiseAssets();
}



void TutorialGame::LoseGame() {

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
		renderer->DrawString("Your Score: " + std::to_string(playerScore), Vector2(10, 50), 40);
		renderer->DrawString("Player 2 Score: " + std::to_string(enemyScore), Vector2(10, 60), 40);
		renderer->DrawString("Esc to go back to main menu", Vector2(10, 80), 30);
	}

}

void TutorialGame::WinGame() {

	float offset = 20.0f;
	if (player->GetTransform().GetPosition().x <= 100 + offset && player->GetTransform().GetPosition().x >= 100 - offset &&
		player->GetTransform().GetPosition().z <= 100 + offset && player->GetTransform().GetPosition().z >= 100 - offset) {

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
			renderer->DrawString("Your Score: " + std::to_string(playerScore), Vector2(10, 50), 40);
			renderer->DrawString("Player 2 Score: " + std::to_string(enemyScore), Vector2(10, 60), 40);
			renderer->DrawString("Esc to go back to main menu", Vector2(10, 80), 30);
		}
	}
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

	//if (!practiceMode) {
	useGravity = true;
	physics->UseGravity(useGravity);
	//}

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
		}
		else {
			Debug::Print("(G)ravity off", Vector2(5, 95));
		}
	}

	if (practiceMode) {
		playerScore = 999999;
	}

	TextOnScreen(dt);
	LoseGame();
	WinGame();

	SelectObject();
	MoveSelectedObject();
	physics->Update(dt);

	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetPosition();
		Vector3 camPos = objPos + lockedOffset;

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
		Vector3 camPos = objPos + Vector3(0, 14, 40); //higher z = farther back camera

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler();

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x + 10);
		world->GetMainCamera()->SetYaw(angles.y);
	}




	if (testStateObject) {
		testStateObject->Update(dt);
	}

	if (!practiceMode) {
		test(dt);
	}


	world->UpdateWorld(dt);
	renderer->Update(dt);

	Debug::FlushRenderables(dt);
	renderer->Render();
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


GameObject* TutorialGame::JumpPad() {
	GameObject* jumpPad = new GameObject();

	Vector3 dimensions = Vector3(5, 2, 5);
	AABBVolume* volume = new AABBVolume(dimensions);

	jumpPad->SetBoundingVolume((CollisionVolume*)volume);

	jumpPad->GetTransform()
		.SetPosition(Vector3(20, 0, 20))
		.SetScale(dimensions * 2);

	jumpPad->SetName("jumppad");

	jumpPad->SetRenderObject(new RenderObject(&jumpPad->GetTransform(), cubeMesh, basicTex, basicShader));
	jumpPad->SetPhysicsObject(new PhysicsObject(&jumpPad->GetTransform(), jumpPad->GetBoundingVolume()));

	jumpPad->GetPhysicsObject()->SetInverseMass(0);
	/////
	jumpPad->GetPhysicsObject()->SetCRes(1.0f);  //higher number more elastic
	jumpPad->GetPhysicsObject()->InitCubeInertia();

	jumpPad->GetPhysicsObject()->SetCollisonType(CollisionType::JUMPPAD);
	jumpPad->GetPhysicsObject()->SetState(ObjectState::STATIC);

	world->AddGameObject(jumpPad);

	return jumpPad;
}

GameObject* TutorialGame::SlowFloor() {
	GameObject* slowfloor = new GameObject();

	Vector3 dimensions = Vector3(30, 0.01, 10);
	AABBVolume* volume = new AABBVolume(dimensions);

	slowfloor->SetBoundingVolume((CollisionVolume*)volume);

	slowfloor->GetTransform()
		.SetPosition(Vector3(60, 0, 60))
		.SetScale(dimensions * 2);

	slowfloor->SetName("slowfloor");

	slowfloor->SetRenderObject(new RenderObject(&slowfloor->GetTransform(), cubeMesh, basicTex, basicShader));
	slowfloor->SetPhysicsObject(new PhysicsObject(&slowfloor->GetTransform(), slowfloor->GetBoundingVolume()));

	slowfloor->GetPhysicsObject()->SetInverseMass(0);
	/////
	slowfloor->GetPhysicsObject()->SetCRes(1.0f);  //higher number more elastic
	slowfloor->GetPhysicsObject()->InitCubeInertia();

	slowfloor->GetPhysicsObject()->SetCollisonType(CollisionType::JUMPPAD);
	slowfloor->GetPhysicsObject()->SetState(ObjectState::STATIC);

	world->AddGameObject(slowfloor);

	return slowfloor;
}


void TutorialGame::test(float dt) {

	enemytimer += dt;
	float offset = 8;
	Vector3 enemyPos = enemy->GetTransform().GetPosition();
	float force = 10;

	if (physics->slowfloorE == true) {
		force = 5;
	}

	//going for coin
	float coinDistance = 10;
	if (enemyPos.x - coinDistance <= apple->GetTransform().GetPosition().x && enemyPos.x + coinDistance >= apple->GetTransform().GetPosition().x &&
		enemyPos.z - coinDistance <= apple->GetTransform().GetPosition().z && enemyPos.z + coinDistance >= apple->GetTransform().GetPosition().z) {
		Vector3 direction = apple->GetTransform().GetPosition() - enemyPos;
		direction.Normalise();
		enemy->GetPhysicsObject()->AddForce(direction * force);
		//std::cout << "Going after coin\n";
	}


	//going for bonusBall
	coinDistance = 15;
	if (physics->enemy1 == false && (enemyPos.x - coinDistance <= bonusBall->GetTransform().GetPosition().x && enemyPos.x + coinDistance >= bonusBall->GetTransform().GetPosition().x &&
		enemyPos.z - coinDistance <= bonusBall->GetTransform().GetPosition().z && enemyPos.z + coinDistance >= bonusBall->GetTransform().GetPosition().z)) {
		Vector3 direction = bonusBall->GetTransform().GetPosition() - enemyPos;
		direction.Normalise();
		enemy->GetPhysicsObject()->AddForce(direction * force);
		//std::cout << "Going after bonusBall\n";
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
			Vector3 tempPos = Vector3(enemyPos.x + 80, enemyPos.y, enemyPos.z + 80);
			NavigationPath path;
			NavigationGrid grid("TestGrid1.txt");
			bool found = grid.FindPath(Vector3(80, 0, 10), tempPos, path);

			Vector3 pos;
			while (path.PopWaypoint(pos)) {
				testNodes.push_back(pos);
			}

			if (!found || enemytimer > 10) {
				testNodes.clear();
				std::cout << "Stuck, teleporting near player!\n";
				enemy->GetTransform().SetPosition(player->GetTransform().GetPosition() + Vector3(5,0,5));
				tempPos = Vector3(enemy->GetTransform().GetPosition().x + 80, enemy->GetTransform().GetPosition().y, enemy->GetTransform().GetPosition().z + 80);
				found = grid.FindPath(Vector3(80, 0, 10), tempPos, path);

				while (path.PopWaypoint(pos)) {
					testNodes.push_back(pos);
				}

			}

			if (found) {
				std::cout << "Recalculating Path\n";
				//DisplayPathfinding();
			}


			enemytimer = 0;
		}

		Vector3 direction = testNodes.back() - enemyPos;
		direction.Normalise();
		enemy->GetPhysicsObject()->AddForce(direction * force);


	}


}


void TutorialGame::OppositeForce() {


}




void TutorialGame::UpdateKeys() {

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
		lockedObject = nullptr;
		playerScore = 1000;
		enemyScore = 1000;
		timer = 0;
		loseGame = false;
		winGame = false;
		once = false;
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

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM9)) {
			lockedObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}

}

void TutorialGame::DebugObjectMovement() {
	//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject && (selectionObject->GetName() != "player")) {

		selectionObject->GetPhysicsObject()->ClearForces();

		//Debug::DrawLine(Vector3(0, 10, 0), Vector3(0, 10, 0.3f * 3), Vector4(1, 0, 0, 1));  //line from initial camera pos to object

		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 50, 0));
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
		force = 50.0f;
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



	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::SPACE) && physics->jump == true) {

		player->GetPhysicsObject()->AddForce(Vector3(0, 60 * force, 0));
		physics->jump = false;
	}
	if (player->GetPhysicsObject()->GetForce().y > force) { //pulling player down faster
		player->GetPhysicsObject()->AddForce(Vector3(0, -9.8 * force, 0));
	}

	if (physics->jumpforce == true) { //jumping on jumppad
		player->GetPhysicsObject()->AddForce(Vector3(0, force * 200, 0));
		physics->jumpforce = false;
	}



}



void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();


	//BridgeConstraintTest();
	InitMixedGridWorld(5, 5, 3.5f, 3.5f);
	InitGameExamples();
	//InitCubeGridWorld(5,5,10,10,Vector3(5,5,5));
	//InitSphereGridWorld(5, 5, 10, 10, 10);
	InitDefaultFloor();

	TestPathfinding();
	//DisplayPathfinding();
	testStateObject = AddStateObjectToWorld(Vector3(30, 0, 0));


	enemy->GetTransform().SetPosition(Vector3(testNodeCopy.back().x, 0, testNodeCopy.back().z));

	JumpPad();
	SlowFloor();
}

//tut 8 Constraints and solvers
void TutorialGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(8, 8, 8);

	float invCubeMass = 5; // how heavy the middle pieces are
	int numLinks = 10;
	float maxDistance = 30; // constraint distance
	float cubeDistance = 20; // distance between links

	Vector3 startPos = Vector3(10, 5, 15);

	//infinite mass, start and end of bridge
	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);
	GameObject* end = AddCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), cubeSize, 0);

	GameObject* previous = start;

	for (int i = 0; i < numLinks; ++i) {
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass);
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}

	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance); //take out end if want to make rope
	world->AddConstraint(constraint);

}


/*
A single function to add a large immoveable cube to the bottom of our world
*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();

	Vector3 floorSize = Vector3(100, 2, 100);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetName("floor");

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetRenderObject()->SetColour(Vector4(1, 0.71, 0.96, 1));
	floor->GetRenderObject()->SetOriColour(Vector4(1, 0.71, 0.96, 1));

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
	sphere->GetPhysicsObject()->SetCRes(0.1f);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	////////////////////hollow sphere = sphere1 inertia tensor - sphere2 inertia tensor.////////////////////////////

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

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	/////
	cube->GetPhysicsObject()->SetCRes(1.0f);  //higher number more elastic
	cube->GetPhysicsObject()->InitCubeInertia();

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

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
				//AddCapsuleToWorld(position, 0.85f * 3, 0.3f * 3);
			}
			else {
				//AddCapsuleToWorld(position, 0.85f*3, 0.3f*3);
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}


	AddSphereToWorld(Vector3(160, 3, 160), 2);
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
	AddPlayerToWorld(Vector3(0, 0.02, 0));
	AddEnemyToWorld(Vector3(5, 0.02, 0));
	AddBonusToWorld(Vector3(10, 5, 0));
	AddBonusToWorld(Vector3(20, 5, 0));
	AddbonusBall(Vector3(50, 15, 50));
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

	player->GetRenderObject()->SetColour(Vector4(0, 0.88, 0.01, 1));
	player->GetRenderObject()->SetOriColour(Vector4(0, 0.88, 0.01, 1));

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
	float inverseMass = 0.5f;

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

	enemy->GetRenderObject()->SetColour(Vector4(1, 0.08, 0, 1));
	enemy->GetRenderObject()->SetOriColour(Vector4(1, 0.08, 0, 1));

	enemy->GetPhysicsObject()->SetInverseMass(inverseMass);
	enemy->GetPhysicsObject()->InitSphereInertia();

	enemy->GetPhysicsObject()->SetCRes(0.2);
	enemy->GetPhysicsObject()->SetCollisonType(CollisionType::ENEMY);
	enemy->GetPhysicsObject()->SetState(ObjectState::DYNAMIC);

	world->AddGameObject(enemy);

	return enemy;
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

	apple->GetRenderObject()->SetColour(Vector4(1, 0.85, 0, 1));
	apple->GetRenderObject()->SetOriColour(Vector4(1, 0.85, 0, 1));

	apple->GetPhysicsObject()->SetInverseMass(0.001f);
	apple->GetPhysicsObject()->InitSphereInertia();

	apple->GetPhysicsObject()->SetCollisonType(CollisionType::BONUS);
	apple->GetPhysicsObject()->SetState(ObjectState::STATIC);

	world->AddGameObject(apple);

	return apple;
}

GameObject* TutorialGame::AddbonusBall(const Vector3& position) {
	//GameObject* apple = new GameObject();
	bonusBall = new GameObject();

	SphereVolume* volume = new SphereVolume(1.25f);
	bonusBall->SetBoundingVolume((CollisionVolume*)volume);
	bonusBall->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	bonusBall->SetName("bonusBall");

	bonusBall->SetRenderObject(new RenderObject(&bonusBall->GetTransform(), bonusMesh, nullptr, basicShader));
	bonusBall->SetPhysicsObject(new PhysicsObject(&bonusBall->GetTransform(), bonusBall->GetBoundingVolume()));

	bonusBall->GetRenderObject()->SetColour(Vector4(1, 0.85, 1, 1));
	bonusBall->GetRenderObject()->SetOriColour(Vector4(1, 0.85, 1, 1));

	bonusBall->GetPhysicsObject()->SetInverseMass(1.0f);
	bonusBall->GetPhysicsObject()->InitSphereInertia();

	bonusBall->GetPhysicsObject()->SetCollisonType(CollisionType::BONUS);
	bonusBall->GetPhysicsObject()->SetState(ObjectState::DYNAMIC);


	world->AddGameObject(bonusBall);

	return bonusBall;
}


StateGameObject* TutorialGame::AddStateObjectToWorld(const Vector3& position) {
	StateGameObject* movingCube = new StateGameObject();

	AABBVolume* volume = new AABBVolume(Vector3(2.5, 2.5, 2.5));
	movingCube->SetBoundingVolume((CollisionVolume*)volume);
	movingCube->GetTransform()
		.SetPosition(position)
		.SetScale(Vector3(5, 5, 5));


	movingCube->SetName("movingObject");

	movingCube->SetRenderObject(new RenderObject(&movingCube->GetTransform(), cubeMesh, nullptr, basicShader));
	movingCube->SetPhysicsObject(new PhysicsObject(&movingCube->GetTransform(), movingCube->GetBoundingVolume()));

	movingCube->GetRenderObject()->SetColour(Vector4(1, 1, 0, 1));
	movingCube->GetRenderObject()->SetOriColour(Vector4(1, 1, 0, 1));

	movingCube->GetPhysicsObject()->SetInverseMass(0.01f);
	movingCube->GetPhysicsObject()->InitSphereInertia();
	movingCube->GetPhysicsObject()->SetCRes(0.0f);

	movingCube->GetPhysicsObject()->SetCollisonType(CollisionType::MOVINGOBJECT);
	movingCube->GetPhysicsObject()->SetState(ObjectState::STATIC);

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
	std::string aiStateInfo = "AI State: ";/////////////////////////////
	std::string worldIDInfo = "WorldID: " + std::to_string(selectionObject->GetWorldID());
	std::string colType = "CollisionType: " + selectionObject->GetPhysicsObject()->ToString(selectionObject->GetPhysicsObject()->GetCollisionType());
	std::string isActiveinfo = "IsActive: " + std::to_string(selectionObject->IsActive());
	debugInfos[0] = nameInfo;
	debugInfos[1] = posInfo;
	debugInfos[2] = oriInfo;
	debugInfos[3] = forceInfo;
	debugInfos[4] = massInfo;
	debugInfos[5] = cresInfo;
	debugInfos[6] = aiStateInfo;
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
		return;	//we haven ’t selected anything !
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








//tut 10
void TutorialGame::TestPathfinding() {

	testNodes.clear();
	NavigationGrid grid("TestGrid1.txt");

	NavigationPath outPath; //making path 

	Vector3 startPos(80, 0, 10);

	Vector3 endPos(160, 0, 160);

	bool found = grid.FindPath(startPos, endPos, outPath); //finding path and putting it into outPath

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) { //only pop waypoint if AI is close enough to node
		testNodes.push_back(pos);
	}
	testNodeCopy = testNodes;
}


void TutorialGame::DisplayPathfinding() {
	for (int i = 1; i < testNodes.size(); ++i) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];

		Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
	}

}