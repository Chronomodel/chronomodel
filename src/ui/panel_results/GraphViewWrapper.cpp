#include "GraphViewWrapper.h"
#include "GraphView.h"
#include <QVBoxLayout>
#include <QtGui>
#include <cmath>

GraphViewWrapper::GraphViewWrapper(QWidget* aParent):QScrollArea(aParent),
  mMinHeight(150)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    mViewport = new QWidget();
    mViewport->setMinimumSize(200, 200);

    mLayout = new QVBoxLayout();
    mLayout->setContentsMargins(0, 0, 0, 0);
    mLayout->setSpacing(0);
    mViewport->setLayout(mLayout);
    setWidget(mViewport);
}

GraphViewWrapper::~GraphViewWrapper()
{

}

void GraphViewWrapper::addGraph(GraphView* graph)
{
    mLayout->addWidget(graph);
    mItems.append(graph);
    updateLayout();
}

void GraphViewWrapper::resizeEvent(QResizeEvent*)
{
    updateLayout();
}

void GraphViewWrapper::updateLayout()
{
    int count = mItems.size();
    if(count > 0)
    {
        int H = height();
        int h = fmax(floor(H/mItems.size()), mMinHeight);
        int w = width();

        mViewport->setFixedSize(w, count*h);

        /*for(int i=0; i<count; ++i)
        {
            mItems[i]->setGeometry(0, i*h, w, h);
        }*/
    }
}
