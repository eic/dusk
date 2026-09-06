#pragma once
// SVGWriter.h — Write 2D polylines as SVG paths

#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

struct Point2D { double x, y; };
using Polyline = std::vector<Point2D>;

class SVGWriter {
public:
  /// @param filename   Output SVG path
  /// @param width_mm   Paper width  in mm (used for SVG viewBox)
  /// @param height_mm  Paper height in mm
  SVGWriter(const std::string& filename, double width_mm = 297.0, double height_mm = 210.0)
      : m_width(width_mm), m_height(height_mm) {
    m_out.open(filename);
    if (!m_out.is_open())
      throw std::runtime_error("Cannot open SVG output: " + filename);
  }

  /// Write SVG header — call once before any paths.
  void writeHeader() {
    m_out << R"(<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg"
     width=")" << m_width << R"(mm" height=")" << m_height << R"(mm"
     viewBox="0 0 )" << m_width << " " << m_height << R"(">
<rect width="100%" height="100%" fill="white"/>
)";
  }

  /// Write a group of visible (solid) polylines.
  void beginVisibleGroup(double stroke_width = 0.2) {
    m_out << "<g stroke=\"black\" stroke-width=\"" << stroke_width
          << "\" fill=\"none\" stroke-linecap=\"round\" stroke-linejoin=\"round\">\n";
  }

  /// Write a group of hidden (dashed) polylines.
  void beginHiddenGroup(double stroke_width = 0.1) {
    m_out << "<g stroke=\"#aaaaaa\" stroke-width=\"" << stroke_width
          << "\" fill=\"none\" stroke-dasharray=\"1,1\">\n";
  }

  void endGroup() { m_out << "</g>\n"; }

  /// Append a single polyline as an SVG <path> element.
  /// @param pts     2D points in model coordinates
  /// @param scale   scale factor (magnification * mm_per_unit)
  /// @param cx      SVG canvas centre x (mm)
  /// @param cy      SVG canvas centre y (mm)
  void writePath(const Polyline& pts, double scale, double cx, double cy) {
    if (pts.size() < 2) return;
    m_out << "<path d=\"";
    bool first = true;
    for (const auto& p : pts) {
      // Model Y is up; SVG Y is down → flip sign of y
      double sx = cx + p.x * scale;
      double sy = cy - p.y * scale;
      if (first) {
        m_out << "M " << sx << " " << sy;
        first = false;
      } else {
        m_out << " L " << sx << " " << sy;
      }
    }
    m_out << "\"/>\n";
  }

  void writeFooter() {
    m_out << "</svg>\n";
  }

  bool isOpen() const { return m_out.is_open(); }

private:
  std::ofstream m_out;
  double m_width, m_height;
};
