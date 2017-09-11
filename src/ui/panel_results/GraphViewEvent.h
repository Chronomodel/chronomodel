#ifndef GRAPHVIEWEVENT_H
#define GRAPHVIEWEVENT_H

#include "GraphViewResults.h"

class Event;


class GraphViewEvent: public GraphViewResults
{
    Q_OBJECT
public:
    explicit GraphViewEvent(QWidget *parent = nullptr);
    virtual ~GraphViewEvent();
    
    void setEvent(Event *event);
    
    void generateCurves(TypeGraph typeGraph, Variable variable);
    void updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, bool showCredibility, bool showCalib, bool showWiggle);
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* );
    void updateLayout();
    
private:
    Event* mEvent;
};

#endif
