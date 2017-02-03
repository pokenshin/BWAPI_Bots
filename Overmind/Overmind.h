#pragma once
#include <BWAPI.h>

class Overmind : public BWAPI::AIModule
{
public:
	virtual void onStart(); //Chamado somente uma vez no come�o do jogo.
	virtual void onEnd(bool isWinner); //Chamado somente uma vez no fim do jogo. isWinner determina se o bot ganhou. Retornar� falso se for um replay.
	virtual void onFrame(); //Chamado uma vez a cada frame l�gico no Broodwar
	virtual void onSendText(std::string text); //Chamado quando o usu�rio tenta mandar uma mensagem de texto. Usado principalmente para debung ingame via comandos de texto.
	virtual void onReceiveText(BWAPI::Player player, std::string text); //Chamado quando o cliente recebe uma mensagem de outro player. Pode ser usado para co-op ou chat.
	virtual void onPlayerLeft(BWAPI::Player player); //Chamado quando um jogador deixa a partida
	virtual void onNukeDetect(BWAPI::Position target); //Chamado quando "Nuclear Launch Detected". O target � onde o ponto de impacto foi marcado. Se n�o for um lugar vis�vel do mapa, o target vai receber Positions::Unknown
	virtual void onUnitDiscover(BWAPI::Unit unit); //Chamado quando determinada unidade � descoberta. Inclui o status do Flag::CompleteMapInformation
	virtual void onUnitEvade(BWAPI::Unit unit); //Chamado quando determinada unidade sai da vis�o. Inclui o status do Flag::CompleteMapInformation
	virtual void onUnitShow(BWAPI::Unit unit); //Chamado quando uma unidade invisivel se torna visivel.
	virtual void onUnitHide(BWAPI::Unit unit); //Chamado quando uma unidade vis�vel se torna invis�vel.
	virtual void onUnitCreate(BWAPI::Unit unit); //Chamado quando uma unidade � criada. Morphing do zerg n�o conta como unidade criada, nem estruturas sobre o Vespene.
	virtual void onUnitDestroy(BWAPI::Unit unit); //Chamado quando uma unidade � destru�da via morte ou outros meios. Quando um drone vira um Extractor, ele � destru�do e o Vespene Geyser vira um Extractor.
	virtual void onUnitMorph(BWAPI::Unit unit); //Chamado quando uma unidade muda seu UnitType. Constru��es Zergs, Siege Tanks em Siege Mode ou Vespenes recebendo Refinery. N�o � chamado se a unidade virar um UnitTypes::Unknown.
	virtual void onUnitRenegade(BWAPI::Unit unit); //Chamado quando uma unidade muda de dono, via Mind Control ou Use Map Settings.
	virtual void onSaveGame(std::string gameName); //Chamado quando um jogo � salvo, com a string do nome do arquivo na variavel gameName
	virtual void onUnitComplete(BWAPI::Unit unit); //chamado quando uma unidade muda seu status de completa para incompleta.

};