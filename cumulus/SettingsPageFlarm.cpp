
/***********************************************************************
**
**   SettingsPageFlarm.cpp
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c): 2018 Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
***********************************************************************/

#include <algorithm>

#ifndef QT_5
#include <QtGui>
#else
#include <QtWidgets>
#endif

#ifdef QTSCROLLER
#include <QtScroller>
#endif

#include "SettingsPageFlarm.h"
#include "flarmbase.h"
#include "generalconfig.h"
#include "gpsnmea.h"
#include "layout.h"
#include "rowdelegate.h"

// Timeout in ms for waiting for a FLARM response
#define RESP_TO 5000

/**
 * Constructor
 */
SettingsPageFlarm::SettingsPageFlarm( QWidget *parent ) :
  QWidget( parent ),
  m_table(0)
{
  setObjectName("SettingsPageFlarm");
  setWindowFlags( Qt::Tool );
  setWindowModality( Qt::WindowModal );
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle( tr("Settings - FLARM") );

  if( parent )
    {
      resize( parent->size() );
    }

  QHBoxLayout *topLayout = new QHBoxLayout( this );
  topLayout->setSpacing(5);

  m_table = new QTableWidget( 0, 4, this );
  // list->setSelectionBehavior( QAbstractItemView::SelectRows );
  m_table->setSelectionMode( QAbstractItemView::SingleSelection );
  m_table->setAlternatingRowColors( true );
  m_table->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
  m_table->setHorizontalScrollMode( QAbstractItemView::ScrollPerPixel );

#ifdef ANDROID
  QScrollBar* lvsb = m_table->verticalScrollBar();
  lvsb->setStyleSheet( Layout::getCbSbStyle() );
#endif

#ifdef QSCROLLER
  QScroller::grabGesture( m_table->viewport(), QScroller::LeftMouseButtonGesture );
#endif

#ifdef QTSCROLLER
  QtScroller::grabGesture( m_table->viewport(), QtScroller::LeftMouseButtonGesture );
#endif

  QString style = "QTableView QTableCornerButton::section { background: gray }";
  m_table->setStyleSheet( style );
  QHeaderView *vHeader = m_table->verticalHeader();
  style = "QHeaderView::section { width: 2em }";
  vHeader->setStyleSheet( style );

  // set new row height from configuration
  int afMargin = GeneralConfig::instance()->getListDisplayAFMargin();
  m_rowDelegate = new RowDelegate( m_table, afMargin );
  m_table->setItemDelegate( m_rowDelegate );

  // hide vertical headers
  // QHeaderView *vHeader = list->verticalHeader();
  // vHeader->setVisible(false);

  QTableWidgetItem *item;

  item = new QTableWidgetItem( tr("CMD") );
  m_table->setHorizontalHeaderItem( 0, item );

  item = new QTableWidgetItem( tr("CMD") );
  m_table->setHorizontalHeaderItem( 1, item );

  item = new QTableWidgetItem( tr(" Item ") );
  m_table->setHorizontalHeaderItem( 2, item );

  item = new QTableWidgetItem( tr(" Value ") );
  m_table->setHorizontalHeaderItem( 3, item );

  QHeaderView* hHeader = m_table->horizontalHeader();
  hHeader->setStretchLastSection( true );
#if QT_VERSION >= 0x050000
  hHeader->setSectionsClickable( true );
#else
  hHeader->setClickable( true );
#endif

  connect( hHeader, SIGNAL(sectionClicked(int)),
           this, SLOT(slot_HeaderClicked(int)) );

  connect( m_table, SIGNAL(cellClicked( int, int )),
           this, SLOT(slot_CellClicked( int, int )) );

  topLayout->addWidget( m_table, 2 );

  QGroupBox* buttonBox = new QGroupBox( this );

  int buttonSize = Layout::getButtonSize();
  int iconSize   = buttonSize - 5;

  m_loadButton  = new QPushButton;
  m_loadButton->setIcon( QIcon( GeneralConfig::instance()->loadPixmap( "resort.png" ) ) );
  m_loadButton->setIconSize(QSize(iconSize, iconSize));
  m_loadButton->setMinimumSize(buttonSize, buttonSize);
  m_loadButton->setMaximumSize(buttonSize, buttonSize);
  m_loadButton->setToolTip( tr("Get all data items from FLARM.") );

#if defined(QSCROLLER) || defined(QTSCROLLER)

  m_enableScroller = new QCheckBox("][");
  m_enableScroller->setCheckState( Qt::Checked );
  m_enableScroller->setMinimumHeight( Layout::getButtonSize(12) );

  connect( m_enableScroller, SIGNAL(stateChanged(int)),
	   this, SLOT(slot_scrollerBoxToggled(int)) );

#endif

  m_closeButton = new QPushButton;
  m_closeButton->setIcon(QIcon(GeneralConfig::instance()->loadPixmap("cancel.png")));
  m_closeButton->setIconSize(QSize(iconSize, iconSize));
  m_closeButton->setMinimumSize(buttonSize, buttonSize);
  m_closeButton->setMaximumSize(buttonSize, buttonSize);

  connect( m_loadButton, SIGNAL(clicked() ), this, SLOT(slot_getAllFlarmData()) );
  connect( m_closeButton, SIGNAL(clicked() ), this, SLOT(slot_Close()) );

  // vertical box with operator buttons
  QVBoxLayout *vbox = new QVBoxLayout;

  vbox->setSpacing(0);
  vbox->addWidget( m_loadButton );
  vbox->addStretch(2);

#if defined(QSCROLLER) || defined(QTSCROLLER)

  vbox->addWidget( m_enableScroller, 0, Qt::AlignCenter );
  vbox->addStretch(2);

#endif

  vbox->addSpacing(32);
  vbox->addWidget( m_closeButton );
  buttonBox->setLayout( vbox );
  topLayout->addWidget( buttonBox );

  // Add Flarm signal to our slot to get Flarm configuration data.
  connect( Flarm::instance(), SIGNAL(flarmPflacSentence( QStringList&)),
           this, SLOT(slot_PflacSentence( QStringList&)) );

  // Timer for command time supervision
  m_timer = new QTimer( this );
  m_timer->setSingleShot( true );
  m_timer->setInterval( RESP_TO );

  connect( m_timer, SIGNAL(timeout()), SLOT(slot_Timeout()));
  loadTableItems();
}

SettingsPageFlarm::~SettingsPageFlarm()
{
}

void SettingsPageFlarm::showEvent( QShowEvent *event )
{
  Q_UNUSED( event )
  m_table->setFocus();
}

void SettingsPageFlarm::enableButtons( const bool toggle )
{
  m_loadButton->setEnabled( toggle );
  m_closeButton->setEnabled( toggle );

  // Block all signals from the table.
  m_table->blockSignals( ! toggle );
}

void SettingsPageFlarm::loadTableItems()
{
  // Clear all old data in the list
  m_items.clear();

  m_items << "DEVTYPE;RO;ALL"
          << "SWVER;RO;ALL"
          << "SWEXP;RO;ALL"
          << "FLARMVER;RO;ALL"
          << "BUILD;RO;ALL"
          << "SER;RO;ALL"
          << "REGION;RO;ALL"
          << "RADIOID;RO;ALL"
          << "CAP;RO;ALL"
          << "OBSTDB;RO;ALL"
          << "OBSTEXP;RO;ALL"
          << "IGCSER;RO;ALL"
          << "ID;RW;ALL"
          << "NMEAOUT;RW;ALL"
          << "NMEAOUT1;RW;PF"
          << "NMEAOUT2;RW;PF"
          << "BAUD;RW;ALL"
          << "BAUD1;RW;PF"
          << "BAUD2;RW;PF"
          << "ACFT;RW;ALL"
          << "RANGE;RW;ALL"
          << "VRANGE;RW;PF"
          << "PRIV;RW;ALL"
          << "NOTRACK;RW;ALL"
          << "THRE;RW;ALL"
          << "LOGINT;RW;ALL"
          << "PILOT;RW;ALL"
          << "COPIL;RW;ALL"
          << "GLIDERID;RW;ALL"
          << "GLIDERTYPE;RW;ALL"
          << "COMPID;RW;ALL"
          << "COMPCLASS;RW;ALL"
          << "CFLAGS;RW;ALL"
          << "UI;RW;ALL"
          << "AUDIOOUT;RW;PF"
          << "AUDIOVOLUME;RW;PF"
          << "CLEARMEM;WO;CF"
          << "CLEARLOGS;WO;PF"
          << "CLEAROBST;WO;PF"
          << "DEF;WO;ALL";

  m_table->clearContents();

  for( int i = 0; i < m_items.size(); i++ )
    {
      addRow2List( m_items.at(i) );
    }

  m_table->setCurrentCell( 0, 2 );
  m_table->resizeRowsToContents();
  m_table->resizeColumnsToContents();
  // m_table->resizeColumnToContents(0);
  // m_table->resizeColumnToContents(2);
  // m_table->resizeColumnToContents(3);
}

void SettingsPageFlarm::addRow2List( const QString& rowData )
{
  if( rowData.isEmpty() )
    {
      return;
    }

  QList<QString> items = rowData.split( ";", QString::KeepEmptyParts );

  if( items.size() != 3 )
    {
      return;
    }

  m_table->setRowCount( m_table->rowCount() + 1 );

  int row = m_table->rowCount() - 1;

  QTableWidgetItem* item;

  // Column 0 is used as GET button
  item = new QTableWidgetItem( tr("Get") );
  item->setTextAlignment( Qt::AlignCenter );

  if( items[1] == "RW" || items[1] == "RO" )
    {
      item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    }
  else if( items[1] == "WO" )
    {
      item->setFlags( Qt::ItemIsSelectable );
    }

  m_table->setItem( row, 0, item );

  // Column 1 is used as SET button
  item = new QTableWidgetItem( tr("Set") );
  item->setTextAlignment( Qt::AlignCenter );

  if( items[1] == "RW" || items[1] == "WO" )
    {
      item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    }
  else if( items[1] == "RO" )
    {
      item->setFlags( Qt::ItemIsSelectable );
    }

  m_table->setItem( row, 1, item );

  // "DEVTYPE;RO;ALL"
  // column 2 is set to Flarm's configuration item
  item = new QTableWidgetItem( items[0] );
  item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );

  // Data is set to Flarm's device type
  item->setData( Qt::UserRole, items[2] );
  m_table->setItem( row, 2, item );
  m_table->setCurrentItem( item );

  // column 3 is set to Flarm's configuration item value
  item = new QTableWidgetItem();
  item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );

  // Data is set to item's accessibility
  item->setData( Qt::UserRole, items[1] );
  m_table->setItem( row, 3, item );
}

void SettingsPageFlarm::slot_Close()
{
  setVisible( false );
  emit closed();
  QWidget::close();
}

void SettingsPageFlarm::slot_HeaderClicked( int section )
{
  if( section != 2 )
    {
      // Only the item column can be sorted. All others make no sense.
      return;
    }

  static Qt::SortOrder so = Qt::AscendingOrder;

  m_table->sortByColumn( section, so );

  // Change sort order for the next click.
  so = ( so == Qt::AscendingOrder ) ? Qt::DescendingOrder : Qt::AscendingOrder;
 }

void SettingsPageFlarm::slot_CellClicked( int row, int column )
{
  QTableWidgetItem* item = m_table->item( row, column );

  if( item == static_cast<QTableWidgetItem *>(0) || row < 0 || column < 0 )
    {
      // Item can be a Null pointer, if a row has been removed.
      return;
    }

  QString itemDevType = m_table->item( row, 2 )->data(Qt::UserRole).toString();
  QString itemAccess  = m_table->item( row, 3 )->data(Qt::UserRole).toString();

  QString title, label;

  if( column == 3 )
    {
      // Look, if Flarm item is read/write. In this case the content of the
      // cell can be changed.
      if( itemAccess == "RO" )
        {
          // A read only item cannot be edited.
          return;
        }

      title = tr("Enter item value");
      label = tr("Flarm item value:");

      bool ok;

#ifndef MAEMO5
    QString text = QInputDialog::getText( this,
                                          title,
                                          label,
                                          QLineEdit::Normal,
                                          item->text(),
                                          &ok,
                                          0,
                                          Qt::ImhNoPredictiveText );
#else
    QString text = QInputDialog::getText( this,
                                          title,
                                          label,
                                          QLineEdit::Normal,
                                          item->text(),
                                          &ok,
                                          0 );
#endif

    if( ok )
      {
        item->setText( text );
      }

      return;
    }

  if( column == 0 )
    {
      if( itemAccess == "WO" )
        {
          // A write only item cannot be requested.
          return;
        }

      // Get was clicked
      m_table->item( row, 3 )->setText("");
      QString itemText = m_table->item( row, 2 )->text();
      QString cmd = "$PFLAC,R," + itemText;

      requestFlarmData( cmd, true );
      return;
    }

  if( column == 1 )
    {
      if( itemAccess == "RO" )
        {
          // A read only item cannot be changed.
          return;
        }

      // get Flarm device type
      QString device = FlarmBase::getDeviceType();

      if( itemDevType != "ALL" &&
          ((device.startsWith( "PowerFLARM-") == true && itemDevType != "PF") ||
          (device.startsWith( "PowerFLARM-") == false && itemDevType != "CF")) )
        {
          QString text0 = tr("Configuration item is unsupported by your FLARM!");
          QString text1 = tr("Information");
          messageBox( QMessageBox::Information, text0, text1 );
          return;
        }

      QString itemText  = m_table->item( row, 2 )->text();
      QString itemValue = m_table->item( row, 3 )->text();

      if( itemValue.isEmpty() )
        {
          QString text0 = tr("Configuration item has no value assigned!");
          QString text1 = tr("Warning");


          int button = messageBox( QMessageBox::Warning,
                                   text0,
                                   text1,
                                   QMessageBox::Abort|QMessageBox::Ignore );

          if( button == QMessageBox::Abort )
            {
              return;
            }
        }

      QString cmd = "$PFLAC,S," + itemText + "," + itemValue;

      requestFlarmData( cmd, true );
      return;
    }
}

void SettingsPageFlarm::slot_getAllFlarmData()
{
  if( checkFlarmConnection() == false )
    {
      return;
    }

  // get Flarm device type
  QString device = FlarmBase::getDeviceType();

  for( int i = 0; i < m_table->rowCount(); i++ )
    {
      m_table->item( i, 3 )->setText("");

      if( m_table->item( i, 3 )->data(Qt::UserRole).toString() == "WO" )
        {
          // A write only item cannot be requested.
          continue;
        }

      // devType can be: ALL, PF, CF
      QString devType = m_table->item( i, 2 )->data(Qt::UserRole).toString();

      if( devType != "ALL" )
        {
          // We compare the device type with the device configuration item,
          // to handle incompabilities between the different devices.
          if( device.startsWith( "PowerFLARM-") == true && devType != "PF" )
            {
              // Power Flarm Device but Classic Flam config item
              continue;
            }

          if( device.startsWith( "PowerFLARM-") == false && devType != "CF" )
            {
              // Classic Flarm Device but Power Flam config item
              continue;
            }
        }

      QString itemText = m_table->item( i, 2 )->text();
      QString cmd = "$PFLAC,R," + itemText;

      bool overwriteCursor = ( i == 0 ) ? true : false;

      requestFlarmData( cmd, overwriteCursor );
    }
}

void SettingsPageFlarm::requestFlarmData( QString &command, bool overwriteCursor )
{
  if( checkFlarmConnection() == false )
    {
      return;
    }

  if( overwriteCursor == true )
    {
      QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    }

  // Disable button pressing.
  enableButtons( false );
  m_commands << command;
  nextFlarmCommand();
}

void SettingsPageFlarm::nextFlarmCommand()
{
   if( m_commands.size() == 0 )
    {
      // nothing more to send
      enableButtons( true );
      m_timer->stop();
      QApplication::restoreOverrideCursor();
      // m_table->resizeColumnToContents(3);
      return;
    }

   if( m_timer->isActive() == true )
     {
       // There is already running another command.
       return;
     }

   QString cmd = m_commands.head();

   QByteArray ba = FlarmBase::replaceUmlauts( cmd.toLatin1() );

   bool res = GpsNmea::gps->sendSentence( ba );

   m_timer->start();

  if( res == false )
    {
      QString text0 = tr("Flarm device not reachable!");
      QString text1 = tr("Error");
      messageBox( QMessageBox::Warning, text0, text1 );
      m_commands.clear();
      nextFlarmCommand();
      return;
    }
}

void SettingsPageFlarm::slot_PflacSentence( QStringList& sentence )
{
  // Can be called also due to request from another side.
  if( m_commands.size() == 0 )
    {
      // No command has been requested by this page. We ignore these sentence.
      return;
    }

  // qDebug() << "slot_PflacSentence: executed Command:" << m_commands.head();
  // qDebug() << "Answer:" << sentence;

  // $PFLAC", "A", "DEVTYPE", "PowerFLARM-Core", "67"
  //"$PFLAC", "A", "ERROR", "41"

  if( sentence.size() >= 4 && sentence[1] == "A" )
    {
      if( sentence[2] == "ERROR" )
        {
          m_timer->stop();

          qWarning() << "Command" << m_commands.head() << "returned with ERROR!";

          QString text0 = tr("Command:")
                          + "\n\n"
                          + m_commands.head()
                          + "\n\n"
                          + tr("rejected by Flarm with error.");

          QString text1 = tr("Error");
          messageBox( QMessageBox::Warning, text0, text1 );
        }
      else
        {
          for( int i = 0; i < m_table->rowCount(); i++ )
            {
              QTableWidgetItem* it = m_table->item(i, 0);

              if( it->text() == sentence[2] )
                {
                  m_table->item( i, 1 )->setText( sentence[3] );
                  break;
                }
            }
        }
    }

  QString lastCmd = m_commands.dequeue();

  m_timer->stop();
  nextFlarmCommand();
}

void SettingsPageFlarm::slot_Timeout()
{
  QString text0 = tr("Flarm device not reachable!");
  QString text1 = tr("Error");
  messageBox( QMessageBox::Warning, text0, text1 );

  m_commands.clear();
  nextFlarmCommand();
}

bool SettingsPageFlarm::checkFlarmConnection()
{
  const Flarm::FlarmStatus& status = Flarm::instance()->getFlarmStatus();

  QString text0 = tr("Flarm device not reachable!");
  QString text1 = tr("Error");

  if( status.valid == false || GpsNmea::gps->getConnected() != true )
    {
      // Flarm data were not received or GPS connection is off-line.
      messageBox( QMessageBox::Warning, text0, text1 );
      return false;
    }

  return true;
}

/** Shows a popup message box to the user. */
int SettingsPageFlarm::messageBox( QMessageBox::Icon icon,
                                   QString message,
                                   QString title,
                                   QMessageBox::StandardButtons buttons )
{
  QMessageBox mb( icon,
                  title,
                  message,
                  buttons,
                  this );

#ifdef ANDROID

  mb.show();
  QPoint pos = mapToGlobal(QPoint( width()/2  - mb.width()/2,
                                   height()/2 - mb.height()/2 ));
  mb.move( pos );

#endif

  int ret = mb.exec();

  return ret;
}