#include "HistoryParser.h"
#include "test_file_utils.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <string>
#include <vector>

TEST_CASE("parseHistory uses defaults when file is missing") {
  const auto path = testutils::uniqueTempPath("dusk_missing_history_test", ".txt");
  testutils::removeNoThrow(path);

  ViewParams p = parseHistory(path.string());
  REQUIRE(p.phi == Catch::Approx(90.0));
  REQUIRE(p.theta == Catch::Approx(90.0));
  REQUIRE(p.target_x == Catch::Approx(0.0));
  REQUIRE(p.target_y == Catch::Approx(0.0));
  REQUIRE(p.target_z == Catch::Approx(0.0));
  REQUIRE(p.magnification == Catch::Approx(1.0));
  REQUIRE(p.draw_mode == 1);
  REQUIRE(p.light_phi == Catch::Approx(90.0));
  REQUIRE(p.light_theta == Catch::Approx(180.0));
}

TEST_CASE("parseHistory reads mapped lines from history file") {
  const auto path = testutils::uniqueTempPath("dusk_history_test", ".txt");
  std::vector<std::string> lines(19, "0");
  lines[1]  = "12.5";
  lines[2]  = "33.0";
  lines[4]  = "1.25";
  lines[5]  = "-2.5";
  lines[6]  = "3.75";
  lines[7]  = "2.2";
  lines[8]  = "3";
  lines[17] = "45.0";
  lines[18] = "135.0";

  std::ofstream out(path);
  for (const auto& line : lines) {
    out << line << "\n";
  }
  out.close();

  ViewParams p = parseHistory(path.string());
  REQUIRE(p.phi == Catch::Approx(12.5));
  REQUIRE(p.theta == Catch::Approx(33.0));
  REQUIRE(p.target_x == Catch::Approx(1.25));
  REQUIRE(p.target_y == Catch::Approx(-2.5));
  REQUIRE(p.target_z == Catch::Approx(3.75));
  REQUIRE(p.magnification == Catch::Approx(2.2));
  REQUIRE(p.draw_mode == 3);
  REQUIRE(p.light_phi == Catch::Approx(45.0));
  REQUIRE(p.light_theta == Catch::Approx(135.0));

  testutils::removeNoThrow(path);
}

TEST_CASE("applyCliArgs applies command-line overrides and positional args") {
  ViewParams p;
  std::string inputFile;
  std::string historyFile = ".DAWN_1.history";

  std::vector<std::string> args = {"dusk",
                                   "-d",
                                   "detector.stp",
                                   "--history",
                                   "custom.history",
                                   "--theta",
                                   "10",
                                   "--phi",
                                   "20",
                                   "--mag",
                                   "1.5",
                                   "--draw",
                                   "2",
                                   "-x",
                                   "3.1",
                                   "-y",
                                   "-4.2",
                                   "-z",
                                   "5.3",
                                   "--light-theta",
                                   "140",
                                   "--light-phi",
                                   "60",
                                   "--mesh-deflection",
                                   "0.7",
                                   "--mesh-ang-deflection",
                                   "0.25",
                                   "--parallel",
                                   "positional_a",
                                   "positional_b"};
  std::vector<char*> cargs;
  cargs.reserve(args.size());
  for (auto& s : args) {
    cargs.push_back(s.data());
  }

  auto positional =
      applyCliArgs(static_cast<int>(cargs.size()), cargs.data(), p, inputFile, historyFile);

  REQUIRE(inputFile == "detector.stp");
  REQUIRE(historyFile == "custom.history");
  REQUIRE(p.theta == Catch::Approx(10.0));
  REQUIRE(p.phi == Catch::Approx(20.0));
  REQUIRE(p.magnification == Catch::Approx(1.5));
  REQUIRE(p.draw_mode == 2);
  REQUIRE(p.target_x == Catch::Approx(3.1));
  REQUIRE(p.target_y == Catch::Approx(-4.2));
  REQUIRE(p.target_z == Catch::Approx(5.3));
  REQUIRE(p.light_theta == Catch::Approx(140.0));
  REQUIRE(p.light_phi == Catch::Approx(60.0));
  REQUIRE(p.mesh_deflection == Catch::Approx(0.7));
  REQUIRE(p.mesh_ang_deflection == Catch::Approx(0.25));
  REQUIRE(p.mesh_parallel);
  REQUIRE(positional.size() == 2);
  REQUIRE(positional[0] == "positional_a");
  REQUIRE(positional[1] == "positional_b");
}

TEST_CASE("thetaPhiToDir converts spherical angles to direction vector") {
  double dx = 0.0;
  double dy = 0.0;
  double dz = 0.0;

  thetaPhiToDir(90.0, 0.0, dx, dy, dz);
  REQUIRE(dx == Catch::Approx(1.0).margin(1e-12));
  REQUIRE(dy == Catch::Approx(0.0).margin(1e-12));
  REQUIRE(dz == Catch::Approx(0.0).margin(1e-12));

  thetaPhiToDir(90.0, 90.0, dx, dy, dz);
  REQUIRE(dx == Catch::Approx(0.0).margin(1e-12));
  REQUIRE(dy == Catch::Approx(1.0).margin(1e-12));
  REQUIRE(dz == Catch::Approx(0.0).margin(1e-12));

  thetaPhiToDir(0.0, 10.0, dx, dy, dz);
  REQUIRE(dx == Catch::Approx(0.0).margin(1e-12));
  REQUIRE(dy == Catch::Approx(0.0).margin(1e-12));
  REQUIRE(dz == Catch::Approx(1.0).margin(1e-12));
}
