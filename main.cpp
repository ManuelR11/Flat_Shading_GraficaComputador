#include <SDL.h>
#include <vector>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <filesystem>
#include "fragment.h"
#include "uniform.h"
#include <array>
#include <fstream>
#include "shaders.h"
#include "triangle.h"
#include "color.h"
#include "print.h"



const int SCREEN_WIDTH = 690;
const int SCREEN_HEIGHT = 500;

std::array<std::array<float, SCREEN_WIDTH>, SCREEN_HEIGHT> depthBuffer;

SDL_Renderer* renderer;

Uniform uniforms;

struct Face {
    std::vector<std::array<int, 3>> vertexIndices;
};

std::string getParentDirectory(const std::string& path) {
    std::filesystem::path filePath(path);
    return filePath.parent_path().string();
}

std::string getCurrentPath() {
    return std::filesystem::current_path().string();
}



std::vector<Face> objectFaces;
std::vector<glm::vec3> objectVertices;


// Declare a global objectColor of type Color
Color objectColor = {255, 0, 0}; // Initially set to red

// Declare a global clearColor of type Color
Color backgroundColor = {0, 0, 0}; // Initially set to black

// Function to clear the framebuffer with the backgroundColor
void clear() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    for (auto &row : depthBuffer) {
        std::fill(row.begin(), row.end(), 99999.0f);
    }
}

// Function to set a specific pixel in the framebuffer to the objectColor
void drawPixel(Fragment fragment) {
    if (fragment.position.y < SCREEN_HEIGHT && fragment.position.x < SCREEN_WIDTH &&
        fragment.position.y > 0 && fragment.position.x > 0 &&
        fragment.position.z < depthBuffer[fragment.position.y][fragment.position.x]) {
        SDL_SetRenderDrawColor(renderer, fragment.color.r, fragment.color.g, fragment.color.b, fragment.color.a);
        SDL_RenderDrawPoint(renderer, fragment.position.x, fragment.position.y);
        depthBuffer[fragment.position.y][fragment.position.x] = fragment.position.z;
    }
}

std::vector<std::vector<Vertex>> assemblePrimitives(
        const std::vector<Vertex>& transformedVertices
) {
    std::vector<std::vector<Vertex>> groupedVertices;

    for (int i = 0; i < transformedVertices.size(); i += 3) {
        std::vector<Vertex> vertexGroup;
        vertexGroup.push_back(transformedVertices[i]);
        vertexGroup.push_back(transformedVertices[i+1]);
        vertexGroup.push_back(transformedVertices[i+2]);

        groupedVertices.push_back(vertexGroup);
    }

    return groupedVertices;
}

void renderObject(std::vector<glm::vec3> VBO) {
    // 1. Vertex Shader
    // vertex -> transformedVertices

    std::vector<Vertex> transformedVertices;

    for (int i = 0; i < VBO.size(); i++) {
        glm::vec3 v = VBO[i];

        Vertex vertex = {v, Color(255, 255, 255)};
        Vertex transformedVertex = vertexShader(vertex, uniforms);
        transformedVertices.push_back(transformedVertex);
    }
    std::vector<std::vector<Vertex>> triangles = assemblePrimitives(transformedVertices);

    std::vector<Fragment> fragments;
    for (const std::vector<Vertex>& triangleVertices : triangles) {
        std::vector<Fragment> rasterizedTriangle = triangle(
                triangleVertices[0],
                triangleVertices[1],
                triangleVertices[2]
        );

        fragments.insert(
                fragments.end(),
                rasterizedTriangle.begin(),
                rasterizedTriangle.end()
        );
    }

    for (Fragment fragment : fragments) {
        drawPixel(fragmentShader(fragment));
    }
}


glm::mat4 createViewMatrix() {
    return glm::lookAt(
            glm::vec3(0, 0, -5),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
    );
}

glm::mat4 createProjectionMatrix() {
    float fovInDegrees = 45.0f;
    float aspectRatio = SCREEN_WIDTH / SCREEN_HEIGHT;
    float nearClip = 0.1f;
    float farClip = 100.0f;

    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}



float rotationAngle = 0.0f; // Initialize the angle to 0

glm::mat4 createObjectModelMatrix() {
    rotationAngle += 0.01f; // Adjust the rotation speed
    glm::mat4 rotationY = glm::rotate(glm::mat4(1), rotationAngle, glm::vec3(0, 1, 0)); // Apply rotation in Y
    glm::mat4 rotationX = glm::rotate(glm::mat4(1), glm::radians(180.0f), glm::vec3(1, 0, 0)); // Additional 180-degree rotation in X to correct orientation
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(0.08f)); // Adjust the object scale

    return rotationX * rotationY * scale;
}

glm::mat4 createViewportMatrix() {
    glm::mat4 viewport = glm::mat4(1.0f);

    // Scale
    viewport = glm::scale(viewport, glm::vec3(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, 0.5f));

    // Translate
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));

    return viewport;
}


std::vector<glm::vec3> prepareObjectVertexArray(const std::vector<glm::vec3>& vertices, const std::vector<Face>& faces) {
    std::vector<glm::vec3> vertexArray;

    for (const auto& face : faces) {
        for (const auto& vertexIndices : face.vertexIndices) {
            glm::vec3 vertexPosition = vertices[vertexIndices[0]];
            vertexArray.push_back(vertexPosition);
        }
    }

    return vertexArray;
}


bool loadObject(const std::string& path, std::vector<glm::vec3>& outVertices, std::vector<Face>& outFaces) {
    outVertices.clear();
    outFaces.clear();

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            outVertices.push_back(vertex);
        } else if (type == "f") {
            std::string lineHeader;
            Face face;
            while (iss >> lineHeader) {
                std::istringstream tokenstream(lineHeader);
                std::string token;
                std::array<int, 3> vertexIndices;

                // Read all three values separated by '/'
                for (int i = 0; i < 3; ++i) {
                    std::getline(tokenstream, token, '/');
                    vertexIndices[i] = std::stoi(token) - 1;
                }

                face.vertexIndices.push_back(vertexIndices);
            }
            outFaces.push_back(face);
        }
    }

    file.close();
    return true;
}

void saveBMP(const std::string& filename) {
    int width = SCREEN_WIDTH;
    int height = SCREEN_HEIGHT;

    float zMin = std::numeric_limits<float>::max();
    float zMax = std::numeric_limits<float>::lowest();

    for (const auto& row : depthBuffer) {
        for (const auto& val : row) {
            if (val != 99999.0f) { // Ignore values that haven't been updated
                zMin = std::min(zMin, val);
                zMax = std::max(zMax, val);
            }
        }
    }

    // Ensure zMin and zMax are different
    if (zMin == zMax) {
        std::cerr << "zMin and zMax are equal. This will produce a black or white image.\n";
        return;
    }

    std::cout << "zMin: " << zMin << ", zMax: " << zMax << "\n";

    // Open the file in binary mode
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return;
    }

    // Write the BMP file header
    uint32_t fileSize = 54 + 3 * width * height;
    uint32_t dataOffset = 54;
    uint32_t imageSize = 3 * width * height;
    uint32_t biPlanes = 1;
    uint32_t biBitCount = 24;

    uint8_t header[54] = {'B', 'M',
                          static_cast<uint8_t>(fileSize & 0xFF), static_cast<uint8_t>((fileSize >> 8) & 0xFF), static_cast<uint8_t>((fileSize >> 16) & 0xFF), static_cast<uint8_t>((fileSize >> 24) & 0xFF),
                          0, 0, 0, 0,
                          static_cast<uint8_t>(dataOffset & 0xFF), static_cast<uint8_t>((dataOffset >> 8) & 0xFF), static_cast<uint8_t>((dataOffset >> 16) & 0xFF), static_cast<uint8_t>((dataOffset >> 24) & 0xFF),
                          40, 0, 0, 0,
                          static_cast<uint8_t>(width & 0xFF), static_cast<uint8_t>((width >> 8) & 0xFF), static_cast<uint8_t>((width >> 16) & 0xFF), static_cast<uint8_t>((width >> 24) & 0xFF),
                          static_cast<uint8_t>(height & 0xFF), static_cast<uint8_t>((height >> 8) & 0xFF), static_cast<uint8_t>((height >> 16) & 0xFF), static_cast<uint8_t>((height >> 24) & 0xFF),
                          static_cast<uint8_t>(biPlanes & 0xFF), static_cast<uint8_t>((biPlanes >> 8) & 0xFF),
                          static_cast<uint8_t>(biBitCount & 0xFF), static_cast<uint8_t>((biBitCount >> 8) & 0xFF)};

    file.write(reinterpret_cast<char*>(header), sizeof(header));

    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            float normalized = (depthBuffer[y][x] - zMin) / (zMax - zMin);
            uint8_t color = static_cast<uint8_t>(normalized * 255);
            file.write(reinterpret_cast<char*>(&color), 1);
            file.write(reinterpret_cast<char*>(&color), 1);
            file.write(reinterpret_cast<char*>(&color), 1);
        }
    }

    file.close();
}

int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window* window = SDL_CreateWindow("Space_Object", 200, 200, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    std::string currentPath = getCurrentPath();
    std::string objFileName = "C://Users//rodas//Desktop//CODING11//Repoitorios_GIT//Flat_Shading_GraficaComputador//nave.obj"; // Replace with your OBJ file path

    loadObject(objFileName, objectVertices, objectFaces);

    std::vector<glm::vec3> objectVertexArray = prepareObjectVertexArray(objectVertices, objectFaces);

    renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED
    );

    bool isRunning = true;
    SDL_Event event;

    std::vector<glm::vec3> VBO = {
            {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
            {-0.87f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f},
            {0.87f,  -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f},

            {0.0f, 1.0f,    -1.0f}, {1.0f, 1.0f, 0.0f},
            {-0.87f, -0.5f, -1.0f}, {0.0f, 1.0f, 1.0f},
            {0.87f,  -0.5f, -1.0f}, {1.0f, 0.0f, 1.0f}
    };

    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isRunning = false;
            }
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        break;
                    case SDLK_DOWN:
                        break;
                    case SDLK_LEFT:
                        break;
                    case SDLK_RIGHT:
                        break;
                    case SDLK_s:
                        break;
                    case SDLK_a:
                        break;
                }
            }
        }

        uniforms.model = createObjectModelMatrix();
        uniforms.view = createViewMatrix();
        uniforms.projection = createProjectionMatrix();
        uniforms.viewport = createViewportMatrix();

        clear();

        // Call our renderObject function
        renderObject(objectVertexArray);

        SDL_RenderPresent(renderer);

        // Delay to limit the frame rate
        SDL_Delay(1000 / 60);
        saveBMP("space_object.bmp");
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
