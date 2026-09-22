// Roles
// Vanilla Chernarus, except that every new character spawns as a role - police officer,
// doctor, soldier, lumberjack... - wearing the matching outfit and carrying a few things
// that fit the job. The roles live in roles.json next to this file.
#include "lib/Roles.c"

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
	protected string m_Path; // mission folder, e.g. "./mpmissions/roles.chernarusplus"

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

		string value;
		if (GetGame().CommandlineGetParam("missiontest", value))
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestReport, 15000, false);
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

	// -missiontest: check every role's class names and attachments, then quit
	void TestReport()
	{
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
