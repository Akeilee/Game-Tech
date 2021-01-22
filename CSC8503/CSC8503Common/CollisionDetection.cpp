#include "CollisionDetection.h"
#include "CollisionVolume.h"
#include "AABBVolume.h"
#include "OBBVolume.h"
#include "SphereVolume.h"
#include "../../Common/Vector2.h"
#include "../../Common/Window.h"
#include "../../Common/Maths.h"
#include "Debug.h"

#include <list>
#include <algorithm>  

using namespace NCL;

Vector3 pointFromPlaneIntersect;

Vector3 collidedat;


bool CollisionDetection::RayPlaneIntersection(const Ray& r, const Plane& p, RayCollision& collisions) {
	float ln = Vector3::Dot(p.GetNormal(), r.GetDirection());  //bottom of t equation. plane normal = a,b,c

	if (ln == 0.0f) {
		return false; //direction vectors are perpendicular!
	}

	Vector3 planePoint = p.GetPointOnPlane(); //plane point = x,y,z

	Vector3 pointDir = planePoint - r.GetPosition(); //distance of plane from ray

	float d = Vector3::Dot(pointDir, p.GetNormal()) / ln; // (t) - how far along ray we need to travel to hit plane

	collisions.collidedAt = r.GetPosition() + (r.GetDirection() * d); //p where ray collides with plane

	pointFromPlaneIntersect = r.GetPosition() + (r.GetDirection() * d); /////////////////////

	return true;
}


bool CollisionDetection::PlaneSphereIntersection(const SphereVolume& volumeA, const Plane& p, const Transform& worldTransform, CollisionInfo& collisionInfo) {

	float r = volumeA.GetRadius();
	Vector3 pNormal = p.GetNormal();
	Vector3 planePoint = p.GetPointOnPlane();

	Vector3 spherePos = worldTransform.GetPosition();

	Vector3 pointDir = planePoint - spherePos;
	float d = Vector3::Dot(pointDir, p.GetNormal());

	return p.SphereInPlane(spherePos, r);
}


bool CollisionDetection::RayIntersection(const Ray& r, GameObject& object, RayCollision& collision) {
	bool hasCollided = false;

	const Transform& worldTransform = object.GetTransform();
	const CollisionVolume* volume = object.GetBoundingVolume();

	//if no collision volume then return false
	if (!volume) {
		return false;
	}

	switch (volume->type) {
	case VolumeType::AABB:		hasCollided = RayAABBIntersection(r, worldTransform, (const AABBVolume&)*volume, collision); break;
	case VolumeType::OBB:		hasCollided = RayOBBIntersection(r, worldTransform, (const OBBVolume&)*volume, collision); break;
	case VolumeType::Sphere:	hasCollided = RaySphereIntersection(r, worldTransform, (const SphereVolume&)*volume, collision); break;
	case VolumeType::Capsule:	hasCollided = RayCapsuleIntersection(r, worldTransform, (const CapsuleVolume&)*volume, collision); break;
	}

	return hasCollided;
}

bool CollisionDetection::RayBoxIntersection(const Ray& r, const Vector3& boxPos, const Vector3& boxSize, RayCollision& collision) {
	Vector3 boxMin = boxPos - boxSize; //box max min value of axis
	Vector3 boxMax = boxPos + boxSize;

	Vector3 rayPos = r.GetPosition();
	Vector3 rayDir = r.GetDirection();

	Vector3 tVals(-1, -1, -1);

	//if rayDir is going left, check right of box; 
	//if rayDir going up, check box bottom;   
	//if rayDir going forward, check box back;
	for (int i = 0; i < 3; ++i) { // get best 3 intersections
		if (rayDir[i] > 0) {
			tVals[i] = (boxMin[i] - rayPos[i]) / rayDir[i];
		}
		else if (rayDir[i] < 0) {
			tVals[i] = (boxMax[i] - rayPos[i]) / rayDir[i];
		}
	}

	float bestT = tVals.GetMaxElement();

	if (bestT < 0.0f) {
		return false; //no backwards rays! interesction is behind ray
	}


	Vector3 intersection = rayPos + (rayDir * bestT);
	const float epsilon = 0.0001f; //an amount of leeway in our calcs

	//checking every axis
	for (int i = 0; i < 3; ++i) {
		if (intersection[i] + epsilon < boxMin[i] || intersection[i] - epsilon > boxMax[i]) {
			return false; // best intersection doesn’t touch the box!
		}
	}

	collision.collidedAt = intersection;
	collision.rayDistance = bestT;

	return true;
}

bool CollisionDetection::RayAABBIntersection(const Ray& r, const Transform& worldTransform, const AABBVolume& volume, RayCollision& collision) {
	Vector3 boxPos = worldTransform.GetPosition();
	Vector3 boxSize = volume.GetHalfDimensions();
	return RayBoxIntersection(r, boxPos, boxSize, collision);

}


bool CollisionDetection::RayOBBIntersection(const Ray& r, const Transform& worldTransform, const OBBVolume& volume, RayCollision& collision) {
	Quaternion orientation = worldTransform.GetOrientation();
	Vector3 position = worldTransform.GetPosition();

	Matrix3 transform = Matrix3(orientation);
	Matrix3 invTransform = Matrix3(orientation.Conjugate());

	Vector3 localRayPos = r.GetPosition() - position;

	//transform ray to local space of box
	Ray tempRay(invTransform * localRayPos, invTransform * r.GetDirection());  //position, direction

	bool collided = RayBoxIntersection(tempRay, Vector3(), volume.GetHalfDimensions(), collision); //using AABB test after transforming to local origin

	if (collided) {
		//if collided, transform collision point back into world spcace
		collision.collidedAt = transform * collision.collidedAt + position;

		collidedat = transform * collision.collidedAt + position;
	}

	return collided;

}

bool CollisionDetection::RayCapsuleIntersection(const Ray& r, const Transform& worldTransform, const CapsuleVolume& volume, RayCollision& collision) {

	Vector3 capsulePos = worldTransform.GetPosition(); //1st axis 
	Vector3 upPos = worldTransform.GetOrientation() * Vector3(0, 1, 0);; //2nd axis 
	upPos = upPos.Normalised();
	Vector3 direction = capsulePos - r.GetPosition();
	direction = direction.Normalised();
	Vector3 orthogonal = Vector3::Cross(direction, upPos);
	Vector3 planeFacingRay = (capsulePos + orthogonal); //3rd axis


	Plane tempPlane = Plane::PlaneFromTri(capsulePos, capsulePos + upPos, planeFacingRay);

	RayPlaneIntersection(r, tempPlane, collision);
	Vector3 point = pointFromPlaneIntersect; //collision point p

	Vector3 topSpherePos = capsulePos + upPos * (volume.GetHalfHeight() - volume.GetRadius());
	Vector3 botSpherePos = capsulePos - upPos * (volume.GetHalfHeight() - volume.GetRadius());

	float distanceTop = (point - topSpherePos).Length(); //intersection point and top pos
	float distanceBot = (point -botSpherePos ).Length(); //interesection point and bot pos

	Vector3 vATop = (topSpherePos - point);
	Vector3 vABot = (botSpherePos - point);
	Vector3 vBTop = topSpherePos - capsulePos;
	Vector3 vBBot = botSpherePos - capsulePos;
	float vAvBTop = Vector3::Dot(vATop, vBTop);
	float vAvBBot = Vector3::Dot(vABot, vBBot);

	Vector3 d = capsulePos + upPos * (Vector3::Dot(point - capsulePos, upPos));
	//Debug::DrawLine(point, point + Vector3(0, 2, 0), Debug::RED, 2.0f);
	SphereVolume* vol = new SphereVolume(volume.GetRadius()+0.25); //0.25 is offset


	if (vAvBTop < 0) { //interesection above cylinder section
		if (distanceBot > volume.GetRadius()) {
			capsulePos = topSpherePos;
		}
		else
			return false;
	}

	else if (vAvBBot < 0) { //intersection below cylinder section
		if (distanceTop > volume.GetRadius()) {
			//capsulePos = botSpherePos;
		}
		else
			return false;
	}

	else if (d.Length() - point.Length() < volume.GetRadius()) { //inside cylinder
		capsulePos.y = point.y;
		//return RaySphereIntersection(r, worldTransform, *vol, collision);	
	}
	else {
		return false;
	}

	
	Vector3 dir = (capsulePos - r.GetPosition());
	float project = Vector3::Dot(dir, r.GetDirection());
	Vector3 pointCap = r.GetPosition() + (r.GetDirection() * project);

	float capsuleDistance = (pointCap - capsulePos).Length();
	float offset = sqrt((volume.GetRadius() * volume.GetRadius()) - (capsuleDistance * capsuleDistance));

	collision.rayDistance = project - (offset);
	collision.collidedAt = r.GetPosition() + (r.GetDirection() * collision.rayDistance);

	return true;

}

bool CollisionDetection::RaySphereIntersection(const Ray& r, const Transform& worldTransform, const SphereVolume& volume, RayCollision& collision) {
	Vector3 spherePos = worldTransform.GetPosition();
	float sphereRadius = volume.GetRadius();

	// Get the direction between the ray origin and the sphere origin/centre
	Vector3 dir = (spherePos - r.GetPosition());

	// Then project the sphere’s origin onto our ray direction vector
	float sphereProj = Vector3::Dot(dir, r.GetDirection());

	if (sphereProj < 0.0f) {
		return false; // point is behind the ray!
	}

	// Get closest point on ray line to sphere
	Vector3 point = r.GetPosition() + (r.GetDirection() * sphereProj);

	//distance from sphere origin to point p
	float sphereDist = (point - spherePos).Length();

	if (sphereDist > sphereRadius) {
		return false;
	}

	//finding true intersection point. r.getpos() travels along by rayDistance to intersect sphere
	float offset = sqrt((sphereRadius * sphereRadius) - (sphereDist * sphereDist));

	collision.rayDistance = sphereProj - (offset);
	collision.collidedAt = r.GetPosition() + (r.GetDirection() * collision.rayDistance);

	return true;
}



Matrix4 GenerateInverseView(const Camera& c) {
	float pitch = c.GetPitch();
	float yaw = c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix4::Translation(position) *
		Matrix4::Rotation(-yaw, Vector3(0, -1, 0)) *
		Matrix4::Rotation(-pitch, Vector3(-1, 0, 0));

	return iview;
}

Vector3 CollisionDetection::Unproject(const Vector3& screenPos, const Camera& cam) {
	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	float aspect = screenSize.x / screenSize.y;
	float fov = cam.GetFieldOfVision();
	float nearPlane = cam.GetNearPlane();
	float farPlane = cam.GetFarPlane();

	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(cam) * GenerateInverseProjection(aspect, fov, nearPlane, farPlane);

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(screenPos.x / (float)screenSize.x) * 2.0f - 1.0f,
		(screenPos.y / (float)screenSize.y) * 2.0f - 1.0f,
		(screenPos.z),
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

Ray CollisionDetection::BuildRayFromMouse(const Camera& cam) {
	Vector2 screenMouse = Window::GetMouse()->GetAbsolutePosition();
	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	//We remove the y axis mouse position from height as OpenGL is 'upside down',
	//and thinks the bottom left is the origin, instead of the top left!
	Vector3 nearPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		-0.99999f
	);

	//We also don't use exactly 1.0 (the normalised 'end' of the far plane) as this
	//causes the unproject function to go a bit weird. 
	Vector3 farPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		0.99999f
	);

	Vector3 a = Unproject(nearPos, cam);
	Vector3 b = Unproject(farPos, cam);
	Vector3 c = b - a;

	c.Normalise();

	//std::cout << "Ray Direction:" << c << std::endl;

	return Ray(cam.GetPosition(), c);
}

//http://bookofhook.com/mousepick.pdf
Matrix4 CollisionDetection::GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
	Matrix4 m;

	float t = tan(fov * PI_OVER_360);

	float neg_depth = nearPlane - farPlane;

	const float h = 1.0f / t;

	float c = (farPlane + nearPlane) / neg_depth;
	float e = -1.0f;
	float d = 2.0f * (nearPlane * farPlane) / neg_depth;

	m.array[0] = aspect / h;
	m.array[5] = tan(fov * PI_OVER_360);

	m.array[10] = 0.0f;
	m.array[11] = 1.0f / d;

	m.array[14] = 1.0f / e;

	m.array[15] = -c / (d * e);

	return m;
}

/*
And here's how we generate an inverse view matrix. It's pretty much
an exact inversion of the BuildViewMatrix function of the Camera class!
*/
Matrix4 CollisionDetection::GenerateInverseView(const Camera& c) {
	float pitch = c.GetPitch();
	float yaw = c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix4::Translation(position) *
		Matrix4::Rotation(yaw, Vector3(0, 1, 0)) *
		Matrix4::Rotation(pitch, Vector3(1, 0, 0));

	return iview;
}


/*
If you've read through the Deferred Rendering tutorial you should have a pretty
good idea what this function does. It takes a 2D position, such as the mouse
position, and 'unprojects' it, to generate a 3D world space position for it.

Just as we turn a world space position into a clip space position by multiplying
it by the model, view, and projection matrices, we can turn a clip space
position back to a 3D position by multiply it by the INVERSE of the
view projection matrix (the model matrix has already been assumed to have
'transformed' the 2D point). As has been mentioned a few times, inverting a
matrix is not a nice operation, either to understand or code. But! We can cheat
the inversion process again, just like we do when we create a view matrix using
the camera.

So, to form the inverted matrix, we need the aspect and fov used to create the
projection matrix of our scene, and the camera used to form the view matrix.

*/
Vector3	CollisionDetection::UnprojectScreenPosition(Vector3 position, float aspect, float fov, const Camera& c) {
	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(c) * GenerateInverseProjection(aspect, fov, c.GetNearPlane(), c.GetFarPlane());

	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(position.x / (float)screenSize.x) * 2.0f - 1.0f,
		(position.y / (float)screenSize.y) * 2.0f - 1.0f,
		(position.z) - 1.0f,
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

bool CollisionDetection::ObjectIntersection(GameObject* a, GameObject* b, CollisionInfo& collisionInfo) {
	const CollisionVolume* volA = a->GetBoundingVolume();
	const CollisionVolume* volB = b->GetBoundingVolume();

	if (!volA || !volB) {
		return false;
	}

	collisionInfo.a = a;
	collisionInfo.b = b;

	Transform& transformA = a->GetTransform();
	Transform& transformB = b->GetTransform();

	VolumeType pairType = (VolumeType)((int)volA->type | (int)volB->type);

	if (pairType == VolumeType::AABB) {
		return AABBIntersection((AABBVolume&)*volA, transformA, (AABBVolume&)*volB, transformB, collisionInfo);
	}

	if (pairType == VolumeType::Sphere) {
		return SphereIntersection((SphereVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}

	if (pairType == VolumeType::OBB) {
		return OBBIntersection((OBBVolume&)*volA, transformA, (OBBVolume&)*volB, transformB, collisionInfo);
	}

	if (pairType == VolumeType::Capsule) {
		return CapsuleIntersection((CapsuleVolume&)*volA, transformA, (CapsuleVolume&)*volB, transformB, collisionInfo);
	}


	if (volA->type == VolumeType::AABB && volB->type == VolumeType::Sphere) {
		return AABBSphereIntersection((AABBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBSphereIntersection((AABBVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}


	if (volA->type == VolumeType::OBB && volB->type == VolumeType::Sphere) {
		return OBBSphereIntersection((OBBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::OBB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return OBBSphereIntersection((OBBVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}


	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::Sphere) {
		return SphereCapsuleIntersection((CapsuleVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::Capsule) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return SphereCapsuleIntersection((CapsuleVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}


	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::AABB) {
		return AABBCapsuleIntersection((CapsuleVolume&)*volA, transformA, (AABBVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::AABB && volB->type == VolumeType::Capsule) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBCapsuleIntersection((CapsuleVolume&)*volB, transformB, (AABBVolume&)*volA, transformA, collisionInfo);
	}


	if (volA->type == VolumeType::OBB && volB->type == VolumeType::Capsule) {
		return OBBCapsuleIntersection((OBBVolume&)*volA, transformA, (CapsuleVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::OBB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return OBBCapsuleIntersection((OBBVolume&)*volB, transformB, (CapsuleVolume&)*volA, transformA, collisionInfo);
	}

	//if (volA->type == VolumeType::Sphere && volB->type == VolumeType::Invalid) {
	//	return PlaneSphereIntersection((SphereVolume&)*volA, (Plane&)*volB, transformA, collisionInfo);
	//}

	//if (volA->type == VolumeType::Invalid && volB->type == VolumeType::Sphere) {
	//	collisionInfo.a = b;
	//	collisionInfo.b = a;
	//	return PlaneSphereIntersection((SphereVolume&)*volA, (Plane&)*volB, transformA, collisionInfo);
	//}

	return false;
}


//Test if colliding
bool CollisionDetection::AABBTest(const Vector3& posA, const Vector3& posB, const Vector3& halfSizeA, const Vector3& halfSizeB) {

	Vector3 delta = posB - posA; //distance for each axis
	Vector3 totalSize = halfSizeA + halfSizeB; //sum of box size

	if (abs(delta.x) < totalSize.x &&  //if distance are less than box size
		abs(delta.y) < totalSize.y &&
		abs(delta.z) < totalSize.z) {
		return true;
	}

	return false;
}

//AABB/AABB Collisions
bool CollisionDetection::AABBIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxAPos = worldTransformA.GetPosition();
	Vector3 boxBPos = worldTransformB.GetPosition();

	Vector3 boxASize = volumeA.GetHalfDimensions();
	Vector3 boxBSize = volumeB.GetHalfDimensions();

	bool overlap = AABBTest(boxAPos, boxBPos, boxASize, boxBSize); //test to see if colliding

	if (overlap) {
		static const Vector3 faces[6] =
		{
			Vector3(-1, 0, 0), Vector3(1, 0, 0),
			Vector3(0, -1, 0), Vector3(0, 1, 0),
			Vector3(0, 0, -1), Vector3(0, 0, 1),
		};

		//min max pos of each axis
		Vector3 maxA = boxAPos + boxASize;
		Vector3 minA = boxAPos - boxASize;

		Vector3 maxB = boxBPos + boxBSize;
		Vector3 minB = boxBPos - boxBSize;

		//calculate amount of overlap of boxes
		float distances[6] =
		{
			(maxB.x - minA.x), // distance of box ’b’ to ’left ’ of ’a ’.
			(maxA.x - minB.x), // distance of box ’b’ to ’right ’ of ’a ’.
			(maxB.y - minA.y), // distance of box ’b’ to ’bottom ’ of ’a ’.
			(maxA.y - minB.y), // distance of box ’b’ to ’top ’ of ’a ’.
			(maxB.z - minA.z), // distance of box ’b’ to ’far ’ of ’a ’.
			(maxA.z - minB.z)  // distance of box ’b’ to ’near ’ of ’a ’.
		};

		//finding out least amount of penetration axis
		float penetration = FLT_MAX;
		Vector3 bestAxis;

		for (int i = 0; i < 6; i++) {
			if (distances[i] < penetration) {
				penetration = distances[i];
				bestAxis = faces[i];
			}
		}
		collisionInfo.AddContactPoint(Vector3(), Vector3(), bestAxis, penetration);
		return true;
	}

	return false;

}


//Sphere / Sphere Collision
bool CollisionDetection::SphereIntersection(const SphereVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	float radii = volumeA.GetRadius() + volumeB.GetRadius(); //sum of radius
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition(); //distance between 2 object pos

	float deltaLength = delta.Length();

	if (deltaLength < radii) {
		float penetration = (radii - deltaLength);
		Vector3 normal = delta.Normalised();

		//collision points
		Vector3 localA = normal * volumeA.GetRadius();
		Vector3 localB = -normal * volumeB.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, normal, penetration);
		return true; //we’re colliding !
	}

	return false;
}


//AABB - Sphere Collision
bool CollisionDetection::AABBSphereIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxSize = volumeA.GetHalfDimensions(); //box dimensions
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition(); //relative pos of sphere to box
	Vector3 closestPointOnBox = Maths::Clamp(delta, -boxSize, boxSize);
	Vector3 localPoint = delta - closestPointOnBox; //how far away sphere is to closest point on box
	float distance = localPoint.Length();

	if (distance < volumeB.GetRadius()) { //colliding
		Vector3 collisionNormal = localPoint.Normalised();
		float penetration = (volumeB.GetRadius() - distance);

		//collision points
		Vector3 localA = Vector3(); //colliding at relative position
		Vector3 localB = -collisionNormal * volumeB.GetRadius(); //minus as normals pointing towards B so need to read backwards

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetration);
		return true;
	}

	return false;
}

bool CollisionDetection::OBBIntersection(const OBBVolume& volumeA, const Transform& worldTransformA,
	const OBBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {


	return false;
}

//sphere - capsule
bool CollisionDetection::SphereCapsuleIntersection(const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	
	Vector3 spherePos = worldTransformB.GetPosition();
	Vector3 capsulePos = worldTransformA.GetPosition(); 
	Vector3 upPos = worldTransformA.GetOrientation() * Vector3(0, 1, 0);; 
	upPos = upPos.Normalised();

	Vector3 point = spherePos; 

	Vector3 topSpherePos = capsulePos + upPos * (volumeA.GetHalfHeight() - volumeA.GetRadius());
	Vector3 botSpherePos = capsulePos - upPos * (volumeA.GetHalfHeight() - volumeA.GetRadius());

	float distanceTop = (point - topSpherePos).Length(); //intersection point and top pos
	float distanceBot = (point - botSpherePos).Length(); //interesection point and bot pos

	Vector3 vATop = (topSpherePos - point);
	Vector3 vABot = (botSpherePos - point);
	Vector3 vBTop = topSpherePos - capsulePos;
	Vector3 vBBot = botSpherePos - capsulePos;
	float vAvBTop = Vector3::Dot(vATop, vBTop);
	float vAvBBot = Vector3::Dot(vABot, vBBot);

	Vector3 d = capsulePos + upPos * (Vector3::Dot(point - capsulePos, upPos));
	SphereVolume* vol = new SphereVolume(volumeA.GetRadius()); 


	if (vAvBTop < 0) { //interesection above cylinder section
		if (distanceBot > volumeA.GetRadius()) {
			capsulePos = topSpherePos;
		}
		else
			return false;
	}

	else if (vAvBBot < 0) { //intersection below cylinder section
		if (distanceTop > volumeA.GetRadius()) {
			capsulePos = botSpherePos;
		}
		else
			return false;
	}

	else if (d.Length() - point.Length() < volumeA.GetRadius()) { //inside cylinder
		capsulePos.y = point.y;
	}
	else {
		return false;
	}



	float radii = vol->GetRadius() + volumeB.GetRadius(); 
	Vector3 delta = worldTransformB.GetPosition() - capsulePos; 

	float deltaLength = delta.Length();

	if (deltaLength < radii) {
		float penetration = (radii - deltaLength);
		Vector3 normal = delta.Normalised();

		//collision points
		Vector3 localA = normal * vol->GetRadius();
		Vector3 localB = -normal * volumeB.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, normal, penetration);
		return true; 
	}

	return false;
}


//AABB - Capsule
bool CollisionDetection::AABBCapsuleIntersection(const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 AABBPos = worldTransformB.GetPosition();
	Vector3 capsulePos = worldTransformA.GetPosition();
	Vector3 upPos = worldTransformA.GetOrientation() * Vector3(0, 1, 0);
	upPos = upPos.Normalised();


	//getting full height size of capsule
	Vector3 Tip = capsulePos;
	Tip.y += volumeA.GetHalfHeight();
	Vector3 Base = capsulePos;
	Base.y -= volumeA.GetHalfHeight();

	float dist = (Tip - Base).Length();
	Plane* p = new Plane(upPos, dist);
	Vector3 point = p->ProjectPointOntoPlane(AABBPos); 


	//Vector3 point = AABBPos; //collision point p

	Vector3 topSpherePos = capsulePos + upPos * (volumeA.GetHalfHeight() - volumeA.GetRadius());
	Vector3 botSpherePos = capsulePos - upPos * (volumeA.GetHalfHeight() - volumeA.GetRadius());

	float distanceTop = (point - topSpherePos).Length(); //intersection point and top pos
	float distanceBot = (point - botSpherePos).Length(); //interesection point and bot pos

	Vector3 vATop = (topSpherePos - point);
	Vector3 vABot = (botSpherePos - point);
	Vector3 vBTop = topSpherePos - capsulePos;
	Vector3 vBBot = botSpherePos - capsulePos;
	float vAvBTop = Vector3::Dot(vATop, vBTop);
	float vAvBBot = Vector3::Dot(vABot, vBBot);

	Vector3 d = capsulePos + upPos * (Vector3::Dot(point - capsulePos, upPos));
	SphereVolume* vol = new SphereVolume(volumeA.GetRadius()); 


	if (vAvBTop < 0) { //interesection above cylinder section
		if (distanceBot > volumeA.GetRadius()) {
			upPos.y = topSpherePos.y;
		}
		else
			return false;
	}

	else if (vAvBBot < 0) { //intersection below cylinder section
		if (distanceTop > volumeA.GetRadius()) {
			upPos.y = botSpherePos.y;
		}
		else
			return false;
	}

	else if (d.Length()- point.Length()< volumeA.GetRadius() ) { //inside cylinder
		upPos.y = point.y;
	}
	else {
		return false;
	}


	//Transform* t = new Transform();
	//t->SetOrientation(worldTransformA.GetOrientation());
	//t->SetPosition(upPos * Vector3(0,1,0));
	//t->SetScale(worldTransformA.GetScale());

	//return AABBSphereIntersection(volumeB, worldTransformB, *vol, *t, collisionInfo);


	Vector3 boxSize = volumeB.GetHalfDimensions(); 
	Vector3 delta = upPos - AABBPos; 
	Vector3 closestPointOnBox = Maths::Clamp(delta, -boxSize, boxSize);
	Vector3 localPoint = delta - closestPointOnBox; 
	float distance = localPoint.Length();

	if (distance < volumeA.GetRadius()) { //colliding
		Vector3 collisionNormal = localPoint.Normalised();
		float penetration = (volumeA.GetRadius() - distance);

		//collision points
		Vector3 localA = collisionNormal; //colliding at relative position
		Vector3 localB = -collisionNormal * volumeA.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetration);
		return true;
	}

	return false;

}

//OBB - Capsule
bool CollisionDetection::OBBCapsuleIntersection(const OBBVolume& volumeA, const Transform& worldTransformA,
	const CapsuleVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	AABBVolume* aabbVol = new AABBVolume(volumeA.GetHalfDimensions());

	Quaternion orientation = worldTransformA.GetOrientation();
	Transform* transA = new Transform();
	Transform* transB = new Transform();
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();

	transB->SetPosition(worldTransformA.GetOrientation().Conjugate() * delta); //setting to normal world pos

	if (AABBCapsuleIntersection(volumeB, *transB, *aabbVol, *transA,  collisionInfo)) { //colliding

		collisionInfo.point.normal = orientation * collisionInfo.point.normal;
		collisionInfo.point.localA = orientation * collisionInfo.point.localA;
		collisionInfo.point.localB = orientation * collisionInfo.point.localB;

		return true;
	}

	return false;

}

//Capsule - Capsule
bool CollisionDetection::CapsuleIntersection( CapsuleVolume& volumeA,  Transform& worldTransformA,
	 CapsuleVolume& volumeB,  Transform& worldTransformB, CollisionInfo& collisionInfo) {

	// capsule A
	Vector3 aPos = worldTransformA.GetPosition();
	Vector3 aTip = aPos;
	aTip.y += volumeA.GetHalfHeight();
	Vector3 aBase = aPos;
	aBase.y -= volumeA.GetHalfHeight();

	Vector3 aNormal = (aTip-aBase).Normalised();
	Vector3 aSegLineOffset = aNormal * (volumeA.GetRadius()); //capsule's segment (A-B line) 
	Vector3 aA = aBase - aSegLineOffset; //bottomSpherePos
	Vector3 aB = aTip + aSegLineOffset; //topSpherePos


	//capsule B
	Vector3 bPos = worldTransformB.GetPosition();
	Vector3 bTip = bPos;
	bTip.y += volumeB.GetHalfHeight();
	Vector3 bBase = bPos;
	bBase.y -= volumeB.GetHalfHeight();

	Vector3 bNormal = (bTip - bBase).Normalised();
	Vector3 bSegLineOffSet = bNormal * (volumeB.GetRadius());
	Vector3 bA = bBase - bSegLineOffSet; //bottomSpherePos
	Vector3 bB = bTip + bSegLineOffSet; //topSpherePos


	//vectors between top and bottom of capsule spheres 
	Vector3 v0 = aA-bA;
	Vector3 v1 = aB-bA;
	Vector3 v2 = aA -bB;
	Vector3 v3 = aB -bB;

	//distance squared
	float d0 = Vector3::Dot(v0, v0);
	float d1 = Vector3::Dot(v1, v1);
	float d2 = Vector3::Dot(v2, v2);
	float d3 = Vector3::Dot(v3, v3);

	//best capsule A distance along segment line
	Vector3 bestA;
	if (d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1) {
		bestA = aA;
	}
	else {
		bestA = aB;
	}

	//point on capsule B segment nearest to best point on A
	Vector3 ABPointB = bA - bB;
	float tB = Vector3::Dot((bestA- bA), ABPointB) / Vector3::Dot(ABPointB, ABPointB);
	float minB = fmin((fmax(tB,0)), 1);
	Vector3 bestB =  ABPointB * minB + bA;


	//capsule A segment:
	Vector3 ABPointA = aA - aB;
	float tA = Vector3::Dot((bestB- aA), ABPointA) / Vector3::Dot(ABPointA, ABPointA);
	float minA = fmin(fmax(tA, 0), 1);
	bestA = ABPointA * minA + aA;


	Vector3 normal = bestA - bestB;
	float len = normal.Length();
	normal = normal /len;
	float distance = volumeA.GetRadius() + volumeB.GetRadius() - len;

	//collisionInfo.AddContactPoint(bestA, bestB, normal, distance);

	if (distance > 0) {
		SphereVolume* volA = new SphereVolume(volumeA.GetRadius());
		SphereVolume* volB = new SphereVolume(volumeB.GetRadius());
		Transform* transA = new Transform();
		transA->SetOrientation(worldTransformA.GetOrientation());
		transA->SetPosition(bestA);
		transA->SetScale(worldTransformA.GetScale());

		Transform* transB = new Transform();
		transB->SetOrientation(worldTransformB.GetOrientation());
		transB->SetPosition(bestB);
		transB->SetScale(worldTransformB.GetScale());

		return SphereIntersection(*volA, *transA, *volB, *transB, collisionInfo);
	}
	return false;

}


//OBB-Sphere
bool CollisionDetection::OBBSphereIntersection(const OBBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	AABBVolume* aabbVol = new AABBVolume(volumeA.GetHalfDimensions());

	Quaternion orientation = worldTransformA.GetOrientation();
	Transform* transA = new Transform();
	Transform* transB = new Transform();
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();

	transB->SetPosition(worldTransformA.GetOrientation().Conjugate() * delta); //setting to normal world pos

	if (AABBSphereIntersection(*aabbVol, *transA, volumeB, *transB, collisionInfo)) { //colliding

		collisionInfo.point.normal = orientation * collisionInfo.point.normal;
		collisionInfo.point.localA = orientation * collisionInfo.point.localA;
		collisionInfo.point.localB = orientation * collisionInfo.point.localB;

		return true; 
	}

	return false; 

}