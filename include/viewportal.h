#ifndef VIEWPORTAL_H
#define VIEWPORTAL_H

#include <vector>
#include <cstddef>

namespace viewportal {

/**
 * Type of each viewport in the grid.
 */
enum class ViewportType {
    RGB8,          // RGB or RGBA image (ColorImage-style)
    G8,            // Grayscale / single-channel (Luminance8)
    ColoredDepth,  // G8 input displayed with colormap (e.g. jet)
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
 * When passed to updateFrame(), the library copies the pixel data; the caller
 * may reuse or free the buffer immediately after updateFrame() returns.
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
     * Set the next frame to display in an image viewport (RGB8 or G8).
     * Takes a copy of the frame data; the display runs on its own thread and shows
     * the latest copied frame. No-op for other viewport types.
     */
    void updateFrame(size_t viewportIndex, const FrameData& frame);

    /**
     * Return true if the user requested to close the window (thread-safe).
     * The display runs on its own thread; use this in the app loop to exit.
     */
    bool shouldQuit() const;

    /**
     * Non-blocking one-shot key check. Pass the key to watch (e.g. 's').
     * Returns true once when that key was pressed in the GUI (then consumed);
     * returns false otherwise. Thread-safe.
     * Only keys passed to setKeysToWatch() can return true.
     */
    bool checkKey(int key) const;

    /**
     * Register key codes for checkKey(). Call after construction; pass e.g. {'s', ' '}.
     * Keys not in this list will never be seen by checkKey().
     */
    void setKeysToWatch(const std::vector<int>& keys);

private:
    struct Impl;
    Impl* impl_;

    static void displayThreadMain(Impl* impl);
    static void initOnDisplayThread(Impl* impl);
    static void stepFrame(Impl* impl);
};

} // namespace viewportal

#endif // VIEWPORTAL_H
