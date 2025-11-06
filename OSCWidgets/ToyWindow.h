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
#ifndef TOY_WINDOW_H
#define TOY_WINDOW_H

#ifndef TOY_WIDGET_H
#include "ToyWidget.h"
#endif

#ifndef TOY_GRID_H
#include "ToyGrid.h"
#endif

#include <vector>

////////////////////////////////////////////////////////////////////////////////

class EditFrame : public QWidget
{
  Q_OBJECT

public:
  EditFrame(QWidget *parent);

  virtual void InitEditMode();
  virtual void ShutdownEditMode();
  virtual void Translate(const QPoint &delta, bool snap);
  virtual void SetPos(const QPoint &pos);
  virtual bool GetSelected() const { return m_Selected; }
  virtual void SetSelected(bool b);
  virtual void ClipPos(QPoint &pos) const;

signals:
  void pressed(EditFrame *editFrame, bool clearPrevSelection);
  void translated(EditFrame *editFrame, const QPoint &delta);
  void grabbed(EditFrame *editFrame);
  void gridResized(EditFrame *editFrame, const QSize &gridSize);
  void raised(EditFrame *editFrame);
  void lowered(EditFrame *editFrame);
  void deleted(EditFrame *editFrame);

private slots:
  void onGridResized(size_t Id, const QSize &gridSize);
  void onRaise();
  void onLower();
  void onDelete();

protected:
  enum EnumGeomMode
  {
    GEOM_MODE_MOVE,
    GEOM_MODE_TOP,
    GEOM_MODE_BOTTOM,
    GEOM_MODE_LEFT,
    GEOM_MODE_RIGHT,
    GEOM_MODE_SCALE
  };

  enum EnumConstants
  {
    HANDLE_SIZE = 8,
    HANDLE_MARGIN = (HANDLE_SIZE / 2)
  };

  EnumGeomMode m_GeomMode;
  bool m_MouseDown;
  QPoint m_MouseGrabOffset;
  QRect m_MouseGrabGeometry;
  bool m_Selected;

  virtual void contextMenuEvent(QContextMenuEvent *event);
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);
  virtual void paintEvent(QPaintEvent *event);
  virtual EnumGeomMode GetGeomModeForPos(const QPoint &pos) const;
  virtual void SetGeomMode(EnumGeomMode geomMode);
  virtual void SetMouseDown(bool b);
  virtual void UpdateCursor();
};

////////////////////////////////////////////////////////////////////////////////

class ToyWindowTab : public QWidget
{
  Q_OBJECT

public:
  struct sFrame
  {
    sFrame()
      : toy(0)
      , editFrame(0)
    {
    }
    Toy *toy;
    EditFrame *editFrame;
  };

  typedef std::vector<sFrame> FRAME_LIST;

  enum EnumConstants
  {
    GRID_SPACING = 10
  };

  ToyWindowTab(QWidget *parent);

  virtual void SetShowGrid(bool b);
  virtual void SetMode(ToyWidget::EnumMode mode);
  virtual Toy *AddToy(Toy::EnumToyType type, const QSize &gridSize, const QPoint &pos, bool clipToBounds, Toy::Client *pClient);
  virtual bool RemoveToy(Toy *toy);
  virtual const FRAME_LIST &GetFrames() const { return m_Frames; }
  virtual void ClearLabels();
  virtual void AddRecvWidgets(Toy::RECV_WIDGETS &recvWidgets) const;
  virtual void ClearSelection();
  virtual void SelectAll();
  virtual void SetToySelected(Toy *toy, bool b);
  virtual void TranslateSelection(const QPoint &delta, bool snap);

signals:
  void closing(Toy *toy);

private slots:
  void onEditFramePressed(EditFrame *editFrame, bool clearPrevSelection);
  void onEditFrameTranslated(EditFrame *editFrame, const QPoint &delta);
  void onEditFrameGrabbed(EditFrame *editFrame);
  void onEditFrameGridResized(EditFrame *editFrame, const QSize &gridSize);
  void onEditFrameRaised(EditFrame *editFrame);
  void onEditFrameLowered(EditFrame *editFrame);
  void onEditFrameDeleted(EditFrame *editFrame);

protected:
  typedef std::vector<size_t> FRAME_INDICIES;

  ToyWidget::EnumMode m_Mode;
  FRAME_LIST m_Frames;
  bool m_ShowGrid;
  QRect m_GrabbedRect;
  bool m_MouseDown;
  QPoint m_MousePos;
  QRect m_MouseRect;
  FRAME_INDICIES m_MousePrevUnselected;

  virtual void SetMouseDown(bool b);
  virtual size_t GetFrameIndex(EditFrame *editFrame) const;
  virtual void ClipFrameToBounds(size_t index);
  virtual void UpdateMode();
  virtual void SetGrabbedRect(const QRect &r);
  virtual void SetMouseRect(const QRect &r);
  virtual void HandleTranslateKey(QKeyEvent *event);
  virtual void paintEvent(QPaintEvent *event);
  virtual void keyPressEvent(QKeyEvent *event);
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class TabButton : public FadeButton
{
  Q_OBJECT

public:
  TabButton(size_t index, QWidget *parent);

signals:
  void tabSelected(size_t index);
  void tabChanged(size_t index);

private slots:
  void onClicked(bool checked);
  void onRename();
  void onRenameFinished();
  void onRenameTextChanged(const QString &text);

protected:
  size_t m_Index;
  QLineEdit *m_Rename;
  QColor m_ButtonTextColor;

  virtual void AutoSizeFont() {}
  virtual void UpdateLayout();
  virtual void RenderBackground(QPainter &painter, QRectF &r);
  virtual void resizeEvent(QResizeEvent *event);
  virtual void keyPressEvent(QKeyEvent *event);
  virtual void contextMenuEvent(QContextMenuEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class TabBar : public QWidget
{
public:
  TabBar(QWidget *parent);

protected:
  virtual void paintEvent(QPaintEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class ToyWindow : public ToyGrid
{
  Q_OBJECT

public:
  ToyWindow(Client *pClient, QWidget *parent, Qt::WindowFlags flags);

  virtual void SetGridSize(const QSize &gridSize);
  virtual void GetDefaultGridSize(QSize &gridSize) const { gridSize = QSize(2, 1); }
  virtual void SetTabIndex(size_t index);
  virtual Toy *AddToy(EnumToyType type, const QSize &gridSize, const QPoint &pos);
  virtual void SetShowGrid(bool b);
  virtual bool HasColor2() const { return true; }
  virtual const QColor &GetColor2() const { return m_Color2; }
  virtual void SetColor2(const QColor &color2);
  virtual bool HasTextColor() const { return true; }
  virtual const QColor &GetTextColor() const { return m_TextColor; }
  virtual void SetTextColor(const QColor &textColor);
  virtual bool Save(EosLog &log, const QString &path, QStringList &lines);
  virtual bool Load(EosLog &log, const QString &path, QStringList &lines, int &index);
  virtual void ClearLabels();
  virtual void AddRecvWidgets(RECV_WIDGETS &recvWidgets) const;

  static int GetWidgetZOrder(QWidget &w);

private slots:
  void onRecvWidgetsChanged();
  void onToyClosing(Toy *toy);
  void onToyChanged();
  void onToyToggledMainWindow();
  void onTabSelected(size_t index);
  void onTabChanged(size_t index);
  void onLayoutModeSelected();

protected:
  struct sTab
  {
    sTab()
      : button(0)
      , widget(0)
    {
    }
    TabButton *button;
    ToyWindowTab *widget;
  };

  typedef std::vector<sTab> TABS;

  typedef std::multimap<int, Toy *> TOYS_BY_ZORDER;
  typedef std::pair<int, Toy *> TOYS_BY_ZORDER_PAIR;

  TABS m_Tabs;
  size_t m_TabIndex;
  TabBar *m_TabBar;
  QColor m_Color2;
  QColor m_TextColor;

  virtual void UpdateMode();
  virtual void UpdateLayout();
  virtual void UpdateTabs();
  virtual Toy *AddToyToTab(size_t tabIndex, EnumToyType type, const QSize &gridSize, const QPoint &pos);
};

////////////////////////////////////////////////////////////////////////////////

#endif
