#include "viewport.h"
#include <pangolin/handler/handler.h>
#include <stdexcept>

namespace viewportal {

// Forward declarations of factory functions from individual viewport files
std::unique_ptr<Viewport> createRgb8Viewport(const std::string& name, float aspect_ratio);
std::unique_ptr<Viewport> createG8Viewport(const std::string& name, float aspect_ratio, int width, int height);
std::unique_ptr<Viewport> createReconstructionViewport(const std::string& name, float aspect_ratio,
                                                        const pangolin::OpenGlRenderState& render_state);
std::unique_ptr<Viewport> createPlotViewport(const std::string& name, float aspect_ratio);

namespace {
const int kDefaultWidth = 320;
const int kDefaultHeight = 240;
pangolin::OpenGlMatrix getDefaultProj() {
    static pangolin::OpenGlMatrix proj = pangolin::ProjectionMatrix(640, 480, 420, 420, 320, 240, 0.1, 1000);
    return proj;
}
} // namespace

std::unique_ptr<Viewport> createViewport(const std::string& type,
                                         const std::string& name,
                                         float aspect_ratio) {
    pangolin::OpenGlMatrix proj = getDefaultProj();

    if (type == "rgb8") {
        return createRgb8Viewport(name, aspect_ratio);
    }
    if (type == "g8") {
        return createG8Viewport(name, aspect_ratio, kDefaultWidth, kDefaultHeight);
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

std::unique_ptr<Viewport> createViewport(ViewportType type,
                                         const std::string& name,
                                         float aspect_ratio) {
    switch (type) {
    case ViewportType::RGB8:
        return createRgb8Viewport(name, aspect_ratio);
    case ViewportType::G8:
        return createG8Viewport(name, aspect_ratio, kDefaultWidth, kDefaultHeight);
    case ViewportType::Reconstruction: {
        pangolin::OpenGlRenderState render_state(
            getDefaultProj(),
            pangolin::ModelViewLookAt(0, 0.5, -3, 0, 0, 0, pangolin::AxisY)
        );
        return createReconstructionViewport(name, aspect_ratio, render_state);
    }
    case ViewportType::Plot:
        return createPlotViewport(name, aspect_ratio);
    default:
        throw std::runtime_error("Unknown ViewportType");
    }
}

} // namespace viewportal
