// Dev tool: lists map objects around a point whose class or model name matches a
// filter, to find things that aren't in mapgrouppos.xml (helipads, decals...).
//   -probe=x,z,radius,filter   e.g. -probe=6500,2900,1500,heli
void main()
{
	Hive ce = CreateHive();
	if (ce)
		ce.InitOffline();
}

class CustomMission: MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Probe, 8000, false);
	}

	// -surf=x,z,radius,step: ASCII map of surface types (one letter per type, listed after)
	void SurfaceMap()
	{
		string spec;
		if (!GetGame().CommandlineGetParam("surf", spec))
			return;
		array<string> parts = new array<string>();
		spec.Split(",", parts);
		float cx = parts[0].ToFloat();
		float cz = parts[1].ToFloat();
		float radius = parts[2].ToFloat();
		float step = parts[3].ToFloat();

		string letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		array<string> types = new array<string>();
		for (float z = cz + radius; z >= cz - radius; z -= step)
		{
			string row = "";
			for (float x = cx - radius; x <= cx + radius; x += step)
			{
				string type;
				float y = GetGame().SurfaceY(x, z);
				GetGame().SurfaceGetType3D(x, y + 30, z, type);
				float raised = GetGame().SurfaceRoadY(x, z) - y;
				int idx = types.Find(type);
				if (idx < 0)
				{
					types.Insert(type);
					idx = types.Count() - 1;
				}
				string ch = letters.Get(idx);
				if (raised > 0.3)
					ch = "#";
				row = row + ch;
			}
			Print("[SurfMap] z=" + z + " " + row);
		}
		for (int t = 0; t < types.Count(); t++)
			Print("[SurfMap] " + letters.Get(t) + " = " + types[t]);
		Print("[SurfMap] x from " + (cx - radius) + " to " + (cx + radius) + " step " + step + "; # = raised (building/platform)");
		GetGame().RequestExit(0);
	}

	// -ray=x,z: what a ray from the sky hits at that point
	void Ray()
	{
		string spec;
		if (!GetGame().CommandlineGetParam("ray", spec))
			return;
		array<string> parts = new array<string>();
		spec.Split(",", parts);
		float x = parts[0].ToFloat();
		float z = parts[1].ToFloat();
		float terrain = GetGame().SurfaceY(x, z);

		vector hitPos;
		vector hitNormal;
		int component;
		Object hitObject;
		bool hit = DayZPhysics.RaycastRV(Vector(x, terrain + 50, z), Vector(x, terrain - 5, z), hitPos, hitNormal, component, null, null, null, false, false, ObjIntersectView);
		string type;
		GetGame().SurfaceGetType3D(x, terrain + 30, z, type);
		Print("[Ray] at " + x + "," + z + " terrain " + terrain + " road-surface " + GetGame().SurfaceRoadY(x, z) + " surface type " + type);
		if (hit)
		{
			set<Object> objects = new set<Object>();
			DayZPhysics.RaycastRV(Vector(x, terrain + 50, z), Vector(x, terrain - 5, z), hitPos, hitNormal, component, objects, null, null, false, false, ObjIntersectView);
			for (int i = 0; i < objects.Count(); i++)
				Print("[Ray] hit " + objects[i].GetType() + " model " + objects[i].GetModelName() + " at " + objects[i].GetPosition() + " ori " + objects[i].GetOrientation());
			Print("[Ray] first hit at " + hitPos + " normal " + hitNormal + " component " + component);
		}
		else
			Print("[Ray] nothing hit");
		GetGame().RequestExit(0);
	}

	void Probe()
	{
		Ray();
		SurfaceMap();
		string spec = "6500,2900,1500,heli";
		GetGame().CommandlineGetParam("probe", spec);
		array<string> parts = new array<string>();
		spec.Split(",", parts);
		vector centre = Vector(parts[0].ToFloat(), 0, parts[2].ToFloat() * 0 + parts[1].ToFloat());
		float radius = parts[2].ToFloat();
		string filter = parts[3];
		filter.ToLower();

		array<Object> objects = new array<Object>();
		GetGame().GetObjectsAtPosition(centre, radius, objects, null);
		int shown = 0;
		map<string, int> counts = new map<string, int>();
		for (int i = 0; i < objects.Count(); i++)
		{
			string type = objects[i].GetType();
			string model = objects[i].GetModelName();
			string hay = type + " " + model;
			hay.ToLower();
			if (filter != "" && hay.IndexOf(filter) < 0)
				continue;
			string key = type + " model " + model;
			int seen = 0;
			counts.Find(key, seen);
			counts.Set(key, seen + 1);
			if (seen < 3)
				Print("[ObjProbe] " + key + " pos " + objects[i].GetPosition() + " ori " + objects[i].GetOrientation());
			shown++;
		}
		for (int k = 0; k < counts.Count(); k++)
			Print("[ObjProbe] count " + counts.GetElement(k) + " x " + counts.GetKey(k));
		Print("[ObjProbe] " + objects.Count() + " objects within " + radius + " m of " + centre + ", " + shown + " matching '" + filter + "'");
		GetGame().RequestExit(0);
	}
};

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}
