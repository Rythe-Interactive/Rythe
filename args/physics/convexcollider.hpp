#pragma once

#include <physics/physicscollider.hpp>
#include <physics/cubecolliderparams.hpp>

namespace args::physics
{
	class ConvexCollider : public PhysicsCollider
	{
	public:

		ConvexCollider(physicsComponent* colliderOwner);

		virtual void CheckCollision(std::shared_ptr<PhysicsCollider> physicsComponent, physics_manifold* manifold) override;

		virtual void CheckCollisionWith(ConvexCollider* convexCollider, physics_manifold* manifold) override;

		/**@brief Constructs a box-shaped convex hull that encompasses the given mesh
		 * @param mesh : the mesh that the convex hull will encompass
		*/
		void ConstructBoxWithMesh();

		/**@brief Constructs a box-shaped convex hull based on the given parameters
		 * @param cubeParams : the parameters that will be used to create the box.
		*/
		void CreateBox(const cube_collider_params& cubeParams);

	private:

	};
}


