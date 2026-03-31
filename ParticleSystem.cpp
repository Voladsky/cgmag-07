#include "include/ParticleSystem.h"
#include <glm/gtc/random.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "TextureLoad.h"

static const char *vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in float aSize;
out vec4 vColor;
uniform mat4 projection;
void main() {
    gl_PointSize = aSize;
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
})";

static const char *fragmentShaderSource = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;
uniform sampler2D uTexture;
void main() {
    vec4 texColor = texture(uTexture, gl_PointCoord);
    FragColor = vec4(texColor.rgb * vColor.rgb, texColor.a * vColor.a);
})";

static const char *lineVertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
out vec4 vColor;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
})";

static const char *lineFragmentShaderSource = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() {
    FragColor = vColor;
})";

static GLuint createLineShaderProgram()
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &lineVertexShaderSource, nullptr);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &lineFragmentShaderSource, nullptr);
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

static GLuint getLineShaderProgram()
{
    static GLuint prog = createLineShaderProgram();
    return prog;
}

static GLuint createShaderProgram()
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, nullptr);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

static GLuint getShaderProgram()
{
    static GLuint prog = createShaderProgram();
    return prog;
}

NonInstancedBackend::NonInstancedBackend()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // position (vec2)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *)offsetof(Particle, pos));
    glEnableVertexAttribArray(0);
    // color (vec4)
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *)offsetof(Particle, color));
    glEnableVertexAttribArray(1);
    // size (float)
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *)offsetof(Particle, size));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

NonInstancedBackend::~NonInstancedBackend()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void NonInstancedBackend::setup(const std::vector<Particle> &particles)
{
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), particles.data(), GL_DYNAMIC_DRAW);
}

void NonInstancedBackend::updateBuffer(const std::vector<Particle> &particles)
{
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(Particle), particles.data());
}

void NonInstancedBackend::draw(int count)
{
    GLuint prog = getShaderProgram();
    glUseProgram(prog);
    glBindVertexArray(VAO);
    // Draw each particle as a separate draw call
    for (int i = 0; i < count; ++i)
    {
        glDrawArrays(GL_POINTS, i, 1); // Draw one vertex at index i
    }
}

InstancedBackend::InstancedBackend()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &posVBO);
    glGenBuffers(1, &colorVBO);
    glGenBuffers(1, &sizeVBO);

    glBindVertexArray(VAO);
    // position attribute (per instance)
    glBindBuffer(GL_ARRAY_BUFFER, posVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 1); // per instance

    // color attribute (per instance)
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1); // per instance

    // size (attribute)
    glBindBuffer(GL_ARRAY_BUFFER, sizeVBO);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void *)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1); // per instance

    glBindVertexArray(0);
}

InstancedBackend::~InstancedBackend()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &posVBO);
    glDeleteBuffers(1, &colorVBO);
    glDeleteBuffers(1, &sizeVBO); // NEW
}

void InstancedBackend::setup(const std::vector<Particle> &particles)
{
    currentCount = (int)particles.size();
    std::vector<glm::vec2> positions(currentCount);
    std::vector<glm::vec4> colors(currentCount);
    std::vector<float> sizes(currentCount);
    for (size_t i = 0; i < particles.size(); ++i)
    {
        positions[i] = particles[i].pos;
        colors[i] = particles[i].color;
        sizes[i] = particles[i].size;
    }
    glBindBuffer(GL_ARRAY_BUFFER, posVBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec2), positions.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, sizeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizes.size() * sizeof(float), sizes.data(), GL_DYNAMIC_DRAW);
}

void InstancedBackend::updateBuffer(const std::vector<Particle> &particles)
{
    int n = (int)particles.size();
    if (n != currentCount)
    {
        setup(particles); // resize if needed
        return;
    }
    std::vector<glm::vec2> positions(n);
    std::vector<glm::vec4> colors(n);
    std::vector<float> sizes(n);
    for (int i = 0; i < n; ++i)
    {
        positions[i] = particles[i].pos;
        colors[i] = particles[i].color;
        sizes[i] = particles[i].size;
    }
    glBindBuffer(GL_ARRAY_BUFFER, posVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(glm::vec2), positions.data());
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(glm::vec4), colors.data());
    glBindBuffer(GL_ARRAY_BUFFER, sizeVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizes.size() * sizeof(float), sizes.data());
}

void InstancedBackend::draw(int count)
{
    GLuint prog = getShaderProgram();
    glUseProgram(prog);
    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_POINTS, 0, 1, count); // 1 vertex per instance
}

// ---------- ParticleSystem base ----------
ParticleSystem::ParticleSystem(int count, bool useInstanced)
{
    particles.resize(count);
    particleTexture = createWhiteCircleTexture();
    // initialize all particles as dead
    for (auto &p : particles)
        p.life = 0.0f;
    if (useInstanced)
        backend = std::make_unique<InstancedBackend>();
    else
        backend = std::make_unique<NonInstancedBackend>();
    backend->setup(particles);
}

ParticleSystem::~ParticleSystem() = default;

void ParticleSystem::update(float dt)
{
    // respawn dead particles
    for (auto &p : particles)
    {
        if (p.life <= 0.0f)
            respawn(p);
        else
            updateParticle(p, dt);
    }
    backend->updateBuffer(particles);
}

void ParticleSystem::draw()
{
    GLuint prog = getShaderProgram();
    glUseProgram(prog);
    glm::mat4 proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(prog, "projection"), 1, GL_FALSE, &proj[0][0]);

    // Bind texture
    if (particleTexture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, particleTexture);
        glUniform1i(glGetUniformLocation(prog, "uTexture"), 0);
    }

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    backend->draw((int)particles.size());
}
// Implementation in FireworkSystem.cpp

FireworkSystem::FireworkSystem(int count, bool useInstanced)
    : ParticleSystem(count, useInstanced)
{
    setupLineRendering();
    setTexture("spark.png");
}
FireworkSystem::~FireworkSystem()
{
    glDeleteVertexArrays(1, &lineVAO);
    glDeleteBuffers(1, &lineVBO);
    glDeleteBuffers(1, &lineColorVBO);
}

void FireworkSystem::setupLineRendering()
{
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glGenBuffers(1, &lineColorVBO);

    glBindVertexArray(lineVAO);
    // positions
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void *)0);
    glEnableVertexAttribArray(0);
    // colors
    glBindBuffer(GL_ARRAY_BUFFER, lineColorVBO);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void *)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void FireworkSystem::updateLineBuffer(const std::vector<Particle> &particles)
{
    std::vector<glm::vec2> positions;
    std::vector<glm::vec4> colors;
    positions.reserve(particles.size() * 2);
    colors.reserve(particles.size() * 2);

    for (const auto &p : particles)
    {
        if (rand() % 2 == 1) {
            continue;
        }

        if (p.life <= 0.0f)
            continue;

        // line start at emission point
        positions.push_back(emissionPoint);
        // line end at particle position
        positions.push_back(p.pos);

        // Use constant yellow color for trails
        glm::vec4 yellowColor = glm::vec4(1.0f, 0.9f, 0.2f, .3f);
        
        // color at start: semi‑transparent yellow
        glm::vec4 startColor = yellowColor;
        startColor.a = 0.08f;
        colors.push_back(startColor);
        colors.push_back(yellowColor);
        
    }

    lineCount = (int)positions.size() / 2; // number of lines

    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec2),
                 positions.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, lineColorVBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4),
                 colors.data(), GL_DYNAMIC_DRAW);
}

void FireworkSystem::update(float dt)
{
    // Update main particles
    for (auto &p : particles)
    {
        if (p.life <= 0.0f)
        {
            respawn(p);
        }
        else
        {
            updateParticle(p, dt);
        }
    }

    // Update line buffer with active particles
    updateLineBuffer(particles);

    // Update the point‑particle buffer for rendering
    backend->updateBuffer(particles);
}

void FireworkSystem::draw()
{
    // --- Draw lines (trails) ---
    if (lineCount > 0)
    {
        GLuint lineProg = getLineShaderProgram();
        glUseProgram(lineProg);
        glm::mat4 proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(lineProg, "projection"), 1, GL_FALSE, &proj[0][0]);

        glLineWidth(.02f);

        // Enable blending for lines (so they fade nicely)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindVertexArray(lineVAO);
        glDrawArrays(GL_LINES, 0, lineCount * 2);
    }

    // --- Draw point particles (with texture) ---
    GLuint prog = getShaderProgram();
    glUseProgram(prog);
    glm::mat4 proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(prog, "projection"), 1, GL_FALSE, &proj[0][0]);

    // Bind the particle texture (if any)
    if (particleTexture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, particleTexture);
        glUniform1i(glGetUniformLocation(prog, "uTexture"), 0);
    }

    // Blending should already be enabled; but ensure it's on
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    backend->draw((int)particles.size());
}

void FireworkSystem::respawn(Particle &p)
{
    p.pos = emissionPoint + glm::linearRand(glm::vec2(-.001f, -.001f), glm::vec2(.001f, .001f));
    float angle = glm::linearRand(0.0f, 2.0f * 3.14159f);
    float speed = glm::linearRand(0.5f, 1.5f);
    p.speed = glm::vec2(cos(angle), sin(angle)) * speed;
    
    // Make particles yellow instead of random colors
    p.color = glm::vec4(1.0f, 0.9f, 0.2f, 1.0f);  // Bright yellow
    
    p.life = glm::linearRand(.5f, .7f);
    p.size = 40.0f;  // Increased texture size
}

void FireworkSystem::updateParticle(Particle &p, float dt)
{
    p.pos += p.speed * dt;
    p.life -= dt;
    p.color.a = 1;
    p.size -= p.size * dt * 0.001f;
}
// ---------- SmokeSystem ----------
SmokeSystem::SmokeSystem(int count, bool useInstanced) : ParticleSystem(count, useInstanced) {}

void SmokeSystem::respawn(Particle &p)
{
    p.pos = glm::vec2(glm::linearRand(-0.2f, 0.2f), -0.9f);
    p.speed = glm::vec2(glm::linearRand(-0.2f, 0.2f), glm::linearRand(0.5f, 1.2f));
    p.color = glm::vec4(0.3f, 0.3f, 0.3f, 0.7f);
    p.life = glm::linearRand(1.5f, 3.0f);
    p.size = glm::linearRand(10.0f, 20.0f);
}

void SmokeSystem::updateParticle(Particle &p, float dt)
{
    p.speed += glm::vec2(glm::linearRand(-0.5f, 0.5f), 0.5f) * dt;
    p.pos += p.speed * dt;
    p.life -= dt;
    float t = p.life / 3.0f;
    p.color.a = t * 0.7f;
    p.size -= p.size * dt * .01f;
}

// ---------- FireSystem ----------
FireSystem::FireSystem(int count, bool useInstanced) : ParticleSystem(count, useInstanced) {}

void FireSystem::respawn(Particle &p)
{
    p.pos = glm::vec2(glm::linearRand(-0.15f, 0.15f), -0.85f);
    p.speed = glm::vec2(glm::linearRand(-1.0f, 1.0f), glm::linearRand(1.0f, 2.5f));
    p.color = glm::vec4(1.0f, glm::linearRand(0.3f, 0.7f), 0.0f, 1.0f);
    p.life = glm::linearRand(0.5f, 1.2f);
    p.size = glm::gaussRand(6.0f, 2.0f);
}

void FireSystem::updateParticle(Particle &p, float dt)
{
    p.speed.y += 2.0f * dt; // upward acceleration
    p.speed.x += glm::linearRand(-2.0f, 2.0f) * dt;
    p.pos += p.speed * dt;
    p.life -= dt * 2.0f;
    p.color.g = p.life * 0.7f;
    p.color.a = p.life;
    p.size -= p.size * dt * .01f;
}

// ---------- FluidSystem ----------
FluidSystem::FluidSystem(int count, bool useInstanced) : ParticleSystem(count, useInstanced) {}

void FluidSystem::respawn(Particle &p)
{
    p.pos = glm::vec2(glm::linearRand(-0.9f, 0.9f), glm::linearRand(-0.8f, 0.8f));
    p.speed = glm::vec2(glm::linearRand(-0.5f, 0.5f), glm::linearRand(-0.5f, 0.5f));
    p.color = glm::vec4(0.2f, 0.5f, 0.9f, 0.8f);
    p.life = glm::linearRand(2.0f, 4.0f);
    p.size = 3.0f;
}

void FluidSystem::updateParticle(Particle &p, float dt)
{
    // simple gravity + damping
    p.speed += glm::vec2(0.0f, -2.0f) * dt;
    p.speed *= 0.99f;
    p.pos += p.speed * dt;
    p.life -= dt * 0.5f;
    // bounce off walls
    if (p.pos.x < -0.95f)
    {
        p.size *= 0.80f;
        p.pos.x = -0.95f;
        p.speed.x = -p.speed.x * 0.8f;
    }
    if (p.pos.x > 0.95f)
    {
        p.size *= 0.80f;
        p.pos.x = 0.95f;
        p.speed.x = -p.speed.x * 0.8f;
    }
    if (p.pos.y < -0.95f)
    {
        p.size *= 0.80f;
        p.pos.y = -0.95f;
        p.speed.y = -p.speed.y * 0.8f;
    }
    if (p.pos.y > 0.95f)
    {
        p.size *= 0.80f;
        p.pos.y = 0.95f;
        p.speed.y = -p.speed.y * 0.8f;
    }
    p.color.a = (p.life / 4.0f) * 0.8f;
}

// ---------- CloudSystem (Clouds/Steam Jets) ----------
CloudSystem::CloudSystem(int count, bool useInstanced) : ParticleSystem(count, useInstanced) {}

void CloudSystem::respawn(Particle &p)
{
    // Spawn clouds across a wider area (like steam vents or clouds forming)
    p.pos = glm::vec2(
        glm::linearRand(-0.8f, 0.8f), // X position range
        glm::linearRand(-0.9f, -0.6f) // Y position range (bottom area)
    );

    // Slow, drifting movement
    p.speed = glm::vec2(
        glm::linearRand(-0.2f, 0.2f), // Gentle horizontal drift
        glm::linearRand(0.3f, 0.8f)   // Slow upward movement
    );

    p.size = glm::gaussRand(20.0f, 5.0f);

    // Cloud colors: white to light gray with slight blue tint
    float brightness = glm::linearRand(0.7f, 1.0f);
    p.color = glm::vec4(
        brightness,         // R
        brightness * 0.95f, // G (slightly less)
        brightness * 0.9f,  // B (slightly blueish)
        0.6f                // Alpha - semi-transparent
    );

    // Long lifetime for clouds
    p.life = glm::linearRand(2.0f, 4.0f);
}

void CloudSystem::updateParticle(Particle &p, float dt)
{
    // Clouds drift upward with slight randomness
    p.speed += glm::vec2(
                   glm::linearRand(-0.1f, 0.1f), // Random horizontal gusts
                   glm::linearRand(0.05f, 0.15f) // Gentle upward acceleration
                   ) *
               dt;

    // Apply speed limits (clouds shouldn't move too fast)
    p.speed.x = glm::clamp(p.speed.x, -0.5f, 0.5f);
    p.speed.y = glm::clamp(p.speed.y, 0.1f, 1.2f);

    // Update position
    p.pos += p.speed * dt;

    // Reduce life
    p.life -= dt * 0.8f;

    p.size -= p.size * dt * .01f;

    // Fade out as life decreases
    float lifeRatio = p.life / 4.0f; // Normalize based on max life
    p.color.a = lifeRatio * 0.6f;    // Fade to transparent

    // Slightly expand/scale effect (optional - would require point size control)
    // For now, just visual fade

    // Reset if off screen
    if (p.pos.y > 1.0f || p.pos.x < -1.1f || p.pos.x > 1.1f)
    {
        p.life = 0.0f; // Mark for respawn
    }
}

// ---------- GravityFireworkSystem (Fireworks that rise and fall) ----------
GravityFireworkSystem::GravityFireworkSystem(int count, bool useInstanced) : ParticleSystem(count, useInstanced) {}

void GravityFireworkSystem::respawn(Particle &p)
{
    p.pos = launchPoint;

    p.speed = glm::vec2(
        glm::linearRand(-0.5f, 0.5f), 
        glm::linearRand(2.5f, 4.0f)
    );

    p.color = glm::vec4(
        glm::linearRand(0.8f, 1.0f), 
        glm::linearRand(0.5f, 0.8f), 
        glm::linearRand(0.2f, 0.5f), 
        1.0f);

    p.life = glm::linearRand(0.5f, 1.5f);

    p.size = 3.0f;
}

void GravityFireworkSystem::updateParticle(Particle &p, float dt)
{
    // ===== RISING STATE =====
    // Apply gravity during ascent (slows down the rocket)
    p.speed.y -= 4.0f * dt; // Gravity pulls down

    p.pos += p.speed * dt;
    p.life -= dt;
}

GalaxySystem::GalaxySystem(int count, bool useInstanced)
    : ParticleSystem(count, useInstanced) {}

void GalaxySystem::setParameters(int arms, float tightness, float rotSpeed, float maxRad, float minRad)
{
    armCount = arms;
    armTightness = tightness;
    rotationSpeed = rotSpeed;
    radialRange = maxRad;
    minRadius = minRad;
}

void GalaxySystem::respawn(Particle &p)
{
    float radius = glm::linearRand(minRadius, radialRange);

    int arm = rand() % armCount;

    // Logarithmic spiral: angle = arm_offset + tightness * ln(radius)
    float armOffset = arm * (2.0f * 3.14159265358979f / armCount);
    float angle = armOffset + armTightness * log(radius);

    // Store angle and radius in the speed vector (overloaded for our use)
    p.speed.x = angle;  // current angle
    p.speed.y = radius; // current radius (constant for circular orbit)

    p.pos = glm::vec2(cos(angle), sin(angle)) * radius;

    float t = (radius - minRadius) / (radialRange - minRadius);
    p.color = glm::mix(
        glm::vec4(0.8f, 0.8f, 1.0f, 1.0f), // inner (blue‑white)
        glm::vec4(1.0f, 0.5f, 0.2f, 1.0f), // outer (orange‑red)
        t);
    p.color.a = 0.9f;

    p.size = 5.0f * (1.0f - t * 0.5f);

    p.life = 1.0f;
}

void GalaxySystem::updateParticle(Particle &p, float dt)
{
    float angle = p.speed.x;
    float radius = p.speed.y;

    // Angular velocity = rotationSpeed / radius  (differential rotation)
    float angularVelocity = rotationSpeed / radius;
    angle += angularVelocity * dt;

    angle += glm::linearRand(-0.05f, 0.05f) * dt;

    // Wrap angle to keep it manageable (not strictly necessary)
    if (angle > 2.0f * 3.14159265358979f)
        angle -= 2.0f * 3.14159265358979f;
    if (angle < 0.0f)
        angle += 2.0f * 3.14159265358979f;

    // Update stored angle
    p.speed.x = angle;

    // Update position from new angle (radius remains constant)
    p.pos = glm::vec2(cos(angle), sin(angle)) * radius;

    // Keep particle alive
    p.life = 1.0f;
}

// ---------- RingFireworkSystem ----------
RingFireworkSystem::RingFireworkSystem(int count, bool useInstanced) : ParticleSystem(count, useInstanced)
{
    // Initialize ring parameters
    currentRadius = 0.0f;
    ringExpanding = true;
    ringDuration = 2.0f;
    currentRingTime = 0.0f;
    ringColor = glm::vec4(1.0f, 0.3f, 0.8f, 1.0f); // Pink/purple color

    // Store initial ring center
    ringCenter = glm::vec2(0.0f, 0.0f);
}

void RingFireworkSystem::respawn(Particle &p)
{
    // Calculate position based on current ring radius
    float angle = glm::linearRand(0.0f, 2.0f * 3.14159f);
    p.pos = ringCenter + glm::vec2(cos(angle), sin(angle)) * currentRadius;

    // Speed: particles move radially outward (expansion effect)
    float radialSpeed = glm::linearRand(0.5f, 1.5f);
    glm::vec2 radialDir = glm::normalize(p.pos - ringCenter);
    p.speed = radialDir * radialSpeed;

    // Add slight tangential component for spiral effect
    glm::vec2 tangentialDir = glm::vec2(-radialDir.y, radialDir.x);
    p.speed += tangentialDir * glm::linearRand(-0.3f, 0.3f);

    // Color variations around the main ring color
    p.color = glm::vec4(
        ringColor.r * glm::linearRand(0.8f, 1.2f),
        ringColor.g * glm::linearRand(0.6f, 1.0f),
        ringColor.b * glm::linearRand(0.9f, 1.1f),
        1.0f);
    p.color = glm::clamp(p.color, 0.0f, 1.0f);

    p.life = glm::linearRand(0.8f, 1.2f);
    p.size = glm::linearRand(4.0f, 8.0f);
}

void RingFireworkSystem::updateParticle(Particle &p, float dt)
{
    // Particles expand outward and fade
    p.pos += p.speed * dt;
    p.life -= dt;

    // Fade out as particle ages
    p.color.a = glm::clamp(p.life / 1.2f, 0.0f, 1.0f);

    // Shrink slightly
    p.size -= p.size * dt * 0.5f;

    // Kill particles that go too far or live too long
    float distanceFromCenter = glm::length(p.pos - ringCenter);
    if (distanceFromCenter > currentRadius + 0.5f || p.life <= 0.0f)
    {
        p.life = 0.0f;
    }
}

void RingFireworkSystem::update(float dt)
{
    // Update ring animation
    currentRingTime += dt;

    if (ringExpanding)
    {
        // Expand the ring
        currentRadius += 1.5f * dt;

        // When ring reaches max radius, start contracting
        if (currentRadius >= 0.8f)
        {
            ringExpanding = false;
        }
    }
    else
    {
        // Contract the ring
        currentRadius -= 1.5f * dt;

        // When ring contracts fully, reset and create new ring
        if (currentRadius <= 0.05f)
        {
            currentRadius = 0.0f;
            ringExpanding = true;
            currentRingTime = 0.0f;

            // // Slightly change ring center for next cycle
            // ringCenter = glm::vec2(
            //     glm::linearRand(-0.3f, 0.3f),
            //     glm::linearRand(-0.3f, 0.3f)
            // );

            // Change color slightly for variation
            ringColor = glm::vec4(
                glm::linearRand(0.5f, 1.0f),
                glm::linearRand(0.2f, 0.8f),
                glm::linearRand(0.4f, 1.0f),
                1.0f);
        }
    }

    // Update all particles normally
    for (auto &p : particles)
    {
        if (p.life <= 0.0f)
        {
            respawn(p);
        }
        else
        {
            updateParticle(p, dt);
        }
    }

    backend->updateBuffer(particles);
}