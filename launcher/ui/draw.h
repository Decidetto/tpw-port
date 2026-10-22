// draw.h: the launcher screen's drawing primitives on SDL_Renderer. Everything is in points; the canvas sets the
// renderer's scale to the window's pixel density, so the same layout is sharp at 1x and 2x. Shapes are triangles
// (SDL_RenderGeometry) with a one-pixel feather of fading alpha along their edges, which smooths them on every renderer,
// the software one included.
#pragma once
#include <SDL3/SDL.h>

#include <vector>

#include "theme.h"

namespace tpw::ui {

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    float Right() const { return x + w; }
    float Bottom() const { return y + h; }
    float CentreX() const { return x + w * 0.5f; }
    float CentreY() const { return y + h * 0.5f; }
    bool Contains(float px, float py) const { return px >= x && py >= y && px < x + w && py < y + h; }
    Rect Inset(float d) const { return {x + d, y + d, w - 2 * d, h - 2 * d}; }
    Rect Inset(float dx, float dy) const { return {x + dx, y + dy, w - 2 * dx, h - 2 * dy}; }
    Rect Moved(float dx, float dy) const { return {x + dx, y + dy, w, h}; }
    // The top or the bottom `height` points of the rectangle.
    Rect TakeTop(float height) const { return {x, y, w, height}; }
    Rect TakeBottom(float height) const { return {x, y + h - height, w, height}; }
};

struct Point {
    float x, y;
};

class Canvas {
public:
    explicit Canvas(SDL_Renderer* renderer) : renderer_(renderer) {}
    SDL_Renderer* Renderer() const { return renderer_; }

    // Starts a frame: the renderer's scale becomes `scale` (pixels per point) and blending is on.
    void Begin(float scale);
    float Scale() const { return scale_; }
    float Pixel() const { return 1.0f / scale_; }   // one pixel, in points

    void Clear(Rgba colour);
    void VerticalGradient(const Rect& r, Rgba top, Rgba bottom);
    // A rounded rectangle; `feather` is the width of the edge's fade (0: one pixel).
    void FillRound(const Rect& r, float radius, Rgba colour, float feather = 0);
    void FillRoundGradient(const Rect& r, float radius, Rgba top, Rgba bottom);
    // A soft shadow under a rounded rectangle: the shape grown by blur/2 and faded across `blur` points.
    void Shadow(const Rect& r, float radius, float blur, Rgba colour);
    // A rounded rectangle with a dark outline `outline` points wide, drawn inside `r`.
    void Outlined(const Rect& r, float radius, float outline, Rgba fill, Rgba line);
    void OutlinedGradient(const Rect& r, float radius, float outline, Rgba top, Rgba bottom, Rgba line);
    // A dashed outline along a rounded rectangle (the drop field's).
    void DashedRound(const Rect& r, float radius, float width, float dash, float gap, Rgba colour);
    void FillCircle(float cx, float cy, float radius, Rgba colour);
    void FillTriangle(Point a, Point b, Point c, Rgba colour);
    // A thick line through the points, with round joints and ends.
    void Polyline(const std::vector<Point>& points, float width, Rgba colour);
    void CheckMark(float cx, float cy, float size, float width, Rgba colour);

private:
    // A rounded rectangle's contour, clockwise from the top left corner, `segments` + 1 points per corner.
    static void Contour(const Rect& r, float radius, int segments, std::vector<Point>& out);
    int Segments(float radius) const;
    // Fills a rounded rectangle and fades it out across `feather` points around its edge (half inside, half outside).
    // The colour goes from `top` at y = gradTop to `bottom` at y = gradTop + gradHeight.
    void RoundShape(const Rect& r, float radius, float feather, Rgba top, Rgba bottom, float gradTop, float gradHeight);
    // A quad from a to b, `width` wide, with feathered long sides.
    void Segment(Point a, Point b, float width, Rgba colour);
    int Vertex(Point p, Rgba c);
    void Submit();

    SDL_Renderer* renderer_;
    float scale_ = 1;
    std::vector<SDL_Vertex> vertices_;
    std::vector<int> indices_;
};

SDL_FColor ToFColor(Rgba c);
Rgba Mix(Rgba a, Rgba b, float t);   // t = 0: a, t = 1: b

}  // namespace tpw::ui
