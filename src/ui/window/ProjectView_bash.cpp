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

#include "ProjectView_bash.h"

#include <QDir>
#include <QFileDialog>

ProjectView::ProjectView(std::shared_ptr<Project>&, QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    mTable = new QTableWidget(this);
    mTable->setColumnCount(1);
    mTable->setHorizontalHeaderLabels({"Project Files"});
    mTable->setTextElideMode(Qt::ElideNone);

    mAddButton = new QPushButton ("Add File", this);
    mRemoveButton = new QPushButton ("Remove File", this);

    connect(mAddButton, &QPushButton::pressed, this, &ProjectView::tableAdd);
    connect(mRemoveButton, &QPushButton::pressed, this, &ProjectView::tableRemove);

    mTable->setFixedWidth(width()/2);
    mTable->setGeometry(QRect(width()/4, 50, width()/2, height()));

    mAddButton->setGeometry(QRect(0, 0, 150, 0));
    mRemoveButton->setGeometry(QRect(10, mAddButton->pos().y() + mAddButton->height() + 10, 150, 0));

    mLog = new QTextEdit(this);
    setMinimumSize(500, 200);
}

ProjectView::~ProjectView()
{

}

void ProjectView::setScreenDefinition()
{

}

void ProjectView::resizeEvent(QResizeEvent*)
{
    mTable->setFixedWidth(width()/3);
    mTable->setColumnWidth(0, width()/3);
    mTable->setGeometry(QRect(width()/3, 20, width()/3, height() - 20));

    auto x1 = mTable->pos().x();
    x1 = x1 /2 - 75;
    mAddButton->setGeometry(QRect(x1, 50, 150, 50));

    mRemoveButton->setGeometry(QRect(x1, mAddButton->pos().y() + mAddButton->height() + 10, 150, 50));

    x1 = mTable->pos().x() + mTable->width();

    mLog->setGeometry(QRect(x1 + 10, mTable->pos().y(), width() - x1 - 20, mTable->height()));

}

void ProjectView::tableAdd()
{
    const QString currentPath = QDir::homePath();
    const QString path = QFileDialog::getOpenFileName(this,
                                                      tr("Open File"),
                                                      currentPath,
                                                      tr("Chronomodel Project (*.chr)"));

    if (!path.isEmpty()) {
        mTable->setRowCount(mTable->rowCount()+1);

        QTableWidgetItem *newItem = new QTableWidgetItem(path);
        mTable->setItem(mTable->rowCount()-1 , 0, newItem );
        mTable->item(mTable->rowCount()-1 , 0)->setToolTip(path);
    }
}

void ProjectView::tableRemove()
{
    for (const auto i : mTable->selectedItems())
        mTable->removeRow(i->row());
}

void ProjectView::applyFilesSettings(Model*)
{

}

