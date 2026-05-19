#pragma once
// HistoryParser.h — Parse .DAWN_1.history files and override via CLI flags
//
// .DAWN_1.history line map (from dawn_tweak analysis):
//   Line  1: camera distance / scale (float)
//   Line  2: phi   — azimuth angle (degrees)    [dawn_tweak --phi]
//   Line  3: theta — elevation angle (degrees)  [dawn_tweak --theta]
//   Line  4: (unused)
//   Line  5: target_x                           [dawn_tweak -x]
//   Line  6: target_y                           [dawn_tweak -y]
//   Line  7: target_z                           [dawn_tweak -z]
//   Line  8: magnification                      [dawn_tweak --mag]
//   Line  9: drawing mode (1=wireframe, 2=+hidden, 3=outline)  [dawn_tweak --draw]
//   Line 10: epsilon3d
//   Lines 11-17: lighting / colour params
//   Line 18: light_phi                          [dawn_tweak --light-phi]
//   Line 19: light_theta                        [dawn_tweak --light-theta]

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct ViewParams {
  double phi{90.0};        // azimuth  [deg]
  double theta{90.0};      // elevation [deg]
  double target_x{0.0};
  double target_y{0.0};
  double target_z{0.0};
  double magnification{1.0};
  int    draw_mode{1};     // 1=wireframe, 2=+hidden, 3=outline
  double light_phi{90.0};
  double light_theta{180.0};
};

/// Read a .DAWN_1.history file.  Missing or unreadable lines keep defaults.
inline ViewParams parseHistory(const std::string& filename) {
  ViewParams p;
  std::ifstream f(filename);
  if (!f.is_open()) {
    std::cerr << "[dusk] No history file '" << filename << "', using defaults\n";
    return p;
  }

  std::vector<std::string> lines;
  std::string line;
  while (std::getline(f, line)) {
    lines.push_back(line);
  }

  auto readDouble = [&](int lineIdx, double& out) {
    if (lineIdx >= 0 && lineIdx < static_cast<int>(lines.size())) {
      std::istringstream ss(lines[lineIdx]);
      double v;
      if (ss >> v) out = v;
    }
  };
  auto readInt = [&](int lineIdx, int& out) {
    if (lineIdx >= 0 && lineIdx < static_cast<int>(lines.size())) {
      std::istringstream ss(lines[lineIdx]);
      int v;
      if (ss >> v) out = v;
    }
  };

  // line indices are 0-based; dawn_tweak uses 1-based sed line numbers
  readDouble(1, p.phi);          // line 2
  readDouble(2, p.theta);        // line 3
  readDouble(4, p.target_x);    // line 5
  readDouble(5, p.target_y);    // line 6
  readDouble(6, p.target_z);    // line 7
  readDouble(7, p.magnification); // line 8
  readInt   (8, p.draw_mode);   // line 9
  readDouble(17, p.light_phi);  // line 18
  readDouble(18, p.light_theta); // line 19

  return p;
}

/// Parse dusk / dawn CLI arguments, applying overrides on top of a ViewParams base.
/// Recognised flags: --theta --phi --mag --draw -x -y -z --light-theta --light-phi
/// Returns remaining (positional) arguments.
inline std::vector<std::string> applyCliArgs(int argc, char** argv,
                                              ViewParams& p,
                                              std::string& inputFile,
                                              std::string& historyFile) {
  std::vector<std::string> positional;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto nextArg = [&]() -> std::string {
      if (i + 1 < argc) return argv[++i];
      std::cerr << "[dusk] Missing value for " << arg << "\n";
      return "";
    };
    if (arg == "-d" || arg == "--input") {
      inputFile = nextArg();
    } else if (arg == "--history") {
      historyFile = nextArg();
    } else if (arg == "--theta") {
      p.theta = std::stod(nextArg());
    } else if (arg == "--phi") {
      p.phi = std::stod(nextArg());
    } else if (arg == "--mag") {
      p.magnification = std::stod(nextArg());
    } else if (arg == "--draw") {
      p.draw_mode = std::stoi(nextArg());
    } else if (arg == "-x") {
      p.target_x = std::stod(nextArg());
    } else if (arg == "-y") {
      p.target_y = std::stod(nextArg());
    } else if (arg == "-z") {
      p.target_z = std::stod(nextArg());
    } else if (arg == "--light-theta") {
      p.light_theta = std::stod(nextArg());
    } else if (arg == "--light-phi") {
      p.light_phi = std::stod(nextArg());
    } else {
      positional.push_back(arg);
    }
  }
  return positional;
}

/// Convert theta/phi (degrees) to a unit eye-direction vector.
inline void thetaPhiToDir(double theta_deg, double phi_deg,
                           double& dx, double& dy, double& dz) {
  const double deg2rad = M_PI / 180.0;
  double th = theta_deg * deg2rad;
  double ph = phi_deg   * deg2rad;
  dx = std::sin(th) * std::cos(ph);
  dy = std::sin(th) * std::sin(ph);
  dz = std::cos(th);
}
