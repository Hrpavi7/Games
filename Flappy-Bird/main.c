#include "raylib.h"
#include "rlgl.h"
#include <math.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 450

#define MAX_PIPES 100
#define PIPE_WIDTH 80
#define PIPE_CAP_HEIGHT 30
#define PIPE_SPACING 320
#define GAP_SIZE 140

#define GRAVITY 1100.0f
#define JUMP_STRENGTH 380.0f
#define PIPE_SPEED 220.0f
#define ROTATION_SPEED 3.0f

typedef struct Bird
{
    Vector2 position;
    float radius;
    float velocity;
    float rotation;
} Bird;

typedef struct Pipe
{
    Rectangle topRect;
    Rectangle bottomRect;
    bool active;
    bool passed;
} Pipe;

typedef struct Cloud
{
    Vector2 pos;
    float speed;
    int size;
} Cloud;

static Bird bird = {0};
static Pipe pipes[MAX_PIPES] = {0};
static Cloud clouds[5] = {0}; 
static int score = 0;
static int highScore = 0;
static bool gameOver = false;
static bool gamePaused = false;
static float flashTimer = 0.0f;

void InitGame(void);
void UpdateGame(void);
void DrawGame(void);
void UnloadGame(void);
void UpdateDrawFrame(void);

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "flappy bird");
    InitGame();
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }

    UnloadGame();
    CloseWindow();
    return 0;
}

void InitGame(void)
{
    score = 0;
    gameOver = false;
    gamePaused = false;
    flashTimer = 0.0f;

    bird.position = (Vector2){100, SCREEN_HEIGHT / 2.0f};
    bird.velocity = 0;
    bird.radius = 18;
    bird.rotation = 0;

    for (int i = 0; i < MAX_PIPES; i++)
    {
        pipes[i].active = true;
        pipes[i].passed = false;

        float posX = SCREEN_WIDTH + 200 + i * PIPE_SPACING;

        float gapY = GetRandomValue(80, SCREEN_HEIGHT - 80 - GAP_SIZE);

        pipes[i].topRect = (Rectangle){posX, 0, PIPE_WIDTH, gapY};
        pipes[i].bottomRect = (Rectangle){posX, gapY + GAP_SIZE, PIPE_WIDTH, SCREEN_HEIGHT - (gapY + GAP_SIZE)};
    }

    for (int i = 0; i < 5; i++)
    {
        clouds[i].pos = (Vector2){(float)GetRandomValue(0, SCREEN_WIDTH), (float)GetRandomValue(20, 150)};
        clouds[i].speed = (float)GetRandomValue(20, 50);
        clouds[i].size = GetRandomValue(30, 60);
    }
}

void UpdateGame(void)
{
    float dt = GetFrameTime();

    if (!gameOver)
    {
        if (IsKeyPressed('P'))
            gamePaused = !gamePaused;

        if (!gamePaused)
        {
            for (int i = 0; i < 5; i++)
            {
                clouds[i].pos.x -= clouds[i].speed * dt;
                if (clouds[i].pos.x < -100)
                    clouds[i].pos.x = SCREEN_WIDTH + 100;
            }

            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                bird.velocity = -JUMP_STRENGTH;
                bird.rotation = -25.0f;
            }

            bird.velocity += GRAVITY * dt;
            bird.position.y += bird.velocity * dt;

            if (bird.velocity > 100)
            {
                bird.rotation += ROTATION_SPEED;
                if (bird.rotation > 90.0f)
                    bird.rotation = 90.0f;
            }

            for (int i = 0; i < MAX_PIPES; i++)
            {
                pipes[i].topRect.x -= PIPE_SPEED * dt;
                pipes[i].bottomRect.x -= PIPE_SPEED * dt;

                if (pipes[i].topRect.x + pipes[i].topRect.width < 0)
                {
                    float furthestX = 0;
                    for (int j = 0; j < MAX_PIPES; j++)
                    {
                        if (pipes[j].topRect.x > furthestX)
                            furthestX = pipes[j].topRect.x;
                    }

                    float newX = furthestX + PIPE_SPACING;
                    float gapY = GetRandomValue(80, SCREEN_HEIGHT - 80 - GAP_SIZE);

                    pipes[i].topRect.x = newX;
                    pipes[i].topRect.height = gapY;
                    pipes[i].bottomRect.x = newX;
                    pipes[i].bottomRect.y = gapY + GAP_SIZE;
                    pipes[i].bottomRect.height = SCREEN_HEIGHT - (gapY + GAP_SIZE);
                    pipes[i].passed = false;
                }

                if (CheckCollisionCircleRec(bird.position, bird.radius, pipes[i].topRect))
                {
                    gameOver = true;
                    flashTimer = 1.0f;
                }
                if (CheckCollisionCircleRec(bird.position, bird.radius, pipes[i].bottomRect))
                {
                    gameOver = true;
                    flashTimer = 1.0f;
                }

                if (!pipes[i].passed && bird.position.x > pipes[i].topRect.x + PIPE_WIDTH)
                {
                    score++;
                    pipes[i].passed = true;
                }
            }

            if ((bird.position.y - bird.radius) < 0)
            {
                bird.position.y = bird.radius;
                bird.velocity = 0;
            }
            if ((bird.position.y + bird.radius) > SCREEN_HEIGHT)
            {
                gameOver = true;
                flashTimer = 1.0f;
            }
        }
    }
    else
    {
        if (bird.position.y < SCREEN_HEIGHT + 50)
        {
            bird.velocity += GRAVITY * dt;
            bird.position.y += bird.velocity * dt;
            bird.rotation += 5.0f;
        }

        if (IsKeyPressed(KEY_ENTER))
            InitGame();
    }
}

void DrawBird(Bird b)
{
    rlPushMatrix();
    rlTranslatef(b.position.x, b.position.y, 0);
    rlRotatef(b.rotation, 0, 0, 1);

    DrawEllipse(0, 0, 24, 18, GOLD);
    DrawEllipseLines(0, 0, 24, 18, ORANGE);

    DrawCircle(10, -8, 8, RAYWHITE);
    DrawCircle(12, -8, 3, BLACK);

    DrawTriangle((Vector2){14, 2}, (Vector2){14, 10}, (Vector2){26, 6}, ORANGE);
    DrawTriangleLines((Vector2){14, 2}, (Vector2){14, 10}, (Vector2){26, 6}, DARKBROWN);

    DrawEllipse(-6, 4, 10, 6, WHITE);

    rlPopMatrix();
}

void DrawGame(void)
{
    BeginDrawing();
    ClearBackground(SKYBLUE);

    for (int i = 0; i < 5; i++)
    {
        DrawCircle((int)clouds[i].pos.x, (int)clouds[i].pos.y, clouds[i].size, Fade(WHITE, 0.5f));
        DrawCircle((int)clouds[i].pos.x + 20, (int)clouds[i].pos.y + 10, clouds[i].size * 0.8, Fade(WHITE, 0.5f));
    }

    DrawRectangle(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50, (Color){100, 200, 100, 255}); 
    DrawLine(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, SCREEN_HEIGHT - 50, DARKGREEN);        

    Color pipeColor = (Color){0, 200, 0, 255}; 
    Color pipeOutline = DARKGREEN;

    for (int i = 0; i < MAX_PIPES; i++)
    {
        DrawRectangleRec(pipes[i].topRect, pipeColor);
        DrawRectangleLinesEx(pipes[i].topRect, 3, pipeOutline);

        Rectangle topCap = {pipes[i].topRect.x - 4, pipes[i].topRect.height - PIPE_CAP_HEIGHT, PIPE_WIDTH + 8, PIPE_CAP_HEIGHT};
        DrawRectangleRec(topCap, pipeColor);
        DrawRectangleLinesEx(topCap, 3, pipeOutline);

        DrawRectangle(pipes[i].topRect.x + 10, 0, 10, pipes[i].topRect.height, Fade(WHITE, 0.3f));

        DrawRectangleRec(pipes[i].bottomRect, pipeColor);
        DrawRectangleLinesEx(pipes[i].bottomRect, 3, pipeOutline);

        Rectangle botCap = {pipes[i].bottomRect.x - 4, pipes[i].bottomRect.y, PIPE_WIDTH + 8, PIPE_CAP_HEIGHT};
        DrawRectangleRec(botCap, pipeColor);
        DrawRectangleLinesEx(botCap, 3, pipeOutline);

        DrawRectangle(pipes[i].bottomRect.x + 10, pipes[i].bottomRect.y, 10, pipes[i].bottomRect.height, Fade(WHITE, 0.3f));
    }

    DrawBird(bird);

    DrawText(TextFormat("%i", score), SCREEN_WIDTH / 2, 50, 50, WHITE);
    DrawText(TextFormat("%i", score), SCREEN_WIDTH / 2 + 2, 52, 50, BLACK); 

    if (gameOver)
    {
        if (flashTimer > 0)
        {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(WHITE, flashTimer));
            flashTimer -= 0.05f;
        }

        DrawText("GAME OVER", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, 40, WHITE);
        DrawText("PRESS [ENTER]", SCREEN_WIDTH / 2 - 110, SCREEN_HEIGHT / 2 + 20, 30, WHITE);
    }

    EndDrawing();
}

void UnloadGame(void) {}
void UpdateDrawFrame(void)
{
    UpdateGame();
    DrawGame();
}
