#ifndef GraphViewPhase_H
#define GraphViewPhase_H

#include "GraphViewResults.h"

class Phase;


class GraphViewPhase: public GraphViewResults
{
    Q_OBJECT
public:
    explicit GraphViewPhase(QWidget *parent = 0);
    virtual ~GraphViewPhase();
    
    void setPhase(Phase* phase);
    
    void setGraphFont(const QFont& font);
    
protected:
    void paintEvent(QPaintEvent* e);
    void refresh();
    void updateLayout();
    
protected slots:
    void showDuration(bool show);
    
private:
    Phase* mPhase;
    
    GraphView* mDurationGraph;
    Button* mShowDuration;
};

#endif
