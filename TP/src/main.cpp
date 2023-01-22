#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

#include <graphics.h>
#include <SceneView.h>
#include <Texture.h>
#include <Framebuffer.h>
#include <ImGuiRenderer.h>
#include <Material.h>

#include <imgui/imgui.h>


using namespace OM3D;

static float delta_time = 0.0f;
const glm::uvec2 window_size(1600, 900);


void glfw_check(bool cond) {
    if(!cond) {
        const char* err = nullptr;
        glfwGetError(&err);
        std::cerr << "GLFW error: " << err << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void update_delta_time() {
    static double time = 0.0;
    const double new_time = program_time();
    delta_time = float(new_time - time);
    time = new_time;
}

void process_inputs(GLFWwindow* window, Camera& camera) {
    static glm::dvec2 mouse_pos;

    glm::dvec2 new_mouse_pos;
    glfwGetCursorPos(window, &new_mouse_pos.x, &new_mouse_pos.y);

    {
        glm::vec3 movement = {};
        if(glfwGetKey(window, 'W') == GLFW_PRESS) {
            movement += camera.forward();
        }
        if(glfwGetKey(window, 'S') == GLFW_PRESS) {
            movement -= camera.forward();
        }
        if(glfwGetKey(window, 'D') == GLFW_PRESS) {
            movement += camera.right();
        }
        if(glfwGetKey(window, 'A') == GLFW_PRESS) {
            movement -= camera.right();
        }

        float speed = 10.0f;
        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            speed *= 10.0f;
        }

        if(movement.length() > 0.0f) {
            const glm::vec3 new_pos = camera.position() + movement * delta_time * speed;
            camera.set_view(glm::lookAt(new_pos, new_pos + camera.forward(), camera.up()));
        }
    }

    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        const glm::vec2 delta = glm::vec2(mouse_pos - new_mouse_pos) * 0.01f;
        if(delta.length() > 0.0f) {
            glm::mat4 rot = glm::rotate(glm::mat4(1.0f), delta.x, glm::vec3(0.0f, 1.0f, 0.0f));
            rot = glm::rotate(rot, delta.y, camera.right());
            camera.set_view(glm::lookAt(camera.position(), camera.position() + (glm::mat3(rot) * camera.forward()), (glm::mat3(rot) * camera.up())));
        }

    }

    mouse_pos = new_mouse_pos;
}


std::unique_ptr<Scene> create_default_scene() {
    auto scene = std::make_unique<Scene>();

    // Load default cube model
    auto result = Scene::from_gltf(std::string(data_path) + "cube.glb");
    ALWAYS_ASSERT(result.is_ok, "Unable to load default scene");
    scene = std::move(result.value);

    // Add lights
    {
        PointLight light;
        light.set_position(glm::vec3(1.0f, 2.0f, 4.0f));
        light.set_color(glm::vec3(0.0f, 10.0f, 0.0f));
        light.set_radius(100.0f);
        scene->add_object(std::move(light));
    }
    {
        PointLight light;
        light.set_position(glm::vec3(1.0f, 2.0f, -4.0f));
        light.set_color(glm::vec3(10.0f, 0.0f, 0.0f));
        light.set_radius(50.0f);
        scene->add_object(std::move(light));
    }

    return scene;
}


int main(int, char**) {
    DEBUG_ASSERT([] { std::cout << "Debug asserts enabled" << std::endl; return true; }());

    glfw_check(glfwInit());
    DEFER(glfwTerminate());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(window_size.x, window_size.y, "TP window", nullptr, nullptr);
    glfw_check(window);
    DEFER(glfwDestroyWindow(window));

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    init_graphics();

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    std::vector<std::string> files;
    for(const auto& entry : std::filesystem::directory_iterator(data_path)) {
        files.push_back(entry.path().string());
    }

    ImGuiRenderer imgui(window);

    std::unique_ptr<Scene> scene = create_default_scene();
    SceneView scene_view(scene.get());

    std::shared_ptr<Texture> color = std::make_shared<Texture>(window_size, ImageFormat::RGBA8_UNORM);
    std::shared_ptr<Texture> normal = std::make_shared<Texture>(window_size, ImageFormat::RGBA8_UNORM);
    std::shared_ptr<Texture> depth = std::make_shared<Texture>(window_size, ImageFormat::Depth32_FLOAT);
    Framebuffer gbuffer(depth.get(), std::array{color.get(), normal.get()});

    std::shared_ptr<Texture> lit = std::make_shared<Texture>(window_size, ImageFormat::RGBA16_FLOAT);
    Framebuffer main_framebuffer(depth.get(), std::array{lit.get()});

    std::shared_ptr<Texture> tonemap_color = std::make_shared<Texture>(window_size, ImageFormat::RGBA8_UNORM);
    Framebuffer tonemap_framebuffer(nullptr, std::array{tonemap_color.get()});

    auto tonemap_program = Program::from_file("tonemap.comp");

    const auto programs = std::array{
        Program::from_files("lit.frag", "screen.vert"),
        Program::from_files("lit.frag", "screen.vert", {"DEBUG_COLOR"}),
        Program::from_files("lit.frag", "screen.vert", {"DEBUG_NORMAL"}),
        Program::from_files("lit.frag", "screen.vert", {"DEBUG_LIGHT"}),
        Program::from_files("lit.frag", "screen.vert", {"DEBUG_DEPTH"}),
    };
    static bool use_tonemap = true;
    static bool debug = false;
    static int debug_mode = 1;
    Material gbuffer_material = Material();
    gbuffer_material.set_program(programs[0]);

    gbuffer_material.set_texture(0u, color);
    gbuffer_material.set_texture(1u, normal);
    gbuffer_material.set_texture(2u, depth);

    gbuffer_material.set_blend_mode(BlendMode::Alpha);
    gbuffer_material.set_depth_test_mode(DepthTestMode::None);
    gbuffer_material.set_depth_write(false);

    for(;;) {
        glfwPollEvents();
        if(glfwWindowShouldClose(window) || glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            break;
        }

        update_delta_time();

        if(const auto& io = ImGui::GetIO(); !io.WantCaptureMouse && !io.WantCaptureKeyboard) {
            process_inputs(window, scene_view.camera());
        }

        // Render in gbuffer
        {
            gbuffer.bind();
            scene_view.render();
        }

        // Compute lighting gbuffer
        {
            auto frame_data_buffer = scene->frame_data_buffer(scene_view.camera());
            frame_data_buffer->bind(BufferUsage::Uniform, 0);
            auto point_light_buffer = scene->point_light_buffer();
            point_light_buffer->bind(BufferUsage::Storage, 1);
            gbuffer_material.bind();
            main_framebuffer.bind();
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        
        // Tonemap
        {
            tonemap_program->bind();
            lit->bind(0);
            tonemap_color->bind_as_image(1, AccessType::WriteOnly);
            glDispatchCompute(align_up_to(window_size.x, 8) / 8, align_up_to(window_size.y, 8) / 8, 1);
        }

        // Blit tonemap result to screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (use_tonemap)
            tonemap_framebuffer.blit();
        else
            main_framebuffer.blit();

        // GUI
        imgui.start();
        {
            for (const auto& path : files) {
                if(ImGui::Button(path.c_str())) {
                    auto result = Scene::from_gltf(path);
                    if(!result.is_ok) {
                        std::cerr << "Unable to load scene (" << path << ")" << std::endl;
                    } else {
                        scene = std::move(result.value);
                        scene_view = SceneView(scene.get());
                    }
                }
            }
            ImGui::Checkbox("Use tonemap", &use_tonemap);
            ImGui::Checkbox("Debug shader", &debug);
            if (debug) {
                ImGui::RadioButton("Color", &debug_mode, 1);
                ImGui::RadioButton("Normal", &debug_mode, 2);
                ImGui::RadioButton("Light", &debug_mode, 3);
                ImGui::RadioButton("Depth", &debug_mode, 4);
            }
            gbuffer_material.set_program(programs[debug ? debug_mode : 0]);
        }
        imgui.finish();

        glfwSwapBuffers(window);
    }

    scene = nullptr; // destroy scene and child OpenGL objects
}
