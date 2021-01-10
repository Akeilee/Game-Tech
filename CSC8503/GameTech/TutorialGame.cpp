#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"

#include "../CSC8503Common/NavigationGrid.h"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>

using namespace NCL;
using namespace CSC8503;



float timer;
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

	if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(5, 95));
	}
	else {
		Debug::Print("(G)ravity off", Vector2(5, 95));
	}


	TextOnScreen(dt);


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


	if (testStateObject) {
		testStateObject->Update(dt);
	}
	test(dt);

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


	if (decreaseScore >= 1) {

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
		playerScore += 25;
		physics->setPBonus(false);
	}
	if (physics->getEBonus() == true) {
		enemyScore += 25;
		physics->setEbonus(false);
	}


	std::string pScore = "You: " + std::to_string(playerScore);
	std::string eScore = "Player 2: " + std::to_string(enemyScore);

	Debug::Print(pScore, Vector2(1, 6), 30, Vector4(0, 0.88, 0.01, 1));
	Debug::Print(eScore, Vector2(1, 12), 30, Vector4(1, 0.08, 0, 1));

}


void TutorialGame::CollectBonus() {

}


void TutorialGame::test(float dt) {
	//std::cout << "first " << testNodeCopy.back().x << ",";
	//std::cout << testNodeCopy.back().z << "\n";
	//std::cout << testNodeCopy.end()[-2].x;
	//std::cout << testNodeCopy.end()[-2].z << "\n";

	//Vector3 direction = character->GetTransform().GetPosition() - testNodeCopy.back();

	////if (character->GetTransform().GetPosition().x <= testNodeCopy.back().x && character->GetTransform().GetPosition().z == testNodeCopy.back().z) {
	////	testNodeCopy.pop_back();
	////}
	////if (character->GetTransform().GetPosition().x >= testNodeCopy.back().x && character->GetTransform().GetPosition().z == testNodeCopy.back().z) {
	////	testNodeCopy.pop_back();
	////}
	////if (character->GetTransform().GetPosition().x == testNodeCopy.back().x && character->GetTransform().GetPosition().z >= testNodeCopy.back().z) {
	////	testNodeCopy.pop_back();
	////}
	////if (character->GetTransform().GetPosition().x == testNodeCopy.back().x && character->GetTransform().GetPosition().z <= testNodeCopy.back().z) {
	////	testNodeCopy.pop_back();
	////}


	//timer += dt; 

	//if (timer > 1 && testNodeCopy.size() >2) {
	//	testNodeCopy.pop_back();
	//	timer = 0;
	//}

	//if (testNodeCopy.end()[-2].x < testNodeCopy.back().x) {


	//	OppositeForce();
	//	character->GetPhysicsObject()->ClearForces();
	//	character->GetPhysicsObject()->AddTorque({ 0,2,0 });
	//	character->GetPhysicsObject()->AddForce({ -8,0,0 });

	//	tempforce = { -5,0,0 };
	//}

	//else if (testNodeCopy.end()[-2].x > testNodeCopy.back().x) {


	//	OppositeForce();
	//	character->GetPhysicsObject()->ClearForces();
	//	character->GetPhysicsObject()->AddTorque({ 0,-2,0 });
	//	character->GetPhysicsObject()->AddForce({ 8, 0, 0 });

	//	tempforce = { 5, 0, 0 };
	//}

	//else if (testNodeCopy.end()[-2].z < testNodeCopy.back().z) {
	//	OppositeForce();
	//	character->GetPhysicsObject()->ClearForces();
	//	character->GetPhysicsObject()->AddTorque({ 0,-2,0 });
	//	character->GetPhysicsObject()->AddForce({ 0, 0, -8 });

	//	tempforce = { 0, 0, -5 };
	//}
	//else if (testNodeCopy.end()[-2].z > testNodeCopy.back().z) {
	//	OppositeForce();
	//	character->GetPhysicsObject()->ClearForces();
	//	character->GetPhysicsObject()->AddTorque({ 0,2,0 });
	//	character->GetPhysicsObject()->AddForce({ 0, 0, 8 });

	//	tempforce = { 0, 0, 5 };
	//}




	float offset = 4;
	Vector3 enemyPos = enemy->GetTransform().GetPosition();


	if (enemyPos.x < testNodeCopy.back().x + offset && enemyPos.x > testNodeCopy.back().x - offset &&
		enemyPos.z < testNodeCopy.back().z + offset && enemyPos.z > testNodeCopy.back().z - offset) {

		enemyPos.y = 0;

		if (testNodeCopy.size() >= 2) {
			testNodeCopy.pop_back();
		}


		NavigationPath path;
		NavigationGrid grid("TestGrid1.txt");
		bool found = grid.FindPath(enemyPos, Vector3(160, 0, 160), path);

		//DisplayPathfinding();

	}


	Vector3 direction = testNodeCopy.back() - enemyPos;
	direction.Normalise();
	enemy->GetPhysicsObject()->AddForce(direction * 10.0f);



}


void TutorialGame::OppositeForce() {

	//if (tempforce.x > 0) {
	//	character->GetPhysicsObject()->AddForce({ -16,0,0 });

	//}
	//else if (tempforce.x < 0) {
	//	character->GetPhysicsObject()->AddForce({ 16,0,0 });

	//}
	//else if (tempforce.z > 0) {
	//	character->GetPhysicsObject()->AddForce({ 0,0,-16 });

	//}
	//else if (tempforce.z < 0) {
	//	character->GetPhysicsObject()->AddForce({ 0,0, 16 });
	//}
	//else if (tempforce.x == 0 && tempforce.z == 0) {
	//	character->GetPhysicsObject()->AddForce({ 0,0,0 });
	//}
}



void TutorialGame::UpdateKeys() {


	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
		lockedObject = nullptr;
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
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD0)) {
		selectionObject = player;
		inSelectionMode = true;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM0)) {
		displayPath = !displayPath;
	}
	if (displayPath == true) {
		DisplayPathfinding();
	}
}

void TutorialGame::LockedObjectMovement() {
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

void TutorialGame::DebugObjectMovement() {
	//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		Debug::DrawLine(Vector3(0, 10, 0), Vector3(0, 10, 0.3f * 3), Vector4(1, 0, 0, 1));
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
	testStateObject = AddStateObjectToWorld(Vector3(0, 10, 0));


	enemy->GetTransform().SetPosition(testNodeCopy.back());
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

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

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
	AddPlayerToWorld(Vector3(0, 5, 0));
	AddEnemyToWorld(Vector3(5, 5, 0));
	AddBonusToWorld(Vector3(10, 5, 0));
	AddBonusToWorld(Vector3(20, 5, 0));
}

GameObject* TutorialGame::AddPlayerToWorld(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 0.5f;

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

	world->AddGameObject(enemy);

	return enemy;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position) {
	GameObject* apple = new GameObject();

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

	apple->GetPhysicsObject()->SetInverseMass(0.1f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}


StateGameObject* TutorialGame::AddStateObjectToWorld(const Vector3& position) {
	StateGameObject* apple = new StateGameObject();

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

	apple->GetPhysicsObject()->SetInverseMass(0.1f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
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
		renderer->DrawString("Press Q to change to camera mode!", Vector2(5, 85));

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
		renderer->DrawString("Press Q to change to select mode!", Vector2(5, 85));
	}

	if (lockedObject) {
		renderer->DrawString("Press L to unlock object!", Vector2(5, 80));
	}

	else if (selectionObject) {

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

		if (selectionObject->GetName() != "player") {
			Debug::Print(selectionObject->GetName() + "(" + std::to_string(selectionObject->GetWorldID()) + ")", Vector2(screenX - 5, 100 - (screenY)+2), 20, Debug::CYAN);
		}

		string debugInfos[8];

		std::string nameInfo = "Name: " + selectionObject->GetName();
		std::string posInfo = "Position: (" + std::to_string(selectionObject->GetTransform().GetPosition().x) + "," + std::to_string(selectionObject->GetTransform().GetPosition().y) + "," + std::to_string(selectionObject->GetTransform().GetPosition().z) + ')';
		std::string oriInfo = "Orientation: (" + std::to_string(selectionObject->GetTransform().GetOrientation().x) + "," + std::to_string(selectionObject->GetTransform().GetOrientation().y) + "," + std::to_string(selectionObject->GetTransform().GetOrientation().z) + "," + std::to_string(selectionObject->GetTransform().GetOrientation().w) + ')';
		std::string forceInfo = "Force: (" + std::to_string(selectionObject->GetPhysicsObject()->GetForce().x) + "," + std::to_string(selectionObject->GetPhysicsObject()->GetForce().y) + "," + std::to_string(selectionObject->GetPhysicsObject()->GetForce().z) + ')';
		std::string massInfo = "Inverse Mass: " + std::to_string(selectionObject->GetPhysicsObject()->GetInverseMass());
		std::string aiStateInfo = "AI State: ";/////////////////////////////
		std::string worldIDInfo = "WorldID: " + std::to_string(selectionObject->GetWorldID());
		std::string isActiveinfo = "IsActive: " + std::to_string(selectionObject->IsActive());
		debugInfos[0] = nameInfo;
		debugInfos[1] = posInfo;
		debugInfos[2] = oriInfo;
		debugInfos[3] = forceInfo;
		debugInfos[4] = massInfo;
		debugInfos[5] = aiStateInfo;
		debugInfos[6] = worldIDInfo;
		debugInfos[7] = isActiveinfo;

		int a = 15;
		for (int i = 0; i < 8; i++) {
			a += 3;
			Debug::Print(debugInfos[i], Vector2(2, a), 16);
		}
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


/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/
void TutorialGame::MoveSelectedObject() {
	renderer->DrawString("Click Force :" + std::to_string(forceMagnitude), Vector2(35, 5)); // Draw debug text at 10 ,20
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