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
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <set>

namespace viewportal {

namespace {

constexpr float kDefaultAspect = 640.0f / 480.0f;

static int bytesPerPixel(ImageFormat fmt) {
    switch (fmt) {
        case ImageFormat::RGB8: return 3;
        case ImageFormat::RGBA8: return 4;
        case ImageFormat::Luminance8: return 1;
        default: return 3;
    }
}

static size_t frameByteSize(const FrameData& f) {
    if (f.row_stride != 0)
        return static_cast<size_t>(f.height) * static_cast<size_t>(f.row_stride);
    return static_cast<size_t>(f.width) * static_cast<size_t>(f.height) * bytesPerPixel(f.format);
}

static bool isImageViewport(ViewportType t) {
    return t == ViewportType::RGB8 || t == ViewportType::G8 || t == ViewportType::ColoredDepth;
}

struct ViewportFrameState {
    std::vector<std::uint8_t> buffers[2];
    int width[2] = {0, 0};
    int height[2] = {0, 0};
    ImageFormat format[2] = {ImageFormat::RGB8, ImageFormat::RGB8};
    std::atomic<int> write_index{0};
    std::mutex mutex;
};

struct DoubleClickFullscreenHandler : pangolin::Handler {
    static constexpr double kDoubleClickTimeSec = 0.35;
    static constexpr int kDoubleClickSlopPx = 8;

    std::function<void(int view_id)> on_double_click;
    std::function<void(int)> on_key_press;
    double last_click_time = 0.0;
    int last_click_x = 0;
    int last_click_y = 0;
    int last_click_view_id = 0;

    void Keyboard(pangolin::View& view, unsigned char key, int x, int y, bool pressed) override {
        if (pressed && on_key_press)
            on_key_press(static_cast<int>(key));
        pangolin::Handler::Keyboard(view, key, x, y, pressed);
    }

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
    std::vector<ViewportType> viewport_types;  // stored for updateFrame image check
    std::vector<std::unique_ptr<Viewport>> viewports;
    std::vector<std::unique_ptr<ViewportFrameState>> frame_states;  // one per viewport; used only for image viewports
    int fullscreen_view = 0;
    bool state_saved = false;
    std::vector<pangolin::Attach> saved_top, saved_left, saved_right, saved_bottom;
    std::vector<bool> saved_visible;
    std::unique_ptr<DoubleClickFullscreenHandler> double_click_handler;
    std::string window_name;

    std::thread display_thread;
    std::atomic<bool> quit_requested{false};
    std::atomic<bool> init_done{false};
    std::mutex init_mutex;
    std::condition_variable init_cv;

    mutable std::mutex key_mutex;
    mutable std::set<int> pending_keys;

    int init_rows = 0;
    int init_cols = 0;
    std::vector<int> keys_to_watch;
    bool keys_to_watch_pending = false;
    std::set<int> keys_registered;  // only touched on display thread

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
            viewports[static_cast<size_t>(fullscreen_view - 1)]->getView().SetAspect(static_cast<double>(kDefaultAspect));
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
            pangolin::View& v = viewports[idx]->getView();
            // Bounds (0,1,0,1) = fill parent "multi" (area right of panel); no extra left margin
            v.SetBounds(0.0, 1.0, 0.0, 1.0);
            // Negative aspect: fill width, letterbox top/bottom (avoids left/right borders on wide windows)
            v.SetAspect(-static_cast<double>(kDefaultAspect));
        }
        for (size_t i = 0; i < viewports.size(); ++i) {
            viewports[i]->getView().Show(i == idx);
        }
        fullscreen_view = view_id;
    }
};

void ViewPortal::initOnDisplayThread(Impl* impl) {
    const ViewPortalParams& params = impl->params;
    impl->window_name = params.window_title;
    const int rows = impl->init_rows;
    const int cols = impl->init_cols;
    const std::vector<ViewportType>& types = impl->viewport_types;
    const size_t n = static_cast<size_t>(rows * cols);

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
        impl->viewports.push_back(createViewport(types[i], name, aspect));
    }

    for (auto& v : impl->viewports) {
        pangolin::Display("multi").AddDisplay(v->getView());
    }

    pangolin::CreatePanel("ui")
        .SetBounds(0.0, 1.0, 0.0, pangolin::Attach::Pix(params.panel_width));

    for (auto& v : impl->viewports) {
        v->setupUI();
    }

    pangolin::RegisterKeyPressCallback('`', []() {
        pangolin::ShowConsole(pangolin::TrueFalseToggle::Toggle);
    });
    pangolin::RegisterKeyPressCallback('f', []() {
        pangolin::ShowFullscreen(pangolin::TrueFalseToggle::Toggle);
    });

    impl->double_click_handler = std::make_unique<DoubleClickFullscreenHandler>();
    impl->double_click_handler->on_double_click = [impl](int view_id) {
        if (impl->fullscreen_view == view_id) {
            impl->exitFullscreen();
        } else {
            impl->enterFullscreen(view_id);
        }
    };
    impl->double_click_handler->on_key_press = [impl](int key) {
        std::lock_guard<std::mutex> lock(impl->key_mutex);
        impl->pending_keys.insert(key);
    };
    pangolin::Display("multi").SetHandler(impl->double_click_handler.get());

    pangolin::RegisterKeyPressCallback('p', [impl]() {
        for (auto& v : impl->viewports) {
            if (v->onKeyPress('p')) break;
        }
    });

}

void ViewPortal::stepFrame(Impl* impl) {
    {
        std::lock_guard<std::mutex> lock(impl->key_mutex);
        if (impl->keys_to_watch_pending) {
            impl->keys_to_watch_pending = false;
            for (int key : impl->keys_to_watch) {
                if (impl->keys_registered.count(key) == 0) {
                    impl->keys_registered.insert(key);
                    pangolin::RegisterKeyPressCallback(key, [impl, key]() {
                        std::lock_guard<std::mutex> lk(impl->key_mutex);
                        impl->pending_keys.insert(key);
                    });
                }
            }
        }
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const size_t n = impl->viewports.size();
    for (size_t i = 0; i < n; ++i) {
        Viewport* v = impl->viewports[i].get();
        if (!v->isShown() || !v->getView().IsShown())
            continue;
        if (i < impl->frame_states.size() && isImageViewport(impl->viewport_types[i])) {
            ViewportFrameState& fs = *impl->frame_states[i];
            const int read_index = 1 - fs.write_index.load(std::memory_order_acquire);
            if (fs.width[read_index] > 0 && fs.height[read_index] > 0 &&
                fs.buffers[read_index].size() > 0) {
                FrameData fd;
                fd.width = fs.width[read_index];
                fd.height = fs.height[read_index];
                fd.format = fs.format[read_index];
                fd.data = fs.buffers[read_index].data();
                fd.row_stride = 0;
                v->setFrame(fd);
            }
        }
        v->update();
        v->render();
    }
    pangolin::FinishFrame();
}

void ViewPortal::displayThreadMain(Impl* impl) {
    try {
        initOnDisplayThread(impl);
    } catch (...) {
        impl->init_done = true;
        impl->init_cv.notify_one();
        impl->quit_requested = true;
        return;
    }
    {
        std::lock_guard<std::mutex> lock(impl->init_mutex);
        impl->init_done = true;
    }
    impl->init_cv.notify_one();

    while (!impl->quit_requested && !pangolin::ShouldQuit()) {
        stepFrame(impl);
    }
    impl->quit_requested = true;

    impl->viewports.clear();
    impl->double_click_handler.reset();
    pangolin::DestroyWindow(impl->window_name);
}

ViewPortal::ViewPortal(int rows, int cols, const std::vector<ViewportType>& types, const char* window_title)
    : impl_(new Impl)
{
    LoadedParams loaded = loadParams();
    impl_->params = loaded.viewportal;
    impl_->window_title_storage = window_title ? window_title : "";
    impl_->params.window_title = impl_->window_title_storage.c_str();
    const size_t n = static_cast<size_t>(rows * cols);
    if (types.size() != n) {
        delete impl_;
        impl_ = nullptr;
        throw std::invalid_argument("ViewPortal: types.size() must equal rows * cols");
    }
    impl_->init_rows = rows;
    impl_->init_cols = cols;
    impl_->viewport_types = types;
    impl_->frame_states.resize(n);
    for (size_t i = 0; i < n; ++i)
        impl_->frame_states[i] = std::make_unique<ViewportFrameState>();
    impl_->display_thread = std::thread(&ViewPortal::displayThreadMain, impl_);
    {
        std::unique_lock<std::mutex> lock(impl_->init_mutex);
        impl_->init_cv.wait(lock, [this]() { return impl_->init_done.load(); });
    }
    if (impl_->quit_requested) {
        if (impl_->display_thread.joinable())
            impl_->display_thread.join();
        delete impl_;
        impl_ = nullptr;
        throw std::runtime_error("ViewPortal: display thread failed to initialize");
    }
}

ViewPortal::ViewPortal(int rows, int cols, const std::vector<ViewportType>& types,
                       const ViewPortalParams& params)
    : impl_(new Impl)
{
    impl_->params = params;
    const size_t n = static_cast<size_t>(rows * cols);
    if (types.size() != n) {
        delete impl_;
        impl_ = nullptr;
        throw std::invalid_argument("ViewPortal: types.size() must equal rows * cols");
    }
    impl_->init_rows = rows;
    impl_->init_cols = cols;
    impl_->viewport_types = types;
    impl_->frame_states.resize(n);
    for (size_t i = 0; i < n; ++i)
        impl_->frame_states[i] = std::make_unique<ViewportFrameState>();
    impl_->display_thread = std::thread(&ViewPortal::displayThreadMain, impl_);
    {
        std::unique_lock<std::mutex> lock(impl_->init_mutex);
        impl_->init_cv.wait(lock, [this]() { return impl_->init_done.load(); });
    }
    if (impl_->quit_requested) {
        if (impl_->display_thread.joinable())
            impl_->display_thread.join();
        delete impl_;
        impl_ = nullptr;
        throw std::runtime_error("ViewPortal: display thread failed to initialize");
    }
}

ViewPortal::~ViewPortal() {
    if (impl_) {
        impl_->quit_requested = true;
        if (impl_->display_thread.joinable())
            impl_->display_thread.join();
    }
    delete impl_;
    impl_ = nullptr;
}

void ViewPortal::updateFrame(size_t viewportIndex, const FrameData& frame) {
    if (!impl_ || viewportIndex >= impl_->frame_states.size()) return;
    if (!isImageViewport(impl_->viewport_types[viewportIndex])) return;
    if (!frame.data || frame.width <= 0 || frame.height <= 0) return;

    ViewportFrameState& fs = *impl_->frame_states[viewportIndex];
    const size_t byte_size = frameByteSize(frame);
    if (byte_size == 0) return;

    std::lock_guard<std::mutex> lock(fs.mutex);
    const int w = fs.write_index.load(std::memory_order_relaxed);
    if (fs.buffers[w].size() < byte_size)
        fs.buffers[w].resize(byte_size);
    const int bpp = bytesPerPixel(frame.format);
    const size_t row_bytes = static_cast<size_t>(frame.width) * bpp;
    if (frame.row_stride != 0) {
        const int stride = frame.row_stride;
        const std::uint8_t* src = static_cast<const std::uint8_t*>(frame.data);
        for (int y = 0; y < frame.height; ++y)
            std::memcpy(fs.buffers[w].data() + static_cast<size_t>(y) * row_bytes, src + static_cast<size_t>(y) * stride, row_bytes);
    } else {
        std::memcpy(fs.buffers[w].data(), frame.data, byte_size);
    }
    fs.width[w] = frame.width;
    fs.height[w] = frame.height;
    fs.format[w] = frame.format;
    fs.write_index.store(1 - w, std::memory_order_release);
}

bool ViewPortal::shouldQuit() const {
    return impl_ ? impl_->quit_requested.load(std::memory_order_acquire) : true;
}

bool ViewPortal::checkKey(int key) const {
    if (!impl_) return false;
    std::lock_guard<std::mutex> lock(impl_->key_mutex);
    auto it = impl_->pending_keys.find(key);
    if (it != impl_->pending_keys.end()) {
        impl_->pending_keys.erase(it);
        return true;
    }
    return false;
}

void ViewPortal::setKeysToWatch(const std::vector<int>& keys) {
    if (!impl_) return;
    std::lock_guard<std::mutex> lock(impl_->key_mutex);
    impl_->keys_to_watch = keys;
    impl_->keys_to_watch_pending = true;
}

} // namespace viewportal
