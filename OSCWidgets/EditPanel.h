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
#ifndef EDIT_PANEL_H
#define EDIT_PANEL_H

#ifndef QT_INCLUDE_H
#include "QtInclude.h"
#endif

class FadeButton;

////////////////////////////////////////////////////////////////////////////////

class EditButton : public QPushButton
{
  Q_OBJECT

public:
  EditButton(QWidget *parent);

  virtual bool GetSelected() const { return m_Selected; }
  virtual void SetSelected(bool selected);

private slots:
  void onTick();

protected:
  bool m_Selected;
  int m_Alpha;
  qreal m_T;
  QTimer *m_Timer;

  virtual void paintEvent(QPaintEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class ButtonRow : public QWidget
{
public:
  ButtonRow(QWidget *parent);

  virtual QPushButton *AddButton();
  virtual QPushButton *AddButton(const QString &text);
  virtual FadeButton *AddFadeButton();

protected:
  virtual void AddWidget(QWidget *w);
};

////////////////////////////////////////////////////////////////////////////////

class EditPanel : public QWidget
{
  Q_OBJECT

public:
  class EditPanelClient
  {
  public:
    virtual void EditPanelClient_Deleted(EditPanel *editPanel) = 0;
  };

  EditPanel(EditPanelClient &client, QWidget *parent);
  virtual ~EditPanel();

  virtual int GetCols() const;
  virtual void SetCols(int cols);
  virtual int GetRows() const;
  virtual void SetRows(int rows);
  virtual void SetGridEnabled(bool b);
  virtual bool GetHidden() const;
  virtual void SetHidden(bool b);
  virtual void SetHiddenEnabled(bool b);
  virtual void GetText(QString &text) const;
  virtual void SetText(const QString &text);
  virtual void GetImagePath(QString &imagePath) const;
  virtual void SetImagePath(const QString &imagePath);
  virtual void GetImagePath2(QString &imagePath2) const;
  virtual void SetImagePath2(const QString &imagePath2);
  virtual void SetImagePath2Enabled(bool b);
  virtual void GetPath(QString &path) const;
  virtual void SetPath(const QString &path);
  virtual void SetPathEnabled(bool b);
  virtual void GetPath2(QString &path) const;
  virtual void SetPath2(const QString &path);
  virtual void SetPath2Enabled(bool b);
  virtual void GetLabelPath(QString &labelPath) const;
  virtual void SetLabelPath(const QString &labelPath);
  virtual void SetLabelPathEnabled(bool b);
  virtual void GetFeedbackPath(QString &feedbackPath) const;
  virtual void SetFeedbackPath(const QString &feedbackPath);
  virtual void SetFeedbackPathEnabled(bool b);
  virtual void GetTriggerPath(QString &triggerPath) const;
  virtual void SetTriggerPath(const QString &triggerPath);
  virtual void SetTriggerPathEnabled(bool b);
  virtual void GetColor(QColor &color) const;
  virtual void SetColor(const QColor &color);
  virtual void GetColor2(QColor &color2) const;
  virtual void SetColor2(const QColor &color2);
  virtual void SetColor2Enabled(bool b);
  virtual void GetTextColor(QColor &textColor) const;
  virtual void SetTextColor(const QColor &textColor);
  virtual void SetTextColorEnabled(bool b);
  virtual void GetTextColor2(QColor &textColor2) const;
  virtual void SetTextColor2(const QColor &textColor2);
  virtual void SetTextColor2Enabled(bool b);
  virtual void GetMin(QString &n) const;
  virtual void SetMin(const QString &n);
  virtual void GetMax(QString &n) const;
  virtual void SetMax(const QString &n);
  virtual void SetMinMaxEnabled(bool b);
  virtual void GetMin2(QString &n) const;
  virtual void SetMin2(const QString &n);
  virtual void GetMax2(QString &n) const;
  virtual void SetMax2(const QString &n);
  virtual void SetMinMax2Enabled(bool b);
  virtual void GetBPM(QString &n) const;
  virtual void SetBPM(const QString &n);
  virtual void SetBPMEnabled(bool b);
  virtual void SetHelpText(const QString &text);

signals:
  void edited();
  void done();

private slots:
  void onGridChanged(int value);
  void onEditingFinished();
  void onHiddenStateChanged(int state);
  void onPathTextChanged(const QString &text);
  void onPath2TextChanged(const QString &text);
  void onLocalStateChanged(int state);
  void onImagePathButtonClicked(bool checked);
  void onImagePath2ButtonClicked(bool checked);
  void onColorClicked(bool checked);
  void onColor2Clicked(bool checked);
  void onTextColorClicked(bool checked);
  void onTextColor2Clicked(bool checked);
  void onDoneClicked(bool checked);

protected:
  EditPanelClient *m_pClient;
  QLabel *m_GridLabel;
  QSpinBox *m_Cols;
  QSpinBox *m_Rows;
  QLineEdit *m_Text;
  FadeButton *m_ImagePathButton;
  QString m_ImagePath;
  FadeButton *m_ImagePath2Button;
  QString m_ImagePath2;
  QLabel *m_PathLabel;
  QLineEdit *m_Path;
  QLabel *m_Path2Label;
  QLineEdit *m_Path2;
  QCheckBox *m_Local;
  QLabel *m_LabelPathLabel;
  QLineEdit *m_LabelPath;
  QLabel *m_FeedbackPathLabel;
  QLineEdit *m_FeedbackPath;
  QLabel *m_TriggerPathLabel;
  QLineEdit *m_TriggerPath;
  QPushButton *m_Color;
  QPushButton *m_Color2;
  QPushButton *m_TextColor;
  QPushButton *m_TextColor2;
  QLabel *m_MinMaxLabel;
  QLineEdit *m_Min;
  QLineEdit *m_Max;
  QLabel *m_MinMax2Label;
  QLineEdit *m_Min2;
  QLineEdit *m_Max2;
  QLabel *m_BPMLabel;
  QLineEdit *m_BPM;
  QLabel *m_HiddenLabel;
  QCheckBox *m_Hidden;
  QLabel *m_Help;
  unsigned int m_IgnoreEdits;

  virtual void closeEvent(QCloseEvent *event);
  virtual void UpdateLocal(bool primaryPath);
  virtual void SetToolTips(const QString &text, QWidget *label, QWidget *widget);
  virtual bool GetImageFile(QString &path);
};

////////////////////////////////////////////////////////////////////////////////

#endif
