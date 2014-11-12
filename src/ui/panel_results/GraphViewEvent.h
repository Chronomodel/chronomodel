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

private slots:
    void saveGraphData();
    
protected:
    void paintEvent(QPaintEvent* e);
    void refresh();
    
private:
    Event* mEvent;
};

#endif
