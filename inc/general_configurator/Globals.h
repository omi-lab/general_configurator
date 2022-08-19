#ifndef general_configurator_Globals_h
#define general_configurator_Globals_h

#include "tp_utils/StringID.h"

#include "json.hpp"

#include <unordered_set>

namespace general_configurator
{

//##################################################################################################
std::string extractPrefix(const std::string& name);

//##################################################################################################
struct Module
{
  tp_utils::StringID name;
  std::string path;
  std::string type; //!< The TEMPLATE value either lib, app.
  std::string gitRepoURL;
  std::string gitRepoPrefix;
  std::unordered_set<tp_utils::StringID> dependencies;


  //################################################################################################
  std::string prefix() const;

  //################################################################################################
  std::string suffix() const;

  //################################################################################################
  nlohmann::json saveState() const;

  //################################################################################################
  void loadState(const nlohmann::json& j);
};

//##################################################################################################
int runCommand(const std::string& workingDirectory, const std::string& command);

//##################################################################################################
std::string generateModuleName(const std::string& modulePrefix,
                               const std::string& moduleSuffix);

//##################################################################################################
std::string generateTopLevelPathString(const std::string& rootPath,
                                       const std::string& modulePrefix,
                                       const std::string& moduleSuffix);

//##################################################################################################
std::string generateAppPathString(const std::string& rootPath,
                                  const std::string& modulePrefix,
                                  const std::string& moduleSuffix);

//##################################################################################################
std::string generateGitRepoString(const std::string& gitRepoPrefix,
                                  const std::string& modulePrefix,
                                  const std::string& moduleSuffix);

}

#endif
