/*
 * RealSense four-view app (based on depthimage-example pipeline).
 * Connects to an Intel RealSense D435, reads left IR, right IR, depth, and
 * color streams, and displays them in four ViewPortal viewports.
 */

#include "viewportal.h"
#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cstring>

int main(int /*argc*/, char* /*argv*/[])
{
    using namespace viewportal;

    // --- RealSense setup ---
    rs2::pipeline pipe;
    rs2::config cfg;

    cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1, 640, 480, RS2_FORMAT_Y8, 30);
    cfg.enable_stream(RS2_STREAM_INFRARED, 2, 640, 480, RS2_FORMAT_Y8, 30);
    cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_RGB8, 30);

    try {
        pipe.start(cfg);
    } catch (const rs2::error& e) {
        std::cerr << "RealSense error: " << e.what() << std::endl;
        std::cerr << "Make sure the camera is connected via USB." << std::endl;
        return 1;
    }

    // ViewPortal: 1 row x 4 cols — left IR, right IR, depth (jet), color
    std::vector<ViewportType> types = {
        ViewportType::DepthImage,   // left IR (Luminance8)
        ViewportType::DepthImage,   // right IR (Luminance8)
        ViewportType::ColorImage,   // depth with jet colormap (RGB8)
        ViewportType::ColorImage    // RGB color camera
    };

    ViewPortal portal(1, 4, types, "ViewPortal RealSense");

    const int w = 640;
    const int h = 480;
    const float depth_max_meters = 3.0f;

    std::vector<unsigned char> depth_rgb_buffer(3 * w * h);
    std::vector<unsigned char> color_rgb_buffer(3 * w * h);

    while (!portal.shouldQuit()) {
        rs2::frameset frames = pipe.wait_for_frames();

        // Left infrared (viewport 0)
        rs2::video_frame ir_left = frames.get_infrared_frame(1);
        if (ir_left) {
            FrameData leftFrame;
            leftFrame.width = ir_left.get_width();
            leftFrame.height = ir_left.get_height();
            leftFrame.format = ImageFormat::Luminance8;
            leftFrame.data = ir_left.get_data();
            portal.updateFrame(0, leftFrame);
        }

        // Right infrared (viewport 1)
        rs2::video_frame ir_right = frames.get_infrared_frame(2);
        if (ir_right) {
            FrameData rightFrame;
            rightFrame.width = ir_right.get_width();
            rightFrame.height = ir_right.get_height();
            rightFrame.format = ImageFormat::Luminance8;
            rightFrame.data = ir_right.get_data();
            portal.updateFrame(1, rightFrame);
        }

        // Depth (viewport 2): raw -> meters -> 8-bit -> jet colormap -> RGB
        rs2::depth_frame depth = frames.get_depth_frame();
        if (depth) {
            const int dw = depth.get_width();
            const int dh = depth.get_height();
            const float scale = depth.get_units();

            cv::Mat depth_raw(dh, dw, CV_16UC1, (void*)depth.get_data());
            cv::Mat depth_meters;
            depth_raw.convertTo(depth_meters, CV_32FC1, scale);
            depth_meters.setTo(depth_max_meters, depth_meters > depth_max_meters);

            cv::Mat depth_norm;
            depth_meters.convertTo(depth_norm, CV_8UC1, 255.0 / depth_max_meters, 0);

            cv::Mat depth_color;
            cv::applyColorMap(depth_norm, depth_color, cv::COLORMAP_JET);
            cv::Mat depth_rgb;
            cv::cvtColor(depth_color, depth_rgb, cv::COLOR_BGR2RGB);

            if (depth_rgb.isContinuous() && depth_rgb.total() * depth_rgb.elemSize() <= static_cast<size_t>(depth_rgb_buffer.size())) {
                std::memcpy(depth_rgb_buffer.data(), depth_rgb.data, depth_rgb.total() * depth_rgb.elemSize());
            }

            FrameData depthFrame;
            depthFrame.width = dw;
            depthFrame.height = dh;
            depthFrame.format = ImageFormat::RGB8;
            depthFrame.data = depth_rgb_buffer.data();
            portal.updateFrame(2, depthFrame);
        }

        // Color (viewport 3): RGB from camera; convert BGR→RGB if needed
        rs2::video_frame color_frame = frames.get_color_frame();
        if (color_frame) {
            const int cw = color_frame.get_width();
            const int ch = color_frame.get_height();
            const int rs_format = color_frame.get_profile().format();
            const void* color_data = color_frame.get_data();

            FrameData colorFrame;
            colorFrame.width = cw;
            colorFrame.height = ch;
            colorFrame.format = ImageFormat::RGB8;
            if (rs_format == RS2_FORMAT_RGB8) {
                colorFrame.data = color_data;
            } else {
                // e.g. RS2_FORMAT_BGR8: convert to RGB for ViewPortal
                cv::Mat bgr(ch, cw, CV_8UC3, (void*)color_data);
                cv::Mat rgb;
                cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
                if (rgb.isContinuous() && rgb.total() * rgb.elemSize() <= static_cast<size_t>(color_rgb_buffer.size())) {
                    std::memcpy(color_rgb_buffer.data(), rgb.data, rgb.total() * rgb.elemSize());
                }
                colorFrame.data = color_rgb_buffer.data();
            }
            portal.updateFrame(3, colorFrame);
        }

        portal.step();
    }

    pipe.stop();
    return 0;
}
