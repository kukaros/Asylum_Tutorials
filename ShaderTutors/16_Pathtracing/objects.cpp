//=============================================================================================================
#include "objects.h"
#include <algorithm>

Object::Object()
{
	Material.Color = D3DXCOLOR(1, 1, 1, 1);
	Material.Emittance = D3DXVECTOR3(0, 0, 0);
	Material.Transfer = Diffuse;
	Material.RefractiveIndex = 1.0f;
	Material.Shininess = 6;
}
//=============================================================================================================
Sphere::Sphere(const D3DXVECTOR3& c, float r)
{
	center = c;
	radius = r;
}
//=============================================================================================================
bool Sphere::Intersect(D3DXVECTOR3& p, D3DXVECTOR3& n, const Ray& r)
{
	float tca, thc, d2, t, tmpf;
	D3DXVECTOR3 l = center - r.Origin;

	tca = DOT(l, r.Direction);

	if( tca < 0 )
		return false;

	d2 = DOT(l, l) - tca * tca;

	if( d2 > radius * radius )
		return false;

	thc = sqrtf(radius * radius - d2);
	t = min(tca - thc, tca + thc);
	
	p = r.Origin + t * r.Direction;
	n = p - center;
	
	NORMALIZE(n, n);
	return true;
}
//=============================================================================================================
bool Sphere::Intersect(D3DXVECTOR3& p1, D3DXVECTOR3& p2, D3DXVECTOR3& n1, D3DXVECTOR3& n2, const Ray& r)
{
	float tca, thc, d2, t1, t2, tmpf;
	D3DXVECTOR3 l = center - r.Origin;

	tca = DOT(l, r.Direction);

	if( tca < 0 )
		return false;

	d2 = DOT(l, l) - tca * tca;

	if( d2 > radius * radius )
		return false;

	thc = sqrtf(radius * radius - d2);
	t1 = min(tca - thc, tca + thc);
	t2 = max(tca - thc, tca + thc);
	
	p1 = r.Origin + t1 * r.Direction;
	p2 = r.Origin + t2 * r.Direction;

	n1 = p1 - center;
	n2 = p2 - center;
	
	NORMALIZE(n1, n1);
	NORMALIZE(n2, n2);

	return true;
}
//=============================================================================================================
Plane::Plane(const D3DXPLANE& plane)
{
	pl = plane;
}
//=============================================================================================================
bool Plane::Intersect(D3DXVECTOR3& p, D3DXVECTOR3& n, const Ray& r)
{
	float u, t;

	n = D3DXVECTOR3(pl.a, pl.b, pl.c);
	u = DOT(r.Direction, n);

	if( u >= 0 )
		return false;

	t = -(DOT(r.Origin, n) + pl.d) / u;
	p = r.Origin + t * r.Direction;

	return true;
}
//=============================================================================================================
bool Plane::Intersect(D3DXVECTOR3& p1, D3DXVECTOR3& p2, D3DXVECTOR3& n1, D3DXVECTOR3& n2, const Ray& r)
{
	float u, t;

	n1 = n2 = D3DXVECTOR3(pl.a, pl.b, pl.c);
	u = DOT(r.Direction, n1);

	if( u >= 0 )
		return false;

	t = -(DOT(r.Origin, n1) + pl.d) / u;
	p1 = p2 = r.Origin + t * r.Direction;

	return true;
}
//=============================================================================================================
Box::Box(const D3DXVECTOR3& p, const D3DXVECTOR3& s)
{
	low = p - s * 0.5f;
	high = p + s * 0.5f;
}

#define BOXTEST

//=============================================================================================================
bool Box::Intersect(D3DXVECTOR3& p, D3DXVECTOR3& n, const Ray& r)
{
	D3DXVECTOR3 l1(0, 0, 0), l2(0, 0, 0);
	float tn = -FLT_MAX;
	float tf = FLT_MAX;
	float t1, t2;

	// x planes
	if( r.Direction.x == 0 )
	{
		if( r.Origin.x < low.x || r.Origin.x > high.x )
			return false;
	}
	else
	{
		l1.x = -1;
		l2.x = 1;

		t1 = (low.x - r.Origin.x) / r.Direction.x;
		t2 = (high.x - r.Origin.x) / r.Direction.x;

		if( t1 > t2 )
		{
			std::swap(t1, t2);
			std::swap(l1, l2);
		}

		if( t1 > tn )
		{
			tn = t1;
			n = l1;
		}

		if( t2 < tf )
			tf = t2;

		if( tn > tf || tf < 0 )
			return false;
	}

	// y planes
	if( r.Direction.y == 0 )
	{
		if( r.Origin.y < low.y || r.Origin.y > high.y )
			return false;
	}
	else
	{
		l1.x = l2.x = 0;

		l1.y = -1;
		l2.y = 1;

		t1 = (low.y - r.Origin.y) / r.Direction.y;
		t2 = (high.y - r.Origin.y) / r.Direction.y;

		if( t1 > t2 )
		{
			std::swap(t1, t2);
			std::swap(l1, l2);
		}

		if( t1 > tn )
		{
			tn = t1;
			n = l1;
		}

		if( t2 < tf )
			tf = t2;

		if( tn > tf || tf < 0 )
			return false;
	}

	// z planes
	if( r.Direction.z == 0 )
	{
		if( r.Origin.z < low.z || r.Origin.z > high.z )
			return false;
	}
	else
	{
		l1.x = l2.x = 0;
		l1.y = l2.y = 0;

		l1.z = -1;
		l2.z = 1;

		t1 = (low.z - r.Origin.z) / r.Direction.z;
		t2 = (high.z - r.Origin.z) / r.Direction.z;

		if( t1 > t2 )
		{
			std::swap(t1, t2);
			std::swap(l1, l2);
		}

		if( t1 > tn )
		{
			tn = t1;
			n = l1;
		}

		if( t2 < tf )
			tf = t2;

		if( tn > tf || tf < 0 )
			return false;
	}

	p = r.Origin + tn * r.Direction;

	return true;
}
//=============================================================================================================
bool Box::Intersect(D3DXVECTOR3& p1, D3DXVECTOR3& p2, D3DXVECTOR3& n1, D3DXVECTOR3& n2, const Ray& r)
{
	D3DXVECTOR3 l1(0, 0, 0), l2(0, 0, 0);
	float tn = -FLT_MAX;
	float tf = FLT_MAX;
	float t1, t2;

	// x planes
	if( r.Direction.x == 0 )
	{
		if( r.Origin.x < low.x || r.Origin.x > high.x )
			return false;
	}
	else
	{
		l1.x = -1;
		l2.x = 1;

		t1 = (low.x - r.Origin.x) / r.Direction.x;
		t2 = (high.x - r.Origin.x) / r.Direction.x;

		if( t1 > t2 )
		{
			std::swap(t1, t2);
			std::swap(l1, l2);
		}

		if( t1 > tn )
		{
			tn = t1;
			n1 = l1;
		}

		if( t2 < tf )
		{
			tf = t2;
			n2 = l2;
		}

		if( tn > tf || tf < 0 )
			return false;
	}

	// y planes
	if( r.Direction.y == 0 )
	{
		if( r.Origin.y < low.y || r.Origin.y > high.y )
			return false;
	}
	else
	{
		l1.x = l2.x = 0;

		l1.y = -1;
		l2.y = 1;

		t1 = (low.y - r.Origin.y) / r.Direction.y;
		t2 = (high.y - r.Origin.y) / r.Direction.y;

		if( t1 > t2 )
		{
			std::swap(t1, t2);
			std::swap(l1, l2);
		}

		if( t1 > tn )
		{
			tn = t1;
			n1 = l1;
		}

		if( t2 < tf )
		{
			tf = t2;
			n2 = l2;
		}

		if( tn > tf || tf < 0 )
			return false;
	}

	// z planes
	if( r.Direction.z == 0 )
	{
		if( r.Origin.z < low.z || r.Origin.z > high.z )
			return false;
	}
	else
	{
		l1.x = l2.x = 0;
		l1.y = l2.y = 0;

		l1.z = -1;
		l2.z = 1;

		t1 = (low.z - r.Origin.z) / r.Direction.z;
		t2 = (high.z - r.Origin.z) / r.Direction.z;

		if( t1 > t2 )
		{
			std::swap(t1, t2);
			std::swap(l1, l2);
		}

		if( t1 > tn )
		{
			tn = t1;
			n1 = l1;
		}

		if( t2 < tf )
		{
			tf = t2;
			n2 = l2;
		}

		if( tn > tf || tf < 0 )
			return false;
	}

	p1 = r.Origin + tn * r.Direction;
	p2 = r.Origin + tf * r.Direction;

	return true;
}
//=============================================================================================================
