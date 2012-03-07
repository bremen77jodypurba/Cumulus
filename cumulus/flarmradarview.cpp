/***********************************************************************
**
**   flarmview.cpp
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c): 2010-2011 Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#include <QtGui>

#include "flarm.h"
#include "flarmaliaslist.h"
#include "flarmdisplay.h"
#include "flarmradarview.h"
#include "generalconfig.h"
#include "layout.h"

/**
 * Constructor
 */
FlarmRadarView::FlarmRadarView( QWidget *parent ) :
  QWidget( parent ),
  display(0)
{
  setAttribute( Qt::WA_DeleteOnClose );
  setContentsMargins(-4,-8,-4,-8);

  QHBoxLayout *topLayout = new QHBoxLayout( this );
  topLayout->setSpacing(5);

  display = new FlarmDisplay( this );
  topLayout->addWidget( display, 2 );

  connect( Flarm::instance(), SIGNAL(newFlarmPflaaData()),
           display, SLOT(slot_UpdateDisplay()) );

  connect( Flarm::instance(), SIGNAL(flarmPflaaDataTimeout()),
           display, SLOT(slot_ResetDisplay()) );

  QGroupBox* buttonBox = new QGroupBox( this );
  buttonBox->setContentsMargins(2,2,2,2);

  int size = 40;

#if defined MAEMO || defined ANDROID
  size = 60;
#endif

  QPushButton *zoomButton  = new QPushButton;
  zoomButton->setIcon(QIcon(GeneralConfig::instance()->loadPixmap("zoom32.png")));
  zoomButton->setIconSize(QSize(IconSize, IconSize));
  zoomButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::QSizePolicy::Preferred);
  zoomButton->setMinimumSize(size, size);
  zoomButton->setMaximumSize(size, size);

  QPushButton *listButton  = new QPushButton;
  listButton->setIcon(QIcon(GeneralConfig::instance()->loadPixmap("list32.png")));
  listButton->setIconSize(QSize(IconSize, IconSize));
  listButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::QSizePolicy::Preferred);
  listButton->setMinimumSize(size, size);
  listButton->setMaximumSize(size, size);

  display->setUpdateInterval( 2 );
  updateButton = new QPushButton( "2s" );
  updateButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::QSizePolicy::Preferred);
  updateButton->setMinimumSize(size, size);
  updateButton->setMaximumSize(size, size);

  QPushButton *aliasButton  = new QPushButton;
  aliasButton->setIcon(QIcon(GeneralConfig::instance()->loadPixmap("monkey32.png")));
  aliasButton->setIconSize(QSize(IconSize, IconSize));
  aliasButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::QSizePolicy::Preferred);
  aliasButton->setMinimumSize(size, size);
  aliasButton->setMaximumSize(size, size);

  QPushButton *closeButton = new QPushButton;
  closeButton->setIcon(QIcon(GeneralConfig::instance()->loadPixmap("cancel.png")));
  closeButton->setIconSize(QSize(IconSize, IconSize));
  closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::QSizePolicy::Preferred);
  closeButton->setMinimumSize(size, size);
  closeButton->setMaximumSize(size, size);

  addButton = new QPushButton;
  addButton->setIcon(QIcon(GeneralConfig::instance()->loadPixmap("add.png")));
  addButton->setIconSize(QSize(IconSize, IconSize));
  addButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::QSizePolicy::Preferred);
  addButton->setMinimumSize(size, size);
  addButton->setMaximumSize(size, size);

  if( FlarmDisplay::getSelectedObject().isEmpty() )
    {
      // The add button is invisible, if no selection is made.
      addButton->setVisible( false );
    }

  connect( zoomButton, SIGNAL(clicked() ), this, SLOT(slotZoom()) );
  connect( listButton, SIGNAL(clicked() ), this, SLOT(slotOpenListView()) );
  connect( updateButton, SIGNAL(clicked() ), this, SLOT(slotUpdateInterval()) );
  connect( aliasButton, SIGNAL(clicked() ), this, SLOT(slotOpenAliasList()) );
  connect( closeButton, SIGNAL(clicked() ), this, SLOT(slotClose()) );
  connect( addButton, SIGNAL(clicked() ), this, SLOT(slotAddFlarmId()) );

  // vertical box with operator buttons
  QVBoxLayout *vbox = new QVBoxLayout;

  vbox->setSpacing(0);
  vbox->addWidget( zoomButton );
  vbox->addSpacing(10);
  vbox->addWidget( listButton );
  vbox->addSpacing(10);
  vbox->addWidget( updateButton );
  vbox->addSpacing(10);
  vbox->addWidget( aliasButton );
  vbox->addSpacing(10);
  vbox->addWidget( addButton );
  vbox->addStretch(2);
  vbox->addWidget( closeButton );
  buttonBox->setLayout( vbox );

  topLayout->addWidget( buttonBox );
}

/**
 * Destructor
 */
FlarmRadarView::~FlarmRadarView()
{
}

/** Called if zoom level shall be changed. */
void FlarmRadarView::slotZoom()
{
  enum FlarmDisplay::Zoom zoom = display->getZoomLevel();

  if( zoom == FlarmDisplay::Low )
    {
      display->slot_SwitchZoom( FlarmDisplay::Middle );
    }
  else if( zoom == FlarmDisplay::Middle )
    {
      display->slot_SwitchZoom( FlarmDisplay::High );
    }
  else
    {
      display->slot_SwitchZoom( FlarmDisplay::Low );
    }
}

/** Called if list view button was pressed. */
void FlarmRadarView::slotOpenListView()
{
  emit openListView();
}

/** Called if close button was pressed. */
void FlarmRadarView::slotClose()
{
  // Ask FlarmWidget to close the widget.
  emit closeRadarView();
}

/** Called if update interval button was pressed. */
void FlarmRadarView::slotUpdateInterval()
{
  QString text = updateButton->text();

  QString newText = "2s";
  int newValue = 2;

  if( text == "1s" )
    {
      newText = "2s";
      newValue = 2;
    }
  else if( text == "2s" )
    {
      newText = "3s";
      newValue = 3;
    }
  else if( text == "3s" )
    {
      newText = "1s";
      newValue = 1;
    }

  updateButton->setText( newText );
  display->setUpdateInterval( newValue );
}

/** Called if alias list button was pressed. */
void FlarmRadarView::slotOpenAliasList()
{
  emit openAliasList();
}

/** Called to change the visibility of the add Flarm Id button. */
void FlarmRadarView::slotShowAddButton( QString selectedObject )
{
  if( selectedObject.isEmpty() == true )
    {
      // The add button is invisible, if no selection is made.
      addButton->setVisible( false );
    }
  else
    {
      addButton->setVisible( true );
    }
}

/** Called to add an object to the Flarm alias list. */
void FlarmRadarView::slotAddFlarmId()
{
  QString& selectedObject = FlarmDisplay::getSelectedObject();

  if( selectedObject.isEmpty() )
    {
      // There is nothing selected, ignore call.
      return;
    }

  QHash<QString, QString>& aliasHash = FlarmAliasList::getAliasHash();

  if( aliasHash.isEmpty() )
    {
      // try to load it
      FlarmAliasList::loadAliasData();
    }

  aliasHash = FlarmAliasList::getAliasHash();

  // Look for an existing alias name
  QString alias = aliasHash.value( selectedObject, "" );

  // Add the selected Flarm Id to the alias list.
  bool ok;
  alias = QInputDialog::getText( this,
                                 tr("Add alias name"),
                                 tr("Alias name (15) for ") + selectedObject + ":",
                                 QLineEdit::Normal,
                                 alias,
                                 &ok );

  if( !ok || alias.isEmpty() )
    {
      return;
    }

  // Add an alias name to the alias list. An existing alias name will be updated.
  aliasHash.insert( selectedObject, alias.trimmed().left(FlarmAliasList::MaxAliasLength) );
  FlarmAliasList::saveAliasData();
  display->createBackground();
  display->update();
}
