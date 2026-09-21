// Coast Bikes
// One of every motorbike, kitted out and fully fuelled, lined up on the coast
// highway half way between Chernogorsk and Elektrozavodsk. Each bike has a set
// of riding gear on the ground in front of it. Players spawn next to the line-up.
#include "lib/RoadFinder.c"
#include "lib/Spawner.c"

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
	static const float BIKE_SPACING = 2.0;
	static const float GEAR_DISTANCE = 2.0; // from bike centre to the middle of its gear pile

	protected ref array<MotorbikeScript> m_Bikes = new array<MotorbikeScript>();
	protected vector m_PlayerSpawn;

	override void OnInit()
	{
		super.OnInit();

		SpawnLineup();

		string value;
		if (GetGame().CommandlineGetParam("missiontest", value))
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestReport, 20000, false);
	}

	// wheels for every bike, plus the colour matched shields on the Motorbike_02
	array<string> KitFor(string type)
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

	void SpawnLineup()
	{
		vector roadPos;
		float roadHeading, roadWidth;
		if (!RoadFinder.Locate(ANCHOR, 300, roadPos, roadHeading, roadWidth))
		{
			Print("[CoastBikes] no road found near " + ANCHOR);
			return;
		}
		Print("[CoastBikes] road at " + roadPos + " heading " + roadHeading + " width " + roadWidth);

		array<string> types = {"Motorbike_01_Blue", "Motorbike_01_Red", "Motorbike_01_Yellow", "Motorbike_02_Blue", "Motorbike_02_Green", "Motorbike_02_Red", "Motorbike_02_Yellow"};

		// enduro helmet to go with each bike (there is no yellow one)
		array<string> helmets = {"DirtBikeHelmet_Blue", "DirtBikeHelmet_Red", "DirtBikeHelmet_Black", "DirtBikeHelmet_Blue", "DirtBikeHelmet_Green", "DirtBikeHelmet_Red", "DirtBikeHelmet_Black"};

		// bikes stand side by side along the centre line, facing across the road
		vector along = RoadFinder.HeadingToDir(roadHeading);
		float bikeHeading = roadHeading + 90;
		vector bikeFwd = RoadFinder.HeadingToDir(bikeHeading);

		for (int i = 0; i < types.Count(); i++)
		{
			float offset = (i - (types.Count() - 1) * 0.5) * BIKE_SPACING;
			float width;
			vector pos = RoadFinder.Centre(roadPos + along * offset, roadHeading, width);

			MotorbikeScript bike = Spawner.Motorbike(types[i], pos, bikeHeading, KitFor(types[i]));
			if (bike)
				m_Bikes.Insert(bike);

			array<string> gear = {"LeatherJacket_Black", "Jeans_Blue", "HikingBoots_Brown", "AviatorGlasses", "LeatherGloves_Black"};
			gear.InsertAt(helmets[i], 0);
			array<EntityAI> items = Spawner.GroundPile(gear, pos + bikeFwd * GEAR_DISTANCE, bikeHeading, 3, 0.5);

			EntityAI helmet = items[0];
			if (helmet)
			{
				helmet.GetInventory().CreateAttachment("DirtBikeHelmet_Visor");
				helmet.GetInventory().CreateAttachment("DirtBikeHelmet_Mouthguard");
			}
		}

		// players arrive behind the bikes, in the middle of the line
		vector spawn = roadPos - bikeFwd * 3.0;
		m_PlayerSpawn = RoadFinder.OnSurface(spawn[0], spawn[2]);
	}

	// -missiontest: log the state of the line-up once physics has settled, then quit
	void TestReport()
	{
		foreach (MotorbikeScript bike : m_Bikes)
		{
			Print("[CoastBikes] " + bike.GetType() + " pos " + bike.GetPosition() + " ori " + bike.GetOrientation() + " fuel " + bike.GetFluidFraction(MotorbikeFluid.FUEL) + " attachments " + bike.GetInventory().AttachmentCount());
		}

		array<Object> objects = new array<Object>();
		GetGame().GetObjectsAtPosition(m_PlayerSpawn, 15, objects, null);
		int items = 0;
		foreach (Object obj : objects)
		{
			if (obj.IsInherited(Clothing) && !EntityAI.Cast(obj).GetHierarchyParent())
			{
				items++;
				Print("[CoastBikes] gear " + obj.GetType() + " pos " + obj.GetPosition() + " attachments " + EntityAI.Cast(obj).GetInventory().AttachmentCount());
			}
		}
		Print("[CoastBikes] bikes " + m_Bikes.Count() + " gear " + items + " player spawn " + m_PlayerSpawn);
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
