#include "general_configurator/Globals.h"

#include "tp_utils/JSONUtils.h"

namespace general_configurator
{

//##################################################################################################
std::string Module::prefix() const
{
  std::vector<std::string> parts;
  tpSplit(parts, name.toString(), '_');
  if(!parts.empty())
    return parts.front();

  return {};
}

//##################################################################################################
std::string Module::suffix() const
{
  std::vector<std::string> parts;
  tpSplit(parts, name.toString(), '_');
  if(!parts.empty())
    tpRemoveAt(parts, 0);

  std::string result;
  for(const auto& part : parts)
  {
    if(!result.empty())
      result += '_';
    result += part;
  }
  return result;
}

//##################################################################################################
nlohmann::json Module::saveState() const
{
  nlohmann::json j;

  j["name"] = name.toString();
  j["path"] = path;
  j["type"] = type;
  j["gitRepoPrefix"] = gitRepoPrefix;

  j["dependencies"] = nlohmann::json::array();
  for(const auto& dependency : dependencies)
    j["dependencies"].push_back(dependency.toString());

  return j;
}

//##################################################################################################
void Module::loadState(const nlohmann::json& j)
{
  name = TPJSONString(j, "name");
  path = TPJSONString(j, "path");
  type = TPJSONString(j, "type");
  gitRepoPrefix = TPJSONString(j, "gitRepoPrefix");

  dependencies.clear();
  if(auto i=j.find("dependencies"); i!=j.end() && i->is_array())
    for(const auto& jj : *i)
      if(jj.is_string())
        dependencies.insert(std::string(jj));
}

}
