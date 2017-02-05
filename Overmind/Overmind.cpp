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

// Updates the on screen info
void Overmind::updateOnScreenInfo()
{
	// Display the game frame rate as text in the upper left area of the screen
	Broodwar->drawTextScreen(200, 0, "FPS: %d", Broodwar->getFPS());
	Broodwar->drawTextScreen(200, 20, "Incomplete Pools: %i", Broodwar->self()->incompleteUnitCount(UnitTypes::Zerg_Spawning_Pool));
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

	// Building section
	// Builds a spawning pool when it has 200 minerals
	int poolCount = Broodwar->self()->allUnitCount(UnitTypes::Zerg_Spawning_Pool);
	static int poolRetryTime = 0;

	if (Broodwar->self()->minerals() >= UnitTypes::Zerg_Spawning_Pool.mineralPrice() && poolCount < 1 && poolRetryTime + 400 < Broodwar->getFrameCount())
	{
		buildStructure(u, UnitTypes::Zerg_Spawning_Pool);
		poolRetryTime = Broodwar->getFrameCount();
	}
}

//Trains an overlord. Requires the hatchery that will spawn it
void trainOverlord(Unit hatchery)
{
	Unit larva = hatchery->getClosestUnit(GetType == UnitTypes::Zerg_Larva && IsOwned && IsIdle);

	if (larva)
	{
		larva->train(UnitTypes::Zerg_Overlord);
		Error lastErr = Broodwar->getLastError();
		displayError(hatchery->getPosition(), lastErr);
	}
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
void trainDrone(Unit hatchery)
{
	// Retrieve a larva
	Unit larva = hatchery->getClosestUnit(GetType == UnitTypes::Zerg_Larva && IsOwned && IsIdle);

	// If the larva was found
	if (larva)
	{
		larva->train(UnitTypes::Zerg_Drone);
		Error lastErr = Broodwar->getLastError();
		displayError(hatchery->getPosition(), lastErr);
	}
}
// End Custom Functions

// Start BWAI Functions
void Overmind::onStart()
{
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
	// Update on screen information
	updateOnScreenInfo();
	
	// Return if the game is a replay or is paused
	if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self())
		return;

	// Prevent spamming by only running our onFrame once every number of latency frames.
	// Latency frames are the number of frames before commands are processed.
	if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
		return;

	// Iterate through all the units that we own
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
				// Drone Training
				if (haveSupplies(UnitTypes::Zerg_Drone.supplyRequired()))
				{
					trainDrone(u);
				}
				else
				{
					// If there's not an overlord (or anything else) already morphing
					if (Broodwar->self()->incompleteUnitCount() == 0)
						trainOverlord(u);
				}
			}
		}
	} // closure: unit iterator
}

