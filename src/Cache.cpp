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
    tp_utils::mkdir(cacheDirectory, TPCreateFullPath::Yes);
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

//##################################################################################################
bool Cache::isDependency(const tp_utils::StringID& name, const tp_utils::StringID& of) const
{
  std::vector<tp_utils::StringID> deps;
  deps.push_back(of);

  for(size_t i=0; i<deps.size(); i++)
  {
    auto m = module(deps.at(i));

    if(m.name == name)
      return true;

    for(const auto& dep : m.dependencies)
      if(!tpContains(deps, dep))
        deps.push_back(dep);
  }

  return false;
}

//##################################################################################################
void Cache::sortModules(std::vector<Module>& modules) const
{
  auto sordDeps = [&]
  {
    for(;;)
    {
      bool changed=false;

      for(size_t i=1; i<modules.size(); i++)
      {
        for(size_t j=i-1; j<modules.size(); j--)
        {
          if(isDependency(modules[i].name, modules[j].name))
          {
            std::swap(modules[i-1], modules[i]);
            changed = true;
            break;
          }
        }
      }

      if(!changed)
        break;
    }
  };

  sordDeps();

  std::vector<std::string> prefixes;
  for(const auto& m : modules)
    if(auto p=m.prefix(); !tpContains(prefixes, p))
      prefixes.push_back(p);

  std::vector<Module> modulesOld;
  modulesOld.swap(modules);
  modules.reserve(modulesOld.size());
  for(const auto& p : prefixes)
    for(const auto& m : modulesOld)
      if(m.prefix() == p)
        modules.push_back(m);

  sordDeps();
}

//##################################################################################################
std::vector<tp_utils::StringID> Cache::sortDependencies(const std::unordered_set<tp_utils::StringID>& dependencies) const
{
  std::unordered_set<tp_utils::StringID> deps = dependencies;
  std::vector<tp_utils::StringID> result;
  result.reserve(deps.size());

  for(const auto& m : d->modules)
  {
    if(auto i=deps.find(m.name); i!=deps.end())
    {
      result.push_back(m.name);
      deps.erase(i);
    }
  }

  for(const auto& dep : deps)
    result.push_back(dep);

  return result;
}

}
