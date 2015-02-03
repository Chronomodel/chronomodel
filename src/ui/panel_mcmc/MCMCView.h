#ifndef MCMCView_H
#define MCMCView_H

#include <QWidget>


class MCMCView: public QWidget
{
    Q_OBJECT
public:
    MCMCView(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~MCMCView();
};

#endif
