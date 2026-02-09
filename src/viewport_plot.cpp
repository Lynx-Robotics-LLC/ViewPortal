#include "viewport.h"
#include <pangolin/display/display.h>
#include <pangolin/plot/plotter.h>
#include <pangolin/plot/datalog.h>
#include <pangolin/var/var.h>
#include <cmath>
#include <memory>

namespace viewportal {

class PlotViewport : public Viewport {
public:
    PlotViewport(const std::string& name, float aspect_ratio)
        : name_(name),
          plot_paused_(false),
          x_(0.0f),
          xinc_(0.01f) {
        view_ = &pangolin::Display(name).SetAspect(aspect_ratio);
        plotLog_.SetLabels({"x", "Plot 1", "Plot 2"});
        plotter_ = std::make_unique<pangolin::Plotter>(&plotLog_, 0.0f, 4.0f * (float)M_PI, -2.0f, 2.0f, (float)M_PI / 4.0f, 0.5f);
        plotter_->ClearSeries();
        plotter_->AddSeries("$0", "$1", pangolin::DrawingModeLine, pangolin::Colour::Blue(), "Plot 1");
        plotter_->AddSeries("$0", "$2", pangolin::DrawingModeLine, pangolin::Colour::Red(), "Plot 2");
        plotter_->SetBounds(0.0, 1.0, 0.0, 1.0);
        view_->AddDisplay(*plotter_);
    }

    ~PlotViewport() override = default;

    pangolin::View& getView() override { return *view_; }
    std::string getName() const override { return name_; }

    void update() override {
        if (amplitude1_ && frequency1_ && amplitude2_ && frequency2_ && !plot_paused_) {
            double amp1 = amplitude1_->Get();
            double freq1 = frequency1_->Get();
            double amp2 = amplitude2_->Get();
            double freq2 = frequency2_->Get();
            float sin1 = (float)(amp1 * sin(freq1 * x_));
            float sin2 = (float)(amp2 * sin(freq2 * x_));
            plotLog_.Log(x_, sin1, sin2);
            x_ += xinc_;
        }
        if (plotter_) plotter_->SetBounds(0.0, 1.0, 0.0, 1.0);
    }

    void render() override {
        if (view_->IsShown()) {
            view_->Activate();
            if (plotter_) plotter_->Render();
        }
    }

    void setupUI() override {
        std::string prefix = "ui." + name_ + ".";
        show_view_ = std::make_unique<pangolin::Var<bool>>(prefix + "Show", true, true);
        amplitude1_ = std::make_unique<pangolin::Var<double>>(prefix + "Amplitude_1", 1.0, 0.1, 5.0);
        frequency1_ = std::make_unique<pangolin::Var<double>>(prefix + "Frequency_1", 1.0, 0.1, 10.0);
        amplitude2_ = std::make_unique<pangolin::Var<double>>(prefix + "Amplitude_2", 1.0, 0.1, 5.0);
        frequency2_ = std::make_unique<pangolin::Var<double>>(prefix + "Frequency_2", 2.0, 0.1, 10.0);
        pause_button_ = std::make_unique<pangolin::Var<std::function<void(void)>>>(prefix + "Toggle_Pause", [this]() {
            plot_paused_ = !plot_paused_;
        });
    }

    bool isShown() const override {
        return show_view_ && show_view_->Get();
    }

    bool onKeyPress(int key) override {
        if (key == 'p') {
            plot_paused_ = !plot_paused_;
            return true;
        }
        return false;
    }

    void togglePause() { plot_paused_ = !plot_paused_; }
    bool isPaused() const { return plot_paused_; }

private:
    std::string name_;
    pangolin::View* view_;
    pangolin::DataLog plotLog_;
    std::unique_ptr<pangolin::Plotter> plotter_;
    bool plot_paused_;
    float x_;
    const float xinc_ = 0.01f;
    std::unique_ptr<pangolin::Var<bool>> show_view_;
    std::unique_ptr<pangolin::Var<double>> amplitude1_;
    std::unique_ptr<pangolin::Var<double>> frequency1_;
    std::unique_ptr<pangolin::Var<double>> amplitude2_;
    std::unique_ptr<pangolin::Var<double>> frequency2_;
    std::unique_ptr<pangolin::Var<std::function<void(void)>>> pause_button_;
};

std::unique_ptr<Viewport> createPlotViewport(const std::string& name, float aspect_ratio) {
    return std::make_unique<PlotViewport>(name, aspect_ratio);
}

} // namespace viewportal
