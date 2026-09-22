// Bike Spawns
// Vanilla Chernarus with motorbikes everywhere: mopeds (Motorbike_01) outside schools,
// police stations, shops, hospitals and petrol stations in the towns; dirt bikes
// (Motorbike_02) at deer stands, feed shacks, hunting camps and fire stations. All of
// them ready to ride. The where and what comes from spots.json, which
// tools/bike_spots.py generates from bikes.json and the map data.
//
// Spawning waits until the world has settled (vehicles created in the first seconds
// after startup end up with no fuel) and stops at MAX_BIKES to keep the server load sane.
#include "lib/RoadFinder.c"
#include "lib/Spawner.c"
#include "lib/Motorbikes.c"
#include "lib/Placement.c"

class BikeSpot
{
	string building;
	string types;   // "A|B|C" picks one at random
	int count;
	ref array<float> pos = new array<float>();
	float heading;
}

class BikeSpots
{
	ref array<ref BikeSpot> spots = new array<ref BikeSpot>();
}

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

class CustomMission: MissionServer
{
	static const float ROAD_SEARCH = 40.0;   // how far from the building to look for a road
	static const float ROAD_EDGE = 1.2;      // bikes park this far in from the edge of the road
	static const float SPACING = 2.0;
	static const float KEEP_AWAY = 1.8;      // from any other spawned bike
	static const vector BIKE_SIZE = "1.0 1.2 2.4"; // clearance box: width, height, length
	static const int MAX_BIKES = 170;
	// Vehicles created in the first seconds after startup end up with no fuel and can't
	// be refilled (the world is still initialising), so spawning waits, then goes in batches.
	static const int START_DELAY_MS = 20000;
	static const int BATCH = 5;
	static const int BATCH_MS = 250;

	protected string m_Path;
	protected ref BikeSpots m_Spots;
	protected int m_NextSpot;
	protected int m_Limit;
	protected ref array<MotorbikeScript> m_Bikes = new array<MotorbikeScript>();
	protected ref array<vector> m_Taken = new array<vector>();
	protected int m_OnRoad;
	protected int m_NoSpace;
	protected int m_Raised;
	protected int m_Failed;

	void CustomMission(string path)
	{
		m_Path = path.Substring(0, path.LastIndexOf("/"));
	}

	override void OnInit()
	{
		super.OnInit();

		StartSpawning();
	}

	void StartSpawning()
	{
		string error;
		if (!JsonFileLoader<BikeSpots>.LoadFile(m_Path + "/spots.json", m_Spots, error))
		{
			Print("[BikeSpawns] " + error);
			return;
		}

		// -bikelimit=N: stop after N bikes (testing)
		string limitValue;
		m_Limit = MAX_BIKES;
		if (GetGame().CommandlineGetParam("bikelimit", limitValue))
			m_Limit = limitValue.ToInt();

		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(BeginBatches, START_DELAY_MS, false);
	}

	void BeginBatches()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SpawnBatch, BATCH_MS, true);
	}

	void SpawnBatch()
	{
		for (int n = 0; n < BATCH && m_NextSpot < m_Spots.spots.Count() && m_Bikes.Count() < m_Limit; n++)
		{
			SpawnAt(m_Spots.spots[m_NextSpot]);
			m_NextSpot++;
		}

		if (m_NextSpot < m_Spots.spots.Count() && m_Bikes.Count() < m_Limit)
			return;

		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(SpawnBatch);
		Print("[BikeSpawns] " + m_Bikes.Count() + " bikes at " + m_NextSpot + " buildings (" + m_OnRoad + " on roads, " + m_Raised + " on roofs or platforms, " + m_NoSpace + " skipped for lack of space, " + m_Failed + " failed)");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TopUp, 3000, false);
		if (m_Bikes.Count() >= m_Limit)
			Print("[BikeSpawns] hit the " + m_Limit + " bike limit - spots.json has more than that");

		string value;
		if (GetGame().CommandlineGetParam("missiontest", value))
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestReport, 15000, false);
	}

	// The odd bike comes up empty and stays that way whatever you do to it. Those are
	// deleted and spawned again a few metres away, which does work.
	void TopUp()
	{
		int respawned = 0;
		for (int i = 0; i < m_Bikes.Count(); i++)
		{
			MotorbikeScript bike = m_Bikes[i];
			if (bike.GetFluidFraction(MotorbikeFluid.FUEL) > 0.99)
				continue;
			bike.Fill(MotorbikeFluid.FUEL, bike.GetFluidCapacity(MotorbikeFluid.FUEL));
			if (bike.GetFluidFraction(MotorbikeFluid.FUEL) > 0.99)
				continue;

			string type = bike.GetType();
			vector was = bike.GetPosition();
			float facing = bike.GetOrientation()[0];
			GetGame().ObjectDelete(bike);

			vector pos;
			float heading;
			if (!Placement.FindClear(was, facing, 3, 20, BIKE_SIZE, m_Taken, KEEP_AWAY, pos, heading, true))
				continue;
			MotorbikeScript again = Motorbikes.SpawnReady(type, pos, heading);
			if (!again)
				continue;
			m_Bikes[i] = again;
			m_Taken[i] = pos;
			respawned++;
			Print("[BikeSpawns] respawned an empty " + type + " from " + was + " to " + pos + ", fuel now " + again.GetFluidFraction(MotorbikeFluid.FUEL));
		}
		if (respawned > 0)
			Print("[BikeSpawns] respawned " + respawned + " empty bikes");
	}

	// Parks a spot's bikes side by side on the nearest road, on the building's side of
	// it. If there is no road, or the road is blocked, each bike gets its own clear
	// patch of ground near the building instead.
	void SpawnAt(BikeSpot spot)
	{
		vector anchor = Vector(spot.pos[0], spot.pos[1], spot.pos[2]);

		vector roadPos;
		float roadHeading, width;
		bool onRoad = RoadFinder.Locate(anchor, ROAD_SEARCH, roadPos, roadHeading, width);
		vector centre, along;
		if (onRoad)
		{
			// the side of the road the building is on
			vector side = RoadFinder.HeadingToDir(roadHeading + 90);
			if (vector.Dot(side, anchor - roadPos) < 0)
				side = side * -1;
			centre = roadPos + side * (width * 0.5 - ROAD_EDGE);
			along = RoadFinder.HeadingToDir(roadHeading);
			m_OnRoad++;
		}

		for (int i = 0; i < spot.count; i++)
		{
			vector pos;
			float heading;
			bool placed = false;

			if (onRoad)
			{
				// along the kerb, sliding further along if something is in the way
				for (int slide = 0; slide < 6 && !placed; slide++)
				{
					float offset = (i - (spot.count - 1) * 0.5 + slide) * SPACING;
					pos = centre + along * offset;
					pos[1] = GetGame().SurfaceRoadY(pos[0], pos[2]);
					heading = roadHeading;
					placed = Placement.IsClear(pos, heading, BIKE_SIZE) && !Placement.TooClose(pos, m_Taken, KEEP_AWAY);
				}
			}
			if (!placed)
				placed = Placement.FindClear(anchor, spot.heading, 5, 25, BIKE_SIZE, m_Taken, KEEP_AWAY, pos, heading);
			// nowhere on the ground: a roof, platform or floor will do, as long as it is clear
			if (!placed)
			{
				placed = Placement.FindClear(anchor, spot.heading, 3, 25, BIKE_SIZE, m_Taken, KEEP_AWAY, pos, heading, true);
				if (placed)
					m_Raised++;
			}
			if (!placed)
			{
				m_NoSpace++;
				continue;
			}

			MotorbikeScript bike = Motorbikes.SpawnReady(Roles_Choose(spot.types), pos, heading);
			if (bike)
			{
				m_Bikes.Insert(bike);
				m_Taken.Insert(pos);
			}
			else
				m_Failed++;
		}
	}

	// "A|B|C" -> one of them
	static string Roles_Choose(string spec)
	{
		array<string> options = new array<string>();
		spec.Split("|", options);
		if (options.Count() == 0)
			return spec;
		return options.GetRandomElement();
	}

	// -missiontest: sanity check the spawned bikes, then quit
	void TestReport()
	{
		int upright = 0;
		int fuelled = 0;
		int damaged = 0;
		int lost = 0;
		float closest = 1000;
		for (int i = 0; i < m_Bikes.Count(); i++)
		{
			MotorbikeScript bike = m_Bikes[i];
			vector ori = bike.GetOrientation();
			if (Math.AbsFloat(ori[1]) < 15 && Math.AbsFloat(ori[2]) < 15)
				upright++;
			if (bike.GetFluidFraction(MotorbikeFluid.FUEL) > 0.99)
				fuelled++;
			if (bike.GetHealth01("", "") < 0.99)
				damaged++;
			for (int j = i + 1; j < m_Bikes.Count(); j++)
			{
				float d = vector.Distance(bike.GetPosition(), m_Bikes[j].GetPosition());
				if (d < closest)
					closest = d;
			}
			vector p = bike.GetPosition();
			float above = p[1] - GetGame().SurfaceY(p[0], p[2]);
			if (above < -1 || above > 12)
				lost++;
			bool odd = above < -1 || above > 12 || bike.GetFluidFraction(MotorbikeFluid.FUEL) < 0.99 || Math.AbsFloat(ori[2]) > 15;
			if (odd)
				Print("[BikeSpawns] odd: " + bike.GetType() + " fuel " + bike.GetFluidFraction(MotorbikeFluid.FUEL) + " pos " + p + " above terrain " + above + " pitch " + ori[1] + " roll " + ori[2] + " spawned at " + m_Taken[i]);
		}
		Print("[BikeSpawns] test: " + m_Bikes.Count() + " bikes, " + upright + " upright, " + fuelled + " fuelled, " + damaged + " damaged, " + lost + " fallen through the map, closest pair " + closest + " m");
		GetGame().RequestExit(0);
	}
};

Mission CreateCustomMission(string path)
{
	return new CustomMission(path);
}
