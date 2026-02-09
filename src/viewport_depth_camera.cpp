#include "viewport.h"
#include <pangolin/display/display.h>
#include <pangolin/gl/gl.h>
#include <pangolin/var/var.h>
#include <cmath>
#include <memory>

namespace viewportal {

class DepthCameraViewport : public Viewport {
public:
    DepthCameraViewport(const std::string& name, float aspect_ratio, int width, int height)
        : name_(name),
          width_(width),
          height_(height),
          depthImageArray_(nullptr) {
        depthImageArray_ = new unsigned char[width_ * height_];
        depthTexture_ = pangolin::GlTexture(width_, height_, GL_LUMINANCE, false, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE);
        view_ = &pangolin::Display(name).SetAspect(aspect_ratio);
    }

    ~DepthCameraViewport() override {
        if (depthImageArray_) {
            delete[] depthImageArray_;
            depthImageArray_ = nullptr;
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
            depthTexture_.Upload(user_frame_.data, GL_LUMINANCE, GL_UNSIGNED_BYTE);
            return;
        }
        if (show_depth_ && show_depth_->Get()) {
            setDepthImageData(depthImageArray_, width_, height_);
            depthTexture_.Upload(depthImageArray_, GL_LUMINANCE, GL_UNSIGNED_BYTE);
        }
    }

    void render() override {
        if (view_->IsShown() && show_depth_ && show_depth_->Get()) {
            view_->Activate();
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            depthTexture_.RenderToViewportFlipY();
        }
    }

    void setupUI() override {
        std::string prefix = "ui." + name_ + ".";
        show_view_ = std::make_unique<pangolin::Var<bool>>(prefix + "Show", true, true);
        show_depth_ = std::make_unique<pangolin::Var<bool>>(prefix + "Show_Depth", true, true);
        depth_scale_ = std::make_unique<pangolin::Var<double>>(prefix + "Depth_Scale", 1.0, 0.1, 5.0);
    }

    bool isShown() const override {
        return show_view_ && show_view_->Get();
    }

private:
    void ensureTextureSize(int w, int h) {
        if (w == width_ && h == height_) return;
        width_ = w;
        height_ = h;
        if (depthImageArray_) {
            delete[] depthImageArray_;
            depthImageArray_ = nullptr;
        }
        depthImageArray_ = new unsigned char[width_ * height_];
        depthTexture_ = pangolin::GlTexture(width_, height_, GL_LUMINANCE, false, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE);
    }

    void setDepthImageData(unsigned char* imageArray, int width, int height) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float dist = sqrt((x - width / 2.0f) * (x - width / 2.0f) + (y - height / 2.0f) * (y - height / 2.0f));
                float normalized = dist / (width * 0.7f);
                unsigned char depth = (unsigned char)(255.0f * (1.0f - normalized));
                imageArray[y * width + x] = depth;
            }
        }
    }

    std::string name_;
    pangolin::View* view_;
    int width_;
    int height_;
    unsigned char* depthImageArray_;
    pangolin::GlTexture depthTexture_;
    std::unique_ptr<pangolin::Var<bool>> show_view_;
    std::unique_ptr<pangolin::Var<bool>> show_depth_;
    std::unique_ptr<pangolin::Var<double>> depth_scale_;
    FrameData user_frame_;
};

std::unique_ptr<Viewport> createDepthCameraViewport(const std::string& name, float aspect_ratio, int width, int height) {
    return std::make_unique<DepthCameraViewport>(name, aspect_ratio, width, height);
}

} // namespace viewportal
