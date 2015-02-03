#include "ChronoApp.h"
#include <QtWidgets>

ChronoApp::ChronoApp(int& argc, char** argv):QApplication(argc, argv)
{
    
}

ChronoApp::~ChronoApp()
{
    
}

bool ChronoApp::event(QEvent* e)
{
    // TODO : handle file drag directly onto the application icon on Mac
    
    /*QFileOpenEvent* foe = dynamic_cast<QFileOpenEvent*>(e);
    if(foe)
    {
        QDesktopServices::openUrl(QUrl("http://www.chronomodel.fr", QUrl::TolerantMode));
        return true;
    }*/
    if(e->type() == QEvent::FileOpen)
    {
        QMessageBox box;
        box.setText(static_cast<QFileOpenEvent*>(e)->file());
        box.exec();
    }
    return QApplication::event(e);
}