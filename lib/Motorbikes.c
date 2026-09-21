// Vanilla motorbikes (DayZ 1.30+).
class Motorbikes
{
	// Spawns a ready-to-ride motorbike: universal parts, the given attachments
	// (wheels, shields...) and a full tank. heading is in degrees, 0 = north.
	static MotorbikeScript Spawn(string type, vector pos, float heading, array<string> attachments)
	{
		MotorbikeScript bike = MotorbikeScript.Cast(GetGame().CreateObjectEx(type, pos, ECE_PLACE_ON_SURFACE));
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
