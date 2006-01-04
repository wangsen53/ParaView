
#include "pqMultiView.h"

#include <QLayout>
#include <QSplitter>

namespace {
  
  QFrame* makeNewFrame()
    {
    QFrame* frame = new QFrame();
    frame->setFrameShadow(QFrame::Sunken);
    frame->setFrameShape(QFrame::Panel);
    frame->setLineWidth(2);
    return frame;
    }
  QSplitter* makeNewSplitter(Qt::Orientation orientation)
    {
    QSplitter* splitter = new QSplitter(orientation);
    return splitter;
    }
}

pqMultiView::pqMultiView(QWidget* parent)
  : QFrame(parent)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);
  this->setLayout(layout);
  QSplitter* splitter = new QSplitter(this);
  splitter->setObjectName("MultiViewSplitter");
  layout->addWidget(splitter);
  splitter->addWidget(makeNewFrame());
}

pqMultiView::~pqMultiView()
{
}

QWidget* pqMultiView::replaceView(pqMultiView::Index index, QWidget* widget)
{
  if(!widget)
    return NULL;

  QWidget* w = this->widgetOfIndex(index);
  QSplitter* splitter = qobject_cast<QSplitter*>(w->parentWidget());
  
  if(splitter)
    {
    // get location of widget in splitter
    int location = splitter->indexOf(w);
    // save splitter sizes
    QList<int> sizes;
    if(splitter->count() > 1)
      {
      sizes = splitter->sizes();
      }

    splitter->hide();

    // remove widget
    w->setParent(NULL);
    // add replacement at same location
    splitter->insertWidget(location, widget);
    
    splitter->show();

    // ensure same splitter sizes
    if(splitter->count() > 1)
      {
      splitter->setSizes(sizes);
      }
    return w;
    }
  return NULL;
}

void pqMultiView::removeView(QWidget* widget)
{
  QSplitter* splitter = qobject_cast<QSplitter*>(widget->parentWidget());
  if(splitter)
    {

    // remove widget
    widget->setParent(NULL);
    
    // if splitter is empty, add place holder
    if(splitter->count() == 0 && splitter->parentWidget() == this)
      {
      splitter->addWidget(makeNewFrame());
      }
    // if splitter can be merged with parent splitter
    else if(splitter->count() < 2 && splitter->parentWidget() != this)
      {
      QWidget* otherWidget = splitter->widget(0);
      QSplitter* parentSplitter = qobject_cast<QSplitter*>(splitter->parentWidget());
      Q_ASSERT(parentSplitter != NULL);
      int location = parentSplitter->indexOf(splitter);
      // get sizes
      QList<int> sizes = parentSplitter->sizes();
      // in Qt 4.0.1, setParent has a side effect of calling XFlush, so we see some intermediate stuff
      otherWidget->setParent(NULL);   
      splitter->setParent(NULL);
      parentSplitter->insertWidget(location, otherWidget);
      // restore sizes
      parentSplitter->setSizes(sizes);
      delete splitter;
      }
    else
      {
      // will Qt 4.1 fix this so we don't have to call refresh?
      // fix stray splitter handles because children were removed
      splitter->refresh();
      }
    }
}

pqMultiView::Index pqMultiView::splitView(pqMultiView::Index index, Qt::Orientation orientation)
{

  QFrame* newFrame = NULL;

  QWidget* w = this->widgetOfIndex(index);
  Q_ASSERT(w != NULL);
  QSplitter* splitter = qobject_cast<QSplitter*>(w->parentWidget());
  if(!splitter)
    return Index();

  // if there is only one item in splitter
  if(splitter->count() < 2)
    {
    // change orientation
    splitter->setOrientation(orientation);
    // add new place holder
    newFrame = makeNewFrame();
    splitter->addWidget(newFrame);
    // make equal spacing
    QList<int> sizes = splitter->sizes();
    int sum=0, i;
    for(i=0; i<sizes.size(); i++)
      sum += sizes[i];
    for(i=0; i<sizes.size(); i++)
      sizes[i] = sum / sizes.size();
    splitter->setSizes(sizes);
    }
  // else if the orientation isn't the same, we need to make a new child splitter
  // with the desired orientation
  else if(splitter->orientation() != orientation)
    {
    // get parent sizes
    QList<int> parentsizes = splitter->sizes();
    splitter->hide();
    
    int location = splitter->indexOf(w);
    QSplitter* newSplitter = makeNewSplitter(orientation);
    // add splitter to splitter
    splitter->insertWidget(location, newSplitter);
    // remove from old splitter, and add to new splitter
    w->setParent(newSplitter);
    newSplitter->addWidget(w);
    // add new place holder
    newFrame = makeNewFrame();
    newSplitter->addWidget(newFrame);
    
    splitter->show();
    // ensure same sizes for parent splitter
    splitter->setSizes(parentsizes);

    // make equal spacing for new splitter
    QList<int> sizes = newSplitter->sizes();
    int sum=0, i;
    for(i=0; i<sizes.size(); i++)
      sum += sizes[i];
    for(i=0; i<sizes.size(); i++)
      sizes[i] = sum / sizes.size();
    newSplitter->setSizes(sizes);
    
    QByteArray n = "MultiViewSplitter:";
    Index idxName = this->indexOf(newSplitter);
    for(i=0; i<idxName.size(); i++)
      {
      QString tmp;
      tmp.setNum(idxName[i]);
      if(i != 0)
        n += ",";
      n += tmp;
      }
    newSplitter->setObjectName(n);
    }
  else
    {
    // insert new below or on right of existing one
    newFrame = makeNewFrame();
    splitter->insertWidget(splitter->indexOf(w)+1, newFrame);
    
    // make equal spacing
    QList<int> sizes = splitter->sizes();
    int sum=0, i;
    for(i=0; i<sizes.size(); i++)
      sum += sizes[i];
    for(i=0; i<sizes.size(); i++)
      sizes[i] = sum / sizes.size();
    splitter->setSizes(sizes);
    }
    
  return indexOf(newFrame);
}

pqMultiView::Index pqMultiView::indexOf(QWidget* widget) const
{
  Index index;
  
  if(!widget)
    return index;

  QWidget* parent = widget->parentWidget();
  while(parent && parent != this)
    {
    QSplitter* splitter = qobject_cast<QSplitter*>(parent);
    if(splitter)
      {
      index.push_front(splitter->indexOf(widget));
      }
    else
      {
      QLayout* layout = parent->layout();
      Q_ASSERT(layout != NULL);
      index.push_front(layout->indexOf(widget));
      }
    widget = parent;
    parent = parent->parentWidget();
    }
  return index;
}


QWidget* pqMultiView::widgetOfIndex(Index index)
{
  if(index.empty() && static_cast<QSplitter*>(this->layout()->itemAt(0)->widget())->count() == 1)
    {
    return static_cast<QSplitter*>(this->layout()->itemAt(0)->widget())->widget(0);
    }
  else if(index.empty())
    {
    return NULL;
    }

  Index::iterator iter = index.begin();
  Index::iterator end = index.end();
  QWidget* w = this->layout()->itemAt(0)->widget();
  for(; iter != end && w; ++iter)
    {
    QSplitter* splitter = qobject_cast<QSplitter*>(w);
    if(splitter)
      {
      w = splitter->widget(*iter);
      }
    else
      {
      return NULL;
      }
    }
  
  // Bad index was passed in, return NULL
  if(iter != index.end())
    {
    w = NULL;
    }

  return w;
}


