#include "general_configurator/MainWindow.h"
#include "general_configurator/Cache.h"

#include "tp_utils_filesystem/Globals.h"

#include <QApplication>
#include <QStandardPaths>

using namespace general_configurator;

//##################################################################################################
int main(int argc, char* argv[])
{
  tp_utils_filesystem::init();

  QApplication app(argc, argv);
  app.setOrganizationName("Tdp");
  app.setApplicationName("Configurator");

  general_configurator::Cache cache(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString());

  MainWindow mainWindow(&cache);
  mainWindow.showMaximized();

  return app.exec();
}
