/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

// semantic relations plugin
#include "QmitkDataNodeSetControlPointAction.h"
#include "QmitkDataNodeRemoveFromSemanticRelationsAction.h"

// semantic relations module
#include <mitkControlPointManager.h>
#include <mitkDICOMHelper.h>
#include <mitkSemanticRelationsDataStorageAccess.h>
#include <mitkSemanticRelationException.h>
#include <mitkSemanticRelationsInference.h>
#include <mitkUIDGeneratorBoost.h>

// semantic relations UI module
#include "QmitkControlPointDialog.h"

// qt
#include <QInputDialog>
#include <QMessageBox>

QmitkDataNodeSetControlPointAction::QmitkDataNodeSetControlPointAction(QWidget* parent, berry::IWorkbenchPartSite::Pointer workbenchPartSite)
  : QAction(parent)
  , QmitkAbstractSemanticRelationsAction(workbenchPartSite)
{
  setText(tr("Set control point"));
  m_Parent = parent;
  InitializeAction();
}

QmitkDataNodeSetControlPointAction::QmitkDataNodeSetControlPointAction(QWidget* parent, berry::IWorkbenchPartSite* workbenchPartSite)
  : QAction(parent)
  , QmitkAbstractSemanticRelationsAction(berry::IWorkbenchPartSite::Pointer(workbenchPartSite))
{
  setText(tr("Set control point"));
  m_Parent = parent;
  InitializeAction();
}

QmitkDataNodeSetControlPointAction::~QmitkDataNodeSetControlPointAction()
{
  // nothing here
}

void QmitkDataNodeSetControlPointAction::InitializeAction()
{
  connect(this, &QAction::triggered, this, &QmitkDataNodeSetControlPointAction::OnActionTriggered);
}

void QmitkDataNodeSetControlPointAction::OnActionTriggered(bool /*checked*/)
{
  if (nullptr == m_SemanticRelationsIntegration)
  {
    return;
  }

  if (m_DataStorage.IsExpired())
  {
    return;
  }

  auto dataNode = GetSelectedNode();
  if (dataNode.IsNull())
  {
    return;
  }

  QmitkControlPointDialog* inputDialog = new QmitkControlPointDialog(m_Parent);
  inputDialog->setWindowTitle("Set control point");
  inputDialog->SetCurrentDate(mitk::SemanticRelationsInference::GetControlPointOfImage(dataNode));

  int dialogReturnValue = inputDialog->exec();
  if (QDialog::Rejected == dialogReturnValue)
  {
    return;
  }

  const QDate& userSelectedDate = inputDialog->GetCurrentDate();
  mitk::SemanticTypes::ControlPoint controlPoint;
  controlPoint.UID = mitk::UIDGeneratorBoost::GenerateUID();
  controlPoint.date = boost::gregorian::date(userSelectedDate.year(),
                                             userSelectedDate.month(),
                                             userSelectedDate.day());

  mitk::SemanticTypes::InformationType informationType;
  mitk::SemanticTypes::ExaminationPeriod examinationPeriod;
  mitk::SemanticRelationsDataStorageAccess::DataNodeVector allSpecificImages;
  try
  {
    mitk::SemanticTypes::CaseID caseID = mitk::GetCaseIDFromDataNode(dataNode);
    informationType = mitk::SemanticRelationsInference::GetInformationTypeOfImage(dataNode);
    // see if the examination period - information type cell is already taken
    examinationPeriod = mitk::FindFittingExaminationPeriod(caseID, controlPoint);
    auto semanticRelationsDataStorageAccess = mitk::SemanticRelationsDataStorageAccess(m_DataStorage.Lock());
    try
    {
      allSpecificImages = semanticRelationsDataStorageAccess.GetAllSpecificImages(caseID, informationType, examinationPeriod);
    }
    catch (const mitk::SemanticRelationException&)
    {
      // just continue since an exception means that there is no specific image
    }
  }
  catch (const mitk::SemanticRelationException& e)
  {
    std::stringstream exceptionMessage; exceptionMessage << e;
    QMessageBox msgBox(QMessageBox::Warning,
      "Could not set the control point.",
      "The program wasn't able to correctly set the control point.\n"
      "Reason:\n" + QString::fromStdString(exceptionMessage.str() + "\n"));
    msgBox.exec();
  }

  if (!allSpecificImages.empty())
  {
    // examination period - information type cell is already taken
    // ask if cell should be overwritten
    QMessageBox::StandardButton answerButton =
      QMessageBox::question(nullptr,
        "Specific image already exists.",
        QString::fromStdString("Force overwriting existing image " + informationType + " at " + examinationPeriod.name + "?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (answerButton == QMessageBox::Yes)
    {
      try
      {
        // remove already existent images at specific cell
        for (const auto& specificImage : allSpecificImages)
        {
          RemoveFromSemanticRelationsAction::Run(m_SemanticRelationsIntegration.get(), m_DataStorage.Lock(), specificImage);
        }
      }
      catch (const mitk::SemanticRelationException& e)
      {
        std::stringstream exceptionMessage; exceptionMessage << e;
        QMessageBox msgBox(QMessageBox::Warning,
          "Could not set the control point.",
          "The program wasn't able to correctly set the control point.\n"
          "Reason:\n" + QString::fromStdString(exceptionMessage.str() + "\n"));
        msgBox.exec();
      }
    }
    else
    {
      // else case is: no overwriting
      return;
    }
  }

  // specific image does not exist or has been removed; setting the control point should work
  try
  {
    m_SemanticRelationsIntegration->UnlinkImageFromControlPoint(dataNode);
    m_SemanticRelationsIntegration->SetControlPointOfImage(dataNode, controlPoint);
  }
  catch (const mitk::SemanticRelationException& e)
  {
    std::stringstream exceptionMessage; exceptionMessage << e;
    QMessageBox msgBox(QMessageBox::Warning,
      "Could not set the control point.",
      "The program wasn't able to correctly set the control point.\n"
      "Reason:\n" + QString::fromStdString(exceptionMessage.str() + "\n"));
    msgBox.exec();
  }
}
