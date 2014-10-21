#ifndef GraphViewDate_H
#define GraphViewDate_H

#include "GraphViewResults.h"

class Date;


class GraphViewDate: public GraphViewResults
{
    Q_OBJECT
public:
    explicit GraphViewDate(QWidget *parent = 0);
    virtual ~GraphViewDate();
    
    void setDate(Date* date);
    
    // ----------------------------------------------------------------
    
    void showHisto(bool showTheta, bool showSigma, bool showDelta, bool showCalib, bool showAllChains, const QList<bool>& showChainList, bool showHPD, int thresholdHPD);
    void showTrace(bool showTheta, bool showSigma, bool showDelta, const QList<bool>& showChainList);
    void showAccept(bool showTheta, bool showSigma, bool showDelta, const QList<bool>& showChainList);
    
    // ----------------------------------------------------------------
    
protected:
    void paintEvent(QPaintEvent* e);
    
private:
    Date* mDate;
};


#endif
