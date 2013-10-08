#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QAction;
class QMenu;
class MainWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);

signals:

private:
    void loadPlugins();

private:
    MainWidget* mMainWidget;
    QMenu* mDatationMenu;
};

#endif // MAINWINDOW_H
