// Companion to Hunterz' "Vehicle Shooting" (load this after it): shoot from any passenger
// seat of any vehicle, with any firearm.

modded class PlayerBase
{
	// Vehicle Shooting only allows vehicles that use one of the vanilla seat animation
	// sets, which rules out the DayZ Expansion helicopters. Allow every passenger seat.
	override bool CanStartVehicleShooting(Transport trans)
	{
		if (!trans || !IsVehicleShootingWeapon(GetItemInHands()))
			return false;

		CarScript car;
		if (!Class.CastTo(car, trans))
			return false;

		return car.CrewMemberIndex(this) != DayZPlayerConstants.VEHICLESEAT_DRIVER;
	}

	// ...and any firearm, not only pistols and revolvers.
	override bool IsVehicleShootingWeapon(EntityAI item)
	{
		return Weapon_Base.Cast(item) != null;
	}
};
