#ifndef Button_H
#define Button_H

#include <QPushButton>


class Button: public QPushButton
{
    Q_OBJECT
public:
    enum    ColorState
    {
        eDefault = 0,
        eReady = 1,
        eWarning = 2
    };
    
    Button(QWidget* parent = nullptr);
    Button(const QString& text, QWidget* parent = nullptr);
    ~Button();
    void init();
    
    void setFlatVertical();
    void setFlatHorizontal();
    void setIsClose(bool isClose);
    void setIconOnly(bool iconOnly) { mIconOnly = iconOnly; }



    void setColorState(ColorState state);
    
protected:
    void paintEvent(QPaintEvent* e);

    virtual void enterEvent(QEvent * e);
    virtual void leaveEvent(QEvent *e);

    bool mFlatVertical;
    bool mFlatHorizontal;
    bool mIsClose;

    bool mIconOnly;
    bool mMouseOver;
    
    ColorState mColorState;
    
public:
    bool mUseMargin;
};

#endif
