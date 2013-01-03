/***********************************************************************
**
**   numberEditor.cpp
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c): 2012-2013 Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#include <QtGui>

#include "numberInputPad.h"
#include "numberEditor.h"

/**
 * Constructor
 */
NumberEditor::NumberEditor( QWidget *parent,
                            QString number,
                            QString prefix,
                            QString suffix ) :
  QLabel( parent ),
  m_nip(0),
  m_prefix(prefix),
  m_number(number),
  m_suffix(suffix),
  m_title(tr("edit number")),
  m_decimalFlag(true),
  m_pmFlag(true),
  m_validator(0),
  m_inputMask(""),
  m_maxLength(32767)
{
//  QPalette p = palette();
//  p.setColor( QPalette::Window, Qt::white );
//  setPalette(p);
//  setAutoFillBackground( true );

  setBackgroundRole( QPalette::Light );
  setAutoFillBackground( true );
  setAlignment(Qt::AlignCenter);
  setMargin(1);
  setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  setText();
}

NumberEditor::~NumberEditor()
{
}

void NumberEditor::mousePressEvent( QMouseEvent* event )
{
  // Opens the number input pad after a mouse press.
  if( ! m_nip )
    {
      m_nip = new NumberInputPad( m_number, this );
      m_nip->setWindowTitle( m_title );
      m_nip->setDecimalVisible( m_decimalFlag );
      m_nip->setPmVisible( m_pmFlag );
      m_nip->setValidator( m_validator );
      m_nip->setMaxLength( m_maxLength );
      m_nip->setInputMask( m_inputMask );
      connect( m_nip, SIGNAL(number(const QString&) ),
               SLOT(slot_Number(const QString&)) );
      m_nip->show();

#ifdef ANDROID
      m_nip->getEditor()->setFocus();

      QPoint pos = mapToGlobal(QPoint( width()/2 - m_nip.width()/2, height()/2 - m_nip.height()/2 ));
      m_nip.move( pos );
#endif

    }

  event->accept();
}

void NumberEditor::slot_Number( const QString& number )
{
  m_nip = 0;
  m_number = number;
  setText();
  emit numberEdited( number );
}
