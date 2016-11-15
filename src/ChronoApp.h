#ifndef CHRONOAPP_H
#define CHRONOAPP_H

#include <QApplication>

class MainWindow;

class ChronoApp: public QApplication
{
public:
    ChronoApp(int& argc, char** argv);
    virtual ~ChronoApp();

protected:
    virtual bool event(QEvent* e);
};

#endif
