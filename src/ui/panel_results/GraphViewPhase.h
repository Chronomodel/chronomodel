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
    
    // ----------------------------------------------------------------
    
    void showHisto(bool showAlpha, bool showBeta, bool showPredict, bool showAllChains, const QList<bool>& showChainList, bool showHPD, int thresholdHPD);
    void showTrace(bool showAlpha, bool showBeta, bool showPredict, const QList<bool>& showChainList);
    void showAccept(bool showAlpha, bool showBeta, bool showPredict, const QList<bool>& showChainList);
    
    // ----------------------------------------------------------------
    
protected:
    void paintEvent(QPaintEvent* e);
    
private:
    Phase* mPhase;
};

#endif
