class CfgPatches
{
	class VehicleShootingAnywhere
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts", "VehicleShooting"};
	};
};

class CfgMods
{
	class VehicleShootingAnywhere
	{
		dir = "VehicleShootingAnywhere";
		name = "VehicleShootingAnywhere";
		type = "mod";
		dependencies[] = {"World"};

		class defs
		{
			class worldScriptModule
			{
				value = "";
				files[] = {"VehicleShootingAnywhere/Scripts/4_World"};
			};
		};
	};
};
