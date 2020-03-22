// this was rewritten from some Intel code
// http://www.gamasutra.com/features/20000131/barad_01.htm
//
// Intel copyright :
//
//   Copyright (c) 2001 Intel Corporation.
//
// Permission is granted to use, copy, distribute and prepare 
// derivative works of this library for any purpose and without fee,
// provided, that the above copyright notice and this statement 
// appear in all copies. Intel makes no representations about the
// suitability of this library for any purpose, and specifically
// disclaims all warranties. See LEGAL.TXT for all the legal
// information.

#pragma once

// Define the vectorization of intrinsic.
#if defined(_WIN32) | defined(_WIN64)
#define ENABLE_VEC 1
#endif
#ifdef ENABLE_VEC
#include <fvec.h>
#endif // ENABLE_VEC
#include <assert.h>
#include <string.h>
#include <vector>

#define _USE_MATH_DEFINES
#include <math.h>


#ifndef WIN32
// for if not defined (Sparc for instance)
#ifndef fabsf
#define fabsf fabs
#endif

#ifndef sqrtf
#define sqrtf sqrt
#endif

#ifndef cosf
#define cosf cos
#endif

#ifndef sinf
#define sinf sin
#endif

#ifndef tanf
#define tanf tan
#endif

#ifndef cotf
#define cotf cot
#endif

#ifndef asinf
#define asinf asin
#endif

#ifndef acosf
#define acosf acos
#endif

#ifndef atan2f
#define atan2f atan2
#endif

#ifndef fmodf
#define fmodf fmod
#endif
#endif // WIN32

namespace sgl {

	constexpr float epsilon = 1e-5f;
	constexpr float huge = 3.402823466E+38f;

#ifdef ENABLE_VEC
#	ifdef WIN32
#		pragma pack(push,16) // 16 bit aligned
#	endif // WIN32
#else
	// for compatibility with Intel structures
	typedef struct __m128 {
		float       m128_f32[4];
	} __m128;
#endif // ENABLE_VEC

	class matrix;
	class vector;
	class vector2;
	class vector3;

	class matrix {
	public:
		union {
			struct {
				__m128 _L1, _L2, _L3, _L4;
			};
			struct {
				float   _11, _12, _13, _14;
				float   _21, _22, _23, _24;
				float   _31, _32, _33, _34;
				float   _41, _42, _43, _44;
			};
		};

		// Constructors and conversions:

		matrix() : matrix(
			1, 0, 0, 0, 
			0, 1, 0, 0, 
			0, 0, 1, 0,
			0, 0, 0, 1) {}
		matrix(const matrix& m) : 
			_L1(m._L1), _L2(m._L2), _L3(m._L3), _L4(m._L4) {}
		matrix(
			float _11, float _12, float _13, float _14,
			float _21, float _22, float _23, float _24,
			float _31, float _32, float _33, float _34,
			float _41, float _42, float _43, float _44);
		matrix(const float* arr);

		matrix& operator = (const matrix& a) {
			_L1 = a._L1; _L2 = a._L2; _L3 = a._L3; _L4 = a._L4;
			return *this;
		}

		// Accessing elements:

		float& operator () (int i, int j) {
			assert((0 <= i) && (i <= 3) && (0 <= j) && (j <= 3));
#pragma warning(disable : 26451 6297)
			return *(((float*)&_11) + (i << 2) + j);
		}
#ifdef ENABLE_VEC
		F32vec4& operator () (int i) {
			assert((0<=i) && (i<=3));
			return *(((F32vec4 *)&_11) + i);
		}
		F32vec4& operator [] (int i) {
			assert((0<=i) && (i<=3));
			return *(((F32vec4 *)&_11) + i);
		}
		F32vec4& operator [] (int i) const {
			assert((0<=i) && (i<=3));
			return *(((F32vec4 *)&_11) + i);
		}
#endif // ENABLE_VEC

		// Operators and member functions:

		friend matrix operator * (const matrix&, const matrix&);
		friend matrix operator + (const matrix&, const matrix&);
		friend matrix operator - (const matrix&, const matrix&);
		friend matrix operator + (const matrix&);
		friend matrix operator - (const matrix&);
		friend matrix operator * (const matrix&, const float);
		friend matrix operator * (const float, const matrix&);

		matrix& operator *= (const matrix&);
		matrix& operator *= (const float);
		matrix& operator += (const matrix&);
		matrix& operator -= (const matrix&);

		// Transposes the matrix
		void Transpose();
		// Inverses the matrix and returns the determinant
		float Inverse();

		// Returns the determinant
		float Determinant();
		// Returns the minimum absolute value of the matrix
		float MinValue();
		// Returns the maximum absolute value of the matrix
		float MaxValue();

		// Other Constructors:

		void ZeroMatrix();
		void IdentityMatrix();
		void TranslateMatrix(
			const float dx, const float dy, const float dz);
		void ScaleMatrix(
			const float a, const float b, const float c);
		void ScaleMatrix(const float a);
		void RotateXMatrix(const float rads);
		void RotateYMatrix(const float rads);
		void RotateZMatrix(const float rads);
	};

#define matrix4 matrix

	// ----------------------------------------------------------
	//  Name:   vector, vector4
	//  Desc:   A vector of 4 floats.
	// ----------------------------------------------------------
	class vector {
	public:
		union {
			__m128 vec;
			struct { 
				float   x,y,z,w;
			};
		};

		// Constructors and conversions:

		vector() : vector(0, 0, 0, 1) {}
		vector(const vector& v) : vec(v.vec) {}
		explicit vector(const __m128 &m) : vec(m) {}
#ifdef ENABLE_VEC
		explicit vector(const F32vec4 &m) : vec(m) {}
#endif // ENABLE_VEC
		explicit vector(const float a)
#ifdef ENABLE_VEC
			: vec(F32vec4(a)) {}
#else
			: x(a), y(a), z(a), w(a) {}
#endif // ENABLE_VEC
		vector(
			const float x, 
			const float y, 
			const float z, const float w=1.0f) 
#ifdef ENABLE_VEC
			: vec(F32vec4(w,z,y,x)) {}
#else
			: x(x), y(y), z(z), w(w) {}
#endif // ENABLE_VEC
		explicit vector(const float *arr) 
#ifdef ENABLE_VEC
			: vec(_mm_loadu_ps(arr)) {}
#else
			: x(arr[0]), y(arr[1]), z(arr[2]), w(arr[3]) {} 
#endif // ENABLE_VEC
		operator __m128() const { return vec; }
#ifdef ENABLE_VEC
		operator F32vec4() const { return vec; }
#endif // ENABLE_VEC

		vector& operator = (const vector& a) 
			{ vec = a.vec; return *this; }
		vector& operator = (const vector3&);
#ifdef ENABLE_VEC
		vector& operator = (const F32vec4 &a) 
			{ vec = a; return *this; }
#endif // ENABLE_VEC
		vector& operator = (const __m128 &a) 
			{ vec = a; return *this; }

		// Accessing elements

		float& operator () (int i) {
			assert((0<=i) && (i<=3));
			return *(((float *)&vec) + i);
		}
		float& operator [] (int i) {
			assert((0<=i) && (i<=3));
			return *(((float *)&vec) + i);
		}
		float& operator [] (int i) const {
			assert((0<=i) && (i<=3));
			return *(((float *)&vec) + i);
		}

		// Operators and member functions:

		friend vector operator * (const vector&, const matrix&);
		// Dot Product
		friend float operator * (const vector&, const vector&);
		// Cross Product
		friend vector operator % (const vector&, const vector&);
		// Elements Product
		friend vector operator | (const vector&, const vector&);
		friend vector operator * (const vector&, const float);
		friend vector operator * (const float, const vector&);
		friend vector operator + (const vector&);
		friend vector operator + (const vector&, const vector&);
		friend vector operator + (const vector&, const float);
		friend vector operator + (const float, const vector&);
		friend vector operator - (const vector&);
		friend vector operator - (const vector&, const vector&);
		// Normalize
		friend vector operator ~ (const vector&);

		vector& operator *= (const matrix&);
		vector& operator *= (const float);
		vector& operator |= (const vector&);
		vector& operator += (const vector&);
		vector& operator += (const float);
		vector& operator -= (const vector&);

		float Length() const;
		float LengthSquared() const;
		vector& Normalize();
	};

#define vector4 vector

	// ----------------------------------------------------------
	//  Name:   vector3
	//  Desc:   A vector of 3 floats.
	// ----------------------------------------------------------
	class vector3 {
	public:
		union {
			__m128 vec;
			struct { 
				float   x,y,z;
				//  float   _spacer_;
			};
		};

		// Constructors and conversions:

		vector3() : vector3(0, 0, 0) {}
		vector3(const vector3& v) : vec(v.vec) {}
		explicit vector3(const __m128 &m) : vec(m) {}
#ifdef ENABLE_VEC
		explicit vector3(const F32vec4 &m) : vec(m) {}
#endif // ENABLE_VEC
		explicit vector3(const float a) 
#ifdef ENABLE_VEC
			: vec(F32vec4(a)) {}
#else
			: x(a), y(a), z(a) {}
#endif // ENABLE_VEC
		vector3(const float x, const float y, const float z) 
#ifdef ENABLE_VEC
			: vec(F32vec4(0.0f,z,y,x)) {}
#else
			: x(x), y(y), z(z) {}
#endif // ENABLE_VEC
		explicit vector3(const float *arr) 
#ifdef ENABLE_VEC
			: vec(_mm_loadl_pi(_mm_movelh_ps(
				_mm_load_ss(arr+2),
				_mm_load_ss(arr+2)), 
				(__m64*)arr)) {}
#else
			: x(arr[0]), y(arr[1]), z(arr[2]) {}
#endif // ENABLE_VEC
		operator __m128() const { return vec; }
#ifdef ENABLE_VEC
		operator F32vec4() const { return vec; }
#endif // ENABLE_VEC

		vector3& operator=(const vector3& a) 
			{ vec = a.vec; return *this; }
		vector3& operator=(const vector& a);
#ifdef ENABLE_VEC
		vector3& operator=(const F32vec4 &a) 
			{ vec = a; return *this; }
#endif // ENABLE_VEC
		vector3& operator=(const __m128 &a) 
			{ vec = a; return *this; }

		// Accessing elements:

		float& operator () (int i) {
			assert((0<=i) && (i<=2));
			return *(((float *)&vec) + i);
		}
		float& operator [] (int i) {
			assert((0<=i) && (i<=2));
			return *(((float *)&vec) + i);
		}
		float& operator [] (int i) const {
			assert((0<=i) && (i<=2));
			return *(((float *)&vec) + i);
		}

		// Operators and member functions:

		friend vector operator * (const vector3&, const matrix&);
		// Dot Product
		friend float operator * (const vector3&, const vector3&);
		// Cross Product
		friend vector3 operator % (const vector3&, const vector3&);
		friend vector3 operator * (const vector3&, const float);
		friend vector3 operator * (const float, const vector3&);
		// Elements Product
		friend vector3 operator | (const vector3&, const vector3&);
		friend vector3 operator + (const vector3&);
		friend vector3 operator + (const vector3&, const vector3&);
		friend vector3 operator - (const vector3&);
		friend vector3 operator - (const vector3&, const vector3&);
		// Normalize
		friend vector3 operator ~ (const vector3&);

		vector3& operator *= (const matrix&);
		vector3& operator *= (const float);
		vector3& operator |= (const vector3&);
		vector3& operator += (const vector3&);
		vector3& operator -= (const vector3&);

		float Length();
		vector3& Normalize();
	};

	// ----------------------------------------------------------
	//  Name:   vector2
	//  Desc:   A vector of 2 floats. (very simple)
	// ----------------------------------------------------------
	class vector2 {
	public:
		union {
			__m128 vec;
			struct { 
				float   x,y;
				//  float   _spacer_[2];
			};
		};

		// Constructors and conversions:

		vector2() : vector2(0, 0) {}
		vector2(const vector3& v) : vec(v.vec) {}
		explicit vector2(const __m128 &m) : vec(m) {}
#ifdef ENABLE_VEC
		explicit vector2(const F32vec4 &m) : vec(m) {}
#endif // ENABLE_VEC
		explicit vector2(const float a) 
#ifdef ENABLE_VEC
			: vec(F32vec4(a)) {}
#else
			: x(a), y(a) {}
#endif // ENABLE_VEC
		vector2(const float x, const float y) 
#ifdef ENABLE_VEC
			: vec(F32vec4(0.0f,0.0f,y,x)) {}
#else
			: x(x), y(y) {}
#endif
		vector2(const float *arr) 
#ifdef ENABLE_VEC
			: vec(_mm_loadl_pi(_mm_movelh_ps(
				_mm_load_ss(arr+2),
				_mm_load_ss(arr+2)), 
				(__m64*)arr)) {}
#else
			: x(arr[0]), y(arr[1]) {}
#endif // ENABLE_VEC
		operator __m128() const { return vec; }
#ifdef ENABLE_VEC
		operator F32vec4() const { return vec; }
#endif // ENABLE_VEC

		vector2& operator = (const vector2& a) 
			{ vec = a.vec; return *this; }
		vector2& operator = (const vector& a);
#ifdef ENABLE_VEC
		vector2& operator = (const F32vec4 &a) 
			{ vec = a; return *this; }
#endif // ENABLE_VEC
		vector2& operator = (const __m128 &a) 
			{ vec = a; return *this; }

		// Accessing elements:

		float& operator () (int i) {
			assert((0<=i) && (i<=1));
			return *(((float *)&vec) + i);
		}
		float& operator [] (int i) {
			assert((0<=i) && (i<=1));
			return *(((float *)&vec) + i);
		}
		float& operator [] (int i) const {
			assert((0<=i) && (i<=1));
			return *(((float *)&vec) + i);
		}

		// Operators and member functions:

		friend vector2 operator * (const vector2&, const float);
		friend vector2 operator * (const float, const vector2&);
		// Elements Product
		friend vector2 operator | (const vector2&, const vector2&);
		friend vector2 operator + (const vector2&);
		friend vector2 operator + (const vector2&, const vector2&);
		friend vector2 operator - (const vector2&);
		friend vector2 operator - (const vector2&, const vector2&);

		vector2& operator *= (const float);
		vector2& operator |= (const vector2&);
		vector2& operator += (const vector2&);
		vector2& operator -= (const vector2&);
	};

#ifdef ENABLE_VEC
#pragma pack(pop)
#endif // ENABLE_VEC

	// Other functions:

	void MatrixMult(
		const matrix& A, const matrix& B, matrix& Res);
	void VectorMult(
		const vector& Vec, const matrix& Mat, vector& Res);
	matrix MatrixMult(
		const matrix& A, const matrix& B);

	inline matrix MatrixInverse(matrix& m)      
		{ matrix l=m; l.Inverse(); return l; }
	inline float Determinant(matrix& m)           
		{ return m.Determinant(); }
	inline matrix MatrixTranspose(matrix& m)    
		{ matrix l=m; l.Transpose(); return l; }

	// some stuff for deformation
	matrix AA2M3(const vector3& v, const float a);
	inline vector3 transform_loc(
		const matrix4& m, const vector3& v) 
		{ vector3 r; r = v * m; return r; }
	inline vector3 transform_vec(
		const matrix4& m, const vector3& v) 
	{ 
		matrix4 m2(m); 
		vector4 v2(0.0f); 
		m2._L4 = v2.vec;	
		vector3 r; 
		r = v * m2; 
		return r;	
	}

	inline matrix Perspective(
		const float fovy,
		const float aspect,
		const float z_near,
		const float z_far)
	{
		assert(aspect != 0.0f);
		assert(z_far != z_near);
		assert(aspect <= 2 * M_PI);
		float tan_half_fovy = tanf(fovy / 2.0f);
		matrix result;
		result.ZeroMatrix();
		result(0, 0) = 1.f / (aspect * tan_half_fovy);
		result(1, 1) = 1.f / (tan_half_fovy);
		result(2, 2) = -(z_far + z_near) / (z_far - z_near);
		result(2, 3) = -1.f;
		result(3, 2) = -(2.f * z_far * z_near) / (z_far - z_near);
		return result;
	}

	inline matrix Projection(
		const float fovy, 
		const float aspect, 
		const float z_near, 
		const float z_far)
	{
		assert(aspect != 0.0f);
		assert(z_far != z_near);
		assert(aspect <= 2 * M_PI);
		float tan_half_fovy = tanf(fovy / 2.0f);
		matrix result;
		result.ZeroMatrix();
		result(0, 0) = 1.f / (aspect * tan_half_fovy);
		result(1, 1) = 1.f / (tan_half_fovy);
		result(2, 2) = z_far / (z_far - z_near);
		result(2, 3) = 1.f;
		result(3, 2) = -(z_far * z_near) / (z_far - z_near);
		return result;
	}

#ifdef ENABLE_VEC
	// Some useful macros:

	extern const __int32 __MASKSIGNs_[4];
#define _MASKSIGNs_ (*(F32vec4*)&__MASKSIGNs_)

#define _mm_ror_ps(vec,i)\
	(((i)%4) ?\
		(_mm_shuffle_ps(vec,vec,\
			_MM_SHUFFLE((unsigned char)(i+3)%4,\
			(unsigned char)(i+2)%4,\
			(unsigned char)(i+1)%4,\
			(unsigned char)(i+0)%4))) :\
		(vec))
#define _mm_rol_ps(vec,i)\
	(((i)%4) ?\
		(_mm_shuffle_ps(vec,vec,\
			_MM_SHUFFLE((unsigned char)(7-i)%4,\
			(unsigned char)(6-i)%4,\
			(unsigned char)(5-i)%4,\
			(unsigned char)(4-i)%4))) :\
		(vec))
#define _mm_abs_ps(vec)\
	_mm_andnot_ps(_MASKSIGNs_,vec)
#define _mm_neg_ps(vec)\
	_mm_xor_ps(_MASKSIGNs_,vec)
#endif // ENABLE_VEC

	// inlined functions
#ifdef ENABLE_VEC
	extern const float __1000_[4];
	extern const __int32 __0FFF_[4];
	extern const float __ZERONE_[4];
#define _0FFF_      (*(F32vec4*)&__0FFF_)           // 0 * * *
#define _ZERONE_    (*(F32vec4*)&__ZERONE_)         // 1 0 0 1
#define _1000_      (*(F32vec4*)&__1000_)           // 1 0 0 0
#endif // ENABLE_VEC

	/***********************************************************/
	/*                Matrix Constructors                      */
	/***********************************************************/

	inline matrix::matrix(  
		float f11, float f12, float f13, float f14,
		float f21, float f22, float f23, float f24,
		float f31, float f32, float f33, float f34,
		float f41, float f42, float f43, float f44  )
	{
#ifdef ENABLE_VEC
		_L1 = F32vec4(f14, f13, f12, f11);
		_L2 = F32vec4(f24, f23, f22, f21);
		_L3 = F32vec4(f34, f33, f32, f31);
		_L4 = F32vec4(f44, f43, f42, f41);
#else
		_11 = f11;
		_12 = f12;
		_13 = f13;
		_14 = f14;

		_21 = f21;
		_22 = f22;
		_23 = f23;
		_24 = f24;

		_31 = f31;
		_32 = f32;
		_33 = f33;
		_34 = f34;

		_41 = f41;
		_42 = f42;
		_43 = f43;
		_44 = f44;
#endif // ENABLE_VEC
	}

	inline matrix::matrix(const float *arr) 
	{
#ifdef ENABLE_VEC
		_L1 = _mm_loadu_ps(arr);
		_L2 = _mm_loadu_ps(arr+4);
		_L3 = _mm_loadu_ps(arr+8);
		_L4 = _mm_loadu_ps(arr+12);
#else
		memcpy(&_L1, arr, sizeof(__m128));
		memcpy(&_L2, arr+4, sizeof(__m128));
		memcpy(&_L3, arr+8, sizeof(__m128));
		memcpy(&_L4, arr+12, sizeof(__m128));
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   ZeroMatrix
	//  Desc:   Sets all elements in [Res] to zero.
	// ----------------------------------------------------------
	inline void matrix::ZeroMatrix() {
#ifdef ENABLE_VEC
		_L1 = _L2 = _L3 = _L4 = _mm_setzero_ps();
#else
		memset(&_L1, 0, sizeof(__m128));
		memset(&_L2, 0, sizeof(__m128));
		memset(&_L3, 0, sizeof(__m128));
		memset(&_L4, 0, sizeof(__m128));
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   IdentityMatrix
	//  Desc:   Returns the Identity Matrix.
	// ----------------------------------------------------------
	inline void matrix::IdentityMatrix() {
#ifdef ENABLE_VEC
		__m128 zerone = _ZERONE_;
		_mm_storel_pi((__m64 *)&_11, zerone);
		_mm_storel_pi((__m64 *)&_13, _mm_setzero_ps());
		_mm_storeh_pi((__m64 *)&_21, zerone);
		_mm_storel_pi((__m64 *)&_23, _mm_setzero_ps());
		_mm_storel_pi((__m64 *)&_31, _mm_setzero_ps());
		_mm_storel_pi((__m64 *)&_33, zerone);
		_mm_storel_pi((__m64 *)&_41, _mm_setzero_ps());
		_mm_storeh_pi((__m64 *)&_43, zerone);
#else
		ZeroMatrix();
		_11 = _22 = _33 = _44 = 1.0f;
#endif // ENABLE_VEC
	}


	/***********************************************************/
	/*                     Matrix Operators                    */
	/***********************************************************/

	// ----------------------------------------------------------
	//  Name:   MatrixMult
	//  Desc:   Matrix multiplication of A and B. Returns[A]*[B].
	// ----------------------------------------------------------
	inline matrix MatrixMult(const matrix& A, const matrix& B) {
		matrix res;
#ifdef ENABLE_VEC
		F32vec4 r1, r2;
		F32vec4 B1 = B._L1, B2 = B._L2, B3 = B._L3, B4 = B._L4;

		r1 = F32vec4(A._11) * B1;
		r2 = F32vec4(A._21) * B1;
		r1 += F32vec4(A._12) * B2;
		r2 += F32vec4(A._22) * B2;
		r1 += F32vec4(A._13) * B3;
		r2 += F32vec4(A._23) * B3;
		r1 += F32vec4(A._14) * B4;
		r2 += F32vec4(A._24) * B4;
		res._L1 = r1;
		res._L2 = r2;

		r1 = F32vec4(A._31) * B1;
		r2 = F32vec4(A._41) * B1;
		r1 += F32vec4(A._32) * B2;
		r2 += F32vec4(A._42) * B2;
		r1 += F32vec4(A._33) * B3;
		r2 += F32vec4(A._43) * B3;
		r1 += F32vec4(A._34) * B4;
		r2 += F32vec4(A._44) * B4;
		res._L3 = r1;
		res._L4 = r2;
#else
		res._11 = A._11 * B._11 + A._12 * B._21 + 
					 A._13 * B._31 + A._14 * B._41;
		res._12 = A._11 * B._12 + A._12 * B._22 + 
					 A._13 * B._32 + A._14 * B._42;
		res._13 = A._11 * B._13 + A._12 * B._23 + 
					 A._13 * B._33 + A._14 * B._43;
		res._14 = A._11 * B._14 + A._12 * B._24 + 
					 A._13 * B._34 + A._14 * B._44;

		res._21 = A._21 * B._11 + A._22 * B._21 + 
					 A._23 * B._31 + A._24 * B._41;
		res._22 = A._21 * B._12 + A._22 * B._22 + 
					 A._23 * B._32 + A._24 * B._42;
		res._23 = A._21 * B._13 + A._22 * B._23 + 
					 A._23 * B._33 + A._24 * B._43;
		res._24 = A._21 * B._14 + A._22 * B._24 + 
					 A._23 * B._34 + A._24 * B._44;

		res._31 = A._31 * B._11 + A._32 * B._21 + 
					 A._33 * B._31 + A._34 * B._41;
		res._32 = A._31 * B._12 + A._32 * B._22 + 
					 A._33 * B._32 + A._34 * B._42;
		res._33 = A._31 * B._13 + A._32 * B._23 + 
					 A._33 * B._33 + A._34 * B._43;
		res._34 = A._31 * B._14 + A._32 * B._24 + 
					 A._33 * B._34 + A._34 * B._44;

		res._41 = A._41 * B._11 + A._42 * B._21 + 
					 A._43 * B._31 + A._44 * B._41;
		res._42 = A._41 * B._12 + A._42 * B._22 + 
					 A._43 * B._32 + A._44 * B._42;
		res._43 = A._41 * B._13 + A._42 * B._23 + 
					 A._43 * B._33 + A._44 * B._43;
		res._44 = A._41 * B._14 + A._42 * B._24 + 
					 A._43 * B._34 + A._44 * B._44;
#endif // ENABLE_VEC
		return res;
	}

	// ----------------------------------------------------------
	//  Name:   matrix * matrix
	//  Desc:   Matrix multiplication of A and B. Returns[A]*[B].
	// ----------------------------------------------------------
	inline matrix operator * (const matrix& A, const matrix& B) {
		return MatrixMult(A, B);
	}

	// -------------------------------------------------------
	//  Name:   matrix *= matrix
	//  Desc:   Matrix multiplication of A by B. [A] =[A]*[B].
	// -------------------------------------------------------
	inline matrix& matrix::operator *= (const matrix& B) {
#ifdef ENABLE_VEC
		F32vec4 r1, r2;
		F32vec4 B1 = B._L1, B2 = B._L2, B3 = B._L3, B4 = B._L4;

		r1 = (F32vec4)_mm_shuffle_ps(_L1,_L1,0x00) * B1;
		r2 = (F32vec4)_mm_shuffle_ps(_L2,_L2,0x00) * B1;
		r1 += (F32vec4)_mm_shuffle_ps(_L1,_L1,0x55) * B2;
		r2 += (F32vec4)_mm_shuffle_ps(_L2,_L2,0x55) * B2;
		r1 += (F32vec4)_mm_shuffle_ps(_L1,_L1,0xAA) * B3;
		r2 += (F32vec4)_mm_shuffle_ps(_L2,_L2,0xAA) * B3;
		r1 += (F32vec4)_mm_shuffle_ps(_L1,_L1,0xFF) * B4;
		r2 += (F32vec4)_mm_shuffle_ps(_L2,_L2,0xFF) * B4;
		_L1 = r1;
		_L2 = r2;

		r1 = (F32vec4)_mm_shuffle_ps(_L3,_L3,0x00) * B1;
		r2 = (F32vec4)_mm_shuffle_ps(_L4,_L4,0x00) * B1;
		r1 += (F32vec4)_mm_shuffle_ps(_L3,_L3,0x55) * B2;
		r2 += (F32vec4)_mm_shuffle_ps(_L4,_L4,0x55) * B2;
		r1 += (F32vec4)_mm_shuffle_ps(_L3,_L3,0xAA) * B3;
		r2 += (F32vec4)_mm_shuffle_ps(_L4,_L4,0xAA) * B3;
		r1 += (F32vec4)_mm_shuffle_ps(_L3,_L3,0xFF) * B4;
		r2 += (F32vec4)_mm_shuffle_ps(_L4,_L4,0xFF) * B4;
		_L3 = r1;
		_L4 = r2;
#else
		*this = MatrixMult(*this, B);
#endif // ENABLE_VEC
		return *this;
	}

	// ----------------------------------------------------------
	//  Name:   matrix * float
	//  Desc:   Matrix elements multiplication with scalar. 
	//				Returns [A]*s.
	// ----------------------------------------------------------
	inline matrix operator * (const matrix& A, const float s) {
		matrix res;
#ifdef ENABLE_VEC
		vector S = vector(s);
		res._L1 = A._L1 * S;
		res._L2 = A._L2 * S;
		res._L3 = A._L3 * S;
		res._L4 = A._L4 * S;
#else
		res._L1 = vector(A._L1) * s;
		res._L2 = vector(A._L2) * s;
		res._L3 = vector(A._L3) * s;
		res._L4 = vector(A._L4) * s;
#endif // ENABLE_VEC
		return res;
	}
	inline matrix operator * (const float s, const matrix& A) {
		matrix res;
#ifdef ENABLE_VEC
		F32vec4 S = F32vec4(s);
		res._L1 = A._L1 * S;
		res._L2 = A._L2 * S;
		res._L3 = A._L3 * S;
		res._L4 = A._L4 * S;
#else
		res._L1 = vector(A._L1) * s;
		res._L2 = vector(A._L2) * s;
		res._L3 = vector(A._L3) * s;
		res._L4 = vector(A._L4) * s;
#endif // ENABLE_VEC
		return res;
	}

	// ----------------------------------------------------------
	//  Name:   matrix *= float
	//  Desc:   Matrix elements multiplication by scalar. 
	//				[A] = [A]*s.
	// ----------------------------------------------------------
	inline matrix& matrix::operator *= (const float s) {
#ifdef ENABLE_VEC
		F32vec4 S = F32vec4(s);
		_L1 = _L1 * S;
		_L2 = _L2 * S;
		_L3 = _L3 * S;
		_L4 = _L4 * S;
#else
		_L1 = vector(_L1) * s;
		_L2 = vector(_L2) * s;
		_L3 = vector(_L3) * s;
		_L4 = vector(_L4) * s; 
#endif // ENABLE_VEC
		return *this;
	}

	// ----------------------------------------------------------
	//  Name:   matrix + matrix
	//  Desc:   Matrix elements addition between A and B. 
	//				Returns [A]+[B].
	// ----------------------------------------------------------
	inline matrix operator + (const matrix& A, const matrix& B) {
		matrix res;
#ifdef ENABLE_VEC
		res._L1 = _mm_add_ps(A._L1, B._L1);
		res._L2 = _mm_add_ps(A._L2, B._L2);
		res._L3 = _mm_add_ps(A._L3, B._L3);
		res._L4 = _mm_add_ps(A._L4, B._L4);
#else
		res._L1 = (__m128)(vector(A._L1) + vector(B._L1));
		res._L2 = (__m128)(vector(A._L2) + vector(B._L2));
		res._L3 = (__m128)(vector(A._L3) + vector(B._L3));
		res._L4 = (__m128)(vector(A._L4) + vector(B._L4));
#endif // ENABLE_VEC
		return res;
	}

	// ----------------------------------------------------------
	//  Name:   matrix += matrix
	//  Desc:   Matrix addition of A by B. [A] = [A]+[B].
	// ----------------------------------------------------------
	inline matrix& matrix::operator += (const matrix& B) {
#ifdef ENABLE_VEC
		_L1 = _mm_add_ps(_L1, B._L1);
		_L2 = _mm_add_ps(_L2, B._L2);
		_L3 = _mm_add_ps(_L3, B._L3);
		_L4 = _mm_add_ps(_L4, B._L4);
#else
		_L1 = (__m128)(vector(_L1) + vector(B._L1));
		_L2 = (__m128)(vector(_L2) + vector(B._L2));
		_L3 = (__m128)(vector(_L3) + vector(B._L3));
		_L4 = (__m128)(vector(_L4) + vector(B._L4));
#endif // ENABLE_VEC
		return *this;
	}

	// ----------------------------------------------------------
	//  Name:   matrix - matrix
	//  Desc:   Matrix elements substraction between A and B. 
	//				Returns [A]-[B].
	// ----------------------------------------------------------
	inline matrix operator - (const matrix& A, const matrix& B) {
		matrix res;
#ifdef ENABLE_VEC
		res._L1 = _mm_sub_ps(A._L1, B._L1);
		res._L2 = _mm_sub_ps(A._L2, B._L2);
		res._L3 = _mm_sub_ps(A._L3, B._L3);
		res._L4 = _mm_sub_ps(A._L4, B._L4);
#else
		res._L1 = (__m128)(vector(A._L1) - vector(B._L1));
		res._L2 = (__m128)(vector(A._L2) - vector(B._L2));
		res._L3 = (__m128)(vector(A._L3) - vector(B._L3));
		res._L4 = (__m128)(vector(A._L4) - vector(B._L4));
#endif // ENABLE_VEC
		return res;
	}

	// ----------------------------------------------------------
	//  Name:   matrix -= matrix
	//  Desc:   Matrix substraction of A by B. [A] = [A]-[B].
	// ----------------------------------------------------------
	inline matrix& matrix::operator -= (const matrix& B) {
#ifdef ENABLE_VEC
		_L1 = _mm_sub_ps(_L1, B._L1);
		_L2 = _mm_sub_ps(_L2, B._L1);
		_L3 = _mm_sub_ps(_L3, B._L1);
		_L4 = _mm_sub_ps(_L4, B._L1);
#else
		_L1 = (__m128)(vector(_L1) - vector(B._L1));
		_L2 = (__m128)(vector(_L2) - vector(B._L2));
		_L3 = (__m128)(vector(_L3) - vector(B._L3));
		_L4 = (__m128)(vector(_L4) - vector(B._L4));
#endif // ENABLE_VEC
		return *this;
	}

	// ----------------------------------------------------------
	//  Name:   -matrix (Unary minus)
	//  Desc:   Matrix elements are negate. Returns -[A].
	// ----------------------------------------------------------
	inline matrix operator - (const matrix& A) {
		matrix res;
#ifdef ENABLE_VEC
		__m128 masksign = _MASKSIGNs_;
		res._L1 = _mm_xor_ps(masksign,A._L1);
		res._L2 = _mm_xor_ps(masksign,A._L2);
		res._L3 = _mm_xor_ps(masksign,A._L3);
		res._L4 = _mm_xor_ps(masksign,A._L4);
#else
		res._L1 = (__m128)(-vector(A._L1));
		res._L2 = (__m128)(-vector(A._L2));
		res._L3 = (__m128)(-vector(A._L3));
		res._L4 = (__m128)(-vector(A._L4));
#endif // ENABLE_VEC
		return res;
	}

	// ----------------------------------------------------------
	//  Name:   +matrix (Unary plus)
	//  Desc:   Returns the same matrix [A].
	// ----------------------------------------------------------
	inline matrix operator + (const matrix& A) {
		return A;
	}

	// ----------------------------------------------------------
	//  Name:   matrix::Transpose                           t
	//  Desc:   Transpose the 4x4 matrix. Matrix is set to [M] .
	// ----------------------------------------------------------
	inline void matrix::Transpose() {
#ifdef ENABLE_VEC
		__m128	xmm0 = _mm_unpacklo_ps(_L1,_L2),
			xmm1 = _mm_unpacklo_ps(_L3,_L4),
			xmm2 = _mm_unpackhi_ps(_L1,_L2),
			xmm3 = _mm_unpackhi_ps(_L3,_L4);

		_L1 = _mm_movelh_ps(xmm0,xmm1);
		_L2 = _mm_movehl_ps(xmm1,xmm0);
		_L3 = _mm_movelh_ps(xmm2,xmm3);
		_L4 = _mm_movehl_ps(xmm3,xmm2);
#else
#define xchange(a,b) { float c = a; a = b; b = c; }
		xchange(_12, _21);
		xchange(_13, _31);
		xchange(_14, _41);
		xchange(_23, _32);
		xchange(_24, _42);
		xchange(_34, _43);
#undef xchange
#endif // ENABLE_VEC
	}


	/***********************************************************/
	/*                    Vector[4] Operators                  */
	/***********************************************************/

	// ----------------------------------------------------------
	//  Name:   VectorMult
	//  Desc:   Vector multiplication with matrix.
	// ----------------------------------------------------------
	inline void VectorMult(
		const vector& Vec, const matrix& Mat, vector& res) {
#ifdef ENABLE_VEC
		F32vec4 result;
		result  = (F32vec4)_mm_shuffle_ps(Vec,Vec,0x00) * Mat._L1;
		result += (F32vec4)_mm_shuffle_ps(Vec,Vec,0x55) * Mat._L2;
		result += (F32vec4)_mm_shuffle_ps(Vec,Vec,0xAA) * Mat._L3;
		result += (F32vec4)_mm_shuffle_ps(Vec,Vec,0xFF) * Mat._L4;
		res = result;
#else 
		res.x = Vec * vector(Mat._L1);
		res.y = Vec * vector(Mat._L2);
		res.z = Vec * vector(Mat._L3);
		res.w = Vec * vector(Mat._L4);
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   VectorMult
	//  Desc:   Vector multiplication with matrix.
	// ----------------------------------------------------------
	inline vector VectorMult(
		const vector& Vec, const matrix& Mat) {
#ifdef ENABLE_VEC
		F32vec4 result;
		result  = (F32vec4)_mm_shuffle_ps(Vec,Vec,0x00) * Mat._L1;
		result += (F32vec4)_mm_shuffle_ps(Vec,Vec,0x55) * Mat._L2;
		result += (F32vec4)_mm_shuffle_ps(Vec,Vec,0xAA) * Mat._L3;
		result += (F32vec4)_mm_shuffle_ps(Vec,Vec,0xFF) * Mat._L4;
		return (vector)result;
#else
		vector res;
		res.x = Vec * vector(Mat._L1);
		res.y = Vec * vector(Mat._L2);
		res.z = Vec * vector(Mat._L3);
		res.w = Vec * vector(Mat._L4);
		return res;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector * matrix
	//  Desc:   Vector multiplication with matrix.
	// ----------------------------------------------------------
	inline vector operator * (
		const vector& Vec, const matrix& Mat) {
		return VectorMult(Vec, Mat);
	}

	// ----------------------------------------------------------
	//  Name:   vector *= matrix
	//  Desc:   Vector multiplication with matrix.
	// ----------------------------------------------------------
	inline vector& vector::operator *= (const matrix& Mat) {
		VectorMult(*this, Mat, *this);
		return *this;
	}

	// ----------------------------------------------------------
	//  Name:   vector * vector                          _   _
	//  Desc:   Dot product of the two vectors. Returns [A]·[B].
	// ----------------------------------------------------------
	inline float operator * (const vector& A, const vector& B) {
#ifdef ENABLE_VEC
		F32vec4 r = _mm_mul_ps(A,B);
		r = _mm_add_ps(_mm_movehl_ps(r,r),r);
		F32vec1 t = _mm_add_ss(_mm_shuffle_ps(r,r,1), r);
		return *(float *)&t;
#else
		return A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector % vector
	//  Desc:   Cross product of the two 3D vectors.
	// ----------------------------------------------------------
	inline vector operator % (const vector& A, const vector& B) {
#ifdef ENABLE_VEC
		F32vec4 l1, l2, m1, m2;
		l1 = _mm_shuffle_ps(A,A, _MM_SHUFFLE(3,1,0,2));
		l2 = _mm_shuffle_ps(B,B, _MM_SHUFFLE(3,0,2,1));
		m2 = l1*l2 & _0FFF_;
		l1 = _mm_shuffle_ps(A,A, _MM_SHUFFLE(3,0,2,1));
		l2 = _mm_shuffle_ps(B,B, _MM_SHUFFLE(3,1,0,2));
		m1 = l1*l2;
		return (vector)(m1-m2);
#else
		vector res;
		res.x = A.y * B.z - A.z * B.y;
		res.y = A.x * B.z - A.z * B.x;
		res.z = A.x * B.y - A.y * B.x;
		// 1.0f or 0.0f ???
		res.w = 1.0f;
		return res;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector * float
	//  Desc:   Multiplies the vector elements by scalar.
	// ----------------------------------------------------------
	inline vector operator * (const vector& V, const float s) {
#ifdef ENABLE_VEC
		return (vector)(V.vec * F32vec4(s));
#else
		vector res;
		res.x = V.x * s;
		res.y = V.y * s;
		res.z = V.z * s;
		res.w = V.w * s;
		return res;
#endif // ENABLE_VEC
	}
	inline vector operator * (const float s, const vector& V) {
#ifdef ENABLE_VEC
		return (vector)(V.vec * F32vec4(s));
#else
		vector res;
		res.x = V.x * s;
		res.y = V.y * s;
		res.z = V.z * s;
		res.w = V.w * s;
		return res;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector *= float                  ___     ___
	//  Desc:   Vector multiplication by float. [Vec] = [Vec]*s.
	// ----------------------------------------------------------
	inline vector& vector::operator *= (const float s) {
#ifdef ENABLE_VEC
		vec = vec * F32vec4(s);
#else
		x *= s;
		y *= s;
		z *= s;
		w *= s;
#endif // ENABLE_VEC
		return *this;
	}

	// ----------------------------------------------------------
	//  Name:   vector | vector
	//  Desc:   Vector elements product.
	// ----------------------------------------------------------
	inline vector operator | (const vector& A, const vector& B) {
#ifdef ENABLE_VEC
		return (vector)_mm_mul_ps(A.vec, B.vec);
#else
		vector res;
		res.x = A.x * B.x;
		res.y = A.y * B.y;
		res.z = A.z * B.z;
		res.w = A.w * B.w;
		return res;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector |= vector
	//  Desc:   Vector elements product.
	// ----------------------------------------------------------
	inline vector& vector::operator |= (const vector& B) {
#ifdef ENABLE_VEC
		vec = _mm_mul_ps(vec, B.vec);
#else
		x *= B.x;
		y *= B.y;
		z *= B.z;
		w *= B.w;
#endif // ENABLE_VEC
		return *this;
	}

	// ----------------------------------------------------------
	//  Name:   vector + vector       _   _
	//  Desc:   Vector addition. Returns [A]+[B].
	// ----------------------------------------------------------
	inline vector operator + (const vector& A, const vector& B) {
#ifdef ENABLE_VEC
		return (vector)_mm_add_ps(A.vec, B.vec);
#else
		vector res;
		res.x = A.x + B.x;
		res.y = A.y + B.y;
		res.z = A.z + B.z;
		res.w = A.w + B.w;
		return res;
#endif // ENABLE_VEC
	}

	inline vector operator + (const vector& A, const float f) {
		vector res;
		res.x = A.x + f;
		res.y = A.y + f;
		res.z = A.z + f;
		res.w = A.w + f;
		return res;
	}

	inline vector operator + (const float f, const vector& A) {
		return A + f;
	}

	// ----------------------------------------------------------
	//  Name:   vector - vector               _   _
	//  Desc:   Vector substraction. Returns [A]-[B].
	// ----------------------------------------------------------
	inline vector operator - (const vector& A, const vector& B) {
#ifdef ENABLE_VEC
		return (vector)_mm_sub_ps(A.vec, B.vec);
#else
		vector res;
		res.x = A.x - B.x;
		res.y = A.y - B.y;
		res.z = A.z - B.z;
		res.w = A.w - B.w;
		return res;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector += vector        _     _   _
	//  Desc:   Vector addition.       [A] = [A]+[B].
	// ----------------------------------------------------------
	inline vector& vector::operator += (const vector& B) {
#ifdef ENABLE_VEC
		vec = _mm_add_ps(vec, B.vec);
#else
		x += B.x;
		y += B.y;
		z += B.z;
		w += B.w;
#endif // ENABLE_VEC
		return *this;
	}

	inline vector& vector::operator += (const float f) {
		x += f;
		y += f;
		z += f;
		w += f;
		return *this;
	}

	// ----------------------------------------------------------
	//  Name:   vector -= vector        _     _   _
	//  Desc:   Vector substraction.   [A] = [A]-[B].
	// ----------------------------------------------------------
	inline vector& vector::operator -= (const vector& B) {
#ifdef ENABLE_VEC
		vec = _mm_sub_ps(vec, B.vec);
#else
		x -= B.x;
		y -= B.y;
		z -= B.z;
		w -= B.w;
#endif // ENABLE_VEC
		return *this;
	}

	// ----------------------------------------------------------
	//  Name:   -vector (Unary minus)                  _
	//  Desc:   Vectors elements are negate. Returns -[A].
	// ----------------------------------------------------------
	inline vector operator - (const vector& A) {
#ifdef ENABLE_VEC
		return (vector)_mm_xor_ps(_MASKSIGNs_,A);
#else
		vector res;
		res.x = -A.x;
		res.y = -A.y;
		res.z = -A.z;
		res.w = -A.w;
		return res;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   +vector (Unary plus)
	//  Desc:   Returns the same vector.
	// ----------------------------------------------------------
	inline vector operator + (const vector& A) {
		return A;
	}

	inline float vector::LengthSquared() const {
#ifdef ENABLE_VEC
		F32vec4 r = _mm_mul_ps(vec, vec);
		r = _mm_add_ps(_mm_movehl_ps(r, r), r);
		F32vec1 t = _mm_add_ss(_mm_shuffle_ps(r, r, 1), r);
		return (*(float*)&t);
#else
		return x * x + y * y + z * z;
#endif // ENABLE_VEC

	}

	// ----------------------------------------------------------
	//  Name:   vector::Length
	//  Desc:   Returns the length of the vector.
	// ----------------------------------------------------------
	inline float vector::Length() const {
#ifdef ENABLE_VEC
		F32vec4 r = _mm_mul_ps(vec,vec);
		r = _mm_add_ps(_mm_movehl_ps(r,r),r);
		F32vec1 t = _mm_add_ss(_mm_shuffle_ps(r,r,1), r);
		t = sqrt(t);
		return (*(float *)&t);
#else
		return sqrt(x*x + y*y + z*z);
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector::Normalize
	//  Desc:   Normalized the (X,Y,Z) elements of the vector.
	// ----------------------------------------------------------
	inline vector& vector::Normalize() {
#ifdef ENABLE_VEC
		F32vec4 r = _mm_mul_ps(vec,vec);
		r = _mm_add_ps(_mm_movehl_ps(r,r),r);
		F32vec1 t = _mm_add_ss(_mm_shuffle_ps(r,r,1), r);
#ifdef ZERO_VECTOR
		t = _mm_cmpneq_ss(t, _mm_setzero_ps()) & rsqrt_nr(t);
#else
		t = rsqrt_nr(t);
#endif
		vec = _mm_mul_ps(vec, _mm_shuffle_ps(t,t,0x00));
#else
		float _1_length = 1.0f / this->Length();
		x *= _1_length;
		y *= _1_length;
		z *= _1_length;
		w *= _1_length;
#endif // ENABLE_VEC
		return *this;
	}

	// ----------------------------------------------------------
	//  Name:   ~vector [Normalize]
	//  Desc:   Normalized the source vector.
	// ----------------------------------------------------------
	inline vector operator ~ (const vector& V) {
#ifdef ENABLE_VEC
		F32vec4 r = _mm_mul_ps(V,V);
		r = _mm_add_ps(_mm_movehl_ps(r,r),r);
		F32vec1 t = _mm_add_ss(_mm_shuffle_ps(r,r,1), r);
#ifdef ZERO_VECTOR
		t = _mm_cmpneq_ss(t, _mm_setzero_ps()) & rsqrt_nr(t);
#else
		t = rsqrt_nr(t);
#endif
		r = _mm_mul_ps(V, _mm_shuffle_ps(t,t,0x00));
#else
		vector r = V;
		float _1_length = 1.0f / r.Length();
		r.x = V.x * _1_length;
		r.y = V.y * _1_length;
		r.z = V.z * _1_length;
		r.w = V.w * _1_length;
#endif // ENABLE_VEC
		return (vector)r;
	}

	// ----------------------------------------------------------
	//  Name:   vector = vector3 (assignment)
	//  Desc:   Convert a vector3 into vector.
	// ----------------------------------------------------------
	inline vector& vector::operator = (const vector3& a) {
#ifdef ENABLE_VEC
		vec = _mm_and_ps(a.vec, _0FFF_);
		vec = _mm_or_ps(vec, _1000_);   // Set W to one.
#else
		memcpy(&vec, &(a.vec), 3 * sizeof(float));
		w = 1;
#endif // ENABLE_VEC
		return *this;
	}


	/***********************************************************/
	/*                    Vector[3] Operators                  */
	/***********************************************************/

	// ----------------------------------------------------------
	//  Name:   VectorMult
	//  Desc:   Vector multiplication with matrix.
	// ----------------------------------------------------------
	inline void VectorMult(
		const vector3& Vec, const matrix& Mat, vector& res) {
#ifdef ENABLE_VEC
		F32vec4 result = Mat._L4;
		result += F32vec4(_mm_shuffle_ps(Vec,Vec,0x00)) * Mat._L1;
		result += F32vec4(_mm_shuffle_ps(Vec,Vec,0x55)) * Mat._L2;
		result += F32vec4(_mm_shuffle_ps(Vec,Vec,0xAA)) * Mat._L3;
		res = result;
#else 
		vector v;
		v.vec = Vec.vec;
		res.x = v.x * Mat._11 + v.y * Mat._12 + 
				  v.z * Mat._13 + v.w * Mat._14;
		res.y = v.x * Mat._21 + v.y * Mat._22 + 
				  v.z * Mat._23 + v.w * Mat._24;
		res.z = v.x * Mat._31 + v.y * Mat._32 + 
				  v.z * Mat._33 + v.w * Mat._34;
		res.w = v.x * Mat._41 + v.y * Mat._42 + 
				  v.z * Mat._43 + v.w * Mat._44;
#endif // ENABLE_VEC
	}

	inline void VectorMult(
		const vector3& Vec, const matrix& Mat, vector3& res) {
#ifdef ENABLE_VEC
		F32vec4 result = Mat._L4;
		result += F32vec4(_mm_shuffle_ps(Vec,Vec,0x00)) * Mat._L1;
		result += F32vec4(_mm_shuffle_ps(Vec,Vec,0x55)) * Mat._L2;
		result += F32vec4(_mm_shuffle_ps(Vec,Vec,0xAA)) * Mat._L3;

		F32vec1 W = _mm_shuffle_ps(result, result, 0xFF);
		W = F32vec1(1.0f)/W;
		res = result * _mm_shuffle_ps(W, W, 0x00);
#else
		res.x = Vec.x * Mat._11 + Vec.y * Mat._12 + 
				  Vec.z * Mat._13 + 1.0f * Mat._14;
		res.y = Vec.x * Mat._21 + Vec.y * Mat._22 + 
				  Vec.z * Mat._23 + 1.0f * Mat._24;
		res.z = Vec.x * Mat._31 + Vec.y * Mat._32 + 
				  Vec.z * Mat._33 + 1.0f * Mat._34;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   VectorMult
	//  Desc:   Vector multiplication with matrix.
	// ----------------------------------------------------------
	inline vector VectorMult(
		const vector3& Vec, const matrix& Mat) {
#ifdef ENABLE_VEC
		F32vec4 result = Mat._L4;
		result += F32vec4(_mm_shuffle_ps(Vec,Vec,0x00)) * Mat._L1;
		result += F32vec4(_mm_shuffle_ps(Vec,Vec,0x55)) * Mat._L2;
		result += F32vec4(_mm_shuffle_ps(Vec,Vec,0xAA)) * Mat._L3;
		return (vector)result;
#else
		vector res;
		res.x = Vec.x * Mat._11 + Vec.y * Mat._12 + 
				  Vec.z * Mat._13 + 1.0f * Mat._14;
		res.y = Vec.x * Mat._21 + Vec.y * Mat._22 + 
				  Vec.z * Mat._23 + 1.0f * Mat._24;
		res.z = Vec.x * Mat._31 + Vec.y * Mat._32 + 
				  Vec.z * Mat._33 + 1.0f * Mat._34;
		return res;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector3 * matrix
	//  Desc:   Vector multiplication with matrix.
	// ----------------------------------------------------------
	inline vector operator * (
		const vector3& Vec, const matrix& Mat) {
		return VectorMult(Vec, Mat);
	}

	// ----------------------------------------------------------
	//  Name:   vector3 *= matrix
	//  Desc:   Vector multiplication with matrix.
	// ----------------------------------------------------------
	inline vector3& vector3::operator *= (const matrix& Mat) {
		VectorMult(*this, Mat, *this);
		return *this;
	}

	// ----------------------------------------------------------
	//  Name:   vector3 * vector3                        _   _
	//  Desc:   Dot product of the two vectors. Returns [A]·[B].
	// ----------------------------------------------------------
	inline float operator * (
		const vector3& A, const vector3& B) {
#ifdef ENABLE_VEC
		F32vec4 r = _mm_mul_ps(A,B);
		F32vec1 t = _mm_add_ss(
			_mm_shuffle_ps(r,r,1), 
			_mm_add_ps(_mm_movehl_ps(r,r),r));
		return (*(float *)&t);
#else
		return A.x * B.x + A.y * B.y + A.z * B.z;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector3 % vector3                         _   _
	//  Desc:   Cross product of the two vectors. Returns[A]x[B].
	// ----------------------------------------------------------
	inline vector3 operator % (
		const vector3& A, const vector3& B) {
#ifdef ENABLE_VEC
		F32vec4 l1, l2, m1, m2;
		l1 = _mm_shuffle_ps(A,A, _MM_SHUFFLE(3,1,0,2));
		l2 = _mm_shuffle_ps(B,B, _MM_SHUFFLE(3,0,2,1));
		m2 = l1*l2;
		l1 = _mm_shuffle_ps(A,A, _MM_SHUFFLE(3,0,2,1));
		l2 = _mm_shuffle_ps(B,B, _MM_SHUFFLE(3,1,0,2));
		m1 = l1*l2;
		return (vector3)(m1-m2);
#else
		vector3 ret;
		ret.x = A.y * B.z - A.z * B.y;
		ret.y = A.x * B.z - A.z * B.x;
		ret.z = A.x * B.y - A.y * B.x;
		return ret;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector3 * float
	//  Desc:   Multiply the vector elements by scalar.
	// ----------------------------------------------------------
	inline vector3 operator * (const vector3& V, const float s) {
#ifdef ENABLE_VEC
		return (vector3)(V.vec * F32vec4(s));
#else
		vector3 ret;
		ret.x = V.x * s;
		ret.y = V.y * s;
		ret.z = V.z * s;
		return V;
#endif // ENABLE_VEC
	}
	inline vector3 operator * (const float s, const vector3& V) {
#ifdef ENABLE_VEC
		return (vector3)(V.vec * F32vec4(s));
#else
		vector3 ret;
		ret.x = V.x * s;
		ret.y = V.y * s;
		ret.z = V.z * s;
		return V;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector3 *= float                 ___     ___
	//  Desc:   Vector multiplication by float. [Vec] = [Vec]*s.
	// ----------------------------------------------------------
	inline vector3& vector3::operator *= (const float s) {
#ifdef ENABLE_VEC
		vec = vec * F32vec4(s);
		return *this;
#else
		x *= s;
		y *= s;
		z *= s;
		return *this;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector3 | vector3
	//  Desc:   Vector elements product.
	// ----------------------------------------------------------
	inline vector3 operator | (
		const vector3& A, const vector3& B) {
#ifdef ENABLE_VEC
		return (vector3)_mm_mul_ps(A.vec, B.vec);
#else
		vector3 ret;
		ret.x = A.x * B.x;
		ret.y = A.y * B.y;
		ret.z = A.z * B.z;
		return ret;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector3 |= vector3
	//  Desc:   Vector elements product.
	// ----------------------------------------------------------
	inline vector3& vector3::operator |= (const vector3& B) {
#ifdef ENABLE_VEC
		vec = _mm_mul_ps(vec, B.vec);
		return *this;
#else
		x *= B.x;
		y *= B.y;
		z *= B.z;
		return *this;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector3 + vector3         _   _
	//  Desc:   Vector addition. Returns [A]+[B].
	// ----------------------------------------------------------
	inline vector3 operator + (
		const vector3& A, const vector3& B) {
#ifdef ENABLE_VEC
		return (vector3)_mm_add_ps(A.vec, B.vec);
#else
		vector3 ret;
		ret.x = A.x + B.x;
		ret.y = A.y + B.y;
		ret.z = A.z + B.z;
		return ret;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector3 - vector3             _   _
	//  Desc:   Vector substraction. Returns [A]-[B].
	// ----------------------------------------------------------
	inline vector3 operator - (
		const vector3& A, const vector3& B) {
#ifdef ENABLE_VEC
		return (vector3)_mm_sub_ps(A.vec, B.vec);
#else
		vector3 ret;
		ret.x = A.x - B.x;
		ret.y = A.y - B.y;
		ret.z = A.z - B.z;
		return ret;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector3 += vector3      _     _   _
	//  Desc:   Vector addition.       [A] = [A]+[B].
	// ----------------------------------------------------------
	inline vector3& vector3::operator += (const vector3& B) {
#ifdef ENABLE_VEC
		vec = _mm_add_ps(vec, B.vec);
		return *this;
#else
		x += B.x;
		y += B.y;
		z += B.z;
		return *this;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector3 -= vector3      _     _   _
	//  Desc:   Vector substraction.   [A] = [A]-[B].
	// ----------------------------------------------------------
	inline vector3& vector3::operator -= (const vector3& B) {
#ifdef ENABLE_VEC
		vec = _mm_sub_ps(vec, B.vec);
		return *this;
#else
		x -= B.x;
		y -= B.y;
		z -= B.z;
		return *this;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   -vector3 (Unary minus)                 _
	//  Desc:   Vectors elements are negate. Returns -[A].
	// ----------------------------------------------------------
	inline vector3 operator - (const vector3& A) {
#ifdef ENABLE_VEC
		return (vector3)_mm_xor_ps(_MASKSIGNs_,A);
#else
		vector3 ret;
		ret.x = -A.x;
		ret.y = -A.y;
		ret.z = -A.z;
		return ret;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   +vector3 (Unary plus)
	//  Desc:   Returns the same vector.
	// ----------------------------------------------------------
	inline vector3 operator + (const vector3& A) {
		return A;
	}

	// ----------------------------------------------------------
	//  Name:   vector3::Length
	//  Desc:   Returns the length of the vector.
	// ----------------------------------------------------------
	inline float vector3::Length() {
#ifdef ENABLE_VEC
		F32vec4 r = _mm_mul_ps(vec,vec);
		F32vec1 t = _mm_add_ss(
			_mm_shuffle_ps(r,r,1), 
			_mm_add_ss(_mm_movehl_ps(r,r),r));
		t = sqrt(t);
		return (*(float *)&t);
#else
		float val = *this * *this;
		return sqrt(val);
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector3::Normalize
	//  Desc:   Normalized the (X,Y,Z) elements of the vector.
	// ----------------------------------------------------------
	inline vector3& vector3::Normalize() {
#ifdef ENABLE_VEC
		F32vec4 r = _mm_mul_ps(vec,vec);
		F32vec1 t = _mm_add_ss(
			_mm_shuffle_ps(r,r,1), 
			_mm_add_ss(_mm_movehl_ps(r,r),r));
#ifdef ZERO_VECTOR
		t = _mm_cmpneq_ss(t, _mm_setzero_ps()) & rsqrt_nr(t);
#else
		t = rsqrt_nr(t);
#endif
		vec = _mm_mul_ps(vec, _mm_shuffle_ps(t,t,0x00));
		return *this;
#else
		float l = this->Length();
		l = 1.0f / l;
		*this *= l;
		return *this;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   ~vector3 [Normalize]
	//  Desc:   Normalized the source vector.
	// ----------------------------------------------------------
	inline vector3 operator ~ (const vector3& V) {
#ifdef ENABLE_VEC
		F32vec4 r = _mm_mul_ps(V.vec,V.vec);
		F32vec1 t = _mm_add_ss(
			_mm_shuffle_ps(r,r,1), 
			_mm_add_ss(_mm_movehl_ps(r,r),r));
#ifdef ZERO_VECTOR
		t = _mm_cmpneq_ss(t, _mm_setzero_ps()) & rsqrt_nr(t);
#else
		t = rsqrt_nr(t);
#endif
		return (vector3)_mm_mul_ps(
			V.vec, 
			_mm_shuffle_ps(t,t,0x00));
#else
		vector3 res = V;
		float l = 1.0f / res.Length();
		res.x *= l;
		res.y *= l;
		res.z *= l;
		return res;
#endif // ENABLE_VEC
	}

	// ----------------------------------------------------------
	//  Name:   vector3 = vector (assignment)
	//  Desc:   Convert a vector into vector3.
	// ----------------------------------------------------------
	inline vector3& vector3::operator = (const vector& a) {
#ifdef ENABLE_VEC
		F32vec1 W = _mm_shuffle_ps(a, a, 0xFF);
		W = F32vec1(1.0f)/W;
		vec = (F32vec4)a.vec * _mm_shuffle_ps(W, W, 0x00);
#else
		float s = 1.0f / a.w;
		x = a.x * s;
		y = a.y * s;
		z = a.z * s;
#endif // ENABLE_VEC
		return *this;
	}

	inline vector2 operator * (const vector2& v, const float f)
	{
		vector2 res = v;
		res.x *= f;
		res.y *= f;
		return res;
	}

	inline vector2 operator * (const float f, const vector2& v)
	{
		vector2 res = v;
		res.x *= f;
		res.y *= f;
		return res;
	}

	inline vector2 operator | (const vector2& v1, const vector2& v2)
	{
		vector2 res;
		res.x = v1.x * v2.x;
		res.y = v1.y * v2.y;
		return res;
	}

	inline vector2 operator + (const vector2& v)
	{
		vector2 res = v;
		return res;
	}

	inline vector2 operator + (const vector2& v1, const vector2& v2)
	{
		vector2 res;
		res.x = v1.x + v2.x;
		res.y = v1.y + v2.y;
		return res;
	}

	inline vector2 operator - (const vector2& v)
	{
		vector2 res;
		res.x = -v.x;
		res.y = -v.y;
		return res;
	}

	inline vector2 operator - (const vector2& v1, const vector2& v2)
	{
		vector2 res;
		res.x = v1.x - v2.x;
		res.y = v1.y - v2.y;
		return res;
	}

	inline vector2& vector2::operator *= (const float f)
	{
		x *= f;
		y *= f;
		return *this;
	}

	inline vector2& vector2::operator |= (const vector2& v)
	{
		x *= v.x;
		y *= v.y;
		return *this;
	}

	inline vector2& vector2::operator += (const vector2& v)
	{
		x += v.x;
		y += v.y;
		return *this;
	}

	inline vector2& vector2::operator -= (const vector2& v)
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	// Comparaison operator (for now == and !=)
	inline bool operator==(const vector& v1, const vector& v2)
	{
		if (fabsf(v1.x - v2.x) > epsilon) return false;
		if (fabsf(v1.y - v2.y) > epsilon) return false;
		if (fabsf(v1.z - v2.z) > epsilon) return false;
		if (fabsf(v1.w - v2.w) > epsilon) return false;
		return true;
	}

	inline bool operator!=(const vector& v1, const vector& v2) 
	{
		return !operator==(v1, v2);
	}

	inline bool operator==(const vector3& v1, const vector3& v2)
	{
		if (fabsf(v1.x - v2.x) > epsilon) return false;
		if (fabsf(v1.y - v2.y) > epsilon) return false;
		if (fabsf(v1.z - v2.z) > epsilon) return false;
		return true;
	}

	inline bool operator!=(const vector3& v1, const vector3& v2)
	{
		return !operator==(v1, v2);
	}

	inline bool operator==(const vector2& v1, const vector2& v2)
	{
		if (fabsf(v1.x - v2.x) > epsilon) return false;
		if (fabsf(v1.y - v2.y) > epsilon) return false;
		return true;
	}

	inline bool operator!=(const vector2& v1, const vector2& v2)
	{
		return !operator==(v1, v2);
	}

	// Comparison operator <.

	inline bool operator<(const vector& v1, const vector& v2)
	{
		std::vector<float> va1 = { v1.x, v1.y, v1.z, v1.w };
		std::vector<float> va2 = { v2.x, v2.y, v2.z, v2.w };
		return va1 < va2;
	}

	inline bool operator<(const vector3& v1, const vector3& v2)
	{
		std::vector<float> va1 = { v1.x, v1.y, v1.z };
		std::vector<float> va2 = { v2.x, v2.y, v2.z };
		return va1 < va2;
	}

	inline bool operator<(const vector2& v1, const vector2& v2)
	{
		std::vector<float> va1 = { v1.x, v1.y };
		std::vector<float> va2 = { v2.x, v2.y };
		return va1 < va2;
	}

} // end namespace sgl.
