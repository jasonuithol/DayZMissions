// Sets of gear laid out on the ground.
class Loadouts
{
	// Riding gear in a small grid centred on pos: enduro helmet (with visor and
	// mouthguard), leather jacket, jeans, boots, aviators and gloves.
	static void Rider(vector pos, float heading, string helmet)
	{
		array<string> gear = {"LeatherJacket_Black", "Jeans_Blue", "HikingBoots_Brown", "AviatorGlasses", "LeatherGloves_Black"};
		gear.InsertAt(helmet, 0);
		array<EntityAI> items = Spawner.GroundPile(gear, pos, heading, 3, 0.5);

		array<string> helmetParts = {"DirtBikeHelmet_Visor", "DirtBikeHelmet_Mouthguard"};
		Spawner.Attach(items[0], helmetParts);
	}

	// Three piles in a row starting at pos and running along heading: military clothing,
	// a loaded M4A1 and SVD, then magazines, ammo and kit. About 8 m long, 1 m deep.
	static void Military(vector pos, float heading)
	{
		vector along = RoadFinder.HeadingToDir(heading);
		float pileHeading = heading + 90;

		array<string> clothing = {"BallisticHelmet_Green", "GorkaEJacket_Summer", "GorkaPants_Summer", "MilitaryBoots_Black", "TacticalGloves_Green", "Balaclava3Holes_Green", "PlateCarrierVest", "AssaultBag_Green", "MilitaryBelt", "NVGoggles", "NVGHeadstrap", "GhillieSuit_Woodland"};
		array<EntityAI> items = Spawner.GroundPile(clothing, pos - along * 3.0, pileHeading, 6, 0.6);
		array<string> vestParts = {"PlateCarrierPouches", "PlateCarrierHolster"};
		Spawner.Attach(items[6], vestParts);
		array<string> nvgParts = {"Battery9V"};
		Spawner.Attach(items[9], nvgParts);

		array<string> rifles = {"M4A1", "SVD"};
		items = Spawner.GroundPile(rifles, pos + along * 0.5, pileHeading, 2, 0.7);
		array<string> m4Parts = {"M4_RISHndgrd", "M4_OEBttstck", "ACOGOptic", "M4_Suppressor"};
		Rifle(items[0], "Mag_STANAG_30Rnd", m4Parts);
		array<string> svdParts = {"PSO1Optic"};
		Rifle(items[1], "Mag_SVD_10Rnd", svdParts);

		array<string> kit = {"Mag_STANAG_30Rnd", "Mag_STANAG_30Rnd", "AmmoBox_556x45_20Rnd", "Rangefinder", "CombatKnife", "Mag_SVD_10Rnd", "Mag_SVD_10Rnd", "AmmoBox_762x54_20Rnd", "PersonalRadio", "Canteen"};
		items = Spawner.GroundPile(kit, pos + along * 3.5, pileHeading, 5, 0.5);
		foreach (EntityAI item : items)
		{
			Magazine mag = Magazine.Cast(item);
			if (mag && !mag.IsAmmoPile())
				mag.ServerSetAmmoMax();
		}
	}

	// attachments plus a full magazine and a round in the chamber
	static void Rifle(EntityAI item, string magazine, array<string> attachments)
	{
		Spawner.Attach(item, attachments);

		Weapon_Base weapon = Weapon_Base.Cast(item);
		if (weapon && !weapon.SpawnAttachedMagazine(magazine))
			Print("[Loadouts] could not load " + weapon.GetType() + " with " + magazine);
	}
}
