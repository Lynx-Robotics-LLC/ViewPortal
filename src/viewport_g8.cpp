#include "viewport.h"
#include <pangolin/display/display.h>
#include <pangolin/gl/gl.h>
#include <pangolin/var/var.h>
#include <cmath>
#include <memory>

namespace viewportal {

class G8Viewport : public Viewport {
public:
    G8Viewport(const std::string& name, float aspect_ratio, int width, int height)
        : name_(name),
          width_(width),
          height_(height),
          imageBuffer_(nullptr) {
        imageBuffer_ = new unsigned char[width_ * height_];
        luminanceTexture_ = pangolin::GlTexture(width_, height_, GL_LUMINANCE, false, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE);
        view_ = &pangolin::Display(name).SetAspect(aspect_ratio);
    }

    ~G8Viewport() override {
        if (imageBuffer_) {
            delete[] imageBuffer_;
            imageBuffer_ = nullptr;
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
            luminanceTexture_.Upload(user_frame_.data, GL_LUMINANCE, GL_UNSIGNED_BYTE);
            return;
        }
        setPlaceholderImageData(imageBuffer_, width_, height_);
        luminanceTexture_.Upload(imageBuffer_, GL_LUMINANCE, GL_UNSIGNED_BYTE);
    }

    void render() override {
        if (view_->IsShown()) {
            view_->Activate();
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            luminanceTexture_.RenderToViewportFlipY();
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
        if (imageBuffer_) {
            delete[] imageBuffer_;
            imageBuffer_ = nullptr;
        }
        imageBuffer_ = new unsigned char[width_ * height_];
        luminanceTexture_ = pangolin::GlTexture(width_, height_, GL_LUMINANCE, false, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE);
    }

    void setPlaceholderImageData(unsigned char* imageArray, int width, int height) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float dist = sqrt((x - width / 2.0f) * (x - width / 2.0f) + (y - height / 2.0f) * (y - height / 2.0f));
                float normalized = dist / (width * 0.7f);
                unsigned char value = (unsigned char)(255.0f * (1.0f - normalized));
                imageArray[y * width + x] = value;
            }
        }
    }

    std::string name_;
    pangolin::View* view_;
    int width_;
    int height_;
    unsigned char* imageBuffer_;
    pangolin::GlTexture luminanceTexture_;
    std::unique_ptr<pangolin::Var<bool>> show_view_;
    FrameData user_frame_;
};

std::unique_ptr<Viewport> createG8Viewport(const std::string& name, float aspect_ratio, int width, int height) {
    return std::make_unique<G8Viewport>(name, aspect_ratio, width, height);
}

} // namespace viewportal
