#pragma once
// StepLoader.h — Load a STEP file into a compound TopoDS_Shape using OCCT 7.x

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XSControl_WorkSession.hxx>

#include <iostream>
#include <stdexcept>
#include <string>

/// Load a STEP file and return a single compound shape containing all free shapes.
inline TopoDS_Shape loadStep(const std::string& filename) {
  // Create an XCAF document
  Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
  Handle(TDocStd_Document) doc;
  app->NewDocument("MDTV-XCAF", doc);

  // Read the STEP file
  STEPCAFControl_Reader reader;
  reader.SetColorMode(Standard_True);
  reader.SetNameMode(Standard_True);
  reader.SetLayerMode(Standard_True);

  IFSelect_ReturnStatus status = reader.ReadFile(filename.c_str());
  if (status != IFSelect_RetDone) {
    throw std::runtime_error("Failed to read STEP file: " + filename);
  }

  if (!reader.Transfer(doc)) {
    throw std::runtime_error("Failed to transfer STEP data: " + filename);
  }

  // Collect all free (top-level) shapes
  Handle(XCAFDoc_ShapeTool) shapeTool =
      XCAFDoc_DocumentTool::ShapeTool(doc->Main());

  TDF_LabelSequence freeLabels;
  shapeTool->GetFreeShapes(freeLabels);

  if (freeLabels.IsEmpty()) {
    throw std::runtime_error("No shapes found in STEP file: " + filename);
  }

  std::cerr << "[dusk] Loaded " << freeLabels.Size() << " top-level shape(s) from "
            << filename << std::endl;

  // Build a compound from all free shapes
  BRep_Builder builder;
  TopoDS_Compound compound;
  builder.MakeCompound(compound);
  for (Standard_Integer i = 1; i <= freeLabels.Size(); ++i) {
    TopoDS_Shape shape = shapeTool->GetShape(freeLabels.Value(i));
    if (!shape.IsNull()) {
      builder.Add(compound, shape);
    }
  }

  return compound;
}
