#include "general_configurator/UpdateCache.h"
#include "general_configurator/Cache.h"

#include "tp_utils/Progress.h"
#include "tp_utils/FileUtils.h"

#include <iostream>
#include <algorithm>

namespace general_configurator
{

namespace
{

//##################################################################################################
auto parsePRI = [](const std::string& path, const auto& closure)
{
  std::string  data = tp_utils::readTextFile(path);
  std::vector<std::string> lines;
  tpSplit(lines, data, '\n');

  for(auto line : lines)
  {
    line.erase(std::remove_if(line.begin(), line.end(), [](auto c)
    {
      return isspace(c) || c=='+';
    }), line.end());

    std::vector<std::string> parts;
    tpSplit(parts, line, '=', TPSplitBehavior::SkipEmptyParts);
    if(parts.size() == 2)
      closure(parts);
  }
};

}

//##################################################################################################
bool updateCache(Cache& cache, tp_utils::Progress* progress)
{
  std::string reposDirectory = tp_utils::pathAppend(cache.cacheDirectory(), "repos");
  std::string tmpFile = tp_utils::pathAppend(cache.cacheDirectory(), "tmp.txt");

  {
    auto p = progress->addChildStep("Deleting existing repos", 0.1f);
    tp_utils::rm(reposDirectory, TPRecursive::Yes);
    tp_utils::mkdir(reposDirectory, TPCreateFullPath::Yes);
    p->addMessage("Done");
    p->setProgress(1.0f);
  }

  {
    auto p = progress->addChildStep("Cloning template modules", 0.3f);
    p->addMessage("Cloning repos into: " + reposDirectory);

    const auto& sourceRepos = cache.sourceRepos();
    float f=0;
    for(const auto& sourceRepo : sourceRepos)
    {
      p->addMessage("Cloning: " + sourceRepo);
      int ret = runCommand(reposDirectory, "git clone " + sourceRepo);
      if(ret != 0)
      {
        p->addError("Failed to clone: " + sourceRepo);
        p->addError("Return code: " + std::to_string(ret));
        return false;
      }

      f+=1.0f/float(sourceRepo.size());
      p->setProgress(f);
    }
  }

  {
    auto p = progress->addChildStep("Runing tpUpdate to fetch submodules", 0.9f);
    int ret = runCommand(reposDirectory, "tpUpdate noupdate");
    if(ret != 0)
    {
      p->addError("Failed to run tpUpdate!");
      p->addError("Return code: " + std::to_string(ret));
      return false;
    }

    p->addMessage("Done");
    p->setProgress(1.0f);
  }

  {
    auto p = progress->addChildStep("Reading dependencies", 1.0f);


    auto paths = tp_utils::listDirectories(reposDirectory);
    std::sort(paths.begin(), paths.end());

    std::vector<Module> modules;
    modules.reserve(paths.size());

    float f=0;
    for(const auto& path : paths)
    {
      auto moduleName = tp_utils::filename(path);
      p->addMessage("Reading dependencies of: " + moduleName);

      auto filePath = [&](const std::string& filename)
      {
        return tp_utils::pathAppend(path, filename);
      };

      Module& module = modules.emplace_back();
      module.name = moduleName;

      {
        tp_utils::rm(tmpFile, TPRecursive::Yes);
        runCommand(path, "git config --get remote.origin.url > " + tmpFile);

        module.gitRepoURL = tp_utils::readTextFile(tmpFile);
        module.gitRepoURL.erase(std::remove_if(module.gitRepoURL.begin(), module.gitRepoURL.end(), isspace), module.gitRepoURL.end());
        module.gitRepoPrefix = module.gitRepoURL;

        auto i = module.gitRepoPrefix.rfind(moduleName);
        if(i<module.gitRepoPrefix.size())
          module.gitRepoPrefix = module.gitRepoPrefix.substr(0, i);

        tp_utils::rm(tmpFile, TPRecursive::No);
      }

      parsePRI(filePath("dependencies.pri"), [&](const auto& parts)
      {
        if(parts.front() == "DEPENDENCIES")
          module.dependencies.insert(parts.at(1));
      });

      parsePRI(filePath("vars.pri"), [&](const auto& parts)
      {
        if(parts.front() == "TEMPLATE")
          module.type = parts.at(1);
      });

      f+=1.0f/float(modules.size());
      p->setProgress(f);
    }

    cache.sortModules(modules);
    cache.setModules(modules);
  }

  return true;
}

//##################################################################################################
std::unordered_set<tp_utils::StringID> parseSubmodules(const std::string& path)
{
  std::unordered_set<tp_utils::StringID> subdirs;

  parsePRI(path, [&](const auto& parts)
  {
    if(parts.front() == "SUBDIRS")
      subdirs.insert(parts.at(1));
  });

  return subdirs;
}

}
