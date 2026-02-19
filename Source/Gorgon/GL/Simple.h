#pragma once

#include "../Geometry/Point3D.h"
#include "../Geometry/Transform3D.h"
#include "../Geometry/Point.h"


namespace Gorgon :: GL {

	class QuadVertices {
	public:
		QuadVertices() { }

		QuadVertices(Geometry::Point3D p1, Geometry::Point3D p2, Geometry::Point3D p3, Geometry::Point3D p4) {
			Vertices[0] = p1;
			Vertices[1] = p2;
			Vertices[2] = p3;
			Vertices[3] = p4;
		}

		Geometry::Point3D &operator[] (int ind) {
			return Vertices[ind];
		}

		const Geometry::Point3D &operator[] (int ind) const {
			return Vertices[ind];
		}

		union {
			Geometry::Point3D Vertices[4];
			float Data[12];
		};
	};

	inline QuadVertices operator *(const Geometry::Transform3D &transform, const QuadVertices &quad) {
		QuadVertices ret;

		for(int i=0; i<4; i++)
			ret[i] = transform * quad[i];

		return ret;
	}

	class QuadTextureCoords {
	public:
		QuadTextureCoords() { };

		QuadTextureCoords(Geometry::Pointf p1, Geometry::Pointf p2, Geometry::Pointf p3, Geometry::Pointf p4) {
			Coords[0] = p1;
			Coords[1] = p2;
			Coords[2] = p3;
			Coords[3] = p4;
		}

		Geometry::Pointf &operator[] (int ind) {
			return Coords[ind];
		}

		const Geometry::Pointf &operator[] (int ind) const {
			return Coords[ind];
		}

		void FlipY() {
			Coords[0].Y = 1-Coords[0].Y;
			Coords[1].Y = 1-Coords[1].Y;
			Coords[2].Y = 1-Coords[2].Y;
			Coords[3].Y = 1-Coords[3].Y;
		}

		union {
			Geometry::Pointf Coords[4];
			float Data[8];
		};
	};

}