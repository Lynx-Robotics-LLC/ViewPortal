#include "viewport.h"
#include <pangolin/handler/handler.h>
#include <stdexcept>

namespace viewportal {

// Forward declarations of factory functions from individual viewport files
std::unique_ptr<Viewport> createColorCameraViewport(const std::string& name, float aspect_ratio);
std::unique_ptr<Viewport> createDepthCameraViewport(const std::string& name, float aspect_ratio, int width, int height);
std::unique_ptr<Viewport> createReconstructionViewport(const std::string& name, float aspect_ratio,
                                                        const pangolin::OpenGlRenderState& render_state);
std::unique_ptr<Viewport> createPlotViewport(const std::string& name, float aspect_ratio);

std::unique_ptr<Viewport> createViewport(const std::string& type,
                                         const std::string& name,
                                         float aspect_ratio) {
    static pangolin::OpenGlMatrix proj = pangolin::ProjectionMatrix(640, 480, 420, 420, 320, 240, 0.1, 1000);
    static const int kDefaultWidth = 320;
    static const int kDefaultHeight = 240;

    if (type == "color_camera" || type == "color") {
        return createColorCameraViewport(name, aspect_ratio);
    }
    if (type == "depth_camera" || type == "depth") {
        return createDepthCameraViewport(name, aspect_ratio, kDefaultWidth, kDefaultHeight);
    }
    if (type == "reconstruction" || type == "recon") {
        pangolin::OpenGlRenderState render_state(
            proj,
            pangolin::ModelViewLookAt(0, 0.5, -3, 0, 0, 0, pangolin::AxisY)
        );
        return createReconstructionViewport(name, aspect_ratio, render_state);
    }
    if (type == "plot") {
        return createPlotViewport(name, aspect_ratio);
    }
    throw std::runtime_error("Unknown viewport type: " + type);
}

} // namespace viewportal
