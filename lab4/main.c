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
static void touch_event_cb(int fd)
{
	int type, x, y, finger;
	static int xp[5] = {0}, yp[5] = {0}, color[5] = {0};
	type = touch_read(fd, &x, &y, &finger);
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
		close(fd);
		task_delete_file(fd);
		break;
	default:
		return;
	}
	fb_update();
	return;
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
	// 打开多点触摸设备文件, 返回文件fd
	touch_fd = touch_init("/dev/input/event2");
	// 添加任务, 当touch_fd文件可读时, 会自动调用touch_event_cb函数
	task_add_file(touch_fd, touch_event_cb);

	task_loop(); // 进入任务循环
	return 0;
}
