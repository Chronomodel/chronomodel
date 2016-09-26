#ifndef PhasesList_H
#define PhasesList_H

#include <QListWidget>

class QListWidgetItem;
class Phase;
class Fact;


class PhasesList: public QListWidget
{
    Q_OBJECT
public:
    PhasesList(QWidget* parent = 0);
    ~PhasesList();

public slots:
    void updatePhase();
    void phaseClickedAt(int row);
    void setCurrentPhase(Phase* phase);
    void resetPhasesList(Phase* currentPhase = 0);
    
    void checkFactPhases(Fact* fact);
    
protected slots:
    void itemHasChanged(QListWidgetItem* item);
    void keyPressEvent(QKeyEvent* keyEvent);
};

#endif
