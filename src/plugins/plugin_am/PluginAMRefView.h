#ifndef PluginAMRefView_H
#define PluginAMRefView_H

#if USE_PLUGIN_AM

#include "../GraphViewRefAbstract.h"

class PluginAM;
class GraphView;
class QLocale;


class PluginAMRefView: public GraphViewRefAbstract
{
    Q_OBJECT
public:
    explicit PluginAMRefView(QWidget* parent = 0);
    virtual ~PluginAMRefView();
    
    void setDate(const Date& d, const ProjectSettings& settings);
    
public slots:
    void zoomX(const double min, const double max);
    void setMarginRight(const int margin);
protected:
    void resizeEvent(QResizeEvent* e);
    
private:
    GraphView* mGraph;
};

#endif
#endif
