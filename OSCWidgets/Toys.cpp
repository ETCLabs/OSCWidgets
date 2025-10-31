// Copyright (c) 2018 Electronic Theatre Controls, Inc.,
// http://www.etcconnect.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "Toys.h"

#include "EosLog.h"
#include "OSCParser.h"
#include "ToyWidget.h"
#include "Utils.h"

// TODO: restoring a maximized toy does not unmaximize to previous geometry

////////////////////////////////////////////////////////////////////////////////

Toys::Toys(Toy::Client *pClient, QWidget *pParent)
    : m_pClient(pClient),
      m_pParent(pParent),
      m_FramesEnabled(true),
      m_TopMost(false),
      m_Opacity(100),
      m_Loading(false) {}

////////////////////////////////////////////////////////////////////////////////

void Toys::Clear() {
  for (TOY_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    (*i)->deleteLater();
  m_List.clear();
}

////////////////////////////////////////////////////////////////////////////////

Toy *Toys::AddToy(Toy::EnumToyType type) {
  Toy *toy = Toy::Create(type, m_pClient, /*parent*/ 0, GetWindowFlags());
  if (toy) {
    QSize gridSize;
    toy->GetDefaultGridSize(gridSize);
    toy->SetGridSize(gridSize);

    connect(toy, SIGNAL(recvWidgetsChanged()), this,
            SLOT(onRecvWidgetsChanged()));
    connect(toy, SIGNAL(closing(Toy *)), this, SLOT(onToyClosing(Toy *)));
    connect(toy, SIGNAL(changed()), this, SLOT(onToyChanged()));
    connect(toy, SIGNAL(toggleMainWindow()), this,
            SLOT(onToyToggledMainWindow()));
    if (m_Opacity != 100) toy->setWindowOpacity(m_Opacity * 0.01);
    m_List.push_back(toy);

    if (!m_Loading) {
      toy->showNormal();
      BuildRecvWidgetsTable();
      toy->raise();
    }

    emit changed();
  }

  return toy;
}

////////////////////////////////////////////////////////////////////////////////

void Toys::SetFramesEnabled(bool b) {
  if (m_FramesEnabled != b) {
    m_FramesEnabled = b;
    UpdateWindowFlags();
  }
}

////////////////////////////////////////////////////////////////////////////////

void Toys::SetTopMost(bool b) {
  if (m_TopMost != b) {
    m_TopMost = b;
    UpdateWindowFlags();
  }
}

////////////////////////////////////////////////////////////////////////////////

void Toys::SnapToEdges() {
  for (TOY_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    (*i)->SnapToEdges();
}

////////////////////////////////////////////////////////////////////////////////

Qt::WindowFlags Toys::GetWindowFlags() const {
  Qt::WindowFlags flags = Qt::Window;

  if (m_TopMost) flags |= Qt::WindowStaysOnTopHint;

  if (!m_FramesEnabled) flags |= Qt::FramelessWindowHint;

  return flags;
}

////////////////////////////////////////////////////////////////////////////////

void Toys::UpdateWindowFlags() {
  for (TOY_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++) {
    Toy *toy = *i;
    QPoint pos(toy->frameGeometry().topLeft());
    Qt::WindowStates ws = toy->windowState();
    bool wasVisible = toy->isVisible();

    toy->hide();

    toy->setWindowFlags(GetWindowFlags());

    if (ws.testFlag(Qt::WindowMinimized)) {
      toy->showMinimized();
    } else if (ws.testFlag(Qt::WindowMaximized)) {
      toy->showMaximized();
      if (!wasVisible) toy->close();
    } else {
      toy->move(pos);
      toy->showNormal();
      Toy::ClipToScreen(*toy);
      if (!wasVisible) toy->close();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void Toys::SetOpacity(int opacity) {
  opacity = qBound(10, opacity, 100);
  if (m_Opacity != opacity) {
    m_Opacity = opacity;

    qreal t = (m_Opacity * 0.01);
    for (TOY_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
      (*i)->setWindowOpacity(t);
  }
}

////////////////////////////////////////////////////////////////////////////////

void Toys::ClearLabels() {
  for (TOY_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    (*i)->ClearLabels();
}

////////////////////////////////////////////////////////////////////////////////

void Toys::Recv(char *data, size_t len) {
  if (data && len != 0 &&
      (!m_RecvWidgets.empty() || !m_WildcardRecvWidgets.empty())) {
    QString recvPath;

    for (size_t i = 0; i < len; i++) {
      if (data[i] == 0) {
        if (i > 0) recvPath = QString::fromUtf8(data);
        break;
      }
    }

    if (!recvPath.isEmpty()) {
      size_t argCount = 0xffffffff;
      OSCArgument *args = OSCArgument::GetArgs(data, len, argCount);

      for (Toy::RECV_WIDGETS_RANGE range = m_RecvWidgets.equal_range(recvPath);
           range.first != range.second; range.first++) {
        ToyWidget *w = range.first->second;
        w->Recv(recvPath, args, argCount);
      }

      if (!m_WildcardRecvWidgets.empty()) {
#if 0
				QRegExp rx;
				rx.setPatternSyntax(QRegExp::Wildcard);
				rx.setCaseSensitivity(Qt::CaseSensitive);

				// special-case for wildcard matches.  If no arguments, use last OSC address as a string argument				
				OSCArgument pathArg;
				QByteArray pathArgStr;

				if(args==0 || argCount==0)
				{
					int index = recvPath.lastIndexOf(OSC_ADDR_SEPARATOR);
					if(index >= 0)
					{
						QString str = recvPath.mid(index+1);
						if( !str.isEmpty() )
						{
							pathArgStr = str.toUtf8();
							pathArg.Init(OSCArgument::OSC_TYPE_STRING, pathArgStr.data(), pathArgStr.size()+1);
						}
					}
				}

				for(Toy::RECV_WIDGETS::const_iterator i=m_WildcardRecvWidgets.begin(); i!=m_WildcardRecvWidgets.end(); ++i)
				{
					const QString &path = i->first;
					rx.setPattern(path);
					if(rx.exactMatch(recvPath))
					{
						ToyWidget *w = i->second;
						if(args==0 || argCount==0)
							w->Recv(recvPath, &pathArg, 1);
						else
							w->Recv(recvPath, args, argCount);
					}
				}
#endif
      }

      if (args) delete[] args;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void Toys::BuildRecvWidgetsTable() {
  m_RecvWidgets.clear();
  m_WildcardRecvWidgets.clear();

  for (TOY_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    (*i)->AddRecvWidgets(m_RecvWidgets);

  for (Toy::RECV_WIDGETS::iterator i = m_RecvWidgets.begin();
       i != m_RecvWidgets.end();) {
    const QString &path = i->first;
    if (path.contains('*')) {
      ToyWidget *toy = i->second;
      m_WildcardRecvWidgets.insert(Toy::RECV_WIDGETS_PAIR(path, toy));
      m_RecvWidgets.erase(i++);
    } else
      i++;
  }
}

////////////////////////////////////////////////////////////////////////////////

bool Toys::Save(EosLog &log, const QString &path, QStringList &lines) {
  for (TOY_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    (*i)->Save(log, path, lines);

  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool Toys::Load(EosLog &log, const QString &path, QStringList &lines,
                int &index) {
  Clear();

  while (index >= 0 && index < lines.size()) {
    m_Loading = true;

    QStringList items;
    Utils::GetItemsFromQuotedString(lines[index], items);

    if (items.size() > 0) {
      bool ok = false;
      int n = items[0].toInt(&ok);
      if (ok && n >= 0 && n < Toy::TOY_COUNT) {
        Toy *toy = AddToy(static_cast<Toy::EnumToyType>(n));
        if (toy)
          toy->Load(log, path, lines, index);
        else
          index++;
      } else
        index++;
    } else
      index++;

    m_Loading = false;
  }

  BuildRecvWidgetsTable();

  return true;
}

////////////////////////////////////////////////////////////////////////////////

void Toys::ActivateToy(size_t index) {
  if (index < m_List.size()) {
    Toy *toy = m_List[index];

    Qt::WindowStates ws = toy->windowState();

    if (ws.testFlag(Qt::WindowMaximized))
      toy->showMaximized();
    else
      toy->showNormal();

    toy->activateWindow();
  }
}

////////////////////////////////////////////////////////////////////////////////

void Toys::ActivateToys(Toy::EnumToyType toyType) {
  for (size_t i = (m_List.size() - 1); i < m_List.size(); i--) {
    if (m_List[i]->GetType() == toyType) ActivateToy(i);
  }
}

////////////////////////////////////////////////////////////////////////////////

void Toys::ActivateAllToys(bool b) {
  if (b) {
    for (size_t i = (m_List.size() - 1); i < m_List.size(); i--) ActivateToy(i);
  } else {
    for (TOY_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
      (*i)->hide();
  }
}

////////////////////////////////////////////////////////////////////////////////

bool Toys::HasVisibleToys() const {
  for (TOY_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++) {
    if ((*i)->isVisible()) return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

void Toys::DeleteToy(size_t index) {
  if (index < m_List.size()) {
    Toy *toy = m_List[index];

    m_List.erase(m_List.begin() + index);

    toy->close();
    toy->deleteLater();

    BuildRecvWidgetsTable();

    emit changed();
  }
}

////////////////////////////////////////////////////////////////////////////////

void Toys::RefreshAdvancedSettings() {
  for (TOY_LIST::iterator i = m_List.begin(); i != m_List.end(); i++)
    (*i)->StartTimer();
}

////////////////////////////////////////////////////////////////////////////////

void Toys::Connected() {
  for (TOY_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    (*i)->Connected();
}

////////////////////////////////////////////////////////////////////////////////

void Toys::Disconnected() {
  for (TOY_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    (*i)->Disconnected();
}

////////////////////////////////////////////////////////////////////////////////

void Toys::onRecvWidgetsChanged() { BuildRecvWidgetsTable(); }

////////////////////////////////////////////////////////////////////////////////

void Toys::onToyClosing(Toy *toy) {
  for (size_t i = 0; i < m_List.size(); i++) {
    if (m_List[i] == toy) {
      DeleteToy(i);
      break;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void Toys::onToyChanged() { emit changed(); }

////////////////////////////////////////////////////////////////////////////////

void Toys::onToyToggledMainWindow() { emit toggleMainWindow(); }

////////////////////////////////////////////////////////////////////////////////
