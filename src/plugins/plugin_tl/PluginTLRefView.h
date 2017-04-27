#ifndef PluginTLRefView_H
#define PluginTLRefView_H

#if USE_PLUGIN_TL

#include "../GraphViewRefAbstract.h"

class PluginTL;
class GraphView;
class QVBoxLayout;


class PluginTLRefView: public GraphViewRefAbstract
{
    Q_OBJECT
public:
    explicit PluginTLRefView(QWidget* parent = nullptr);
    virtual ~PluginTLRefView();
    
    void setDate(const Date& date, const ProjectSettings& settings);
    
public slots:
    void zoomX(const double min, const double max);
    void setMarginRight(const int margin);
protected:
    void resizeEvent(QResizeEvent* e);
    
private:
   // GraphView* mGraph;
};

#endif
#endif
