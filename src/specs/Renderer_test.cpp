#include "../lib/utest.h"
#include "../Renderer.cpp"

// std
#include <filesystem>

UTEST(Renderer, InitSetsClipRect) {
    SDL_Window *win = SDL_CreateWindow("Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
    Renderer::init(win);
    Renderer::Rect clip;
    Renderer::getClipRect(&clip);
    EXPECT_EQ(clip.x, 0);
    EXPECT_EQ(clip.y, 0);
    EXPECT_EQ(clip.w, 640);
    EXPECT_EQ(clip.h, 480);
    SDL_DestroyWindow(win);
}
UTEST(Utf8ToCodePointTest, HandlesAscii) {
    unsigned cp;
    const char *next = Renderer::utf8toCodePoint("A", &cp);
    EXPECT_EQ(cp, 'A');
    EXPECT_EQ(*next, '\0');
}

UTEST(Utf8ToCodePointTest, HandlesTwoByte) {
    unsigned cp;
    const char *next = Renderer::utf8toCodePoint("\xc3\xa9", &cp); // é
    EXPECT_EQ(cp, 0xE9);
    EXPECT_EQ(*next, '\0');
}

UTEST(Utf8ToCodePointTest, HandlesThreeByte) {
    unsigned cp;
    const char *next = Renderer::utf8toCodePoint("\xe2\x82\xac", &cp); // €
    EXPECT_EQ(cp, 0x20AC);
    EXPECT_EQ(*next, '\0');
}

UTEST(Utf8ToCodePointTest, HandlesFourByte) {
    unsigned cp;
    const char *next = Renderer::utf8toCodePoint("\xf0\x9f\x98\x80", &cp); // 😀
    EXPECT_EQ(cp, 0x1F600);
    EXPECT_EQ(*next, '\0');
}

UTEST(RendererTest, NewImageCreatesValidImage) {
    int width = 10, height = 20;
    Renderer::Image* img = Renderer::newImage(width, height);
    ASSERT_NE(img, nullptr);
    ASSERT_NE(img->pixels, nullptr);
    ASSERT_EQ(img->w, width);
    ASSERT_EQ(img->h, height);
    delete img;
}

UTEST(RendererTest, LoadGlyphSetReturnsValidGlyphSetWithValidFont) {
    std::filesystem::path cwd = std::filesystem::current_path();
    Renderer::Font* font = Renderer::loadFont(cwd.parent_path().append("src/specs/assets/font.ttf").c_str(), 16.0f);
    stbtt_InitFont(&font->stbfont, static_cast<const unsigned char*>(font->data), 1);

    Renderer::GlyphSet* set = Renderer::loadGlyphSet(font, 1);
    ASSERT_NE(set, nullptr);
    ASSERT_NE(set->image, nullptr);

    Renderer::freeImage(set->image);
    free(set);
    free(font->data);
}

UTEST(ColorTest, blendPixel) {
    Renderer::Color dst = {255, 0, 0, 255}; // Red, opaque
    Renderer::Color src = {0, 255, 0, 128}; // Green, half-transparent

    Renderer::Color result = blendPixel(dst, src);

    // Expected: blend of red and green, half alpha
    assert(result.r >= 127 && result.r <= 128); // Should be about half red
    assert(result.g >= 127 && result.g <= 128); // Should be about half green
    assert(result.b == 0); // No blue
}