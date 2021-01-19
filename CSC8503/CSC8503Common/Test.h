#pragma once
#include "GameObject.h";
#include "BehaviourNode.h";
#include "BehaviourAction.h";

namespace NCL {
	namespace CSC8503 {
		class BehaviourNode;
		class GameObject;


		class Test {

		public:
			GameObject* gameobject;
			GameObject* GetTransform() { gameobject->GetTransform(); };

			BehaviourAction* findCoin = new BehaviourAction("Find Coin", [&](float dt, BehaviourState state)-> BehaviourState {
				aaa += dt;
				//std::cout << aaa << "\n";
				if (state == BehaviourState::Initialise) {
					std::cout << "Finding Coin!\n";
					Vector3 direction = nCoinPos - coinMinerPos;
					coinMinerAI->GetPhysicsObject()->AddForce(direction * force);
					state = BehaviourState::Ongoing;
				}

				else if (state == BehaviourState::Ongoing) {
					if (physics->minedCoin == true && aaa > 50000) {
						std::cout << "Found Coin !\n";
						noOfCoins++;
						physics->minedCoin = false;
						Vector3 direction = nCoinPos - coinMinerPos;
						coinMinerAI->GetPhysicsObject()->AddForce(direction * force);
						aaa = 0;
						return BehaviourState::Success;
					}

					if (aaa > 50000) {
						aaa = 0;
						return BehaviourState::Failure;
					}
				}
				return state; // will be ’ongoing ’ until success
				}
			);

		protected:


		};