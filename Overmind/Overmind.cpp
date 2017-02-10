#include "Overmind.h"
#include <iostream>

using namespace BWAPI;
using namespace Filter;

// Custom Functions
// Error displaying function
void displayError(Position pos, Error lastErr)
{
	// However, drawing the error once will only appear for a single frame
	// so create an event that keeps it on the screen for some frames
	Broodwar->registerEvent([pos, lastErr](Game*){ Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); },   // action
		nullptr,    // condition
		Broodwar->getLatencyFrames());  // frames to run
}

//Checks if the unit is valid
bool isValid(Unit u)
{
	// Checks if the unit exists
	if (!u->exists())
		return false;

	// Check if the unit is locked down, maelstrommed or in stasis
	if (u->isLockedDown() || u->isMaelstrommed() || u->isStasised())
		return false;

	// Checks if the unit is loaded, unpowered or stuck somehow
	if (u->isLoaded() || !u->isPowered() || u->isStuck())
		return false;

	// Checks if the unit is incomplete or busy constructing
	if (!u->isCompleted() || u->isConstructing())
		return false;

	// else return true	
	return true;
}

// Builds something.
void Overmind::buildStructure(UnitType building)
{
	// Retrieve a larva
	Unit drone = Broodwar->getClosestUnit(startPos, IsWorker && IsOwned);

	// The position to build, where the normal Computer AI would build
	TilePosition targetBuildLocation = Broodwar->getBuildLocation(building, drone->getTilePosition());

	// If the position is valid
	if (targetBuildLocation)
	{
		// Draws the build location
		Broodwar->registerEvent([targetBuildLocation, building](Game*)
		{
			Broodwar->drawBoxMap(Position(targetBuildLocation),
				Position(targetBuildLocation + building.tileSize()),
				Colors::Blue);
		},
			nullptr,  // condition
			building.buildTime() + 100);  // frames to run

		// Order the builder to construct the supply structure
		drone->build(building, targetBuildLocation);
	}
}

// Handles the Worker AI
void handleWorkerAI(Unit u)
{
	// Send Idle Workers to minerals
	if (u->isIdle())
	{
		// Order workers carrying a resource to return them to the center,
		// otherwise find a mineral patch to harvest.
		if (u->isCarryingGas() || u->isCarryingMinerals())
		{
			u->returnCargo();
		}
		else if (!u->getPowerUp())  // The worker cannot harvest anything if it
		{                             // is carrying a powerup such as a flag
			// Harvest from the nearest mineral patch or gas refinery
			if (!u->gather(u->getClosestUnit(IsMineralField || IsRefinery)))
			{
				// If the call fails, then print the last error message
				Broodwar << Broodwar->getLastError() << std::endl;
			}
		} // closure: has no powerup
	} // End Idle Workers
}

//Checks if the player has enough supplies to train a certain unit. Requires the UnitType you want to check.
bool haveSupplies(UnitType unitType)
{
	//Checks if we have enough supplies
	if ((Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed()) < unitType.supplyRequired())
	{
		return false;
	}
	else
	{
		return true;
	}
}

//Trains an unit. Requires the hatchery that will spawn it.
void Overmind::trainUnit(UnitType type)
{
	if (!hatches.empty())
	{
		hatches.train(type);
	}
}

//Attempts to follow a build order
void Overmind::processBuildOrder()
{
	//Stores the current step we are currently in.
	//static int currentStep = 0;
	int currentSupply = Broodwar->self()->supplyUsed();
	int maxSupply = Broodwar->self()->supplyTotal();

	//Unit Count
	int droneCount = Broodwar->self()->allUnitCount(UnitTypes::Zerg_Drone);
	int lingCount = Broodwar->self()->allUnitCount(UnitTypes::Zerg_Zergling);
	int overlordCount = Broodwar->self()->allUnitCount(UnitTypes::Zerg_Overlord);

	//Building Count
	int hatchCount = Broodwar->self()->allUnitCount(UnitTypes::Zerg_Hatchery);
	int poolCount = Broodwar->self()->allUnitCount(UnitTypes::Zerg_Spawning_Pool);

	//Resource Count
	int mineralCount = Broodwar->self()->minerals();
	int gasCount = Broodwar->self()->gas();

	//Gets a builder
	Unit drone = Broodwar->getClosestUnit(startPos, Filter::IsWorker && Filter::IsOwned);

	//Counters to avoid double building related crashes
	static int poolRetryTime = 0;

	// While we have less than 18 drones, prioritize drones
	if (droneCount < 18)
	{
		// 6/9 - Drone(7)
		// 7/9 - Drone(8)
		// 8/9 - Drone(9)
		// 8/9 - Drone(10) (after 9/9 - Pool)
		// 9/18 - Drone(11) (after 9/9 - Overlord)
		// 13/18 - Drone(12) (after lings)
		// 14/18 - Drone(13) (after lings)
		// 15/18 - Drone(14) (after lings)
		// 17/18 - Drone(15) (after 3rd Overlord and Hatch)
		// 18/27 - Drone(16) (after 3rd Overlord and Hatch)
		// 19/27 - Drone(17) (after 3rd Overlord and Hatch)
		// 20/27 - Drone(18) (after 3rd Overlord and Hatch)
		if (mineralCount >= UnitTypes::Zerg_Drone.mineralPrice() &&
			haveSupplies(UnitTypes::Zerg_Drone) &&
				(
					(currentSupply < 9 || currentSupply > 12) || // Drones until 9/9 or after lings, pool and overlords (12/18)
					currentSupply != 16 || // Dont build a drone at 16/18 because 16/18 - Overlord
					(currentSupply == 15 && hatchCount == 2) // Only build at 15/18 if second hatch is on the way because 15/18 - Hatch
				)
			)
		{
			trainUnit(UnitTypes::Zerg_Drone);
		}
		// 9/9 - Pool
		else if (currentSupply == 9 && 
			maxSupply == 9 && 
			poolCount == 0 && 
			mineralCount >= UnitTypes::Zerg_Spawning_Pool.mineralPrice() && 
			poolRetryTime + 400 < Broodwar->getFrameCount())
		{
			buildStructure(UnitTypes::Zerg_Spawning_Pool);
			poolRetryTime = Broodwar->getFrameCount();
		}
		// 9/9 - Overlord
		// 16/18 - Overlord
		else if (((currentSupply == maxSupply && overlordCount <= 1) || // 9/9 - Overlord
			(currentSupply == 16 && overlordCount <= 2)) && // 16/18 - Overlord
			poolCount == 1 &&
			mineralCount >= UnitTypes::Zerg_Overlord)
		{
			trainUnit(UnitTypes::Zerg_Overlord);
		}
		// 10 / 18 - Zergling(2)
		// 11 / 18 - Zergling(4)
		// 12 / 18 - Zergling(6)
		else if (currentSupply > 9 &&
			currentSupply < 13 &&
			lingCount < 6 &&
			mineralCount >= UnitTypes::Zerg_Zergling.mineralPrice() &&
			haveSupplies(UnitTypes::Zerg_Zergling)
			)
		{
			trainUnit(UnitTypes::Zerg_Zergling);
		}
		// 15 / 18 - Hatch	
		else if (currentSupply == 15 &&
			hatchCount == 1 &&
			mineralCount >= UnitTypes::Zerg_Hatchery.mineralPrice())
		{
			buildStructure(UnitTypes::Zerg_Hatchery);
			hatches = Broodwar->getUnitsInRadius(startPos, 99999, IsResourceDepot && IsOwned);
		}
	}
	// 21+ Lings and Overlords
	//TODO
}

//Converts a boolean to string, for debug porpouses
std::string boolToString(bool predicate)
{
	if (predicate)
		return "True";
	else
		return "False";
}

void Overmind::displayInfo()
{
	// Update on screen information
	Broodwar->drawTextScreen(200, 0, "FPS: %d", Broodwar->getFPS());
	Broodwar->drawTextScreen(200, 10, "Drones: %i", Broodwar->self()->allUnitCount(UnitTypes::Zerg_Drone));
	Broodwar->drawTextScreen(200, 20, "Lings: %i", Broodwar->self()->allUnitCount(UnitTypes::Zerg_Zergling));
	Broodwar->drawTextScreen(200, 30, "Overlords: %i", Broodwar->self()->allUnitCount(UnitTypes::Zerg_Overlord));
	Broodwar->drawTextScreen(200, 40, "Hatcheries: %i", Broodwar->self()->allUnitCount(UnitTypes::Zerg_Hatchery));
	Broodwar->drawTextScreen(200, 50, "Pools: %i", Broodwar->self()->allUnitCount(UnitTypes::Zerg_Spawning_Pool));
}
// End Custom Functions

// Start BWAI Functions
void Overmind::onStart()
{
	Broodwar->sendText("Awaken my child and embrace the glory that is your birthright.");

	//Sets our start position
	startPos = Position(Broodwar->self()->getStartLocation().x, Broodwar->self()->getStartLocation().y);
	//Locates the main hatch and adds to the hatches Unitset
	Unit hatch = Broodwar->getClosestUnit(startPos, IsResourceDepot && IsOwned);
	hatches.insert(hatch);
}

void Overmind::onEnd(bool isWinner)
{

}

void Overmind::onSendText(std::string text)
{

}

void Overmind::onReceiveText(BWAPI::Player player, std::string text)
{

}

void Overmind::onPlayerLeft(BWAPI::Player player)
{

}

void Overmind::onNukeDetect(BWAPI::Position target)
{

}

void Overmind::onUnitDiscover(BWAPI::Unit unit)
{

}

void Overmind::onUnitEvade(BWAPI::Unit unit)
{

}

void Overmind::onUnitShow(BWAPI::Unit unit)
{

}

void Overmind::onUnitHide(BWAPI::Unit unit)
{

}

void Overmind::onUnitCreate(BWAPI::Unit unit)
{
}

void Overmind::onUnitDestroy(BWAPI::Unit unit)
{

}

void Overmind::onUnitMorph(BWAPI::Unit unit)
{

}

void Overmind::onUnitRenegade(BWAPI::Unit unit)
{

}

void Overmind::onSaveGame(std::string gameName)
{

}

void Overmind::onUnitComplete(BWAPI::Unit unit)
{

}

void Overmind::onFrame()
{
	//Displays stuff on screen.
	displayInfo();
	
	// Return if the game is a replay or is paused
	if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self())
		return;

	// Prevent spamming by only running our onFrame once every number of latency frames.
	// Latency frames are the number of frames before commands are processed.
	if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
		return;

	// Unit handling
	// Unit iterator
	for (auto &u : Broodwar->self()->getUnits())
	{
		// Checks if the unit is valid
		if (isValid(u))
		{
			// Drone AI section
			if (u->getType() == UnitTypes::Zerg_Drone)
			{
				// Calls the function to handle Worker-related stuff
				handleWorkerAI(u);
			}
		}
	}

	//Process our build order.
	processBuildOrder();
}