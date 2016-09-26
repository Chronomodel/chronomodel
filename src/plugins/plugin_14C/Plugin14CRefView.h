#ifndef Plugin14CRefView_H
#define Plugin14CRefView_H

#if USE_PLUGIN_14C

#include "../GraphViewRefAbstract.h"

class Plugin14C;
class GraphView;
class QVBoxLayout;


class Plugin14CRefView: public GraphViewRefAbstract
{
    Q_OBJECT
public:
    explicit Plugin14CRefView(QWidget* parent = 0);
    virtual ~Plugin14CRefView();
    
    void setDate(const Date& date, const ProjectSettings& settings);
    
public slots:
    void zoomX(const float min, const float max);
    void setMarginRight(const int margin);
protected:
    void resizeEvent(QResizeEvent* e);
    
private:
    GraphView* mGraph;
};

#endif
#endif
