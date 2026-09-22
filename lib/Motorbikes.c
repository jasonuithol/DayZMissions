// Vanilla motorbikes (DayZ 1.30+).
class Motorbikes
{
	// Wheels for every bike, plus the colour matched shields on the Motorbike_02.
	static array<string> KitFor(string type)
	{
		string model = type.Substring(0, 12); // "Motorbike_0x"
		array<string> kit = new array<string>();
		kit.Insert(model + "_Wheel_1");
		kit.Insert(model + "_Wheel_2");

		if (model == "Motorbike_02")
		{
			string colour = type.Substring(13, type.Length() - 13);
			kit.Insert(model + "_ShieldFront_" + colour);
			kit.Insert(model + "_ShieldLeft_" + colour);
			kit.Insert(model + "_ShieldRight_" + colour);
		}
		return kit;
	}

	// A ready-to-ride bike of its usual kit.
	static MotorbikeScript SpawnReady(string type, vector pos, float heading)
	{
		return Spawn(type, pos, heading, KitFor(type));
	}

	// Spawns a ready-to-ride motorbike: universal parts, the given attachments
	// (wheels, shields...) and a full tank. heading is in degrees, 0 = north.
	static MotorbikeScript Spawn(string type, vector pos, float heading, array<string> attachments)
	{
		MotorbikeScript bike = MotorbikeScript.Cast(GetGame().CreateObjectEx(type, pos, ECE_PLACE_ON_SURFACE | ECE_SETUP));
		if (!bike)
		{
			Print("[Motorbikes] failed to create " + type);
			return null;
		}

		bike.SetOrientation(Vector(heading, 0, 0));
		bike.SetLifetime(Spawner.LIFETIME);

		bike.GetInventory().CreateInInventory("HeadlightH7");
		if (bike.IsVitalSparkPlug())
			bike.GetInventory().CreateInInventory("SparkPlug");
		if (bike.NeedElectricitySourceDevice())
			bike.GetInventory().CreateInInventory(bike.GetElectricitySourceDeviceType());

		foreach (string attachment : attachments)
		{
			if (!bike.GetInventory().CreateInInventory(attachment))
				Print("[Motorbikes] " + type + ": could not attach " + attachment);
		}

		bike.Fill(MotorbikeFluid.FUEL, bike.GetFluidCapacity(MotorbikeFluid.FUEL));
		return bike;
	}
}
