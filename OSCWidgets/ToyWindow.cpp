#include "ToyWindow.h"
#include "Toys.h"
#include "Utils.h"

////////////////////////////////////////////////////////////////////////////////

#define TAB_SPACING	1

////////////////////////////////////////////////////////////////////////////////

EditFrame::EditFrame(QWidget *pWidget)
	: QWidget(pWidget)
	, m_GeomMode(GEOM_MODE_MOVE)
	, m_MouseDown(false)
	, m_Selected(false)
{
	setMouseTracking(true);
	UpdateCursor();
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::InitEditMode()
{
	QWidget *pParent = parentWidget();
	if( pParent )
		setGeometry(0, 0, pParent->width(), pParent->height());
	raise();
	show();
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::ShutdownEditMode()
{
	hide();
	SetGeomMode(GEOM_MODE_MOVE);
	SetMouseDown(false);
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::SetSelected(bool b)
{
	if(m_Selected != b)
	{
		m_Selected = b;
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

EditFrame::EnumGeomMode EditFrame::GetGeomModeForPos(const QPoint &pos) const
{
	int r = (width() - HANDLE_SIZE);
	int b = (height() - HANDLE_SIZE);
	
	if(pos.x()>=r && pos.y()>=b)
		return GEOM_MODE_SCALE;
	else if(pos.x() <= HANDLE_SIZE)
		return GEOM_MODE_LEFT;
	else if(pos.x() >= r)
		return GEOM_MODE_RIGHT;
	else if(pos.y() <= HANDLE_SIZE)
		return GEOM_MODE_TOP;
	else if(pos.y() >= b)
		return GEOM_MODE_BOTTOM;
	
	return GEOM_MODE_MOVE;
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::SetGeomMode(EnumGeomMode geomMode)
{
	if(m_GeomMode != geomMode)
	{
		m_GeomMode = geomMode;
		UpdateCursor();
	}
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::SetMouseDown(bool b)
{
	if(m_MouseDown != b)
	{
		m_MouseDown = b;
		UpdateCursor();
		emit grabbed(m_MouseDown ? this : 0);
	}
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::UpdateCursor()
{
	switch( m_GeomMode )
	{
		case GEOM_MODE_TOP:
		case GEOM_MODE_BOTTOM:
			setCursor(Qt::SizeVerCursor);
			break;
			
		case GEOM_MODE_LEFT:
		case GEOM_MODE_RIGHT:
			setCursor(Qt::SizeHorCursor);
			break;
			
		case GEOM_MODE_SCALE:
			setCursor(Qt::SizeFDiagCursor);
			break;
			
		default:
			setCursor(m_MouseDown ? Qt::ClosedHandCursor : Qt::OpenHandCursor);
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::Translate(const QPoint &delta, bool snap)
{
	QWidget *pParent = parentWidget();
	if( pParent )
	{
		QPoint newPos(pParent->pos() + delta);
		if( snap )
		{
			QPoint snappedPos(newPos);
			Utils::Snap(ToyWindowTab::GRID_SPACING, snappedPos);
			if(delta.x() != 0)
				newPos.setX( snappedPos.x() );
			if(delta.y() != 0)
				newPos.setY( snappedPos.y() );
		}
		
		SetPos(newPos);
	}
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::ClipPos(QPoint &pos) const
{
	QWidget *pParent = parentWidget();
	if( pParent )
	{
		QWidget *pGrandParent = pParent->parentWidget();
		if( pGrandParent )
		{
			int maxX = (pGrandParent->width() - pParent->width());
			if(pos.x() > maxX)
				pos.setX(maxX);
			if(pos.x() < 0)
				pos.setX(0);
			
			int maxY = (pGrandParent->height() - pParent->height());
			if(pos.y() > maxY)
				pos.setY(maxY);
			if(pos.y() < 0)
				pos.setY(0);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::SetPos(const QPoint &pos)
{
	QWidget *pParent = parentWidget();
	if( pParent )
	{
		QWidget *pGrandParent = pParent->parentWidget();
		if( pGrandParent )
		{
			QPoint newPos(pos);
			ClipPos(newPos);
			pParent->move( newPos );
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);
	
	menu.addAction(QIcon(":/assets/images/MenuIconUp.png"), tr("Bring to Top"), this, SLOT(onRaise()));
	menu.addAction(QIcon(":/assets/images/MenuIconDown.png"), tr("Send to Bottom"), this, SLOT(onLower()));
	
	GridSizeMenu *gridSizeMenu = new GridSizeMenu(0, QSize(QUICK_GRID_WIDTH,QUICK_GRID_HEIGHT), QIcon(":/assets/images/MenuIconGrid.png"), tr("Grid"));
	connect(gridSizeMenu, SIGNAL(gridResized(size_t,const QSize&)), this, SLOT(onGridResized(size_t,const QSize&)));
	menu.addSeparator();
	menu.addMenu(gridSizeMenu);
	
	menu.addSeparator();
	menu.addAction(QIcon(":/assets/images/MenuIconTrash.png"), tr("Delete"), this, SLOT(onDelete()));
	
	menu.exec( event->globalPos() );
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::onGridResized(size_t /*Id*/, const QSize &gridSize)
{
	emit gridResized(this, gridSize);
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::onRaise()
{
	emit raised(this);
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::onLower()
{
	emit lowered(this);
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::onDelete()
{
	emit deleted(this);
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::mousePressEvent(QMouseEvent *event)
{
	if( !event->buttons().testFlag(Qt::RightButton) )
	{
		m_MouseGrabGeometry = geometry();
		m_MouseGrabOffset = event->pos();
		SetGeomMode( GetGeomModeForPos(m_MouseGrabOffset) );
		SetMouseDown(true);
		bool clearPrevSelection = (	!event->modifiers().testFlag(Qt::ShiftModifier) &&
									!event->modifiers().testFlag(Qt::ControlModifier) );
		emit pressed(this, clearPrevSelection);
	}
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::mouseMoveEvent(QMouseEvent *event)
{
	if( m_MouseDown )
	{
		bool snap = !event->modifiers().testFlag(Qt::ShiftModifier);
		QWidget *pParent = parentWidget();
		QWidget *pGrandParent = (pParent ? pParent->parentWidget() : 0);
		if( pGrandParent )
		{
			switch( m_GeomMode )
			{
				case GEOM_MODE_TOP:
					{
						int dy = (m_MouseGrabGeometry.y() - m_MouseGrabOffset.y());
						int y = (pGrandParent->mapFromGlobal(event->globalPos()).y() + dy);
						if( snap )
							Utils::Snap(ToyWindowTab::GRID_SPACING, y);
						
						if(y < 0)
						{
							y = 0;
						}
						else
						{
							int maxY = (pParent->geometry().bottom() - pParent->minimumHeight());
							if(y > maxY)
								y = maxY;
						}
						
						int dh = (pParent->y() - y);
						pParent->move(pParent->x(), y);
						pParent->resize(pParent->width(), pParent->height() + dh);
						resize( pParent->size() );
						emit grabbed(this);
					}
					break;
					
				case GEOM_MODE_BOTTOM:
					{
						int dy = (m_MouseGrabGeometry.bottom() - m_MouseGrabOffset.y());
						int y = (pGrandParent->mapFromGlobal(event->globalPos()).y() + dy);
						if( snap )
							Utils::Snap(ToyWindowTab::GRID_SPACING, y);
						
						int newHeight = (y - pParent->y());
						if(newHeight < pParent->minimumHeight())
						{
							newHeight = pParent->minimumHeight();
						}
						else
						{
							int maxHeight = (pGrandParent->height() - pParent->y());
							if(newHeight > maxHeight)
								newHeight = maxHeight;
						}
						
						pParent->resize(pParent->width(), newHeight);
						resize( pParent->size() );
						emit grabbed(this);
					}
					break;
					
				case GEOM_MODE_LEFT:
					{
						int dx = (m_MouseGrabGeometry.x() - m_MouseGrabOffset.x());
						int x = (pGrandParent->mapFromGlobal(event->globalPos()).x() + dx);
						if( snap )
							Utils::Snap(ToyWindowTab::GRID_SPACING, x);
						
						if(x < 0)
						{
							x = 0;
						}
						else
						{
							int maxX = (pParent->geometry().right() - pParent->minimumWidth());
							if(x > maxX)
								x = maxX;
						}
						
						int dw = (pParent->x() - x);
						pParent->move(x, pParent->y());
						pParent->resize(pParent->width() + dw, pParent->height());
						resize( pParent->size() );
						emit grabbed(this);
					}
					break;
					
				case GEOM_MODE_RIGHT:
					{
						int dx = (m_MouseGrabGeometry.right() - m_MouseGrabOffset.x());
						int x = (pGrandParent->mapFromGlobal(event->globalPos()).x() + dx);
						if( snap )
							Utils::Snap(ToyWindowTab::GRID_SPACING, x);
						
						int newWidth = (x - pParent->x());
						if(newWidth < pParent->minimumWidth())
						{
							newWidth = pParent->minimumWidth();
						}
						else
						{
							int maxWidth = (pGrandParent->width() - pParent->x());
							if(newWidth > maxWidth)
								newWidth = maxWidth;
						}
						
						pParent->resize(newWidth, pParent->height());
						resize( pParent->size() );
						emit grabbed(this);
					}
					break;
					
				case GEOM_MODE_SCALE:
					{
						float w = m_MouseGrabGeometry.width();
						if(w != 0)
						{
							float ar = m_MouseGrabGeometry.height()/w;
							if(ar != 0)
							{
								// same behavior as GEOM_MODE_RIGHT
								int dx = (m_MouseGrabGeometry.right() - m_MouseGrabOffset.x());
								int x = (pGrandParent->mapFromGlobal(event->globalPos()).x() + dx);
								if( snap )
									Utils::Snap(ToyWindowTab::GRID_SPACING, x);
								
								int newWidth = (x - pParent->x());
								if(newWidth < pParent->minimumWidth())
								{
									newWidth = pParent->minimumWidth();
								}
								else
								{
									int maxWidth = (pGrandParent->width() - pParent->x());
									if(newWidth > maxWidth)
										newWidth = maxWidth;
								}
								
								// adjust height, preseving aspect ratio
								int newHeight = qRound(newWidth * ar);
								if(newHeight < pParent->minimumHeight())
								{
									newHeight = pParent->minimumHeight();
									newWidth = qRound(newHeight/ar);
								}
								else
								{
									int maxHeight = (pGrandParent->height() - pParent->y());
									if(newHeight > maxHeight)
									{
										newHeight = maxHeight;
										newWidth = qRound(newHeight/ar);
									}
								}
								
								pParent->resize(newWidth, newHeight);
								resize( pParent->size() );
								emit grabbed(this);
							}
						}
					}
					break;
					
				default:
					{
						QPoint newPos(pGrandParent->mapFromGlobal(event->globalPos()) - m_MouseGrabOffset);
						if( snap )
							Utils::Snap(ToyWindowTab::GRID_SPACING, newPos);

						QPoint delta = (newPos - pParent->pos());
						if(delta.x()!=0 || delta.y()!=0)
							emit translated(this, delta);
						
						emit grabbed(this);
					}
					break;
			}
		}
	}
	else
		SetGeomMode( GetGeomModeForPos(event->pos()) );
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::mouseReleaseEvent(QMouseEvent* /*event*/)
{
	SetMouseDown(false);
}

////////////////////////////////////////////////////////////////////////////////

void EditFrame::paintEvent(QPaintEvent* /*event*/)
{
	QRect r( rect() );
	r.adjust(0, 0, -1, -1);
	
	QPainter painter(this);
	
	QColor color(m_Selected ? QColor(0,200,60) : QColor(255,255,255));
	
	if( m_Selected )
	{
		QColor fillColor(color);
		fillColor.setAlpha(40);
		painter.fillRect(r, fillColor);
		
		QWidget *pParent = parentWidget();
		if( pParent )
		{
			QRect g( pParent->geometry() );
			QString label = QString("(%1,%2)\n%3x%4").arg(g.x()).arg(g.y()).arg(g.width()).arg(g.height());
			painter.setPen( QColor(0,0,0) );
			painter.drawText(r.adjusted(1,1,1,1), Qt::AlignCenter, label);
			painter.setPen( QColor(255,255,255) );
			painter.drawText(r, Qt::AlignCenter, label);
		}
	}
	
	r.adjust(HANDLE_MARGIN, HANDLE_MARGIN, -HANDLE_MARGIN-1, -HANDLE_MARGIN-1);
	
	for(int i=0; i<2; i++)
	{
		QColor handleColor((i==0) ? QColor(0,0,0) : color);
		
		painter.setBrush(Qt::NoBrush);
		if(i == 0)
			painter.setPen( QPen(handleColor,3) );
		else
			painter.setPen(handleColor);
		painter.drawRect(r);
	
		QRect scaleRect(width()-HANDLE_SIZE-1, height()-HANDLE_SIZE-1, HANDLE_SIZE, HANDLE_SIZE);
		if(i == 0)
			scaleRect.adjust(-1, -1, 1, 1);
		painter.fillRect(scaleRect, handleColor);
	}
}

////////////////////////////////////////////////////////////////////////////////

ToyWindowTab::ToyWindowTab(QWidget *parent)
	: QWidget(parent)
	, m_Mode(ToyWidget::MODE_DEFAULT)
	, m_ShowGrid(false)
	, m_MouseDown(false)
{
	setAutoFillBackground(false);
	setFocusPolicy(Qt::StrongFocus);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::SetShowGrid(bool b)
{
	if(m_ShowGrid != b)
	{
		m_ShowGrid = b;
		UpdateGridBackground();
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::SetGrabbedRect(const QRect &r)
{
	if(m_GrabbedRect != r)
	{
		m_GrabbedRect = r;
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::SetMode(ToyWidget::EnumMode mode)
{
	if(m_Mode != mode)
	{
		m_Mode = mode;
		UpdateMode();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::UpdateMode()
{
	if(m_Mode == ToyWidget::MODE_EDIT)
	{
		SetShowGrid(true);
		SetGrabbedRect( QRect() );
		for(FRAME_LIST::const_iterator i=m_Frames.begin(); i!=m_Frames.end(); i++)
			i->editFrame->InitEditMode();
	}
	else
	{
		SetShowGrid(false);
		SetGrabbedRect( QRect() );
		for(FRAME_LIST::const_iterator i=m_Frames.begin(); i!=m_Frames.end(); i++)
			i->editFrame->ShutdownEditMode();
	}
	
	ClearSelection();
	SetMouseDown(false);
}

////////////////////////////////////////////////////////////////////////////////

Toy* ToyWindowTab::AddToy(Toy::EnumToyType type, const QSize &gridSize, const QPoint &pos, bool clipToBounds, Toy::Client *pClient)
{
	sFrame frame;

	frame.toy = Toy::Create(type, pClient, this, /*flags*/0);
	if( frame.toy )
	{
		frame.toy->setContentsMargins(0, 0, 0, 0);
		frame.toy->SetGridSize(gridSize);
		QColor transparentColor( frame.toy->GetColor() );
		transparentColor.setAlpha(0);
		frame.toy->SetColor(transparentColor);
		frame.toy->setAutoFillBackground(false);
		QPoint toyPos(mapFromGlobal(pos) - QPoint(frame.toy->width()/2,frame.toy->height()/2));
		Utils::Snap(ToyWindowTab::GRID_SPACING, toyPos);
		frame.toy->move(toyPos);

		frame.editFrame = new EditFrame( frame.toy );
		if(m_Mode == ToyWidget::MODE_EDIT)
			frame.editFrame->InitEditMode();
		else
			frame.editFrame->ShutdownEditMode();

		connect(frame.editFrame, SIGNAL(pressed(EditFrame*,bool)), this, SLOT(onEditFramePressed(EditFrame*,bool)));
		connect(frame.editFrame, SIGNAL(translated(EditFrame*,const QPoint&)), this, SLOT(onEditFrameTranslated(EditFrame*,const QPoint&)));
		connect(frame.editFrame, SIGNAL(grabbed(EditFrame*)), this, SLOT(onEditFrameGrabbed(EditFrame*)));
		connect(frame.editFrame, SIGNAL(gridResized(EditFrame*,const QSize&)), this, SLOT(onEditFrameGridResized(EditFrame*,const QSize&)));
		connect(frame.editFrame, SIGNAL(raised(EditFrame*)), this, SLOT(onEditFrameRaised(EditFrame*)));
		connect(frame.editFrame, SIGNAL(lowered(EditFrame*)), this, SLOT(onEditFrameLowered(EditFrame*)));
		connect(frame.editFrame, SIGNAL(deleted(EditFrame*)), this, SLOT(onEditFrameDeleted(EditFrame*)));

		m_Frames.push_back(frame);
		
		if( clipToBounds )
			ClipFrameToBounds(m_Frames.size()-1);
		
		frame.toy->show();
	}
	
	return frame.toy;
}

////////////////////////////////////////////////////////////////////////////////

bool ToyWindowTab::RemoveToy(Toy *toy)
{
	for(FRAME_LIST::iterator i=m_Frames.begin(); i!=m_Frames.end(); i++)
	{
		sFrame frame = *i;
		if(frame.toy == toy)
		{
			frame.toy->close();
			frame.toy->deleteLater();
			frame.editFrame->hide();
			frame.editFrame->deleteLater();
			m_Frames.erase(i);
			return true;
		}
	}
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::ClipFrameToBounds(size_t index)
{
	if(index < m_Frames.size())
	{
		const sFrame &frame = m_Frames[index];
		QRect r( frame.toy->geometry() );
		int maxX = (width() - 1);
		if(r.right() > maxX)
			r.setX(maxX - r.width() + 1);
		if(r.x() < 0)
			r.moveTo(0, r.y());
		int maxY = (height() - 1);
		if(r.bottom() > maxY)
			r.setY(maxY - r.height() + 1);
		if(r.y() < 0)
			r.setY(0);
		if(frame.toy->geometry().topLeft() != r.topLeft())
			frame.toy->move( r.topLeft() );
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::UpdateGridBackground()
{
	if( m_ShowGrid )
	{
		bool dirty = false;
		
		if(m_GridBackgroundImage.size() != size())
		{
			m_GridBackgroundImage = QImage(size(), QImage::Format_RGB32);
			dirty = true;
		}
		
		QColor bgColor( palette().color(QPalette::Window) );
		if(m_GridBackgroundColor != bgColor)
			dirty = true;
		
		if(dirty && !m_GridBackgroundImage.isNull())
		{
			m_GridBackgroundImage.fill(bgColor);
			
			QPainter painter;
			if( painter.begin(&m_GridBackgroundImage) )
			{
				Utils::MakeContrastingColor(0.5, bgColor);
				
				QFont fnt( font() );
				fnt.setPixelSize(100);
				int textWidth = QFontMetrics(fnt).width( tr("Layout Mode") );
				if(textWidth > 0)
				{
					int preferredTextWidth = qRound(width() * 0.8);
					qreal scale = preferredTextWidth/static_cast<qreal>(textWidth);
					fnt.setPixelSize( qRound(fnt.pixelSize()*scale) );
					painter.setFont(fnt);
					
					QColor textColor(bgColor);
					textColor.setAlpha(64);
					painter.setPen(textColor);
					painter.setRenderHint(QPainter::TextAntialiasing);
					painter.drawText(QRect(0,0,width(),height()), Qt::AlignCenter, tr("Layout Mode"));
				}
				
				painter.setPen(bgColor);
				
				for(int x = 0; x<=width(); x+=GRID_SPACING)
				{
					for(int y=0; y<=height(); y+=GRID_SPACING)
						painter.drawPoint(x, y);
				}
				
				painter.end();
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

size_t ToyWindowTab::GetFrameIndex(EditFrame *editFrame) const
{
	if(editFrame != 0)
	{
		for(size_t i=0; i<m_Frames.size(); i++)
		{
			if(m_Frames[i].editFrame == editFrame)
				return i;
		}
	}
	
	return m_Frames.size();
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::ClearLabels()
{
	for(FRAME_LIST::const_iterator i=m_Frames.begin(); i!=m_Frames.end(); i++)
		i->toy->ClearLabels();
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::AddRecvWidgets(Toy::RECV_WIDGETS &recvWidgets) const
{
	for(FRAME_LIST::const_iterator i=m_Frames.begin(); i!=m_Frames.end(); i++)
		i->toy->AddRecvWidgets(recvWidgets);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::ClearSelection()
{
	for(FRAME_LIST::const_iterator i=m_Frames.begin(); i!=m_Frames.end(); i++)
		i->editFrame->SetSelected(false);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::SelectAll()
{
	for(FRAME_LIST::const_iterator i=m_Frames.begin(); i!=m_Frames.end(); i++)
		i->editFrame->SetSelected(true);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::SetToySelected(Toy *toy, bool b)
{
	for(FRAME_LIST::const_iterator i=m_Frames.begin(); i!=m_Frames.end(); i++)
	{
		if(i->toy == toy)
		{
			i->editFrame->SetSelected(b);
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

bool ToyWindowTab::event(QEvent *event)
{
	switch( event->type() )
	{
		case QEvent::PaletteChange:
			UpdateGridBackground();
			break;

		default:
			break;
	}
	
	return QWidget::event(event);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::resizeEvent(QResizeEvent* /*event*/)
{
	UpdateGridBackground();
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::paintEvent(QPaintEvent* /*event*/)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, false);
	
	if(m_ShowGrid && !m_GridBackgroundImage.isNull())
	{
		painter.drawImage(0, 0, m_GridBackgroundImage);
	}
	else
	{
		QColor bgColor( palette().color(QPalette::Window) );
		painter.fillRect(rect(), bgColor);
	}
	
	if( !m_GrabbedRect.isNull() )
	{
		painter.setPen( QColor(0,200,60,60) );
		int x1 = m_GrabbedRect.x();
		int y1 = m_GrabbedRect.y();
		int x2 = m_GrabbedRect.right();
		int y2 = m_GrabbedRect.bottom();
		painter.drawLine(x1, 0, x1, height());
		painter.drawLine(x2, 0, x2, height());
		painter.drawLine(0, y1, width(), y1);
		painter.drawLine(0, y2, width(), y2);
	}
	
	if(m_MouseDown && !m_MouseRect.isEmpty())
		painter.fillRect(m_MouseRect, QColor(0,200,60,30));
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::onEditFramePressed(EditFrame *editFrame, bool clearPrevSelection)
{
	if( editFrame )
	{
		if( clearPrevSelection )
		{
			if( !editFrame->GetSelected() )
			{
				ClearSelection();
				editFrame->SetSelected(true);
			}
		}
		else
			editFrame->SetSelected( !editFrame->GetSelected() );
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::onEditFrameTranslated(EditFrame* /*editFrame*/, const QPoint &delta)
{
	TranslateSelection(delta, /*snap*/false);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::onEditFrameGrabbed(EditFrame *editFrame)
{
	QWidget *pParent = (editFrame ? editFrame->parentWidget() : 0);
	SetGrabbedRect(pParent ? pParent->geometry() : QRect());
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::onEditFrameGridResized(EditFrame *editFrame, const QSize &gridSize)
{
	size_t frameIndex = GetFrameIndex(editFrame);
	if(frameIndex < m_Frames.size())
	{
		const sFrame &frame = m_Frames[frameIndex];
		if( ToyGrid::ConfirmGridResize(this,/*tab*/false,frame.toy->GetGridSize(),gridSize) )
		{
			frame.toy->SetGridSize(gridSize);
			ClipFrameToBounds(frameIndex);
			frame.editFrame->InitEditMode();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::onEditFrameRaised(EditFrame *editFrame)
{
	for(FRAME_LIST::const_iterator i=m_Frames.begin(); i!=m_Frames.end(); i++)
	{
		if(i->editFrame==editFrame || i->editFrame->GetSelected())
		{
			i->toy->raise();
			i->toy->update();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::onEditFrameLowered(EditFrame *editFrame)
{
	for(FRAME_LIST::const_iterator i=m_Frames.begin(); i!=m_Frames.end(); i++)
	{
		if(i->editFrame==editFrame || i->editFrame->GetSelected())
		{
			i->toy->lower();
			i->toy->update();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::onEditFrameDeleted(EditFrame *editFrame)
{
	FRAME_LIST deleteList;
	deleteList.reserve( m_Frames.size() );
	for(FRAME_LIST::const_iterator i=m_Frames.begin(); i!=m_Frames.end(); i++)
	{
		if(i->editFrame==editFrame || i->editFrame->GetSelected())
			deleteList.push_back( *i );
	}
	
	for(FRAME_LIST::const_iterator i=deleteList.begin(); i!=deleteList.end(); i++)
		emit closing( i->toy );
}

////////////////////////////////////////////////////////////////////////////////

TabButton::TabButton(size_t index, QWidget *parent)
	: FadeButton(parent)
	, m_Index(index)
	, m_Rename(0)
{
	QFont fnt( font() );
	fnt.setPixelSize(12);
	setFont(fnt);
	
	connect(this, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
}

////////////////////////////////////////////////////////////////////////////////

void TabButton::RenderBackground(QPainter &painter, QRectF &r)
{
	painter.drawRoundedRect(r.adjusted(0,0,0,ROUNDED+1), ROUNDED, ROUNDED);
}

////////////////////////////////////////////////////////////////////////////////

void TabButton::UpdateLayout()
{
	if( m_Rename )
		m_Rename->setGeometry(0, 0, width(), height());
}

////////////////////////////////////////////////////////////////////////////////

void TabButton::resizeEvent(QResizeEvent *event)
{
	FadeButton::resizeEvent(event);
	UpdateLayout();
}

////////////////////////////////////////////////////////////////////////////////

void TabButton::keyPressEvent(QKeyEvent *event)
{
	if(event->key() == Qt::Key_Escape)
	{
		if(m_Rename && m_Rename->hasFocus())
		{
			onRenameFinished();
			event->accept();
			return;
		}
	}
	
	FadeButton::keyPressEvent(event);
}

////////////////////////////////////////////////////////////////////////////////

void TabButton::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);
	menu.addAction(QIcon(":/assets/images/MenuIconEdit.png"), tr("Rename"), this, SLOT(onRename()));
	menu.exec( event->globalPos() );
	event->accept();
}

////////////////////////////////////////////////////////////////////////////////

void TabButton::onRename()
{
	// hide button text during edit
	m_ButtonTextColor = palette().color(QPalette::ButtonText);
	QPalette pal( palette() );
	pal.setColor(QPalette::ButtonText, QColor(0,0,0,0));
	setPalette(pal);
	
	if( !m_Rename )
	{
		m_Rename = new QLineEdit(this);
		m_Rename->setAlignment(Qt::AlignCenter);
		m_Rename->setFrame(false);
		connect(m_Rename, SIGNAL(returnPressed()), this, SLOT(onRenameFinished()));
		connect(m_Rename, SIGNAL(editingFinished()), this, SLOT(onRenameFinished()));
		connect(m_Rename, SIGNAL(textChanged(const QString&)), this, SLOT(onRenameTextChanged(const QString&)));
	}
	
	pal = m_Rename->palette();
	pal.setColor(QPalette::Base, QColor(0,0,0,0));
	pal.setColor(QPalette::Text, m_ButtonTextColor);
	m_Rename->setPalette(pal);
	
	m_Rename->setText( text() );
	m_Rename->selectAll();
	m_Rename->raise();
	m_Rename->show();
	m_Rename->setFocus();
	UpdateLayout();
}

////////////////////////////////////////////////////////////////////////////////

void TabButton::onRenameFinished()
{
	if( m_Rename )
	{
		m_Rename->hide();
		
		// restore button text
		QPalette pal( palette() );
		pal.setColor(QPalette::ButtonText, m_ButtonTextColor);
		setPalette(pal);
		
		setText( m_Rename->text() );
		emit tabChanged(m_Index);
	}
}

////////////////////////////////////////////////////////////////////////////////

void TabButton::onRenameTextChanged(const QString &text)
{
	setText(text);
	emit tabChanged(m_Index);
}

////////////////////////////////////////////////////////////////////////////////

void TabButton::onClicked(bool /*checked*/)
{
	emit tabSelected(m_Index);
}

////////////////////////////////////////////////////////////////////////////////

TabBar::TabBar(QWidget *parent)
	: QWidget(parent)
{
}

////////////////////////////////////////////////////////////////////////////////

void TabBar::paintEvent(QPaintEvent* /*event*/)
{
	QPainter painter(this);
	painter.fillRect(QRect(0,0,width(),height()-TAB_SPACING), palette().color(QPalette::ButtonText));
	painter.fillRect(QRect(0,height()-TAB_SPACING,width(),TAB_SPACING), palette().color(QPalette::Button));
}

////////////////////////////////////////////////////////////////////////////////

ToyWindow::ToyWindow(Client *pClient, QWidget *parent, Qt::WindowFlags flags)
	: ToyGrid(TOY_WINDOW, pClient, parent, flags)
	, m_TabIndex(0)
	, m_Color2(TEXT_COLOR)
	, m_TextColor(BG_COLOR)
{
	setAutoFillBackground(true);
	
	m_TabBar = new TabBar(this);
	UpdateTabs();
	
	connect(this, SIGNAL(layoutModeSelected()), this, SLOT(onLayoutModeSelected()));
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::SetGridSize(const QSize &gridSize)
{
	size_t numTabs = 1;
	
	if(gridSize.width() > 1)
		numTabs = static_cast<size_t>( gridSize.width() );
	
	if(m_Tabs.size() != numTabs)
	{
		m_Tabs.reserve(numTabs);
		
		while(m_Tabs.size() > numTabs)
		{
			sTab &tab = m_Tabs.back();
			tab.button->hide();
			tab.button->deleteLater();
			tab.widget->hide();
			tab.widget->deleteLater();
			m_Tabs.pop_back();
		}
		
		while(m_Tabs.size() < numTabs)
		{
			sTab tab;
			tab.button = new TabButton(m_Tabs.size(), m_TabBar);
			tab.button->setText( tr("Tab %1").arg(m_Tabs.size()+1) );
			tab.button->show();
			connect(tab.button, SIGNAL(tabSelected(size_t)), this, SLOT(onTabSelected(size_t)));
			connect(tab.button, SIGNAL(tabChanged(size_t)), this, SLOT(onTabChanged(size_t)));
			tab.widget = new ToyWindowTab(this);
			connect(tab.widget, SIGNAL(closing(Toy*)), this, SLOT(onToyClosing(Toy*)));
			tab.widget->hide();
			m_Tabs.push_back(tab);
		}
		
		if(m_TabIndex >= m_Tabs.size())
			m_TabIndex = (m_Tabs.size() - 1);
		
		UpdateTabs();
		UpdateLayout();
		
		emit recvWidgetsChanged();
		emit changed();
	}
	
	m_GridSize = QSize(static_cast<int>(m_Tabs.size()), 1);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::SetTabIndex(size_t index)
{
	if(index >= m_Tabs.size())
		index = (m_Tabs.size() - 1);
	
	if(m_TabIndex != index)
	{
		m_TabIndex = index;
		UpdateTabs();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::UpdateTabs()
{
	QPalette pal( m_TabBar->palette() );
	pal.setColor(QPalette::Button, m_Color2);
	pal.setColor(QPalette::ButtonText, m_TextColor);
	m_TabBar->setPalette(pal);
	
	for(size_t i=0; i<m_Tabs.size(); i++)
	{
		sTab &tab = m_Tabs[i];
		bool selected = (i == m_TabIndex);
		QPalette tabPal( tab.button->palette() );
		if( selected )
		{
			tabPal.setColor(QPalette::Button, m_Color2);
			tabPal.setColor(QPalette::ButtonText, m_TextColor);
		}
		else
		{
			QColor translucent(m_Color2);
			translucent.setAlpha(40);
			tabPal.setColor(QPalette::Button, translucent);
			tabPal.setColor(QPalette::ButtonText, m_Color2);
		}
		tab.button->setPalette(tabPal);
		tab.widget->setVisible(selected);
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::SetShowGrid(bool b)
{
	for(TABS::const_iterator i=m_Tabs.begin(); i!=m_Tabs.end(); i++)
		i->widget->SetShowGrid(b);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::SetColor2(const QColor &color2)
{
	if(m_Color2 != color2)
	{
		m_Color2 = color2;
		UpdateTabs();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::SetTextColor(const QColor &textColor)
{
	if(m_TextColor != textColor)
	{
		m_TextColor = textColor;
		UpdateTabs();
	}
}

////////////////////////////////////////////////////////////////////////////////

Toy* ToyWindow::AddToy(EnumToyType type, const QSize &gridSize, const QPoint &pos)
{
	return AddToyToTab(m_TabIndex, type, gridSize, pos);
}

////////////////////////////////////////////////////////////////////////////////

Toy* ToyWindow::AddToyToTab(size_t tabIndex, EnumToyType type, const QSize &gridSize, const QPoint &pos)
{
	Toy *toy = 0;
	
	if(tabIndex < m_Tabs.size())
	{
		ToyWindowTab *tab = m_Tabs[tabIndex].widget;
		bool clipToBounds = !m_Loading;
		toy = tab->AddToy(type, gridSize, pos, clipToBounds, m_pClient);
		if( toy )
		{
			connect(toy, SIGNAL(recvWidgetsChanged()), this, SLOT(onRecvWidgetsChanged()));
			connect(toy, SIGNAL(closing(Toy*)), this, SLOT(onToyClosing(Toy*)));
			connect(toy, SIGNAL(changed()), this, SLOT(onToyChanged()));
			connect(toy, SIGNAL(toggleMainWindow()), this, SLOT(onToyToggledMainWindow()));
			connect(toy, SIGNAL(layoutModeSelected()), this, SLOT(onLayoutModeSelected()));
			
			if( !m_Loading )
			{
				tab->ClearSelection();
				tab->SetToySelected(toy, true);
				activateWindow();
				tab->setFocus();
				emit recvWidgetsChanged();
				emit changed();
			}
		}
	}
	
	return toy;
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::UpdateMode()
{
	ToyGrid::UpdateMode();
	
	for(TABS::const_iterator i=m_Tabs.begin(); i!=m_Tabs.end(); i++)
		i->widget->SetMode(m_Mode);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::UpdateLayout()
{
	if(m_Tabs.size() > 1)
	{
		int maxTabHeight = 0;
		for(TABS::const_iterator i=m_Tabs.begin(); i!=m_Tabs.end(); i++)
		{
			int h = i->button->sizeHint().height();
			if(h > maxTabHeight)
				maxTabHeight = h;
		}
		
		m_TabBar->setGeometry(0, 0, width(), maxTabHeight+TAB_SPACING);
		
		int x = 0;
		int y = m_TabBar->geometry().bottom() + 1;
		QRect widgetRect(0, y, width(), height()-y);
		for(size_t i=0; i<m_Tabs.size(); i++)
		{
			const sTab &tab = m_Tabs[i];
			tab.button->setGeometry(x, 0, tab.button->sizeHint().width(), maxTabHeight);
			x += tab.button->width();
			tab.button->show();
			tab.widget->setGeometry(widgetRect);
		}
		
		m_TabBar->show();
	}
	else
	{
		QRect widgetRect(0, 0, width(), height());
		for(size_t i=0; i<m_Tabs.size(); i++)
			m_Tabs[i].widget->setGeometry(widgetRect);
		
		m_TabBar->hide();
	}
}

////////////////////////////////////////////////////////////////////////////////

int ToyWindow::GetWidgetZOrder(QWidget &w)
{
	QWidget *pParent = w.parentWidget();
	if( pParent )
	{
		const QObjectList &childs = pParent->children();
		for(int i=0; i<childs.size(); i++)
		{
			QWidget *childWidget = qobject_cast<QWidget*>( childs[i] );
			if(childWidget == (&w))
				return i;
		}
	}
	
	return INT_MAX;
}

////////////////////////////////////////////////////////////////////////////////

bool ToyWindow::Save(EosLog &log, const QString &path, QStringList &lines)
{
	ToyGrid::Save(log, path, lines);
	
	lines << QString::number( static_cast<int>(m_TabIndex) );
	
	for(TABS::const_iterator i=m_Tabs.begin(); i!=m_Tabs.end(); i++)
	{
		const ToyWindowTab *tab = i->widget;
		const ToyWindowTab::FRAME_LIST &frames = tab->GetFrames();
		
		// save in Qt children order to preserve z-ordering
		TOYS_BY_ZORDER toysByZOrder;
		for(ToyWindowTab::FRAME_LIST::const_iterator j=frames.begin(); j!=frames.end(); j++)
		{
			Toy *toy = j->toy;
			toysByZOrder.insert( TOYS_BY_ZORDER_PAIR(GetWidgetZOrder(*toy),toy) );
		}
		
		QString line;
		
		line.append( QString("%1").arg(Utils::QuotedString(i->button->text())) );
		line.append( QString(", %1").arg(static_cast<int>(toysByZOrder.size())) );
		
		lines << line;
		
		for(TOYS_BY_ZORDER::const_iterator j=toysByZOrder.begin(); j!=toysByZOrder.end(); j++)
			j->second->Save(log, path, lines);
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool ToyWindow::Load(EosLog &log, const QString &path, QStringList &lines, int &index)
{
	ToyGrid::Load(log, path, lines, index);
	
	if(index>=0 && index<lines.size())
	{
		m_Loading = true;
		
		QStringList items;
		Utils::GetItemsFromQuotedString(lines[index++], items);
		
		m_TabIndex = 0;
		if(items.size() > 0)
		{
			int n = items[0].toInt();
			if(n>=0 && n<static_cast<int>(m_Tabs.size()))
				m_TabIndex = static_cast<size_t>(n);
		}
		
		for(size_t tabIndex=0; tabIndex<m_Tabs.size() && index<lines.size(); tabIndex++)
		{
			sTab &tab = m_Tabs[tabIndex];
			Utils::GetItemsFromQuotedString(lines[index++], items);
			
			if(items.size() > 0)
				tab.button->setText( items[0] );
			
			if(items.size() > 1)
			{
				int numFrames = items[1].toInt();

				Toys::TOY_LIST addedToys;
				addedToys.reserve(numFrames);

				for(int frameIndex=0; frameIndex<numFrames && index<lines.size(); frameIndex++)
				{
					Utils::GetItemsFromQuotedString(lines[index], items);
					
					bool ok = false;
					int n = items[0].toInt(&ok);
					if(ok && n>=0 && n<Toy::TOY_COUNT)
					{
						Toy *toy = AddToyToTab(tabIndex, static_cast<Toy::EnumToyType>(n), QSize(1,1), QPoint(0,0));
						if( toy )
						{
							toy->Load(log, path, lines, index);
							addedToys.push_back(toy);
						}
					}
				}

				// ensure proper z-ordering
				for(Toys::TOY_LIST::const_iterator i=addedToys.begin(); i!=addedToys.end(); i++)
					(*i)->raise();
			}
		}
		
		m_Loading = false;
	}
	
	UpdateLayout();
	UpdateTabs();
	
	emit recvWidgetsChanged();
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::ClearLabels()
{
	for(TABS::const_iterator i=m_Tabs.begin(); i!=m_Tabs.end(); i++)
		i->widget->ClearLabels();
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::AddRecvWidgets(RECV_WIDGETS &recvWidgets) const
{
	for(TABS::const_iterator i=m_Tabs.begin(); i!=m_Tabs.end(); i++)
		i->widget->AddRecvWidgets(recvWidgets);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::TranslateSelection(const QPoint &delta, bool snap)
{
	QPoint clippedDelta(delta);
	
	for(FRAME_LIST::const_iterator i=m_Frames.begin(); i!=m_Frames.end(); i++)
	{
		EditFrame *editFrame = i->editFrame;
		if( editFrame->GetSelected() )
		{
			QPoint prevPos( i->toy->pos() );
			QPoint newPos(prevPos + delta);
			if( snap )
				Utils::Snap(GRID_SPACING, newPos);
			i->editFrame->ClipPos(newPos);
			QPoint frameDelta(newPos - prevPos);
			if(abs(frameDelta.x()) < abs(clippedDelta.x()))
				clippedDelta.setX( frameDelta.x() );
			if(abs(frameDelta.y()) < abs(clippedDelta.y()))
				clippedDelta.setY( frameDelta.y() );
		}
	}
	
	if(clippedDelta.x()!=0 || clippedDelta.y()!=0)
	{
		for(FRAME_LIST::const_iterator i=m_Frames.begin(); i!=m_Frames.end(); i++)
		{
			EditFrame *editFrame = i->editFrame;
			if( editFrame->GetSelected() )
				i->editFrame->Translate(clippedDelta, /*snap*/false);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::HandleTranslateKey(QKeyEvent *event)
{
	QPoint delta(0, 0);
	
	switch( event->key() )
	{
		case Qt::Key_Left:	delta.setX(-1); break;
		case Qt::Key_Up:	delta.setY(-1); break;
		case Qt::Key_Right:	delta.setX(1); break;
		case Qt::Key_Down:	delta.setY(1); break;
	}
	
	if(delta.x()!=0 || delta.y()!=0)
	{
		bool snap = !event->modifiers().testFlag(Qt::ShiftModifier);
		
		if( snap )
		{
			delta.rx() *= GRID_SPACING;
			delta.ry() *= GRID_SPACING;
		}
		
		TranslateSelection(delta, snap);
		
		event->accept();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::SetMouseRect(const QRect &r)
{
	if(m_MouseRect != r)
	{
		m_MouseRect = r;
		
		if(m_MouseDown && !m_MouseRect.isEmpty())
		{
			for(FRAME_INDICIES::const_iterator i=m_MousePrevUnselected.begin(); i!=m_MousePrevUnselected.end(); i++)
			{
				size_t index = *i;
				if(index < m_Frames.size())
				{
					const sFrame &frame = m_Frames[index];
					frame.editFrame->SetSelected( m_MouseRect.intersects(frame.toy->geometry()) );
				}
			}
		}
		
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::SetMouseDown(bool b)
{
	if(m_MouseDown != b)
	{
		m_MouseDown = b;
		
		m_MousePrevUnselected.clear();
		
		if( m_MouseDown )
		{
			m_MousePrevUnselected.reserve( m_Frames.size() );
			for(size_t i=0; i<m_Frames.size(); i++)
			{
				if( !m_Frames[i].editFrame->GetSelected() )
					m_MousePrevUnselected.push_back(i);
			}
		}
		else
			SetMouseRect( QRect() );
		
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::keyPressEvent(QKeyEvent *event)
{
	if(m_Mode == ToyWidget::MODE_EDIT)
	{
		switch( event->key() )
		{
			case Qt::Key_A:
				{
					if( event->modifiers().testFlag(Qt::ControlModifier) )
					{
						SelectAll();
						event->accept();
					}
				}
				break;
				
			case Qt::Key_Left:
			case Qt::Key_Up:
			case Qt::Key_Right:
			case Qt::Key_Down:
				HandleTranslateKey(event);
				break;
				
			case Qt::Key_Delete:
			case Qt::Key_Backspace:
				onEditFrameDeleted(/*editFrame*/0);
				break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::mousePressEvent(QMouseEvent *event)
{
	if(m_Mode == ToyWidget::MODE_EDIT)
	{
		Qt::KeyboardModifiers modifiers = event->modifiers();
		if(!modifiers.testFlag(Qt::ShiftModifier) && !modifiers.testFlag(Qt::ControlModifier))
			ClearSelection();
		
		m_MousePos = event->pos();
		SetMouseDown(true);
		SetMouseRect( QRect() );
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::mouseMoveEvent(QMouseEvent *event)
{
	if( m_MouseDown )
	{
		int x1 = m_MousePos.x();
		int x2 = event->pos().x();
		if(x2 < x1)
			qSwap(x1, x2);
		int y1 = m_MousePos.y();
		int y2 = event->pos().y();
		if(y2 < y1)
			qSwap(y1, y2);
		SetMouseRect( QRect(QPoint(x1,y1),QPoint(x2,y2)) );
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindowTab::mouseReleaseEvent(QMouseEvent* /*event*/)
{
	SetMouseDown(false);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::onRecvWidgetsChanged()
{
	emit recvWidgetsChanged();
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::onToyClosing(Toy *toy)
{
	if( toy )
	{
		for(TABS::const_iterator i=m_Tabs.begin(); i!=m_Tabs.end(); i++)
		{
			if( i->widget->RemoveToy(toy) )
			{
				emit recvWidgetsChanged();
				emit changed();
				break;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::onToyChanged()
{
	emit changed();
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::onToyToggledMainWindow()
{
	emit toggleMainWindow();
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::onTabSelected(size_t index)
{
	SetTabIndex(index);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::onTabChanged(size_t /*index*/)
{
	UpdateLayout();
}

////////////////////////////////////////////////////////////////////////////////

void ToyWindow::onLayoutModeSelected()
{
	m_EditWidgetIndex = m_List.size();
	SetMode(ToyWidget::MODE_EDIT);
	EditWidget((m_EditWidgetIndex<m_List.size()) ? m_List[m_EditWidgetIndex] : 0, /*toggle*/false);
}

////////////////////////////////////////////////////////////////////////////////

