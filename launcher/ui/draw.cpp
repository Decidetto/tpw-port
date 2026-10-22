// draw.cpp: the launcher screen's drawing primitives (draw.h).
#include "draw.h"

#include <algorithm>
#include <cmath>

namespace tpw::ui {

namespace {
constexpr float kPi = 3.14159265358979f;
}

SDL_FColor ToFColor(Rgba c) {
    return {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
}

Rgba Mix(Rgba a, Rgba b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto lerp = [t](uint8_t x, uint8_t y) { return static_cast<uint8_t>(std::lround(x + (y - x) * t)); };
    return {lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), lerp(a.a, b.a)};
}

void Canvas::Begin(float scale) {
    scale_ = scale > 0 ? scale : 1;
    SDL_SetRenderScale(renderer_, scale_, scale_);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
}

void Canvas::Clear(Rgba colour) {
    SDL_SetRenderDrawColor(renderer_, colour.r, colour.g, colour.b, colour.a);
    SDL_RenderClear(renderer_);
}

int Canvas::Vertex(Point p, Rgba c) {
    SDL_Vertex v;
    v.position = {p.x, p.y};
    v.color = ToFColor(c);
    v.tex_coord = {0, 0};
    vertices_.push_back(v);
    return static_cast<int>(vertices_.size()) - 1;
}

void Canvas::Submit() {
    if (!indices_.empty())
        SDL_RenderGeometry(renderer_, nullptr, vertices_.data(), static_cast<int>(vertices_.size()), indices_.data(),
                           static_cast<int>(indices_.size()));
    vertices_.clear();
    indices_.clear();
}

void Canvas::VerticalGradient(const Rect& r, Rgba top, Rgba bottom) {
    int a = Vertex({r.x, r.y}, top), b = Vertex({r.Right(), r.y}, top);
    int c = Vertex({r.Right(), r.Bottom()}, bottom), d = Vertex({r.x, r.Bottom()}, bottom);
    indices_.insert(indices_.end(), {a, b, c, a, c, d});
    Submit();
}

int Canvas::Segments(float radius) const {
    return std::clamp(static_cast<int>(std::ceil(radius * scale_ / 2.5f)), 2, 24);
}

void Canvas::Contour(const Rect& r, float radius, int segments, std::vector<Point>& out) {
    radius = std::clamp(radius, 0.0f, std::min(r.w, r.h) * 0.5f);
    out.clear();
    // Corner centres clockwise from the top left, each arc from the edge before it to the edge after it.
    const Point centres[4] = {{r.x + radius, r.y + radius},
                              {r.Right() - radius, r.y + radius},
                              {r.Right() - radius, r.Bottom() - radius},
                              {r.x + radius, r.Bottom() - radius}};
    for (int corner = 0; corner < 4; ++corner) {
        float start = kPi + corner * kPi * 0.5f;
        for (int i = 0; i <= segments; ++i) {
            float a = start + (kPi * 0.5f) * i / segments;
            out.push_back({centres[corner].x + std::cos(a) * radius, centres[corner].y + std::sin(a) * radius});
        }
    }
}

void Canvas::RoundShape(const Rect& r, float radius, float feather, Rgba top, Rgba bottom, float gradTop,
                        float gradHeight) {
    if (r.w <= 0 || r.h <= 0) return;
    if (feather <= 0) feather = Pixel();
    float half = feather * 0.5f;
    Rect innerRect = r.Inset(std::min(half, std::min(r.w, r.h) * 0.5f));
    Rect outerRect = r.Inset(-half);
    float outerRadius = std::min(radius, std::min(r.w, r.h) * 0.5f) + half;
    int segments = Segments(outerRadius);
    static thread_local std::vector<Point> inner, outer;
    Contour(innerRect, std::max(0.0f, radius - half), segments, inner);
    Contour(outerRect, outerRadius, segments, outer);
    auto colourAt = [&](float y) {
        if (gradHeight <= 0) return top;
        return Mix(top, bottom, (y - gradTop) / gradHeight);
    };
    int n = static_cast<int>(inner.size());
    int centre = Vertex({r.CentreX(), r.CentreY()}, colourAt(r.CentreY()));
    int firstInner = static_cast<int>(vertices_.size());
    for (const Point& p : inner) Vertex(p, colourAt(p.y));
    int firstOuter = static_cast<int>(vertices_.size());
    for (const Point& p : outer) {
        Rgba c = colourAt(p.y);
        c.a = 0;
        Vertex(p, c);
    }
    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        indices_.insert(indices_.end(), {centre, firstInner + i, firstInner + j});
        indices_.insert(indices_.end(), {firstInner + i, firstOuter + i, firstOuter + j});
        indices_.insert(indices_.end(), {firstInner + i, firstOuter + j, firstInner + j});
    }
    Submit();
}

void Canvas::FillRound(const Rect& r, float radius, Rgba colour, float feather) {
    RoundShape(r, radius, feather, colour, colour, 0, 0);
}

void Canvas::FillRoundGradient(const Rect& r, float radius, Rgba top, Rgba bottom) {
    RoundShape(r, radius, 0, top, bottom, r.y, r.h);
}

void Canvas::Shadow(const Rect& r, float radius, float blur, Rgba colour) {
    RoundShape(r, radius, blur, colour, colour, 0, 0);
}

void Canvas::Outlined(const Rect& r, float radius, float outline, Rgba fill, Rgba line) {
    FillRound(r, radius, line);
    FillRound(r.Inset(outline), std::max(0.0f, radius - outline), fill);
}

void Canvas::OutlinedGradient(const Rect& r, float radius, float outline, Rgba top, Rgba bottom, Rgba line) {
    FillRound(r, radius, line);
    Rect in = r.Inset(outline);
    RoundShape(in, std::max(0.0f, radius - outline), 0, top, bottom, in.y, in.h);
}

void Canvas::FillCircle(float cx, float cy, float radius, Rgba colour) {
    FillRound({cx - radius, cy - radius, 2 * radius, 2 * radius}, radius, colour);
}

void Canvas::FillTriangle(Point a, Point b, Point c, Rgba colour) {
    int i = Vertex(a, colour), j = Vertex(b, colour), k = Vertex(c, colour);
    indices_.insert(indices_.end(), {i, j, k});
    Submit();
}

void Canvas::Segment(Point a, Point b, float width, Rgba colour) {
    float dx = b.x - a.x, dy = b.y - a.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-4f) return;
    float nx = -dy / len, ny = dx / len;
    float half = width * 0.5f, f = Pixel() * 0.5f;
    float core = std::max(0.0f, half - f), edge = half + f;
    Rgba clear = colour.WithAlpha(0);
    int o1 = Vertex({a.x + nx * edge, a.y + ny * edge}, clear);
    int c1 = Vertex({a.x + nx * core, a.y + ny * core}, colour);
    int c2 = Vertex({a.x - nx * core, a.y - ny * core}, colour);
    int o2 = Vertex({a.x - nx * edge, a.y - ny * edge}, clear);
    int o3 = Vertex({b.x + nx * edge, b.y + ny * edge}, clear);
    int c3 = Vertex({b.x + nx * core, b.y + ny * core}, colour);
    int c4 = Vertex({b.x - nx * core, b.y - ny * core}, colour);
    int o4 = Vertex({b.x - nx * edge, b.y - ny * edge}, clear);
    indices_.insert(indices_.end(), {o1, o3, c3, o1, c3, c1, c1, c3, c4, c1, c4, c2, c2, c4, o4, c2, o4, o2});
}

void Canvas::Polyline(const std::vector<Point>& points, float width, Rgba colour) {
    for (size_t i = 0; i + 1 < points.size(); ++i) Segment(points[i], points[i + 1], width, colour);
    Submit();
    for (const Point& p : points) FillCircle(p.x, p.y, width * 0.5f, colour);
}

void Canvas::CheckMark(float cx, float cy, float size, float width, Rgba colour) {
    Polyline({{cx - 0.34f * size, cy + 0.02f * size}, {cx - 0.1f * size, cy + 0.26f * size},
              {cx + 0.36f * size, cy - 0.26f * size}},
             width, colour);
}

void Canvas::DashedRound(const Rect& r, float radius, float width, float dash, float gap, Rgba colour) {
    std::vector<Point> contour;
    Contour(r, radius, Segments(radius) * 2, contour);
    contour.push_back(contour.front());
    float period = dash + gap;
    float along = 0;   // distance from the start of the contour
    std::vector<Point> run;
    auto flush = [&]() {
        if (run.size() >= 2) {
            for (size_t i = 0; i + 1 < run.size(); ++i) Segment(run[i], run[i + 1], width, colour);
            Submit();
            FillCircle(run.front().x, run.front().y, width * 0.5f, colour);
            FillCircle(run.back().x, run.back().y, width * 0.5f, colour);
        }
        run.clear();
    };
    for (size_t i = 0; i + 1 < contour.size(); ++i) {
        Point a = contour[i], b = contour[i + 1];
        float len = std::hypot(b.x - a.x, b.y - a.y);
        float t = 0;
        while (t < len) {
            float phase = std::fmod(along + t, period);
            bool on = phase < dash;
            float next = on ? dash - phase : period - phase;   // distance to the next switch
            float step = std::min(next, len - t);
            Point p{a.x + (b.x - a.x) * t / len, a.y + (b.y - a.y) * t / len};
            Point q{a.x + (b.x - a.x) * (t + step) / len, a.y + (b.y - a.y) * (t + step) / len};
            if (on) {
                if (run.empty()) run.push_back(p);
                run.push_back(q);
                if (step >= next) flush();
            }
            t += step;
        }
        along += len;
    }
    flush();
}

}  // namespace tpw::ui
