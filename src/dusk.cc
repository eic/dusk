// dusk.cc — OCCT-based replacement for dawn
//
// Usage:  dusk -d file.stp [options]
//
// Options (override .DAWN_1.history values):
//   --theta <deg>     elevation angle (default: 90)
//   --phi   <deg>     azimuth angle   (default: 90)
//   --mag   <float>   magnification   (default: 1.0)
//   --draw  <int>     drawing mode 1=wireframe 2=+hidden 3=outline (default: 1)
//   -x/-y/-z <float>  view target     (default: 0 0 0)
//   --history <file>  history file    (default: .DAWN_1.history)
//   -o <file>         output SVG      (default: <stem>.svg)
//
// The program reads .DAWN_1.history in the current directory (same as dawn)
// and then applies any CLI overrides on top.

#include "HistoryParser.h"
#include "StepLoader.h"
#include "SVGWriter.h"

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBndLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <Bnd_Box.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_PolyAlgo.hxx>
#include <HLRBRep_PolyHLRToShape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Discretise a projected edge (from HLR PolyHLRToShape) into 2D points.
// The HLR output edges are already in view-plane coordinates (X, Y are 2D).
static Polyline discretiseEdge(const TopoDS_Edge& edge, double deflection = 0.5) {
  Polyline pts;
  BRepAdaptor_Curve curve(edge);
  GCPnts_QuasiUniformDeflection disc(curve, deflection);
  if (!disc.IsDone()) {
    return pts;
  }
  for (int i = 1; i <= disc.NbPoints(); ++i) {
    gp_Pnt p = disc.Value(i);
    pts.push_back({p.X(), p.Y()});
  }
  return pts;
}

// Walk all edges in a compound shape and collect 2D polylines.
static std::vector<Polyline> extractPolylines(const TopoDS_Shape& compound, double deflection) {
  std::vector<Polyline> result;
  for (TopExp_Explorer ex(compound, TopAbs_EDGE); ex.More(); ex.Next()) {
    Polyline pl = discretiseEdge(TopoDS::Edge(ex.Current()), deflection);
    if (pl.size() >= 2) {
      result.push_back(std::move(pl));
    }
  }
  return result;
}

// Compute bounding box of all polylines.
static void polylineBounds(const std::vector<Polyline>& pls, double& xmin, double& xmax,
                           double& ymin, double& ymax) {
  xmin = ymin = std::numeric_limits<double>::max();
  xmax = ymax = -std::numeric_limits<double>::max();
  for (const auto& pl : pls) {
    for (const auto& p : pl) {
      xmin = std::min(xmin, p.x);
      xmax = std::max(xmax, p.x);
      ymin = std::min(ymin, p.y);
      ymax = std::max(ymax, p.y);
    }
  }
}

int main(int argc, char** argv) {
  try {
    if (argc < 2) {
      std::cerr << "Usage: dusk -d file.stp [--theta deg] [--phi deg] "
                   "[--mag f] [--draw 1|2|3] [-x f] [-y f] [-z f] "
                   "[--mesh-deflection mm] [--mesh-ang-deflection rad] [--parallel] "
                   "[--history file] [-o output.svg]\n";
      return 1;
    }

    // Defaults
    std::string inputFile;
    std::string historyFile = ".DAWN_1.history";
    std::string outputFile;

    // Parse history file first, then CLI overrides
    ViewParams params = parseHistory(historyFile);

    // Extract -o / --output before applyCliArgs (it doesn't know about -o)
    std::vector<std::string> filtered;
    filtered.reserve(static_cast<size_t>(argc - 1)); // Reserve to avoid reallocations
    for (int i = 1; i < argc; ++i) {
      std::string arg(argv[i]);
      if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
        outputFile = argv[++i];
      } else {
        filtered.push_back(arg);
      }
    }
    // Rebuild argc/argv-like structure for applyCliArgs
    std::vector<char*> fargv;
    fargv.push_back(argv[0]);
    for (auto& s : filtered) {
      fargv.push_back(const_cast<char*>(s.c_str()));
    }

    applyCliArgs(static_cast<int>(fargv.size()), fargv.data(), params, inputFile, historyFile);

    // Re-parse history if --history was given on CLI
    if (historyFile != ".DAWN_1.history") {
      params = parseHistory(historyFile);
      applyCliArgs(static_cast<int>(fargv.size()), fargv.data(), params, inputFile, historyFile);
    }

    if (inputFile.empty()) {
      std::cerr << "Error: no input file specified. Use -d file.stp\n";
      return 1;
    }

    // Default output file: replace extension with .svg
    if (outputFile.empty()) {
      fs::path p(inputFile);
      outputFile = (p.parent_path() / p.stem()).string() + ".svg";
    }

    std::cerr << "[dusk] Input:  " << inputFile << "\n"
              << "[dusk] Output: " << outputFile << "\n"
              << "[dusk] theta=" << params.theta << " phi=" << params.phi
              << " mag=" << params.magnification << " draw=" << params.draw_mode << "\n"
              << "[dusk] target=(" << params.target_x << "," << params.target_y << ","
              << params.target_z << ")\n";

    // ------------------------------------------------------------------
    // 1. Load STEP
    // ------------------------------------------------------------------
    TopoDS_Shape shape;
    // Inner try-catch provides a more specific error message for STEP loading failures,
    // distinguishing them from other errors that might occur in the program.
    try {
      shape = loadStep(inputFile);
    } catch (const std::exception& e) {
      std::cerr << "Error loading STEP: " << e.what() << "\n";
      return 1;
    }

    // ------------------------------------------------------------------
    // 1b. Tessellate for PolyAlgo (required before HLR)
    // ------------------------------------------------------------------
    std::cerr << "[dusk] Meshing geometry (deflection=" << params.mesh_deflection << "mm)...\n";
    BRepMesh_IncrementalMesh mesher(shape, params.mesh_deflection, /*isRelative=*/false,
                                    params.mesh_ang_deflection, params.mesh_parallel);
    mesher.Perform();

    // ------------------------------------------------------------------
    // 2. Set up camera (eye direction from theta/phi)
    // ------------------------------------------------------------------
    double edx = 0.0;
    double edy = 0.0;
    double edz = 0.0;
    thetaPhiToDir(params.theta, params.phi, edx, edy, edz);

    gp_Dir eyeDir(edx, edy, edz);
    gp_Pnt target(params.target_x, params.target_y, params.target_z);

    // Up vector: prefer world Z; if eye is along Z, fall back to Y
    gp_Dir upDir(0, 0, 1);
    if (std::abs(eyeDir.Dot(upDir)) > 0.999) {
      upDir = gp_Dir(0, 1, 0);
    }
    // Make up perpendicular to eye
    gp_Vec upVec(upDir);
    gp_Vec eyeVec(eyeDir);
    upVec = upVec - eyeVec * eyeVec.Dot(upVec);
    if (upVec.Magnitude() < 1e-9) {
      upVec = gp_Vec(0, 1, 0);
    }
    upDir = gp_Dir(upVec);

    gp_Ax2 cameraAx(target, eyeDir, upDir);
    HLRAlgo_Projector projector(cameraAx); // orthographic

    // ------------------------------------------------------------------
    // 3. Hidden Line Removal
    // ------------------------------------------------------------------
    std::cerr << "[dusk] Running HLR (PolyAlgo)...\n";
    Handle(HLRBRep_PolyAlgo) hlrAlgo = new HLRBRep_PolyAlgo;
    hlrAlgo->Projector(projector);
    hlrAlgo->Load(shape);
    hlrAlgo->Update();

    HLRBRep_PolyHLRToShape hlrShape;
    hlrShape.Update(hlrAlgo);

    TopoDS_Shape visEdges = hlrShape.VCompound(); // visible
    TopoDS_Shape hidEdges = hlrShape.HCompound(); // hidden

    // ------------------------------------------------------------------
    // 4. Extract 2D polylines (HLR output is already in view-plane coords)
    // ------------------------------------------------------------------
    const double deflection = 0.5; // mm; adjust for quality vs speed

    std::vector<Polyline> visPoly = extractPolylines(visEdges, deflection);
    std::vector<Polyline> hidPoly;
    if (params.draw_mode >= 2 && !hidEdges.IsNull()) {
      hidPoly = extractPolylines(hidEdges, deflection);
    }

    std::cerr << "[dusk] Visible polylines: " << visPoly.size() << "  Hidden: " << hidPoly.size()
              << "\n";

    // ------------------------------------------------------------------
    // 5. Compute bounding box and scale
    // ------------------------------------------------------------------
    std::vector<Polyline> allPoly = visPoly;
    allPoly.insert(allPoly.end(), hidPoly.begin(), hidPoly.end());

    if (allPoly.empty()) {
      std::cerr << "[dusk] Warning: no edges to draw.\n";
    }

    double xmin = 0.0;
    double xmax = 0.0;
    double ymin = 0.0;
    double ymax = 0.0;
    polylineBounds(allPoly, xmin, xmax, ymin, ymax);

    const double margin_mm = 10.0;
    const double page_w    = 297.0;
    const double page_h    = 210.0;
    double draw_w          = page_w - 2 * margin_mm;
    double draw_h          = page_h - 2 * margin_mm;

    double model_w = (xmax - xmin);
    double model_h = (ymax - ymin);
    if (model_w < 1e-9) {
      model_w = 1.0;
    }
    if (model_h < 1e-9) {
      model_h = 1.0;
    }

    // Scale: fit to page, then apply magnification factor
    double auto_scale = std::min(draw_w / model_w, draw_h / model_h);
    double scale      = auto_scale * params.magnification;

    // Canvas centre
    double cx = page_w / 2.0;
    double cy = page_h / 2.0;
    // Offset for target point
    double tcx = cx - ((xmin + xmax) / 2.0) * scale;
    double tcy = cy + ((ymin + ymax) / 2.0) * scale;

    // ------------------------------------------------------------------
    // 6. Write SVG
    // ------------------------------------------------------------------
    SVGWriter svg(outputFile, page_w, page_h);
    svg.writeHeader();

    double vis_stroke = (params.draw_mode == 3) ? 0.3 : 0.2;

    svg.beginVisibleGroup(vis_stroke);
    for (const auto& pl : visPoly) {
      svg.writePath(pl, scale, tcx, tcy);
    }
    svg.endGroup();

    if (!hidPoly.empty()) {
      svg.beginHiddenGroup(0.1);
      for (const auto& pl : hidPoly) {
        svg.writePath(pl, scale, tcx, tcy);
      }
      svg.endGroup();
    }

    svg.writeFooter();
    std::cerr << "[dusk] Wrote " << outputFile << "\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  } catch (...) {
    std::cerr << "Error: unknown exception (non-standard type).\n";
    return 1;
  }
}
