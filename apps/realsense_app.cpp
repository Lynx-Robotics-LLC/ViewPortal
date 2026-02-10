/*
 * RealSense four-view app (based on depthimage-example pipeline).
 * Connects to an Intel RealSense D435, reads left IR, right IR, depth, and
 * color streams, and displays them in four ViewPortal viewports.
 * Frame capture is separated into capture functions; the GUI just receives
 * the captured frame data.
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
constexpr float kDepthMaxMeters = 0.50f;

/** Holds the current frameset and buffers so FrameData pointers stay valid for one capture. */
struct RealsenseCapture {
    rs2::frameset frameset;
    std::vector<unsigned char> depth_rgb_buffer;
    std::vector<unsigned char> color_rgb_buffer;

    FrameData left_ir;
    FrameData right_ir;
    FrameData depth_rgb;
    FrameData color_rgb;

    RealsenseCapture() {
        depth_rgb_buffer.resize(3 * kWidth * kHeight);
        color_rgb_buffer.resize(3 * kWidth * kHeight);
    }
};

/** Capture one frameset and fill left_ir, right_ir, depth_rgb, color_rgb. Returns false on failure. */
bool captureFrames(rs2::pipeline& pipe, RealsenseCapture& out) {
    out.left_ir.data = nullptr;
    out.right_ir.data = nullptr;
    out.depth_rgb.data = nullptr;
    out.color_rgb.data = nullptr;

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

        cv::Mat depth_color;
        cv::applyColorMap(depth_norm, depth_color, cv::COLORMAP_JET);
        cv::Mat depth_rgb;
        cv::cvtColor(depth_color, depth_rgb, cv::COLOR_BGR2RGB);

        if (depth_rgb.isContinuous() && depth_rgb.total() * depth_rgb.elemSize() <= out.depth_rgb_buffer.size()) {
            std::memcpy(out.depth_rgb_buffer.data(), depth_rgb.data, depth_rgb.total() * depth_rgb.elemSize());
        }
        out.depth_rgb.width = dw;
        out.depth_rgb.height = dh;
        out.depth_rgb.format = ImageFormat::RGB8;
        out.depth_rgb.data = out.depth_rgb_buffer.data();
        out.depth_rgb.row_stride = 0;
    }

    rs2::video_frame color_frame = out.frameset.get_color_frame();
    if (color_frame) {
        const int cw = color_frame.get_width();
        const int ch = color_frame.get_height();
        const int rs_format = color_frame.get_profile().format();
        const void* color_data = color_frame.get_data();

        out.color_rgb.width = cw;
        out.color_rgb.height = ch;
        out.color_rgb.format = ImageFormat::RGB8;
        out.color_rgb.row_stride = 0;
        if (rs_format == RS2_FORMAT_RGB8) {
            out.color_rgb.data = color_data;
        } else {
            cv::Mat bgr(ch, cw, CV_8UC3, (void*)color_data);
            cv::Mat rgb;
            cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
            if (rgb.isContinuous() && rgb.total() * rgb.elemSize() <= out.color_rgb_buffer.size()) {
                std::memcpy(out.color_rgb_buffer.data(), rgb.data, rgb.total() * rgb.elemSize());
            }
            out.color_rgb.data = out.color_rgb_buffer.data();
        }
    }

    return true;
}

} // namespace

int main(int /*argc*/, char* /*argv*/[])
{
    using namespace viewportal;

    rs2::pipeline pipe;
    rs2::config cfg;

    cfg.enable_stream(RS2_STREAM_DEPTH, kWidth, kHeight, RS2_FORMAT_Z16, 30);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1, kWidth, kHeight, RS2_FORMAT_Y8, 30);
    cfg.enable_stream(RS2_STREAM_INFRARED, 2, kWidth, kHeight, RS2_FORMAT_Y8, 30);
    cfg.enable_stream(RS2_STREAM_COLOR, kWidth, kHeight, RS2_FORMAT_RGB8, 30);

    try {
        pipe.start(cfg);
    } catch (const rs2::error& e) {
        std::cerr << "RealSense error: " << e.what() << std::endl;
        std::cerr << "Make sure the camera is connected via USB." << std::endl;
        return 1;
    }

    // Viewport types: 0 left IR (grayscale), 1 right IR (grayscale), 2 depth (jet RGB), 3 color (RGB)
    std::vector<ViewportType> types = {
        ViewportType::G8,    // left IR  (Luminance8)
        ViewportType::G8,    // right IR (Luminance8)
        ViewportType::RGB8,  // depth colormap (RGB8)
        ViewportType::RGB8   // color camera (RGB8)
    };

    ViewPortal portal(1, 4, types, "ViewPortal RealSense");

    RealsenseCapture capture;
    while (!portal.shouldQuit()) {
        if (!captureFrames(pipe, capture))
            continue;
        // if (capture.left_ir.data)
        //     portal.updateFrame(0, capture.left_ir);
        if (capture.right_ir.data)
            portal.updateFrame(1, capture.right_ir);
        if (capture.depth_rgb.data)
            portal.updateFrame(2, capture.depth_rgb);
        if (capture.color_rgb.data)
            portal.updateFrame(3, capture.color_rgb);
    }

    pipe.stop();
    return 0;
}
