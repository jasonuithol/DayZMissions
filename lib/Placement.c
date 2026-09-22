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
