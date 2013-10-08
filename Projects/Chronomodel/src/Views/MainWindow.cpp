#include "MainWindow.h"
#include "MainWidget.h"
#include "PluginInterface.h"
#include <QtCore>
#include <QtGui>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    mMainWidget = new MainWidget();
    this->setCentralWidget(mMainWidget);

    this->setWindowTitle("Chronomodel");

    mDatationMenu = this->menuBar()->addMenu(tr("&Datation"));
    this->loadPlugins();
}

void MainWindow::loadPlugins()
{
    QDir pluginsDir = QDir(qApp->applicationDirPath());

#if defined(Q_OS_WIN)
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
        pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    if(pluginsDir.dirName() == "MacOS")
    {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif
    pluginsDir.cd("plugins");

    foreach(QString fileName, pluginsDir.entryList(QDir::Files))
    {
        QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
        QObject* plugin = loader.instance();
        qDebug() << plugin;
        if(plugin)
        {
            PluginInterface* iPlugin = qobject_cast<PluginInterface*>(plugin);
            if(iPlugin)
            {
                QString menu_name = iPlugin->getMenuName();
                QAction* action = new QAction(menu_name, plugin);
                mDatationMenu->addAction(action);
                connect(action, SIGNAL(triggered()), mMainWidget, SLOT(openDatationDialog()));
            }
        }
    }
}

