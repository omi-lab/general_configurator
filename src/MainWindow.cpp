#include "general_configurator/MainWindow.h"
#include "general_configurator/UpdateCache.h"
#include "general_configurator/Cache.h"
#include "general_configurator/Generate.h"

#include "tp_qt_widgets/BlockingOperationDialog.h"
#include "tp_qt_widgets/FileDialogLineEdit.h"

#include "tp_utils/RefCount.h"
#include "tp_utils/FileUtils.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QApplication>
#include <QFileDialog>
#include <QSettings>

namespace general_configurator
{

//##################################################################################################
struct MainWindow::Private
{
  TP_REF_COUNT_OBJECTS("general_configurator::MainWindow::Private");
  TP_NONCOPYABLE(Private);

  Q* q;
  Cache* cache;

  QPlainTextEdit* sourceRepos{nullptr};
  QListWidget* appTemplates{nullptr};
  QListWidget* libraries{nullptr};

  tp_qt_widgets::FileDialogLineEdit* rootPath{nullptr};
  QLineEdit* appPath{nullptr};

  QLineEdit* modulePrefix{nullptr};
  QLineEdit* moduleSuffix{nullptr};
  QLineEdit* gitRepo{nullptr};

  Module appTemplateModule;

  //################################################################################################
  Private(Q* q_, Cache* cache_):
    q(q_),
    cache(cache_)
  {
    cacheChanged.connect(cache->changed);
  }

  //################################################################################################
  void updateCacheClicked()
  {
    std::vector<std::string> s;
    tpSplit(s, sourceRepos->toPlainText().toStdString(), '\n', TPSplitBehavior::SkipEmptyParts);
    cache->setSourceRepos(s);

    tp_qt_widgets::BlockingOperationDialog::exec(poll, "Updating the cache", q, [&](tp_utils::Progress* progress)
    {
      return updateCache(*cache, progress);
    });
  }

  //################################################################################################
  void sortCacheClicked()
  {
    auto modules = cache->modules();
    cache->sortModules(modules);
    cache->setModules(modules);
  }

  //################################################################################################
  void sortSubmodulesClicked()
  {
    auto dir = rootPath->text();

    auto path = QFileDialog::getOpenFileName(q, "Select submodules.pri", dir, "submodules.pri").toStdString();
    if(path.empty())
      return;

    auto subdirs = parseSubmodules(path);
    if(subdirs.empty())
      return;

    std::string submodules = generateSubmodules(*cache, std::string(), subdirs);
    if(submodules.empty())
      return;

    tp_utils::writeTextFile(path, submodules);
  }

  //################################################################################################
  void resetLibraries()
  {
    libraries->clear();
    for(const auto& module : cache->modules())
    {
      if(module.type == "lib" || module.type == "subdirs" )
      {
        auto item = new QListWidgetItem(QString::fromStdString(module.name.toString()));
        item->setCheckState(Qt::Unchecked);
        libraries->addItem(item);
      }
    }
  }

  //################################################################################################
  void populateUI()
  {
    {
      sourceRepos->clear();
      QString s;
      for(const auto& sourceRepo : cache->sourceRepos())
        s += QString::fromStdString(sourceRepo) + '\n';
      sourceRepos->setPlainText(s);
    }

    {
      appTemplates->clear();
      for(const auto& module : cache->modules())
      {
        if(module.type == "app")
        {
          auto item = new QListWidgetItem(QString::fromStdString(module.name.toString()));
          appTemplates->addItem(item);
        }
      }

      if(appTemplates->count()>0)
        appTemplates->item(0)->setSelected(true);
    }

    selectedAppTemplateChanged();
    updatePaths();
  }

  //################################################################################################
  tp_utils::Callback<void()> cacheChanged = [&]()
  {
    populateUI();
  };

  //################################################################################################
  bool inPoll=false;
  std::function<bool()> poll = [&]
  {
    if(inPoll)
      return true;

    inPoll=true;
    QApplication::processEvents();
    inPoll=false;

    return true;
  };

  //################################################################################################
  void updatePaths()
  {
    QSettings().setValue("rootPath", rootPath->text());

    std::string rootPath     = this->rootPath->text().toStdString();
    std::string modulePrefix = this->modulePrefix->text().toStdString();
    std::string moduleSuffix = this->moduleSuffix->text().toStdString();

    std::string appPathString = generateAppPathString(rootPath,
                                                      modulePrefix,
                                                      moduleSuffix);

    std::string gitRepoString = generateGitRepoString(appTemplateModule.gitRepoPrefix,
                                                      modulePrefix,
                                                      moduleSuffix);

    appPath->setText(QString::fromStdString(appPathString));
    gitRepo->setText(QString::fromStdString(gitRepoString));
  }

  //################################################################################################
  tp_utils::StringID appTemplateName()
  {
    if(auto i = appTemplates->selectedItems(); !i.empty())
      return i.front()->text().toStdString();
    return tp_utils::StringID();
  }

  //################################################################################################
  void selectedAppTemplateChanged()
  {
    resetLibraries();

    appTemplateModule = cache->module(appTemplateName());

    for(const auto& dependency : appTemplateModule.dependencies)
      checkLibrary(dependency, false, true);

    updatePaths();
  }

  //################################################################################################
  void checkLibrary(const tp_utils::StringID& name, bool partial, bool required)
  {
    auto module = cache->module(name);

    for(const auto& dependency : module.dependencies)
      checkLibrary(dependency, true, required);

    for(int row=0; row<libraries->count(); row++)
    {
      auto item = libraries->item(row);

      if(name == item->text().toStdString())
      {
        if(partial && item->checkState() == Qt::Unchecked)
          item->setCheckState(Qt::PartiallyChecked);
        else if(!partial)
          item->setCheckState(Qt::Checked);

        if(required)
        {
          item->setFlags(item->flags() & (~Qt::ItemIsUserCheckable));
          QFont fnt = item->font();
          fnt.setWeight(QFont::Bold);
          item->setFont(fnt);
        }

        break;
      }
    }
  }
  //################################################################################################
  void uncheckLibrary(const tp_utils::StringID& name)
  {
    for(int row=0; row<libraries->count(); row++)
    {
      auto item = libraries->item(row);

      if(name == item->text().toStdString())
      {
        item->setCheckState(Qt::Unchecked);
        break;
      }
    }

    for(const auto& module : cache->modules())
      if(tpContains(module.dependencies, name))
        uncheckLibrary(module.name);

    for(int row=0; row<libraries->count(); row++)
    {
      auto item = libraries->item(row);
      tp_utils::StringID n = item->text().toStdString();
      if(item->checkState() == Qt::PartiallyChecked && !tpContains(allDependencies(), n))
        uncheckLibrary(n);
    }
  }

  //################################################################################################
  void itemClicked(QListWidgetItem* item)
  {
    if((item->flags() & Qt::ItemIsUserCheckable) != Qt::ItemIsUserCheckable)
      return;

    tp_utils::StringID name = item->text().toStdString();

    if(item->checkState() == Qt::Checked)
      checkLibrary(name, false, false);

    else if(item->checkState() == Qt::Unchecked)
      uncheckLibrary(name);
  }

  //################################################################################################
  std::unordered_set<tp_utils::StringID> selectedLibraries()
  {
    std::unordered_set<tp_utils::StringID> names;
    for(int row=0; row<libraries->count(); row++)
      if(auto item = libraries->item(row); item->checkState() == Qt::Checked)
        names.insert(item->text().toStdString());
    return names;
  }

  //################################################################################################
  std::unordered_set<tp_utils::StringID> allDependencies()
  {
    std::unordered_set<tp_utils::StringID> names;
    for(int row=0; row<libraries->count(); row++)
    {
      auto item = libraries->item(row);
      tp_utils::StringID name = item->text().toStdString();

      if(((item->flags() & Qt::ItemIsUserCheckable) != Qt::ItemIsUserCheckable) || item->checkState() == Qt::Checked)
        names.insert(name);

      if(item->checkState() != Qt::Unchecked)
        for(const auto& dependency : cache->module(name).dependencies)
          names.insert(dependency);
    }
    return names;
  }
};

//##################################################################################################
MainWindow::MainWindow(Cache* cache):
  d(new Private(this, cache))
{
  setWindowTitle("tdp-libs Configurator");

  auto mainLayout = new QGridLayout(this);

  auto t = [&](auto l, auto title)
  {
    auto lbl = new QLabel(title);
    lbl->setWordWrap(true);
    l->addWidget(lbl);
  };

  {
    auto widget = new QGroupBox();
    mainLayout->addWidget(widget, 0, 0);

    auto l = new QVBoxLayout(widget);

    t(l, "<b>0. Source repos</b><br>"
         "Set the list of git repos that will be fetched and parsed to build a list of libraries "
         "and app templates. The module will be cloned and if it has a submodules.pri file that "
         "will be parsed and all the submodules will also be cloned.");
    d->sourceRepos = new QPlainTextEdit();
    l->addWidget(d->sourceRepos);

    {
      auto button = new QPushButton("Update cache");
      l->addWidget(button);
      connect(button, &QPushButton::clicked, this, [&]{d->updateCacheClicked();});
    }

    {
      auto button = new QPushButton("Sort cache");
      l->addWidget(button);
      connect(button, &QPushButton::clicked, this, [&]{d->sortCacheClicked();});
    }

    {
      auto button = new QPushButton("Sort existing submodules.pri");
      l->addWidget(button);
      connect(button, &QPushButton::clicked, this, [&]{d->sortSubmodulesClicked();});
    }
  }

  {
    auto widget = new QGroupBox();
    mainLayout->addWidget(widget, 0, 1);

    auto l = new QVBoxLayout(widget);

    t(l, "<b>1. App template</b><br>"
         "Select the template that you would like to create your new program from.");
    d->appTemplates = new QListWidget();
    d->appTemplates->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    l->addWidget(d->appTemplates);

    connect(d->appTemplates, &QListWidget::itemSelectionChanged, this, [&]{d->selectedAppTemplateChanged();});
  }

  {
    auto widget = new QGroupBox();
    mainLayout->addWidget(widget, 1, 0);

    auto l = new QVBoxLayout(widget);

    t(l, "<b>2. Libraries</b><br>"
         "Select the libraries that you want to use in your program.");
    d->libraries = new QListWidget();
    l->addWidget(d->libraries);

    connect(d->libraries, &QListWidget::itemClicked, this, [&](QListWidgetItem* item){d->itemClicked(item);});
  }

  {
    auto widget = new QGroupBox();
    mainLayout->addWidget(widget, 1, 1);

    auto l = new QVBoxLayout(widget);

    t(l, "<b>3. Generate program</b><br>"
         "Configure the name and paths of the program you want to generate.");

    l->addSpacing(20);

    {
      auto ll = new QHBoxLayout();
      l->addLayout(ll);
      auto lbl = new QLabel("Root path: ");
      lbl->setFixedWidth(80);
      ll->addWidget(lbl);
      d->rootPath = new tp_qt_widgets::FileDialogLineEdit();
      d->rootPath->setText(QSettings().value("rootPath").toString());
      connect(d->rootPath, &tp_qt_widgets::FileDialogLineEdit::selectionChanged, this, [&]{d->updatePaths();});
      ll->addWidget(d->rootPath, 0, Qt::AlignLeft);
    }

    l->addSpacing(10);

    {
      auto ll = new QHBoxLayout();
      l->addLayout(ll);
      auto lbl = new QLabel("App name: ");
      lbl->setFixedWidth(80);
      ll->addWidget(lbl);
      d->modulePrefix = new QLineEdit();
      d->modulePrefix->setPlaceholderText("prefix");
      d->modulePrefix->setMaximumWidth(60);
      connect(d->modulePrefix, &QLineEdit::textChanged, this, [&]{d->updatePaths();});
      ll->addWidget(d->modulePrefix);

      ll->addWidget(new QLabel("_"));

      d->moduleSuffix = new QLineEdit();
      d->moduleSuffix->setPlaceholderText("suffix");
      d->moduleSuffix->setMaximumWidth(120);
      connect(d->moduleSuffix, &QLineEdit::textChanged, this, [&]{d->updatePaths();});
      ll->addWidget(d->moduleSuffix);

      ll->addStretch();
    }

    l->addSpacing(10);

    {
      auto ll = new QHBoxLayout();
      l->addLayout(ll);
      auto lbl = new QLabel("App path: ");
      lbl->setFixedWidth(80);
      ll->addWidget(lbl);
      d->appPath = new QLineEdit();
      d->appPath->setReadOnly(true);
      ll->addWidget(d->appPath);
    }

    l->addSpacing(10);

    {
      auto ll = new QHBoxLayout();
      l->addLayout(ll);
      auto lbl = new QLabel("Git repo: ");
      lbl->setFixedWidth(80);
      ll->addWidget(lbl);
      d->gitRepo = new QLineEdit();
      d->gitRepo->setReadOnly(true);
      ll->addWidget(d->gitRepo);
    }

    l->addSpacing(10);

    l->addSpacing(20);

    auto generateButton = new QPushButton("Generate");
    l->addWidget(generateButton, 0, Qt::AlignLeft);

    connect(generateButton, &QPushButton::clicked, this, [&]
    {
      tp_qt_widgets::BlockingOperationDialog::exec(d->poll, "Updating the cache", this, [&](tp_utils::Progress* progress)
      {
        return generateApp(*d->cache,
                           d->appTemplateName(),
                           d->rootPath->text().toStdString(),
                           d->modulePrefix->text().toStdString(),
                           d->moduleSuffix->text().toStdString(),
                           d->selectedLibraries(),
                           d->allDependencies(),
                           progress);
      });
    });

    l->addStretch();
  }

  d->populateUI();
}

//##################################################################################################
MainWindow::~MainWindow()
{
  delete d;
}

}
