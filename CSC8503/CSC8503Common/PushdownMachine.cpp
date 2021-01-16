#include "PushdownMachine.h"
#include "PushdownState.h"
using namespace NCL::CSC8503;


bool PushdownMachine::Update(float dt) {
	if (activeState) {
		PushdownState* newState = nullptr;
		PushdownState::PushdownResult result = activeState->OnUpdate(dt, &newState);  //pushdownresult is enum

		switch (result) {

		case PushdownState::Pop:
		{
			activeState->OnSleep();
			delete activeState;
			stateStack.pop(); //state finished what it was doing

			if (stateStack.empty()) {
				return false;
			}
			else {
				activeState = stateStack.top();
				activeState->OnAwake(dt);
			}

		}break;

		case PushdownState::Push:
		{
			activeState->OnSleep();

			stateStack.push(newState);
			activeState = newState;
			activeState->OnAwake(dt);

		}break;

		}

	}
	else {  //setting up first state
		stateStack.push(initialState);
		activeState = initialState;
		activeState->OnAwake(dt);
	}

	return true;

}