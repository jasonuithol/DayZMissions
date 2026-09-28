// Spawns ready-to-use vehicles at a list of spots (spots.json, made by
// tools/vehicle_spots.py). The mission supplies a VehicleFactory that knows how to
// create and fuel its kind of vehicle; this class does the where and when:
//
//  - waits START_DELAY_MS after init (vehicles created in the first seconds after
//    server start end up with no fuel and can't be filled) and spawns in batches
//  - an "exact" spot (a vanilla vehicle spawn point) is used as is when clear;
//    otherwise a vehicle parks at the kerb of the nearest road on the building's
//    side, or on clear ground near the building, or on a roof / platform / floor
//  - a few seconds later, vehicles that came up empty or dropped through a floor
//    are respawned on open ground, then on the building's roof, then removed
//  - persistence: a spot that still has one of its vehicle types parked at it (loaded
//    from storage) is left alone, so a restart only fills the spots that were emptied

class VehicleSpot
{
	string building;
	string types;    // "A|B|C" picks one at random
	int count;
	ref array<float> pos = new array<float>();
	float heading;
	bool exact;      // pos/heading are a proper parking spot, try them first
	float nearWater; // only use the spot if a pond, river or the sea is within this many metres
	bool offshore;   // a ship: park it out at sea off this spot instead of on land
}

class VehicleSpotList
{
	ref array<ref VehicleSpot> spots = new array<ref VehicleSpot>();
}

// What the mission knows about its vehicles.
class VehicleFactory
{
	EntityAI Spawn(string type, vector pos, float heading) { return null; }
	float FuelFraction(EntityAI vehicle) { return 1; }
	void Refill(EntityAI vehicle) {}
	// clearance box: width, height, length
	vector Size(string type) { return "2.0 1.8 4.6"; }
	// every vehicle already on the server when spawning starts (loaded from persistence);
	// a mission can't enumerate entities on its own, so this comes from the mods it runs
	// with (Expansion keeps a list). Empty means nothing is kept and every spot is filled.
	array<EntityAI> Existing() { return new array<EntityAI>(); }
}

// Helpers over the list of vehicles the factory says already exist (see Existing()).
class VehicleRegistry
{
	static int CountOfType(array<EntityAI> all, string type)
	{
		int n = 0;
		for (int i = 0; i < all.Count(); i++)
			if (all[i] && all[i].GetType() == type)
				n++;
		return n;
	}

	// vehicles whose type is one of "A|B|C" within radius of pos (height ignored)
	static array<EntityAI> Near(array<EntityAI> all, string types, vector pos, float radius)
	{
		array<EntityAI> found = new array<EntityAI>();
		array<string> options = new array<string>();
		types.Split("|", options);
		for (int i = 0; i < all.Count(); i++)
		{
			EntityAI vehicle = all[i];
			if (!vehicle || options.Find(vehicle.GetType()) < 0)
				continue;
			vector at = vehicle.GetPosition();
			if (vector.Distance(Vector(at[0], 0, at[2]), Vector(pos[0], 0, pos[2])) <= radius)
				found.Insert(vehicle);
		}
		return found;
	}
}

class VehicleSpots
{
	static const float ROAD_SEARCH = 40.0;   // how far from the building to look for a road
	static const float ROAD_EDGE = 1.2;      // vehicles park this far in from the edge of the road
	static const float KEEP_AWAY = 1.8;      // gap between vehicles
	static const int START_DELAY_MS = 20000;
	static const int BATCH = 5;              // spots per tick
	static const int BATCH_MS = 250;
	static const int SETTLE_MS = 3000;
	static const float MAX_TILT = 10;        // degrees between the vehicle's up and the ground's normal after settling; more means it sits on something
	static const float KEPT_EXACT = 10;      // a vehicle this close to its own spawn point still counts as parked there
	static const float KEPT_NEAR = 150;      // ... or this close to the building it was placed at (a kerb spot can be well down the road)
	static const float KEPT_OFFSHORE = 1600; // ... or this far out to sea off the port

	protected string m_Tag;
	protected ref VehicleFactory m_Factory;
	protected ref VehicleSpotList m_Spots;
	protected int m_NextSpot;
	protected int m_Limit;
	protected int m_TopUpRound;

	protected ref array<EntityAI> m_Vehicles = new array<EntityAI>();
	protected ref array<EntityAI> m_Kept = new array<EntityAI>();     // loaded from persistence, left as they are
	protected ref array<EntityAI> m_Existing = new array<EntityAI>(); // everything on the server before spawning
	protected ref array<vector> m_Taken = new array<vector>();    // where every vehicle (spawned or kept) stands, for spacing
	protected ref array<vector> m_Placed = new array<vector>();   // where each spawned vehicle was put
	protected ref array<vector> m_Anchors = new array<vector>();  // the building each spawned vehicle belongs to
	protected int m_Exact;
	protected int m_OnRoad;
	protected int m_Raised;
	protected int m_NoSpace;
	protected int m_NoWater;
	protected int m_Offshore;
	protected int m_Failed;
	protected bool m_Done;

	void VehicleSpots(string tag, VehicleFactory factory)
	{
		m_Tag = "[" + tag + "] ";
		m_Factory = factory;
	}

	// the ones spawned this boot and the ones kept from persistence
	array<EntityAI> Vehicles()
	{
		array<EntityAI> all = new array<EntityAI>();
		for (int i = 0; i < m_Vehicles.Count(); i++)
			all.Insert(m_Vehicles[i]);
		for (int k = 0; k < m_Kept.Count(); k++)
			all.Insert(m_Kept[k]);
		return all;
	}
	int Total() { return m_Vehicles.Count() + m_Kept.Count(); }
	bool IsDone() { return m_Done; }

	// Loads path and starts spawning up to `limit` vehicles.
	bool Start(string path, int limit)
	{
		string text, error;
		m_Spots = new VehicleSpotList();
		if (!JsonFile.Read(path, text, error) || !(new JsonSerializer()).ReadFromString(m_Spots, text, error))
		{
			Print(m_Tag + path + ": " + error);
			m_Done = true;
			return false;
		}
		m_Limit = limit;
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(BeginBatches, START_DELAY_MS, false);
		return true;
	}

	void BeginBatches()
	{
		m_Existing = m_Factory.Existing();
		Print(m_Tag + m_Existing.Count() + " vehicles on the server before spawning");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SpawnBatch, BATCH_MS, true);
	}

	void SpawnBatch()
	{
		for (int n = 0; n < BATCH && m_NextSpot < m_Spots.spots.Count() && Total() < m_Limit; n++)
		{
			SpawnAt(m_Spots.spots[m_NextSpot]);
			m_NextSpot++;
		}

		if (m_NextSpot < m_Spots.spots.Count() && Total() < m_Limit)
			return;

		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(SpawnBatch);
		Print(m_Tag + m_Vehicles.Count() + " vehicles spawned at " + m_NextSpot + " spots, " + m_Kept.Count() + " kept from persistence (" + m_Exact + " on their own spot, " + m_OnRoad + " at a kerb, " + m_Raised + " on roofs or floors, " + m_Offshore + " at sea, " + m_NoSpace + " skipped for lack of space, " + m_NoWater + " skipped for lack of water, " + m_Failed + " failed)");
		if (Total() >= m_Limit && m_NextSpot < m_Spots.spots.Count())
			Print(m_Tag + "hit the " + m_Limit + " vehicle limit - spots.json has more than that");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TopUp, SETTLE_MS, false);
	}

	void SpawnAt(VehicleSpot spot)
	{
		vector anchor = Vector(spot.pos[0], spot.pos[1], spot.pos[2]);
		string type = Choose(spot.types);
		vector size = m_Factory.Size(type);

		// still occupied from before the restart: keep what is there
		float keptRadius = KEPT_NEAR;
		if (spot.exact)
			keptRadius = KEPT_EXACT;
		if (spot.offshore)
			keptRadius = KEPT_OFFSHORE;
		array<EntityAI> parked = VehicleRegistry.Near(m_Existing, spot.types, anchor, keptRadius);
		int wanted = spot.count;
		for (int p = 0; p < parked.Count() && wanted > 0; p++)
		{
			if (m_Kept.Find(parked[p]) >= 0)
				continue;
			m_Kept.Insert(parked[p]);
			m_Taken.Insert(parked[p].GetPosition());
			wanted--;
		}
		if (wanted <= 0)
			return;

		if (spot.nearWater > 0 && !Placement.WaterWithin(anchor, spot.nearWater))
		{
			m_NoWater++;
			return;
		}
		if (spot.offshore)
		{
			vector sea;
			if (!Placement.FindOpenSea(anchor, 250, 1500, size[2], sea))
			{
				m_NoWater++;
				return;
			}
			EntityAI ship = m_Factory.Spawn(type, sea, vector.Direction(sea, anchor).VectorToAngles()[0]);
			if (ship)
			{
				m_Vehicles.Insert(ship);
				m_Taken.Insert(sea);
				m_Placed.Insert(sea);
				m_Anchors.Insert(anchor);
				m_Offshore++;
				Print(m_Tag + type + " at sea off " + anchor + " at " + sea + ", " + vector.Distance(anchor, sea) + " m out");
			}
			else
				m_Failed++;
			return;
		}

		vector roadPos, centre, along;
		float roadHeading, width;
		bool onRoad = false;
		if (!spot.exact)
		{
			onRoad = RoadFinder.Locate(anchor, ROAD_SEARCH, roadPos, roadHeading, width);
			if (onRoad)
			{
				// the side of the road the building is on
				vector side = RoadFinder.HeadingToDir(roadHeading + 90);
				if (vector.Dot(side, anchor - roadPos) < 0)
					side = side * -1;
				centre = roadPos + side * (width * 0.5 - ROAD_EDGE);
				along = RoadFinder.HeadingToDir(roadHeading);
			}
		}

		for (int i = 0; i < wanted; i++)
		{
			vector pos;
			float heading;
			bool placed = false;

			if (spot.exact)
			{
				pos = anchor;
				pos[1] = GetGame().SurfaceRoadY(pos[0], pos[2]);
				heading = spot.heading;
				placed = Placement.IsClear(pos, heading, size, true) && !Placement.TooClose(pos, m_Taken, KEEP_AWAY);
				if (placed)
					m_Exact++;
			}
			if (!placed && onRoad)
			{
				// along the kerb, sliding further along if something is in the way
				float spacing = size[2] + KEEP_AWAY;
				for (int slide = 0; slide < 6 && !placed; slide++)
				{
					float offset = (i - (wanted - 1) * 0.5 + slide) * spacing;
					pos = centre + along * offset;
					pos[1] = GetGame().SurfaceRoadY(pos[0], pos[2]);
					heading = roadHeading;
					placed = Placement.IsClear(pos, heading, size) && !Placement.TooClose(pos, m_Taken, KEEP_AWAY);
				}
				if (placed)
					m_OnRoad++;
			}
			if (!placed)
				placed = Placement.FindClear(anchor, spot.heading, 5, 25, size, m_Taken, KEEP_AWAY, pos, heading);
			// nowhere on the ground: a roof, platform or floor will do, as long as it is clear
			if (!placed)
			{
				placed = Placement.FindClear(anchor, spot.heading, 3, 25, size, m_Taken, KEEP_AWAY, pos, heading, true);
				if (placed)
					m_Raised++;
			}
			if (!placed)
			{
				m_NoSpace++;
				continue;
			}

			EntityAI vehicle = m_Factory.Spawn(type, pos, heading);
			if (vehicle)
			{
				m_Vehicles.Insert(vehicle);
				m_Taken.Insert(pos);
				m_Placed.Insert(pos);
				m_Anchors.Insert(anchor);
			}
			else
				m_Failed++;
		}
	}

	void TopUp()
	{
		m_TopUpRound++;
		int respawned = 0;
		for (int i = 0; i < m_Vehicles.Count(); i++)
		{
			EntityAI vehicle = m_Vehicles[i];
			string why = "";
			if (m_Factory.FuelFraction(vehicle) < 0.99)
			{
				m_Factory.Refill(vehicle);
				if (m_Factory.FuelFraction(vehicle) < 0.99)
					why = "came up empty";
			}
			bool onLand = !GetGame().SurfaceIsSea(m_Placed[i][0], m_Placed[i][2]);
			if (m_Placed[i][1] - vehicle.GetPosition()[1] > 2 && onLand)
				why = "fell through the floor";
			if (onLand)
			{
				// tilt relative to the slope it stands on, so a hillside is fine but a wreck under one end is not
				vector at = vehicle.GetPosition();
				float offSlope = Math.Acos(Math.Clamp(vector.Dot(vehicle.GetDirectionUp(), GetGame().SurfaceGetNormal(at[0], at[2])), -1, 1)) * Math.RAD2DEG;
				if (offSlope > MAX_TILT)
					why = "sits " + offSlope + " deg off the ground slope";
			}
			if (why == "")
				continue;

			string type = vehicle.GetType();
			vector was = m_Placed[i];
			float facing = vehicle.GetOrientation()[0];
			GetGame().ObjectDelete(vehicle);
			m_Taken.RemoveItem(was);

			vector pos;
			float heading;
			string where = "on open ground";
			bool found = false;
			if (m_TopUpRound == 1)
				found = Placement.FindClear(was, facing, 3, 60, m_Factory.Size(type), m_Taken, KEEP_AWAY, pos, heading);
			else if (m_TopUpRound == 2)
			{
				found = Placement.FindRoof(m_Anchors[i], 20, 2.5, m_Factory.Size(type), m_Taken, KEEP_AWAY, pos, heading);
				where = "on a roof";
				if (found)
					m_Raised++;
			}
			if (!found)
			{
				m_Vehicles.Remove(i);
				m_Placed.Remove(i);
				m_Anchors.Remove(i);
				i--;
				Print(m_Tag + "removed a " + type + " that " + why + " at " + was);
				continue;
			}
			EntityAI again = m_Factory.Spawn(type, pos, heading);
			if (!again)
				continue;
			m_Vehicles[i] = again;
			m_Placed[i] = pos;
			m_Taken.Insert(pos);
			respawned++;
			Print(m_Tag + "respawned a " + type + " that " + why + " from " + was + " to " + pos + " " + where);
		}
		if (respawned > 0)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TopUp, SETTLE_MS, false);
			return;
		}
		m_Done = true;
	}

	// -missiontest: sanity check the spawned vehicles
	void Report()
	{
		int upright = 0;
		int fuelled = 0;
		int damaged = 0;
		int lost = 0;
		float closest = 1000;
		for (int i = 0; i < m_Vehicles.Count(); i++)
		{
			EntityAI vehicle = m_Vehicles[i];
			vector ori = vehicle.GetOrientation();
			vector p = vehicle.GetPosition();
			float above = p[1] - GetGame().SurfaceY(p[0], p[2]);
			if (GetGame().SurfaceIsSea(p[0], p[2]))
				above = 0;
			if (Math.AbsFloat(ori[1]) < 15 && Math.AbsFloat(ori[2]) < 15)
				upright++;
			if (m_Factory.FuelFraction(vehicle) > 0.99)
				fuelled++;
			if (vehicle.GetHealth01("", "") < 0.99)
				damaged++;
			if (above < -3)
				lost++;
			for (int j = i + 1; j < m_Vehicles.Count(); j++)
			{
				float d = vector.Distance(p, m_Vehicles[j].GetPosition());
				if (d < closest)
					closest = d;
			}
			if (vehicle.GetType().IndexOf("Expansion") == 0)
				Print(m_Tag + "modded: " + vehicle.GetType() + " at " + p + " for building at " + m_Anchors[i] + " (" + vector.Distance(p, m_Anchors[i]) + " m away)");
			if (above < -3 || above > 3 || m_Factory.FuelFraction(vehicle) < 0.99 || Math.AbsFloat(ori[2]) > 15)
				Print(m_Tag + "odd: " + vehicle.GetType() + " fuel " + m_Factory.FuelFraction(vehicle) + " pos " + p + " above terrain " + above + " pitch " + ori[1] + " roll " + ori[2] + " spawned at " + m_Placed[i]);
		}
		Print(m_Tag + "test: " + m_Kept.Count() + " vehicles kept from persistence, " + m_Existing.Count() + " were on the server before spawning");
		Print(m_Tag + "test: " + m_Vehicles.Count() + " vehicles, " + upright + " upright, " + fuelled + " fuelled, " + damaged + " damaged, " + lost + " fallen through the map, closest pair " + closest + " m");
	}

	// "A|B|C" -> one of them
	static string Choose(string spec)
	{
		array<string> options = new array<string>();
		spec.Split("|", options);
		if (options.Count() == 0)
			return spec;
		return options.GetRandomElement();
	}
}
