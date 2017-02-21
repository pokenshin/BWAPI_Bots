#include "Overmind.h"
#include <iostream>
#include "BWEM/src/bwem.h"

using namespace BWAPI;
using namespace Filter;
using namespace BWEM;
using namespace BWEM::BWAPI_ext;
using namespace BWEM::utils;

namespace { auto & theMap = BWEM::Map::Instance(); }

//Custom variables;
Unitset hatches; // Our Hatcheries
Unitset zerglings; // Our Zerglings
Position startPos; // Our Start Position

// BWEM
std::vector<Area> allAreas;
std::vector<Base> allBases;
std::vector<Base> ourExpos;
std::vector<Base> enemyExpos;
Position enemyMain;
Position ourMain;


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

// Handles the Zergling AI
void handleZerglingAI(Unit u)
{
	//Attack-moves the zerglings to the nearest choke
	//u->attack(nearestChoke->getCenter());
}

void handleOverlordAI(Unit u)
{

	if (u->isIdle())
	{
		// Moves the overlord to the enemy base if its idle and we still have bases unscouted.	
	}
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

//Checks if we have any larva avaliable
bool Overmind::haveLarva()
{
	Unitset larvae = hatches.getLarva();

	if (larvae.size() > 0)
	{
		for (auto &u : larvae)
		{
			if (isValid(u))
				return true;
		}
		return false;
	}
	else
		return false;
}

//Attempts to follow a build order
void Overmind::processBuildOrder()
{
	//Stores the current step we are currently in.
	//static int currentStep = 0;
	int currentSupply = Broodwar->self()->supplyUsed() / 2;
	int maxSupply = Broodwar->self()->supplyTotal() / 2;

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
	static int overlordRetryTime = 0;
	static int hatchRetryTime = 0;

	// While we have less than 18 drones, prioritize drones
	if (droneCount < 18)
	{
		// 6/9 - Drone(7)
		// 7/9 - Drone(8)
		// 8/9 - Drone(9)
		// 8/9 - Drone(9) (after 9/9 - Pool)
		// 9/17 - Drone(10) (after 9/9 - Overlord)
		// 13/17 - Drone(11) (after lings)
		// 14/17 - Drone(12) (after lings)
		// 15/17 - Drone(13) (after lings)
		// 16/17 - Drone(13) (after 3rd Overlord and Hatch)
		// 17/25 - Drone(14) (after 3rd Overlord and Hatch)
		// 18/25 - Drone(15) (after 3rd Overlord and Hatch)
		// 19/25 - Drone(16) (after 3rd Overlord and Hatch)
		// 20/25 - Drone(17) (after 3rd Overlord and Hatch)
		// 21/25 - Drone(18) (after 3rd Overlord and Hatch)
		if (mineralCount >= UnitTypes::Zerg_Drone.mineralPrice() &&
			haveSupplies(UnitTypes::Zerg_Drone) &&
				(
					currentSupply != 15 || //Dont train a drone at 15 because 15/17 - Hatchery
					(currentSupply == 15 && hatchCount == 2) //Only train a drone if hatch is already on the way
				) &&
				(
					currentSupply != 16 || // Dont train a drone at 16/18 because 16/18 - Overlord
					(currentSupply == 16 && overlordCount == 3 && hatchCount == 2) // Only build drones at 16 after OL and Hatch
				) &&
				(
					(currentSupply <= 9 || currentSupply >= 13) || // Drones until 9/9 or after lings, pool and overlords (12/18)
			 		(currentSupply > 14 && hatchCount == 2) // Only build at 15/18 if second hatch is on the way because 15/18 - Hatch
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
		// 16/17 - Overlord
		else if (((currentSupply == maxSupply && overlordCount <= 1 && poolCount == 1) || // 9/9 - Overlord after 9/9 - Pool
			(currentSupply == 16 && overlordCount < 3)) && // 16/18 - Overlord
			mineralCount >= UnitTypes::Zerg_Overlord.mineralPrice() &&
			overlordRetryTime + 100 + UnitTypes::Zerg_Overlord.buildTime() < Broodwar->getFrameCount())
		{
			if (haveLarva()) // Sees if we have enough larvae so we dont trigger the timer without training an overlord. Saves time at 16/18 - Overlord
			{
				trainUnit(UnitTypes::Zerg_Overlord);
				overlordRetryTime = Broodwar->getFrameCount();
			}
		}
		// 10 / 17 - Zergling(2)
		// 11 / 17 - Zergling(4)
		// 12 / 17 - Zergling(6)
		else if (currentSupply > 9 &&
			currentSupply < 13 &&
			lingCount < 6 &&
			mineralCount >= UnitTypes::Zerg_Zergling.mineralPrice() &&
			haveSupplies(UnitTypes::Zerg_Zergling)
			)
		{
			trainUnit(UnitTypes::Zerg_Zergling);
		}
		// 15 / 17 - Hatch	
		else if (currentSupply == 15 &&
			hatchCount == 1 &&
			hatchRetryTime + 200 + UnitTypes::Zerg_Hatchery.buildTime() < Broodwar->getFrameCount() &&
			mineralCount >= UnitTypes::Zerg_Hatchery.mineralPrice())
		{
			buildStructure(UnitTypes::Zerg_Hatchery);
			hatchRetryTime = Broodwar->getFrameCount();
		}
	}
	else // 21+ Lings and Overlords
	{
		//Builds an Overlord if we're supply blocked and have the minerals
		if (currentSupply == maxSupply && mineralCount >= UnitTypes::Zerg_Overlord.mineralPrice() && overlordRetryTime + 100 + UnitTypes::Zerg_Overlord.buildTime() < Broodwar->getFrameCount() && haveLarva())
		{
			trainUnit(UnitTypes::Zerg_Overlord);
			overlordRetryTime = Broodwar->getFrameCount();
		}
		//Builds Zerglings if we have supplies and the minerals
		if (haveSupplies(UnitTypes::Zerg_Zergling) && mineralCount >= UnitTypes::Zerg_Zergling.mineralPrice())
		{
			trainUnit(UnitTypes::Zerg_Zergling);
		}
	}
}

//Converts a predicate result (true or false) to string, for debug porpouses
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
	Broodwar->drawTextScreen(50, 0, "FPS: %d", Broodwar->getFPS());
	Broodwar->drawTextScreen(50, 10, "Drones: %i", Broodwar->self()->allUnitCount(UnitTypes::Zerg_Drone));
	Broodwar->drawTextScreen(50, 20, "Lings: %i", Broodwar->self()->allUnitCount(UnitTypes::Zerg_Zergling));
	Broodwar->drawTextScreen(50, 30, "Overlords: %i", Broodwar->self()->allUnitCount(UnitTypes::Zerg_Overlord));
	Broodwar->drawTextScreen(50, 40, "Hatcheries: %i", Broodwar->self()->allUnitCount(UnitTypes::Zerg_Hatchery));
	Broodwar->drawTextScreen(50, 50, "Pools: %i", Broodwar->self()->allUnitCount(UnitTypes::Zerg_Spawning_Pool));
	Broodwar->drawTextScreen(50, 60, "Our Start Pos: %i, %i", startPos);
}
// End Custom Functions

// Start BWAI Functions
void Overmind::onStart()
{
	try
	{ 
		// Flavour Text
		Broodwar->sendText("Awaken my child and embrace the glory that is your birthright.");
		// Enables user input during the game
		Broodwar->enableFlag(BWAPI::Flag::UserInput);

		// Retrieve you and your enemy's races. enemy() will just return the first enemy.
		// If you wish to deal with multiple enemies then you must use enemies().
		if (Broodwar->enemy()) // First make sure there is an enemy
			Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;

		Broodwar << "Map initialization..." << std::endl;

		theMap.Initialize(); // Initializes the BWEM variable
		theMap.EnableAutomaticPathAnalysis(); // Enables automatic path analysis for the map
		bool startingLocationsOK = theMap.FindBasesForStartingLocations(); // Tries to find the starting locations
		assert(startingLocationsOK); // Makes the code fail if we dont find the starting locations

		BWEM::utils::MapPrinter::Initialize(&theMap);
		BWEM::utils::printMap(theMap);      // will print the map into the file <StarCraftFolder>bwapi-data/map.bmp
		BWEM::utils::pathExample(theMap);   // add to the printed map a path between two starting locations

		//Finds our main location
		startPos = BWAPI::Position(Broodwar->self()->getStartLocation().x, Broodwar->self()->getStartLocation().y);

		Broodwar << "glhf" << std::endl;
	}
	catch (const std::exception & e)
	{
		Broodwar << "ERROR at onStart! " << e.what() << std::endl;
	}
}

void Overmind::onEnd(bool isWinner)
{

}

void Overmind::onSendText(std::string text)
{
	BWEM::utils::MapDrawer::ProcessCommand(text);
	Broodwar->sendText("%s", text.c_str());
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
	try
	{
		if (unit->getType().isMineralField())
			theMap.OnMineralDestroyed(unit);
		else if (unit->getType().isSpecialBuilding())
			theMap.OnStaticBuildingDestroyed(unit);
	}
	catch (const std::exception & e)
	{
		Broodwar << "ERROR at onUnitDestroy! " << e.what() << std::endl;
	}


}

void Overmind::onUnitMorph(BWAPI::Unit unit)
{
	if (unit->getType() == UnitTypes::Zerg_Zergling && unit->getPlayer() == Broodwar->self())
		zerglings.insert(unit); // Adds the newly born zergling to our zerglings group
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
	try
	{
		if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self())
			return;

		// Draws the map instead of printing it to a file
		BWEM::utils::gridMapExample(theMap);
		BWEM::utils::drawMap(theMap);

		//Displays stuff on screen.
		displayInfo();

		// Updates our hatch list (probably best if setup elsewhere)
		hatches = Broodwar->getUnitsInRadius(startPos, 99999, IsResourceDepot && IsOwned);

		// Return if the game is a replay or is paused


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
				// Overlord AI section
				if (u->getType() == UnitTypes::Zerg_Overlord)
				{
					handleOverlordAI(u);
				}
				//Zergling AI section
				if (u->getType() == UnitTypes::Zerg_Zergling)
				{
					handleZerglingAI(u);
				}
			}
		}

		//Process our build order.
		processBuildOrder();
	}
	catch (const std::exception & e)
	{
		Broodwar << "ERROR at onFrame! " << e.what() << std::endl;
	}
	

}