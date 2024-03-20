/*
 * square5.c
 * this program lets you move the square, with vsync
 */

/* the width and height of the screen */
#define WIDTH 240
#define HEIGHT 160

/* these identifiers define different bit positions of the display control */
#define MODE4 0x0004
#define BG2 0x0400

/* this bit indicates whether to display the front or the back buffer
 * this allows us to refer to bit 4 of the display_control register */
#define SHOW_BACK 0x10;

/* the screen is simply a pointer into memory at a specific address this
 *  * pointer points to 16-bit colors of which there are 240x160 */
volatile unsigned short *screen = (volatile unsigned short *)0x6000000;

/* the display control pointer points to the gba graphics register */
volatile unsigned long *display_control = (volatile unsigned long *)0x4000000;

/* the address of the color palette used in graphics mode 4 */
volatile unsigned short *palette = (volatile unsigned short *)0x5000000;

/* pointers to the front and back buffers - the front buffer is the start
 * of the screen array and the back buffer is a pointer to the second half */
volatile unsigned short *front_buffer = (volatile unsigned short *)0x6000000;
volatile unsigned short *back_buffer = (volatile unsigned short *)0x600A000;

/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be volatile as well
 */
volatile unsigned short *buttons = (volatile unsigned short *)0x04000130;

/* the bit positions indicate each button - the first bit is for A, second for
 * B, and so on, each constant below can be ANDED into the register to get the
 * status of any one button */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

/* the scanline counter is a memory cell which is updated to indicate how
 * much of the screen has been drawn */
volatile unsigned short *scanline_counter = (volatile unsigned short *)0x4000006;

/* wait for the screen to be fully drawn so we can do something during vblank */
void wait_vblank()
{
    /* wait until all 160 lines have been updated */
    while (*scanline_counter < 160)
    {
    }
}

/* this function checks whether a particular button has been pressed */
unsigned char button_pressed(unsigned short button)
{
    /* and the button register with the button constant we want */
    unsigned short pressed = *buttons & button;

    /* if this value is zero, then it's not pressed */
    if (pressed == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/* keep track of the next palette index */
int next_palette_index = 0;

/*
 * function which adds a color to the palette and returns the
 * index to it
 */
unsigned char add_color(unsigned char r, unsigned char g, unsigned char b)
{
    unsigned short color = b << 10;
    color += g << 5;
    color += r;

    /* add the color to the palette */
    palette[next_palette_index] = color;

    /* increment the index */
    next_palette_index++;

    /* return index of color just added */
    return next_palette_index - 1;
}

/* a colored square :) */
struct square
{
    int x, y, xsize, ysize;
    unsigned char color;
    int dy;
    int dx;
};

/* put a pixel on the screen in mode 4 */
void put_pixel(volatile unsigned short *buffer, int row, int col, unsigned char color)
{
    /* find the offset which is the regular offset divided by two */
    unsigned short offset = (row * WIDTH + col) >> 1;

    /* read the existing pixel which is there */
    unsigned short pixel = buffer[offset];

    /* if it's an odd column */
    if (col & 1)
    {
        /* put it in the left half of the short */
        buffer[offset] = (color << 8) | (pixel & 0x00ff);
    }
    else
    {
        /* it's even, put it in the left half */
        buffer[offset] = (pixel & 0xff00) | color;
    }
}

/* draw a square onto the screen */
void draw_square(volatile unsigned short *buffer, struct square *s)
{
    short row, col;
    /* for each row of the square */
    for (row = s->y; row < (s->y + s->ysize); row++)
    {
        /* loop through each column of the square */
        for (col = s->x; col < (s->x + s->xsize); col++)
        {
            /* set the screen location to this color */
            put_pixel(buffer, row, col, s->color);
        }
    }
}

/* clear the screen right around the square */
void update_screen(volatile unsigned short *buffer, unsigned short color, struct square *s)
{
    short row, col;
    for (row = s->y - 3; row < (s->y + s->ysize + 3); row++)
    {
        for (col = s->x - 3; col < (s->x + s->xsize + 3); col++)
        {
            put_pixel(buffer, row, col, color);
        }
    }
}

/* this function takes a video buffer and returns to you the other one */
volatile unsigned short *flip_buffers(volatile unsigned short *buffer)
{
    /* if the back buffer is up, return that */
    if (buffer == front_buffer)
    {
        /* clear back buffer bit and return back buffer pointer */
        *display_control &= ~SHOW_BACK;
        return back_buffer;
    }
    else
    {
        /* set back buffer bit and return front buffer */
        *display_control |= SHOW_BACK;
        return front_buffer;
    }
}

/* handle the buttons which are pressed down */
void handle_buttons(struct square *s)
{
    /* move the square with the arrow keys */
    if (button_pressed(BUTTON_DOWN))
    {
        if (s->y != 160)
        {
            s->y += 1;
        }
    }
    if (button_pressed(BUTTON_UP))
    {
        if (s->y != 0)
        {
            s->y -= 1;
        }
    }
}

void move_paddle(struct square *s, struct square *ball)
{
    s->y += s->dy;

    if ( (s->y - ball->y) >= 5)
    {
        s->dy = -1;
    }
    else if ( (ball->y - s->y) >= 5)
    {
        s->dy = 1;
    }
}

void move_ball(struct square *s, struct square *g, struct square *ball)

{

    ball->y += ball->dy;
    ball->x += ball->dx;
}

void ball_paddle_collision(struct square *s, struct square *ball)
{

    if (ball->x <= s->x)
    {
        if ((ball->y + ball->ysize) >= (s->y) && (ball->y) <= (s->y + s->ysize))
        {
                ball->dx = 1;
        }
    }
}

void ball_paddle_collision_ai(struct square *s, struct square *ball)
{
    if (ball->x >= s->x)
    {
        if ((ball->y + ball->ysize) >= (s->y) && (ball->y) <= (s->y + s->ysize))
        {
            ball->dx = -1;
        }
    }
}


/* clear the screen to black */
void clear_screen(volatile unsigned short *buffer, unsigned short color)
{
    unsigned short row, col;
    /* set each pixel black */
    for (row = 0; row < HEIGHT; row++)
    {
        for (col = 0; col < WIDTH; col++)
        {
            put_pixel(buffer, row, col, color);
        }
    }
}

/* the main function */
int main()
{
    /* we set the mode to mode 4 with bg2 on */
    *display_control = MODE4 | BG2;

    /* make a green square */
    struct square s = {10, 10, 2, 10, add_color(0, 20, 2)};
    struct square ball = {100, 90, 2, 2, add_color(0, 20, 2)};
    struct square g = {200, 10, 2, 10, add_color(0, 20, 2)};
    struct square net = {120, 0, 1, 160, add_color(31, 31, 31)};

    struct square player_tic1 = {20, 150, 1, 1, add_color(0, 20, 2)};
    struct square player_tic2 = {22, 150, 1, 1, add_color(0, 20, 2)};
    struct square player_tic3 = {24, 150, 1, 1, add_color(0, 20, 2)};
    struct square player_tic4 = {26, 150, 1, 1, add_color(0, 20, 2)};
    struct square player_tic5 = {28, 150, 1, 1, add_color(0, 20, 2)};

    struct square ai_tic1 = {160, 150, 1, 1, add_color(0, 20, 2)};
    struct square ai_tic2 = {162, 150, 1, 1, add_color(0, 20, 2)};
    struct square ai_tic3 = {164, 150, 1, 1, add_color(0, 20, 2)};
    struct square ai_tic4 = {166, 150, 1, 1, add_color(0, 20, 2)};
    struct square ai_tic5 = {168, 150, 1, 1, add_color(0, 20, 2)};

    g.dy = 1;
    ball.dx = -1;
    ball.dy = 1;

    /* add black to the palette */
    unsigned char black = add_color(0, 0, 0);

    /* the buffer we start with */
    volatile unsigned short *buffer = front_buffer;

    /* clear whole screen first */
    clear_screen(front_buffer, black);
    clear_screen(back_buffer, black);

    int player_score = 0;
    int ai_score = 0;
    /* loop forever */
    while (1)
    {
        /* clear the screen - only the areas around the square! */
        update_screen(buffer, black, &s);
        update_screen(buffer, black, &g);
        update_screen(buffer, black, &ball);
        /* draw the square */
        draw_square(buffer, &s);
        draw_square(buffer, &g);
        draw_square(buffer, &ball);
        draw_square(buffer, &net);
        /* Drawing tics */

        if (ai_score >= 1)
        {
            draw_square(buffer, &ai_tic1);
        }
        if (ai_score >= 2)
        {
            draw_square(buffer, &ai_tic2);
        }
        if (ai_score >= 3)
        {
            draw_square(buffer, &ai_tic3);
        }
        if (ai_score >= 4)
        {
            draw_square(buffer, &ai_tic4);
        }
        if (ai_score >= 5)
        {
            draw_square(buffer, &ai_tic5);
        }

        if (player_score >= 1)
        {
            draw_square(buffer, &player_tic1);
        }
        if (player_score >= 2)
        {
            draw_square(buffer, &player_tic2);
        }
        if (player_score >= 3)
        {
            draw_square(buffer, &player_tic3);
        }
        if (player_score >= 4)
        {
            draw_square(buffer, &player_tic4);
        }
        if (player_score >= 5)
        {
            draw_square(buffer, &player_tic5);
        }

        if (ai_score == 5 || player_score == 5)
        {
            break;
        }


        /* handle button input */
        handle_buttons(&s);

        ball_paddle_collision(&s, &ball);
        ball_paddle_collision_ai(&g, &ball);
        move_ball(&s, &g, &ball);
        move_paddle(&g, &ball);



        if (ball.y == 160)
        {
            update_screen(front_buffer, black, &ball);
            update_screen(back_buffer, black, &ball);
            ball.dy = -1;
        }
        if (ball.y == 0)
        {
            update_screen(front_buffer, black, &ball);
            update_screen(back_buffer, black, &ball);
            ball.dy = 1;
        }

        // Score
        if (ball.x == 5)
        {

            clear_screen(front_buffer, black);
            clear_screen(back_buffer, black);
            ai_score += 1;

            ball.x = 100;
            ball.y = 10;
        }

        if (ball.x == 205)
        {

            clear_screen(front_buffer, black);
            clear_screen(back_buffer, black);

            player_score += 1;
            ball.x = 100;
            ball.y = 10;
        }

        /* wiat for vblank before switching buffers */
        wait_vblank();

        /* swap the buffers */
        buffer = flip_buffers(buffer);
    }
}
