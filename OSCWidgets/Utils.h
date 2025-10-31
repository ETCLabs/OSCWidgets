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
#ifndef UTILS_H
#define UTILS_H

#ifndef QT_INCLUDE_H
#include "QtInclude.h"
#endif

#ifdef WIN32
#include <Windows.h>
#endif

#include <map>

////////////////////////////////////////////////////////////////////////////////

class Utils
{
public:
  static QString QuotedString(const QString &str);
  static bool IsLocalOSCPath(const QString &path);
  static bool MakeLocalOSCPath(bool b, QString &path);
  static void GetItemsFromQuotedString(const QString &str, QStringList &items);
  static void BlockFakeMouseEvents(bool b);
  static void RegisterTouchWidget(QWidget &widget);
  static bool IsBrightColor(const QColor &color);
  static void MakeContrastingColor(float percent, QColor &color);
  static void MakeBrighterColor(float percent, QColor &color);
  static void MakeDarkerColor(float percent, QColor &color);
  static void Snap(int snap, int &value);
  static void Snap(int snap, QPoint &point);

#ifdef WIN32
  typedef BOOL(WINAPI *FuncRegisterTouchWindow)(HWND hWnd, ULONG ulFlags);
  typedef BOOL(WINAPI *FuncUnregisterTouchWindow)(HWND hWnd);
  static void *sm_pFuncRegisterTouchWindow;
  static void *sm_pFuncUnregisterTouchWindow;
  static HHOOK sm_MouseMsgHook;
#endif
};

////////////////////////////////////////////////////////////////////////////////

class PixmapCache
{
public:
  struct sPixmapCacheItem
  {
    const QPixmap *pixmap;
    unsigned int refCount;
  };

  typedef std::map<QString, sPixmapCacheItem> PIXMAP_LIST;

  PixmapCache();
  virtual ~PixmapCache();

  virtual void Clear();
  virtual const QPixmap *Create(const QString &path);
  virtual const QPixmap *Get(const QString &path) const;
  virtual void Destroy(const QString &path);
  virtual const PIXMAP_LIST &GetList() const { return m_List; }
  virtual void Get(const QString &path, QPixmap &pixmap) const;
  virtual void GetScaledToFit(const QString &path, const QSize &size, QPixmap &pixmap) const;
  virtual void GetScaledToFill(const QString &path, const QSize &size, QPixmap &pixmap) const;

  static void Instantiate();
  static void Shutdown();
  static PixmapCache &Instance() { return *sm_Instance; }
  static bool FitRectInBounds(const QRect &bounds, Qt::Alignment alignment, QRect &r);

protected:
  PIXMAP_LIST m_List;

  static PixmapCache *sm_Instance;
};

////////////////////////////////////////////////////////////////////////////////

#define PMC PixmapCache::Instance()

////////////////////////////////////////////////////////////////////////////////

#endif
