#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <pangolin/display/view.h>
#include <memory>
#include <string>

namespace viewportal {

/**
 * Base class for viewports in the Pangolin GUI.
 * Provides a common interface for different types of viewports.
 */
class Viewport {
public:
    virtual ~Viewport() = default;

    /**
     * Get the Pangolin view for this viewport.
     */
    virtual pangolin::View& getView() = 0;

    /**
     * Update view data (called once per frame before render).
     * Default no-op; override in viewports that need per-frame updates.
     */
    virtual void update() {}

    /**
     * Render the viewport content.
     * Called every frame when the viewport is shown.
     */
    virtual void render() = 0;

    /**
     * Get the name/identifier of this viewport.
     */
    virtual std::string getName() const = 0;

    /**
     * Check if this viewport should be shown.
     */
    virtual bool isShown() const { return true; }

    /**
     * Set up UI controls for this viewport.
     * Called after the viewport is created to add buttons, sliders, etc. to the left menu panel.
     * Controls should use the "ui.<name>.<control_name>" naming convention.
     */
    virtual void setupUI() {}

    /**
     * Optional key press handler (e.g. 'p' for plot pause).
     * Return true if the key was handled.
     */
    virtual bool onKeyPress(int key) { (void)key; return false; }
};

/**
 * Factory function to create different types of viewports.
 */
std::unique_ptr<Viewport> createViewport(
    const std::string& type,
    const std::string& name,
    float aspect_ratio = 640.0f / 480.0f
);

} // namespace viewportal

#endif // VIEWPORT_H
