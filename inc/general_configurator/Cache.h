#ifndef general_configurator_Cache_h
#define general_configurator_Cache_h

#include "general_configurator/Globals.h"

#include "tp_utils/CallbackCollection.h"

namespace general_configurator
{

//##################################################################################################
class Cache
{
public:
  //################################################################################################
  Cache(const std::string& cacheDirectory);

  //################################################################################################
  ~Cache();

  //################################################################################################
  const std::string& cacheDirectory() const;

  //################################################################################################
  void setSourceRepos(const std::vector<std::string>& sourceRepos);

  //################################################################################################
  const std::vector<std::string>& sourceRepos() const;

  //################################################################################################
  void setModules(const std::vector<Module>& modules);

  //################################################################################################
  const std::vector<Module>& modules() const;

  //################################################################################################
  Module module(const tp_utils::StringID& name) const;  

  //################################################################################################
  bool isDependency(const tp_utils::StringID& name, const tp_utils::StringID& of) const;

  //################################################################################################
  void sortModules(std::vector<Module>& modules) const;

  //################################################################################################
  std::vector<tp_utils::StringID> sortDependencies(const std::unordered_set<tp_utils::StringID>& dependencies) const;

  //################################################################################################
  tp_utils::CallbackCollection<void()> changed;

private:
  struct Private;
  friend struct Private;
  Private* d;
};

}

#endif
