// Copyright (c) 2018 Electronic Theatre Controls, Inc., http://www.etcconnect.com
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

#pragma once
#ifndef TOYS_H
#define TOYS_H

#ifndef QT_INCLUDE_H
#include "QtInclude.h"
#endif

#ifndef EOS_TIMER_H
#include "EosTimer.h"
#endif

#ifndef TOY_H
#include "Toy.h"
#endif

#include <vector>

////////////////////////////////////////////////////////////////////////////////

class Toys : public QObject
{
  Q_OBJECT

public:
  typedef std::vector<Toy *> TOY_LIST;

  Toys(Toy::Client *pClient, QWidget *pParent);

  virtual void Clear();
  virtual Toy *AddToy(Toy::EnumToyType type);
  const TOY_LIST &GetList() const { return m_List; }
  virtual bool GetFramesEnabled() const { return m_FramesEnabled; }
  virtual void SetFramesEnabled(bool b);
  virtual bool GetTopMost() const { return m_TopMost; }
  virtual void SetTopMost(bool b);
  virtual void SnapToEdges();
  virtual int GetOpacity() const { return m_Opacity; }
  virtual void SetOpacity(int opacity);
  virtual void ClearLabels();
  virtual void Recv(char *data, size_t len);
  virtual bool Save(EosLog &log, const QString &path, QStringList &lines);
  virtual bool Load(EosLog &log, const QString &path, QStringList &lines, int &index);
  virtual void ActivateToy(size_t index);
  virtual void ActivateToys(Toy::EnumToyType toyType);
  virtual void ActivateAllToys(bool b);
  virtual bool HasVisibleToys() const;
  virtual void DeleteToy(size_t index);
  virtual void RefreshAdvancedSettings();
  virtual void Connected();
  virtual void Disconnected();

signals:
  void changed();
  void toggleMainWindow();

private slots:
  void onRecvWidgetsChanged();
  void onToyClosing(Toy *toy);
  void onToyChanged();
  void onToyToggledMainWindow();

protected:
  Toy::Client *m_pClient;
  QWidget *m_pParent;
  TOY_LIST m_List;
  bool m_FramesEnabled;
  bool m_TopMost;
  int m_Opacity;
  Toy::RECV_WIDGETS m_RecvWidgets;
  Toy::RECV_WIDGETS m_WildcardRecvWidgets;
  bool m_Loading;

  virtual void BuildRecvWidgetsTable();
  virtual Qt::WindowFlags GetWindowFlags() const;
  virtual void UpdateWindowFlags();
};

////////////////////////////////////////////////////////////////////////////////

#endif
