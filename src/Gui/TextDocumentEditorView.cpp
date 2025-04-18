/***************************************************************************
 *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#endif

#include "TextDocumentEditorView.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Document.h"
#include "MainWindow.h"


using namespace Gui;

TYPESYSTEM_SOURCE_ABSTRACT(Gui::TextDocumentEditorView, Gui::MDIView)  // NOLINT

TextDocumentEditorView::TextDocumentEditorView(App::TextDocument* txtDoc,
                                               QPlainTextEdit* e,
                                               QWidget* parent)
    : MDIView(Application::Instance->getDocument(txtDoc->getDocument()), parent)
    , editor {e}
    , textDocument {txtDoc}
{
    setupEditor();
    setupConnection();
    setCentralWidget(editor);
    setWindowIcon(Gui::BitmapFactory().iconFromTheme("TextDocument"));

    // clang-format off
    // update editor actions on request
    Gui::MainWindow* mw = Gui::getMainWindow();
    connect(editor, &QPlainTextEdit::undoAvailable, mw, &MainWindow::updateEditorActions);
    connect(editor, &QPlainTextEdit::redoAvailable, mw, &MainWindow::updateEditorActions);
    connect(editor, &QPlainTextEdit::copyAvailable, mw, &MainWindow::updateEditorActions);
    connect(editor, &QPlainTextEdit::textChanged, this, &TextDocumentEditorView::textChanged);
    // clang-format on
}

TextDocumentEditorView::~TextDocumentEditorView()
{
    textConnection.disconnect();
    labelConnection.disconnect();
}

void TextDocumentEditorView::showEvent(QShowEvent* event)
{
    Gui::MainWindow* mw = Gui::getMainWindow();
    mw->updateEditorActions();
    MDIView::showEvent(event);
}

void TextDocumentEditorView::closeEvent(QCloseEvent* event)
{
    MDIView::closeEvent(event);
    if (event->isAccepted()) {
        aboutToClose = true;
        Gui::MainWindow* mw = Gui::getMainWindow();
        mw->updateEditorActions();
    }
}

void TextDocumentEditorView::setupEditor()
{
    // clang-format off
    connect(getEditor()->document(), &QTextDocument::modificationChanged,
            this, &TextDocumentEditorView::setWindowModified);
    // clang-format on
    labelChanged();
    refresh();
}

void TextDocumentEditorView::setupConnection()
{
    // NOLINTBEGIN
    textConnection =
        textDocument->connectText(std::bind(&TextDocumentEditorView::sourceChanged, this));
    labelConnection =
        textDocument->connectLabel(std::bind(&TextDocumentEditorView::labelChanged, this));
    // NOLINTEND
}

void TextDocumentEditorView::sourceChanged()
{
    refresh();
}

void TextDocumentEditorView::textChanged()
{
    saveToObject();
}

void TextDocumentEditorView::labelChanged()
{
    setWindowTitle(QString::fromUtf8(textDocument->Label.getValue()) + QStringLiteral("[*]"));
}

void TextDocumentEditorView::refresh()
{
    QString text = QString::fromUtf8(textDocument->Text.getValue());
    getEditor()->setPlainText(text);
}

void TextDocumentEditorView::saveToObject()
{
    boost::signals2::shared_connection_block textBlock {textConnection};
    textDocument->Text.setValue(getEditor()->document()->toPlainText().toUtf8());
    textDocument->purgeTouched();
}

QStringList TextDocumentEditorView::undoActions() const
{
    QStringList undo;
    undo << tr("Edit text");
    return undo;
}

QStringList TextDocumentEditorView::redoActions() const
{
    QStringList redo;
    redo << tr("Edit text");
    return redo;
}

bool TextDocumentEditorView::onHasMsg(const char* msg) const
{
    // don't allow any actions if the editor is being closed
    if (aboutToClose) {
        return false;
    }

    if (strcmp(msg, "Save") == 0) {
        return true;
    }
    if (strcmp(msg, "Cut") == 0) {
        return (!getEditor()->isReadOnly() && getEditor()->textCursor().hasSelection());
    }
    if (strcmp(msg, "Copy") == 0) {
        return (getEditor()->textCursor().hasSelection());
    }
    if (strcmp(msg, "Paste") == 0) {
        if (getEditor()->isReadOnly()) {
            return false;
        }
        QClipboard* cb = QApplication::clipboard();
        QString text = cb->text();
        return !text.isEmpty();
    }
    if (strcmp(msg, "Undo") == 0) {
        return (getEditor()->document()->isUndoAvailable());
    }
    if (strcmp(msg, "Redo") == 0) {
        return (getEditor()->document()->isRedoAvailable());
    }
    return false;
}

bool TextDocumentEditorView::onMsg(const char* msg, const char** output)
{
    Q_UNUSED(output)

    // don't allow any actions if the editor is being closed
    if (aboutToClose) {
        return false;
    }

    if (strcmp(msg, "Save") == 0) {
        saveToObject();
        getGuiDocument()->save();
        return true;
    }
    if (strcmp(msg, "Cut") == 0) {
        getEditor()->cut();
        return true;
    }
    if (strcmp(msg, "Copy") == 0) {
        getEditor()->copy();
        return true;
    }
    if (strcmp(msg, "Paste") == 0) {
        getEditor()->paste();
        return true;
    }
    if (strcmp(msg, "Undo") == 0) {
        getEditor()->undo();
        return true;
    }
    if (strcmp(msg, "Redo") == 0) {
        getEditor()->redo();
        return true;
    }
    return false;
}

#include "moc_TextDocumentEditorView.cpp"
