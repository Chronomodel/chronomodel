#ifndef FactsList_H
#define FactsList_H

#include <QListWidget>
#include "Fact.h"


class FactsList: public QListWidget
{
    Q_OBJECT
public:
    FactsList(QWidget* parent = 0);
    ~FactsList();

public slots:
    void factClickedAt(int row);
    void setCurrentFact(Fact* fact);
    void resetFactsList();
    
protected slots:
    void keyPressEvent(QKeyEvent* keyEvent);
};

#endif
