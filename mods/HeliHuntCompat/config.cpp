class CfgPatches
{
	class HeliHuntCompat
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Vehicles_Wheeled", "MBM_HondaCRF450"};
	};
};

// MBM Honda CRF450R handling. As shipped it understeers badly: 25 degrees of lock, steering
// that slows to 10 deg/s at speed and a locked centre differential. Everything not listed
// here keeps the mod's own value.
class CfgVehicles
{
	// The street tyre (also used for the bike's two invisible stabiliser wheels) has about
	// half the grip of the mod's own offroad tyres (1.9), and slides around. Match them.
	class CarWheel;
	class MBM_HondaCRF450_Wheel: CarWheel
	{
		tyreGrip = 1.9;
	};

	class CarScript;
	class MBM_HondaCRF450_base: CarScript
	{
		class SimulationModule
		{
			class Steering
			{
				maxSteeringAngle = 38;
				// pairs of {speed km/h, degrees per second}
				increaseSpeed[] = {0, 70, 60, 40, 100, 20};
				decreaseSpeed[] = {0, 90, 60, 60, 100, 35};
				centeringSpeed[] = {0, 0, 15, 30, 60, 50, 100, 70};
			};
			class CentralDifferential
			{
				ratio = 1.0;
				type = "DIFFERENTIAL_OPEN";
			};
		};
	};
};
