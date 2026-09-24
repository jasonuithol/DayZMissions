// Finding clear ground to put things on.
class Placement
{
	static const float MAX_SLOPE = 0.85; // cosine of the steepest acceptable slope (~32 deg)

	// Nothing solid (buildings, walls, trees, other vehicles) in a box of `size`
	// (x width, y height, z length) standing at pos, which must be dry and not too
	// steep. Unless allowRaised, pos must also be on the terrain or a road rather than
	// a roof, platform or floor.
	static bool IsClear(vector pos, float heading, vector size, bool allowRaised = false)
	{
		if (GetGame().SurfaceIsSea(pos[0], pos[2]) || GetGame().SurfaceIsPond(pos[0], pos[2]))
			return false;
		if (GetGame().SurfaceGetNormal(pos[0], pos[2])[1] < MAX_SLOPE)
			return false;
		// roads sit within a few cm of the terrain; anything higher is a roof or a floor
		if (!allowRaised && pos[1] - GetGame().SurfaceY(pos[0], pos[2]) > 0.3)
			return false;

		vector centre = pos;
		centre[1] = centre[1] + size[1] * 0.5 + 0.1;
		array<Object> excluded = new array<Object>();
		return !GetGame().IsBoxCollidingGeometry(centre, Vector(heading, 0, 0), size, ObjIntersect.View, ObjIntersect.Geom, excluded);
	}

	// Searches rings around anchor, from minRadius out to maxRadius, for clear ground,
	// starting in the direction of `preferHeading`. Nothing in `taken` may be closer
	// than `keepAway`. On success pos is on the ground and heading faces the anchor.
	// With allowRaised, roofs and platforms count too (the bike lands on top of them).
	static bool FindClear(vector anchor, float preferHeading, float minRadius, float maxRadius, vector size, array<vector> taken, float keepAway, out vector pos, out float heading, bool allowRaised = false)
	{
		for (float r = minRadius; r <= maxRadius; r += 2.0)
		{
			for (int step = 0; step < 12; step++)
			{
				// alternate left and right of the preferred direction
				float turn = ((step + 1) / 2) * 30;
				if (step % 2 == 1)
					turn = -turn;
				float dirHeading = preferHeading + turn;
				vector dir = RoadFinder.HeadingToDir(dirHeading);

				vector candidate = anchor + dir * r;
				if (allowRaised)
					candidate[1] = GetGame().SurfaceRoadY(candidate[0], candidate[2]);
				else
					candidate[1] = GetGame().SurfaceY(candidate[0], candidate[2]);
				float faceHeading = dirHeading + 180;

				if (!IsClear(candidate, faceHeading, size, allowRaised))
					continue;
				if (TooClose(candidate, taken, keepAway))
					continue;

				pos = candidate;
				heading = faceHeading;
				return true;
			}
		}
		return false;
	}

	// A spot on top of a building near anchor: at least minHeight above the terrain and
	// clear. Roofs are found by sampling around the anchor from the middle outwards, so
	// the building the anchor belongs to is tried first.
	static bool FindRoof(vector anchor, float maxRadius, float minHeight, vector size, array<vector> taken, float keepAway, out vector pos, out float heading)
	{
		for (float r = 0; r <= maxRadius; r += 2.0)
		{
			for (int step = 0; step < 12; step++)
			{
				float dirHeading = step * 30;
				vector candidate = anchor + HeadingToDir(dirHeading) * r;
				float terrain = GetGame().SurfaceY(candidate[0], candidate[2]);
				candidate[1] = GetGame().SurfaceRoadY(candidate[0], candidate[2]);
				if (candidate[1] - terrain < minHeight)
					continue;
				if (!IsClear(candidate, dirHeading, size, true))
					continue;
				if (TooClose(candidate, taken, keepAway))
					continue;

				pos = candidate;
				heading = dirHeading;
				return true;
				if (r == 0)
					break;
			}
		}
		return false;
	}

	static vector HeadingToDir(float heading)
	{
		return RoadFinder.HeadingToDir(heading);
	}

	// Any pond, river or sea within `radius` of pos (rings of samples).
	static bool WaterWithin(vector pos, float radius)
	{
		for (float r = 0; r <= radius; r += 15)
		{
			for (int step = 0; step < 16; step++)
			{
				vector p = pos + HeadingToDir(step * 22.5) * r;
				if (GetGame().SurfaceIsSea(p[0], p[2]) || GetGame().SurfaceIsPond(p[0], p[2]))
					return true;
			}
		}
		return false;
	}

	// A patch of open sea near anchor big enough for a ship of `length`: the point and
	// a ring of 0.8 x length around it are all sea, the seabed at least 12 m down in the
	// middle and 6 m at the ring. Searches from minRadius to maxRadius away, in every
	// direction, nearest first. pos is at sea level.
	static bool FindOpenSea(vector anchor, float minRadius, float maxRadius, float length, out vector pos)
	{
		float seaLevel = GetGame().SurfaceGetSeaLevel();
		for (float r = minRadius; r <= maxRadius; r += 50)
		{
			for (int step = 0; step < 16; step++)
			{
				vector p = anchor + HeadingToDir(step * 22.5) * r;
				if (!OpenSeaAt(p, length * 0.8))
					continue;
				pos = Vector(p[0], seaLevel, p[2]);
				return true;
			}
		}
		return false;
	}

	static bool OpenSeaAt(vector p, float ring)
	{
		float seaLevel = GetGame().SurfaceGetSeaLevel();
		if (!GetGame().SurfaceIsSea(p[0], p[2]) || GetGame().SurfaceY(p[0], p[2]) > seaLevel - 12)
			return false;
		for (int step = 0; step < 16; step++)
		{
			vector q = p + HeadingToDir(step * 22.5) * ring;
			if (!GetGame().SurfaceIsSea(q[0], q[2]) || GetGame().SurfaceY(q[0], q[2]) > seaLevel - 6)
				return false;
		}
		return true;
	}

	static bool TooClose(vector pos, array<vector> taken, float keepAway)
	{
		for (int i = 0; i < taken.Count(); i++)
		{
			if (vector.Distance(pos, taken[i]) < keepAway)
				return true;
		}
		return false;
	}
}
