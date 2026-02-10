#include "viewport.h"
#include <pangolin/display/display.h>
#include <pangolin/gl/gl.h>
#include <pangolin/var/var.h>
#include <memory>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace viewportal {

namespace {

// JET-like colormap LUT: 256 entries, RGB (same order as GL_RGB)
void buildJetRgbLut(unsigned char* lut) {
    for (int i = 0; i < 256; ++i) {
        float t = i / 255.0f;
        float r = std::clamp(1.5f - 4.0f * std::abs(t - 0.75f), 0.0f, 1.0f);
        float g = std::clamp(1.5f - 4.0f * std::abs(t - 0.5f),  0.0f, 1.0f);
        float b = std::clamp(1.5f - 4.0f * std::abs(t - 0.25f), 0.0f, 1.0f);
        lut[i * 3 + 0] = static_cast<unsigned char>(r * 255.0f);
        lut[i * 3 + 1] = static_cast<unsigned char>(g * 255.0f);
        lut[i * 3 + 2] = static_cast<unsigned char>(b * 255.0f);
    }
}

void applyJetToG8(const unsigned char* g8, int width, int height, unsigned char* rgb, const unsigned char* lut) {
    const size_t n = static_cast<size_t>(width) * height;
    for (size_t i = 0; i < n; ++i) {
        unsigned char v = g8[i];
        rgb[i * 3 + 0] = lut[v * 3 + 0];
        rgb[i * 3 + 1] = lut[v * 3 + 1];
        rgb[i * 3 + 2] = lut[v * 3 + 2];
    }
}

} // namespace

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
                unsigned char jet_lut[256 * 3];
                buildJetRgbLut(jet_lut);
                const auto* g8 = static_cast<const unsigned char*>(user_frame_.data);
                applyJetToG8(g8, user_frame_.width, user_frame_.height, rgb_buffer_, jet_lut);
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
