#include "viewport.h"
#include <pangolin/display/display.h>
#include <pangolin/gl/gl.h>
#include <pangolin/var/var.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <cstring>

namespace viewportal {

class ColoredDepthViewport : public Viewport {
public:
    ColoredDepthViewport(const std::string& name, float aspect_ratio, int width, int height)
        : name_(name),
          width_(width),
          height_(height),
          rgb_buffer_(nullptr) {
        rgb_buffer_ = new unsigned char[3 * width_ * height_];
        colorTexture_ = pangolin::GlTexture(width_, height_, GL_RGB, false, 0, GL_RGB, GL_UNSIGNED_BYTE);
        view_ = &pangolin::Display(name).SetAspect(aspect_ratio);
    }

    ~ColoredDepthViewport() override {
        if (rgb_buffer_) {
            delete[] rgb_buffer_;
            rgb_buffer_ = nullptr;
        }
    }

    pangolin::View& getView() override { return *view_; }
    std::string getName() const override { return name_; }

    void setFrame(const FrameData& frame) override {
        user_frame_ = frame;
    }

    void update() override {
        if (user_frame_.data != nullptr && user_frame_.width > 0 && user_frame_.height > 0) {
            ensureTextureSize(user_frame_.width, user_frame_.height);
            if (user_frame_.format == ImageFormat::Luminance8) {
                cv::Mat gray(user_frame_.height, user_frame_.width, CV_8UC1, (void*)user_frame_.data);
                cv::Mat colored;
                cv::applyColorMap(gray, colored, cv::COLORMAP_JET);
                cv::Mat rgb;
                cv::cvtColor(colored, rgb, cv::COLOR_BGR2RGB);
                const size_t n = static_cast<size_t>(user_frame_.width) * user_frame_.height * 3;
                if (n <= static_cast<size_t>(3 * width_ * height_)) {
                    std::memcpy(rgb_buffer_, rgb.data, n);
                }
                colorTexture_.Upload(rgb_buffer_, GL_RGB, GL_UNSIGNED_BYTE);
            }
            return;
        }
        std::memset(rgb_buffer_, 0, static_cast<size_t>(3 * width_ * height_));
        colorTexture_.Upload(rgb_buffer_, GL_RGB, GL_UNSIGNED_BYTE);
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
    void ensureTextureSize(int w, int h) {
        if (w == width_ && h == height_) return;
        width_ = w;
        height_ = h;
        if (rgb_buffer_) {
            delete[] rgb_buffer_;
            rgb_buffer_ = nullptr;
        }
        rgb_buffer_ = new unsigned char[3 * width_ * height_];
        colorTexture_ = pangolin::GlTexture(width_, height_, GL_RGB, false, 0, GL_RGB, GL_UNSIGNED_BYTE);
    }

    std::string name_;
    pangolin::View* view_;
    int width_;
    int height_;
    unsigned char* rgb_buffer_;
    pangolin::GlTexture colorTexture_;
    std::unique_ptr<pangolin::Var<bool>> show_view_;
    FrameData user_frame_;
};

std::unique_ptr<Viewport> createColoredDepthViewport(const std::string& name, float aspect_ratio, int width, int height) {
    return std::make_unique<ColoredDepthViewport>(name, aspect_ratio, width, height);
}

} // namespace viewportal
