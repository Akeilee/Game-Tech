#include "StateGameObject.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/State.h"

#include "TutorialGame.h"

using namespace NCL;
using namespace CSC8503;


StateGameObject::StateGameObject() {
	counter = 0.0f;
	stateMachine = new StateMachine();

	State* stateA = new State([&](float dt)-> void {
		this->MoveLeft(dt);
		}
	);

	State* stateB = new State([&](float dt)-> void {
		this->MoveRight(dt);
		}
	);

	stateMachine->AddState(stateA);
	stateMachine->AddState(stateB);

	stateMachine->AddTransition(new StateTransition(stateA, stateB, [&]() -> bool {
		return //this->counter > 6.0f;
			this->GetTransform().GetPosition().x < -85;
		}
	));

	stateMachine->AddTransition(new StateTransition(stateB, stateA, [&]() -> bool {
		return //this->counter < 0.0f;
			this->GetTransform().GetPosition().x > -65;
		}
	));
}


StateGameObject ::~StateGameObject() {
	delete stateMachine;
}


void StateGameObject::Update(float dt) {
	stateMachine->Update(dt);
}


void StateGameObject::MoveLeft(float dt) {
	GetPhysicsObject()->AddForce({ -900, 0, 0 });
	counter += dt;
}

void StateGameObject::MoveRight(float dt) {
	GetPhysicsObject()->AddForce({ 900, 0, 0 });
	counter -= dt;
}