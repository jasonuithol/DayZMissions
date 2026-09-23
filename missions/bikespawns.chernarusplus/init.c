// Bike Spawns
// Vanilla Chernarus with motorbikes everywhere: mopeds (Motorbike_01) outside schools,
// police stations, shops, hospitals and petrol stations in the towns; dirt bikes
// (Motorbike_02) at deer stands, feed shacks, hunting camps and fire stations. All of
// them ready to ride. The where and what comes from spots.json, which
// tools/vehicle_spots.py generates from vehicles.json and the map data; lib/VehicleSpots.c
// does the placing.
#include "lib/JsonFile.c"
#include "lib/RoadFinder.c"
#include "lib/Spawner.c"
#include "lib/Motorbikes.c"
#include "lib/Placement.c"
#include "lib/VehicleSpots.c"

void main()
{
	//INIT ECONOMY--------------------------------------
	Hive ce = CreateHive();
	if ( ce )
		ce.InitOffline();

	//DATE RESET AFTER ECONOMY INIT-------------------------
	int year, month, day, hour, minute;
	int reset_month = 9, reset_day = 20;
	GetGame().GetWorld().GetDate(year, month, day, hour, minute);

	if ((month == reset_month) && (day < reset_day))
	{
		GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
	}
	else
	{
		if ((month == reset_month + 1) && (day > reset_day))
		{
			GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
		}
		else
		{
			if ((month < reset_month) || (month > reset_month + 1))
			{
				GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
			}
		}
	}
}

class BikeFactory: VehicleFactory
{
	override EntityAI Spawn(string type, vector pos, float heading)
	{
		return Motorbikes.SpawnReady(type, pos, heading);
	}

	override float FuelFraction(EntityAI vehicle)
	{
		return MotorbikeScript.Cast(vehicle).GetFluidFraction(MotorbikeFluid.FUEL);
	}

	override void Refill(EntityAI vehicle)
	{
		MotorbikeScript bike = MotorbikeScript.Cast(vehicle);
		bike.Fill(MotorbikeFluid.FUEL, bike.GetFluidCapacity(MotorbikeFluid.FUEL));
	}

	override vector Size(string type)
	{
		return "1.0 1.2 2.4";
	}
}

class CustomMission: MissionServer
{
	static const int MAX_BIKES = 170; // to keep the server load sane

	protected string m_Path;
	protected ref VehicleSpots m_Bikes;

	// path is the mission script, "./mpmissions/<mission>/mission.c"; keep its folder
	void CustomMission(string path)
	{
		m_Path = path.Substring(0, path.LastIndexOf("/"));
	}

	override void OnInit()
	{
		super.OnInit();

		// -bikelimit=N: stop after N bikes (testing)
		string value;
		int limit = MAX_BIKES;
		if (GetGame().CommandlineGetParam("bikelimit", value))
			limit = value.ToInt();

		m_Bikes = new VehicleSpots("BikeSpawns", new BikeFactory());
		m_Bikes.Start(m_Path + "/spots.json", limit);

		if (GetGame().CommandlineGetParam("missiontest", value))
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestReport, 5000, true);
	}

	// -missiontest: once spawning has finished, report and quit
	void TestReport()
	{
		if (!m_Bikes.IsDone())
			return;
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(TestReport);
		m_Bikes.Report();
		GetGame().RequestExit(0);
	}
};

Mission CreateCustomMission(string path)
{
	return new CustomMission(path);
}
