#pragma once

#include <array>
#include <iostream>
#include <cstring>

#include "Point3D.h"
#include "Point.h"

namespace Gorgon :: Geometry {
	template<class T_>
	class basic_Transform3D {
	public:
		basic_Transform3D() {
			LoadIdentity();
		}

		basic_Transform3D(std::initializer_list<std::array<T_, 4>> init) {
			int i=0;
			for(auto r : init) {
				for(int j=0; j<4; j++) {
					mat[i][j] = r[j];
				}
				i+=1;
				if(i==4) break;
			}
		}
		
		void LoadIdentity() {
			std::memset(&mat[0], 0, 4*4*sizeof(T_));

			for(int i=0; i<4; i++) {
				mat[i][i] = 1;
			}
		}

		T_ Get(int row, int col) const { return mat[row][col]; }

		T_ &operator()(int row, int col) { return mat[row][col]; }

		basic_Point3D<T_> operator *(const basic_Point3D<T_> &p) const {
			auto f = p.X * mat[3][0] + p.Y * mat[3][1] + p.Z * mat[3][2] + mat[3][3];

			return{
				(p.X * mat[0][0] + p.Y * mat[0][1] + p.Z * mat[0][2] + mat[0][3]) / f,
				(p.X * mat[1][0] + p.Y * mat[1][1] + p.Z * mat[1][2] + mat[1][3]) / f,
				(p.X * mat[2][0] + p.Y * mat[2][1] + p.Z * mat[2][2] + mat[2][3]) / f
			};
		}

		basic_Point<T_> operator *(const basic_Point<T_> &p) const {
			auto f = p.X * mat[3][0] + p.Y * mat[3][1] + mat[3][3];

			return{
				(p.X * mat[0][0] + p.Y * mat[0][1] + mat[0][3]) / f,
				(p.X * mat[1][0] + p.Y * mat[1][1] + mat[1][3]) / f
			};
		}

		basic_Transform3D &operator *=(const basic_Transform3D &t) {
			basic_Transform3D o = *this;
			for(int i=0; i<4; i++)
				for(int j=0; j<4; j++)
					mat[i][j] = o.mat[i][0] * t.mat[0][j] + o.mat[i][1] * t.mat[1][j] + o.mat[i][2] * t.mat[2][j] + o.mat[i][3] * t.mat[3][j];

			return *this;
		}

		void Translate(const basic_Point3D<T_> &p) {
			basic_Transform3D t({
				{1, 0, 0, p.X},
				{0, 1, 0, p.Y},
				{0, 0, 1, p.Z},
				{0, 0, 0, 1}
			});

			(*this)*=t;
		}

		void Translate(T_ x, T_ y, T_ z = 0) {
			basic_Transform3D t({
				{1, 0, 0, x},
				{0, 1, 0, y},
				{0, 0, 1, z},
				{0, 0, 0, 1}
			});

			(*this)*=t;
		}

		void Scale(T_ x, T_ y, T_ z = 1) {
			basic_Transform3D t({
				{x, 0, 0, 0},
				{0, y, 0, 0},
				{0, 0, z, 0},
				{0, 0, 0, 1}
			});

			(*this)*=t;
		}

		void Scale(T_ f) {
			basic_Transform3D t({
				{f, 0, 0, 0},
				{0, f, 0, 0},
				{0, 0, f, 0},
				{0, 0, 0, 1}
			});

			(*this)*=t;
		}

		void Rotate(T_ x, T_ y, T_ z) {
			T_ ca = std::cos(z), sa = std::sin(z);
			T_ cb = std::cos(y), sb = std::sin(y);
			T_ cc = std::cos(x), sc = std::sin(x);

			basic_Transform3D t({
				{ca*cb, ca*sb*sc - sa*cc, ca*sb*cc + sa*sc, 0},
				{sa*cb, sa*cb*sc + ca*cc, sa*sb*cc - ca*sc, 0},
				{-sb  , cb*sc           , cb*cc           , 0},
				{0    , 0               , 0               , 1}
			});

			(*this)*=t;
		}

		/// Rotates around the given plane, vec should be a unit vector
		void Rotate(const basic_Point3D<T_> &vec, T_ ang) {
			T_ c = std::cos(ang), s = std::sin(ang);
			T_ C = 1 - c;

			basic_Transform3D t({
				{vec.X*vec.X + (vec.Y*vec.Y + vec.Z*vec.Z)*c, vec.X*vec.Y*C - vec.Z*s                    , vec.Z*vec.X*C + vec.Y*s                    , 0},
				{vec.X*vec.Y*C + vec.Z*s                    , vec.Y*vec.Y + (vec.X*vec.X + vec.Z*vec.Z)*c, vec.Z*vec.Y*C + vec.X*s                    , 0},
				{vec.X*vec.Z*C - vec.Y*s                    , vec.Z*vec.Y*C + vec.X*s                    , vec.Z*vec.Z + (vec.X*vec.X + vec.Y*vec.Y)*c, 0},
				{0                                          , 0                                          , 0                                          , 1}
			});

			(*this)*=t;
		}
		
		/// This function transposes only 3x3 portion of the matrix
		void Transpose() {
            *this = {
                {mat[0][0], mat[1][0], mat[2][0], mat[0][3]},
				{mat[0][1], mat[1][1], mat[2][1], mat[1][3]},
				{mat[0][2], mat[1][2], mat[2][2], mat[2][3]},
				{mat[3][0], mat[3][1], mat[3][2], mat[3][3]}
            };
        }
        
        T_ *Data() {
            return vec;
        }

	private:
        union {
            T_ mat[4][4];
            T_ vec[16] = {};
        };
	};

	template<class T_>
	std::ostream &operator <<(std::ostream &out, const basic_Transform3D<T_> &transform) {
		for(int i=0; i<4; i++) {
			for(int j=0; j<4; j++) {
				out<<transform.Get(i, j)<<"\t";
			}
			out<<std::endl;
		}

		return out;
	}

	using Transform3D = basic_Transform3D<Float>;
}
