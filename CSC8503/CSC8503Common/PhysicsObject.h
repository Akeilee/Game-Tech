#pragma once
#include "../../Common/Vector3.h"
#include "../../Common/Matrix3.h"

#include <string>

using namespace NCL::Maths;

namespace NCL {
	class CollisionVolume;

	enum class CollisionType {
		FLOOR,
		PLAYER,
		ENEMY,
		BONUS,
		JUMPPAD,
		MOVINGOBJECT,
		NONE,
	};


	namespace CSC8503 {
		class Transform;

		class PhysicsObject {
		public:
			PhysicsObject(Transform* parentTransform, const CollisionVolume* parentVolume);
			~PhysicsObject();

			Vector3 GetLinearVelocity() const {
				return linearVelocity;
			}

			Vector3 GetAngularVelocity() const {
				return angularVelocity;
			}

			Vector3 GetTorque() const {
				return torque;
			}

			Vector3 GetForce() const {
				return force;
			}

			void SetInverseMass(float invMass) {
				inverseMass = invMass;
			}

			float GetInverseMass() const {
				return inverseMass;
			}

			//tut 6 set bounciness
			void SetCRes(float cRes) {
				cRestitution = cRes;
			}

			float GetCRes() const {
				return cRestitution;
			}

			void SetCollisonType(CollisionType ct) {
				collisionType = ct;
			}
			CollisionType GetCollisionType()const {
				return collisionType;
			}

			//printing enums
			std::string ToString(CollisionType ct) const {
				switch (ct) {
				case CollisionType::FLOOR:
					return "FLOOR";
					break;
				case CollisionType::PLAYER:
					return "PLAYER";
					break;
				case CollisionType::ENEMY:
					return "ENEMY";
					break;
				case CollisionType::BONUS:
					return "BONUS";
					break;
				case CollisionType::JUMPPAD:
					return "JUMPPAD";
					break;
				case CollisionType::MOVINGOBJECT:
					return "MOVINGOBJECT";
					break;
				case CollisionType::NONE:
					return "NONE";
					break;
				}
			}



			void ApplyAngularImpulse(const Vector3& force);
			void ApplyLinearImpulse(const Vector3& force);

			void AddForce(const Vector3& force);

			void AddForceAtPosition(const Vector3& force, const Vector3& position);

			void AddTorque(const Vector3& torque);


			void ClearForces();

			void SetLinearVelocity(const Vector3& v) {
				linearVelocity = v;
			}

			void SetAngularVelocity(const Vector3& v) {
				angularVelocity = v;
			}

			void InitCubeInertia();
			void InitSphereInertia();

			void UpdateInertiaTensor();

			Matrix3 GetInertiaTensor() const {
				return inverseInteriaTensor;
			}


		protected:
			const CollisionVolume* volume;
			Transform* transform;

			float inverseMass;
			float elasticity;
			float friction;
			float cRestitution;

			//linear stuff
			Vector3 linearVelocity;
			Vector3 force;


			//angular stuff
			Vector3 angularVelocity;
			Vector3 torque;
			Vector3 inverseInertia;
			Matrix3 inverseInteriaTensor;


			CollisionType collisionType;
		};
	}
}

