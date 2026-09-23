// Roles (stable)
// Vanilla Chernarus, except that every new character spawns as a role - police officer,
// doctor, soldier, lumberjack... - wearing the matching outfit and carrying a few things
// that fit the job. The roles live in roles.json next to this file. And there are ready
// to drive cars at ~150 of the vanilla car spawn points (spots.json, from vehicles.json),
// and a DayZ Expansion helicopter on each of the five helipads on the map.
#include "lib/JsonFile.c"
#include "lib/RoadFinder.c"
#include "lib/Spawner.c"
#include "lib/Placement.c"
#include "lib/VehicleSpots.c"
#include "lib/Roles.c"

// Vanilla cars, kitted out the way the debug menu does it and every fluid topped up.
class CarFactory: VehicleFactory
{
	override EntityAI Spawn(string type, vector pos, float heading)
	{
		pos[1] = pos[1] + 0.3; // created a hair below the ground, a vehicle falls through the map
		CarScript car = CarScript.Cast(GetGame().CreateObjectEx(type, pos, ECE_PLACE_ON_SURFACE | ECE_SETUP));
		if (!car)
		{
			Print("[Cars] failed to create " + type);
			return null;
		}
		car.SetOrientation(Vector(heading, 0, 0));
		car.SetLifetime(Spawner.LIFETIME);
		car.OnDebugSpawn();
		Refill(car);
		return car;
	}

	override float FuelFraction(EntityAI vehicle)
	{
		return CarScript.Cast(vehicle).GetFluidFraction(CarFluid.FUEL);
	}

	override void Refill(EntityAI vehicle)
	{
		CarScript car = CarScript.Cast(vehicle);
		car.Fill(CarFluid.FUEL, car.GetFluidCapacity(CarFluid.FUEL));
		car.Fill(CarFluid.OIL, car.GetFluidCapacity(CarFluid.OIL));
		car.Fill(CarFluid.COOLANT, car.GetFluidCapacity(CarFluid.COOLANT));
		car.Fill(CarFluid.BRAKE, car.GetFluidCapacity(CarFluid.BRAKE));
	}

	override vector Size(string type)
	{
		if (type.Contains("Truck"))
			return "2.6 3.0 7.5";
		return "2.0 1.8 4.6";
	}
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
	static const int MAX_CARS = 150; // to keep the server load sane

	// Chernarus has five helipads (decal_heli_army; tools: objprobe -findmodel=decal_heli):
	// the camp east of Chernogorsk, two at Balota, two at the Vybor military base. Headings
	// line the aircraft up with the H.
	static const vector CHERNO_PAD = "7236.25 0 3063.27";
	static const vector PLAYER_SPAWN = "7247 0 3061"; // 10 m east of the Chernogorsk pad
	protected ref array<CarScript> m_Helis = new array<CarScript>();

	protected string m_Path; // mission folder, e.g. "./mpmissions/roles.chernarusplus"
	protected ref VehicleSpots m_Cars;

	// path is the mission script, "./mpmissions/<mission>/mission.c"; keep its folder
	void CustomMission(string path)
	{
		m_Path = path.Substring(0, path.LastIndexOf("/"));
	}

	override void OnInit()
	{
		super.OnInit();

		int count = Roles.Load(m_Path + "/roles.json");
		Print("[Roles] loaded " + count + " roles from " + m_Path);

		m_Cars = new VehicleSpots("Cars", new CarFactory());
		m_Cars.Start(m_Path + "/spots.json", MAX_CARS);

		// like the cars: not in the first seconds after startup, or they come up empty
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SpawnHelis, VehicleSpots.START_DELAY_MS, false);

		string value;
		if (GetGame().CommandlineGetParam("missiontest", value))
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestReport, 5000, true);
	}

	// Instead of the vanilla random clothes, dress the new character as a random role.
	override void EquipCharacter(MenuDefaultCharacterData char_data)
	{
		Role role = Roles.Pick();
		if (role)
		{
			Roles.Equip(m_player, role);
			NotificationSystem.SendNotificationToPlayerExtended(m_player, 10, "You are a " + role.name, "Check your pockets.");
		}

		StartingEquipSetup(m_player, true);
	}

	// everyone still gets the vanilla freshie basics
	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{
		EntityAI itemEnt = player.GetInventory().CreateInInventory("BandageDressing");
		player.SetQuickBarEntityShortcut(itemEnt, 1);

		string chemlightArray[] = { "Chemlight_White", "Chemlight_Yellow", "Chemlight_Green", "Chemlight_Red" };
		itemEnt = player.GetInventory().CreateInInventory(chemlightArray[Math.RandomInt(0, 4)]);
		player.SetQuickBarEntityShortcut(itemEnt, 2);

		string fruitArray[] = { "Apple", "Pear", "Plum" };
		itemEnt = player.GetInventory().CreateInInventory(fruitArray[Math.RandomInt(0, 3)]);
		player.SetQuickBarEntityShortcut(itemEnt, 3);
	}

	// One helicopter per pad: each type once, the fifth pad gets a random one, shuffled.
	void SpawnHelis()
	{
		array<vector> pads = {CHERNO_PAD, "5030.24 0 2355.83", "5054.59 0 2333.00", "4155.93 0 11027.65", "4169.05 0 10990.72"};
		array<float> headings = {153, 73, 170, 117, -76};

		array<string> types = {"ExpansionUh1h", "ExpansionMh6", "ExpansionMerlin", "ExpansionGyrocopter"};
		array<string> order = new array<string>();
		for (int t = 0; t < types.Count(); t++)
			order.Insert(types[t]);
		order.Insert(types.GetRandomElement());
		for (int shuffle = order.Count() - 1; shuffle > 0; shuffle--)
			order.SwapItems(shuffle, Math.RandomInt(0, shuffle + 1));

		for (int i = 0; i < pads.Count(); i++)
		{
			vector pos = pads[i];
			pos[1] = GetGame().SurfaceRoadY(pos[0], pos[2]);
			CarScript heli = Spawner.Vehicle(order[i], pos, headings[i], HeliKit(order[i]));
			if (heli)
			{
				m_Helis.Insert(heli);
				Print("[Heli] " + order[i] + " on the helipad at " + heli.GetPosition());
			}
		}
	}

	// what each Expansion helicopter needs to fly (from the mod's own debug spawn routines)
	array<string> HeliKit(string type)
	{
		array<string> kit = new array<string>();
		kit.Insert("ExpansionHelicopterBattery");
		kit.Insert("HeadlightH7");
		if (type == "ExpansionGyrocopter")
		{
			kit.Insert("SparkPlug");
			return kit;
		}
		kit.Insert("ExpansionHydraulicHoses");
		kit.Insert("ExpansionIgniterPlug");
		if (type == "ExpansionMerlin")
		{
			kit.Insert("ExpansionMerlinFrontWheel");
			kit.Insert("ExpansionMerlinFrontWheel");
			kit.Insert("ExpansionMerlinBackWheel");
			kit.Insert("ExpansionMerlinBackWheel");
		}
		return kit;
	}

	// everyone spawns beside the helicopter instead of on the coast
	override PlayerBase CreateCharacter(PlayerIdentity identity, vector pos, ParamsReadContext ctx, string characterName)
	{
		pos = PLAYER_SPAWN;
		pos[1] = GetGame().SurfaceY(pos[0], pos[2]);

		Entity playerEnt;
		playerEnt = GetGame().CreatePlayer(identity, characterName, pos, 0, "NONE");
		Class.CastTo(m_player, playerEnt);

		GetGame().SelectPlayer(identity, m_player);

		return m_player;
	}

	// -missiontest: check every role's class names and attachments and the cars, then quit
	void TestReport()
	{
		if (!m_Cars.IsDone())
			return;
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(TestReport);
		m_Cars.Report();
		for (int h = 0; h < m_Helis.Count(); h++)
		{
			CarScript heli = m_Helis[h];
			Print("[Heli] test: " + heli.GetType() + " pos " + heli.GetPosition() + " ori " + heli.GetOrientation() + " fuel " + heli.GetFluidFraction(CarFluid.FUEL) + " hydraulic " + heli.GetFluidFraction(CarFluid.OIL) + " attachments " + heli.GetInventory().AttachmentCount() + " above terrain " + (heli.GetPosition()[1] - GetGame().SurfaceY(heli.GetPosition()[0], heli.GetPosition()[2])));
		}
		Print("[Heli] test: " + m_Helis.Count() + " helicopters");

		array<ref Role> roles = Roles.All();
		for (int i = 0; i < roles.Count(); i++)
		{
			Role role = roles[i];
			string clothes = "";
			for (int j = 0; j < role.clothing.Count(); j++)
				clothes = clothes + " " + role.clothing[j];
			Print("[Roles] " + role.name + " (weight " + role.weight.ToString() + "): " + role.items.Count() + " items;" + clothes);
		}

		// does Pick() actually spread across roles?
		map<string, int> picks = new map<string, int>();
		for (int n = 0; n < 200; n++)
		{
			Role picked = Roles.Pick();
			int seen = 0;
			picks.Find(picked.name, seen);
			picks.Set(picked.name, seen + 1);
		}
		string histogram = "";
		for (int k = 0; k < picks.Count(); k++)
			histogram = histogram + " " + picks.GetKey(k) + "=" + picks.GetElement(k);
		Print("[Roles] 200 picks (" + Math.RandomFloat(0, 22) + " " + Math.RandomFloat(0, 22) + "):" + histogram);

		int problems = Roles.Validate("8500 0 2796");
		Print("[Roles] validated " + Roles.All().Count() + " roles, " + problems + " problems");
		GetGame().RequestExit(0);
	}
};

Mission CreateCustomMission(string path)
{
	return new CustomMission(path);
}
