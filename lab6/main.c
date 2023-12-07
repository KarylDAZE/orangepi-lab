#include <stdio.h>
#include "../common/common.h"

#define COLOR_BACKGROUND FB_COLOR(0xff, 0xff, 0xff)
#define BUTTON_WIDTH 100
#define BUTTON_HEIGHT 60
#define RED FB_COLOR(255, 0, 0)
#define ORANGE FB_COLOR(255, 165, 0)
#define YELLOW FB_COLOR(255, 255, 0)
#define GREEN FB_COLOR(0, 255, 0)
#define CYAN FB_COLOR(0, 127, 255)
#define BLUE FB_COLOR(0, 0, 255)
#define PURPLE FB_COLOR(139, 0, 255)
#define WHITE FB_COLOR(255, 255, 255)
#define BLACK FB_COLOR(0, 0, 0)

static int touch_fd;
static int bluetooth_fd;

typedef struct touch
{
    int type;
    int x;
    int y;
    int color;
    int finger;
} touch;

static int xp[FINGER_NUM_MAX * 2] = {0}, yp[FINGER_NUM_MAX * 2] = {0}, color[FINGER_NUM_MAX * 2] = {0};

static int touch_draw(int type, int x, int y, int sameColor, int finger)
{
    // sameColor不为0时，传递的是另一个设备的color
    if (sameColor)
        color[finger] = sameColor;
    switch (type)
    {
    case TOUCH_PRESS:
        printf("TOUCH_PRESS：x=%d,y=%d,finger=%d\n", x, y, finger);
        if (x > 0 && x < BUTTON_WIDTH && y > 0 && y < BUTTON_HEIGHT)
        {
            fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BACKGROUND);
            fb_draw_border(0, 0, BUTTON_WIDTH, BUTTON_HEIGHT, BLACK);
            fb_draw_text(20, 40, "清空", 30, BLACK);
            break;
        }
        if (x > 0 && x < BUTTON_WIDTH && y > SCREEN_HEIGHT - BUTTON_HEIGHT && y < SCREEN_HEIGHT)
        {
            exit(0);
        }
        if (!sameColor)
            while (!(color[finger] = rand()))
                ;
        fb_draw_pixel(x, y, color[finger]);
        fb_draw_pixel(x - 1, y, color[finger]);
        fb_draw_pixel(x + 1, y, color[finger]);
        fb_draw_pixel(x, y - 1, color[finger]);
        fb_draw_pixel(x, y + 1, color[finger]);
        xp[finger] = x;
        yp[finger] = y;
        break;
    case TOUCH_MOVE:
        printf("TOUCH_MOVE：x=%d,y=%d,finger=%d\n", x, y, finger);
        fb_draw_line(xp[finger], yp[finger], x, y, color[finger]);
        fb_draw_line(xp[finger] - 1, yp[finger], x - 1, y, color[finger]);
        fb_draw_line(xp[finger] + 1, yp[finger], x + 1, y, color[finger]);
        fb_draw_line(xp[finger], yp[finger] - 1, x, y - 1, color[finger]);
        fb_draw_line(xp[finger], yp[finger] + 1, x, y + 1, color[finger]);
        xp[finger] = x;
        yp[finger] = y;

        break;
    case TOUCH_RELEASE:
        printf("TOUCH_RELEASE：x=%d,y=%d,finger=%d\n", x, y, finger);

        break;
    case TOUCH_ERROR:
        printf("close touch fd\n");
        return -1;
        break;
    default:
        return 1;
    }
    fb_update();
    return 0;
}

static void touch_event_cb(int fd)
{
    int type, x, y, finger;
    type = touch_read(fd, &x, &y, &finger);
    int flag = touch_draw(type, x, y, 0, finger);
    if (flag == -1)
    {
        close(fd);
        task_delete_file(fd);
    }
    else if (flag == 0)
    // send touch info to client
    {
        // 一共2*5根手指，finger=[5,9]表示是从对方设备传输来的touch
        touch serverTouch = {type, x, y, color[finger], finger + 5};
        myWrite_nonblock(bluetooth_fd, &serverTouch, sizeof(serverTouch));
    }
    return;
}

static void bluetooth_tty_event_cb(int fd)
{
    int n;
    touch clientTouch;
    n = myRead_nonblock(fd, &clientTouch, sizeof(clientTouch));
    if (n <= 0)
    {
        printf("close bluetooth tty fd\n");
        // task_delete_file(fd);
        // close(fd);
        exit(0);
        return;
    }
    if (touch_draw(clientTouch.type, clientTouch.x, clientTouch.y, clientTouch.color, clientTouch.finger) == 1)
    {
        close(fd);
        task_delete_file(fd);
    }
    fb_update();
    return;
}

static int bluetooth_tty_init(const char *dev)
{
    int fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK); /*非阻塞模式*/
    if (fd < 0)
    {
        printf("bluetooth_tty_init open %s error(%d): %s\n", dev, errno, strerror(errno));
        return -1;
    }
    return fd;
}

int main(int argc, char *argv[])
{
    fb_init("/dev/fb0");
    font_init("./font.ttc");
    fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BACKGROUND);

    // 绘制一个清除屏幕的按钮
    fb_draw_border(0, 0, BUTTON_WIDTH, BUTTON_HEIGHT, BLACK);
    fb_draw_text(20, 40, "清空", 30, BLACK);

    fb_update();
    // 对于不同设备 event序号可能不同
    touch_fd = touch_init("/dev/input/event1");
    task_add_file(touch_fd, touch_event_cb);

    bluetooth_fd = bluetooth_tty_init("/dev/rfcomm0");
    if (bluetooth_fd == -1)
        return 0;
    task_add_file(bluetooth_fd, bluetooth_tty_event_cb);

    task_loop();
    return 0;
}