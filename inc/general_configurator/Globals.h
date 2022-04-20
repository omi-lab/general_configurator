#ifndef general_configurator_Globals_h
#define general_configurator_Globals_h

#include "tp_utils/StringID.h"

#include "json.hpp"

#include <unordered_set>

namespace general_configurator
{

//##################################################################################################
struct Module
{
  tp_utils::StringID name;
  std::string path;
  std::string type; //!< The TEMPLATE value either lib, app.
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

}

#endif
