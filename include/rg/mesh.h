//
// Created by matf-rg on 4.12.20..
//

#ifndef PROJECT_BASE_MESH_H
#define PROJECT_BASE_MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <rg/Error.h>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;

    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct Texture {
    unsigned id;
    std::string type; // texture_diffuse, texture_specular, texture_normal, texture_height
    std::string path;
};

class Mesh {
    unsigned VAO;
    void setupMesh() {
        unsigned VBO;
        unsigned EBO;

        glGenVertexArray(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Normal)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, TexCoords)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Tangent)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Bitangent)));

        glBindVertexArray(0);
    }

public:
    std::vector<Vertex> vertices;
    std::vector<unsigned> indices;
    std::vector<Texture> textures;

    Mesh(const std::vector<Vertex>& vs, const std::vector<unsigned>& ind, const std::vector<Texture>& tex) : vertices(vs), indices(ind), textures(tex) {
        setupMesh();
    }

    void Draw(Shader& shader) {
        unsigned diffuseNr = 1;
        unsigned specularNr = 1;
        unsigned normalNr = 1;
        unsigned heightNr = 1;

        for (unsigned i = 0; i < textures.size(); ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            std::string name = textures[i].type;
            std::string number;

            if (name == "texture_diffuse") {
                number = std::to_string(diffuseNr++); // 1
            } else if (name == "texture_specular") {
                number = std::to_string(specularNr++);
            } else if (name == "texture_normal") {
                number = std::to_string(normalNr++);
            } else if (name == "texture_height") {
                number = std::to_string(heightNr++);
            } else {
                ASSERT(false, "Unknown texture type");
            }
            name.append(number);
            shader.setInt(name, i); // texture_diffuse1
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        glBindVertexArray(VAO);
        glDrawElements(GL_TEXTURE_2D, indices.size(), GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    }
};

#endif //PROJECT_BASE_MESH_H
