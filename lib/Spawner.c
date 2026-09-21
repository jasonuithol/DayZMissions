// Helpers for placing vehicles and loose items from mission scripts.
class Spawner
{
	static const float LIFETIME = 3888000; // 45 days, same as vanilla vehicles

	// Spawns a ready-to-ride motorbike: universal parts, the given attachments
	// (wheels, shields...) and a full tank. heading is in degrees, 0 = north.
	static MotorbikeScript Motorbike(string type, vector pos, float heading, array<string> attachments)
	{
		MotorbikeScript bike = MotorbikeScript.Cast(GetGame().CreateObjectEx(type, pos, ECE_PLACE_ON_SURFACE));
		if (!bike)
		{
			Print("[Spawner] failed to create " + type);
			return null;
		}

		bike.SetOrientation(Vector(heading, 0, 0));
		bike.SetLifetime(LIFETIME);

		bike.GetInventory().CreateInInventory("HeadlightH7");
		if (bike.IsVitalSparkPlug())
			bike.GetInventory().CreateInInventory("SparkPlug");
		if (bike.NeedElectricitySourceDevice())
			bike.GetInventory().CreateInInventory(bike.GetElectricitySourceDeviceType());

		foreach (string attachment : attachments)
		{
			if (!bike.GetInventory().CreateInInventory(attachment))
				Print("[Spawner] " + type + ": could not attach " + attachment);
		}

		bike.Fill(MotorbikeFluid.FUEL, bike.GetFluidCapacity(MotorbikeFluid.FUEL));
		return bike;
	}

	// Drops a single item on the ground at pos.
	static EntityAI GroundItem(string type, vector pos, float heading)
	{
		EntityAI item = EntityAI.Cast(GetGame().CreateObjectEx(type, pos, ECE_PLACE_ON_SURFACE));
		if (!item)
		{
			Print("[Spawner] failed to create " + type);
			return null;
		}

		item.SetOrientation(Vector(heading, 0, 0));
		item.SetLifetime(LIFETIME);
		return item;
	}

	// Lays items out on the ground in a grid of `columns` columns centred on pos;
	// rows run along heading, columns across it. Returns the items in the order given.
	static array<EntityAI> GroundPile(array<string> types, vector pos, float heading, int columns, float spacing)
	{
		vector fwd = RoadFinder.HeadingToDir(heading);
		vector side = RoadFinder.HeadingToDir(heading + 90);
		int rows = (types.Count() + columns - 1) / columns;
		array<EntityAI> items = new array<EntityAI>();

		for (int i = 0; i < types.Count(); i++)
		{
			int r = i / columns;
			int c = i - r * columns;
			float col = c - (columns - 1) * 0.5;
			float row = r - (rows - 1) * 0.5;
			vector p = pos + side * (col * spacing) + fwd * (row * spacing);
			items.Insert(GroundItem(types[i], RoadFinder.OnSurface(p[0], p[2]), heading));
		}
		return items;
	}
}
