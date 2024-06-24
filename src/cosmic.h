
#include "pch.h"



#include "shader.h"
#include "camera.h"
#include "cModel.h"
#include "assetManager.h"



bool useBatchRendering = false;


glm::vec3 objectColor = glm::vec3(1.0f);
glm::vec3 lightColor = glm::vec3(1.0f);


// Create camera 
Camera camera(glm::vec3(0.0f, 0.5f, 3.0f));
bool flashlight = false;

// Material
bool useNormalTexture = false;

// Function prototypes
void gui();
void processInput(float deltaTime);

//Screen dimension constants
const int SCR_WIDTH = 2560;
const int SCR_HEIGHT = 1440;

bool stopRendering = false;
bool showUI = false;


// DeltaTime
float deltaTime = 0.0f;
float lastFrame = 0.0f;

GLuint VAO, VBO, EBO;

struct Timer
{
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    std::chrono::duration<float> duration;
    std::string m_title = "Timer";
    Timer()
    {
        startTimer();
    }
    void setTitle(const char* title)
    {
        m_title = title;
    }
    void startTimer()
    {
        start = std::chrono::high_resolution_clock::now();
    }
    void stopTimer()
    {
        end = std::chrono::high_resolution_clock::now();
        duration = end - start;

        float ms = duration.count() * 1000.0f;

        std::cout << m_title << " took: " << ms << "ms" << std::endl;
    }
};

void processInput(float deltaTime)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_g)
        {
            showUI = !showUI;
        }
        if (event.type == SDL_QUIT || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE))
        {
            stopRendering = true;
        }
        if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_f)
        {
            flashlight = !flashlight;
        }
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_1)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        }
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_2)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_3)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        if (event.type == SDL_MOUSEMOTION && !showUI)
        {
            camera.ProcessMouseMovement(event.motion.xrel, event.motion.yrel);


            // Update the view matrix in your render loop
            //view = camera.GetViewMatrix();

        }
        if (event.type == SDL_MOUSEWHEEL)
        {
            if (event.wheel.y != 0)
            {
                camera.ProcessMouseScroll(event.wheel.y);
            }
        }

    }

    // check keyboard state (which keys are still pressed)
    const uint8_t* state = SDL_GetKeyboardState(nullptr);

    if (state[SDL_SCANCODE_W])
    {
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (state[SDL_SCANCODE_S])
    {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (state[SDL_SCANCODE_A])
    {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (state[SDL_SCANCODE_D])
    {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
    if (state[SDL_SCANCODE_E])
    {
        camera.ProcessKeyboard(UP, deltaTime);
    }
    if (state[SDL_SCANCODE_Q])
    {
        camera.ProcessKeyboard(DOWN, deltaTime);
    }
    if (state[SDL_SCANCODE_LSHIFT])
    {
        camera.MovementSpeed = 1.0f;
    }
    else
        camera.MovementSpeed = 5.0f;



}


void gui()
{
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Camera Position: X: %.3f  Y: %.3f  Z: %.3f", camera.Position.x, camera.Position.y, camera.Position.z);
    ImGui::Checkbox("Use normal texture: ", &useNormalTexture);
    ImGui::Checkbox("Use batch rendering", &useBatchRendering);

    if (ImGui::ColorEdit3("Edit lightColor", (float*)&lightColor));

    if (ImGui::ColorEdit3("Edit objectColor", (float*)&objectColor));

    ImGui::End();


}

int run()
{
    //windowContext windowC = initializer::init_window_SDL("CosmicManor", SCR_WIDTH, SCR_HEIGHT);
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "%s: %s\n", "Couldn't initialize SDL", SDL_GetError());
        return -1;
    }

    // Windows DPI awareness
    SetProcessDPIAware();

    // Setup MSAA
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1);

    // Set OpenGL version to 3.3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);


    // Also request a depth buffer
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Create the window
    SDL_Window* window = SDL_CreateWindow("Cosmic Manor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCR_WIDTH, SCR_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    if (window == nullptr)
    {
        fprintf(stderr, "%s: %s\n", "Couldn't set video mode", SDL_GetError());
        return -1;
    }

    static SDL_GLContext maincontext = SDL_GL_CreateContext(window);
    if (maincontext == nullptr)
    {
        fprintf(stderr, "%s: %s\n", "Failed to create OpenGL context", SDL_GetError());
        return -1;
    }

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    // MSAA ENABLE
    glEnable(GL_MULTISAMPLE);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);

    // Flip textures vertically so they don't end up upside-down.
    stbi_set_flip_vertically_on_load(false);

    // 1 for v-sync
    SDL_GL_SetSwapInterval(0);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    //Enable face culling
    glEnable(GL_CULL_FACE);

    // IMGUI
    float dpi, defaultDpi, scalingFactor;
    if (SDL_GetDisplayDPI(0, &dpi, nullptr, nullptr) == 0) {
        defaultDpi = 96.0f;
        scalingFactor = dpi / defaultDpi;
    }
    else {
        scalingFactor = 1.0f;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = scalingFactor;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, maincontext);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Create a shader
    Shader shader("shaders/shader.vert", "shaders/shader.frag");
    Shader postShader("shaders/postShader.vert", "shaders/postShader.frag");
    Shader simpleShader("shaders/simpleShader.vert", "shaders/simpleShader.frag");

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /*
    std::unordered_map<std::string, fs::path> files;
    {
        Timer timer;
        timer.setTitle("Look up all files");
        timer.startTimer();
        files = findModelFiles("../spaceStationProjectDirectory/TODO");
    }
    fs::path file;
    auto it = files.find("SponzaDDS.gltf");
    if (it != files.end()) 
    {
        file = it->second;  // Access the value using iterator
    }
    else 
    {
        std::cout << "Key not found" << std::endl;
    }
    std::string filePath = file.generic_string();
    */
    std::string filePath = "../spaceStationProjectDirectory/TODO/";
    std::string file = filePath + "Box/glTF-Draco/Box.gltf";

    Timer timer;
    timer.setTitle("Load GLTF file");
    timer.startTimer();
    cModel myModel(file.c_str());
    timer.stopTimer();

    timer.setTitle("Upload data to gpu");
    timer.startTimer();
    myModel.uploadToGpu(shader);
    timer.stopTimer();

    myModel.batchTest();


    SDL_SetRelativeMouseMode(SDL_TRUE);
    glEnable(GL_DEPTH_TEST);

    shader.use();
    // directional Light
    static glm::vec3 lightDir(-2.0f, -1.0f, 3.0f);
    static glm::vec3 lightColor(0.5f, 0.5f, 0.5f);
    shader.setVec3("dirLight.direction", lightDir);
    shader.setVec3("dirLight.ambient", lightColor);
    shader.setVec3("dirLight.diffuse", lightColor);
    shader.setVec3("dirLight.specular", lightColor);
    shader.setBool("dirLight.enabled", true);

    while (!stopRendering)
    {
        float currentFrame = SDL_GetTicks64() / 1000.0f;
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        //Input
        processInput(deltaTime);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 10000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
 
        myModel.Draw(shader, useBatchRendering);




        if (showUI)
        {
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            gui();
            SDL_SetRelativeMouseMode(SDL_FALSE);
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        }
        else
            SDL_SetRelativeMouseMode(SDL_TRUE);


        
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        // Swap buffers
        SDL_GL_SwapWindow(window);
    }


    // Cleanup imgui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    //Cleanup sdl
    SDL_GL_DeleteContext(maincontext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}