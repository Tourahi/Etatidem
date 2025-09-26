#include "../lib/utest.h"
#include "../Renderer.cpp"

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