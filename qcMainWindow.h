/*=========================================================================

   Program: ParaView
   Module:    qcMainWindow.h

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
#ifndef __QCMainWindow_h
#define __QCMainWindow_h
#include <QMainWindow>
#include <QPointer>

class pqRenderView;
class pqPipelineSource;
class pqPipelineRepresentation;
class pqRubberBandHelper;
class pqServer;

/// qcMainWindow for our application.
class qcMainWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;
public:
  qcMainWindow(QWidget *parent = 0, Qt::WindowFlags flags=0);
  ~qcMainWindow();

protected slots:
  void initialize(pqServer*);
  void cleanup();

  /// called when a new dataset is loaded.
  void onDataLoaded(pqPipelineSource*);

  /// called when the user picks a new array to contour by.
  /// We update the properties on the contour filter as well as
  /// update the coloring of the slice.
  void contourByChanged();

  void endSelection();

  /// called to clear all contours.
  void clearContours();

protected:
  /// Creates a new render view and places it in the container.
  pqRenderView* createRenderView(QWidget* container);

  QPointer<pqRenderView> View1;
  QPointer<pqRenderView> View2;
  QPointer<pqPipelineSource> ActiveSource;
  QPointer<pqPipelineSource> Slice;
  QPointer<pqPipelineSource> Contour;
  QPointer<pqPipelineSource> SliceContour;

  QPointer<pqPipelineRepresentation> ActiveSourceRepr;
  QPointer<pqPipelineRepresentation> SliceRepr;
  QPointer<pqPipelineRepresentation> ContourRepr;
  QPointer<pqPipelineRepresentation> SliceContourRepr;

  pqRubberBandHelper* SelectionHelper;
private:
  Q_DISABLE_COPY(qcMainWindow)
  class pqInternals;
  pqInternals* Ui;
};

#endif

