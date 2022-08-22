#ifndef general_configurator_Generate_h
#define general_configurator_Generate_h

#include "general_configurator/Globals.h"

namespace tp_utils
{
class Progress;
}

namespace general_configurator
{
class Cache;

//##################################################################################################
bool generateApp(const Cache& cache,
                 const tp_utils::StringID& templateModuleId,
                 const std::string& rootPath,
                 const std::string& modulePrefix,
                 const std::string& moduleSuffix,
                 const std::unordered_set<tp_utils::StringID>& selectedLibraries,
                 const std::unordered_set<tp_utils::StringID>& allDependencies,
                 tp_utils::Progress* progress);

//##################################################################################################
std::string generateSubmodules(const Cache& cache,
                               const std::string& moduleName,
                               const std::unordered_set<tp_utils::StringID>& allDependencies);


}

#endif
