#include "general_configurator/Globals.h"

#include "tp_utils/FileUtils.h"
#include "tp_utils/JSONUtils.h"

namespace general_configurator
{

//##################################################################################################
std::string extractPrefix(const std::string& name)
{
  std::vector<std::string> parts;
  tpSplit(parts, name, '_');
  if(!parts.empty())
    return parts.front();

  return {};
}

//##################################################################################################
std::string Module::prefix() const
{
  return extractPrefix(name.toString());
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
  j["gitRepoURL"] = gitRepoURL;
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
  gitRepoURL = TPJSONString(j, "gitRepoURL");
  gitRepoPrefix = TPJSONString(j, "gitRepoPrefix");

  dependencies.clear();
  if(auto i=j.find("dependencies"); i!=j.end() && i->is_array())
    for(const auto& jj : *i)
      if(jj.is_string())
        dependencies.insert(std::string(jj));
}

//##################################################################################################
int runCommand(const std::string& workingDirectory, const std::string& command)
{
  std::string s = "cd " + workingDirectory + " && " + command;
  return std::system(s.c_str());
}

//##################################################################################################
std::string generateModuleName(const std::string& modulePrefix,
                               const std::string& moduleSuffix)
{
  return modulePrefix + '_' + moduleSuffix;
}

//##################################################################################################
std::string generateTopLevelPathString(const std::string& rootPath,
                                       const std::string& modulePrefix,
                                       const std::string& moduleSuffix)
{
  std::string appPathString = rootPath;
  appPathString = tp_utils::pathAppend(appPathString, modulePrefix);
  appPathString = tp_utils::pathAppend(appPathString, moduleSuffix);
  return appPathString;
}

//##################################################################################################
std::string generateAppPathString(const std::string& rootPath,
                                  const std::string& modulePrefix,
                                  const std::string& moduleSuffix)
{
  std::string appPathString = generateTopLevelPathString(rootPath, modulePrefix, moduleSuffix);
  appPathString = tp_utils::pathAppend(appPathString, generateModuleName(modulePrefix, moduleSuffix));
  return appPathString;
}

//##################################################################################################
std::string generateGitRepoString(const std::string& gitRepoPrefix,
                                  const std::string& modulePrefix,
                                  const std::string& moduleSuffix)
{
  std::string gitRepoString = gitRepoPrefix;
  gitRepoString += generateModuleName(modulePrefix, moduleSuffix) + ".git";
  return gitRepoString;
}

}
