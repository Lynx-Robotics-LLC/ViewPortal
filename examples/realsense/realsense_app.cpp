/*
 * Standalone RealSense example using the ViewPortal library (FetchContent).
 * Connects to an Intel RealSense D435, reads left IR, right IR, depth, and
 * color streams, and displays them in five ViewPortal viewports.
 * Viewport 4 shows a snapshot of the current color frame when 's' is pressed.
 */

#include "viewportal.h"
#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cstring>

namespace {

using namespace viewportal;

constexpr int kWidth = 640;
constexpr int kHeight = 480;
constexpr float kDepthMaxMeters = 1.50f;

struct RealsenseCapture {
    rs2::frameset frameset;
    std::vector<unsigned char> depth_buffer;

    FrameData left_ir;
    FrameData right_ir;
    FrameData depth;
    FrameData color_rgb;

    RealsenseCapture() {
        depth_buffer.resize(kWidth * kHeight);
        left_ir.data = nullptr;
        right_ir.data = nullptr;
        depth.data = nullptr;
        color_rgb.data = nullptr;
    }
};

bool initRealsense(rs2::pipeline& pipe) {
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH, kWidth, kHeight, RS2_FORMAT_Z16, 30);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1, kWidth, kHeight, RS2_FORMAT_Y8, 30);
    cfg.enable_stream(RS2_STREAM_INFRARED, 2, kWidth, kHeight, RS2_FORMAT_Y8, 30);
    cfg.enable_stream(RS2_STREAM_COLOR, kWidth, kHeight, RS2_FORMAT_RGB8, 30);
    try {
        pipe.start(cfg);
        return true;
    } catch (const rs2::error& e) {
        std::cerr << "RealSense error: " << e.what() << std::endl;
        std::cerr << "Make sure the camera is connected via USB." << std::endl;
        return false;
    }
}

bool captureFrames(rs2::pipeline& pipe, RealsenseCapture& out) {
    try {
        out.frameset = pipe.wait_for_frames();
    } catch (const rs2::error&) {
        return false;
    }

    rs2::video_frame ir_left = out.frameset.get_infrared_frame(1);
    if (ir_left) {
        out.left_ir.width = ir_left.get_width();
        out.left_ir.height = ir_left.get_height();
        out.left_ir.format = ImageFormat::Luminance8;
        out.left_ir.data = ir_left.get_data();
        out.left_ir.row_stride = 0;
    }

    rs2::video_frame ir_right = out.frameset.get_infrared_frame(2);
    if (ir_right) {
        out.right_ir.width = ir_right.get_width();
        out.right_ir.height = ir_right.get_height();
        out.right_ir.format = ImageFormat::Luminance8;
        out.right_ir.data = ir_right.get_data();
        out.right_ir.row_stride = 0;
    }

    rs2::depth_frame depth = out.frameset.get_depth_frame();
    if (depth) {
        const int dw = depth.get_width();
        const int dh = depth.get_height();
        const float scale = depth.get_units();

        cv::Mat depth_raw(dh, dw, CV_16UC1, (void*)depth.get_data());
        cv::Mat depth_meters;
        depth_raw.convertTo(depth_meters, CV_32FC1, scale);
        depth_meters.setTo(kDepthMaxMeters, depth_meters > kDepthMaxMeters);

        cv::Mat depth_norm;
        depth_meters.convertTo(depth_norm, CV_8UC1, 255.0f / kDepthMaxMeters, 0);

        if (depth_norm.isContinuous() && depth_norm.total() * depth_norm.elemSize() <= out.depth_buffer.size()) {
            std::memcpy(out.depth_buffer.data(), depth_norm.data, depth_norm.total() * depth_norm.elemSize());
        }
        out.depth.width = dw;
        out.depth.height = dh;
        out.depth.format = ImageFormat::Luminance8;
        out.depth.data = out.depth_buffer.data();
        out.depth.row_stride = 0;
    }

    rs2::video_frame color_frame = out.frameset.get_color_frame();
    if (color_frame) {
        const int cw = color_frame.get_width();
        const int ch = color_frame.get_height();
        out.color_rgb.width = cw;
        out.color_rgb.height = ch;
        out.color_rgb.format = ImageFormat::RGB8;
        out.color_rgb.row_stride = 0;
        out.color_rgb.data = color_frame.get_data();
    }

    return true;
}

} // namespace

int main(int /*argc*/, char* /*argv*/[])
{
    using namespace viewportal;

    rs2::pipeline pipe;
    if (!initRealsense(pipe)) {
        return 1;
    }

    std::vector<ViewportType> types = {
        ViewportType::G8,
        ViewportType::G8,
        ViewportType::ColoredDepth,
        ViewportType::RGB8,
        ViewportType::RGB8
    };

    ViewPortalParams params;
    params.window_title = "ViewPortal RealSense (Example)";
    ViewPortal portal(1, 5, types, params);
    portal.setKeysToWatch({' '});

    RealsenseCapture capture;
    while (!portal.shouldQuit()) {
        if (!captureFrames(pipe, capture))
            continue;
        if (capture.left_ir.data)
            portal.updateFrame(0, capture.left_ir);
        if (capture.right_ir.data)
            portal.updateFrame(1, capture.right_ir);
        if (capture.depth.data)
            portal.updateFrame(2, capture.depth);
        if (capture.color_rgb.data)
            portal.updateFrame(3, capture.color_rgb);
        if (portal.checkKey('s') && capture.color_rgb.data)
            portal.updateFrame(4, capture.color_rgb);
    }

    pipe.stop();
    return 0;
}
