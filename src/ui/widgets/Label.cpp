    #include "Label.h"
#include "Painting.h"
#include <QtWidgets>


Label::Label(QWidget* parent):QLabel(parent),
mIsTitle(false)
{
    init();
}

Label::Label(const QString& text, QWidget* parent):QLabel(text, parent)
{
    init();
}

Label::~Label()
{

}

void Label::init()
{
    //QFont f = font();
    //f.setPointSize(pointSize(11));
    //setFont(f);
    setAlignment(Qt::AlignVCenter | Qt::AlignRight);
}

void Label::setIsTitle(bool isTitle)
{
    mIsTitle = isTitle;
    
    if(mIsTitle) {
        setAutoFillBackground(true);
        
        QPalette palette = QLabel::palette();
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Window, Painting::mainColorDark);
        setPalette(palette);
        
        QFont f = font();
        f.setPointSizeF(QApplication::font().pointSizeF()*1.3);
        //f.setPointSize(pointSize(13));
        setFont(f);
        
        setAlignment(Qt::AlignCenter);
        setFixedHeight(20);
    }
    
    update();
}

void Label::setLight()
{
    QPalette palette = QLabel::palette();
    palette.setColor(QPalette::WindowText, QColor(200, 200, 200));
    setPalette(palette);
}

void Label::setDark()
{
    QPalette palette = QLabel::palette();
    palette.setColor(QPalette::WindowText, Qt::black);
    setPalette(palette);
}
