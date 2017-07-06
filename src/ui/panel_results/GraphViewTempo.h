#ifndef GRAPHVIEWTEMPO_H
#define GRAPHVIEWTEMPO_H

#include "GraphViewResults.h"

class Phase;


class GraphViewTempo: public GraphViewResults
{
    Q_OBJECT
public:
    explicit GraphViewTempo(QWidget *parent = nullptr);
    virtual ~GraphViewTempo();
    
    void setPhase(Phase* phase);
    void setGraphFont(const QFont& font);

    Phase* mPhase;
    
    void generateCurves(TypeGraph typeGraph, Variable variable);
    void updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, bool showCredibility, bool showCalib, bool showWiggle);


protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* );
    void updateLayout();
    
protected slots:
//    void saveGraphData() const;
    

};

#endif
