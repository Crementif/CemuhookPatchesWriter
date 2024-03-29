/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
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

#ifndef NOTEPAD_H
#define NOTEPAD_H

//! [all]
//! [1]
#include <QMainWindow>
#include "highlighter.h"
//! [1]

//! [2]
QT_BEGIN_NAMESPACE
namespace Ui {
class Notepad;
}
QT_END_NAMESPACE
//! [2]

//! [3]
class Notepad : public QMainWindow
{
    Q_OBJECT
//! [3]

//! [4]
public:
    explicit Notepad(QWidget *parent = nullptr);
//! [4]
//! [5]
    ~Notepad();
//! [5]

private slots:
    void newDocument();

    void open();

    void openFile(QString buildFile);

    void save();

    void exit();

    void copy();

    void cut();

    void paste();

    void undo();

    void redo();

    void about();

    void compileSourceOnTextChanged(QString sourcePatchesText, bool removeCommentsFromSource);

    void setCompiledOutput(QString text);

    void changeSetting(QString settingName, QString settingValue);
//! [6]

    void on_textEdit_textChanged();

    void on_actionKeep_window_always_on_top_toggled(bool arg1);

    void on_actionRemove_comments_from_compiled_source_toggled(bool arg1);

    void on_actionOnly_show_first_patch_toggled(bool arg1);
private:
    Ui::Notepad *ui;
    QString buildFile;
    QString sourceFile;
    bool removeCommentsFromSource = false;
    bool onlyShowFirstPatch = false;
    Highlighter *highlighter;
    Highlighter *highlighter2;
//! [6]
};
//! [all]

#endif // NOTEPAD_H
