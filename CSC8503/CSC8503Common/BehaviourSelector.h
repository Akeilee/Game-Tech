#pragma once
#include "BehaviourNodeWithChildren.h"

class BehaviourSelector : public BehaviourNodeWithChildren {

public:

	BehaviourSelector(const std::string& nodeName) : BehaviourNodeWithChildren(nodeName) {}
	~BehaviourSelector() {}

	BehaviourState Execute(float dt) override {
		// std :: cout << " Executing selector " << name << "\n";
		for (auto& i : childNodes) {
			BehaviourState nodeState = i->Execute(dt);
			switch (nodeState) {
			case BehaviourState::Failure: continue; //if failure keep scanning
			case BehaviourState::Success:
			case BehaviourState::Ongoing:
			{
				currentState = nodeState; //if success or ongoing
				return currentState;
			}

			}

		}
		return BehaviourState::Failure; //if all children fails then selctor fails

	}

};