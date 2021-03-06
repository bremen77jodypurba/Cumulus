/***********************************************************************
**
**   taskeditor.cpp
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2002      by Heiner Lamprecht
**                   2008-2018 by Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
***********************************************************************/

#ifndef QT_5
#include <QtGui>
#else
#include <QtWidgets>
#endif

#ifdef QTSCROLLER
#include <QtScroller>
#endif

#include "airfield.h"
#include "AirfieldListWidget.h"
#include "distance.h"
#include "flighttask.h"
#include "generalconfig.h"
#include "layout.h"
#include "listwidgetparent.h"
#include "mainwindow.h"
#include "mapcontents.h"
#include "radiopoint.h"
#include "SinglePointListWidget.h"
#include "RadioPointListWidget.h"
#include "rowdelegate.h"
#include "taskeditor.h"
#include "taskpoint.h"
#include "taskpointeditor.h"
#include "waypointlistwidget.h"
#include "wpeditdialog.h"

extern MapContents *_globalMapContents;

TaskEditor::TaskEditor( QWidget* parent,
                        QStringList &taskNamesInUse,
                        FlightTask* task ) :
  QWidget( parent ),
  taskNamesInUse( taskNamesInUse ),
  lastSelectedItem(0),
  m_lastEditedTP(-1)
{
  setObjectName("TaskEditor");
  setWindowFlags( Qt::Tool );
  setWindowModality( Qt::WindowModal );
  setAttribute( Qt::WA_DeleteOnClose );

  if( MainWindow::mainWindow() )
    {
      // Resize the dialog to the same size as the main window has. That will
      // completely hide the parent window.
      resize( MainWindow::mainWindow()->size() );
    }

  if ( task )
    {
      task2Edit = task;
      editState = TaskEditor::edit;
      setWindowTitle( task2Edit->getTaskTypeString() );
      editedTaskName = task->getTaskName();
    }
  else
    {
      task2Edit = new FlightTask( 0, false, "" );
      editState = TaskEditor::create;
      setWindowTitle(tr("New Task"));
    }

  Qt::InputMethodHints imh;

  taskName = new QLineEdit( this );
  taskName->setBackgroundRole( QPalette::Light );
  imh = (taskName->inputMethodHints() | Qt::ImhNoPredictiveText);
  taskName->setInputMethodHints(imh);

  // The task name maximum length is 10 characters. We calculate
  // the length of a M string of 10 characters. That is the maximum
  // width of the QLineEdit widget.
  QFontMetrics fm( font() );
  int maxInputLength = fm.width("MMMMMMMMMM");
  taskName->setMinimumWidth( maxInputLength );
  taskName->setMaximumWidth( maxInputLength );

  connect( taskName, SIGNAL(returnPressed()),
           MainWindow::mainWindow(), SLOT(slotCloseSip()) );

  taskList = new QTreeWidget( this );
  taskList->setObjectName("taskList");

  taskList->setRootIsDecorated(false);
  taskList->setItemsExpandable(false);
  taskList->setUniformRowHeights(true);
  taskList->setAlternatingRowColors(true);
  taskList->setSelectionBehavior(QAbstractItemView::SelectRows);
  taskList->setSelectionMode(QAbstractItemView::SingleSelection);
  taskList->setColumnCount(4);
  taskList->hideColumn( 0 );

  const int iconSize = Layout::iconSize( font() );
  taskList->setIconSize( QSize(iconSize, iconSize) );

  // set new row height from configuration
  int afMargin = GeneralConfig::instance()->getListDisplayAFMargin();
  taskList->setItemDelegate( new RowDelegate( taskList, afMargin ) );

  QStringList sl;
  sl << tr("ID")
     << tr("Type")
     << tr("Waypoint")
     << tr("Length");

  taskList->setHeaderLabels(sl);
#if QT_VERSION >= 0x050000
  taskList->header()->setSectionResizeMode( QHeaderView::ResizeToContents );
#else
  taskList->header()->setResizeMode( QHeaderView::ResizeToContents );
#endif

  taskList->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
  taskList->setHorizontalScrollMode( QAbstractItemView::ScrollPerPixel );

#ifdef QSCROLLER
  QScroller::grabGesture( taskList->viewport(), QScroller::LeftMouseButtonGesture );
#endif

#ifdef QTSCROLLER
  QtScroller::grabGesture( taskList->viewport(), QtScroller::LeftMouseButtonGesture );
#endif

  upButton = new QPushButton( this );
  upButton->setIcon( QIcon(GeneralConfig::instance()->loadPixmap( "up.png", true )) );
  upButton->setIconSize(QSize(iconSize, iconSize));
#ifndef ANDROID
  upButton->setToolTip( tr("move selected waypoint up") );
#endif
  downButton = new QPushButton( this );
  downButton->setIcon( QIcon(GeneralConfig::instance()->loadPixmap( "down.png", true )) );
  downButton->setIconSize(QSize(iconSize, iconSize));
#ifndef ANDROID
  downButton->setToolTip( tr("move selected waypoint down") );
#endif
  invertButton = new QPushButton( this );
  invertButton->setIcon( QIcon(GeneralConfig::instance()->loadPixmap( "resort.png", true )) );
  invertButton->setIconSize(QSize(iconSize, iconSize));
#ifndef ANDROID
  invertButton->setToolTip( tr("reverse waypoint order") );
#endif
  addButton = new QPushButton( this );
  addButton->setIcon( QIcon(GeneralConfig::instance()->loadPixmap( "left.png", true )) );
  addButton->setIconSize(QSize(iconSize, iconSize));
#ifndef ANDROID
  addButton->setToolTip( tr("add waypoint") );
#endif
  delButton = new QPushButton( this );
  delButton->setIcon( QIcon(GeneralConfig::instance()->loadPixmap( "right.png", true )) );
  delButton->setIconSize(QSize(iconSize, iconSize));
#ifndef ANDROID
  delButton->setToolTip( tr("remove waypoint") );
#endif
  QPushButton* okButton = new QPushButton( this );
  okButton->setIcon( QIcon(GeneralConfig::instance()->loadPixmap( "ok.png", true )) );
  okButton->setIconSize(QSize(iconSize, iconSize));
#ifndef ANDROID
  okButton->setToolTip( tr("save task") );
#endif
  QPushButton* cancelButton = new QPushButton( this );
  cancelButton->setIcon( QIcon(GeneralConfig::instance()->loadPixmap( "cancel.png", true )) );
  cancelButton->setIconSize(QSize(iconSize, iconSize));
#ifndef ANDROID
  cancelButton->setToolTip( tr("cancel task") );
#endif

  // all single widgets and layouts in this grid
  QGridLayout* totalLayout = new QGridLayout( this );
  totalLayout->setMargin(5);

  QHBoxLayout* headlineLayout = new QHBoxLayout;
  totalLayout->addLayout( headlineLayout, 0, 0, 1, 3 );

  headlineLayout->setMargin(0);
  headlineLayout->addWidget( new QLabel( tr("Name:") ) );
  headlineLayout->addWidget( taskName );

  // Combo box for toggling between waypoint, airfield, outlanding lists
  listSelectCB = new QComboBox(this);
  listSelectCB->setEditable(false);
  headlineLayout->addWidget( listSelectCB );
  //headlineLayout->addSpacing(25);

  // QStyle* style = QApplication::style();
  defaultButton = new QPushButton;
  // defaultButton->setIcon(style->standardIcon(QStyle::SP_DialogResetButton));
  defaultButton->setIcon( QIcon(GeneralConfig::instance()->loadPixmap("clear-32.png")) );
  defaultButton->setIconSize(QSize(iconSize, iconSize));
#ifndef ANDROID
  defaultButton->setToolTip(tr("Set task figure default schemas"));
#endif
  headlineLayout->addWidget(defaultButton);
  //headlineLayout->addSpacing(20);

  editButton = new QPushButton;
  editButton->setIcon( QIcon(GeneralConfig::instance()->loadPixmap("edit_new.png")) );
  editButton->setIconSize(QSize(iconSize, iconSize));
#ifndef ANDROID
  editButton->setToolTip(tr("Edit selected waypoint"));
#endif
  headlineLayout->addWidget(editButton);
  //headlineLayout->setSpacing(20);
  headlineLayout->addWidget(okButton);
  //headlineLayout->addSpacing(20);
  headlineLayout->addWidget(cancelButton);

  totalLayout->addWidget( taskList, 1, 0 );

  int scale = Layout::getIntScaledDensity();

  // contains the task editor buttons
  QVBoxLayout* buttonLayout = new QVBoxLayout;
  buttonLayout->setMargin(0);
  buttonLayout->addStretch( 10 );
  buttonLayout->addWidget( invertButton );
  buttonLayout->addSpacing(10 * scale);
  buttonLayout->addWidget( upButton );
  buttonLayout->addSpacing(10 * scale);
  buttonLayout->addWidget( downButton );
  buttonLayout->addSpacing(30 * scale);
  buttonLayout->addWidget( addButton  );
  buttonLayout->addSpacing(10 * scale);
  buttonLayout->addWidget( delButton );
  buttonLayout->addStretch( 10 );
  totalLayout->addLayout( buttonLayout, 1, 1 );

  // Waypoint list is always shown
  listSelectText.append( tr("Waypoints") );
  pointDataList.append( new WaypointListWidget( this, false ) );

  // The other list are only shown, if they are not empty.
  // Airfield list
  if( _globalMapContents->getListLength( MapContents::AirfieldList ) > 0 ||
      _globalMapContents->getListLength( MapContents::GliderfieldList ) > 0 )
    {
      listSelectText.append( tr("Airfields") );
      QVector<enum MapContents::ListID> itemList;
      itemList << MapContents::AirfieldList << MapContents::GliderfieldList;
      pointDataList.append( new AirfieldListWidget( itemList, this, false ) );
    }

  // Outlanding list
  if( _globalMapContents->getListLength( MapContents::OutLandingList ) > 0 )
    {
      listSelectText.append( tr("Fields") );
      QVector<enum MapContents::ListID> itemList;
      itemList << MapContents::OutLandingList;
      pointDataList.append( new AirfieldListWidget( itemList, this, false ) );
    }

  // Navaids
  if( _globalMapContents->getListLength( MapContents::RadioList ) > 0 )
    {
      listSelectText.append( tr("Navaids") );
      QVector<enum MapContents::ListID> itemList;
      itemList << MapContents::RadioList;
      pointDataList.append( new RadioPointListWidget( itemList, this, false ) );
    }

  // Hotspots
  if( _globalMapContents->getListLength( MapContents::HotspotList ) > 0 )
    {
      listSelectText.append( tr("Hotspots") );
      QVector<enum MapContents::ListID> itemList;
      itemList << MapContents::HotspotList;
      pointDataList.append( new SinglePointListWidget( itemList, this, false ) );
    }

  for( int i = 0; i < pointDataList.size(); i++ )
    {
      listSelectCB->addItem(listSelectText[i], i);
      totalLayout->addWidget( pointDataList[i], 1, 2 );
    }

  QList<Waypoint>& wpList = _globalMapContents->getWaypointList();

  if ( wpList.count() == 0 &&
       ( _globalMapContents->getListLength( MapContents::AirfieldList ) > 0 ||
         _globalMapContents->getListLength( MapContents::GliderfieldList ) > 0 ) )
    {
      // If waypoint list is zero and airfields are available,
      // select the airfield list:
      listSelectCB->setCurrentIndex( 1 );
      slotToggleList( 1 );
    }
  else
    {
      // If airfield list is zero, select the waypoint list as default.
      listSelectCB->setCurrentIndex( 0 );
      slotToggleList( 0 );
    }

  if ( editState == TaskEditor::edit )
    {
      taskName->setText( task2Edit->getTaskName() );

      QList<TaskPoint *> tmpList = task2Edit->getTpList();

      // @AP: Make a deep copy from all elements of the list
      for ( int i=0; i < tmpList.count(); i++ )
        {
          tpList.append( new TaskPoint( *tmpList.at(i)) );
        }
    }

  showTask();

  connect( addButton,    SIGNAL( clicked() ),
           this, SLOT( slotAddWaypoint() ) );
  connect( delButton,    SIGNAL( clicked() ),
           this, SLOT( slotRemoveWaypoint() ) );
  connect( upButton,     SIGNAL( clicked() ),
           this, SLOT( slotMoveWaypointUp() ) );
  connect( downButton,   SIGNAL( clicked() ),
           this, SLOT( slotMoveWaypointDown() ) );
  connect( invertButton, SIGNAL( clicked() ),
           this, SLOT( slotInvertWaypoints() ) );

  connect( defaultButton, SIGNAL(clicked()),
           this, SLOT(slotSetTaskPointsDefaultSchema()));
  connect( editButton, SIGNAL(clicked()),
           this, SLOT(slotEditTaskPoint()));

  connect( okButton, SIGNAL( clicked() ),
           this, SLOT( slotAccept() ) );
  connect( cancelButton, SIGNAL( clicked() ),
           this, SLOT( slotReject() ) );

  connect( listSelectCB, SIGNAL(activated(int)),
           this, SLOT(slotToggleList(int)));

  connect( taskList, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
           this, SLOT(slotCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)) );
}

TaskEditor::~TaskEditor()
{
  qDeleteAll(tpList);
  tpList.clear();
}

void TaskEditor::showTask()
{
  if ( tpList.count() == 0 )
    {
      enableCommandButtons();
      return;
    }

  // That updates all task internal data
  task2Edit->setTaskPointList( FlightTask::copyTpList( &tpList ) );

  QString txt = task2Edit->getTaskTypeString() +
                " / " + task2Edit->getTaskDistanceString();

  setWindowTitle(txt);

  QList<TaskPoint *> tmpList = task2Edit->getTpList();

  taskList->clear();

  QStringList rowList;
  QString typeName, distance, idString;

  double distTotal = 0.0;

  for( int loop = 0; loop < tmpList.size(); loop++ )
    {
      TaskPoint* tp = tmpList.at( loop );
      typeName = tp->getTaskPointTypeString();

      distTotal += tp->distance;

      distance = Distance::getText(tp->distance*1000, true, 1);
      idString = QString( "%1").arg( loop, 2, 10, QLatin1Char('0') );

      rowList.clear();
      rowList << idString << typeName << tp->getWPName() << distance;

      const int iconSize = Layout::iconSize( font() );

      QTreeWidgetItem* item = new QTreeWidgetItem(rowList, 0);

      if( tmpList.size() > 1 )
        {
          item->setIcon ( 1, tp->getIcon( iconSize ) );
        }

      if( tp->getUserEditFlag() == true )
        {
          // Mark the user edited entry
          item->setBackground( 1, QBrush(Qt::yellow) );
        }

      taskList->addTopLevelItem( item );

      // reselect last selected item
      if( lastSelectedItem == loop )
        {
          taskList->setCurrentItem( taskList->topLevelItem(loop) );
        }
    }

  enableCommandButtons();
  lastSelectedItem = -1;

  if( distTotal > 0.0 )
    {
      distance = Distance::getText( distTotal*1000, true, 1 );

      rowList.clear();
      rowList << "Total" << "" << tr("Total") << distance;

      QTreeWidgetItem* item = new QTreeWidgetItem(rowList, 0);

      // make the total line unselectable
      item->setFlags( Qt::ItemIsEnabled );

      QFont font = item->font(1);
      font.setBold( true );
      item->setFont( 2, font );
      item->setFont( 3, font );
      taskList->addTopLevelItem( item );
    }

  resizeTaskListColumns();
}

/**
 * aligns the task list columns to their contents
*/
void TaskEditor::resizeTaskListColumns()
{
  taskList->resizeColumnToContents(0);
  taskList->resizeColumnToContents(1);
  taskList->resizeColumnToContents(2);
  taskList->resizeColumnToContents(3);
}

void TaskEditor::slotAddWaypoint()
{
  Waypoint* wp = pointDataList[listSelectCB->currentIndex()]->getCurrentWaypoint();

  if( wp == 0 )
    {
      return;
    }

  QTreeWidgetItem *item = taskList->currentItem();

  if( item == 0 )
    {
      // empty list

      // A taskpoint is only a single point and not more!
      TaskPoint* tp = new TaskPoint( *wp );
      tpList.append( tp );

      // Remember last position.
      lastSelectedItem = 0;
    }
  else
    {
      int id = taskList->indexOfTopLevelItem( item );
      id++;

      // A taskpoint is only a single point and not more!
      TaskPoint* tp = new TaskPoint( *wp );
      tpList.insert( id, tp );

      // Remember last position.
      lastSelectedItem = id;
    }

  setTaskPointFigureSchemas( tpList, false );
  showTask();
}

void TaskEditor::slotRemoveWaypoint()
{
  QTreeWidgetItem* selected = taskList->currentItem();

  if( selected == 0 )
    {
      return;
    }

  int id = taskList->indexOfTopLevelItem( taskList->currentItem() );

  delete taskList->takeTopLevelItem( taskList->currentIndex().row() );
  delete tpList.takeAt( id );

  // Remember last position.
  if( id >= tpList.size() )
    {
      lastSelectedItem = tpList.size() - 1;
    }
  else
    {
      lastSelectedItem = id;
    }

  setTaskPointFigureSchemas( tpList, false );
  showTask();
}

void TaskEditor::slotInvertWaypoints()
{
  if ( tpList.count() < 2 )
    {
      // not possible to invert order, if elements are less than 2
      return;
    }

  // invert list order
  for ( int i = tpList.count() - 2; i >= 0; i-- )
    {
      TaskPoint* tp = tpList.at(i);
      tpList.removeAt(i);
      tpList.append( tp );
    }

  // If start and end point have the same coordinates, the taskpoint figure
  // schemes should be switched also.
  WGSPoint* start = tpList.first()->getWGSPositionPtr();
  WGSPoint* end   = tpList.last()->getWGSPositionPtr();

  if( *start == *end )
    {
      TaskPoint* tps = tpList.first();
      TaskPoint* tpe = tpList.last();

      enum GeneralConfig::ActiveTaskFigureScheme ss =
          tps->getActiveTaskPointFigureScheme();

      enum GeneralConfig::ActiveTaskFigureScheme es =
          tpe->getActiveTaskPointFigureScheme();

      if( ss != es )
        {
          tps->setActiveTaskPointFigureScheme( es );
          tpe->setActiveTaskPointFigureScheme( ss );
        }
    }

  // After an invert the first task item is selected.
  lastSelectedItem = 0;
  showTask();
}

void TaskEditor::slotEditTaskPoint ()
{
  int id = taskList->indexOfTopLevelItem( taskList->currentItem() );

  if( id < 0 )
  {
    return;
  }

  m_lastEditedTP = id;

  TaskPoint* modPoint = tpList.at(id);
  TaskPointEditor *tpe = new TaskPointEditor(this, modPoint );

  connect( tpe, SIGNAL(taskPointEdited(TaskPoint*)),
           this, SLOT(slotTaskPointEdited(TaskPoint*)));

  tpe->setVisible( true );
}

void TaskEditor::slotTaskPointEdited( TaskPoint* editedTaskPoint )
{
  Q_UNUSED( editedTaskPoint )

  // That updates the task point list in the flight task.
  showTask();

  if( m_lastEditedTP >= 0 )
    {
      // Set selection back to the state before editing
      QTreeWidgetItem* item = taskList->topLevelItem( m_lastEditedTP );

      if( item != 0 )
	{
	  taskList->setCurrentItem( item );
	}
    }
}

void TaskEditor::slotAccept()
{
  // Check, if a valid task has been defined. Tasks with less than
  // four task points are incomplete
  if ( tpList.count() < 2 )
    {
      QMessageBox mb( QMessageBox::Critical,
                      tr( "Task Incomplete" ),
                      tr( "Task needs at least a start and a finish point!" ),
                      QMessageBox::Ok,
                      this );

    #ifdef ANDROID

      mb.show();
      QPoint pos = mapToGlobal(QPoint( width()/2  - mb.width()/2,
                                       height()/2 - mb.height()/2 ));
      mb.move( pos );

    #endif

      mb.exec();
      return;
    }

  // Check, if in the task are not two waypoints with the same coordinates
  // in order.
  for( int i = 0; i < tpList.count() - 1; i++ )
    {
      if( tpList.at(i)->getWGSPositionRef() == tpList.at(i+1)->getWGSPositionRef() )
	{
	  QMessageBox mb( QMessageBox::Critical,
			  tr("Double points in order"),
			  QString(tr("Points %1 and %2 have the same coordinates.\nPlease remove one of them!")).arg(i+1).arg(i+2),
			  QMessageBox::Ok,
			  this );

#ifdef ANDROID

	  mb.show();
	  QPoint pos = mapToGlobal(QPoint( width()/2  - mb.width()/2,
					   height()/2 - mb.height()/2 ));
	  mb.move( pos );

#endif

	  mb.exec();
	  return;
	}
    }

  QString txt = taskName->text();

  // Check if the user has entered a task name
  if ( txt.length() == 0 )
    {
      QMessageBox mb( QMessageBox::Critical,
                      tr("Name Missing"),
                      tr("Enter a name for the task to save it"),
                      QMessageBox::Ok,
                      this );

    #ifdef ANDROID

      mb.show();
      QPoint pos = mapToGlobal(QPoint( width()/2  - mb.width()/2,
                                       height()/2 - mb.height()/2 ));
      mb.move( pos );

    #endif

      mb.exec();
      return;
    }

  if ( ( editState == TaskEditor::create && taskNamesInUse.contains( txt ) > 0 ) ||
       ( editState != TaskEditor::create && txt != editedTaskName &&
         taskNamesInUse.contains( txt ) > 0 ) )
    {
      // Check if the task name does not conflict with existing onces.
      // The name must be unique in the task name space
      QMessageBox mb( QMessageBox::Critical,
                      tr( "Name in Use"),
                      tr( "Please enter a different name" ),
                      QMessageBox::Ok,
                      this );

#ifdef ANDROID

      mb.show();
      QPoint pos = mapToGlobal(QPoint( width()/2  - mb.width()/2,
                                       height()/2 - mb.height()/2 ));
      mb.move( pos );

#endif
      mb.exec();
      return;
    }

  // Take over changed task data and publish it
  task2Edit->setTaskName(txt);

  if ( editState == TaskEditor::create )
    {
      emit newTask( task2Edit );
    }
  else
    {
      emit editedTask( task2Edit );
    }

  // closes and destroys window
  close();
}

void TaskEditor::slotReject()
{
  // delete rejected task object
  delete task2Edit;

  // close and destroy window
  close();
}

void TaskEditor::slotMoveWaypointUp()
{
  if( taskList->selectedItems().size() == 0 ||
      taskList->topLevelItemCount() <= 2 )
    {
      return;
    }

  int id = taskList->indexOfTopLevelItem( taskList->currentItem() );

  // we can't move the first item up
  if( id <= 0 )
    {
      return;
    }

  lastSelectedItem = id - 1;

  tpList.move( id, id - 1 );

  setTaskPointFigureSchemas( tpList, false );
  showTask();
}

void TaskEditor::slotMoveWaypointDown()
{
  if( taskList->selectedItems().size() == 0 ||
      taskList->topLevelItemCount() <= 2 )
    {
      return;
    }

  int id = taskList->indexOfTopLevelItem( taskList->currentItem() );

  // we can't move the last item down
  if( id == -1 || id == taskList->topLevelItemCount() - 1 ||
      ( id == taskList->topLevelItemCount() - 2 &&
        taskList->topLevelItem( taskList->topLevelItemCount() - 1)->text( 0 ) == "Total" ) )
    {
      return;
    }

  lastSelectedItem = id + 1;

  tpList.move(id,  id + 1);

  setTaskPointFigureSchemas( tpList, false );
  showTask();
}

/** Toggle between the point data lists on user request */
void TaskEditor::slotToggleList(int index)
{
  for( int i = 0; i < pointDataList.size(); i++ )
    {
      if( i != index )
        {
          pointDataList[i]->hide();
        }
      else
        {
          pointDataList[i]->show();
        }
    }
}

void TaskEditor::slotCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
  Q_UNUSED(current)
  Q_UNUSED(previous)

  enableCommandButtons();
}

void TaskEditor::enableCommandButtons()
{
  if( tpList.size() == 0 )
    {
      upButton->setEnabled( false );
      downButton->setEnabled( false );
      invertButton->setEnabled( false );
      addButton->setEnabled( true );
      delButton->setEnabled( false );
      editButton->setEnabled (false);
      defaultButton->setEnabled (false);
    }
  else if( tpList.size() == 1 )
    {
      upButton->setEnabled( false );
      downButton->setEnabled( false );
      invertButton->setEnabled( false );
      addButton->setEnabled( true );
      delButton->setEnabled( true );
      editButton->setEnabled (true);
      defaultButton->setEnabled (false);
    }
  else
    {
      invertButton->setEnabled( true );
      addButton->setEnabled( true );
      delButton->setEnabled( true );
      editButton->setEnabled( true );
      defaultButton->setEnabled( true );

      if( taskList->topLevelItemCount() && taskList->currentItem() == 0 )
        {
          // If no item is selected we select the first one.
          taskList->setCurrentItem(taskList->topLevelItem(taskList->indexOfTopLevelItem(0)));
        }

      int id = taskList->indexOfTopLevelItem( taskList->currentItem() );

      if( id > 0 )
        {
          upButton->setEnabled( true );
        }
      else
        {
          // At the first position, no up allowed
          upButton->setEnabled( false );
        }

      if( id == -1 || id == taskList->topLevelItemCount() - 1 ||
          ( id == taskList->topLevelItemCount() - 2 &&
            taskList->topLevelItem( taskList->topLevelItemCount() - 1)->text( 0 ) == "Total" ) )
        {
          // At the last allowed down position. No further down allowed.
          downButton->setEnabled( false );
        }
      else
        {
          downButton->setEnabled( true );
        }
    }
}

void TaskEditor::swapTaskPointSchemas( TaskPoint* tp1, TaskPoint* tp2 )
{
  qDebug() << "TaskEditor::swapTaskPointSchemas";

  qDebug() << "tp1=" << tp1->getName() << tp1->getActiveTaskPointFigureScheme();
  qDebug() << "tp2=" << tp2->getName() << tp2->getActiveTaskPointFigureScheme();

  Distance d1, d2;

  d1 = tp1->getTaskCircleRadius();
  d2 = tp2->getTaskCircleRadius();

  tp1->setTaskCircleRadius( d2 );
  tp2->setTaskCircleRadius( d1 );

  d1 = tp1->getTaskSectorInnerRadius();
  d2 = tp2->getTaskSectorInnerRadius();

  tp1->setTaskSectorInnerRadius( d2 );
  tp2->setTaskSectorInnerRadius( d1 );

  d1 = tp1->getTaskSectorOuterRadius();
  d2 = tp2->getTaskSectorOuterRadius();

  tp1->setTaskSectorOuterRadius( d2 );
  tp2->setTaskSectorOuterRadius( d1 );

  double l1, l2;

  l1 = tp1->getTaskLine().getLineLength();
  l2 = tp2->getTaskLine().getLineLength();

  tp1->getTaskLine().setLineLength( l2 );
  tp2->getTaskLine().setLineLength( l1 );

  int a1, a2;

  a1 = tp1->getTaskSectorAngle();
  a2 = tp2->getTaskSectorAngle();

  tp1->setTaskSectorAngle( a2 );
  tp2->setTaskSectorAngle( a1 );

  enum GeneralConfig::ActiveTaskFigureScheme tfs1, tfs2;

  tfs1 = tp1->getActiveTaskPointFigureScheme();
  tfs2 = tp2->getActiveTaskPointFigureScheme();

  tp1->setActiveTaskPointFigureScheme( tfs2 );
  tp2->setActiveTaskPointFigureScheme( tfs1 );

  bool e1, e2;

  e1 = tp1->getUserEditFlag();
  e2 = tp2->getUserEditFlag();

  tp1->setUserEditFlag( e2 );
  tp2->setUserEditFlag( e1 );
}

void TaskEditor::setTaskPointFigureSchemas( QList<TaskPoint *>& tpList,
					    const bool setDefaultFigure )
{
  // As first set the right task point type
  for( int i = 0; i < tpList.size(); i++ )
    {
      if( i == 0 )
        {
          tpList.at(i)->setTaskPointType(TaskPointTypes::Start);
        }
      else if( tpList.size() >= 2 && i == tpList.size() - 1 )
        {
          tpList.at(i)->setTaskPointType(TaskPointTypes::Finish);
        }
      else
        {
          tpList.at(i)->setTaskPointType(TaskPointTypes::Turn);
        }

      // Set task point figure schema to default, if the user has not edited the
      // task point.
      if( setDefaultFigure == true || tpList.at(i)->getUserEditFlag() == false )
	{
	  tpList.at(i)->setConfigurationDefaults();
	}
    }
}

void TaskEditor::slotSetTaskPointsDefaultSchema()
{
  QMessageBox mb( QMessageBox::Question,
                  tr( "Defaults?" ),
                  tr( "Reset all TP schemas to default configuration values?" ),
                  QMessageBox::Yes | QMessageBox::No,
                  this );

  mb.setDefaultButton( QMessageBox::No );

#ifdef ANDROID

  mb.show();
  QPoint pos = mapToGlobal(QPoint( width()/2  - mb.width()/2,
                                   height()/2 - mb.height()/2 ));
  mb.move( pos );

#endif

  if( mb.exec() == QMessageBox::Yes )
    {
      setTaskPointFigureSchemas( tpList, true );
      showTask();
    }
}

void TaskEditor::slotWpEdited( Waypoint &editedWp )
{
  QTreeWidgetItem *item = taskList->currentItem();

  if( item == 0 )
    {
      return;
    }

  int idx = taskList->indexOfTopLevelItem( item );

  if( idx == -1 )
    {
      return;
    }

  if( idx >= tpList.size() )
    {
      return;
    }

  // Take old point from the list
  TaskPoint* tp = tpList.takeAt( idx );
  delete tp;

  tp = new TaskPoint( editedWp );
  tpList.insert( idx, tp );

  // Remember last position.
  lastSelectedItem = idx;

  setTaskPointFigureSchemas( tpList, false );
  showTask();
}
