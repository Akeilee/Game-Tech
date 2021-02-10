#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "StateGameObject.h"
#include "StateGameObjectUD.h"
#include "../CSC8503Common/BehaviourSequence.h"
#include "../CSC8503Common/BehaviourAction.h"

namespace NCL {
	namespace CSC8503 {

		enum class EnemyState {
			MOVING,
			ATTACK,
			COLLECT,
			BONUSBALL,
			IDLE,
		};


		class TutorialGame {
		public:
			TutorialGame();

			void LoseGame(float dt);
			void WinGame(float dt);

			~TutorialGame();

			virtual void UpdateGame(float dt);

			void SetPracticeMode(bool p) {
				practiceMode = p;
			}
			bool GetPracticeMode() {
				return practiceMode;
			}

			void SetRestart(bool r) {
				restart = r;
			}
			bool GetRestart() {
				return restart;
			}

			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			bool useGravity;


		protected:

			void CoinMineMovement(float dt);
			void PlaneBonusIntersection();
			void TextOnScreen(float dt);
			void EnemyAI(float dt);
			void FallenOffStage();

			vector <Vector3> testNodes;
			vector <Vector3> testNodeCopy;
			void TestPathfinding();
			void DisplayPathfinding();

			void InitBehavTreeExample();

			GameObject* AddBehavCoin(string name, Vector4 colour, Vector3 position);

			void TestBehaviourTree(float dt);

			GameObject* AddJumpPad(const Vector3& position, const Vector3& size);
			GameObject* SlowFloor();
			GameObject* FastFloor(const Vector3& position, const Vector3& size);
			GameObject* FloatingFloor(const Vector3& position, const Vector3& size);
			GameObject* OBBFloor(const Vector3& position, const Vector3& size);

			void SpawnFallingObject(float dt);

			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void InitWorld();

			void Walls();

			void InitGameExamples();

			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void InitDefaultFloor();
			void BridgeConstraintTest();

			void BallConstraint();

			bool SelectObject();
			void PrintDebugInfo();
			void MoveSelectedObject();

			void DebugObjectMovement();
			void MovePlayer();
			void LockedObjectMovement();

			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);

			GameObject* AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass = 10.0f);

			GameObject* AddPlayerToWorld(const Vector3& position);
			GameObject* AddEnemyToWorld(const Vector3& position);
			GameObject* AddCoinMiner(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);

			GameObject* AddBonusBall(const Vector3& position);

			GameObject* player; /////
			GameObject* enemy; /////


			//tut9
			StateGameObject* AddStateObjectToWorld(const Vector3& position);
			StateGameObject* testStateObject;

			StateGameObjectUD* AddUpDownState(const Vector3& position);
			StateGameObjectUD* upDownStateObject;


			GameTechRenderer* renderer;
			PhysicsSystem* physics;
			GameWorld* world;

			bool inSelectionMode;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;

			OGLMesh* capsuleMesh = nullptr;
			OGLMesh* cubeMesh = nullptr;
			OGLMesh* sphereMesh = nullptr;
			OGLTexture* basicTex = nullptr;
			OGLShader* basicShader = nullptr;

			//Coursework Meshes
			OGLMesh* charMeshA = nullptr;
			OGLMesh* charMeshB = nullptr;
			OGLMesh* enemyMesh = nullptr;
			OGLMesh* bonusMesh = nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject = nullptr;
			Vector3 lockedOffset = Vector3(0, 14, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}


			float enemytimer;
			float enemyAITimer;
			float timer;
			float decreaseScore;
			float spawntimer;
			float delSpawns;
			float coinCounter = 0;
			float behavTimer;
			vector<GameObject*> fallingSpawns;
			int playerScore;
			int enemyScore;
			
			bool loseGame;
			bool winGame;
			bool once;
			bool movePlayer;
			bool displayPath;
			bool testBehavOnce;
			bool goToCoin;

			GameObject* apple;
			GameObject* bonusBall;
			GameObject* movingSphere;
			vector<GameObject*> pingpong;
			vector<GameObject*> testObjects;
			bool practiceMode;

			EnemyState enemyState;

			GameObject* coinMinerAI;
			GameObject* behavCoin;
			vector<GameObject*> behavCoins;
			vector<GameObject*> bonus;

			bool spawnedCoins;

			bool restart;
			bool turnOnAI;
			Vector3 coinPos;
			bool getCoin;
		};
	}
}

