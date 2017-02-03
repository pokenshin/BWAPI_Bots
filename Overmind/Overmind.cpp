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
void updateOnScreenInfo()
{
	// Display the game frame rate as text in the upper left area of the screen
	Broodwar->drawTextScreen(200, 0, "FPS: %d", Broodwar->getFPS());
	Broodwar->drawTextScreen(200, 20, "Average FPS: %f", Broodwar->getAverageFPS());
}

// Handles the Worker AI
void handleWorkerAI(Unit u)
{
	// if our worker is idle
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
	} // closure: if idle
}

void handleHatcheryAI(Unit u)
{
	// Orders the Hatchery to build more Drones.
	if (u->isIdle() && !u->train(u->getType().getRace().getWorker()))
	{
		Position pos = u->getPosition();
		Error lastErr = Broodwar->getLastError();
		// If that fails, draw the error at the location so that you can visibly see what went wrong!
		displayError(pos, lastErr);

		// Retrieve the supply provider type in the case that we have run out of supplies
		UnitType overlord = u->getType().getRace().getSupplyProvider();
		static int lastChecked = 0;

		// If we are supply blocked and haven't tried constructing more recently
		if (lastErr == Errors::Insufficient_Supply &&
			lastChecked + 400 < Broodwar->getFrameCount() &&
			Broodwar->self()->incompleteUnitCount(overlord) == 0)
		{
			lastChecked = Broodwar->getFrameCount();

			// Retrieve a unit that is capable of constructing the supply needed
			Unit larva = u->getClosestUnit(GetType == overlord.whatBuilds().first &&
				IsIdle && IsOwned);
			// If the larva was found
			if (larva)
				// Morph the Overlord!
				larva->train(overlord);
		} // closure: insufficient supply
	} // closure: failed to train idle unit
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
		// Ignore the unit if it no longer exists
		// Make sure to include this block when handling any Unit pointer!
		if (!u->exists())
			continue;

		// Ignore the unit if it has one of the following status ailments
		if (u->isLockedDown() || u->isMaelstrommed() || u->isStasised())
			continue;

		// Ignore the unit if it is in one of the following states
		if (u->isLoaded() || !u->isPowered() || u->isStuck())
			continue;

		// Ignore the unit if it is incomplete or busy constructing
		if (!u->isCompleted() || u->isConstructing())
			continue;


		// Finally make the unit do some stuff!


		// If the unit is a worker unit
		if (u->getType().isWorker())
		{
			// Calls the function to handle Worker-related stuff
			handleWorkerAI(u);
		}

		// If the unit is a Hatchery
		else if (u->getType().isResourceDepot())
		{
			// Calls the function to handle Hatchery AI
			handleHatcheryAI(u);
		}
	} // closure: unit iterator
}

