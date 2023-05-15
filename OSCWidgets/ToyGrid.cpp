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

#include "ToyGrid.h"
#include "Utils.h"

////////////////////////////////////////////////////////////////////////////////

GridSizeButton::GridSizeButton(int col, int row, QWidget *parent)
	: FadeButton_NoTouch(parent)
	, m_Col(col)
	, m_Row(row)
{
	connect(this, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
}

////////////////////////////////////////////////////////////////////////////////

void GridSizeButton::onClicked(bool /*checked*/)
{
	emit clickedGridSize(m_Col, m_Row);
}

////////////////////////////////////////////////////////////////////////////////

void GridSizeButton::StartHover()
{
	FadeButton_NoTouch::StartHover();
	emit hoveredGridSize(m_Col, m_Row);
}

////////////////////////////////////////////////////////////////////////////////

GridSizeMenu::GridSizeMenu(size_t Id, const QSize &gridSize, const QIcon &icon, const QString &title, QWidget* parent/* =0 */)
	: QMenu(title, parent)
	, m_Id(Id)
	, m_GridSize(gridSize)
{
	setIcon(icon);

	m_Label = new QLabel(" ", this);
	m_Label->setAlignment(Qt::AlignCenter);
	m_Label->resize( m_Label->sizeHint() );

	int y = 0;
	m_Label->setGeometry(0, 0, sizeHint().width(), m_Label->height());
	y += m_Label->height();

	m_Buttons = new GridSizeButton**[ gridSize.width() ];
	for(int col=0; col<gridSize.width(); col++)
		m_Buttons[col] = new GridSizeButton*[ gridSize.height() ];

	for(int row=0; row<gridSize.height(); row++)
	{
		for(int col=0; col<gridSize.width(); col++)
		{
			GridSizeButton *button = new GridSizeButton(col, row, this);
			connect(button, SIGNAL(hoveredGridSize(int,int)), this, SLOT(onHovered(int,int)));
			connect(button, SIGNAL(clickedGridSize(int,int)), this, SLOT(onClicked(int,int)));
			button->setGeometry(col*QUICK_GRID_BUTTON_SIZE, y + row*QUICK_GRID_BUTTON_SIZE, QUICK_GRID_BUTTON_SIZE, QUICK_GRID_BUTTON_SIZE);
			m_Buttons[col][row] = button;
		}
	}

	SetHover(0, 0);

	resize( sizeHint() );
}

////////////////////////////////////////////////////////////////////////////////

void GridSizeMenu::SetHover(int hoverCol, int hoverRow)
{
	if(hoverCol < 0)
		hoverCol = 0;

	if(hoverRow < 0)
		hoverRow = 0;

	QPalette pal( m_Buttons[0][0]->palette() );
	for(int row=0; row<m_GridSize.height(); row++)
	{
		for(int col=0; col<m_GridSize.width(); col++)
		{
			bool highlight = (col<=hoverCol && row<=hoverRow);
			pal.setColor(QPalette::Button, highlight ? QColor(0,85,127) : QColor(60,60,60));

			GridSizeButton *button = m_Buttons[col][row];
			button->setPalette(pal);

			if(highlight &&	(col==hoverCol || row==hoverRow))
				button->Flash();
		}
	}

	m_Label->setText( QString("%1 x %2").arg(hoverCol+1).arg(hoverRow+1) );
}

////////////////////////////////////////////////////////////////////////////////

QSize GridSizeMenu::sizeHint() const
{
	return QSize(m_GridSize.width()*QUICK_GRID_BUTTON_SIZE, m_Label->height() + m_GridSize.height()*QUICK_GRID_BUTTON_SIZE);
}

////////////////////////////////////////////////////////////////////////////////

void GridSizeMenu::onHovered(int col, int row)
{
	SetHover(col, row);
}

////////////////////////////////////////////////////////////////////////////////

void GridSizeMenu::onClicked(int col, int row)
{
	emit gridResized(m_Id, QSize(col+1,row+1));
	emit triggered(nullptr);
}

////////////////////////////////////////////////////////////////////////////////

ToyGrid::ToyGrid(EnumToyType type, Client *pClient, QWidget *parent, Qt::WindowFlags flags)
	: Toy(type, pClient, parent, flags)
	, m_Mode(ToyWidget::MODE_DEFAULT)
	, m_GridSize(0, 0)
	, m_EditPanel(nullptr)
	, m_IgnoreEdits(0)
	, m_SendOnConnect(false)
	, m_pContextMenu(nullptr)
	, m_Loading(false)
{
	QString name;
	Toy::GetName(m_Type, name);
	SetText(name);
	
	SetColor( palette().color(QPalette::Window) );
	UpdateImagePath();
}

////////////////////////////////////////////////////////////////////////////////

ToyGrid::~ToyGrid()
{
	Clear();
}

////////////////////////////////////////////////////////////////////////////////

 void ToyGrid::Clear()
 {
	SetGridSize( QSize(0,0) );
 }

 ////////////////////////////////////////////////////////////////////////////////

void ToyGrid::SetMode(ToyWidget::EnumMode mode)
{
	if(m_Mode != mode)
	{
		m_Mode = mode;
		UpdateMode();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::UpdateMode()
{
	for(WIDGET_LIST::const_iterator i=m_List.begin(); i!=m_List.end(); i++)
		(*i)->SetMode(m_Mode);
	
	switch( m_Mode )
	{
		case ToyWidget::MODE_DEFAULT:
			CloseEditPanel();
			break;
			
		case ToyWidget::MODE_EDIT:
			{
				CreateEditPanel();

				if( m_EditPanel )
				{
					QWidget *pParent = parentWidget();
					if( pParent )
						m_EditPanel->move( mapToGlobal(QPoint(width(),0)) );
					else
						m_EditPanel->move(frameGeometry().topRight() + QPoint(1,0));
					m_EditPanel->show();
					ClipToScreen( *m_EditPanel );
				}
			}
			break;
	}

	UpdateText();
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::SetGridSize(const QSize &gridSize)
{
	QSize gs(gridSize);
	if(gs.width() < 1)
		gs.setWidth(1);
	if(gs.height() < 1)
		gs.setHeight(1);
	
	if(m_GridSize != gs)
	{
		QSize widgetSize(0, 0);
		if( !m_List.empty() )
			widgetSize = m_List.front()->size();
		
		m_GridSize = gs;
		
		size_t numWidgets = static_cast<size_t>(m_GridSize.width() * m_GridSize.height());
		
		// remove excess
		while(m_List.size() > numWidgets)
		{
			m_List.back()->deleteLater();
			m_List.pop_back();
		}
		
		// add new
		while(m_List.size() < numWidgets)
		{
			ToyWidget *widget = CreateWidget();
			if( widget )
			{
				m_List.push_back(widget);
				ApplyDefaultSettings(widget, m_List.size());
				widget->SetMode(m_Mode);
				
				if( !m_Loading )
					widget->show();
				
				connect(widget, SIGNAL(edit(ToyWidget*)), this, SLOT(onWidgetEdited(ToyWidget*)));
			}
			else
				break;
		}

		setMinimumSize(m_GridSize.width()*24, m_GridSize.height()*24);
		
		m_EditWidgetIndex = m_List.size();
		
		if( !m_Loading )
		{
			if( widgetSize.isEmpty() )
				widgetSize = GetDefaultWidgetSize();
			AutoSize(widgetSize);
			UpdateLayout();
			emit recvWidgetsChanged();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::AutoSize(const QSize &widgetSize)
{
	resize(	contentsMargins().left() + m_GridSize.width()*widgetSize.width() + contentsMargins().right(),
			contentsMargins().top() + m_GridSize.height()*widgetSize.height() + contentsMargins().bottom() );
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::ApplyDefaultSettings(ToyWidget *widget, size_t index)
{
	widget->SetText( QString::number(index) );

	QString pathName;
	Toy::GetDefaultPathName(m_Type, pathName);

	QString path = QString("/%1/%2").arg(pathName).arg(index);
	widget->SetPath(path);
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::SetText(const QString &text)
{
	if(m_Text != text)
	{
		m_Text = text;
		UpdateText();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::UpdateText()
{
	QString title(m_Text);
	if(m_Mode == ToyWidget::MODE_EDIT)
		title.append( tr(" :: EDIT") );
	setWindowTitle(title);
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::SetImagePath(const QString &imagePath)
{
	if(m_ImagePath != imagePath)
	{
		m_ImagePath = imagePath;
		UpdateImagePath();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::UpdateImagePath()
{
	if(parentWidget() == 0)
	{
		QIcon icon(m_ImagePath);
		if( icon.isNull() )
			SetDefaultWindowIcon( *this );
		else
			setWindowIcon(icon);
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::GetName(QString &name) const
{
	if( m_Text.isEmpty() )
		Toy::GetName(m_Type, name);
	else
		name = m_Text;
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::SetColor(const QColor &color)
{
	if(m_Color != color)
	{
		m_Color = color;
		UpdateColor();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::UpdateColor()
{
	QPalette pal( palette() );
	pal.setColor(QPalette::Window, m_Color);
	setPalette(pal);
	setAutoFillBackground(true);
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::SetColor2(const QColor& /*color2*/)
{
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::SetTextColor(const QColor& /*textColor*/)
{
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::UpdateLayout()
{
	UpdateLayoutForRect( rect() );
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::UpdateLayoutForRect(const QRect &r)
{
	if( !m_GridSize.isEmpty() )
	{
		int w = (r.width() - contentsMargins().left() - contentsMargins().right())/m_GridSize.width();
		int h = (r.height() - contentsMargins().top() - contentsMargins().bottom())/m_GridSize.height();
		int x = (r.x() + contentsMargins().left());
		int y = (r.y() + contentsMargins().top());
		int col = 0;
		for(WIDGET_LIST::const_iterator i=m_List.begin(); i!=m_List.end(); i++)
		{
			(*i)->setGeometry(x, y, w, h);
			if(++col >= m_GridSize.width())
			{
				col = 0;
				x = (r.x() + contentsMargins().left());
				y += h;
			}
			else
				x += w;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::EditWidget(ToyWidget *widget, bool toggle)
{
	if( !m_EditPanel )
		return;

	++m_IgnoreEdits;

	for(WIDGET_LIST::const_iterator i=m_List.begin(); i!=m_List.end(); i++)
	{
		ToyWidget *w = *i;
		if(w == widget)
		{
			w->SetSelected(!toggle || !w->GetSelected());
		}
		else
			w->SetSelected(false);
	}
	
	m_EditPanel->SetCols( m_GridSize.width() );
	m_EditPanel->SetRows( m_GridSize.height() );
	
	if(widget && widget->GetSelected())
	{
		m_EditPanel->SetGridEnabled(false);
		m_EditPanel->SetText( widget->GetText() );
		m_EditPanel->SetImagePath( widget->GetImagePath() );
		if( widget->HasImagePath2() )
		{
			m_EditPanel->SetImagePath2( widget->GetImagePath2() );
			m_EditPanel->SetImagePath2Enabled(true);
		}
		else
		{
			m_EditPanel->SetImagePath2( QString() );
			m_EditPanel->SetImagePath2Enabled(false);
		}
		if( widget->HasPath() )
		{
			m_EditPanel->SetPath( widget->GetPath() );
			m_EditPanel->SetPathEnabled(true);
		}
		else
		{
			m_EditPanel->SetPath( QString() );
			m_EditPanel->SetPathEnabled(false);
		}
		if( widget->HasPath2() )
		{
			m_EditPanel->SetPath2( widget->GetPath2() );
			m_EditPanel->SetPath2Enabled(true);
		}
		else
		{
			m_EditPanel->SetPath2( QString() );
			m_EditPanel->SetPath2Enabled(false);
		}
		m_EditPanel->SetLabelPath( widget->GetLabelPath() );
		m_EditPanel->SetLabelPathEnabled(true);
		if( widget->HasFeedbackPath() )
		{
			m_EditPanel->SetFeedbackPath( widget->GetFeedbackPath() );
			m_EditPanel->SetFeedbackPathEnabled(true);
		}
		else
		{
			m_EditPanel->SetFeedbackPath( QString() );
			m_EditPanel->SetFeedbackPathEnabled(false);
		}
		if( widget->HasTriggerPath() )
		{
			m_EditPanel->SetTriggerPath( widget->GetTriggerPath() );
			m_EditPanel->SetTriggerPathEnabled(true);
		}
		else
		{
			m_EditPanel->SetTriggerPath( QString() );
			m_EditPanel->SetTriggerPathEnabled(false);
		}
		if( widget->HasMinMax() )
		{
			m_EditPanel->SetMin( widget->GetMin() );
			m_EditPanel->SetMax( widget->GetMax() );
			m_EditPanel->SetMinMaxEnabled(true);
		}
		else
		{
			m_EditPanel->SetMin( QString() );
			m_EditPanel->SetMax( QString() );
			m_EditPanel->SetMinMaxEnabled(false);
		}
		if( widget->HasMinMax2() )
		{
			m_EditPanel->SetMin2( widget->GetMin2() );
			m_EditPanel->SetMax2( widget->GetMax2() );
			m_EditPanel->SetMinMax2Enabled(true);
		}
		else
		{
			m_EditPanel->SetMin2( QString() );
			m_EditPanel->SetMax2( QString() );
			m_EditPanel->SetMinMax2Enabled(false);
		}
		if( widget->HasBPM() )
		{
			m_EditPanel->SetBPM( widget->GetBPM() );
			m_EditPanel->SetBPMEnabled(true);
		}
		else
		{
			m_EditPanel->SetBPM( QString() );
			m_EditPanel->SetBPMEnabled(false);
		}
		if( widget->HasVisible() )
		{
			m_EditPanel->SetHidden( !widget->GetVisible() );
			m_EditPanel->SetHiddenEnabled(true);
		}
		else
		{
			m_EditPanel->SetHidden(false);
			m_EditPanel->SetHiddenEnabled(false);
		}
		m_EditPanel->SetColor( widget->GetColor() );
		if( widget->HasColor2() )
		{
			m_EditPanel->SetColor2( widget->GetColor2() );
			m_EditPanel->SetColor2Enabled(true);
		}
		else
			m_EditPanel->SetColor2Enabled(false);
		m_EditPanel->SetTextColor( widget->GetTextColor() );
		m_EditPanel->SetTextColorEnabled(true);
		if( widget->HasTextColor2() )
		{
			m_EditPanel->SetTextColor2( widget->GetTextColor2() );
			m_EditPanel->SetTextColor2Enabled(true);
		}
		else
			m_EditPanel->SetTextColor2Enabled(false);
		m_EditPanel->SetHelpText( widget->GetHelpText() );
	}
	else
	{
		m_EditPanel->SetGridEnabled(true);
		m_EditPanel->SetText(m_Text);
		m_EditPanel->SetImagePath(m_ImagePath);
		m_EditPanel->SetImagePath2( QString() );
		m_EditPanel->SetImagePath2Enabled(false);
		m_EditPanel->SetPath( QString() );
		m_EditPanel->SetPathEnabled(false);
		m_EditPanel->SetPath2Enabled(false);
		m_EditPanel->SetLabelPath( QString() );
		m_EditPanel->SetLabelPathEnabled(false);
		m_EditPanel->SetFeedbackPath( QString() );
		m_EditPanel->SetFeedbackPathEnabled(false);
		m_EditPanel->SetTriggerPath( QString() );
		m_EditPanel->SetTriggerPathEnabled(false);
		m_EditPanel->SetMin( QString() );
		m_EditPanel->SetMax( QString() );
		m_EditPanel->SetMinMaxEnabled(false);
		m_EditPanel->SetMin2( QString() );
		m_EditPanel->SetMax2( QString() );
		m_EditPanel->SetMinMax2Enabled(false);
		m_EditPanel->SetBPM( QString() );
		m_EditPanel->SetBPMEnabled(false);
		m_EditPanel->SetColor(m_Color);
		if( HasColor2() )
		{
			m_EditPanel->SetColor2( GetColor2() );
			m_EditPanel->SetColor2Enabled(true);
		}
		else
			m_EditPanel->SetColor2Enabled(false);
		if( HasTextColor() )
		{
			m_EditPanel->SetTextColor( GetTextColor() );
			m_EditPanel->SetTextColor2( GetTextColor() );
			m_EditPanel->SetTextColorEnabled(true);
		}
		else
		{
			m_EditPanel->SetTextColor( palette().color(QPalette::Disabled,QPalette::ButtonText) );
			m_EditPanel->SetTextColorEnabled(false);
		}
		m_EditPanel->SetTextColor2Enabled(false);
		m_EditPanel->SetHidden(false);
		m_EditPanel->SetHiddenEnabled(false);
		m_EditPanel->SetHelpText( QString() );
	}
	
	--m_IgnoreEdits;
}

////////////////////////////////////////////////////////////////////////////////

const ToyWidget* ToyGrid::ToyWidgetAt(const QPoint &pos) const
{
	size_t index = ToyWidgetIndexAt(pos);
	return ((index<m_List.size()) ? m_List[index] : 0);
}

////////////////////////////////////////////////////////////////////////////////

size_t ToyGrid::ToyWidgetIndexAt(const QPoint &pos) const
{
	for(size_t i=0; i<m_List.size(); i++)
	{
		if( m_List[i]->geometry().contains(pos) )
			return i;
	}
	
	return m_List.size();
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::AddRecvWidgets(RECV_WIDGETS &recvWidgets) const
{
	for(WIDGET_LIST::const_iterator i=m_List.begin(); i!=m_List.end(); i++)
	{
		ToyWidget *w = *i;
		if( !w->GetLabelPath().isEmpty() )
			recvWidgets.insert( RECV_WIDGETS_PAIR(w->GetLabelPath(),w) );
		if(w->HasFeedbackPath() && !w->GetFeedbackPath().isEmpty())
			recvWidgets.insert( RECV_WIDGETS_PAIR(w->GetFeedbackPath(),w) );
		if(w->HasTriggerPath() && !w->GetTriggerPath().isEmpty())
			recvWidgets.insert( RECV_WIDGETS_PAIR(w->GetTriggerPath(),w) );
	}
}

////////////////////////////////////////////////////////////////////////////////

bool ToyGrid::Save(EosLog &log, const QString &path, QStringList &lines)
{
	QRect r(frameGeometry().topLeft(), size());

	QString imagePath(m_ImagePath);
	Toy::ResourceAbsolutePathToRelative(&log, path, imagePath);

	QString line;

	line.append( QString("%1").arg(static_cast<int>(m_Type)) );
	line.append( QString(", %1").arg(r.x()) );
	line.append( QString(", %1").arg(r.y()) );
	line.append( QString(", %1").arg(r.width()) );
	line.append( QString(", %1").arg(r.height()) );
	line.append( QString(", %1").arg(static_cast<int>(windowState())) );
	line.append( QString(", %1").arg(static_cast<int>(isVisible() ? 1 : 0)) );
	line.append( QString(", %1").arg(m_GridSize.width()) );
	line.append( QString(", %1").arg(m_GridSize.height()) );
	line.append( QString(", %1").arg(Utils::QuotedString(m_Text)) );
	line.append( QString(", %1").arg(Utils::QuotedString(imagePath)) );
	line.append( QString(", %1").arg(m_Color.rgba(),0,16) );
	line.append( QString(", %1").arg(static_cast<int>(m_SendOnConnect ? 1 : 0)) );

	lines << line;

	for(WIDGET_LIST::const_iterator i=m_List.begin(); i!=m_List.end(); i++)
		(*i)->Save(log, path, lines);

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool ToyGrid::Load(EosLog &log, const QString &path, QStringList &lines, int &index)
{
	if(index>=0 && index<lines.size())
	{
		m_Loading = true;
		
		QStringList items;
		Utils::GetItemsFromQuotedString(lines[index++], items);

		QSize gridSize(0, 0);
		if(items.size() > 7)
		{
			gridSize.setWidth( items[7].toInt() );
			gridSize.setHeight( items[8].toInt() );
			SetGridSize(gridSize);
			setGeometry(items[1].toInt(), items[2].toInt(), items[3].toInt(), items[4].toInt());
			
			if(items.size() > 9)
				SetText( items[9] );
			
			if(items.size() > 10)
			{
				QString imagePath( items[10] );
				Toy::ResourceRelativePathToAbsolute(&log, path, imagePath);
				SetImagePath(imagePath);
			}
			
			if(items.size() > 11)
				SetColor( QColor::fromRgba(items[11].toUInt(0,16)) );
			
			if(items.size() > 12)
				SetSendOnConnect(items[12].toInt() != 0);
			
			int numToyWidgets = (gridSize.width() * gridSize.height());
			for(int i=0; i<numToyWidgets && index<lines.size(); i++)
			{
				size_t widgetIndex = static_cast<size_t>(i);
				if(widgetIndex < m_List.size())
					m_List[widgetIndex]->Load(log, path, lines, index);
			}
			
			if(parentWidget() == 0)
			{
//				Qt::WindowStates ws = static_cast<Qt::WindowStates>( items[5].toInt() );
				bool windowVisible = (items[6].toInt() != 0);
//				if( ws.testFlag(Qt::WindowMinimized) )
//				{
//					if( ws.testFlag(Qt::WindowMaximized) )
//						showMaximized();
//					showMinimized();
//					windowVisible = false;
//				}
//				else if( ws.testFlag(Qt::WindowMaximized) )
//				{
//					showMaximized();
//					if( !windowVisible )
//						close();
//				}
//				else
				{
					ClipToScreen( *this );
					if( windowVisible )
					{
						showNormal();
						raise();
					}
					else
						close();
				}
			}
			else
				show();
		}
		
		m_Loading = false;
		
		UpdateLayout();
		
		emit recvWidgetsChanged();
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::ClearLabels()
{
	for(WIDGET_LIST::const_iterator i=m_List.begin(); i!=m_List.end(); i++)
		(*i)->ClearLabel();
}

////////////////////////////////////////////////////////////////////////////////

Toy* ToyGrid::AddToy(EnumToyType /*type*/, const QSize& /*gridSize*/, const QPoint& /*pos*/)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::GetDefaultGridSize(QSize &gridSize) const
{
	gridSize = QSize(1, 1);
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::CreateEditPanel()
{
	if( !m_EditPanel )
	{
		m_EditPanel = new EditPanel(*this, this);
		connect(m_EditPanel, SIGNAL(edited()), this, SLOT(onEdited()));
		connect(m_EditPanel, SIGNAL(done()), this, SLOT(onDone()));
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::CloseEditPanel()
{
	if( m_EditPanel )
	{
		m_EditPanel->close();
		m_EditPanel = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool ToyGrid::ConfirmGridResize(QWidget *parent, bool tab, const QSize &beforeSize, const QSize &afterSize)
{
	if(beforeSize != afterSize)
	{
		// confirmation prompt if shrinking
		if(afterSize.width()<beforeSize.width() || afterSize.height()<beforeSize.height())
		{
			QString text;
			
			if(	tab )
			{
				text = tr("Reduce tabs from %1 to %2\n\nAre you sure?")
				.arg( beforeSize.width() )
				.arg( afterSize.width() );
			}
			else
			{
				text = tr("Resize grid from (%1 x %2) to (%3 x %4)\n\nAre you sure?")
				.arg( beforeSize.width() )
				.arg( beforeSize.height() )
				.arg( afterSize.width() )
				.arg( afterSize.height() );
			}
			
			QMessageBox mb(QMessageBox::NoIcon, tr("OSCWidgets"), text, QMessageBox::NoButton, parent);
			mb.setIconPixmap( QPixmap(":/assets/images/IconQuestion.png") );
			QPushButton *yesButton = mb.addButton(tr("Yes"), QMessageBox::AcceptRole);
			mb.addButton(tr("No"), QMessageBox::DestructiveRole);
			mb.addButton(tr("Cancel"), QMessageBox::RejectRole);
			
			mb.exec();
			
			if(mb.clickedButton() == yesButton)
				return true;
		}
		else
		{
			// expanding, no confirmation required
			return true;
		}
	}
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::HandleGridResize(bool tab, const QSize &size)
{
	if( ConfirmGridResize(this,tab,m_GridSize,size) )
	{
		SetGridSize(size);
		emit changed();
	}
	
	if( m_pContextMenu )
		m_pContextMenu->close();
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::EditPanelClient_Deleted(EditPanel* /*editPanel*/)
{
	m_EditPanel = 0;
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::resizeEvent(QResizeEvent *event)
{
	Toy::resizeEvent(event);
	UpdateLayout();
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::contextMenuEvent(QContextMenuEvent *event)
{
	QString name;
	
	QMenu menu(this);
	
	bool hasLayoutMode = (parentWidget()!=0 || m_Type==TOY_WINDOW);
	
	if(m_Type == TOY_WINDOW)
	{
		QMenu *addMenu = menu.addMenu(QIcon(":/assets/images/MenuIconAdd.png"), tr("Add..."));
		if( addMenu )
		{
			QString str;
			for(size_t i=0; i<Toy::TOY_COUNT; i++)
			{
				if(i != TOY_WINDOW)
				{
					EnumToyType toyType = static_cast<Toy::EnumToyType>(i);
					Toy::GetName(toyType, str);
					GridSizeMenu *gridSizeMenu = new GridSizeMenu(i, QSize(QUICK_GRID_WIDTH,QUICK_GRID_HEIGHT), QIcon(), str);
					connect(gridSizeMenu, SIGNAL(gridResized(size_t,const QSize&)), this, SLOT(onToyAdded(size_t,const QSize&)));
					addMenu->addMenu(gridSizeMenu);
				}
			}
		}
		
		GridSizeMenu *gridSizeMenu = new GridSizeMenu(0, QSize(QUICK_GRID_TABS,1), QIcon(":/assets/images/MenuIconGrid.png"), tr("Tabs"));
		connect(gridSizeMenu, SIGNAL(gridResized(size_t,const QSize&)), this, SLOT(onTabResized(size_t,const QSize&)));
		menu.addMenu(gridSizeMenu);
	}
	
	if(m_Type != TOY_WINDOW)
	{
		m_EditWidgetIndex = ToyWidgetIndexAt( event->pos() );
		if(m_EditWidgetIndex < m_List.size())
		{
			const ToyWidget *toyWidget = m_List[m_EditWidgetIndex];
			name = toyWidget->GetText();
			if( name.isEmpty() )
				Toy::GetDefaultPathName(m_Type, name);
			menu.addAction(QIcon(":/assets/images/MenuIconEdit.png"), tr("Edit %1...").arg(name), this, SLOT(onEditToyWidget()));
		}

		GridSizeMenu *gridSizeMenu = new GridSizeMenu(0, QSize(QUICK_GRID_WIDTH,QUICK_GRID_HEIGHT), QIcon(":/assets/images/MenuIconGrid.png"), tr("Grid"));
		connect(gridSizeMenu, SIGNAL(gridResized(size_t,const QSize&)), this, SLOT(onGridResized(size_t,const QSize&)));
		menu.addMenu(gridSizeMenu);
	}

	GetName(name);
	
	if( hasLayoutMode )
		menu.addAction(QIcon(":/assets/images/MenuIconSettings.png"), tr("Layout Mode..."), this, SLOT(onLayoutMode()));
	else
		menu.addAction(QIcon(":/assets/images/MenuIconSettings.png"), tr("%1 Settings...").arg(name), this, SLOT(onEdit()));

	if(m_Mode == ToyWidget::MODE_EDIT)
		menu.addAction(QIcon(":/assets/images/MenuIconCheck.png"), tr("Done Editing"), this, SLOT(onDone()));

	menu.addAction(QIcon(":/assets/images/MenuIconRefresh.png"), tr("Clear OSC Labels"), this, SLOT(onClearLabels()));

	menu.addSeparator();
	menu.addAction(QIcon(":/assets/images/MenuIconTrash.png"), tr("Delete %1...").arg(name), this, SLOT(onDelete()));
	
	menu.addSeparator();
	menu.addAction(QIcon(":/assets/images/MenuIconHome.png"), tr("Toggle Main Window"), this, SLOT(onToggleMainWindow()));

	m_pContextMenu = &menu;
    menu.exec( event->globalPos() );
	m_pContextMenu = 0;
	event->accept();
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::closeEvent(QCloseEvent *event)
{
	Toy::closeEvent(event);
	CloseEditPanel();
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::onEdit()
{
	m_EditWidgetIndex = m_List.size();
	onEditToyWidget();
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::onEditToyWidget()
{
	SetMode(ToyWidget::MODE_EDIT);
	EditWidget((m_EditWidgetIndex<m_List.size()) ? m_List[m_EditWidgetIndex] : 0, /*toggle*/false);
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::onLayoutMode()
{
	emit layoutModeSelected();
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::onDelete()
{
	QString name;
	GetName(name);
	QMessageBox mb(QMessageBox::NoIcon, tr("Delete"), tr("Are you sure you want to delete %1").arg(name), QMessageBox::Yes|QMessageBox::Cancel, this);
	mb.setIconPixmap( QPixmap(":/assets/images/IconQuestion.png") );
	if(mb.exec() == QMessageBox::Yes)
	{
		CloseEditPanel();
		emit closing(this);
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::onToggleMainWindow()
{
	emit toggleMainWindow();
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::onWidgetEdited(ToyWidget *widget)
{
	EditWidget(widget, /*toggle*/true);
}

////////////////////////////////////////////////////////////////////////////////

 void ToyGrid::onEdited()
 {
	if(!m_EditPanel || m_IgnoreEdits!=0)
		return;
		
	ToyWidget *widget = 0;
	for(WIDGET_LIST::const_iterator i=m_List.begin(); i!=m_List.end(); i++)
	{
		if( (*i)->GetSelected() )
		{
			widget = *i;
			break;
		}
	}
	
	if( widget )
	{
		QString str;
		m_EditPanel->GetText(str);
		widget->SetText(str);

		m_EditPanel->GetImagePath(str);
		widget->SetImagePath(str);
		
		if( widget->HasImagePath2() )
		{
			m_EditPanel->GetImagePath2(str);
			widget->SetImagePath2(str);
		}
		
		if( widget->HasPath() )
		{
			m_EditPanel->GetPath(str);
			widget->SetPath(str);
		}
		
		if( widget->HasPath2() )
		{
			m_EditPanel->GetPath2(str);
			widget->SetPath2(str);
		}

		bool recvWidgetsDirty = false;
		m_EditPanel->GetLabelPath(str);
		if(widget->GetLabelPath() != str)
		{
			widget->SetLabelPath(str);
			recvWidgetsDirty = true;
		}

		if( widget->HasFeedbackPath() )
		{
			m_EditPanel->GetFeedbackPath(str);
			if(widget->GetFeedbackPath() != str)
			{
				widget->SetFeedbackPath(str);
				recvWidgetsDirty = true;
			}
		}

		if( widget->HasTriggerPath() )
		{
			m_EditPanel->GetTriggerPath(str);
			if(widget->GetTriggerPath() != str)
			{
				widget->SetTriggerPath(str);
				recvWidgetsDirty = true;
			}
		}
		
		m_EditPanel->GetMin(str);
		widget->SetMin(str);
		
		m_EditPanel->GetMax(str);
		widget->SetMax(str);

		if( widget->HasMinMax2() )
		{
			m_EditPanel->GetMin2(str);
			widget->SetMin2(str);
		
			m_EditPanel->GetMax2(str);
			widget->SetMax2(str);
		}

		if( widget->HasBPM() )
		{
			m_EditPanel->GetBPM(str);
			widget->SetBPM(str);
		}

		if( widget->HasVisible() )
			widget->SetVisible( !m_EditPanel->GetHidden() );
		
		QColor color;
		m_EditPanel->GetColor(color);
		widget->SetColor(color);
		
		if( widget->HasColor2() )
		{
			m_EditPanel->GetColor2(color);
			widget->SetColor2(color);
		}
		
		m_EditPanel->GetTextColor(color);
		widget->SetTextColor(color);
		
		if( widget->HasTextColor2() )
		{
			m_EditPanel->GetTextColor2(color);
			widget->SetTextColor2(color);
		}

		if( recvWidgetsDirty )
			emit recvWidgetsChanged();
	}
	else
	{
		QSize gridSize(m_EditPanel->GetCols(), m_EditPanel->GetRows());
		SetGridSize(gridSize);
	
		QString str;
		m_EditPanel->GetText(str);
		SetText(str);

		m_EditPanel->GetImagePath(str);
		SetImagePath(str);
		
		QColor color;
		m_EditPanel->GetColor(color);
		SetColor(color);
		
		if( HasColor2() )
		{
			m_EditPanel->GetColor2(color);
			SetColor2(color);
		}
		
		if( HasTextColor() )
		{
			m_EditPanel->GetTextColor(color);
			SetTextColor(color);
		}
	}

	emit changed();
 }

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::onDone()
{
	if(m_IgnoreEdits != 0)
		return;
	
	SetMode(ToyWidget::MODE_DEFAULT);
	activateWindow();
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::onGridResized(size_t /*Id*/, const QSize &size)
{
	HandleGridResize(/*tab*/false, size);
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::onTabResized(size_t /*Id*/, const QSize &size)
{
	HandleGridResize(/*tab*/true, size);
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::onToyAdded(size_t toyType, const QSize &gridSize)
{
	if(m_Mode != ToyWidget::MODE_EDIT)
	{
		SetMode(ToyWidget::MODE_EDIT);
		EditWidget(0, /*toggle*/false);
	}
	
	QPoint pos;
	
	if( m_pContextMenu )
		pos = m_pContextMenu->mapToGlobal( QPoint(0,0) );
	
	AddToy(static_cast<Toy::EnumToyType>(toyType), gridSize, pos);
	
	if( m_pContextMenu )
		m_pContextMenu->close();
}

////////////////////////////////////////////////////////////////////////////////

void ToyGrid::onClearLabels()
{
	ClearLabels();
}

////////////////////////////////////////////////////////////////////////////////
