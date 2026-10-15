/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
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
    Button(const Button &button, QWidget* parent = nullptr);
    ~Button();
    //void init();

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
    void setCheckState(const bool checkState);
    virtual void setCheckable(const bool checkable);
    virtual void setText(const QString &text);
    virtual void resizeEvent(QResizeEvent* e);

protected:
    void paintEvent(QPaintEvent* e);

    virtual void enterEvent(QEnterEvent * e);
    virtual void leaveEvent(QEvent *e);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual bool event(QEvent* event); //Pour essaie des toolTips qui ne fonctionnent plus

    bool mFlatVertical;
    bool mFlatHorizontal;
    bool mIsClose;

    bool mIconOnly;
    bool mMouseOver;

    ColorState mColorState;
    QFont mFont;

public:
    bool mUseMargin;

};

#endif
