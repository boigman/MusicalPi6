/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>

#include <QCoreApplication>
#include <cmath>

#include "button.h"
#include "keyboard.h"

//! [0]
Keyboard::Keyboard(QWidget *parent)
    : QWidget(parent)
{
    display = new QLineEdit("");
    display->setReadOnly(true);
    display->setAlignment(Qt::AlignRight);
//    display->setAlignment(Qt::AlignBottom);
    display->setMaxLength(15);

    QFont font = display->font();
    font.setPointSize(font.pointSize() + 8);
    display->setFont(font);

    QString alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for(int i=0; i < NumLetterButtons; ++i) {
        QString currLetter = alphabet.mid(i,1);
//        qDebug() << "currLetter: " + currLetter;
        letterButtons[i] = createButton(currLetter, SLOT(letterClicked()));
    }
    bkspButton = createButton("Bksp", SLOT(bkspClicked()));
    spcButton = createButton("Spc", SLOT(spcClicked()));

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    mainLayout->addWidget(display, 0, 0, 1, 6);
    for (int i = 0; i < NumLetterButtons; ++i) {
        int row = (i / 7)+1;
        int column = (i % 7) + 1;
//        qDebug() << "addWidget: " << (QString)letterButtons[i]->text() << ", row: " << QString::number(row) << ", col: " << QString::number(column);
        mainLayout->addWidget(letterButtons[i], row, column,1,1);
    }
    mainLayout->addWidget(spcButton, 4, 6, 1, 1);
    mainLayout->addWidget(bkspButton, 4, 7, 1, 1);

    setLayout(mainLayout);

    setWindowTitle(tr("Keyboard"));
}

uint toKey(QString const & str) {
    QKeySequence seq(str);
    uint keyCode;

    // We should only working with a single key here
    if(seq.count() == 1)
        keyCode = seq[0];
    else {
        // Should be here only if a modifier key (e.g. Ctrl, Alt) is pressed.
 //       assert(seq.count() == 0);

        // Add a non-modifier key "A" to the picture because QKeySequence
        // seems to need that to acknowledge the modifier. We know that A has
        // a keyCode of 65 (or 0x41 in hex)
        seq = QKeySequence(str + "+A");
 //       assert(seq.count() == 1);
 //       assert(seq[0] > 65);
        keyCode = seq[0] - 65;
    }
    return keyCode;
}

void Keyboard::bkspClicked()
{
    Button *clickedButton = qobject_cast<Button *>(sender());
    QString letterValue = clickedButton->text();
    QString textString = display->text().left(display->text().size()-1);

    display->setText(textString);
    qDebug() << "in bkspClicked: ";
    QKeyEvent *event = new QKeyEvent ( QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
//    QCoreApplication::postEvent (this->parent(), event);
    QCoreApplication::postEvent(this->parent(), event);
}

void Keyboard::spcClicked()
{
//    Button *clickedButton = qobject_cast<Button *>(sender());
    QString letterValue = " ";
    display->setText(display->text() + letterValue);
    qDebug() << "in spcClicked: " << letterValue;
    QKeyEvent *event = new QKeyEvent(QKeyEvent::KeyPress, Qt::Key_Space, Qt::NoModifier," ");

//    QKeyEvent *event = new QKeyEvent ( QEvent::KeyPress, toKey(letterValue), Qt::NoModifier);
//     QKeyEvent *event = new QKeyEvent(QKeyEvent::KeyPress, QChar(Qt::Key_Space), Qt::NoModifier);
//    QCoreApplication::postEvent (this->parent(), event);
    QCoreApplication::postEvent(this->parent(), event);
}

void Keyboard::letterClicked()
{
    Button *clickedButton = qobject_cast<Button *>(sender());
    QString letterValue = clickedButton->text();
    display->setText(display->text() + letterValue);
    qDebug() << "in letterClicked: " << letterValue;
    QKeyEvent *event = new QKeyEvent ( QEvent::KeyPress, toKey(letterValue), Qt::NoModifier);
//    QCoreApplication::postEvent (this->parent(), event);
    QCoreApplication::postEvent(this->parent(), event);
}

void Keyboard::clear() {
    display->setText("");
}

Button *Keyboard::createButton(const QString &text, const char *member)
{
    Button *button = new Button(text);
    connect(button, SIGNAL(clicked()), this, member);
    return button;
}
