/*
 * Standalone ViewPortal sample using the ViewPortal library (FetchContent).
 * 2x2 grid with RGB8, G8, Reconstruction, and Plot.
 * Frame capture (OpenCV camera or synthetic) is separated into capture functions;
 * the GUI just receives the captured frame data.
 */

#include "viewportal.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <cstring>

namespace {

using namespace viewportal;

constexpr int kColorWidth = 320;
constexpr int kColorHeight = 240;
constexpr int kDepthWidth = 320;
constexpr int kDepthHeight = 240;

struct ViewportalCapture {
    std::vector<unsigned char> color_buffer;
    std::vector<unsigned char> depth_buffer;

    FrameData color_rgb;
    FrameData depth_g8;

    ViewportalCapture() {
        color_buffer.resize(3 * kColorWidth * kColorHeight);
        depth_buffer.resize(kDepthWidth * kDepthHeight);
    }
};

bool captureFrames(cv::VideoCapture* cap, ViewportalCapture& out) {
    out.color_rgb.data = nullptr;
    out.depth_g8.data = nullptr;

    out.color_rgb.width = kColorWidth;
    out.color_rgb.height = kColorHeight;
    out.color_rgb.format = ImageFormat::RGB8;
    out.color_rgb.row_stride = 0;

    if (cap && cap->isOpened()) {
        cv::Mat frame;
        if (cap->read(frame)) {
            cv::Mat rgb;
            cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
            cv::Mat resized;
            cv::resize(rgb, resized, cv::Size(kColorWidth, kColorHeight));
            if (resized.isContinuous() && resized.total() * resized.elemSize() <= out.color_buffer.size()) {
                std::memcpy(out.color_buffer.data(), resized.data, resized.total() * resized.elemSize());
            }
        } else {
            for (size_t i = 0; i < out.color_buffer.size(); ++i)
                out.color_buffer[i] = static_cast<unsigned char>(rand() % 256);
        }
    } else {
        for (size_t i = 0; i < out.color_buffer.size(); ++i)
            out.color_buffer[i] = static_cast<unsigned char>(rand() % 256);
    }
    out.color_rgb.data = out.color_buffer.data();

    // Synthetic depth (radial gradient)
    for (int y = 0; y < kDepthHeight; ++y) {
        for (int x = 0; x < kDepthWidth; ++x) {
            float dx = x - kDepthWidth / 2.0f;
            float dy = y - kDepthHeight / 2.0f;
            float dist = std::sqrt(dx * dx + dy * dy);
            float normalized = dist / (kDepthWidth * 0.7f);
            out.depth_buffer[y * kDepthWidth + x] = static_cast<unsigned char>(255.0f * (1.0f - std::min(1.0f, normalized)));
        }
    }
    out.depth_g8.width = kDepthWidth;
    out.depth_g8.height = kDepthHeight;
    out.depth_g8.format = ImageFormat::Luminance8;
    out.depth_g8.data = out.depth_buffer.data();
    out.depth_g8.row_stride = 0;

    return true;
}

} // namespace

int main(int /*argc*/, char* /*argv*/[])
{
    using namespace viewportal;

    const int rows = 2;
    const int cols = 2;
    std::vector<ViewportType> types = {
        ViewportType::RGB8,
        ViewportType::G8,
        ViewportType::Reconstruction,
        ViewportType::Plot
    };

    ViewPortalParams params;
    params.window_width = 1280;
    params.window_height = 720;
    params.panel_width = 200;
    params.window_title = "ViewPortal Sample (Example)";

    ViewPortal portal(rows, cols, types, params);

    cv::VideoCapture cap;
    bool use_camera = false;
    for (int i = 0; i < 4; ++i) {
        if (cap.open(i)) {
            use_camera = true;
            break;
        }
    }
    if (!use_camera) {
        std::cout << "No camera found; using synthetic frames for color/depth viewports." << std::endl;
    }

    ViewportalCapture capture;
    while (!portal.shouldQuit()) {
        if (!captureFrames(use_camera ? &cap : nullptr, capture))
            continue;
        if (capture.color_rgb.data)
            portal.updateFrame(0, capture.color_rgb);
        if (capture.depth_g8.data)
            portal.updateFrame(1, capture.depth_g8);
    }

    return 0;
}
