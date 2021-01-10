#pragma once
#include "../CSC8503Common/GameWorld.h"
#include <set>

namespace NCL {
	namespace CSC8503 {
		class PhysicsSystem	{
		public:
			PhysicsSystem(GameWorld& g);
			~PhysicsSystem();

			void Clear();

			void Update(float dt);

			void UseGravity(bool state) {
				applyGravity = state;
			}

			void SetGlobalDamping(float d) {
				globalDamping = d;
			}

			void SetGravity(const Vector3& g);

			float GetLinearDamping() { return linearDamping; };
			void SetLinearDaming(float d) { linearDamping = d; };

			bool getPBonus() { return pBonusAdd; };
			bool getEBonus() { return eBonusAdd; };

			void setPBonus(bool b) { pBonusAdd = b; };
			void setEbonus(bool b) { eBonusAdd = b; };

		protected:
			void BasicCollisionDetection();
			void BroadPhase();
			void NarrowPhase();

			void ClearForces();

			void IntegrateAccel(float dt);
			void IntegrateVelocity(float dt);

			void UpdateConstraints(float dt);

			void UpdateCollisionList();
			void UpdateObjectAABBs();

			void ImpulseResolveCollision(GameObject& a , GameObject&b, CollisionDetection::ContactPoint& p) ;

			GameWorld& gameWorld;

			bool	applyGravity;
			Vector3 gravity;
			float	dTOffset;
			float	globalDamping;

			std::set<CollisionDetection::CollisionInfo> allCollisions;
			std::set <CollisionDetection::CollisionInfo> broadphaseCollisions; //store collision pair for broadphase. Using set ensures unique values - no duplicates. Checks collisions once per object

			bool useBroadPhase		= true;
			int numCollisionFrames	= 5;

			float linearDamping;

			bool pBonusAdd = false;
			bool eBonusAdd = false;
			
		};
	}
}

