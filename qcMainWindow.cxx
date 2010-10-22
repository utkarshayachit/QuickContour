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
#include "pqFieldSelectionAdaptor.h"
#include "pqLoadDataReaction.h"
#include "pqObjectBuilder.h"
#include "pqObjectInspectorWidget.h"
#include "pqParaViewBehaviors.h"
#include "pqParaViewMenuBuilders.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqRubberBandHelper.h"
#include "pqServerConnectReaction.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerSelectionModel.h"
#include "vtkDataObject.h"
#include "vtkProperty.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMSourceProxy.h"

#include <QDialog>

class qcMainWindow::pqInternals : public Ui::MainWindow
{
public:
  QPointer<pqFieldSelectionAdaptor> contourByDomain;
};

//-----------------------------------------------------------------------------
qcMainWindow::qcMainWindow(QWidget *_parent, Qt::WindowFlags _flags):
  Superclass(_parent, _flags)
{
  this->Ui = new pqInternals();
  this->Ui->setupUi(this);

  // Whenever the user connects to a server, we setup the environment and clean
  // it up when the user disconnects from that server.
  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(serverChanged(pqServer*)), this, SLOT(initialize(pqServer*)));
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(aboutToRemoveServer(pqServer*)), this, SLOT(cleanup()));

  // pqRubberBandHelper will be used to help us make the selection when the
  // user clicks on the 2D view.
  this->SelectionHelper = new pqRubberBandHelper(this);
  QObject::connect(this->Ui->actionAdd, SIGNAL(triggered()),
    this->SelectionHelper, SLOT(beginSurfaceSelection()));
  QObject::connect(
    this->SelectionHelper, SIGNAL(selectionFinished(int, int, int, int)),
    this, SLOT(endSelection()), Qt::QueuedConnection);

  // Initialize all readers available to ParaView. Now our application can load
  // all types of datasets supported by ParaView.
  vtkSMProxyManager::GetProxyManager()->GetReaderFactory()->RegisterPrototypes("sources");

  // We just want to use the default ParaView help menu.
  pqParaViewMenuBuilders::buildHelpMenu(*this->Ui->menu_Help);
  new pqParaViewBehaviors(this, this);

  // Setup standard responses for the load data and connect actions.
  new pqServerConnectReaction(this->Ui->actionConnect);
  pqLoadDataReaction* ldr = new pqLoadDataReaction(this->Ui->actionLoad);

  // When a dataset is loaded we want to setup the visualization pipelines.
  QObject::connect(ldr, SIGNAL(loadedData(pqPipelineSource*)),
    this, SLOT(onDataLoaded(pqPipelineSource*)));

  // When user clicks clear, we want to remove all existing contours.
  QObject::connect(this->Ui->actionClear, SIGNAL(triggered()), this,
    SLOT(clearContours()));
}

//-----------------------------------------------------------------------------
qcMainWindow::~qcMainWindow()
{
  delete this->Ui;
  this->Ui = NULL;
}

//-----------------------------------------------------------------------------
void qcMainWindow::initialize(pqServer* server)
{
  if (server)
    {
    this->View1 = this->createRenderView(this->Ui->renderFrame1);
    this->View2 = this->createRenderView(this->Ui->renderFrame2);
    this->View2->setCenterAxesVisibility(false);
    this->View2->setOrientationAxesVisibility(false);
    // remove roll/rotate interactions from the 2D view.
    vtkSMPropertyHelper helper(this->View2->getProxy(), "CameraManipulators");
    for (unsigned int cc=0; cc < helper.GetNumberOfElements(); cc++)
      {
      vtkSMProxy* manip = helper.GetAsProxy(cc);
      if (manip &&
        (strcmp(manip->GetXMLName(), "TrackballRotate") == 0 ||
         strcmp(manip->GetXMLName(), "TrackballRoll") == 0))
        {
        helper.Remove(manip);
        cc--;
        }
      }
    this->SelectionHelper->setView(this->View2);
    }
}

//-----------------------------------------------------------------------------
void qcMainWindow::cleanup()
{
  this->View1 = 0;
  this->View2 = 0;
  this->ActiveSource = 0;
  this->Slice =0;
  this->Contour =0;
  this->SliceContour = 0;
  this->ActiveSourceRepr =0;
  this->SliceRepr=0;
  this->ContourRepr=0;
  this->SliceContourRepr=0;
  this->Ui->colorBy->setRepresentation(NULL);
  this->SelectionHelper->setView(NULL);
  pqApplicationCore::instance()->getSelectionModel()->setCurrentItem(NULL,
    pqServerManagerSelectionModel::Clear);
  pqActiveObjects::instance().setActiveSource(NULL);
  delete this->Ui->contourByDomain;
  this->Ui->contourByDomain = NULL;
}

//-----------------------------------------------------------------------------
pqRenderView* qcMainWindow::createRenderView(QWidget* widget)
{
  delete widget->layout();
  QHBoxLayout *hbox = new QHBoxLayout(widget);
  hbox->setMargin(0);
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  pqRenderView *view = qobject_cast<pqRenderView*>(
    builder->createView(pqRenderView::renderViewType(),
      pqActiveObjects::instance().activeServer()));
  hbox->addWidget(view->getWidget());
  return view;
}

//-----------------------------------------------------------------------------
void qcMainWindow::onDataLoaded(pqPipelineSource* source)
{
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  if (this->Contour)
    {
    builder->destroy(this->Contour);
    }
  if (this->Slice)
    {
    builder->destroy(this->Slice);
    }
  if (this->ActiveSource)
    {
    builder->destroy(this->ActiveSource);
    }

  this->ActiveSource = source;

  // Popup a dialog allowing user to choose reader parameters.
  QDialog dialog(this);
  dialog.setWindowTitle("Configure Reader Parameters");
  QHBoxLayout* hbox = new QHBoxLayout(&dialog);
  pqObjectInspectorWidget* inspector = new pqObjectInspectorWidget(&dialog);
  inspector->setDeleteButtonVisibility(false);
  hbox->addWidget(inspector);
  inspector->setProxy(source);
  QObject::connect(inspector, SIGNAL(postaccept()), &dialog, SLOT(accept()));
  dialog.resize(350, 500);
  dialog.exec();
  inspector->setProxy(NULL);
  delete inspector;

  pqDataRepresentation* repr1 = builder->createDataRepresentation(
    source->getOutputPort(0), this->View1);
  vtkSMPropertyHelper(repr1->getProxy(), "Representation").Set(VTK_WIREFRAME);
  vtkSMPropertyHelper(repr1->getProxy(), "Opacity").Set(0.5);
  repr1->getProxy()->UpdateVTKObjects();
  this->ActiveSourceRepr = qobject_cast<pqPipelineRepresentation*>(repr1);

  // Show the slice in the View2.
  this->Slice = builder->createFilter("filters", "Cut", source);
  pqDataRepresentation* repr2 = builder->createDataRepresentation(
    this->Slice->getOutputPort(0), this->View2);
  this->SliceRepr = qobject_cast<pqPipelineRepresentation*>(repr2);

  // Show the contour in the View1
  this->Contour = builder->createFilter("filters", "Contour", source);
  pqDataRepresentation* repr3 = builder->createDataRepresentation(
    this->Contour->getOutputPort(0), this->View1);
  this->ContourRepr = qobject_cast<pqPipelineRepresentation*>(repr3);

  // The colorBy control, controls the scalar coloring for the contours
  this->Ui->colorBy->setRepresentation(repr3);

  this->SliceContour = builder->createFilter("filters", "Cut", this->Contour);
  this->SliceContourRepr = qobject_cast<pqPipelineRepresentation*>(
    builder->createDataRepresentation(
      this->SliceContour->getOutputPort(0), this->View2));

  this->Contour->updatePipeline();

  this->Ui->contourByDomain = new pqFieldSelectionAdaptor(
    this->Ui->contourBy,
    this->Contour->getProxy()->GetProperty("SelectInputScalars"));
  this->Ui->contourBy->setCurrentIndex(1);

  QObject::connect(this->Ui->contourByDomain,
    SIGNAL(selectionChanged()), this, SLOT(contourByChanged()));
  this->Ui->contourBy->setCurrentIndex(0);

  this->View1->resetViewDirection(1, 0, 0, 0, 0, 1);
  this->View2->resetViewDirection(1, 0, 0, 0, 0, 1);
  this->View1->render();
  this->View2->render();
}

//-----------------------------------------------------------------------------
void qcMainWindow::contourByChanged()
{
  QString arrayName = this->Ui->contourByDomain->scalar().toAscii().data();
  vtkSMPropertyHelper(this->Contour->getProxy(), "SelectInputScalars").Set(4,
    arrayName.toAscii().data());
  vtkSMPropertyHelper(this->Contour->getProxy(),
    "ContourValues").SetNumberOfElements(0);
  this->Contour->getProxy()->UpdateVTKObjects();

  this->SliceRepr->colorByArray(arrayName.toAscii().data(),
    vtkDataObject::FIELD_ASSOCIATION_POINTS);

  this->View1->render();
  this->View2->render();
}

//-----------------------------------------------------------------------------
void qcMainWindow::endSelection()
{
  this->SelectionHelper->endSelection();

  if (!this->ActiveSource)
    {
    return;
    }

  // Now we get information about the cell that the user picked.
  vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(
    this->Slice->getProxy());
  vtkSMSourceProxy* extracted = source->GetSelectionOutput(0);
  extracted->UpdatePipeline();
  vtkPVDataInformation* info = extracted->GetDataInformation();
  vtkPVArrayInformation* arrayInfo =
    info->GetPointDataInformation()->GetArrayInformation(
      this->Ui->contourByDomain->scalar().toAscii().data());
  if (!arrayInfo)
    {
    return;
    }

  double value = (arrayInfo->GetComponentRange(0)[0] +
    arrayInfo->GetComponentRange(0)[1])/2.0;
  unsigned int num_values = vtkSMPropertyHelper(this->Contour->getProxy(),
    "ContourValues").GetNumberOfElements();
  vtkSMPropertyHelper(this->Contour->getProxy(),
    "ContourValues").Set(num_values, value);
  this->Contour->getProxy()->UpdateVTKObjects();

  this->View1->render();
  this->View2->render();
}

//-----------------------------------------------------------------------------
void qcMainWindow::clearContours()
{
  if (!this->ActiveSource)
    {
    return;
    }
  vtkSMPropertyHelper(this->Contour->getProxy(),
    "ContourValues").SetNumberOfElements(0);
  this->Contour->getProxy()->UpdateVTKObjects();
  this->View1->render();
  this->View2->render();
}
