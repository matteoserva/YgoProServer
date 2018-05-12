#include "datautils.h"
#include "data_manager.h"
#include "deck_manager.h"


const unsigned short PRO_VERSION = 0x133A;
bool DataUtils::LoadCards()
{
        ygo::deckManager.LoadLFList();
    if(wcsstr(ygo::deckManager._lfList[0].listName,L"TCG"))
        std::swap(ygo::deckManager._lfList[0],ygo::deckManager._lfList[1]);
                
        if(!ygo::dataManager.LoadDB("cards.cdb"))
    {
                printf("unable to load cards.cdb\n");
                return false;
        }
    ygo::dataManager.LoadDB("LiveUpdate/prerelease.cdb");
    ygo::dataManager.LoadDB("LiveUpdate/fixsetcode.cdb");
    ygo::dataManager.LoadDB("LiveUpdate/official.cdb");
        return true;
}

