/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#ifndef BUTTON_H
#define BUTTON_H

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

    void setFixedSize(const QSize &size) {QPushButton::setFixedSize(size); }
    void setFixedSize(int w, int h) {QPushButton::setFixedSize(w, h); }
    void setFixedWidth(int w) {QPushButton::setFixedWidth(w); }
    void setFixedHeight(int h) {QPushButton::setFixedHeight(h); }

    QSize sizeHint() const {return QPushButton::sizeHint(); }
    QSize minimumSizeHint() const {return QPushButton::minimumSizeHint(); }

    inline void resize(int w, int h)
    { QPushButton::resize(QSize(w, h)); }

    inline void setGeometry(int ax, int ay, int aw, int ah)
    { QPushButton::setGeometry(QRect(ax, ay, aw, ah)); }

    inline QRect rect() const
    { return QPushButton::rect(); }

    inline const QRect geometry() const
    { return QPushButton::geometry(); }

    inline QSize size() const
    { return QPushButton::size(); }

    inline int width() const
    { return QPushButton::width(); }

    inline int height() const
    { return QPushButton::height(); }


    void setColorState(ColorState state);
    virtual void setCheckable(const bool checkable);

protected:
    void paintEvent(QPaintEvent* e);

    virtual void enterEvent(QEnterEvent * e);
    virtual void leaveEvent(QEvent *e);
    virtual void keyPressEvent(QKeyEvent* event);

    bool mFlatVertical;
    bool mFlatHorizontal;
    bool mIsClose;

    bool mIconOnly;
    bool mMouseOver;

    ColorState mColorState;

public:
    bool mUseMargin;

signals:
      void click();
};

#endif
