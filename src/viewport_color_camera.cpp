#include "viewport.h"
#include <pangolin/display/display.h>
#include <pangolin/gl/gl.h>
#include <pangolin/var/var.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>
#include <memory>

namespace viewportal {

class ColorCameraViewport : public Viewport {
public:
    ColorCameraViewport(const std::string& name, float aspect_ratio)
        : name_(name),
          width_(320),
          height_(240),
          use_camera_(false),
          colorImageArray_(nullptr) {
        use_camera_ = tryOpenCamera();
        if (!use_camera_) {
            std::cout << "Warning: Could not open USB camera, using random noise instead" << std::endl;
        }
        size_t color_buffer_size = 3 * width_ * height_;
        colorImageArray_ = new unsigned char[color_buffer_size];
        colorTexture_ = pangolin::GlTexture(width_, height_, GL_RGB, false, 0, GL_RGB, GL_UNSIGNED_BYTE);
        view_ = &pangolin::Display(name).SetAspect(aspect_ratio);
    }

    ~ColorCameraViewport() override {
        if (cap_.isOpened()) cap_.release();
        if (colorImageArray_) {
            delete[] colorImageArray_;
            colorImageArray_ = nullptr;
        }
    }

    pangolin::View& getView() override { return *view_; }
    std::string getName() const override { return name_; }

    void update() override {
        if (use_camera_ && cap_.isOpened()) {
            cv::Mat frame;
            if (cap_.read(frame)) {
                cv::Mat rgb_frame;
                cv::cvtColor(frame, rgb_frame, cv::COLOR_BGR2RGB);
                colorTexture_.Upload(rgb_frame.data, GL_RGB, GL_UNSIGNED_BYTE);
            } else {
                setColorImageData(colorImageArray_, 3 * width_ * height_);
                colorTexture_.Upload(colorImageArray_, GL_RGB, GL_UNSIGNED_BYTE);
            }
        } else {
            setColorImageData(colorImageArray_, 3 * width_ * height_);
            colorTexture_.Upload(colorImageArray_, GL_RGB, GL_UNSIGNED_BYTE);
        }
    }

    void render() override {
        if (view_->IsShown()) {
            view_->Activate();
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            colorTexture_.RenderToViewportFlipY();
        }
    }

    void setupUI() override {
        std::string prefix = "ui." + name_ + ".";
        show_view_ = std::make_unique<pangolin::Var<bool>>(prefix + "Show", true, true);
    }

    bool isShown() const override {
        return show_view_ && show_view_->Get();
    }

private:
    void setColorImageData(unsigned char* imageArray, int size) {
        for (int i = 0; i < size; i++) {
            imageArray[i] = (unsigned char)(rand() / (RAND_MAX / 255.0));
        }
    }

    bool tryOpenCamera() {
        for (int camera_index = 0; camera_index < 4; ++camera_index) {
            if (cap_.open(camera_index)) {
                width_ = (int)cap_.get(cv::CAP_PROP_FRAME_WIDTH);
                height_ = (int)cap_.get(cv::CAP_PROP_FRAME_HEIGHT);
                std::cout << "Opened camera " << camera_index << " (" << width_ << "x" << height_ << ")" << std::endl;
                return true;
            }
        }
        return false;
    }

    std::string name_;
    pangolin::View* view_;
    cv::VideoCapture cap_;
    int width_;
    int height_;
    bool use_camera_;
    unsigned char* colorImageArray_;
    pangolin::GlTexture colorTexture_;
    std::unique_ptr<pangolin::Var<bool>> show_view_;
};

std::unique_ptr<Viewport> createColorCameraViewport(const std::string& name, float aspect_ratio) {
    return std::make_unique<ColorCameraViewport>(name, aspect_ratio);
}

} // namespace viewportal
