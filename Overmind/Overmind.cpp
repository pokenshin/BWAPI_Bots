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

// Builds something using a drone.
void buildStructure(Unit drone, UnitType building)
{

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

//Checks if the player has enough supplies. For some reason, requires a supply number you want to check and a hatchery
bool haveSupplies(int req)
{
	//Checks if we have enough supplies
	if ((Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed()) < req)
	{
		return false;
	}
	else
	{
		return true;
	}
}

//Trains a drone. Requires the hatchery that will spawn it.
void trainUnit(Unit hatchery, UnitType type)
{
	// Retrieve a larva
	Unit larva = hatchery->getClosestUnit(GetType == UnitTypes::Zerg_Larva && IsOwned && IsIdle);

	// If the larva was found
	if (larva)
	{
		larva->train(type);
		Error lastErr = Broodwar->getLastError();
		displayError(hatchery->getPosition(), lastErr);
	}
}
// End Custom Functions

// Start BWAI Functions
void Overmind::onStart()
{
	//Makes the game FAAAAAAST
	Broodwar->setFrameSkip(20);
	Broodwar->sendText("Awaken my child and embrace the glory that is your birthright.");
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
	// Finds the start Position
	Position startPos = Position(Broodwar->self()->getStartLocation().x, Broodwar->self()->getStartLocation().y);

	//Unit Count
	int droneCount = Broodwar->self()->allUnitCount(UnitTypes::Zerg_Drone);
	int lingCount = Broodwar->self()->allUnitCount(UnitTypes::Zerg_Zergling);
	int overlordCount = Broodwar->self()->allUnitCount(UnitTypes::Zerg_Overlord);

	//Building Count
	int hatchCount = Broodwar->self()->allUnitCount(UnitTypes::Zerg_Hatchery);
	int poolCount = Broodwar->self()->allUnitCount(UnitTypes::Zerg_Spawning_Pool);

	//Counters to avoid double building related crashes
	static int poolRetryTime = 0;

	// Update on screen information
	Broodwar->drawTextScreen(200, 0, "FPS: %d", Broodwar->getFPS());
	Broodwar->drawTextScreen(200, 10, "Drones: %i", droneCount);
	Broodwar->drawTextScreen(200, 20, "Lings: %i", lingCount);
	Broodwar->drawTextScreen(200, 30, "Overlords: %i", overlordCount);
	Broodwar->drawTextScreen(200, 40, "Hatcheries: %i", hatchCount);
	Broodwar->drawTextScreen(200, 50, "Pools: %i", poolCount);
	
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

			// Hatchery AI Section (Training Units)
			else if (u->getType() == UnitTypes::Zerg_Hatchery)
			{
				// Unit Training
				// Drone Training
				// Currently: trains a drone if we have supplies and less than 9 drones
				if (droneCount <= 9 && haveSupplies(UnitTypes::Zerg_Drone.supplyRequired()))
					trainUnit(u, UnitTypes::Zerg_Drone);

				// Ling Training
				// Currently: trains a zergling if we have supplies, a Spawning Pool and at least 9 drones
				else if (poolCount > 0 && haveSupplies(UnitTypes::Zerg_Zergling.supplyRequired()))
					trainUnit(u, UnitTypes::Zerg_Zergling);

				// Overlord Training
				// Currently: trains an overlord if we are supply blocked and not training anything else (improve this)
				else if (Broodwar->self()->incompleteUnitCount() == 0)
					trainUnit(u, UnitTypes::Zerg_Overlord);
			}
		}
	} 

	// Building section
	// Gets the worker to build stuff
	Unit drone = Broodwar->getClosestUnit(startPos, Filter::IsWorker && Filter::IsOwned);


	// Builds a spawning pool when it has 200 minerals, it has no pools and after a brief retry time has been elapsed
	if (Broodwar->self()->minerals() >= UnitTypes::Zerg_Spawning_Pool.mineralPrice() && poolCount < 1 && poolRetryTime + 400 < Broodwar->getFrameCount())
	{
		buildStructure(drone, UnitTypes::Zerg_Spawning_Pool);
		poolRetryTime = Broodwar->getFrameCount();
	}

	// Builds another hatchery when it has at least 500 minerals
	if (Broodwar->self()->minerals() >= (UnitTypes::Zerg_Hatchery.mineralPrice() + 200))
	{
		buildStructure(drone, UnitTypes::Zerg_Hatchery);
	}

	
}

