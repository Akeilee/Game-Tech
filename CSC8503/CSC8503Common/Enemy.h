#pragma once
#include "GameObject.h"


namespace NCL {
	namespace CSC8503 {
		class GameObject;

		enum class state {
			MOVING,
			ATTACK,
			COLLECT,
			POWERUP,
			IDLE,
		};

		class Enemy : public GameObject {
		public:

		protected:


		};
	}
};


