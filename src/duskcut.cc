// duskcut.cc — OCCT-based replacement for dawncut
//
// Cut a STEP file by a half-space defined by plane n·x = d.
// Keeps the portion of the geometry where n·x <= d (same convention as dawncut).
//
// Usage:  duskcut nx ny nz d input.stp [output.stp]
//
// The cutting plane normal is (nx, ny, nz) and the signed offset is d (mm).
// Geometry on the side n·x <= d is retained (i.e. the half-space "behind" the plane).
//
// Example (same as dawncut):
//   duskcut -1 0 0 1 detector.stp cut.stp   → keeps x >= 1 mm (normal points -x)

#include "StepLoader.h"

#include <BRep_Builder.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <STEPControl_StepModelType.hxx>
#include <STEPControl_Writer.hxx>
#include <TopoDS_Shape.hxx>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
  if (argc < 6) {
    std::cerr << "Usage: duskcut nx ny nz d input.stp [output.stp]\n"
              << "\n"
              << "Cuts the STEP geometry by the half-space n·x <= d.\n"
              << "Arguments match dawncut: nx ny nz = plane normal, d = signed offset (mm).\n";
    return 1;
  }

  double nx = std::stod(argv[1]);
  double ny = std::stod(argv[2]);
  double nz = std::stod(argv[3]);
  double d  = std::stod(argv[4]);
  std::string inputFile  = argv[5];
  std::string outputFile = (argc >= 7) ? argv[6]
      : (fs::path(inputFile).stem().string() + "_cut.stp");

  std::cerr << "[duskcut] Input:  " << inputFile << "\n"
            << "[duskcut] Output: " << outputFile << "\n"
            << "[duskcut] Plane normal: (" << nx << "," << ny << "," << nz
            << ")  d=" << d << "\n";

  // Normalise the cutting plane normal
  double len = std::sqrt(nx*nx + ny*ny + nz*nz);
  if (len < 1e-12) {
    std::cerr << "Error: zero-length plane normal.\n";
    return 1;
  }
  nx /= len; ny /= len; nz /= len;
  d  /= len;

  // ------------------------------------------------------------------
  // 1. Load STEP
  // ------------------------------------------------------------------
  TopoDS_Shape shape;
  try {
    shape = loadStep(inputFile);
  } catch (const std::exception& e) {
    std::cerr << "Error loading STEP: " << e.what() << "\n";
    return 1;
  }

  // ------------------------------------------------------------------
  // 2. Build half-space solid
  //    Plane: n·x = d  (passes through point n*d, normal n)
  //    We keep the side where n·x <= d, i.e. the "behind" side.
  // ------------------------------------------------------------------
  gp_Dir  planeNorm(nx, ny, nz);
  gp_Pnt  planeOrig(nx*d, ny*d, nz*d);
  gp_Pln  cuttingPlane(planeOrig, planeNorm);

  // Make a large face on the plane
  const double faceSize = 2.0e6; // 2 km — larger than any EIC detector
  TopoDS_Face planeFace = BRepBuilderAPI_MakeFace(cuttingPlane,
                                                   -faceSize, faceSize,
                                                   -faceSize, faceSize);

  // A point on the side we KEEP (n·x < d → move opposite to normal)
  gp_Pnt keepPoint(planeOrig.X() - nx * faceSize * 0.5,
                   planeOrig.Y() - ny * faceSize * 0.5,
                   planeOrig.Z() - nz * faceSize * 0.5);

  BRepPrimAPI_MakeHalfSpace halfSpaceMaker(planeFace, keepPoint);
  if (!halfSpaceMaker.IsDone()) {
    std::cerr << "Error: failed to build half-space solid.\n";
    return 1;
  }
  TopoDS_Shape halfSpace = halfSpaceMaker.Solid();

  // ------------------------------------------------------------------
  // 3. Boolean intersection: shape ∩ half-space
  // ------------------------------------------------------------------
  std::cerr << "[duskcut] Running BRepAlgoAPI_Common...\n";
  BRepAlgoAPI_Common common(shape, halfSpace);
  common.SetRunParallel(Standard_True);
  common.Build();

  if (!common.IsDone() || common.HasErrors()) {
    std::cerr << "Error: BRepAlgoAPI_Common failed.\n";
    return 1;
  }
  TopoDS_Shape result = common.Shape();

  // ------------------------------------------------------------------
  // 4. Write output STEP
  // ------------------------------------------------------------------
  STEPControl_Writer writer;
  IFSelect_ReturnStatus status = writer.Transfer(result, STEPControl_AsIs);
  if (status != IFSelect_RetDone) {
    std::cerr << "Error: STEP transfer failed.\n";
    return 1;
  }
  status = writer.Write(outputFile.c_str());
  if (status != IFSelect_RetDone) {
    std::cerr << "Error: STEP write failed.\n";
    return 1;
  }

  std::cerr << "[duskcut] Wrote " << outputFile << "\n";
  return 0;
}
