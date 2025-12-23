#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <rlgl.h>


#define MAX_WALLS 100
#define MAX_TARGETS 10
#define MAX_PARTICLES 200
#define MAX_KILLFEED 5
#define GRAVITY 18.0f
#define JUMP_FORCE 8.0f
#define WALK_SPEED 6.0f
#define SENSITIVITY 0.25f



typedef enum { 
    WPN_RIFLE, 
    WPN_PISTOL, 
    WPN_KNIFE, 
    WPN_GRENADE 
} WeaponType;

typedef enum {
    PARTICLE_BLOOD,
    PARTICLE_SPARK,
    PARTICLE_EXPLOSION,
    PARTICLE_SMOKE
} ParticleType;

typedef struct {
    Vector3 position;
    Vector3 velocity;
    Color color;
    float size;
    float life; 
    ParticleType type;
    bool active;
} Particle;

typedef struct {
    Vector3 position;
    Vector3 size;
    Color color;
    Color outlineColor;
} Wall;

typedef struct {
    Vector3 position;
    bool active;
    int health;
    float hitTimer; 
    float deathTimer; 
    int id; 
} Target;

typedef struct {
    Vector3 position;
    Vector3 velocity;
    float timer;
    bool active;
    bool exploding;
} Grenade;

typedef struct {
    char killer[32];
    char victim[32];
    WeaponType weapon;
    bool headshot;
    float timer;
    bool active;
} KillMessage;

typedef struct {
    Camera3D camera;
    WeaponType weapon;
    WeaponType lastWeapon;
    
    
    Vector3 velocity;
    bool isGrounded;
    
    
    int ammoRifle;
    int reserveRifle;
    int ammoPistol;
    int reservePistol;
    int grenades;
    int health;
    
    
    float shootCooldown;
    float recoilOffset;     
    float recoilPitch;      
    float equipTimer;       
    float walkTimer;        
    Vector2 weaponSway;     
    float muzzleFlashTimer; 
    
    
    float reloadTimer;
    bool isReloading;
    float inspectTimer;
    bool isInspecting;
} Player;


Wall mapWalls[MAX_WALLS];
Target targets[MAX_TARGETS];
Particle particles[MAX_PARTICLES];
KillMessage killFeed[MAX_KILLFEED];
int wallCount = 0;
Grenade activeNade = { 0 };


void AddKillMsg(const char* killer, const char* victim, WeaponType wpn, bool hs) {
    
    for (int i = MAX_KILLFEED - 1; i > 0; i--) {
        killFeed[i] = killFeed[i-1];
    }
    
    strncpy(killFeed[0].killer, killer, 31);
    strncpy(killFeed[0].victim, victim, 31);
    killFeed[0].weapon = wpn;
    killFeed[0].headshot = hs;
    killFeed[0].timer = 5.0f; 
    killFeed[0].active = true;
}


void SpawnParticle(Vector3 pos, Vector3 vel, Color col, float size, float life, ParticleType type) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i].position = pos;
            particles[i].velocity = vel;
            particles[i].color = col;
            particles[i].size = size;
            particles[i].life = life;
            particles[i].type = type;
            particles[i].active = true;
            return;
        }
    }
}

void UpdateParticles(float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;

        particles[i].life -= dt;
        particles[i].position = Vector3Add(particles[i].position, Vector3Scale(particles[i].velocity, dt));

        if (particles[i].type == PARTICLE_BLOOD) {
            particles[i].velocity.y -= GRAVITY * dt; 
        } else if (particles[i].type == PARTICLE_EXPLOSION) {
            particles[i].size += dt * 10.0f; 
            particles[i].color.a = (unsigned char)(particles[i].life * 255.0f * 5.0f);
        } else if (particles[i].type == PARTICLE_SMOKE) {
            particles[i].velocity.y += 2.0f * dt;
            particles[i].size += dt * 2.0f;
        }

        if (particles[i].life <= 0) particles[i].active = false;
    }
}

void DrawParticles3D() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        DrawCube(particles[i].position, particles[i].size, particles[i].size, particles[i].size, particles[i].color);
    }
}


void AddWall(Vector3 pos, Vector3 size, Color col) {
    if (wallCount < MAX_WALLS) {
        mapWalls[wallCount].position = pos;
        mapWalls[wallCount].size = size;
        mapWalls[wallCount].color = col;
        mapWalls[wallCount].outlineColor = ColorBrightness(col, -0.3f);
        wallCount++;
    }
}

void ResetGame() {
    wallCount = 0;
    
    AddWall((Vector3){0, -0.5f, 0}, (Vector3){60, 1, 60}, (Color){80, 80, 80, 255});
    
    
    AddWall((Vector3){-15, 2.5f, 15}, (Vector3){10, 6, 1}, DARKGRAY); 
    AddWall((Vector3){15, 2.5f, -15}, (Vector3){10, 6, 1}, DARKGRAY); 
    
    
    AddWall((Vector3){-5, 1, 5}, (Vector3){2, 2, 2}, ORANGE);
    AddWall((Vector3){5, 1.5f, -5}, (Vector3){3, 3, 3}, BEIGE);
    AddWall((Vector3){0, 1, 10}, (Vector3){2, 2, 6}, BROWN);

    
    for (int i = 0; i < MAX_TARGETS; i++) {
        targets[i].position = (Vector3){ (float)GetRandomValue(-20, 20), 0.0f, (float)GetRandomValue(-20, 20) };
        targets[i].active = true;
        targets[i].health = 100;
        targets[i].hitTimer = 0;
        targets[i].deathTimer = 0;
        targets[i].id = i + 1;
    }
    
    
    for(int i=0; i<MAX_PARTICLES; i++) particles[i].active = false;
    for(int i=0; i<MAX_KILLFEED; i++) killFeed[i].active = false;
}


void DrawWeaponRect(float x, float y, float w, float h, Color c) {
    DrawRectangle((int)x, (int)y, (int)w, (int)h, c);
    DrawRectangleLines((int)x, (int)y, (int)w, (int)h, ColorBrightness(c, -0.3f));
}


Vector2 RotatePoint(Vector2 point, float angle) {
    float s = sinf(angle);
    float c = cosf(angle);
    return (Vector2){ point.x * c - point.y * s, point.x * s + point.y * c };
}

int main() {
    InitWindow(1280, 720, "CS2 Engine - Enhanced 2.0");
    SetTargetFPS(60);
    DisableCursor();

    Player p = { 0 };
    p.camera.position = (Vector3){ 0.0f, 2.0f, -10.0f };
    p.camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
    p.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    p.camera.fovy = 75.0f;
    p.camera.projection = CAMERA_PERSPECTIVE;
    
    
    p.ammoRifle = 30;
    p.reserveRifle = 90;
    p.ammoPistol = 20;
    p.reservePistol = 120;
    p.grenades = 3;
    p.health = 100;
    p.weapon = WPN_RIFLE;
    p.lastWeapon = WPN_RIFLE;
    p.equipTimer = 1.0f;

    ResetGame();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        
        WeaponType targetWeapon = p.weapon;
        if (IsKeyPressed(KEY_ONE)) targetWeapon = WPN_RIFLE;
        if (IsKeyPressed(KEY_TWO)) targetWeapon = WPN_PISTOL;
        if (IsKeyPressed(KEY_THREE)) targetWeapon = WPN_KNIFE;
        if (IsKeyPressed(KEY_FOUR)) targetWeapon = WPN_GRENADE;
        
        if (targetWeapon != p.weapon) {
            p.lastWeapon = p.weapon;
            p.weapon = targetWeapon;
            p.equipTimer = 0.0f; 
            p.shootCooldown = 0.5f;
            p.isReloading = false; 
            p.isInspecting = false; 
            p.reloadTimer = 0;
        }
        
        
        if (IsKeyPressed(KEY_F) && !p.isReloading && p.weapon != WPN_GRENADE) {
            p.isInspecting = true;
            p.inspectTimer = 0.0f;
        }
        
        
        if (IsKeyPressed(KEY_R) && !p.isReloading && !p.isInspecting && p.weapon != WPN_KNIFE && p.weapon != WPN_GRENADE) {
            bool canReload = false;
            if (p.weapon == WPN_RIFLE && p.ammoRifle < 30 && p.reserveRifle > 0) canReload = true;
            if (p.weapon == WPN_PISTOL && p.ammoPistol < 20 && p.reservePistol > 0) canReload = true;
            
            if (canReload) {
                p.isReloading = true;
                p.reloadTimer = 0.0f;
            }
        }
        
        if (IsKeyPressed(KEY_T)) ResetGame();

        
        Vector2 mouseDelta = GetMouseDelta();
        UpdateCameraPro(&p.camera, 
            (Vector3){
                (IsKeyDown(KEY_W) - IsKeyDown(KEY_S)) * WALK_SPEED * dt,
                (IsKeyDown(KEY_D) - IsKeyDown(KEY_A)) * WALK_SPEED * dt,
                0
            },
            (Vector3){
                mouseDelta.x * SENSITIVITY,
                mouseDelta.y * SENSITIVITY + (p.recoilPitch * 0.1f), 
                0
            },
            0.0f
        );
        p.recoilPitch = Lerp(p.recoilPitch, 0, dt * 5.0f); 

        
        p.velocity.y -= GRAVITY * dt;
        p.camera.position.y += p.velocity.y * dt;

        if (p.camera.position.y <= 2.0f) {
            p.camera.position.y = 2.0f;
            p.velocity.y = 0;
            p.isGrounded = true;
        } else {
            p.isGrounded = false;
        }

        if (IsKeyPressed(KEY_SPACE) && p.isGrounded) {
            p.velocity.y = JUMP_FORCE;
        }

        
        bool isMoving = (IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) || IsKeyDown(KEY_D)) && p.isGrounded;
        if (isMoving) p.walkTimer += dt * 10.0f;
        else p.walkTimer = 0.0f;

        
        p.weaponSway.x = Lerp(p.weaponSway.x, -mouseDelta.x * 2.0f, dt * 5.0f);
        p.weaponSway.y = Lerp(p.weaponSway.y, -mouseDelta.y * 2.0f, dt * 5.0f);

        
        p.equipTimer = fminf(p.equipTimer + dt * 3.0f, 1.0f);

        
        if (p.isInspecting) {
            p.inspectTimer += dt;
            if (p.inspectTimer > 3.14f * 2) { 
                 p.isInspecting = false; 
            }
        }
        
        
        if (p.isReloading) {
            p.reloadTimer += dt;
            float reloadTime = (p.weapon == WPN_RIFLE) ? 2.0f : 1.5f;
            
            if (p.reloadTimer >= reloadTime) {
                
                if (p.weapon == WPN_RIFLE) {
                    int needed = 30 - p.ammoRifle;
                    int take = (needed > p.reserveRifle) ? p.reserveRifle : needed;
                    p.ammoRifle += take;
                    p.reserveRifle -= take;
                } else if (p.weapon == WPN_PISTOL) {
                    int needed = 20 - p.ammoPistol;
                    int take = (needed > p.reservePistol) ? p.reservePistol : needed;
                    p.ammoPistol += take;
                    p.reservePistol -= take;
                }
                p.isReloading = false;
                p.reloadTimer = 0;
            }
        }

        if (p.shootCooldown > 0) p.shootCooldown -= dt;
        if (p.recoilOffset > 0) p.recoilOffset -= dt * 5.0f;
        if (p.muzzleFlashTimer > 0) p.muzzleFlashTimer -= dt;

        
        bool isFiring = (p.weapon == WPN_RIFLE) ? IsMouseButtonDown(MOUSE_LEFT_BUTTON) : IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        
        
        if (isFiring && (p.isInspecting || p.isReloading) && p.weapon != WPN_GRENADE) {
            p.isInspecting = false;
            p.isReloading = false; 
        }

        if (isFiring && p.shootCooldown <= 0 && p.equipTimer >= 0.8f && !p.isReloading) {
            bool shotFired = false;
            int dmg = 0;
            float spread = 0.0f;
            float range = 1000.0f; 

            if (p.weapon == WPN_RIFLE && p.ammoRifle > 0) { 
                p.ammoRifle--; p.shootCooldown = 0.1f; p.recoilOffset = 0.2f; p.recoilPitch = 2.0f; dmg = 35; shotFired = true; spread = 0.05f; p.muzzleFlashTimer = 0.05f;
            }
            else if (p.weapon == WPN_PISTOL && p.ammoPistol > 0) { 
                p.ammoPistol--; p.shootCooldown = 0.15f; p.recoilOffset = 0.15f; p.recoilPitch = 1.5f; dmg = 25; shotFired = true; spread = 0.02f; p.muzzleFlashTimer = 0.05f;
            }
            else if (p.weapon == WPN_KNIFE) { 
                p.shootCooldown = 0.5f; p.recoilOffset = -0.5f; dmg = 55; shotFired = true; range = 3.5f; 
            }
            else if (p.weapon == WPN_GRENADE && p.grenades > 0 && !activeNade.active) {
                p.grenades--;
                activeNade.active = true;
                activeNade.timer = 2.0f;
                activeNade.position = p.camera.position;
                Vector3 dir = Vector3Normalize(Vector3Subtract(p.camera.target, p.camera.position));
                dir.y += 0.2f; 
                activeNade.velocity = Vector3Scale(dir, 20.0f);
                p.shootCooldown = 1.0f;
                p.equipTimer = 0.0f; 
            }

            
            if (shotFired && p.weapon != WPN_GRENADE) {
                Ray ray = GetMouseRay((Vector2){1280/2, 720/2}, p.camera);
                
                
                if (p.weapon != WPN_KNIFE) {
                    ray.direction.x += ((float)GetRandomValue(-100, 100)/10000.0f) * spread;
                    ray.direction.y += ((float)GetRandomValue(-100, 100)/10000.0f) * spread;
                }

                bool hitAnything = false;
                
                
                for (int i=0; i<MAX_TARGETS; i++) {
                    if (!targets[i].active || targets[i].health <= 0) continue;
                    
                    BoundingBox hBox = {(Vector3){targets[i].position.x-0.3f, 1.4f, targets[i].position.z-0.3f}, (Vector3){targets[i].position.x+0.3f, 1.9f, targets[i].position.z+0.3f}};
                    BoundingBox bBox = {(Vector3){targets[i].position.x-0.4f, 0.0f, targets[i].position.z-0.4f}, (Vector3){targets[i].position.x+0.4f, 1.4f, targets[i].position.z+0.4f}};
                    
                    RayCollision headHit = GetRayCollisionBox(ray, hBox);
                    RayCollision bodyHit = GetRayCollisionBox(ray, bBox);

                    
                    bool validHit = false;
                    bool isHeadshot = false;

                    if (headHit.hit && headHit.distance < range) { validHit = true; isHeadshot = true; }
                    else if (bodyHit.hit && bodyHit.distance < range) { validHit = true; isHeadshot = false; }

                    if (validHit) {
                        hitAnything = true;
                        float hitDmg = isHeadshot ? dmg * 4 : dmg; 
                        targets[i].health -= hitDmg;
                        targets[i].hitTimer = 0.2f;
                        
                        
                        for(int p=0; p<5; p++) {
                            Vector3 hitPos = isHeadshot ? headHit.point : bodyHit.point;
                            SpawnParticle(hitPos, 
                                (Vector3){(float)GetRandomValue(-10,10)*0.1f, (float)GetRandomValue(0,10)*0.1f, (float)GetRandomValue(-10,10)*0.1f}, 
                                RED, 0.1f, 0.5f, PARTICLE_BLOOD);
                        }

                        if (targets[i].health <= 0 && targets[i].deathTimer == 0) {
                            targets[i].deathTimer = 1.5f;
                            
                            AddKillMsg("Player", "Enemy", p.weapon, isHeadshot);
                        }
                        break; 
                    }
                }

                
                if (!hitAnything) {
                    for(int i=0; i<wallCount; i++) {
                         RayCollision col = GetRayCollisionBox(ray, (BoundingBox){
                             Vector3Subtract(mapWalls[i].position, Vector3Scale(mapWalls[i].size, 0.5f)),
                             Vector3Add(mapWalls[i].position, Vector3Scale(mapWalls[i].size, 0.5f))
                         });
                         if(col.hit && col.distance < range) {
                             SpawnParticle(col.point, (Vector3){col.normal.x*2, col.normal.y*2, col.normal.z*2}, YELLOW, 0.05f, 0.2f, PARTICLE_SPARK);
                             break;
                         }
                    }
                }
            }
        }

        
        if (activeNade.active) {
            if (!activeNade.exploding) {
                activeNade.velocity.y -= GRAVITY * dt;
                activeNade.position = Vector3Add(activeNade.position, Vector3Scale(activeNade.velocity, dt));
                
                if (activeNade.position.y < 0.2f) { 
                    activeNade.position.y = 0.2f; 
                    activeNade.velocity.y *= -0.5f; 
                    activeNade.velocity.x *= 0.7f; 
                    activeNade.velocity.z *= 0.7f;
                }

                activeNade.timer -= dt;
                if (activeNade.timer <= 0) {
                    activeNade.exploding = true;
                    activeNade.timer = 0.5f; 
                    for(int i=0; i<30; i++) {
                        SpawnParticle(activeNade.position, 
                            (Vector3){(float)GetRandomValue(-50,50)*0.1f, (float)GetRandomValue(-50,50)*0.1f, (float)GetRandomValue(-50,50)*0.1f}, 
                            ORANGE, 0.5f, 0.6f, PARTICLE_EXPLOSION);
                    }
                    
                    for(int i=0; i<MAX_TARGETS; i++) {
                        if(targets[i].active && Vector3Distance(targets[i].position, activeNade.position) < 8.0f) {
                            targets[i].health -= 80;
                            targets[i].hitTimer = 0.2f;
                             if (targets[i].health <= 0 && targets[i].deathTimer == 0) {
                                 targets[i].deathTimer = 1.5f;
                                 AddKillMsg("Player", "Enemy", WPN_GRENADE, false);
                             }
                        }
                    }
                }
            } else {
                activeNade.timer -= dt;
                if (activeNade.timer <= 0) activeNade.active = false;
            }
        }
        
        UpdateParticles(dt);
        
        
        for (int i=0; i<MAX_KILLFEED; i++) {
            if (killFeed[i].active) {
                killFeed[i].timer -= dt;
                if (killFeed[i].timer <= 0) killFeed[i].active = false;
            }
        }

        
        BeginDrawing();
            ClearBackground(SKYBLUE);
            BeginMode3D(p.camera);
                
                DrawGrid(60, 1.0f);

                for (int i=0; i<wallCount; i++) {
                    DrawCube(mapWalls[i].position, mapWalls[i].size.x, mapWalls[i].size.y, mapWalls[i].size.z, mapWalls[i].color);
                    DrawCubeWires(mapWalls[i].position, mapWalls[i].size.x, mapWalls[i].size.y, mapWalls[i].size.z, mapWalls[i].outlineColor);
                }

                for (int i=0; i<MAX_TARGETS; i++) {
                    if (targets[i].active) {
                        Vector3 pos = targets[i].position;
                        if (targets[i].health <= 0) {
                            targets[i].deathTimer -= dt;
                            if (targets[i].deathTimer <= 0) targets[i].active = false;
                            DrawCube((Vector3){pos.x, 0.2f, pos.z}, 1.5f, 0.4f, 2.5f, DARKGRAY);
                        } else {
                            Color skin = targets[i].hitTimer > 0 ? RED : BEIGE;
                            Color shirt = targets[i].hitTimer > 0 ? RED : BLUE;
                            
                            DrawCube((Vector3){pos.x-0.2f, 0.7f, pos.z}, 0.25f, 1.4f, 0.3f, DARKBLUE);
                            DrawCube((Vector3){pos.x+0.2f, 0.7f, pos.z}, 0.25f, 1.4f, 0.3f, DARKBLUE);
                            DrawCube((Vector3){pos.x, 1.8f, pos.z}, 0.9f, 0.9f, 0.5f, shirt);
                            DrawCube((Vector3){pos.x, 2.5f, pos.z}, 0.5f, 0.5f, 0.5f, skin);
                            DrawCube((Vector3){pos.x-0.6f, 1.8f, pos.z}, 0.2f, 0.8f, 0.2f, skin);
                            DrawCube((Vector3){pos.x+0.6f, 1.8f, pos.z}, 0.2f, 0.8f, 0.2f, skin);
                        }
                        targets[i].hitTimer -= dt;
                    }
                }

                if (activeNade.active && !activeNade.exploding) DrawSphere(activeNade.position, 0.3f, DARKGREEN);
                DrawParticles3D();

            EndMode3D();

            
            
            
            int cx = 1280/2, cy = 720/2;
            int chGap = (int)(p.velocity.y != 0 ? 10 : 5);
            if (p.recoilOffset > 0) chGap += 10;
            DrawRectangle(cx - 10 - chGap, cy - 1, 10, 2, GREEN);
            DrawRectangle(cx + chGap, cy - 1, 10, 2, GREEN);
            DrawRectangle(cx - 1, cy - 10 - chGap, 2, 10, GREEN);
            DrawRectangle(cx - 1, cy + chGap, 2, 10, GREEN);

            
            float bobY = sinf(p.walkTimer) * 10.0f;
            float bobX = cosf(p.walkTimer * 0.5f) * 5.0f;
            float equipY = (1.0f - p.equipTimer) * (1.0f - p.equipTimer) * 400.0f;
            float recoilKick = p.recoilOffset * 100.0f; 
            
            
            float inspectX = 0, inspectY = 0, inspectRot = 0;
            if (p.isInspecting) {
                inspectX = sinf(p.inspectTimer * 2.0f) * 50.0f;
                inspectY = sinf(p.inspectTimer * 4.0f) * 20.0f;
                inspectRot = sinf(p.inspectTimer * 3.0f) * 15.0f; 
            }

            
            float reloadY = 0;
            if (p.isReloading) {
                
                reloadY = sinf(p.reloadTimer * 3.14f) * 200.0f;
            }
            
            float wx = 1280 - 300 + p.weaponSway.x + bobX + inspectX;
            float wy = 720 - 300 + p.weaponSway.y + bobY + equipY + recoilKick + reloadY + inspectY;

            if (p.weapon == WPN_RIFLE) {
                
                rlPushMatrix();
                rlTranslatef(wx, wy, 0);
                rlRotatef(inspectRot, 0, 0, 1);
                rlTranslatef(-wx, -wy, 0);

                DrawWeaponRect(wx, wy + 50, 40, 150, (Color){101, 67, 33, 255});
                DrawWeaponRect(wx - 20, wy - 50, 60, 150, (Color){40, 40, 40, 255});
                DrawWeaponRect(wx, wy - 200, 20, 150, (Color){20, 20, 20, 255});
                DrawWeaponRect(wx - 30, wy + 20, 40, 80, (Color){139, 69, 19, 255});
                if (p.muzzleFlashTimer > 0) DrawCircle((int)wx + 10, (int)wy - 220, 40 + (float)GetRandomValue(-10,10), (Color){255, 255, 0, 200});

                rlPopMatrix();
            } 
            else if (p.weapon == WPN_PISTOL) {
                rlPushMatrix();
                rlTranslatef(wx, wy, 0);
                rlRotatef(inspectRot, 0, 0, 1);
                rlTranslatef(-wx, -wy, 0);

                DrawWeaponRect(wx + 50, wy + 100, 40, 100, (Color){30, 30, 30, 255});
                DrawWeaponRect(wx + 30, wy + 50, 80, 100, (Color){50, 50, 50, 255});
                if (p.muzzleFlashTimer > 0) DrawCircle((int)wx + 70, (int)wy + 30, 25 + (float)GetRandomValue(-5,5), (Color){255, 200, 0, 200});

                rlPopMatrix();
            }
            else if (p.weapon == WPN_KNIFE) {
                float stabY = (p.recoilOffset < 0) ? p.recoilOffset * 200.0f : 0;
                
                rlPushMatrix();
                rlTranslatef(wx, wy, 0);
                rlRotatef(inspectRot * 2.0f, 0, 0, 1); 
                rlTranslatef(-wx, -wy, 0);
                
                DrawWeaponRect(wx + 80, wy + 100 + stabY, 30, 120, (Color){40, 30, 20, 255}); 
                DrawWeaponRect(wx + 85, wy - 20 + stabY, 20, 120, LIGHTGRAY);
                
                rlPopMatrix();
            }
            else if (p.weapon == WPN_GRENADE) {
                DrawCircle((int)wx + 100, (int)wy + 100, 40, DARKGREEN);
                DrawCircleLines((int)wx + 100, (int)wy + 100, 40, BLACK);
                DrawRectangle((int)wx + 90, (int)wy + 50, 20, 30, GRAY);
            }

            
            DrawText(TextFormat("HP: %03d", p.health), 20, 670, 40, RED);
            
            char* ammoText = "---";
            if (p.weapon == WPN_RIFLE) ammoText = TextFormat("%d / %d", p.ammoRifle, p.reserveRifle);
            if (p.weapon == WPN_PISTOL) ammoText = TextFormat("%d / %d", p.ammoPistol, p.reservePistol);
            if (p.weapon == WPN_GRENADE) ammoText = TextFormat("%d", p.grenades);
            
            
            if (p.isReloading) DrawText("RELOADING...", 1000, 620, 30, RED);
            else if ((p.weapon == WPN_RIFLE && p.ammoRifle == 0) || (p.weapon == WPN_PISTOL && p.ammoPistol == 0)) DrawText("PRESS 'R'", 1100, 620, 30, RED);

            DrawText(ammoText, 1100, 670, 40, YELLOW);
            DrawText("1:AK 2:GLOCK 3:KNIFE 4:NADE | F:INSPECT R:RELOAD T:RESET", 20, 20, 20, WHITE);

            
            int kfY = 20;
            for (int i=0; i<MAX_KILLFEED; i++) {
                if (killFeed[i].active) {
                    
                    int startX = 1260;
                    
                    
                    int enemyW = MeasureText(killFeed[i].victim, 20);
                    int playerW = MeasureText(killFeed[i].killer, 20);
                    int iconW = 30; 
                    int gap = 10;
                    
                    
                    Color bg = (Color){0, 0, 0, (unsigned char)(killFeed[i].timer > 1.0f ? 150 : killFeed[i].timer * 150.0f)};
                    Color txt = (Color){255, 255, 255, (unsigned char)(killFeed[i].timer > 1.0f ? 255 : killFeed[i].timer * 255.0f)};
                    Color red = (Color){230, 41, 55, (unsigned char)(killFeed[i].timer > 1.0f ? 255 : killFeed[i].timer * 255.0f)};

                    
                    int totalW = enemyW + playerW + iconW + (killFeed[i].headshot ? 30 : 0) + (gap * 4);
                    DrawRectangle(startX - totalW, kfY, totalW, 30, bg);
                    
                    
                    int curX = startX - 10;
                    
                    
                    DrawText(killFeed[i].victim, curX - enemyW, kfY + 5, 20, txt);
                    curX -= (enemyW + gap);
                    
                    
                    if (killFeed[i].headshot) {
                        DrawCircle(curX - 10, kfY + 15, 8, red);
                        DrawCircle(curX - 10, kfY + 15, 4, bg); 
                        curX -= (20 + gap);
                    }
                    
                    
                    Color wpnCol = GRAY;
                    const char* wpnShort = "?";
                    if (killFeed[i].weapon == WPN_RIFLE) { wpnCol = DARKBROWN; wpnShort = "AK"; }
                    if (killFeed[i].weapon == WPN_PISTOL) { wpnCol = GRAY; wpnShort = "GL"; }
                    if (killFeed[i].weapon == WPN_KNIFE) { wpnCol = MAROON; wpnShort = "KN"; }
                    if (killFeed[i].weapon == WPN_GRENADE) { wpnCol = DARKGREEN; wpnShort = "HE"; }
                    
                    DrawRectangle(curX - 30, kfY + 5, 30, 20, wpnCol);
                    DrawText(wpnShort, curX - 25, kfY + 8, 10, WHITE);
                    curX -= (30 + gap);
                    
                    
                    DrawText(killFeed[i].killer, curX - playerW, kfY + 5, 20, txt);
                    
                    kfY += 35;
                }
            }

        EndDrawing();
    }
    CloseWindow();
    return 0;
}
