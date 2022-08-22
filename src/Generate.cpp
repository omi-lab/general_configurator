#include "general_configurator/Generate.h"
#include "general_configurator/Cache.h"

#include "tp_utils/Progress.h"
#include "tp_utils/FileUtils.h"

namespace general_configurator
{

//##################################################################################################
bool generateApp(const Cache& cache,
                 const tp_utils::StringID& templateModuleId,
                 const std::string& rootPath,
                 const std::string& modulePrefix,
                 const std::string& moduleSuffix,
                 const std::unordered_set<tp_utils::StringID>& selectedLibraries,
                 const std::unordered_set<tp_utils::StringID>& allDependencies,
                 tp_utils::Progress* progress)
{
  Module templateModule = cache.module(templateModuleId);

  std::string moduleName = generateModuleName(modulePrefix,
                                              moduleSuffix);

  std::string topLevelPathString = generateTopLevelPathString(rootPath,
                                                              modulePrefix,
                                                              moduleSuffix);

  std::string appPathString = generateAppPathString(rootPath,
                                                    modulePrefix,
                                                    moduleSuffix);

  std::string gitRepoString = generateGitRepoString(templateModule.gitRepoPrefix,
                                                    modulePrefix,
                                                    moduleSuffix);


  //-- Create the module directory -----------------------------------------------------------------
  {
    progress->addMessage("Create app module path: " + appPathString);
    if(!tp_utils::mkdir(appPathString, tp_utils::CreateFullPath::Yes))
    {
      progress->addError("Failed to create module directory!");
      return false;
    }
    progress->setProgress(0.05f);
  }

  //-- Clone the template into the module directory ------------------------------------------------
  {
    std::string cloneCommand = "git clone " + templateModule.gitRepoURL + " .";
    progress->addMessage("Clone template: " + cloneCommand);
    int ret = runCommand(appPathString, cloneCommand);
    if(ret != 0)
    {
      progress->addError("Failed to clone: " + templateModule.gitRepoURL);
      progress->addError("Return code: " + std::to_string(ret));
      return false;
    }
    progress->setProgress(0.2f);
  }

  //-- Delete the .git directory -------------------------------------------------------------------
  {
    std::string gitDir = tp_utils::pathAppend(appPathString, ".git");
    progress->addMessage("Delete .git directory: " + gitDir);
    if(!tp_utils::rm(gitDir, true))
    {
      progress->addError("Failed to delete .git directory!");
      return false;
    }
    progress->setProgress(0.25f);
  }

  //-- Rename files --------------------------------------------------------------------------------
  {
    progress->addMessage("Rename files.");

    auto rename = [&](const std::string& startsWith, const std::string& to)
    {
      std::string renameCommand = "find . -depth -name '";
      renameCommand += startsWith + "*' -exec bash -c ' mv $0 ${0/";
      renameCommand += startsWith + "/";
      renameCommand += to + "}' {} \\;";

      int ret = runCommand(appPathString, renameCommand);
      if(ret != 0)
      {
        progress->addError("Failed to rename: " + startsWith + " to: " + to);
        progress->addError("Return code: " + std::to_string(ret));
        return false;
      }

      return true;
    };

    if(!rename(templateModule.name.toString(), moduleName))
      return false;

    if(!rename(templateModule.suffix(), moduleSuffix))
      return false;

    progress->setProgress(0.3f);
  }

  //-- Search replace module names -----------------------------------------------------------------
  {
    progress->addMessage("Replace module names.");

    auto replace = [&](const std::string& from, const std::string& to)
    {
      std::string replaceCommand = "find . -name '*' -type f -exec bash -c ' sed -i '' -e 's/";
      replaceCommand += from + "/";
      replaceCommand += to + "/g' $0' {} \\;";

      int ret = runCommand(appPathString, replaceCommand);
      if(ret != 0)
      {
        progress->addError("Failed to replace: " + from + " to: " + to);
        progress->addError("Return code: " + std::to_string(ret));
        return false;
      }

      return true;
    };

    if(!replace(templateModule.name.toString(), moduleName))
      return false;

    if(!replace(templateModule.suffix(), moduleSuffix))
      return false;

    progress->setProgress(0.35f);
  }

  //-- Generate submodules.pri ---------------------------------------------------------------------
  {
    progress->addMessage("Generate submodules.");

    std::string submodules = generateSubmodules(cache, moduleName, allDependencies);
    std::string submodulesFile = tp_utils::pathAppend(appPathString, "submodules.pri");
    tp_utils::writeTextFile(submodulesFile, submodules);

    progress->setProgress(0.4f);
  }

  //-- Generate dependencies.pri -------------------------------------------------------------------
  {
    progress->addMessage("Generate dependencies.");

    std::string dependencies;

    for(const auto& m : selectedLibraries)
      dependencies += "DEPENDENCIES += " + m.toString() + "\n";

    dependencies += "\nINCLUDEPATHS += " + moduleName + "/inc\n";

    std::string submodulesFile = tp_utils::pathAppend(appPathString, "dependencies.pri");
    tp_utils::writeTextFile(submodulesFile, dependencies);

    progress->setProgress(0.45f);
  }

  //-- Git Init ------------------------------------------------------------------------------------
  {
    progress->addMessage("Git init.");

    std::string initCommand = "git init";
    if(int ret = runCommand(appPathString, initCommand); ret != 0)
    {
      progress->addError("Failed to init.");
      progress->addError("Return code: " + std::to_string(ret));
      return false;
    }

    progress->setProgress(0.5f);
  }

  //-- Git Remote Add ------------------------------------------------------------------------------
  {
    progress->addMessage("Git remote add.");

    std::string initCommand = "git remote add origin " + gitRepoString;
    if(int ret = runCommand(appPathString, initCommand); ret != 0)
    {
      progress->addError("Failed to add git remote: " + gitRepoString);
      progress->addError("Return code: " + std::to_string(ret));
      return false;
    }

    progress->setProgress(0.55f);
  }

  //-- Copy top level files ------------------------------------------------------------------------
  {
    auto copy = [&](const std::string& srcName, const std::string& dstName)
    {
      auto from = tp_utils::pathAppend(appPathString, srcName);
      auto to = tp_utils::pathAppend(topLevelPathString, dstName);
      tp_utils::copyFile(from, to);
    };

    copy("Makefile.top", "Makefile");
    copy("CMakeFiles.top", "CMakeFiles.txt");
    copy(moduleSuffix + ".pro", moduleSuffix + ".pro");
  }

  //-- tpUpdate ------------------------------------------------------------------------------------
  {
    progress->addMessage("Run tpUpdate.");

    if(int ret = runCommand(topLevelPathString, "tpUpdate"); ret != 0)
    {
      progress->addError("Failed to run tpUpdate.");
      progress->addError("Return code: " + std::to_string(ret));
      return false;
    }

    progress->setProgress(1.0f);
  }

  return true;
}

//##################################################################################################
std::string generateSubmodules(const Cache& cache,
                               const std::string& moduleName,
                               const std::unordered_set<tp_utils::StringID>& allDependencies)
{
  std::string submodules;
  std::string previousPrefix;

  for(const auto& m : cache.sortDependencies(allDependencies))
  {
    std::string prefix = extractPrefix(m.toString());
    if(!previousPrefix.empty() && previousPrefix != prefix)
      submodules += "\n";
    previousPrefix.swap(prefix);

    submodules += "SUBDIRS += " + m.toString() + "\n";
  }

  submodules += "\n";

  if(!moduleName.empty())
    submodules += "SUBDIRS += " + moduleName + "\n\n";

  return submodules;
}
}
