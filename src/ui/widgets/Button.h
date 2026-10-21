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
    void setIconOnly(bool iconOnly);

    void setColorState(ColorState state);
    void setCheckState(const bool checkState);
    void setCheckable(const bool checkable);

protected:
    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;
    bool event(QEvent* event) override; // gestion des toolTips (conditionnés par AppSettings::mShowHelp)

    void updateFont(); // ajuste mFont à la largeur du bouton (mode texte)

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
