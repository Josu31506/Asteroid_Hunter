#include "raylib.h"
#include "quadtree.h"

#include "raymath.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

    enum GameState { GAME_MENU, GAME_PLAYING, GAME_PAUSED, GAME_OVER };
    GameState currentGameState = GAME_MENU;

    bool playWithBot = true;
    bool playerWon = false;
    int SCREEN_WIDTH = 1280;
    int SCREEN_HEIGHT = 720;

    const int MAX_ASTEROIDS = 2000;
    const int MAX_BULLETS = 60;
    const int MAX_EXPLOSIONS = 25;
    const int WIN_SCORE = 2500;

    struct EntityData {
        Vector2 velocity;
        int size;
        bool active;
        int id;
    };

    struct Explosion {
        Vector2 position;
        int currentFrame;
        int frameCounter;
        float scale;
        bool active; };

    struct BotPlayer {
        GameObject entity;
        Vector2 velocity;
        float rotation;
        bool active;
        float fireTimer;
        int score;
        float wanderTimer;
        float aimError;
    };

    GameObject asteroids[MAX_ASTEROIDS];
    EntityData astData[MAX_ASTEROIDS];
    GameObject bullets[MAX_BULLETS];
    EntityData bulData[MAX_BULLETS];
    Explosion explosions[MAX_EXPLOSIONS];
    int globalIdCounter = 0;
    int playerScore = 0;
    GameObject ship;
    Vector2 shipVel = { 0, 0 };
    float visualRotation = 0;
    int lives = 3;
    float spawnTimer = 0;
    BotPlayer bot;


    Texture2D background, shipTex, asteroidTex, explosionTex;
    Sound fxShot, fxExplosion;
    Music music;


    void SplitAsteroid(int index, bool hitByPlayer) {
        int currentSize = astData[index].size;
        Point pos = asteroids[index].position;
        astData[index].active = false;

        if (hitByPlayer) playerScore += 100;
        else bot.score += 100;

        if (currentSize > 1) {
            int created = 0;
            for (int i = 0; i < MAX_ASTEROIDS && created < 2; i++) {
                if (!astData[i].active) {
                    astData[i].active = true;
                    astData[i].size = currentSize - 1;
                    astData[i].id = globalIdCounter++;
                    asteroids[i].position = pos;
                    asteroids[i].width = (float)astData[i].size * 30.0f;
                    asteroids[i].height = (float)astData[i].size * 30.0f;
                    asteroids[i].type = 2;
                    asteroids[i].id = astData[i].id;
                    float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
                    astData[i].velocity = { cosf(angle) * 2.5f, sinf(angle) * 2.5f };
                    created++;
                }
            }
        }
    }

    void ResetGame() {
        playerScore = 0; bot.score = 0; lives = 3; playerWon = false;
        ship.position = { (float)SCREEN_WIDTH / 3, (float)SCREEN_HEIGHT / 2 };
        shipVel = { 0, 0 }; spawnTimer = 3.0f;
        bot.entity.position = { (float)SCREEN_WIDTH * 0.7f, (float)SCREEN_HEIGHT / 2 };
        bot.velocity = { 0, 0 }; bot.active = playWithBot; bot.rotation = 0;
        bot.wanderTimer = 0; bot.aimError = 0;

        for (int i = 0; i < MAX_ASTEROIDS; i++) astData[i].active = false;
        for (int i = 0; i < MAX_BULLETS; i++) bulData[i].active = false;
        for (int i = 0; i < MAX_EXPLOSIONS; i++) explosions[i].active = false;

        for (int i = 0; i < 15; i++) {
            astData[i].active = true; astData[i].size = 3; astData[i].id = globalIdCounter++;
            asteroids[i].position = {(float)GetRandomValue(0, SCREEN_WIDTH), (float)GetRandomValue(0, SCREEN_HEIGHT)};
            asteroids[i].width = 60; asteroids[i].height = 60; asteroids[i].type = 2;
            asteroids[i].id = astData[i].id;
            astData[i].velocity = {(float)GetRandomValue(-200, 200)/100.0f, (float)GetRandomValue(-200, 200)/100.0f};
        }
    }

void SuperCleanSprite(Image *image, bool isExplosion)
    {
        if (!image || !image->data) return;

        ImageFormat(image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        Color *p = (Color *)image->data;

        for (int i = 0; i < image->width * image->height; i++)
        {
            if (isExplosion)
            {
                int br = (p[i].r + p[i].g + p[i].b) / 3;

                const int cutoff = 35;
                const float gain = 3.0f;

                int a = (int)((br - cutoff) * gain);
                if (a < 0) a = 0;
                if (a > 255) a = 255;

                p[i].a = (unsigned char)a;


                if (p[i].a == 0) p[i] = (Color){0,0,0,0};
            }
            else
            {
                if (p[i].r > 210 && p[i].g > 210 && p[i].b > 210) p[i] = BLANK;
            }
        }
    }


    void UpdateBotAI(BotPlayer &b, GameObject asts[], EntityData data[], Sound shotSound) {
        if (!b.active) return;

        float closestDist = 2000.0f;
        int targetIdx = -1;


        for (int i = 0; i < MAX_ASTEROIDS; i++) {
            if (data[i].active) {
                float d = Vector2Distance({b.entity.position.x, b.entity.position.y}, {asts[i].position.x, asts[i].position.y});
                if (d < closestDist) { closestDist = d; targetIdx = i; }
            }
        }

        b.wanderTimer -= GetFrameTime();

        if (targetIdx != -1 && b.wanderTimer <= 0) {
           
            if (GetRandomValue(0, 100) < 5) b.aimError = (float)GetRandomValue(-15, 15); // Cambia el error de vez en cuando

            float angleToTarget = atan2f(asts[targetIdx].position.y - b.entity.position.y, asts[targetIdx].position.x - b.entity.position.x) * RAD2DEG;
            float diff = (angleToTarget + b.aimError) - b.rotation;

            while (diff > 180) diff -= 360;
            while (diff < -180) diff += 360;
            b.rotation += diff * 0.05f;

            b.velocity.x += cosf(b.rotation * DEG2RAD) * 0.15f;
            b.velocity.y += sinf(b.rotation * DEG2RAD) * 0.15f;


            b.fireTimer += GetFrameTime();
            if (b.fireTimer > 0.6f && closestDist < 600) {
                if (GetRandomValue(0, 10) > 2) {
                    for (int i = 0; i < MAX_BULLETS; i++) if (!bulData[i].active) {
                        bulData[i].active = true; bullets[i].position = b.entity.position;
                        bullets[i].width = 10; bullets[i].height = 10; bullets[i].type = 4;
                        bulData[i].velocity = { cosf(b.rotation * DEG2RAD) * 11.0f, sinf(b.rotation * DEG2RAD) * 11.0f };
                        b.fireTimer = 0;
                        PlaySound(shotSound);
                        break;
                    }
                } else {
                    b.fireTimer = 0.3f;
                }
            }
        } else {

            if (b.wanderTimer <= -1.5f) {
                b.wanderTimer = (float)GetRandomValue(1, 3);
                b.velocity.x += (float)GetRandomValue(-5, 5);
                b.velocity.y += (float)GetRandomValue(-5, 5);
            }
            b.rotation += 2.0f;
        }


        b.entity.position.x += b.velocity.x; b.entity.position.y += b.velocity.y;
        b.velocity.x *= 0.96f; b.velocity.y *= 0.96f;


        if (b.entity.position.x > SCREEN_WIDTH) b.entity.position.x = 0; else if (b.entity.position.x < 0) b.entity.position.x = SCREEN_WIDTH;
        if (b.entity.position.y > SCREEN_HEIGHT) b.entity.position.y = 0; else if (b.entity.position.y < 0) b.entity.position.y = SCREEN_HEIGHT;
    }

static inline int Dist2(Color a, Color b)
    {
        int dr = (int)a.r - (int)b.r;
        int dg = (int)a.g - (int)b.g;
        int db = (int)a.b - (int)b.b;
        return dr*dr + dg*dg + db*db;
    }

void UpdateDrawFrame(void) {
        
        if (IsMusicStreamPlaying(music)) {
            UpdateMusicStream(music);
        }
        
        else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || GetKeyPressed() > 0) {
            PlayMusicStream(music);
        }
        switch (currentGameState) {
                case GAME_MENU:
                    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)) playWithBot = !playWithBot;
                    if (IsKeyPressed(KEY_ENTER)) { ResetGame(); currentGameState = GAME_PLAYING; }
                    break;

                case GAME_PLAYING:
                {
                    // --- ERROR 1 CORREGIDO: CAPTURAR ESCAPE AQUÍ ---
                    if (IsKeyPressed(KEY_ESCAPE)) {
                        currentGameState = GAME_PAUSED;
                        break; // Salimos del switch para que no procese el frame de juego
                    }
                    if (lives <= 0) { playerWon = false; currentGameState = GAME_OVER; }
                    if (playerScore >= WIN_SCORE) { playerWon = true; currentGameState = GAME_OVER; }
                    if (playWithBot && bot.score >= WIN_SCORE) { playerWon = false; currentGameState = GAME_OVER; }

                    float dt = GetFrameTime();
                    if (spawnTimer > 0) spawnTimer -= dt;

                    if (IsKeyDown(KEY_W)) shipVel.y -= 0.6f;
                    if (IsKeyDown(KEY_S)) shipVel.y += 0.6f;
                    if (IsKeyDown(KEY_A)) shipVel.x -= 0.6f;
                    if (IsKeyDown(KEY_D)) shipVel.x += 0.6f;

                    shipVel.x *= 0.94f; shipVel.y *= 0.94f;
                    if (Vector2Length(shipVel) > 0.1f) visualRotation = atan2f(shipVel.y, shipVel.x) * RAD2DEG;
                    ship.position.x += shipVel.x; ship.position.y += shipVel.y;

                    if (ship.position.x > SCREEN_WIDTH) ship.position.x = 0; else if (ship.position.x < 0) ship.position.x = SCREEN_WIDTH;
                    if (ship.position.y > SCREEN_HEIGHT) ship.position.y = 0; else if (ship.position.y < 0) ship.position.y = SCREEN_HEIGHT;

                    if (IsKeyPressed(KEY_Q)) {
                        for (int i = 0; i < MAX_BULLETS; i++) {
                            if (!bulData[i].active) {
                                bulData[i].active = true;
                                bullets[i].position = ship.position;
                                bullets[i].type = 3; // Tipo jugador
                                
                                bulData[i].velocity = {
                                    cosf(visualRotation * DEG2RAD) * 12.0f,
                                    sinf(visualRotation * DEG2RAD) * 12.0f
                                };
                                
                                PlaySound(fxShot);
                                break; 
                            }
                        }
                    }

                    if(bot.active) UpdateBotAI(bot, asteroids, astData, fxShot);

                    QuadTree qt(CustomRectangle(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, SCREEN_WIDTH, SCREEN_HEIGHT));
                    qt.insert(ship); if(bot.active) qt.insert(bot.entity);

                    for (int i = 0; i < MAX_ASTEROIDS; i++) if (astData[i].active) {
                        asteroids[i].position.x += astData[i].velocity.x; asteroids[i].position.y += astData[i].velocity.y;
                        if (asteroids[i].position.x > SCREEN_WIDTH) asteroids[i].position.x = 0; else if (asteroids[i].position.x < 0) asteroids[i].position.x = SCREEN_WIDTH;
                        if (asteroids[i].position.y > SCREEN_HEIGHT) asteroids[i].position.y = 0; else if (asteroids[i].position.y < 0) asteroids[i].position.y = SCREEN_HEIGHT;
                        qt.insert(asteroids[i]);
                    }

                    if (spawnTimer <= 0) {
                        GameObjectList sCol;
                        qt.query(ship.getBounds(), sCol);
                        for(int i = 0; i < sCol.size(); i++) if(sCol.get(i).type == 2) {
                            lives--; ship.position = {(float)SCREEN_WIDTH/3, (float)SCREEN_HEIGHT/2}; shipVel = {0,0}; spawnTimer = 3.0f; break;
                        }
                    }

                    for (int i = 0; i < MAX_BULLETS; i++) if (bulData[i].active) {
                        bullets[i].position.x += bulData[i].velocity.x; bullets[i].position.y += bulData[i].velocity.y;
                        if (bullets[i].position.x < 0 || bullets[i].position.x > SCREEN_WIDTH || bullets[i].position.y < 0 || bullets[i].position.y > SCREEN_HEIGHT) bulData[i].active = false;
                        else {
                            GameObjectList bCol; qt.query(CustomRectangle(bullets[i].position.x, bullets[i].position.y, 10, 10), bCol);
                            for (int j = 0; j < bCol.size(); j++) if (bCol.get(j).type == 2) {
                                bulData[i].active = false;
                                for(int k=0; k<MAX_ASTEROIDS; k++) if(astData[k].active && asteroids[k].id == bCol.get(j).id) {
                                    SplitAsteroid(k, bullets[i].type == 3);
                                    PlaySound(fxExplosion);
                                    for(int e=0; e<MAX_EXPLOSIONS; e++) if(!explosions[e].active) {
                                        explosions[e].active = true; explosions[e].position = {bCol.get(j).position.x, bCol.get(j).position.y};
                                        explosions[e].currentFrame = 0; explosions[e].scale = (float)astData[k].size * 80.0f; break;
                                    }
                                    break;
                                }
                                break;
                            }
                        }
                    }
                    break;
                }
            case GAME_PAUSED:
                
                if (IsKeyPressed(KEY_ESCAPE)) currentGameState = GAME_PLAYING;
                if (IsKeyPressed(KEY_Q)) currentGameState = GAME_MENU;
                break;

            case GAME_OVER:
                if (IsKeyPressed(KEY_ENTER)) currentGameState = GAME_MENU;
                break;
            }

            BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(background, {0,0,(float)background.width, (float)background.height}, {0,0,(float)SCREEN_WIDTH, (float)SCREEN_HEIGHT}, {0,0}, 0, WHITE);

    if (currentGameState == GAME_MENU) {
        DrawText("DUO ASTEROID HUNTER", SCREEN_WIDTH/2 - 350, SCREEN_HEIGHT/2 - 150, 60, RAYWHITE);
        Color soloColor = (!playWithBot) ? YELLOW : GRAY; Color botColor = (playWithBot) ? YELLOW : GRAY;
        DrawText(playWithBot ? "  SOLO MODE" : "> SOLO MODE", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2, 30, soloColor);
        DrawText(playWithBot ? "> DUO MODE (WITH BOT)" : "  DUO MODE (WITH BOT)", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 + 50, 30, botColor);
        DrawText("FIRST TO 3000 WINS!", SCREEN_WIDTH/2 - 140, SCREEN_HEIGHT/2 + 110, 25, GREEN);
        DrawText("PRESS ENTER TO START", SCREEN_WIDTH/2 - 160, SCREEN_HEIGHT/2 + 180, 25, LIGHTGRAY);
    }
    else if (currentGameState == GAME_PLAYING || currentGameState == GAME_PAUSED || currentGameState == GAME_OVER) {
        Color shipC = (spawnTimer > 0) ? Fade(SKYBLUE, 0.5f) : WHITE;

        float shipSize = 75.0f;
        float origin = shipSize / 2.0f;

        DrawTexturePro(shipTex, {0,0,(float)shipTex.width,(float)shipTex.height},
               {ship.position.x, ship.position.y, shipSize, shipSize},
               {origin, origin}, visualRotation + 90, shipC);

        if(bot.active) {
            DrawTexturePro(shipTex, {0,0,(float)shipTex.width,(float)shipTex.height},
                           {bot.entity.position.x, bot.entity.position.y, shipSize, shipSize},
                           {origin, origin}, bot.rotation + 90, RED);
        }

        for(int i=0; i<MAX_ASTEROIDS; i++) if(astData[i].active) {
            
            float r = (float)astData[i].size * 17.0f;

            DrawTexturePro(asteroidTex, {0,0,(float)asteroidTex.width,(float)asteroidTex.height},
                          {asteroids[i].position.x, asteroids[i].position.y, r*2, r*2},
                          {r, r}, 0, WHITE);
        }

        for (int i = 0; i < MAX_EXPLOSIONS; i++) {
            if (explosions[i].active) {
               
                explosions[i].frameCounter++;
                if (explosions[i].frameCounter >= 5) {
                    explosions[i].currentFrame++;
                    explosions[i].frameCounter = 0;
                }

                if (explosions[i].currentFrame >= 6) {
                    explosions[i].active = false;
                } else {
                 
                    int frameWidth = explosionTex.width / 6;
                    int frameHeight = explosionTex.height;

                    
                    Rectangle src = {
                        (float)(explosions[i].currentFrame * frameWidth),
                        0.0f,
                        (float)frameWidth,
                        (float)frameHeight
                    };

                    
                    float drawW = explosions[i].scale;
                   
                    float drawH = drawW * ((float)frameHeight / (float)frameWidth);

                    Rectangle dst = {
                        explosions[i].position.x,
                        explosions[i].position.y,
                        drawW,
                        drawH
                    };

                    Vector2 origin = { drawW / 2.0f, drawH / 2.0f };

                    DrawTexturePro(explosionTex, src, dst, origin, 0.0f, WHITE);
                }
            }
        }
        for(int i=0; i<MAX_BULLETS; i++) if(bulData[i].active) DrawCircle(bullets[i].position.x, bullets[i].position.y, 4, (bullets[i].type == 3) ? YELLOW : ORANGE);
        DrawText(TextFormat("PLAYER: %i / %i", playerScore, WIN_SCORE), 40, 40, 30, RAYWHITE);
        if(bot.active) DrawText(TextFormat("BOT: %i / %i", bot.score, WIN_SCORE), 40, 75, 30, RED);
        DrawText(TextFormat("LIVES: %i", lives), 40, 110, 30, GREEN);

        if (currentGameState == GAME_PAUSED) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6f));
            DrawText("PAUSA", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 40, 50, YELLOW);
            DrawText("ESC para continuar - Q para salir", SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT/2 + 20, 20, RAYWHITE);
        }
        if (currentGameState == GAME_OVER) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.8f));
            if (playerWon) DrawText("YOU WIN!", SCREEN_WIDTH/2 - 140, SCREEN_HEIGHT/2 - 50, 60, GOLD);
            else DrawText("GAME OVER", SCREEN_WIDTH/2 - 160, SCREEN_HEIGHT/2 - 50, 60, RED);
            DrawText("ENTER PARA VOLVER AL MENU", SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT/2 + 20, 20, LIGHTGRAY);
        }
    }
    EndDrawing();
}
int main() {
    InitWindow(1280, 720, "Asteroid Hunter");
        SetExitKey(0);
    InitAudioDevice();
    SetTargetFPS(60);

    background = LoadTexture("resources/space_bg3.png");

    Image imgNave = LoadImage("resources/ship0.png");
    SuperCleanSprite(&imgNave, false);
    shipTex = LoadTextureFromImage(imgNave);
    UnloadImage(imgNave);

    Image imgAst = LoadImage("resources/asteroid0.png");
    SuperCleanSprite(&imgAst, false);
    asteroidTex = LoadTextureFromImage(imgAst);
    UnloadImage(imgAst);

    Image imgExp = LoadImage("resources/explosion7.png");
    SuperCleanSprite(&imgExp, true);

    explosionTex = LoadTextureFromImage(imgExp);
    SetTextureFilter(explosionTex, TEXTURE_FILTER_POINT);
    UnloadImage(imgExp);


    fxShot = LoadSound("resources/shot.wav");
    fxExplosion = LoadSound("resources/explosion.wav");
    music = LoadMusicStream("resources/music1.mp3");

    SetSoundVolume(fxShot, 0.5f);
    SetSoundVolume(fxExplosion, 0.5f);
    SetMusicVolume(music, 0.4f);

    ship.position = { 1280.0f/2, 720.0f/2 };
        ship.width = 40;
        ship.height = 40;ship.id = 9999;

        bot.entity.width = 30;
        bot.entity.height = 30;
    ResetGame();

#if defined(PLATFORM_WEB)

    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }


    UnloadSound(fxShot);
    UnloadSound(fxExplosion);
    UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();
#endif
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    return 0;
}
