#pragma once
#include "BehaviourNode.h"
#include <vector>

class BehaviourNodeWithChildren : public BehaviourNode {

public:
	BehaviourNodeWithChildren(const std::string& nodeName) : BehaviourNode(nodeName) {};

	~BehaviourNodeWithChildren() {
		for (auto& i : childNodes) {
			delete i;
		}
	}

	void AddChild(BehaviourNode* n) {
		childNodes.emplace_back(n);
	}

	void Reset() override {
		currentState = BehaviourState::Initialise;
		for (auto& i : childNodes) {  //resetting all child nodes to initialise too
			i->Reset();
		}
	}


protected:
	std::vector <BehaviourNode*> childNodes;

};