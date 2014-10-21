#ifndef GRAPHVIEWWRAPPER_H
#define GRAPHVIEWWRAPPER_H

#include <QScrollArea>
#include <QList>

class QVBoxLayout;
class GraphView;

class GraphViewWrapper: public QScrollArea
{
    Q_OBJECT
public:
    explicit GraphViewWrapper(QWidget *parent = 0);
    virtual ~GraphViewWrapper();

    void addGraph(GraphView* graph);

protected:
    void updateLayout();

protected:
    void resizeEvent(QResizeEvent* e);

protected:
    QWidget* mViewport;
    QList<QWidget*> mItems;
    QVBoxLayout* mLayout;
    int mMinHeight;
};

#endif
