// duskcut.cc — OCCT-based replacement for dawncut
//
// Cut a STEP file using the same convention as dawncut.
// dawncut convention: plane  ax + by + cz + d = 0
//   - Clips (removes) the front side:  ax+by+cz+d > 0
//   - Keeps  the back  side:           ax+by+cz+d ≤ 0
//
// Usage:  duskcut a b c d input.stp [output.stp]
//
// Example (same as dawncut):
//   duskcut -1 0 0 1 detector.stp cut.stp   → keeps x >= 1 mm

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
    std::cerr << "Usage: duskcut a b c d input.stp [output.stp]\n"
              << "\n"
              << "Cuts the STEP geometry using dawncut convention: plane ax+by+cz+d=0.\n"
              << "The front side (ax+by+cz+d > 0) is clipped; the back side is kept.\n";
    return 1;
  }

  double nx;
  double ny;
  double nz;
  double d;
  try {
    nx = std::stod(argv[1]);
    ny = std::stod(argv[2]);
    nz = std::stod(argv[3]);
    d  = std::stod(argv[4]);
  } catch (const std::exception& e) {
    std::cerr << "Error parsing plane normal/offset: " << e.what() << "\n";
    return 1;
  }
  std::string inputFile = argv[5];
  std::string outputFile =
      (argc >= 7) ? argv[6] : (fs::path(inputFile).stem().string() + "_cut.stp");

  std::cerr << "[duskcut] Input:  " << inputFile << "\n"
            << "[duskcut] Output: " << outputFile << "\n"
            << "[duskcut] Plane normal: (" << nx << "," << ny << "," << nz << ")  d=" << d << "\n";

  // Normalise the cutting plane normal
  double len = std::sqrt(nx * nx + ny * ny + nz * nz);
  if (len < 1e-12) {
    std::cerr << "Error: zero-length plane normal.\n";
    return 1;
  }
  nx /= len;
  ny /= len;
  nz /= len;
  d /= len;

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
  //    dawncut convention: plane equation is  ax + by + cz + d = 0
  //    i.e. the plane passes through  p0 = -(a,b,c)*d/|n|^2
  //    and the normal points in the (a,b,c) direction.
  //    dawncut clips (removes) the front side: ax+by+cz+d > 0.
  //    We therefore KEEP the back side:        ax+by+cz+d <= 0,
  //    i.e. the half-space opposite the normal direction.
  // ------------------------------------------------------------------
  gp_Dir planeNorm(nx, ny, nz); // nx, ny, nz are already normalised above
  gp_Pnt planeOrig(-nx * d, -ny * d, -nz * d);
  gp_Pln cuttingPlane(planeOrig, planeNorm);

  // Make a large face on the plane
  const double faceSize = 2.0e6; // 2 km — larger than any EIC detector
  TopoDS_Face planeFace =
      BRepBuilderAPI_MakeFace(cuttingPlane, -faceSize, faceSize, -faceSize, faceSize);

  // A point deep on the KEEP side (opposite to the normal = dawncut back side)
  gp_Pnt keepPoint(planeOrig.X() - nx * faceSize, planeOrig.Y() - ny * faceSize,
                   planeOrig.Z() - nz * faceSize);

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
