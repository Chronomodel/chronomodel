#ifndef PluginTLRefView_H
#define PluginTLRefView_H

#if USE_PLUGIN_14C

#include "../GraphViewRefAbstract.h"

class GraphView;
class QVBoxLayout;


class PluginTLRefView: public GraphViewRefAbstract
{
    Q_OBJECT
public:
    explicit PluginTLRefView(QWidget* parent = 0);
    virtual ~PluginTLRefView();
    
    void setDate(const Date& date, const ProjectSettings& settings);
    
public slots:
    void zoomX(double min, double max);
    
protected:
    void resizeEvent(QResizeEvent* e);
    
private:
    GraphView* mGraph;
};

#endif
#endif

