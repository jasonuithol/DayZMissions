// Locates paved roads from script by sampling the surface type.
// Main roads report "asphalt_ext" and sit flush with the terrain; asphalt that is
// raised above the terrain (roofs, platforms, bridges) is ignored.
class RoadFinder
{
	static const string ROAD_SURFACE = "asphalt_ext";

	static bool IsRoad(float x, float z)
	{
		float y = GetGame().SurfaceY(x, z);
		string type;
		GetGame().SurfaceGetType3D(x, y + 20, z, type);
		if (type != ROAD_SURFACE)
			return false;

		return (GetGame().SurfaceRoadY(x, z) - y) < 0.05;
	}

	static vector OnSurface(float x, float z)
	{
		return Vector(x, GetGame().SurfaceRoadY(x, z), z);
	}

	// unit vector for a compass heading in degrees (0 = north/+z, clockwise)
	static vector HeadingToDir(float heading)
	{
		float rad = heading * Math.DEG2RAD;
		return Vector(Math.Sin(rad), 0, Math.Cos(rad));
	}

	// Searches outwards in rings from anchor for the closest road point.
	static bool FindNearest(vector anchor, float maxRadius, out vector pos)
	{
		if (IsRoad(anchor[0], anchor[2]))
		{
			pos = OnSurface(anchor[0], anchor[2]);
			return true;
		}

		for (float r = 2; r <= maxRadius; r += 2)
		{
			float step = 2.0 / r; // ~2m of arc per sample
			for (float a = 0; a < Math.PI2; a += step)
			{
				float x = anchor[0] + Math.Sin(a) * r;
				float z = anchor[2] + Math.Cos(a) * r;
				if (IsRoad(x, z))
				{
					pos = OnSurface(x, z);
					return true;
				}
			}
		}

		return false;
	}

	// how far the road continues from pos along dir
	static float Extent(vector pos, vector dir, float max, float step)
	{
		float d = 0;
		while (d < max)
		{
			float x = pos[0] + dir[0] * (d + step);
			float z = pos[2] + dir[2] * (d + step);
			if (!IsRoad(x, z))
				break;
			d += step;
		}
		return d;
	}

	// Heading (degrees, 0..180) along which the road runs through pos.
	static float FindHeading(vector pos)
	{
		float best = 0;
		float bestLen = -1;
		for (float h = 0; h < 180; h += 2)
		{
			vector dir = HeadingToDir(h);
			float len = Extent(pos, dir, 40, 0.5) + Extent(pos, dir * -1, 40, 0.5);
			if (len > bestLen)
			{
				bestLen = len;
				best = h;
			}
		}
		return best;
	}

	// Moves pos sideways onto the centre line of a road running along heading.
	static vector Centre(vector pos, float heading, out float width)
	{
		vector side = HeadingToDir(heading + 90);
		float right = Extent(pos, side, 15, 0.1);
		float left = Extent(pos, side * -1, 15, 0.1);
		width = left + right;

		float shift = (right - left) * 0.5;
		return OnSurface(pos[0] + side[0] * shift, pos[2] + side[2] * shift);
	}

	// Nearest road to anchor: centre-line position, heading and width.
	static bool Locate(vector anchor, float maxRadius, out vector pos, out float heading, out float width)
	{
		vector hit;
		if (!FindNearest(anchor, maxRadius, hit))
			return false;

		// first pass from wherever we hit the road (probably its edge), then refine from the centre
		heading = FindHeading(hit);
		pos = Centre(hit, heading, width);
		heading = FindHeading(pos);
		pos = Centre(pos, heading, width);
		return true;
	}
}
