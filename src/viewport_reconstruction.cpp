#include "viewport.h"
#include <pangolin/display/display.h>
#include <pangolin/handler/handler.h>
#include <pangolin/gl/gl.h>
#include <pangolin/gl/gldraw.h>
#include <pangolin/var/var.h>
#include <memory>

namespace viewportal {

class ReconstructionViewport : public Viewport {
public:
    ReconstructionViewport(const std::string& name, float aspect_ratio,
                          const pangolin::OpenGlRenderState& render_state)
        : name_(name),
          render_state_(render_state) {
        view_ = &pangolin::Display(name)
            .SetAspect(aspect_ratio)
            .SetHandler(new pangolin::Handler3D(render_state_));
    }

    ~ReconstructionViewport() override = default;

    pangolin::View& getView() override { return *view_; }
    std::string getName() const override { return name_; }

    void render() override {
        if (view_->IsShown()) {
            view_->Activate(render_state_);
            glColor3f(1.0f, 1.0f, 1.0f);
            pangolin::glDrawColouredCube();
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
    std::string name_;
    pangolin::View* view_;
    pangolin::OpenGlRenderState render_state_;
    std::unique_ptr<pangolin::Var<bool>> show_view_;
};

std::unique_ptr<Viewport> createReconstructionViewport(const std::string& name, float aspect_ratio,
                                                        const pangolin::OpenGlRenderState& render_state) {
    return std::make_unique<ReconstructionViewport>(name, aspect_ratio, render_state);
}

} // namespace viewportal
