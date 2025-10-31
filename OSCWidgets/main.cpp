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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "EosTimer.h"
#include "QtInclude.h"
#include "MainWindow.h"
#include "Utils.h"
#include "EosPlatform.h"

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  srand(static_cast<unsigned int>(time(0)));

  EosTimer::Init();

  EosPlatform *platform = EosPlatform::Create();
  if (platform)
  {
    std::string error;
    if (!platform->Initialize(error))
    {
      printf("platform initialization failed\n");
      delete platform;
      platform = 0;
    }
  }

  QApplication app(argc, argv);

  app.setDesktopSettingsAware(false);
  app.setQuitOnLastWindowClosed(false);
  app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
  app.setStyle(QStyleFactory::create("Fusion"));

  QPalette pal;
  pal.setColor(QPalette::Window, QColor(40, 40, 40));
  pal.setColor(QPalette::WindowText, TEXT_COLOR);
  pal.setColor(QPalette::Disabled, QPalette::WindowText, MUTED_COLOR);
  pal.setColor(QPalette::Base, QColor(60, 60, 60));
  pal.setColor(QPalette::AlternateBase, QColor(80, 80, 80));
  pal.setColor(QPalette::Button, QColor(60, 60, 60));
  pal.setColor(QPalette::Light, pal.color(QPalette::Button).lighter(20));
  pal.setColor(QPalette::Midlight, pal.color(QPalette::Button).lighter(10));
  pal.setColor(QPalette::Dark, pal.color(QPalette::Button).darker(20));
  pal.setColor(QPalette::Mid, pal.color(QPalette::Button).darker(10));
  pal.setColor(QPalette::Text, TEXT_COLOR);
  pal.setColor(QPalette::Disabled, QPalette::Text, MUTED_COLOR);
  pal.setColor(QPalette::Highlight, QColor(80, 80, 80));
  pal.setColor(QPalette::HighlightedText, QColor(255, 142, 51));
  pal.setColor(QPalette::ButtonText, TEXT_COLOR);
  pal.setColor(QPalette::Disabled, QPalette::ButtonText, MUTED_COLOR);
  app.setPalette(pal);

  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-Black.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-BlackItalic.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-Bold.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-BoldItalic.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-Italic.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-Light.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-LightItalic.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-Medium.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-MediumItalic.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-Regular.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-Thin.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-ThinItalic.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/RobotoCondensed-Bold.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/RobotoCondensed-BoldItalic.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/RobotoCondensed-Italic.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/RobotoCondensed-Light.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/RobotoCondensed-LightItalic.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/RobotoCondensed-Regular.ttf");

  QFont fnt("Roboto", 10);
  app.setFont(fnt);

  PixmapCache::Instantiate();

  MainWindow *mainWindow = new MainWindow(platform);
  mainWindow->show();
  int result = app.exec();
  delete mainWindow;

  PixmapCache::Shutdown();

  if (platform)
    delete platform;

  return result;
}

////////////////////////////////////////////////////////////////////////////////
