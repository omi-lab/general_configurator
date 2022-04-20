#ifndef general_configurator_UpdateCache_h
#define general_configurator_UpdateCache_h

#include "general_configurator/Globals.h"

namespace tp_utils
{
class Progress;
}

namespace general_configurator
{
class Cache;

//##################################################################################################
bool updateCache(Cache& cache, tp_utils::Progress* progress);

}

#endif
