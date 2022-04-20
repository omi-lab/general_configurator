#include "general_configurator/Cache.h"

#include "tp_utils/FileUtils.h"
#include "tp_utils/DebugUtils.h"

#include "json.hpp"

namespace general_configurator
{

//##################################################################################################
struct Cache::Private
{
  Cache* q;
  const std::string cacheDirectory;

  std::vector<std::string> sourceRepos;
  std::vector<Module> modules;

  //################################################################################################
  Private(Cache* q_, const std::string& cacheDirectory_):
    q(q_),
    cacheDirectory(cacheDirectory_)
  {
    tp_utils::mkdir(cacheDirectory, tp_utils::CreateFullPath::Yes);
  }

  //################################################################################################
  std::string indexPath()
  {
    return tp_utils::pathAppend(cacheDirectory, "index.json");
  }

  //################################################################################################
  void save()
  {
    nlohmann::json j;

    j["sourceRepos"] = nlohmann::json::array();
    for(const auto& sourceRepo : sourceRepos)
      j["sourceRepos"].push_back(sourceRepo);

    j["modules"] = nlohmann::json::array();
    for(const auto& module : modules)
      j["modules"].push_back(module.saveState());

    tp_utils::writeJSONFile(indexPath(), j, 2);

    q->changed();
  }

  //################################################################################################
  void load()
  {
    nlohmann::json j = tp_utils::readJSONFile(indexPath());

    sourceRepos.clear();
    if(auto i=j.find("sourceRepos"); i!=j.end() && i->is_array())
      for(const auto& jj : *i)
        if(jj.is_string())
          sourceRepos.push_back(jj);

    modules.clear();
    if(auto i=j.find("modules"); i!=j.end() && i->is_array())
      for(const auto& jj : *i)
        modules.emplace_back().loadState(jj);
  }
};

//##################################################################################################
Cache::Cache(const std::string& cacheDirectory):
  d(new Private(this, cacheDirectory))
{
  d->load();
}

//##################################################################################################
Cache::~Cache()
{
  delete d;
}

//##################################################################################################
const std::string& Cache::cacheDirectory() const
{
  return d->cacheDirectory;
}

//##################################################################################################
void Cache::setSourceRepos(const std::vector<std::string>& sourceRepos)
{
  d->sourceRepos = sourceRepos;
  d->save();
}

//##################################################################################################
const std::vector<std::string>& Cache::sourceRepos() const
{
  return d->sourceRepos;
}

//##################################################################################################
void Cache::setModules(const std::vector<Module>& modules)
{
  d->modules = modules;
  d->save();
}

//##################################################################################################
const std::vector<Module>& Cache::modules() const
{
  return d->modules;
}

//##################################################################################################
Module Cache::module(const tp_utils::StringID& name) const
{
  for(const auto& module : d->modules)
    if(module.name == name)
      return module;
  return {};
}

}
