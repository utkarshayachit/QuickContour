/*=========================================================================

   Program: ParaView
   Module:    qcMainWindow.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "qcMainWindow.h"
#include "ui_qcMainWindow.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqLoadDataReaction.h"
#include "pqObjectBuilder.h"
#include "pqParaViewBehaviors.h"
#include "pqParaViewMenuBuilders.h"
#include "pqRenderView.h"
#include "pqServerConnectReaction.h"
#include "pqServer.h"

//-----------------------------------------------------------------------------
qcMainWindow::qcMainWindow(QWidget *_parent, Qt::WindowFlags _flags):
  Superclass(_parent, _flags)
{
  Ui::MainWindow ui;
  ui.setupUi(this);

  pqParaViewMenuBuilders::buildHelpMenu(*ui.menu_Help);
  new pqParaViewBehaviors(this, this);

  new pqServerConnectReaction(ui.actionConnect);
  new pqLoadDataReaction(ui.actionLoad);

  pqRenderView* view1 = this->createRenderView(ui.renderFrame1);
  pqRenderView* view2 = this->createRenderView(ui.renderFrame2);
}

//-----------------------------------------------------------------------------
qcMainWindow::~qcMainWindow()
{
}

//-----------------------------------------------------------------------------
pqRenderView* qcMainWindow::createRenderView(QWidget* widget)
{
  QHBoxLayout *hbox = new QHBoxLayout(widget);
  hbox->setMargin(0);
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  pqRenderView *view = qobject_cast<pqRenderView*>(
    builder->createView(pqRenderView::renderViewType(),
      pqActiveObjects::instance().activeServer()));
  hbox->addWidget(view->getWidget());
  return view;
}
