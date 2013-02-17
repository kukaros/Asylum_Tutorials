//=============================================================================================================
#ifndef _OBJECTS_H_
#define _OBJECTS_H_

#include <d3dx9.h>

#define DOT(a, b) \
	(a.x * b.x + a.y * b.y + a.z * b.z)

#define MUL(o, x, y) \
	o.r = x.r * y.r; \
	o.g = x.g * y.g; \
	o.b = x.b * y.b; \
	o.a = x.a * y.a;

#define ADD_4C3()

#define CROSS(o, a, b) \
	o.x = a.y * b.z - a.z * b.y; \
	o.y = a.z * b.x - a.x * b.z; \
	o.z = a.x * b.y - a.y * b.x;

#define LENGTH(v) \
	sqrtf(v.x * v.x + v.y * v.y + v.z * v.z)

#define SQUARED_LENGTH(v) \
	(v.x * v.x + v.y * v.y + v.z * v.z)

#define NORMALIZE(o, v) \
	tmpf = 1.0f / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); \
	o.x = v.x * tmpf; \
	o.y = v.y * tmpf; \
	o.z = v.z * tmpf; \

enum LightTransferMethod
{
	Reflective,
	Refractive,
	Diffuse
};

enum BoxPlanes
{
	Left = 0,
	Right = 1,
	Top = 2,
	Bottom = 3,
	Front = 4,
	Back = 5
};

struct MaterialProperty
{
	D3DXVECTOR3		Emittance;
	D3DXCOLOR		Color;
	float			RefractiveIndex;
	float			Shininess;
	int				Transfer;
};

struct Ray
{
	D3DXVECTOR3 Origin;
	D3DXVECTOR3 Direction;
};

/**
 * \brief Abstract class for ray-traceable objects
 */
class Object
{
public:
	MaterialProperty Material;

	Object();
	virtual ~Object() {}

	virtual void Position(D3DXVECTOR3& out) = 0;
	virtual bool Intersect(D3DXVECTOR3& p, D3DXVECTOR3& n, const Ray& r) = 0;
	virtual bool Intersect(D3DXVECTOR3& p1, D3DXVECTOR3& p2, D3DXVECTOR3& n1, D3DXVECTOR3& n2, const Ray& r) = 0;
};

/**
 * \brief Algebraic sphere
 */
class Sphere : public Object
{
private:
	D3DXVECTOR3 center;
	float radius;

public:
	Sphere(const D3DXVECTOR3& c, float r);

	inline void Position(D3DXVECTOR3& out) {
		out = center;
	}

	bool Intersect(D3DXVECTOR3& p, D3DXVECTOR3& n, const Ray& r);
	bool Intersect(D3DXVECTOR3& p1, D3DXVECTOR3& p2, D3DXVECTOR3& n1, D3DXVECTOR3& n2, const Ray& r);
};

/**
 * \brief Plane
 */
class Plane : public Object
{
private:
	D3DXPLANE pl;

public:
	Plane(const D3DXPLANE& plane);

	inline void Position(D3DXVECTOR3& out) {
		// rossz
		out.x = out.y = out.z = 0;
	}

	bool Intersect(D3DXVECTOR3& p, D3DXVECTOR3& n, const Ray& r);
	bool Intersect(D3DXVECTOR3& p1, D3DXVECTOR3& p2, D3DXVECTOR3& n1, D3DXVECTOR3& n2, const Ray& r);
};

/**
 * \brief Box
 */
class Box : public Object
{
private:
	D3DXVECTOR3 low;
	D3DXVECTOR3 high;

public:
	Box(const D3DXVECTOR3& p, const D3DXVECTOR3& s);

	inline void Position(D3DXVECTOR3& out) {
		out.x = (low.x + high.x) * 0.5f;
		out.y = (low.y + high.y) * 0.5f;
		out.z = (low.z + high.z) * 0.5f;
	}

	bool Intersect(D3DXVECTOR3& p, D3DXVECTOR3& n, const Ray& r);
	bool Intersect(D3DXVECTOR3& p1, D3DXVECTOR3& p2, D3DXVECTOR3& n1, D3DXVECTOR3& n2, const Ray& r);
};

#endif
//=============================================================================================================
