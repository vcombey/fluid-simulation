
#include "userInterface.hpp"

using namespace mod1;

UserInterface::UserInterface(const std::shared_ptr<lib::Pool<RenderedFrame>> &pool,
                             int width,
                             int height) :
                                SdlContext(width, height),
                                m_pool(pool)
{
}

UserInterface::~UserInterface() {}

bool UserInterface::init() {
    if (m_ready == true) {
        std::cerr << __func__ << " : SDL2 already initialized" << std::endl;
        return false;
    }
    m_fpsDisplayer.init();
    m_font = TTF_OpenFont("srcs/fonts/Chalkduster.ttf", 25);
    if (m_font == nullptr) {
        std::cerr << "font is null" << std::endl;
        return false;
    }
    m_surface = SDL_GetWindowSurface(m_win);
    m_ready = true;
    return true;
}
static Uint32 customEventCb(Uint32 interval, void *param)
{
    SDL_Event event;
    SDL_UserEvent userEvent;

    /* In this example, our callback pushes a function
    into the queue, and causes our callback to be called again at the
    same interval: */

    userEvent.type = SDL_USEREVENT;
    userEvent.code = 0;
    userEvent.data1 = NULL;
    userEvent.data2 = param;


    event.type = SDL_USEREVENT;
    event.user = userEvent;

    SDL_PushEvent(&event);
    return(interval);
}

inline int UserInterface::Rgb_to_int(int r, int g, int b) {
    return (r << 16 | g << 8 | b);
}

void UserInterface::start() {
    if (m_ready == false) {
        std::cerr << __func__ << " : SDL2 not initialized" << std::endl;
        return ;
    }

    SDL_Event e;
    RenderedFrame *img;
    Uint32 delay = 1000 / 25;
    SDL_TimerID timerId = 0;

    float math_width = FRAME_WIDTH;
    float math_height = FRAME_HEIGHT;
    int math_x;
    int math_y;
    float deltaWidth = math_width / m_width;
    float deltaHeight = math_height / m_height;

    timerId = SDL_AddTimer(delay, customEventCb, NULL);
    while (m_continueLoopHook && SDL_WaitEvent(&e))
    {
        m_fpsDisplayer.setTimeOrigin();
        switch (e.type) {
            case SDL_KEYDOWN:
                std::cout << "SDL_KEYDOWN: scan code -> " << e.key.keysym.scancode << std::endl;
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_ESCAPE:
                        m_continueLoopHook = false;
                        break;
                    case SDL_SCANCODE_S:
                        std::cout << "touch start" << std::endl;
                        if (timerId == 0)
                            timerId = SDL_AddTimer(delay, customEventCb, NULL);
                        break;
                    case SDL_SCANCODE_H:
                        std::cout << "touch halt" << std::endl;
                        if (timerId) {
                            SDL_RemoveTimer(timerId);
                            timerId = 0;
                        }
                        break;
                    default:
                        break;
                }
                break;
            case SDL_QUIT:
                stop();
                break;
            case SDL_USEREVENT:
                img = m_pool->popRenderedItem();
                if (img) {
                    m_fpsDisplayer.updateFpsCounter();
                    for (int i = 0; i < (m_width * m_height); i++) {
                        float j;
                        math_x = (i % m_width) * deltaWidth;
                        math_y = (i / m_width) * deltaHeight;
                        j = math_y * math_width + math_x;
                        ((int *)m_surface->pixels)[i] = Rgb_to_int(img->m_map[(int)j].r,
                            img->m_map[(int)j].g,
                            img->m_map[(int)j].b);
                    }
                    m_pool->pushOutdatedItem(img);
                    m_fpsDisplayer.fillInformations(m_surface, false);
                    SDL_UpdateWindowSurface(m_win);

                    m_fpsDisplayer.updateIddleField();
                } else {
                    SDL_Color color = { 255, 255, 255, 0 };
                    SDL_Surface *font_surface = TTF_RenderText_Solid(m_font, "FAMINE", color);
                    SDL_BlitSurface(font_surface, NULL, m_surface, NULL);
                    SDL_UpdateWindowSurface(m_win); // TODO SDL_UpdateWindowSurfaceRects
                }
                break;
            case SDL_WINDOWEVENT:
                switch (e.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                        m_width = e.window.data1;
                        m_height = e.window.data2;
                        m_surface = SDL_GetWindowSurface(m_win);
                        std::cout << "new size = " << m_width << " - " << m_height << std::endl;
                        deltaWidth = math_width / m_width;
                        deltaHeight = math_height / m_height;
                        break;
                    case SDL_WINDOWEVENT_MOVED:
                        std::cout << "window has moved !" << std::endl;
                        break;
                    default:
                        std::cout << "default window event" << std::endl;
                        break;
                }
        }
    }
    SDL_DestroyWindow(m_win);
    SDL_RemoveTimer(timerId);
    m_ready = false;
}

void UserInterface::stop() {
    m_continueLoopHook = false;
}
