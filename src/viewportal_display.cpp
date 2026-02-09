#include "viewportal.h"
#include "viewportal_params.h"
#include "viewport.h"
#include <pangolin/var/var.h>
#include <pangolin/gl/gl.h>
#include <pangolin/display/display.h>
#include <pangolin/display/view.h>
#include <pangolin/display/widgets.h>
#include <pangolin/handler/handler.h>
#include <vector>
#include <memory>
#include <array>
#include <functional>
#include <chrono>
#include <cmath>
#include <string>
#include <stdexcept>

namespace viewportal {

namespace {

constexpr float kDefaultAspect = 640.0f / 480.0f;

struct DoubleClickFullscreenHandler : pangolin::Handler {
    static constexpr double kDoubleClickTimeSec = 0.35;
    static constexpr int kDoubleClickSlopPx = 8;

    std::function<void(int view_id)> on_double_click;
    double last_click_time = 0.0;
    int last_click_x = 0;
    int last_click_y = 0;
    int last_click_view_id = 0;

    void Mouse(pangolin::View& view, pangolin::MouseButton button, int x, int y, bool pressed, int button_state) override {
        (void)button_state;
        if (button == pangolin::MouseButtonLeft && pressed && on_double_click) {
            int view_id = 0;
            for (size_t i = 0; i < view.NumChildren(); ++i) {
                if (view[i].IsShown() && view[i].GetBounds().Contains(x, y)) {
                    view_id = static_cast<int>(i) + 1;
                    break;
                }
            }
            if (view_id != 0) {
                double t = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
                if (last_click_time > 0.0 &&
                    (t - last_click_time) < kDoubleClickTimeSec &&
                    last_click_view_id == view_id &&
                    std::abs(x - last_click_x) <= kDoubleClickSlopPx &&
                    std::abs(y - last_click_y) <= kDoubleClickSlopPx) {
                    on_double_click(view_id);
                    last_click_time = 0.0;
                    return;
                }
                last_click_time = t;
                last_click_x = x;
                last_click_y = y;
                last_click_view_id = view_id;
            }
        }
        pangolin::Handler::Mouse(view, button, x, y, pressed, button_state);
    }
};

} // namespace

struct ViewPortal::Impl {
    ViewPortalParams params;
    std::string window_title_storage;  // when non-empty, params.window_title points into this
    std::vector<std::unique_ptr<Viewport>> viewports;
    int fullscreen_view = 0;
    bool state_saved = false;
    std::vector<pangolin::Attach> saved_top, saved_left, saved_right, saved_bottom;
    std::vector<bool> saved_visible;
    std::unique_ptr<DoubleClickFullscreenHandler> double_click_handler;
    std::string window_name;

    void saveCurrentState() {
        const size_t n = viewports.size();
        saved_top.resize(n);
        saved_left.resize(n);
        saved_right.resize(n);
        saved_bottom.resize(n);
        saved_visible.resize(n);
        for (size_t i = 0; i < n; ++i) {
            pangolin::View& v = viewports[i]->getView();
            saved_top[i] = v.top;
            saved_left[i] = v.left;
            saved_right[i] = v.right;
            saved_bottom[i] = v.bottom;
            saved_visible[i] = v.IsShown();
        }
        state_saved = true;
    }

    void exitFullscreen() {
        if (fullscreen_view != 0 && state_saved) {
            for (size_t i = 0; i < viewports.size(); ++i) {
                pangolin::View& v = viewports[i]->getView();
                v.SetBounds(saved_bottom[i], saved_top[i], saved_left[i], saved_right[i]);
                v.Show(saved_visible[i]);
            }
            fullscreen_view = 0;
            state_saved = false;
        }
    }

    void enterFullscreen(int view_id) {
        if (fullscreen_view != 0 && state_saved) {
            exitFullscreen();
        }
        if (!state_saved) {
            saveCurrentState();
        }
        size_t idx = static_cast<size_t>(view_id - 1);
        if (idx < viewports.size()) {
            viewports[idx]->getView().SetBounds(0.0, 1.0, pangolin::Attach::Pix(params.panel_width), 1.0);
        }
        for (size_t i = 0; i < viewports.size(); ++i) {
            viewports[i]->getView().Show(i == idx);
        }
        fullscreen_view = view_id;
    }
};

ViewPortal::ViewPortal(int rows, int cols, const std::vector<ViewportType>& types, const char* window_title)
    : impl_(new Impl)
{
    LoadedParams loaded = loadParams();
    impl_->params = loaded.viewportal;
    impl_->window_title_storage = window_title ? window_title : "";
    impl_->params.window_title = impl_->window_title_storage.c_str();
    try {
        init(rows, cols, types);
    } catch (...) {
        delete impl_;
        impl_ = nullptr;
        throw;
    }
}

ViewPortal::ViewPortal(int rows, int cols, const std::vector<ViewportType>& types,
                       const ViewPortalParams& params)
    : impl_(new Impl)
{
    impl_->params = params;
    try {
        init(rows, cols, types);
    } catch (...) {
        delete impl_;
        impl_ = nullptr;
        throw;
    }
}

void ViewPortal::init(int rows, int cols, const std::vector<ViewportType>& types) {
    const ViewPortalParams& params = impl_->params;
    impl_->window_name = params.window_title;

    const size_t n = static_cast<size_t>(rows * cols);
    if (types.size() != n) {
        throw std::invalid_argument("ViewPortal: types.size() must equal rows * cols");
    }

    pangolin::CreateWindowAndBind(params.window_title, params.window_width, params.window_height);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    pangolin::Display("multi")
        .SetBounds(0.0, 1.0, pangolin::Attach::Pix(params.panel_width), 1.0)
        .SetLayout(pangolin::LayoutEqual);

    const float aspect = kDefaultAspect;
    for (size_t i = 0; i < n; ++i) {
        std::string name = "v" + std::to_string(i);
        impl_->viewports.push_back(createViewport(types[i], name, aspect));
    }

    for (auto& v : impl_->viewports) {
        pangolin::Display("multi").AddDisplay(v->getView());
    }

    pangolin::CreatePanel("ui")
        .SetBounds(0.0, 1.0, 0.0, pangolin::Attach::Pix(params.panel_width));

    for (auto& v : impl_->viewports) {
        v->setupUI();
    }

    pangolin::RegisterKeyPressCallback('`', []() {
        pangolin::ShowConsole(pangolin::TrueFalseToggle::Toggle);
    });
    pangolin::RegisterKeyPressCallback('f', []() {
        pangolin::ShowFullscreen(pangolin::TrueFalseToggle::Toggle);
    });

    impl_->double_click_handler = std::make_unique<DoubleClickFullscreenHandler>();
    impl_->double_click_handler->on_double_click = [this](int view_id) {
        if (impl_->fullscreen_view == view_id) {
            impl_->exitFullscreen();
        } else {
            impl_->enterFullscreen(view_id);
        }
    };
    pangolin::Display("multi").SetHandler(impl_->double_click_handler.get());

    pangolin::RegisterKeyPressCallback('p', [this]() {
        for (auto& v : impl_->viewports) {
            if (v->onKeyPress('p')) break;
        }
    });
}

ViewPortal::~ViewPortal() {
    if (impl_) {
        impl_->viewports.clear();
        pangolin::DestroyWindow(impl_->window_name);
    }
    delete impl_;
    impl_ = nullptr;
}

void ViewPortal::updateFrame(size_t viewportIndex, const FrameData& frame) {
    if (!impl_ || viewportIndex >= impl_->viewports.size()) return;
    impl_->viewports[viewportIndex]->setFrame(frame);
}

void ViewPortal::step() {
    if (!impl_) return;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (auto& v : impl_->viewports) {
        if (v->isShown() && v->getView().IsShown()) {
            v->update();
            v->render();
        }
    }
    pangolin::FinishFrame();
}

bool ViewPortal::shouldQuit() const {
    return impl_ ? pangolin::ShouldQuit() : true;
}

} // namespace viewportal
