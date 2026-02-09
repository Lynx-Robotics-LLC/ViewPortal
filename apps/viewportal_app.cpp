#include "viewport.h"
#include <pangolin/var/var.h>
#include <pangolin/gl/gl.h>
#include <pangolin/display/display.h>
#include <pangolin/display/view.h>
#include <pangolin/display/widgets.h>
#include <pangolin/display/default_font.h>
#include <pangolin/handler/handler.h>
#include <vector>
#include <memory>
#include <array>
#include <functional>
#include <chrono>
#include <cmath>

namespace {

// Handler that detects double-click on the multi view's children and invokes a callback (e.g. fullscreen).
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

int main(int /*argc*/, char* /*argv*/[])
{
    const float aspect_ratio = 640.0f / 480.0f;
    const int UI_WIDTH = 200;
    const size_t N_VIEWS = 4;

    pangolin::CreateWindowAndBind("ViewPortal Multi-View", 1280, 720);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    pangolin::Display("multi")
        .SetBounds(0.0, 1.0, pangolin::Attach::Pix(UI_WIDTH), 1.0)
        .SetLayout(pangolin::LayoutEqual);

    std::vector<std::unique_ptr<viewportal::Viewport>> viewports;
    viewports.push_back(viewportal::createViewport("color_camera", "color", aspect_ratio));
    viewports.push_back(viewportal::createViewport("depth_camera", "depth", aspect_ratio));
    viewports.push_back(viewportal::createViewport("reconstruction", "recon", aspect_ratio));
    viewports.push_back(viewportal::createViewport("plot", "plot", aspect_ratio));

    for (auto& viewport : viewports) {
        pangolin::Display("multi").AddDisplay(viewport->getView());
    }

    pangolin::CreatePanel("ui")
        .SetBounds(0.0, 1.0, 0.0, pangolin::Attach::Pix(UI_WIDTH));

    for (auto& viewport : viewports) {
        viewport->setupUI();
    }

    // Fullscreen state (matches MultiViewCamera: 0 = none, 1-4 = viewport index)
    int fullscreen_view = 0;
    bool state_saved = false;
    std::array<pangolin::Attach, N_VIEWS> saved_top, saved_left, saved_right, saved_bottom;
    std::array<bool, N_VIEWS> saved_visible;

    auto saveCurrentState = [&]() {
        for (size_t i = 0; i < N_VIEWS && i < viewports.size(); ++i) {
            pangolin::View& v = viewports[i]->getView();
            saved_top[i] = v.top;
            saved_left[i] = v.left;
            saved_right[i] = v.right;
            saved_bottom[i] = v.bottom;
            saved_visible[i] = v.IsShown();
        }
        state_saved = true;
    };

    auto exitFullscreen = [&]() {
        if (fullscreen_view != 0 && state_saved) {
            for (size_t i = 0; i < N_VIEWS && i < viewports.size(); ++i) {
                pangolin::View& v = viewports[i]->getView();
                v.SetBounds(saved_bottom[i], saved_top[i], saved_left[i], saved_right[i]);
                v.Show(saved_visible[i]);
            }
            fullscreen_view = 0;
            state_saved = false;
        }
    };

    auto enterFullscreen = [&](int view_id) {
        if (fullscreen_view != 0 && state_saved) {
            exitFullscreen();
        }
        if (!state_saved) {
            saveCurrentState();
        }
        size_t idx = static_cast<size_t>(view_id - 1);
        if (idx < viewports.size()) {
            // Keep view's existing aspect ratio (no SetAspect(0)) so image is never stretched
            viewports[idx]->getView().SetBounds(0.0, 1.0, pangolin::Attach::Pix(UI_WIDTH), 1.0);
        }
        for (size_t i = 0; i < viewports.size(); ++i) {
            viewports[i]->getView().Show(i == idx);
        }
        fullscreen_view = view_id;
    };

    // Key callbacks
    pangolin::RegisterKeyPressCallback('`', []() {
        pangolin::ShowConsole(pangolin::TrueFalseToggle::Toggle);
    });
    pangolin::RegisterKeyPressCallback('f', []() {
        pangolin::ShowFullscreen(pangolin::TrueFalseToggle::Toggle);
    });
    pangolin::RegisterKeyPressCallback('p', [&]() {
        for (auto& v : viewports) {
            if (v->onKeyPress('p')) break;
        }
    });

    // Double-click on a viewport to fullscreen it (or exit fullscreen if already fullscreened)
    DoubleClickFullscreenHandler double_click_handler;
    double_click_handler.on_double_click = [&](int view_id) {
        if (fullscreen_view == view_id) {
            exitFullscreen();
        } else {
            enterFullscreen(view_id);
        }
    };
    pangolin::Display("multi").SetHandler(&double_click_handler);

    while (!pangolin::ShouldQuit()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto& viewport : viewports) {
            if (viewport->isShown() && viewport->getView().IsShown()) {
                viewport->update();
                viewport->render();
            }
        }

        pangolin::FinishFrame();
    }

    viewports.clear();
    pangolin::DestroyWindow("ViewPortal Multi-View");

    return 0;
}
