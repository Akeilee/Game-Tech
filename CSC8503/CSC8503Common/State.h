#pragma once
#include <functional>

namespace NCL {
	namespace CSC8503 {

		typedef std::function <void(float)> StateUpdateFunction; //function doesn't return anything but takes in float parameter represented as timestep

		class State {
		public:
			State() {}

			State(StateUpdateFunction someFunc) {
				func = someFunc;
			}

			void Update(float dt) {
				if (func != nullptr) {
					func(dt);
				}
			}

		protected:
			StateUpdateFunction func;
		};
	}
}