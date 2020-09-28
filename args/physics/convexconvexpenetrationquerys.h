#pragma once
#include <physics/data/penetrationquerys.h>
#include <physics/halfedgeedge.hpp>

namespace args::physics
{
	class ConvexConvexPenetrationQuery : public PenetrationQuery
	{
	public:

		HalfEdgeFace* refFace = nullptr;
		HalfEdgeFace* incFace = nullptr;
		//ConvexCollider*

		ConvexConvexPenetrationQuery(HalfEdgeFace* pRefFace, HalfEdgeFace* pIncFace, math::vec3& pFaceCentroid, math::vec3& pNormal, float& pPenetration, bool pIsARef);

		virtual void populateContactList(physics_manifold& manifold) override;

	};
}


