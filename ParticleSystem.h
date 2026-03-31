#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h>
#include <memory>
#include "TextureLoad.h"

struct Particle
{
    glm::vec2 pos;
    glm::vec2 speed;
    glm::vec4 color;
    float size;
    float life; // 1.0 = full life, 0.0 = dead
};

// Abstract rendering backend
class RenderBackend
{
public:
    virtual ~RenderBackend() = default;
    virtual void setup(const std::vector<Particle> &particles) = 0;
    virtual void updateBuffer(const std::vector<Particle> &particles) = 0;
    virtual void draw(int count) = 0;
};

// Non‑instanced backend: one interleaved VBO, glDrawArrays
class NonInstancedBackend : public RenderBackend
{
public:
    NonInstancedBackend();
    ~NonInstancedBackend();
    void setup(const std::vector<Particle> &particles) override;
    void updateBuffer(const std::vector<Particle> &particles) override;
    void draw(int count) override;

private:
    GLuint VAO, VBO;
};

// Instanced backend: separate position/color buffers with divisors
class InstancedBackend : public RenderBackend
{
public:
    InstancedBackend();
    ~InstancedBackend();
    void setup(const std::vector<Particle> &particles) override;
    void updateBuffer(const std::vector<Particle> &particles) override;
    void draw(int count) override;

private:
    GLuint VAO, posVBO, colorVBO, sizeVBO;
    int currentCount = 0;
};

// Abstract particle system
class ParticleSystem
{
public:
    ParticleSystem(int count, bool useInstanced);
    virtual ~ParticleSystem();

    virtual void update(float dt);
    virtual void draw();

    int getParticleCount() const { return (int)particles.size(); }
    void setTexture(const char *path)
    {
        if (particleTexture)
            glDeleteTextures(1, &particleTexture);
        particleTexture = loadTexture(path);
    }

protected:
    virtual void respawn(Particle &p) = 0; // initialize a dead particle
    virtual void updateParticle(Particle &p, float dt) = 0;

    GLuint particleTexture = 0;

    std::vector<Particle> particles;
    std::unique_ptr<RenderBackend> backend;
    float timeAccumulator = 0.0f;
};

// -------------------------------------------------------------------
// Concrete effect systems
// -------------------------------------------------------------------
class FireworkSystem : public ParticleSystem
{
public:
    FireworkSystem(int count, bool useInstanced = true);
    ~FireworkSystem(); // cleanup line buffers
    void update(float dt) override;
    void draw() override;

protected:
    void respawn(Particle &p) override;
    void updateParticle(Particle &p, float dt) override;

private:
    glm::vec2 emissionPoint = glm::vec2(0.0f, 0.0f); // center
    GLuint lineVAO, lineVBO, lineColorVBO;
    int lineCount = 0;
    void setupLineRendering();
    void updateLineBuffer(const std::vector<Particle> &particles);
};

class SmokeSystem : public ParticleSystem
{
public:
    SmokeSystem(int count, bool useInstanced = true);

protected:
    void respawn(Particle &p) override;
    void updateParticle(Particle &p, float dt) override;
};

class FireSystem : public ParticleSystem
{
public:
    FireSystem(int count, bool useInstanced = true);

protected:
    void respawn(Particle &p) override;
    void updateParticle(Particle &p, float dt) override;
};

class FluidSystem : public ParticleSystem
{
public:
    FluidSystem(int count, bool useInstanced = true);

protected:
    void respawn(Particle &p) override;
    void updateParticle(Particle &p, float dt) override;

private:
    glm::vec2 mouseAttractor = glm::vec2(0.0f);
};

class CloudSystem : public ParticleSystem
{
public:
    CloudSystem(int count, bool useInstanced = true);

protected:
    void respawn(Particle &p) override;
    void updateParticle(Particle &p, float dt) override;

private:
    glm::vec2 emissionArea = glm::vec2(-0.5f, -0.8f); // Area where clouds appear
};

// Add this class declaration after CloudSystem
class GravityFireworkSystem : public ParticleSystem
{
public:
    GravityFireworkSystem(int count, bool useInstanced = true);

protected:
    void respawn(Particle &p) override;
    void updateParticle(Particle &p, float dt) override;

private:
    glm::vec2 launchPoint = glm::vec2(0.0f, -0.9f); // Launch from bottom center
};

class GalaxySystem : public ParticleSystem
{
public:
    GalaxySystem(int count, bool useInstanced = true);
    void respawn(Particle &p) override;
    void updateParticle(Particle &p, float dt) override;

    // Parameters to control the spiral shape and rotation
    void setParameters(int arms, float tightness, float rotSpeed, float maxRad, float minRad = 0.1f);

private:
    int armCount = 2;           // number of spiral arms
    float armTightness = 2.0f;  // winding tightness (larger = tighter)
    float rotationSpeed = 1.5f; // base angular speed factor
    float radialRange = 0.8f;   // maximum radius (in normalized coordinates)
    float minRadius = 0.1f;     // inner radius where particles start
};

class RingFireworkSystem : public ParticleSystem
{
public:
    RingFireworkSystem(int count, bool useInstanced = true);

protected:
    void respawn(Particle &p) override;
    void updateParticle(Particle &p, float dt) override;
    void update(float dt) override;

private:
    float currentRadius;
    bool ringExpanding;
    float ringDuration;
    float currentRingTime;
    glm::vec4 ringColor;
    glm::vec2 ringCenter;
};
