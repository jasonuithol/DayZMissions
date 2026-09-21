// Dev tool: scans north-south columns and logs surface types as z-runs to the
// script log, then shuts the server down. Used to find out how roads look to script.
void ProbeColumn(float x, float zMin, float zMax)
{
	string runType = "";
	float runStart = zMin;
	for (float z = zMin; z <= zMax; z += 1.0)
	{
		string t3d;
		float y = GetGame().SurfaceY(x, z);
		GetGame().SurfaceGetType3D(x, y + 20, z, t3d);
		float dy = GetGame().SurfaceRoadY(x, z) - y;
		string key = t3d;
		if (dy > 0.02)
			key = key + "+raised";
		if (key != runType)
		{
			if (runType != "")
				Print("[RoadProbe] x=" + x + " z=" + runStart + ".." + (z - 1) + " " + runType);
			runType = key;
			runStart = z;
		}
	}
	Print("[RoadProbe] x=" + x + " z=" + runStart + ".." + zMax + " " + runType);
}

void main()
{
	Hive ce = CreateHive();
	if (ce)
		ce.InitOffline();

	for (float x = 7000; x <= 10000; x += 250)
		ProbeColumn(x, 2000, 3600);

	Print("[RoadProbe] done");
	GetGame().RequestExit(0);
}

class CustomMission: MissionServer {};

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}
