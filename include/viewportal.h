#ifndef VIEWPORTAL_H
#define VIEWPORTAL_H

#include <vector>
#include <cstddef>

namespace viewportal {

/**
 * Type of each viewport in the grid.
 */
enum class ViewportType {
    ColorImage,
    DepthImage,
    Reconstruction,
    Plot,
    Count  // for bounds
};

/**
 * Pixel format for image frame data.
 */
enum class ImageFormat {
    RGB8,
    RGBA8,
    Luminance8
};

/**
 * Non-owning descriptor for a single image frame.
 * User provides a pointer to image data; the buffer is not copied.
 */
struct FrameData {
    int width = 0;
    int height = 0;
    ImageFormat format = ImageFormat::RGB8;
    const void* data = nullptr;
    int row_stride = 0;  // 0 means packed (width * bytes_per_pixel per row)
};

/**
 * Optional construction parameters for ViewPortal.
 */
struct ViewPortalParams {
    int window_width = 1280;
    int window_height = 720;
    int panel_width = 200;
    const char* window_title = "ViewPortal";
};

/**
 * Public facade for a multi-viewport display.
 * User defines layout (rows, columns), viewport types, and updates image
 * viewports via updateFrame(). No Pangolin or backend types in the API.
 */
class ViewPortal {
public:
    /**
     * Create a display with a grid of viewports, loading window size and panel from config/params.cfg.
     * \param rows Number of rows in the grid.
     * \param cols Number of columns in the grid.
     * \param types Viewport type for each cell (row-major order). types.size() must equal rows * cols.
     * \param window_title Window title string.
     */
    ViewPortal(int rows, int cols, const std::vector<ViewportType>& types, const char* window_title);

    /**
     * Create a display with a grid of viewports using the given parameters.
     * \param rows Number of rows in the grid.
     * \param cols Number of columns in the grid.
     * \param types Viewport type for each cell (row-major order). types.size() must equal rows * cols.
     * \param params Window size, panel width, and title.
     */
    ViewPortal(int rows, int cols, const std::vector<ViewportType>& types,
               const ViewPortalParams& params);

    ~ViewPortal();

    ViewPortal(const ViewPortal&) = delete;
    ViewPortal& operator=(const ViewPortal&) = delete;

    /**
     * Set the next frame to display in an image viewport (ColorImage or DepthImage).
     * No-op for other viewport types. Data is used at next step().
     */
    void updateFrame(size_t viewportIndex, const FrameData& frame);

    /**
     * Run one display frame: update viewports, render, swap.
     */
    void step();

    /**
     * Return true if the user requested to close the window.
     */
    bool shouldQuit() const;

private:
    void init(int rows, int cols, const std::vector<ViewportType>& types);

    struct Impl;
    Impl* impl_;
};

} // namespace viewportal

#endif // VIEWPORTAL_H
