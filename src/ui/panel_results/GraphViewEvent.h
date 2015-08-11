#ifndef GraphViewEvent_H
#define GraphViewEvent_H

#include "GraphViewResults.h"

class Event;


class GraphViewEvent: public GraphViewResults
{
    Q_OBJECT
public:
    explicit GraphViewEvent(QWidget *parent = 0);
    virtual ~GraphViewEvent();
    
    void setEvent(Event* event);
    
    void generateCurves(TypeGraph typeGraph, Variable variable);
    void updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, bool showCredibility, bool showCalib, bool showWiggle);
    
protected:
    void paintEvent(QPaintEvent* e);
    
private:
    Event* mEvent;
};

#endif
