#ifndef Button_H
#define Button_H

#include <QPushButton>


class Button: public QPushButton
{
    Q_OBJECT
public:
    enum ColorState
    {
        eDefault = 0,
        eReady = 1,
        eWarning = 2
    };
    
    Button(QWidget* parent = 0);
    Button(const QString& text, QWidget* parent = 0);
    ~Button();
    void init();
    
    void setFlatVertical();
    void setFlatHorizontal();
    void setIsClose(bool isClose);
    
    void setColorState(ColorState state);
    
protected:
    void paintEvent(QPaintEvent* e);
    
    bool mFlatVertical;
    bool mFlatHorizontal;
    bool mIsClose;
    
    ColorState mColorState;
    
public:
    bool mUseMargin;
};

#endif
