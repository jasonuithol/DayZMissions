// Helpers for placing vehicles and loose items from mission scripts.
class Spawner
{
	static const float LIFETIME = 3888000; // 45 days, same as vanilla vehicles

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

	// Spawns a car-based vehicle (that includes DayZ Expansion helicopters) with the given
	// attachments and every fluid topped up. heading in degrees, 0 = north.
	static CarScript Vehicle(string type, vector pos, float heading, array<string> attachments)
	{
		CarScript vehicle = CarScript.Cast(GetGame().CreateObjectEx(type, pos, ECE_PLACE_ON_SURFACE));
		if (!vehicle)
		{
			Print("[Spawner] failed to create " + type);
			return null;
		}

		vehicle.SetOrientation(Vector(heading, 0, 0));
		vehicle.SetLifetime(LIFETIME);

		foreach (string attachment : attachments)
		{
			if (!vehicle.GetInventory().CreateInInventory(attachment))
				Print("[Spawner] " + type + ": could not attach " + attachment);
		}

		vehicle.Fill(CarFluid.FUEL, vehicle.GetFluidCapacity(CarFluid.FUEL));
		vehicle.Fill(CarFluid.OIL, vehicle.GetFluidCapacity(CarFluid.OIL));
		vehicle.Fill(CarFluid.COOLANT, vehicle.GetFluidCapacity(CarFluid.COOLANT));
		vehicle.Fill(CarFluid.BRAKE, vehicle.GetFluidCapacity(CarFluid.BRAKE));
		return vehicle;
	}

	// Creates attachments (optics, magazines, pouches...) on an item.
	static void Attach(EntityAI item, array<string> attachments)
	{
		if (!item)
			return;

		foreach (string attachment : attachments)
		{
			if (!item.GetInventory().CreateAttachment(attachment))
				Print("[Spawner] " + item.GetType() + ": could not attach " + attachment);
		}
	}

	// Places a static object (wreck, building...) on the ground. heading in degrees, 0 = north.
	static Object StaticObject(string type, vector pos, float heading)
	{
		Object obj = GetGame().CreateObjectEx(type, pos, ECE_PLACE_ON_SURFACE);
		if (!obj)
		{
			Print("[Spawner] failed to create " + type);
			return null;
		}

		obj.SetOrientation(Vector(heading, 0, 0));
		obj.PlaceOnSurface();
		return obj;
	}

	// Half of an object's footprint along its longest horizontal axis.
	static float HalfLength(Object obj)
	{
		vector minMax[2];
		obj.ClippingInfo(minMax);
		float x = Math.Max(Math.AbsFloat(minMax[0][0]), Math.AbsFloat(minMax[1][0]));
		float z = Math.Max(Math.AbsFloat(minMax[0][2]), Math.AbsFloat(minMax[1][2]));
		return Math.Max(x, z);
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
