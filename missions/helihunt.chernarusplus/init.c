// Heli Hunt (DayZ stable + DayZ Expansion)
// Hunters and hunted on the coast highway between Chernogorsk and Elektrozavodsk.
// A line-up of Honda dirt bikes with riding gear, and 30 m either side of it a
// helicopter, fuelled and ready to fly, with military clothing, an assault rifle and
// a sniper rifle laid out beside it. Players spawn behind the bikes.
#include "lib/RoadFinder.c"
#include "lib/Spawner.c"
#include "lib/Loadouts.c"

void main()
{
	//INIT ECONOMY--------------------------------------
	Hive ce = CreateHive();
	if ( ce )
		ce.InitOffline();

	// always start on a September morning
	GetGame().GetWorld().SetDate(2026, 9, 20, 9, 0);
}

class CustomMission: MissionServer
{
	// roughly the middle of the Cherno - Elektro coast road; the road itself is found from here
	static const vector ANCHOR = "8500 0 2796";
	static const float HELI_DISTANCE = 30.0; // from the anchor to each helicopter, along the road
	static const float GEAR_SIDE = 5.0;      // gear is laid out this far to the side of a helicopter

	static const float BIKE_SPACING = 2.0;
	static const float GEAR_DISTANCE = 2.0;  // from bike centre to the middle of its gear pile

	protected ref array<CarScript> m_Bikes = new array<CarScript>();
	protected ref array<CarScript> m_Helis = new array<CarScript>();
	protected vector m_PlayerSpawn;

	override void OnInit()
	{
		super.OnInit();

		SpawnScene();

		string value;
		if (GetGame().CommandlineGetParam("missiontest", value))
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestReport, 20000, false);
	}

	void SpawnScene()
	{
		vector roadPos;
		float roadHeading, roadWidth;
		if (!RoadFinder.Locate(ANCHOR, 300, roadPos, roadHeading, roadWidth))
		{
			Print("[HeliHunt] no road found near " + ANCHOR);
			return;
		}
		Print("[HeliHunt] road at " + roadPos + " heading " + roadHeading + " width " + roadWidth);

		SpawnBikes(roadPos, roadHeading);

		// Huey: front doors only, the back is left open for the door gunners
		array<string> hueyKit = {"ExpansionUh1hDoor_1_1", "ExpansionUh1hDoor_2_1"};
		SpawnHeli("ExpansionUh1h", hueyKit, roadPos, roadHeading, HELI_DISTANCE);

		// Little Bird: no doors at all
		array<string> mh6Kit = {};
		SpawnHeli("ExpansionMh6", mh6Kit, roadPos, roadHeading, -HELI_DISTANCE);
	}

	void SpawnHeli(string type, array<string> kit, vector roadPos, float roadHeading, float distance)
	{
		vector pos = roadPos;
		float heading = roadHeading;
		RoadFinder.Walk(pos, heading, distance);

		kit.Insert("ExpansionHydraulicHoses");
		kit.Insert("ExpansionIgniterPlug");
		kit.Insert("ExpansionHelicopterBattery");
		kit.Insert("HeadlightH7");

		CarScript heli = Spawner.Vehicle(type, pos, heading, kit);
		if (!heli)
			return;
		m_Helis.Insert(heli);
		Print("[HeliHunt] " + type + " at " + pos + " heading " + heading);

		Loadouts.Military(pos + RoadFinder.HeadingToDir(heading + 90) * GEAR_SIDE, heading);
	}

	// MBM Honda CRF450R, the handlebar variants: they need Survivor Animations for the riding
	// pose (the "MBM_HondaCRF450_W_<Colour>" ones have a steering wheel and work without it).
	// Side by side along the centre line, facing across the road, gear in front of each.
	void SpawnBikes(vector roadPos, float roadHeading)
	{
		array<string> colours = {"Red", "Green", "Blue", "Pink", "Yellow", "Black"};
		array<string> helmets = {"DirtBikeHelmet_Red", "DirtBikeHelmet_Green", "DirtBikeHelmet_Blue", "DirtBikeHelmet_Chernarus", "DirtBikeHelmet_Khaki", "DirtBikeHelmet_Black"};

		vector along = RoadFinder.HeadingToDir(roadHeading);
		float bikeHeading = roadHeading + 90;
		vector bikeFwd = RoadFinder.HeadingToDir(bikeHeading);

		for (int i = 0; i < colours.Count(); i++)
		{
			float offset = (i - (colours.Count() - 1) * 0.5) * BIKE_SPACING;
			float width;
			vector pos = RoadFinder.Centre(roadPos + along * offset, roadHeading, width);

			array<string> kit = {"MBM_HondaCRF450_Wheel", "MBM_HondaCRF450_Wheel", "CarBattery", "SparkPlug", "MBM_CRF450_Radiator"};
			CarScript bike = Spawner.Vehicle("MBM_HondaCRF450_" + colours[i], pos, bikeHeading, kit);
			if (bike)
			{
				m_Bikes.Insert(bike);
				Print("[HeliHunt] " + bike.GetType() + " max steering angle " + GetGame().ConfigGetFloat("CfgVehicles " + bike.GetType() + " SimulationModule Steering maxSteeringAngle") + " tyre grip " + GetGame().ConfigGetFloat("CfgVehicles MBM_HondaCRF450_Wheel tyreGrip") + " tyre width " + GetGame().ConfigGetFloat("CfgVehicles MBM_HondaCRF450_Wheel width") + " torque curve points " + ConfigArrayCount("CfgVehicles " + bike.GetType() + " SimulationModule Engine torqueCurve"));
			}

			Loadouts.Rider(pos + bikeFwd * GEAR_DISTANCE, bikeHeading, helmets[i]);
		}

		// players arrive behind the bikes, in the middle of the line
		vector spawn = roadPos - bikeFwd * 3.0;
		m_PlayerSpawn = RoadFinder.OnSurface(spawn[0], spawn[2]);
	}

	int ConfigArrayCount(string path)
	{
		array<float> values = new array<float>();
		GetGame().ConfigGetFloatArray(path, values);
		return values.Count();
	}

	// -missiontest: log the state of the scene once physics has settled, then quit
	void TestReport()
	{
		foreach (CarScript bike : m_Bikes)
		{
			Print("[HeliHunt] " + bike.GetType() + " pos " + bike.GetPosition() + " ori " + bike.GetOrientation() + " seats " + bike.CrewSize() + " fuel " + bike.GetFluidFraction(CarFluid.FUEL) + " attachments " + bike.GetInventory().AttachmentCount());
		}

		foreach (CarScript heli : m_Helis)
		{
			Print("[HeliHunt] " + heli.GetType() + " pos " + heli.GetPosition() + " ori " + heli.GetOrientation() + " seats " + heli.CrewSize() + " fuel " + heli.GetFluidFraction(CarFluid.FUEL) + " hydraulic " + heli.GetFluidFraction(CarFluid.OIL) + " attachments " + heli.GetInventory().AttachmentCount() + " from spawn " + vector.Distance(heli.GetPosition(), m_PlayerSpawn));

			array<Object> objects = new array<Object>();
			GetGame().GetObjectsAtPosition(heli.GetPosition(), 12, objects, null);
			int items = 0;
			int weapons = 0;
			foreach (Object obj : objects)
			{
				EntityAI entity = EntityAI.Cast(obj);
				if (entity && entity.IsInherited(ItemBase) && !entity.GetHierarchyParent())
				{
					items++;
					Weapon_Base weapon = Weapon_Base.Cast(entity);
					if (weapon)
					{
						weapons++;
						Print("[HeliHunt]   " + weapon.GetType() + " attachments " + weapon.GetInventory().AttachmentCount() + " chambered " + !weapon.IsChamberEmpty(0));
					}
				}
			}
			Print("[HeliHunt]   ground items " + items + " weapons " + weapons);
		}
		Print("[HeliHunt] bikes " + m_Bikes.Count() + " helis " + m_Helis.Count() + " player spawn " + m_PlayerSpawn);
		GetGame().RequestExit(0);
	}

	void SetRandomHealth(EntityAI itemEnt)
	{
		if ( itemEnt )
		{
			float rndHlt = Math.RandomFloat( 0.45, 0.65 );
			itemEnt.SetHealth01( "", "", rndHlt );
		}
	}

	override PlayerBase CreateCharacter(PlayerIdentity identity, vector pos, ParamsReadContext ctx, string characterName)
	{
		if (m_PlayerSpawn != vector.Zero)
			pos = m_PlayerSpawn;

		Entity playerEnt;
		playerEnt = GetGame().CreatePlayer( identity, characterName, pos, 0, "NONE" );
		Class.CastTo( m_player, playerEnt );

		GetGame().SelectPlayer( identity, m_player );

		return m_player;
	}

	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{
		EntityAI itemClothing;
		EntityAI itemEnt;

		itemClothing = player.FindAttachmentBySlotName( "Body" );
		if ( itemClothing )
		{
			SetRandomHealth( itemClothing );

			itemEnt = itemClothing.GetInventory().CreateInInventory( "BandageDressing" );
			player.SetQuickBarEntityShortcut(itemEnt, 2);
		}

		itemClothing = player.FindAttachmentBySlotName( "Legs" );
		if ( itemClothing )
			SetRandomHealth( itemClothing );
	}
};

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}
