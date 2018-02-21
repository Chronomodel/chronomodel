#ifndef TABS_H
#define TABS_H

#include <QWidget>


class Tabs: public QWidget
{
    Q_OBJECT
public:
    Tabs(QWidget* parent = nullptr);
    ~Tabs();
    
    // setter
    void addTab(const QString& name);
    void addTab(QWidget* wid, const QString& name);
    int currentIndex() const;
    void setTab(const int &index, bool notify);
    void setFont(const QFont &font);
    void setTabHeight(const int &h) {mTabHeight = h;}

    void setTabVisible(const int &i , const bool visible) {
        mTabVisible[i] = visible;
        updateLayout();
    }

    // getter
    QWidget* getWidget(const int &i);
    QWidget* getCurrentWidget();
    QRect widgetRect() const;
    QRect minimalGeometry() const;
    int minimalHeight() const;
    int minimalWidth() const;
    int tabHeight() const { return mTabHeight;}
    
signals:
    void tabClicked(const int &index);
    
public slots:
    void showWidget(const int &i);

protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void updateLayout();

private:
    int mTabHeight;
    QStringList mTabNames;
    QList<QRectF> mTabRects;
    QList<QWidget*> mTabWidgets;
    QList<bool> mTabVisible;
    int mCurrentIndex;
    QFont mFont;
};

#endif
