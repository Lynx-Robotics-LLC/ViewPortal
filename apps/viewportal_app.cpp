#include "viewportal.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <cstring>

int main(int /*argc*/, char* /*argv*/[])
{
    using namespace viewportal;

    const int rows = 2;
    const int cols = 2;
    std::vector<ViewportType> types = {
        ViewportType::ColorImage,
        ViewportType::DepthImage,
        ViewportType::Reconstruction,
        ViewportType::Plot
    };

    ViewPortalParams params;
    params.window_width = 1280;
    params.window_height = 720;
    params.panel_width = 200;
    params.window_title = "ViewPortal Multi-View";

    ViewPortal portal(rows, cols, types, params);

    // Optional: OpenCV camera for demo color frames (app-only, not part of public API)
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

    const int color_width = 320;
    const int color_height = 240;
    const int depth_width = 320;
    const int depth_height = 240;
    std::vector<unsigned char> color_buffer(3 * color_width * color_height);
    std::vector<unsigned char> depth_buffer(depth_width * depth_height);

    while (!portal.shouldQuit()) {
        FrameData colorFrame;
        colorFrame.width = color_width;
        colorFrame.height = color_height;
        colorFrame.format = ImageFormat::RGB8;

        if (use_camera && cap.isOpened()) {
            cv::Mat frame;
            if (cap.read(frame)) {
                cv::Mat rgb;
                cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
                cv::Mat resized;
                cv::resize(rgb, resized, cv::Size(color_width, color_height));
                if (resized.isContinuous() && resized.total() * resized.elemSize() <= color_buffer.size()) {
                    std::memcpy(color_buffer.data(), resized.data, resized.total() * resized.elemSize());
                }
            } else {
                for (size_t i = 0; i < color_buffer.size(); ++i)
                    color_buffer[i] = static_cast<unsigned char>(rand() % 256);
            }
        } else {
            for (size_t i = 0; i < color_buffer.size(); ++i)
                color_buffer[i] = static_cast<unsigned char>(rand() % 256);
        }
        colorFrame.data = color_buffer.data();
        portal.updateFrame(0, colorFrame);

        // Synthetic depth (radial gradient)
        for (int y = 0; y < depth_height; ++y) {
            for (int x = 0; x < depth_width; ++x) {
                float dx = x - depth_width / 2.0f;
                float dy = y - depth_height / 2.0f;
                float dist = std::sqrt(dx * dx + dy * dy);
                float normalized = dist / (depth_width * 0.7f);
                depth_buffer[y * depth_width + x] = static_cast<unsigned char>(255.0f * (1.0f - std::min(1.0f, normalized)));
            }
        }
        FrameData depthFrame;
        depthFrame.width = depth_width;
        depthFrame.height = depth_height;
        depthFrame.format = ImageFormat::Luminance8;
        depthFrame.data = depth_buffer.data();
        portal.updateFrame(1, depthFrame);

        portal.step();
    }

    return 0;
}
