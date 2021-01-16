#pragma once
#include "State.h"

namespace NCL {
	namespace CSC8503 {

		class PushdownState {

		public:
			enum PushdownResult {
				Push, Pop, NoChange
			};

			PushdownState() {}
			virtual ~PushdownState() {}

			virtual PushdownResult OnUpdate(float dt, PushdownState** pushFunc) = 0;  //ptr to ptr to instantiate which new state to operate on
			virtual void OnAwake(float dt) {}
			virtual void OnSleep() {}
			
		};
	}
}

