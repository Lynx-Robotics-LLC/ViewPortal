#include "viewport.h"
#include <pangolin/display/display.h>
#include <pangolin/gl/gl.h>
#include <pangolin/var/var.h>
#include <cstdlib>
#include <memory>

namespace viewportal {

class Rgb8Viewport : public Viewport {
public:
    Rgb8Viewport(const std::string& name, float aspect_ratio)
        : name_(name),
          width_(320),
          height_(240),
          colorImageArray_(nullptr) {
        size_t color_buffer_size = 3 * width_ * height_;
        colorImageArray_ = new unsigned char[color_buffer_size];
        colorTexture_ = pangolin::GlTexture(width_, height_, GL_RGB, false, 0, GL_RGB, GL_UNSIGNED_BYTE);
        view_ = &pangolin::Display(name).SetAspect(aspect_ratio);
    }

    ~Rgb8Viewport() override {
        if (colorImageArray_) {
            delete[] colorImageArray_;
            colorImageArray_ = nullptr;
        }
    }

    pangolin::View& getView() override { return *view_; }
    std::string getName() const override { return name_; }

    void setFrame(const FrameData& frame) override {
        user_frame_ = frame;
    }

    void update() override {
        if (user_frame_.data != nullptr && user_frame_.width > 0 && user_frame_.height > 0) {
            ensureTextureSize(user_frame_.width, user_frame_.height, user_frame_.format);
            uploadFrame(user_frame_);
            return;
        }
        setColorImageData(colorImageArray_, 3 * width_ * height_);
        colorTexture_.Upload(colorImageArray_, GL_RGB, GL_UNSIGNED_BYTE);
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

    void ensureTextureSize(int w, int h, ImageFormat fmt) {
        if (w == width_ && h == height_ && fmt == last_format_) return;
        width_ = w;
        height_ = h;
        last_format_ = fmt;
        GLint gl_internal = GL_RGB;
        GLenum gl_format = GL_RGB;
        if (fmt == ImageFormat::RGBA8) {
            gl_internal = GL_RGBA;
            gl_format = GL_RGBA;
        } else if (fmt == ImageFormat::Luminance8) {
            gl_internal = GL_LUMINANCE;
            gl_format = GL_LUMINANCE;
        }
        colorTexture_ = pangolin::GlTexture(w, h, gl_internal, false, 0, gl_format, GL_UNSIGNED_BYTE);
    }

    void uploadFrame(const FrameData& frame) {
        GLenum gl_format = GL_RGB;
        if (frame.format == ImageFormat::RGBA8) gl_format = GL_RGBA;
        else if (frame.format == ImageFormat::Luminance8) gl_format = GL_LUMINANCE;
        colorTexture_.Upload(frame.data, gl_format, GL_UNSIGNED_BYTE);
    }

    FrameData user_frame_;
    ImageFormat last_format_ = ImageFormat::RGB8;

    std::string name_;
    pangolin::View* view_;
    int width_;
    int height_;
    unsigned char* colorImageArray_;
    pangolin::GlTexture colorTexture_;
    std::unique_ptr<pangolin::Var<bool>> show_view_;
};

std::unique_ptr<Viewport> createRgb8Viewport(const std::string& name, float aspect_ratio) {
    return std::make_unique<Rgb8Viewport>(name, aspect_ratio);
}

} // namespace viewportal
