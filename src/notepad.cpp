/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QFont>
#include <QFontDialog>
#include <QScrollBar>

#include "notepad.h"
#include "ui_notepad.h"
#include "highlighter.h"

Notepad::Notepad(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Notepad)
{
    ui->setupUi(this);

    connect(ui->actionNew, &QAction::triggered, this, &Notepad::newDocument);
    connect(ui->actionOpen, &QAction::triggered, this, &Notepad::open);
    connect(ui->actionExit, &QAction::triggered, this, &Notepad::exit);
    connect(ui->actionCopy, &QAction::triggered, this, &Notepad::copy);
    connect(ui->actionCut, &QAction::triggered, this, &Notepad::cut);
    connect(ui->actionPaste, &QAction::triggered, this, &Notepad::paste);
    connect(ui->actionUndo, &QAction::triggered, this, &Notepad::undo);
    connect(ui->actionRedo, &QAction::triggered, this, &Notepad::redo);
    connect(ui->actionAbout, &QAction::triggered, this, &Notepad::about);
    connect(ui->actionSave, &QAction::triggered, this, &Notepad::save);

// Disable menu actions for unavailable features

#if !QT_CONFIG(clipboard)
    ui->actionCut->setEnabled(false);
    ui->actionCopy->setEnabled(false);
    ui->actionPaste->setEnabled(false);
#endif
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(10);
    ui->textEdit->setFont(font);
    ui->textBuildView->setFont(font);

    highlighter = new Highlighter(ui->textEdit->document());
    highlighter = new Highlighter(ui->textBuildView->document());

    // Load settings
    QFile settingsFile(QCoreApplication::applicationDirPath()+"/userdata.ini");
    if (!settingsFile.open(QIODevice::ReadOnly | QFile::Text)) { // The write mode will create the file for us.
        QMessageBox::warning(this, "Info", "Couldn't create or load userdata.ini: " + settingsFile.errorString());
        return;
    }
    QTextStream inSrc(&settingsFile);
    QString textSrc = inSrc.readAll();
    QStringList settingsLines = textSrc.split("\n");
    foreach (const QString &setting, settingsLines) {
        if (setting.startsWith("lastFile = ")) Notepad::openFile(setting.split("lastFile = ")[1]);
    }
    settingsFile.close();
}

void Notepad::changeSetting(QString settingName, QString settingValue) {
    QFile settingsFile(QCoreApplication::applicationDirPath()+"/userdata.ini");
    if (!settingsFile.open(QIODevice::ReadWrite | QFile::Text)) {
        QMessageBox::warning(this, "Info", "Couldn't create or load userdata.ini: " + settingsFile.errorString());
        return;
    }
    QTextStream inSrc(&settingsFile);
    QString textSrc = inSrc.readAll();
    QStringList newSettingsLines;
    QStringList settingsLines = textSrc.split("\n");
    bool settingChanged = false;
    foreach (const QString &setting, settingsLines) {
        if (setting.startsWith(settingName+" = ")) {
            newSettingsLines += settingName+" = "+settingValue;
            settingChanged = true;
        }
        else newSettingsLines += setting;
    }
    if (!settingChanged) newSettingsLines += settingName+" = "+settingValue;
    QTextStream out(&settingsFile);
    out << newSettingsLines.join("\n");
    settingsFile.close();
}

Notepad::~Notepad()
{
    delete ui;
}

void Notepad::compileSourceOnTextChanged()
{
    QString sourcePatchesText = ui->textEdit->toPlainText();
    QString compiledPatchesText = "";
    QStringList sourceLines = sourcePatchesText.split("\n");
    QStringList patchLines;
    QList<int> patchAddresses;

    QStringList patchInterpretedLines;
    int interpretedAddrIndex = 0x0;
    int interpretedConstantsInsertLine = -1;

    int replaceCaveSizeLine = -1;
    int addrIndex = 0x0;

    bool inCodeCaveSection = false;

    const QStringList interpretableLoadStoreInstructions = {"lfs", "lfd", "lis", "lwz", "lbz", "stfs"};
    const QStringList cemuhookShorthandConstants = {".int", ".float", ".byte", ".string"/*Didn't bother fully implementing .string, but it should work-ish.*/};

    if (!sourcePatchesText.isEmpty()) {
        try {
            for (int i=0; i<sourceLines.length(); i++) {
                QString hasCemuhookShorthandConstant = "";
                // Separate comments and the line that needs to be parsed
                QString strippedComment = (sourceLines[i].indexOf("#") > sourceLines[i].indexOf(";"))? "#"+sourceLines[i].section("#", 1) : ";"+sourceLines[i].section(";", 1);
                QString parseLine = QString(sourceLines[i]).remove(strippedComment);

                // Check for things that are not instructions
                if (parseLine.contains("codeCaveSize")) {
                    if (parseLine.contains("auto")) replaceCaveSizeLine = i;
                    interpretedConstantsInsertLine = i+1;
                }

                if (parseLine.trimmed().startsWith("[") && parseLine.trimmed().endsWith("]") && !patchAddresses.empty()) {
                    if (onlyShowFirstPatch) {
                        break;
                    }
                    // Found new patch and reached the end of the previous patch.
                    if (replaceCaveSizeLine != -1) patchLines.replace(replaceCaveSizeLine, "codeCaveSize = 0x"+QString::number(addrIndex+interpretedAddrIndex, 16));
                    for (int j=0; j<patchLines.size(); j++) {
                        if (j==interpretedConstantsInsertLine) {
                            compiledPatchesText+=patchInterpretedLines.join("\n");
                            patchInterpretedLines.clear();
                        }
                        if (patchAddresses[j] == -1) compiledPatchesText+=patchLines[j]+"\n";
                        else compiledPatchesText += patchLines[j].arg(patchAddresses[j]+interpretedAddrIndex, 7, 16, QChar('0')) + "\n";
                    }
                    patchLines.clear();
                    patchAddresses.clear();
                    addrIndex = 0x0;
                    replaceCaveSizeLine = -1;
                    interpretedConstantsInsertLine = -1;
                    inCodeCaveSection = false;
                    interpretedAddrIndex = 0x0;
                    patchInterpretedLines.clear();
                }

                // Check if line contains Cemuhook shorthand constant definitions
                for (int j=0; j<cemuhookShorthandConstants.size(); j++) {
                    if (parseLine.contains(cemuhookShorthandConstants[j])) {
                        hasCemuhookShorthandConstant = cemuhookShorthandConstants[j];
                        break;
                    }
                }

                // Add lines for each thing
                if (parseLine.trimmed().endsWith(":") && !parseLine.contains("=")) {
                    QString symbolName(parseLine.trimmed());
                    symbolName.chop(1);
                    patchLines.append(symbolName+" = 0x%1");
                    patchAddresses.append(addrIndex);
                    inCodeCaveSection = true;
                }
                else if (!hasCemuhookShorthandConstant.isEmpty() && parseLine.startsWith("_") && parseLine.contains("=")) {
                    patchLines.append(parseLine.split(" ")[0]+" = 0x%1");
                    patchLines.append("0x%1 = " +QString(parseLine).section(" ", 1).remove("=").trimmed());
                    patchAddresses.append(addrIndex);
                    patchAddresses.append(addrIndex);
                    addrIndex+=4;
                }
                else if (!hasCemuhookShorthandConstant.isEmpty() && interpretableLoadStoreInstructions.contains(parseLine.split(" ")[0]) && interpretedConstantsInsertLine != -1) {
                    QString valueArg=parseLine.split(",")[1].trimmed();
                    QString registerArg=parseLine.split(" ")[1].split(",")[0];
                    if (interpretableLoadStoreInstructions.contains(valueArg)) QMessageBox::warning(this, "Compile error :/", "Expected a Cemuhook constant to be the second argument in this load instruction.");
                    QString strValue = QString(valueArg).split(hasCemuhookShorthandConstant)[1].remove("(").split(")")[0];
                    patchInterpretedLines.append("");

                    // Deal with values that use Cemu's presets
                    bool pureNumber;
                    double value(strValue.toDouble(&pureNumber));

                    QString AddrRegisterHint = QString(registerArg).replace(QChar('f'), QChar('r')).trimmed();
                    if (parseLine.split("@l(").length() == 2) AddrRegisterHint = parseLine.split("@l(")[1].remove(")").trimmed(); // Use hint from line

                    if (pureNumber) {
                        QString symbolName = "_const"+QString(hasCemuhookShorthandConstant).remove(1, 1).replace(0, 1, QString(hasCemuhookShorthandConstant)[1].toUpper())+QString::number(value, 'f', 1);
                        patchInterpretedLines.append(QString("%1 = 0x%2").arg(symbolName).arg(interpretedAddrIndex, 7, 16, QChar('0')));
                        patchInterpretedLines.append(QString("0x%1 = %2(%3)").arg(interpretedAddrIndex, 7, 16, QChar('0')).arg(hasCemuhookShorthandConstant).arg(value, 7, 'f'));
                        interpretedAddrIndex+=4;
                        patchLines.append("0x%1 = lis "+QString(AddrRegisterHint)+", "+symbolName+"@ha");
                        patchAddresses.append(addrIndex);
                        patchLines.append("0x%1 = "+parseLine.split(" ")[0]+" "+registerArg+", "+symbolName+"@l("+AddrRegisterHint+")");
                        patchAddresses.append(addrIndex+4);
                        addrIndex+=8;
                    }
                    else {
                        QString symbolName = strValue.split("$")[1].split("*")[0].split("-")[0].split("+")[0].split("/")[0];
                        symbolName = "_preset_"+symbolName;
                        if (!patchLines.filter(symbolName).empty()) {
                            int uniqueId = 1;
                            while(!patchLines.filter(symbolName+QString(uniqueId)).empty()) uniqueId++;
                            symbolName+=QString::number(uniqueId);
                        }
                        patchInterpretedLines.append(QString("%1 = 0x%2").arg(symbolName).arg(interpretedAddrIndex, 7, 16, QChar('0')));
                        patchInterpretedLines.append(QString("0x%1 = %2(%3)").arg(interpretedAddrIndex, 7, 16, QChar('0')).arg(hasCemuhookShorthandConstant).arg(strValue));
                        interpretedAddrIndex+=4;
                        patchLines.append("0x%1 = lis "+AddrRegisterHint+", "+symbolName+"@ha");
                        patchAddresses.append(addrIndex);
                        patchLines.append("0x%1 = "+parseLine.split(" ")[0]+" "+registerArg+", "+symbolName+"@l("+AddrRegisterHint+")");
                        patchAddresses.append(addrIndex+4);
                        addrIndex+=8;
                    }
                }
                else if (!parseLine.trimmed().isEmpty() && inCodeCaveSection) {
                    patchLines.append("0x%1 = "+parseLine);
                    patchAddresses.append(addrIndex);
                    addrIndex+=4;
                }
                else {
                    patchLines.append(parseLine);
                    patchAddresses.append(-1);
                }
                if (parseLine.trimmed() == "blr") inCodeCaveSection = false;
                if (strippedComment.length() > 2 && !removeCommentsFromSource) patchLines.last().append(strippedComment);
            }

            // Finish and append last patch.
            if (replaceCaveSizeLine != -1) patchLines.replace(replaceCaveSizeLine, "codeCaveSize = 0x"+QString::number(addrIndex+interpretedAddrIndex, 16));
            for (int j=0; j<patchLines.size(); j++) {
                if (j==interpretedConstantsInsertLine) {
                    compiledPatchesText+=patchInterpretedLines.join("\n");
                    patchInterpretedLines.clear();
                }
                if (patchAddresses[j] == -1) compiledPatchesText+=patchLines[j]+"\n";
                else compiledPatchesText += patchLines[j].arg(patchAddresses[j]+interpretedAddrIndex, 7, 16, QChar('0')) + "\n";
            }

            // Set text to display compiled output
            setCompiledOutput(compiledPatchesText);
        } catch (...) {
            // Don't do anything, prevent crashing
            ui->textBuildView->setPlainText("ERROR: Couldn't compile the source.");
        }
    }
}

void Notepad::setCompiledOutput(QString text) {
    int currentValue = ui->textBuildView->verticalScrollBar()->value();
    ui->textBuildView->setPlainText(text);
    ui->textBuildView->verticalScrollBar()->setValue(currentValue);
}

void Notepad::newDocument()
{
    sourceFile.clear();
    buildFile.clear();
    ui->textEdit->setText(QString());
}

void Notepad::open() {
    Notepad::openFile(QFileDialog::getOpenFileName(this, "Open the patches.txt file", "", tr("patches.txt;;Any files(*.*)")));
}

void Notepad::openFile(QString buildFile) {
    Notepad::buildFile = buildFile;
    // Read patches.txt file
    QFile file(buildFile);
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, "Warning", "Cannot open patches.txt file: " + file.errorString());
        return;
    }
    setWindowTitle(buildFile);
    QTextStream in(&file);
    QString text = in.readAll();
    ui->textBuildView->setPlainText(text);
    file.close();

    // Create file to sourcePatches.txt in the same directory
    QFileInfo sourcePatches(buildFile);
    sourceFile = sourcePatches.absolutePath()+"/sourcePatches.txt";
    QFile srcFile(sourceFile);
    if (!srcFile.open(QIODevice::ReadWrite | QFile::Text)) { // The write mode will create the file for us.
        QMessageBox::warning(this, "Info", "Couldn't find/create the existing source for patches.txt: " + srcFile.errorString());
        return;
    }
    QTextStream inSrc(&srcFile);
    QString textSrc = inSrc.readAll();
    ui->textEdit->setPlainText(textSrc);
    ui->actionSave->setEnabled(true);
    srcFile.close();
}

void Notepad::save()
{
    {
        // Patches.txt file
        QFile file(buildFile);
        if (!file.open(QIODevice::WriteOnly | QFile::Text)) {
            QMessageBox::warning(this, "Warning", "Cannot save patches.txt file: " + file.errorString());
            return;
        }
        QTextStream out(&file);
        QString text = ui->textBuildView->toPlainText();
        out << text;
        file.close();
    }
    {
        // sourcePatches.txt file
        QFile srcFile(sourceFile);
        if (!srcFile.open(QIODevice::WriteOnly | QFile::Text)) {
            QMessageBox::warning(this, "Warning", "Cannot save sourcePatches.txt file: " + srcFile.errorString());
            return;
        }
        QTextStream out(&srcFile);
        QString text = ui->textEdit->toPlainText();
        out << text;
        srcFile.close();
    }

    Notepad::changeSetting("lastFile", Notepad::buildFile);
}

void Notepad::exit()
{
    QCoreApplication::quit();
}

void Notepad::copy()
{
#if QT_CONFIG(clipboard)
    ui->textEdit->copy();
#endif
}

void Notepad::cut()
{
#if QT_CONFIG(clipboard)
    ui->textEdit->cut();
#endif
}

void Notepad::paste()
{
#if QT_CONFIG(clipboard)
    ui->textEdit->paste();
#endif
}

void Notepad::undo()
{
     ui->textEdit->undo();
}

void Notepad::redo()
{
    ui->textEdit->redo();
}

void Notepad::about()
{
   QMessageBox::about(this, tr("About Cemuhook Patches Writer"),
                tr("Modified Qt's notepad example. "
                   "A program that makes writing Cemuhook patches a bit easier by making it more organic and less error-prone."));

}

void Notepad::on_textEdit_textChanged()
{
    compileSourceOnTextChanged();
}

void Notepad::on_actionKeep_window_always_on_top_toggled(bool arg1)
{
    Notepad::setWindowFlag(Qt::WindowStaysOnTopHint, arg1);
    Notepad::show();
}

void Notepad::on_actionRemove_comments_from_compiled_source_toggled(bool arg1)
{
    removeCommentsFromSource = arg1;
    compileSourceOnTextChanged();
}
void Notepad::on_actionOnly_show_first_patch_toggled(bool arg1)
{
    onlyShowFirstPatch = arg1;
    compileSourceOnTextChanged();
}

void Notepad::on_textEdit_selectionChanged()
{

}
