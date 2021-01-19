#include "../../Common/Window.h"

#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"

#include "../CSC8503Common/PushdownState.h"
#include "../CSC8503Common/PushdownMachine.h"

#include "../CSC8503Common/BehaviourAction.h"
#include "../CSC8503Common/BehaviourSequence.h"
#include "../CSC8503Common/BehaviourSelector.h"

#include "../CSC8503Common/NavigationGrid.h"
#include "../CSC8503Common/NavigationMesh.h"

#include "TutorialGame.h"

using namespace NCL;
using namespace CSC8503;



bool exitGame = false;
bool restart = false;
bool practice;



//tut 9
void TestStateMachine() {
	StateMachine* testMachine = new StateMachine();
	int data = 0;

	State* A = new State([&](float dt)->void {  //takes in float and doesn't return anything
		std::cout << "I’m in state A!\n";
		data++;
		}
	);

	State* B = new State([&](float dt)->void {
		std::cout << "I’m in state B!\n";
		data--;
		}
	);

	//captures values by reference
	StateTransition* stateAB = new StateTransition(A, B, [&](void)->bool {  //returns bool to tell state machine whether to transition or not
		return data > 10;
		}
	);
	StateTransition* stateBA = new StateTransition(B, A, [&](void)->bool {
		return data < 0;
		}
	);

	testMachine->AddState(A);
	testMachine->AddState(B);
	testMachine->AddTransition(stateAB);
	testMachine->AddTransition(stateBA);

	for (int i = 0; i < 100; ++i) {
		testMachine->Update(1.0f);
	}

}


////tut 10
//vector <Vector3> testNodes;
//void TestPathfinding() {
//	NavigationGrid grid("TestGrid1.txt");
//
//	NavigationPath outPath; //making path 
//
//	Vector3 startPos(80, 0, 10);
//	Vector3 endPos(180, 0, 180);
//
//	bool found = grid.FindPath(startPos, endPos, outPath); //finding path and putting it into outPath
//
//	Vector3 pos;
//	while (outPath.PopWaypoint(pos)) { //only pop waypoint if AI is close enough to node
//		testNodes.push_back(pos);
//	}
//}
//
//
//void DisplayPathfinding() {
//	for (int i = 1; i < testNodes.size(); ++i) {
//		Vector3 a = testNodes[i - 1];
//		Vector3 b = testNodes[i];
//
//		Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
//	}
//
//}


//tut 11 - new classes
class PauseScreen : public PushdownState {

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::U)) {
			std::cout << "Going back to game...\n";
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;

	}

	void OnAwake(float dt) override {
		std::cout << "Press U to unpause game!\n";
	}

protected:

};


class GameScreen : public PushdownState {

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {

		Debug::Print("P to pause", Vector2(80, 12));
		Debug::Print("U to unpause", Vector2(80, 18));
		Debug::Print("Esc to main menu", Vector2(80, 24));

		pauseReminder -= dt;
		if (pauseReminder < 0) {
			//std::cout << "Coins mined : " << coinsMined << "\n";
			//std::cout << "Press P to pause game , or F1 to return to main menu !\n";
			pauseReminder += 1.0f;
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::P)) {
			std::cout << "Paused Game\n";
			*newState = new PauseScreen();
			return PushdownResult::Push;
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {  //goes back to initial state
			std::cout << "Returning to main menu!\n";
			return PushdownResult::Pop;
		}

		if (g->GetRestart() == true) {
			restart = true;
			g->SetRestart(false);
			std::cout << "Restarting Game...\n";
			return PushdownResult::Pop;
		}

		g->UpdateGame(dt);

		return PushdownResult::NoChange;
	};

	void OnAwake(float dt) override {
		std::cout << "Loading Game...\n";

		if (practice == true) {
			g->SetPracticeMode(true);
		}
		if (practice == false) {
			g->SetPracticeMode(false);
		}

	}

protected:
	float pauseReminder = 1;
	TutorialGame* g = new TutorialGame();

};


class IntroScreen : public PushdownState {

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		//Debug::SetRenderer(renderer);
		//Debug::FlushRenderables(dt);

		world->UpdateWorld(dt);
		renderer->Update(dt);
		renderer->Render();

		renderer->DrawString("Welcome!", Vector2(40, 20), 50, Vector4(0.9, 0.1, 0.6, 1));

		if (counter <= 0) {
			counter = 0;
		}
		if (counter >= 2) {
			counter = 2;
		}

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::DOWN)|| Window::GetKeyboard()->KeyPressed(KeyboardKeys::S)) {
			counter++;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::UP) || Window::GetKeyboard()->KeyPressed(KeyboardKeys::W)) {
			counter--;
		}

		switch (counter) {
		case 0:
			renderer->DrawString("New Game", Vector2(40, 50), 30, Vector4(1, 0.9, 0, 1));
			renderer->DrawString("Practice Mode", Vector2(40, 60), 30);
			renderer->DrawString("Exit", Vector2(40, 70), 30);
			break;
		case 1:
			renderer->DrawString("New Game", Vector2(40, 50), 30);
			renderer->DrawString("Practice Mode", Vector2(40, 60), 30, Vector4(1, 1, 0, 1));
			renderer->DrawString("Exit", Vector2(40, 70), 30);
			break;
		case 2:
			renderer->DrawString("New Game", Vector2(40, 50), 30);
			renderer->DrawString("Practice Mode", Vector2(40, 60), 30);
			renderer->DrawString("Exit", Vector2(40, 70), 30, Vector4(1, 1, 0, 1));
			break;
		}

		if (restart == true) {
			practice = false;
			restart = false;
			std::cout << "Restarted Game!\n";
			*newState = new GameScreen();
			return PushdownResult::Push;
		}

		if ((Window::GetKeyboard()->KeyPressed(KeyboardKeys::RETURN) && counter == 0)) {
			practice = false;
			*newState = new GameScreen();
			return PushdownResult::Push;
		}
		if ((Window::GetKeyboard()->KeyPressed(KeyboardKeys::RETURN) && counter == 1)) {
			practice = true;
			*newState = new GameScreen();
			return PushdownResult::Push;
		}
		if ((Window::GetKeyboard()->KeyPressed(KeyboardKeys::RETURN) && counter == 2) || Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE)) {
			exitGame = true;
			return PushdownResult::Pop;
		}

		
		return PushdownResult::NoChange;
	};

	void OnAwake(float dt) override {
		std::cout << "Game Menu\n";
	}

protected:
	GameWorld* world = new GameWorld();
	GameTechRenderer* renderer = new GameTechRenderer(*world);
	int counter = 0;

};


void TestPushdownAutomata(Window* w) {

	PushdownMachine machine(new IntroScreen());

	while (w->UpdateWindow()) {
		float dt = w->GetTimer()->GetTimeDeltaSeconds();
		if (!machine.Update(dt)) {
			return;
		}
	}

}



//tut 12
void TestBehaviourTree() {
	float behaviourTimer;
	float distanceToTarget;

	BehaviourAction* findKey = new BehaviourAction("Find Key", [&](float dt, BehaviourState state)-> BehaviourState {
		if (state == BehaviourState::Initialise) {
			std::cout << " Looking for a key !\n";
			behaviourTimer = rand() % 100;
			state = BehaviourState::Ongoing;
		}

		else if (state == BehaviourState::Ongoing) {
			behaviourTimer -= dt;  //everytime state is executed, timer will decrement
			if (behaviourTimer <= 0.0f) {
				std::cout << "Found a key !\n";
				return BehaviourState::Success;
			}
		}

		return state; // will be ’ongoing ’ until success
		}
	);


	BehaviourAction* goToRoom = new BehaviourAction("Go To Room", [&](float dt, BehaviourState state)-> BehaviourState {
		if (state == BehaviourState::Initialise) {
			std::cout << "Going to the loot room !\n";
			state = BehaviourState::Ongoing;
		}

		else if (state == BehaviourState::Ongoing) {
			distanceToTarget -= dt;
			if (distanceToTarget <= 0.0f) {
				std::cout << " Reached room !\n";
				return BehaviourState::Success;
			}
		}

		return state; // will be ’ongoing ’ until success
		}
	);


	BehaviourAction* openDoor = new BehaviourAction("Open Door", [&](float dt, BehaviourState state)-> BehaviourState {
		if (state == BehaviourState::Initialise) {  //doesn't take time to execute as just opening door
			std::cout << " Opening Door !\n";
			return BehaviourState::Success;
		}

		return state;
		}
	);


	BehaviourAction* lookForTreasure = new BehaviourAction("Look For Treasure ", [&](float dt, BehaviourState state)-> BehaviourState {
		if (state == BehaviourState::Initialise) {
			std::cout << " Looking for treasure !\n";
			return BehaviourState::Ongoing;
		}

		else if (state == BehaviourState::Ongoing) {
			bool found = rand() % 2; //random either success or failure
			if (found) {
				std::cout << "I found some treasure !\n";
				return BehaviourState::Success;
			}
			std::cout << "No treasure in here ...\n";
			return BehaviourState::Failure;
		}

		return state;
		}
	);


	BehaviourAction* lookForItems = new BehaviourAction("Look For Items ", [&](float dt, BehaviourState state)-> BehaviourState { //if no treasure, maybe there are items to take
		if (state == BehaviourState::Initialise) {
			std::cout << " Looking for items !\n";
			return BehaviourState::Ongoing;
		}

		else if (state == BehaviourState::Ongoing) {
			bool found = rand() % 2;
			if (found) {
				std::cout << "I found some items !\n";
				return BehaviourState::Success;
			}
			std::cout << "No items in here ...\n";
			return BehaviourState::Failure;
		}

		return state;
		}
	);


	//connecting states together by add children
	BehaviourSequence* sequence = new BehaviourSequence("Room Sequence "); //sequence
	sequence->AddChild(findKey);
	sequence->AddChild(goToRoom);
	sequence->AddChild(openDoor);

	BehaviourSelector* selection = new BehaviourSelector("Loot Selection "); //selector
	selection->AddChild(lookForTreasure);
	selection->AddChild(lookForItems);

	BehaviourSequence* rootSequence = new BehaviourSequence("Root Sequence "); //root adding sequence and selector
	rootSequence->AddChild(sequence);
	rootSequence->AddChild(selection);

	for (int i = 0; i < 5; ++i) { //running behaviour tree 5 times, resetting each time
		rootSequence->Reset();
		behaviourTimer = 0.0f;
		distanceToTarget = rand() % 250;
		BehaviourState state = BehaviourState::Ongoing;
		std::cout << "We’re going on an adventure !\n";

		while (state == BehaviourState::Ongoing) {
			state = rootSequence->Execute(1.0f); // fake dt
		}

		if (state == BehaviourState::Success) { //only success if both child states are successful - only successful if either treasure or item has been found
			std::cout << "What a successful adventure !\n";
		}

		else if (state == BehaviourState::Failure) {
			std::cout << "What a waste of time !\n";
		}

	}

	std::cout << "All done !\n";

}





/*
The main function should look pretty familar to you!
We make a window, and then go into a while loop that repeatedly
runs our 'game' until we press escape. Instead of making a 'renderer'
and updating it, we instead make a whole game, and repeatedly update that,
instead.

This time, we've added some extra functionality to the window class - we can
hide or show the
*/

int main() {
	Window* w = Window::CreateGameWindow("CSC8503 Game technology!", 1280, 720);

	TestPushdownAutomata(w); ////////////////////// pause game, menu, etc


	if (!w->HasInitialised()) {
		return -1;
	}
	srand(time(0));
	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);

	//TutorialGame* g = new TutorialGame();

	//g->TestPathfinding(); //////////////////////////// press 0 to show

	//TestBehaviourTree(); /////////////////// find room, key, items, etc


	w->GetTimer()->GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!

	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE) && exitGame != true) {
		float dt = w->GetTimer()->GetTimeDeltaSeconds();


		if (dt > 0.1f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::PRIOR)) {
			w->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NEXT)) {
			w->ShowConsole(false);
		}

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::T)) {
			w->SetWindowPosition(0, 0);
		}


		//TestStateMachine();


		w->SetTitle("8503 Coursework JLee || Gametech frame time: " + std::to_string(1000.0f * dt));

		//g->UpdateGame(dt);



	}



	Window::DestroyGameWindow();
}