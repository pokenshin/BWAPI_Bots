#pragma once
#include <BWAPI.h>

class Overmind : public BWAPI::AIModule
{
public:
	virtual void onStart(); //Called once at the start of the game.
	virtual void onEnd(bool isWinner); //Called once when the game is over. isWinner returns true if the bot won. It will always return false if the game is a replay.
	virtual void onFrame(); //Called once every logical frame in Brood War.
	virtual void onSendText(std::string text); //Called when the (human) user sends a text message. Useful for debugging ingame and sending specific commands.
	virtual void onReceiveText(BWAPI::Player player, std::string text); //Called when the bot recieves a message ingame. Useful for co-op orders/strategies.
	virtual void onPlayerLeft(BWAPI::Player player); //Called when a player leaves the game
	virtual void onNukeDetect(BWAPI::Position target); //Called when a "Nuclear Launch Detected". "target" is the position of the red dot. If it's not visible to the player, it will get a Positions::Unknown
	virtual void onUnitDiscover(BWAPI::Unit unit); //Called when a unit is discovered. Includes status of Flag::CompleteMapInformation
	virtual void onUnitEvade(BWAPI::Unit unit); //Called when a unit goes out of view. Includes status of Flag::CompleteMapInformation
	virtual void onUnitShow(BWAPI::Unit unit); //Called when an invisible unit turns visible.
	virtual void onUnitHide(BWAPI::Unit unit); //Called when a visible unit becomes invisible.
	virtual void onUnitCreate(BWAPI::Unit unit); //Called when a unit is created. Does not include Zerg Units or Vespene refinaries.
	virtual void onUnitDestroy(BWAPI::Unit unit); //Called when a unit is destroyed through death or other means When a drone becomes an extractor, it is destroyed and the Vespene Geyser turns into an Extractor.
	virtual void onUnitMorph(BWAPI::Unit unit); //Called when an unit changes its UnitType. Zerg buildings, Siege Tanks in Siege Mode or Vespene Geysers recieving a Refinery. It's not called when an unit turns into UnitTypes::Unknown.
	virtual void onUnitRenegade(BWAPI::Unit unit); //Called when an unit changes alliegence thorugh Mind Control or some kind of UMS.
	virtual void onSaveGame(std::string gameName); //Called when the game is saved.
	virtual void onUnitComplete(BWAPI::Unit unit); //Called when an unit changes its status from complete to incomplete.
	//Custom Functions
	virtual void displayInfo(); //Called to display debug info on BW screen
	virtual void processBuildOrder(); //Processes the build order
	virtual void trainUnit(BWAPI::UnitType type); // Trains an unit

private:
	BWAPI::Unit mainHatch;
	BWAPI::Position startPos;
};