// Spawn roles: a full outfit plus a few fitting items, defined in a JSON file.
//
// {
//   "roles": [
//     {
//       "name": "Police officer",
//       "weight": 1,
//       "clothing": ["PoliceCap", "PoliceJacket|PoliceJacketOrel", ...],
//       "items": [
//         {"type": "Flashlight", "hands": true, "attachments": ["Battery9V"], "cargo": []},
//         ...
//       ]
//     }
//   ]
// }
//
// "A|B|C" anywhere a class name is expected picks one of them at random.

class RoleItem
{
	string type;
	bool hands;                  // spawn in the player's hands instead of the inventory
	ref array<string> attachments = new array<string>();
	ref array<string> cargo = new array<string>();
}

class Role
{
	string name;
	float weight = 1;
	ref array<string> clothing = new array<string>();
	ref array<ref RoleItem> items = new array<ref RoleItem>();
}

class RoleSet
{
	ref array<ref Role> roles = new array<ref Role>();
}

class Roles
{
	protected static ref RoleSet s_Roles;

	// Loads a roles file. The mission folder is the path handed to CreateCustomMission;
	// GetMissionFolderPath() is empty on a dedicated server at this point.
	static int Load(string path)
	{
		string text, error;
		RoleSet loaded = new RoleSet();
		if (!JsonFile.Read(path, text, error) || !(new JsonSerializer()).ReadFromString(loaded, text, error))
		{
			Print("[Roles] " + path + ": " + error);
			s_Roles = new RoleSet();
			return 0;
		}
		s_Roles = loaded;
		return loaded.roles.Count();
	}

	static array<ref Role> All()
	{
		if (!s_Roles)
			s_Roles = new RoleSet();
		return s_Roles.roles;
	}

	// weighted random choice
	static Role Pick()
	{
		array<ref Role> roles = All();
		float total = 0;
		for (int i = 0; i < roles.Count(); i++)
			total += roles[i].weight;
		if (total <= 0)
			return null;

		float r = Math.RandomFloat(0, total);
		for (int j = 0; j < roles.Count(); j++)
		{
			r -= roles[j].weight;
			if (r <= 0)
				return roles[j];
		}
		return roles[roles.Count() - 1];
	}

	// "A|B|C" -> one of them
	static string Choose(string spec)
	{
		array<string> options = new array<string>();
		spec.Split("|", options);
		if (options.Count() == 0)
			return spec;
		string choice = options.GetRandomElement();
		choice.Trim();
		return choice;
	}

	static bool IsKnown(string spec)
	{
		array<string> options = new array<string>();
		spec.Split("|", options);
		for (int i = 0; i < options.Count(); i++)
		{
			string option = options[i];
			option.Trim();
			if (!GetGame().ConfigIsExisting("CfgVehicles " + option))
				return false;
		}
		return true;
	}

	// Dresses a freshly created player as the role. Assumes they are naked (the
	// mission skips the vanilla random clothing).
	static void Equip(PlayerBase player, Role role)
	{
		for (int i = 0; i < role.clothing.Count(); i++)
		{
			string type = Choose(role.clothing[i]);
			if (!player.GetInventory().CreateInInventory(type))
				Print("[Roles] " + role.name + ": could not wear " + type);
		}

		for (int j = 0; j < role.items.Count(); j++)
		{
			RoleItem spec = role.items[j];
			string itemType = Choose(spec.type);
			EntityAI item;
			if (spec.hands)
				item = player.GetHumanInventory().CreateInHands(itemType);
			else
				item = player.GetInventory().CreateInInventory(itemType);
			if (!item)
			{
				Print("[Roles] " + role.name + ": no room for " + itemType);
				continue;
			}
			Fill(item, spec);
		}
	}

	static void Fill(EntityAI item, RoleItem spec)
	{
		for (int i = 0; i < spec.attachments.Count(); i++)
		{
			string a = Choose(spec.attachments[i]);
			if (!item.GetInventory().CreateAttachment(a))
				Print("[Roles] " + item.GetType() + ": could not attach " + a);
		}
		for (int j = 0; j < spec.cargo.Count(); j++)
		{
			string c = Choose(spec.cargo[j]);
			if (!item.GetInventory().CreateEntityInCargo(c))
				Print("[Roles] " + item.GetType() + ": no room for " + c);
		}
	}

	// Dev check: every class name exists, and every item takes its attachments and
	// cargo. Items are spawned at pos and deleted again. Returns the number of problems.
	static int Validate(vector pos)
	{
		int problems = 0;
		array<ref Role> roles = All();
		for (int r = 0; r < roles.Count(); r++)
		{
			Role role = roles[r];
			for (int c = 0; c < role.clothing.Count(); c++)
			{
				if (!IsKnown(role.clothing[c]))
				{
					Print("[Roles] " + role.name + ": unknown clothing " + role.clothing[c]);
					problems++;
				}
			}
			for (int i = 0; i < role.items.Count(); i++)
			{
				RoleItem spec = role.items[i];
				if (!IsKnown(spec.type))
				{
					Print("[Roles] " + role.name + ": unknown item " + spec.type);
					problems++;
					continue;
				}
				int k;
				for (k = 0; k < spec.attachments.Count(); k++)
				{
					if (!IsKnown(spec.attachments[k]))
					{
						Print("[Roles] " + role.name + ": unknown attachment " + spec.attachments[k]);
						problems++;
					}
				}
				for (k = 0; k < spec.cargo.Count(); k++)
				{
					if (!IsKnown(spec.cargo[k]))
					{
						Print("[Roles] " + role.name + ": unknown cargo " + spec.cargo[k]);
						problems++;
					}
				}

				// try it for real
				EntityAI item = EntityAI.Cast(GetGame().CreateObjectEx(Choose(spec.type), pos, ECE_PLACE_ON_SURFACE));
				if (!item)
				{
					Print("[Roles] " + role.name + ": could not create " + spec.type);
					problems++;
					continue;
				}
				for (k = 0; k < spec.attachments.Count(); k++)
				{
					if (!item.GetInventory().CreateAttachment(Choose(spec.attachments[k])))
					{
						Print("[Roles] " + role.name + ": " + item.GetType() + " does not take attachment " + spec.attachments[k]);
						problems++;
					}
				}
				for (k = 0; k < spec.cargo.Count(); k++)
				{
					if (!item.GetInventory().CreateEntityInCargo(Choose(spec.cargo[k])))
					{
						Print("[Roles] " + role.name + ": " + item.GetType() + " has no room for " + spec.cargo[k]);
						problems++;
					}
				}
				GetGame().ObjectDelete(item);
			}
		}
		return problems;
	}
}
