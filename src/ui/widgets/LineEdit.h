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

#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QLineEdit>

class QFont;

class LineEdit: public QLineEdit
{
    Q_OBJECT

public slots:
    virtual void setVisible(bool visible);
    virtual void show();
    virtual void hide();

public:
    explicit LineEdit(QWidget* parent = nullptr);
    void resetText(double value);
    void resetText(const QString& text);

    void setFont(const QFont &font);
    virtual ~LineEdit();
    void setAdjustText(bool ad = true);

private:
    void adjustFont();

    bool mAdjustText;
    QFont mBaseFont;

protected:
    void resizeEvent(QResizeEvent* e);
};

#endif
