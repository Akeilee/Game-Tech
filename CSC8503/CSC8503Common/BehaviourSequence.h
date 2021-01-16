#pragma once
#include "BehaviourNodeWithChildren.h"

class BehaviourSequence : public BehaviourNodeWithChildren {

public:

	BehaviourSequence(const std::string& nodeName) : BehaviourNodeWithChildren(nodeName) {}
	~BehaviourSequence() {}

	BehaviourState Execute(float dt) override {
		// std :: cout << " Executing sequence " << name << "\n";
		for (auto& i : childNodes) {
			BehaviourState nodeState = i->Execute(dt);
			switch (nodeState) {
			case BehaviourState::Success: continue; //sequence can continue - if success keep scanning
			case BehaviourState::Failure: //if node fails, then it too must fail. Fail state is returned back up behaviour tree,
			case BehaviourState::Ongoing: //and rest of child nodes cannot execute
			{
				currentState = nodeState;
				return nodeState;
			}

			}

		}
		return BehaviourState::Success;

	}

};