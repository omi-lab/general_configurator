#ifndef general_configurator_MainWindow_h
#define general_configurator_MainWindow_h

#include "general_configurator/Globals.h"

#include <QWidget>

namespace general_configurator
{

class Cache;

//##################################################################################################
class MainWindow : public QWidget
{
public:
  //################################################################################################
  MainWindow(Cache* cache);

  //################################################################################################
  ~MainWindow() override;

private:
  struct Private;
  friend struct Private;
  Private* d;
};

}

#endif
