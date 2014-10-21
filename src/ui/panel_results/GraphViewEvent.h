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
    
    // ----------------------------------------------------------------
    
    void showHisto(bool showAllChains, const QList<bool>& showChainList, bool showHPD, int thresholdHPD);
    void showTrace(const QList<bool>& showChainList);
    void showAccept(const QList<bool>& showChainList);
    
    // ----------------------------------------------------------------
    
protected:
    void paintEvent(QPaintEvent* e);
    
private:
    Event* mEvent;
};

#endif
