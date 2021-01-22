#include "StateGameObjectUD.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/State.h"

#include "TutorialGame.h"

using namespace NCL;
using namespace CSC8503;


StateGameObjectUD::StateGameObjectUD() {
	counter = 0.0f;
	stateMachine = new StateMachine();

	State* stateA = new State([&](float dt)-> void {
		this->MoveUp(dt);
		}
	);

	State* stateB = new State([&](float dt)-> void {
		this->MoveDown(dt);
		}
	);

	stateMachine->AddState(stateA);
	stateMachine->AddState(stateB);

	stateMachine->AddTransition(new StateTransition(stateA, stateB, [&]() -> bool {
		return //this->counter > 6.0f;
			this->GetTransform().GetPosition().z < 20;
		}
	));

	stateMachine->AddTransition(new StateTransition(stateB, stateA, [&]() -> bool {
		return //this->counter < 0.0f;
			this->GetTransform().GetPosition().z > 55;
		}
	));
}


StateGameObjectUD ::~StateGameObjectUD() {
	delete stateMachine;
}


void StateGameObjectUD::Update(float dt) {
	stateMachine->Update(dt);
}


void StateGameObjectUD::MoveUp(float dt) {
	GetPhysicsObject()->AddForce({ 0, 0, -1000 });
	counter += dt;
}

void StateGameObjectUD::MoveDown(float dt) {
	GetPhysicsObject()->AddForce({ 0, 0,1000 });
	counter -= dt;
}