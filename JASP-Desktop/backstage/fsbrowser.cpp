//
// Copyright (C) 2015 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public
// License along with this program.  If not, see
// <http://www.gnu.org/licenses/>.
//

#include "fsbrowser.h"

#include <QGridLayout>
#include <QScrollArea>
#include <QMessageBox>

#include "verticalscrollarea.h"
#include "fsentrywidget.h"

FSBrowser::FSBrowser(QWidget *parent) : QWidget(parent)
{
	_browseMode = FSBrowser::BrowseOpenFile;
	_viewType = FSBrowser::IconView;

	QGridLayout *layout = new QGridLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	setLayout(layout);

	VerticalScrollArea *scrollArea = new VerticalScrollArea(this);
	scrollArea->setFrameShape(QScrollArea::NoFrame);
	layout->addWidget(scrollArea);

	_scrollPane = new QWidget;
	scrollArea->setWidget(_scrollPane);

	_scrollPaneLayout = new QVBoxLayout(_scrollPane);
	_scrollPaneLayout->setSpacing(1);
	_scrollPaneLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
	_scrollPane->setLayout(_scrollPaneLayout);

	_buttonGroup = new QButtonGroup(this);

	_authWidget = new AuthWidget(this);
	_authWidget->hide();

	connect(_authWidget, SIGNAL(loginRequested(QString,QString)), this, SLOT(loginRequested(QString,QString)));
}

void FSBrowser::setFSModel(FSBModel *model)
{
	_model = model;
	_model->refresh();
	refresh();

	connect(_model, SIGNAL(entriesChanged()), this, SLOT(refresh()));
	connect(_model, SIGNAL(authenticationSuccess()), this, SLOT(refresh()));
	connect(_model, SIGNAL(authenticationFail(QString)), this, SLOT(authenticationFailed(QString)));
}

void FSBrowser::setBrowseMode(FSBrowser::BrowseMode mode)
{
	_browseMode = mode;
}

void FSBrowser::setViewType(FSBrowser::ViewType viewType)
{
	_viewType = viewType;
}

void FSBrowser::refresh()
{
	if (_model->requiresAuthentication() && _model->isAuthenticated() == false)
	{
		_authWidget->show();
	}
	else
	{
		_authWidget->hide();

		bool compact = false;

		if (_viewType == ListView)
		{
			compact = true;
			_scrollPaneLayout->setContentsMargins(8, 8, 8, 8);
			_scrollPaneLayout->setSpacing(0);
		}
		else
		{
			_scrollPaneLayout->setContentsMargins(12, 12, 12, 12);
			_scrollPaneLayout->setSpacing(8);
		}

		foreach (QAbstractButton *button, _buttonGroup->buttons())
			delete button;

		int id = 0;

		foreach (const FSEntry &entry, _model->entries())
		{
			FSEntryWidget *button = new FSEntryWidget(entry, _scrollPane);
			button->setCompact(compact);

			_buttonGroup->addButton(button, id++);
			_scrollPaneLayout->addWidget(button);

			connect(button, SIGNAL(selected()), this, SLOT(entrySelectedHandler()));
			connect(button, SIGNAL(opened()), this, SLOT(entryOpenedHandler()));
		}
	}
}

void FSBrowser::loginRequested(QString username, QString password)
{
	_model->authenticate(username, password);
}

void FSBrowser::entrySelectedHandler()
{
	FSEntryWidget *entry = qobject_cast<FSEntryWidget*>(this->sender());
	if (entry->entryType() != FSEntry::Folder)
		emit entrySelected(entry->path());
}

void FSBrowser::entryOpenedHandler()
{
	FSEntryWidget *entry = qobject_cast<FSEntryWidget*>(this->sender());

	if (_browseMode == BrowseOpenFolder)
	{
		emit entryOpened(entry->path());
	}
	else
	{
		if (entry->entryType() == FSEntry::Folder)
			_model->setPath(entry->path());
		else
			emit entryOpened(entry->path());
	}
}

void FSBrowser::authenticationFailed(QString message)
{
	QMessageBox::warning(this, "", message);
}

