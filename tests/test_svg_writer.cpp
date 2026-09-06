#include "SVGWriter.h"
#include "test_file_utils.h"

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <string>

TEST_CASE("SVGWriter writes SVG header, path, and footer") {
  const auto path = testutils::uniqueTempPath("dusk_svg_writer_test", ".svg");

  {
    SVGWriter writer(path.string(), 100.0, 50.0);
    writer.writeHeader();
    writer.beginVisibleGroup(0.2);
    Polyline polyline = {{0.0, 0.0}, {2.0, 1.0}, {4.0, -1.0}};
    writer.writePath(polyline, 5.0, 10.0, 20.0);
    writer.endGroup();
    writer.writeFooter();
  }

  std::ifstream in(path);
  REQUIRE(in.is_open());
  const std::string content((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());

  REQUIRE(content.find("<?xml version=\"1.0\" encoding=\"UTF-8\"?>") != std::string::npos);
  REQUIRE(content.find("viewBox=\"0 0 100 50\"") != std::string::npos);
  REQUIRE(content.find("<g stroke=\"black\" stroke-width=\"0.2\"") != std::string::npos);
  REQUIRE(content.find("<path d=\"M 10 20 L 20 15 L 30 25\"/>") != std::string::npos);
  REQUIRE(content.find("</svg>") != std::string::npos);

  testutils::removeNoThrow(path);
}

TEST_CASE("SVGWriter ignores degenerate polylines") {
  const auto path = testutils::uniqueTempPath("dusk_svg_writer_degenerate_test", ".svg");

  {
    SVGWriter writer(path.string(), 10.0, 10.0);
    writer.writeHeader();
    writer.beginVisibleGroup();
    Polyline polyline = {{1.0, 2.0}};
    writer.writePath(polyline, 1.0, 0.0, 0.0);
    writer.endGroup();
    writer.writeFooter();
  }

  std::ifstream in(path);
  REQUIRE(in.is_open());
  const std::string content((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());

  REQUIRE(content.find("<path d=\"") == std::string::npos);

  testutils::removeNoThrow(path);
}
