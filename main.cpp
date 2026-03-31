#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include "ParticleSystem.h"

#define NUM_PARTICLES 100000

// Forward declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, ParticleSystem*& currentSystem,
                  std::unique_ptr<FireworkSystem>& firework,
                  std::unique_ptr<SmokeSystem>& smoke,
                  std::unique_ptr<FireSystem>& fire,
                  std::unique_ptr<FluidSystem>& fluid,
                  std::unique_ptr<CloudSystem>& cloud,
                  std::unique_ptr<GravityFireworkSystem>& gravity,
                  std::unique_ptr<GalaxySystem>& spiral,
                  std::unique_ptr<RingFireworkSystem>& ring,
                  bool& useInstanced);

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

int main()
{
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Particle Effects Demo", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);  // allow gl_PointSize in shader

    bool useInstanced = true;   // start with instanced backend
    auto firework = std::make_unique<FireworkSystem>(NUM_PARTICLES, useInstanced);
    auto smoke    = std::make_unique<SmokeSystem>(NUM_PARTICLES, useInstanced);
    auto fire     = std::make_unique<FireSystem>(NUM_PARTICLES, useInstanced);
    auto fluid    = std::make_unique<FluidSystem>(NUM_PARTICLES, useInstanced);
    auto cloud    = std::make_unique<CloudSystem>(NUM_PARTICLES, useInstanced);
    auto gravity  = std::make_unique<GravityFireworkSystem>(NUM_PARTICLES, useInstanced);
    auto spiral   = std::make_unique<GalaxySystem>(NUM_PARTICLES, useInstanced);
    auto ring     = std::make_unique<RingFireworkSystem>(NUM_PARTICLES, useInstanced);
    ParticleSystem* currentSystem = firework.get();

    float lastTime = glfwGetTime();
    float lastFPSPrintTime = glfwGetTime();
    float frameCount = 0;
    float fps = 0;

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float dt = currentTime - lastTime;
        lastTime = currentTime;
        
        // Calculate FPS
        frameCount++;
        if (currentTime - lastFPSPrintTime >= 1.0f) {
            fps = frameCount;
            frameCount = 0;
            lastFPSPrintTime = currentTime;
            
            // Print FPS to stdout
            std::cout << "FPS: " << fps << std::endl;
        }
        
        if (dt > 0.033f) dt = 0.033f; // clamp

        processInput(window, currentSystem, firework, smoke, fire, fluid, cloud, gravity, spiral, ring, useInstanced);

        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Update and draw the active system
        currentSystem->update(dt);
        currentSystem->draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window, ParticleSystem*& currentSystem,
                  std::unique_ptr<FireworkSystem>& firework,
                  std::unique_ptr<SmokeSystem>& smoke,
                  std::unique_ptr<FireSystem>& fire,
                  std::unique_ptr<FluidSystem>& fluid,
                  std::unique_ptr<CloudSystem>& cloud,
                  std::unique_ptr<GravityFireworkSystem>& gravity,
                  std::unique_ptr<GalaxySystem>& spiral,
                  std::unique_ptr<RingFireworkSystem>& ring,
                  bool& useInstanced)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Switch effects
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) currentSystem = firework.get();
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) currentSystem = smoke.get();
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) currentSystem = fire.get();
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) currentSystem = fluid.get();
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) currentSystem = cloud.get();
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) currentSystem = gravity.get();
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) currentSystem = spiral.get();
    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) currentSystem = ring.get();

    // Toggle backend (recreates the active system with new backend)
    static bool bWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bWasPressed) {
        useInstanced = !useInstanced;
        
        // Recreate all systems with the new backend
        int count = currentSystem->getParticleCount();
        
        firework.reset(new FireworkSystem(count, useInstanced));
        smoke.reset(new SmokeSystem(count, useInstanced));
        fire.reset(new FireSystem(count, useInstanced));
        fluid.reset(new FluidSystem(count, useInstanced));
        cloud.reset(new CloudSystem(count, useInstanced));
        gravity.reset(new GravityFireworkSystem(count, useInstanced));
        spiral.reset(new GalaxySystem(count, useInstanced));
        ring.reset(new RingFireworkSystem(count, useInstanced));

        currentSystem = firework.get(); // reset to firework
        bWasPressed = true;
        std::cout << "Backend: " << useInstanced << '\n';
    } else if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE) {
        bWasPressed = false;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}