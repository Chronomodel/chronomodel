#include "ZoomControls.h"
#include <QtWidgets>


ZoomControls::ZoomControls(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    mZoomIn = new QPushButton();
    mZoomIn->setIcon(QIcon(":zoom_in.png"));
    
    mZoomOut = new QPushButton();
    mZoomOut->setIcon(QIcon(":zoom_out.png"));
    
    mZoomDefault = new QPushButton();
    mZoomDefault->setIcon(QIcon(":zoom_default.png"));
    
    QHBoxLayout* zoomLayout = new QHBoxLayout();
    zoomLayout->setContentsMargins(0, 0, 0, 0);
    zoomLayout->addWidget(mZoomIn);
    zoomLayout->addWidget(mZoomDefault);
    zoomLayout->addWidget(mZoomOut);
    
    setLayout(zoomLayout);
    
    connect(mZoomIn, SIGNAL(clicked()), this, SIGNAL(zoomIn()));
    connect(mZoomOut, SIGNAL(clicked()), this, SIGNAL(zoomOut()));
    connect(mZoomDefault, SIGNAL(clicked()), this, SIGNAL(zoomDefault()));
}

ZoomControls::~ZoomControls()
{

}
