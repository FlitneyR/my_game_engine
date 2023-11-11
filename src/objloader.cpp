#include <objloader.hpp>
#include <fstream>
#include <iostream>

namespace mge {

namespace obj {

struct FileLine {
    enum Type {
        eError,
        ePosition,
        eTexcoord,
        eNormal,
        eFace,
    } m_type;

    int componentCount;

    float mu_floats[4];
    int mu_ints[9];
};

std::istream& operator >>(std::istream& is, FileLine::Type& type) {
    std::string prefix;
    is >> prefix;

    type = FileLine::eError;
    if (prefix == "v") type = FileLine::ePosition;
    else if (prefix == "vt") type = FileLine::eTexcoord;
    else if (prefix == "vn") type = FileLine::eNormal;
    else if (prefix == "f") type = FileLine::eFace;
    
    return is;
}

std::istream& operator >>(std::istream& is, FileLine& line) {
    is >> line.m_type;
    line.componentCount = 0;

    switch (line.m_type) {
    case FileLine::ePosition: {
        for (line.componentCount = 0; line.componentCount < 3; line.componentCount++) {
            is >> line.mu_floats[line.componentCount];
            if (is.fail()) break;
        }

        if (line.componentCount != 3) line.m_type = FileLine::eError;
    }   break;
    case FileLine::eTexcoord: {
        for (line.componentCount = 0; line.componentCount < 2; line.componentCount++) {
            is >> line.mu_floats[line.componentCount];
            if (is.fail()) break;
        }

        if (line.componentCount != 2) line.m_type = FileLine::eError;
    }   break;
    case FileLine::eNormal: {
        for (line.componentCount = 0; line.componentCount < 3; line.componentCount++) {
            is >> line.mu_floats[line.componentCount];
            if (is.fail()) break;
        }

        if (line.componentCount != 3) line.m_type = FileLine::eError;
    }   break;
    case FileLine::eFace: {
        for (line.componentCount = 0; line.componentCount < 9; line.componentCount++) {
            if (is.peek() == '/') is.get();
            is >> line.mu_ints[line.componentCount];
            line.mu_ints[line.componentCount]--;
            if (is.fail()) break;
        }

        if (line.componentCount == 3) break;
        if (line.componentCount == 6) break;
        if (line.componentCount == 9) break;

        line.m_type = FileLine::eError;
    }   break;
    default: break;
    }

    return is;
}

std::ostream& operator <<(std::ostream& os, const FileLine& line) {
    switch (line.m_type) {
    case FileLine::eError:
        return os << "Non-data or malformed line";
    case FileLine::eFace:
        return os <<
            "f " <<
            line.mu_ints[0] << "/" << 
            line.mu_ints[1] << "/" << 
            line.mu_ints[2] << " " <<
            line.mu_ints[3] << "/" <<
            line.mu_ints[4] << "/" <<
            line.mu_ints[5] << " " <<
            line.mu_ints[6] << "/" <<
            line.mu_ints[7] << "/" <<
            line.mu_ints[8];
    case FileLine::eNormal:
        return os << 
            "vn " <<
            line.mu_floats[1] << " " <<
            line.mu_floats[2] << " " <<
            line.mu_floats[3];
    case FileLine::ePosition:
        return os << 
            "v " <<
            line.mu_floats[1] << " " <<
            line.mu_floats[2] << " " <<
            line.mu_floats[3];
    case FileLine::eTexcoord:
        return os << 
            "vt " <<
            line.mu_floats[1] << " " <<
            line.mu_floats[2];
    }
}

}

Mesh<ModelVertex> loadObjMesh(Engine& engine, const char* filename) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    std::vector<int> positionIndices;
    std::vector<int> normalIndices;
    std::vector<int> texcoordIndicies;

    std::vector<uint16_t> indices;
    int vertexCount = 0;

    std::ifstream file(filename);

    if (!file.is_open()) throw std::runtime_error("Failed to open file: " + std::string(filename));

    while (!file.eof()) {
        obj::FileLine line;
        file >> line;

        // std::cout << line << std::endl;

        switch (line.m_type) {
        case obj::FileLine::ePosition: {
            positions.push_back(glm::vec3(line.mu_floats[0], line.mu_floats[1], line.mu_floats[2]));
        }   break;
        case obj::FileLine::eTexcoord: {
            texcoords.push_back(glm::vec2(line.mu_floats[0], 1.f - line.mu_floats[1]));
        }   break;
        case obj::FileLine::eNormal: {
            normals.push_back(glm::vec3(line.mu_floats[0], line.mu_floats[1], line.mu_floats[2]));
        }   break;
        case obj::FileLine::eFace: {
            switch (line.componentCount) {
            case 9: {
                int a = -1, b = -1, c = -1;

                int piA = line.mu_ints[0];
                int tiA = line.mu_ints[1];
                int niA = line.mu_ints[2];
                int piB = line.mu_ints[3];
                int tiB = line.mu_ints[4];
                int niB = line.mu_ints[5];
                int piC = line.mu_ints[6];
                int tiC = line.mu_ints[7];
                int niC = line.mu_ints[8];

                for (size_t i = 0; i < vertexCount && (a < 0 || b < 0 || c < 0); i++) {
                    if (a < 0 && positionIndices[i] == piA && texcoordIndicies[i] == tiA && normalIndices[i] == niA) a = i;
                    if (b < 0 && positionIndices[i] == piB && texcoordIndicies[i] == tiB && normalIndices[i] == niB) b = i;
                    if (c < 0 && positionIndices[i] == piC && texcoordIndicies[i] == tiC && normalIndices[i] == niC) c = i;
                }

                if (a < 0) {
                    positionIndices.push_back(piA);
                    texcoordIndicies.push_back(tiA);
                    normalIndices.push_back(niA);
                    indices.push_back(vertexCount++);
                } else { indices.push_back(a); }

                if (b < 0) {
                    positionIndices.push_back(piB);
                    texcoordIndicies.push_back(tiB);
                    normalIndices.push_back(niB);
                    indices.push_back(vertexCount++);
                } else { indices.push_back(b); }

                if (c < 0) {
                    positionIndices.push_back(piC);
                    texcoordIndicies.push_back(tiC);
                    normalIndices.push_back(niC);
                    indices.push_back(vertexCount++);
                } else { indices.push_back(c); }
            }   break;
            default: {
                // std::cerr
                //     << "WARNING! Failed to parse OBJ line\t"
                //     << "INFO OBJ file faces must contain 3 vertices specifying position, normal, and texcoord" << std::endl;
            }   break;
            }
        }   break;
        default:
            while (file.get() != '\n' && !file.eof());
            break;
        }
    }

    file.close();
    // std::cout << "Reached end of file" << std::endl;

    std::vector<ModelVertex> vertices(vertexCount);

    for (int i = 0; i < vertices.size(); i++) {
        int positionIndex = positionIndices[i];
        int texcoordIndex = texcoordIndicies[i];
        int normalIndex = normalIndices[i];

        vertices[i].m_position = positions[positionIndex];
        vertices[i].m_texcoord = texcoords[texcoordIndex];
        vertices[i].m_normal = normals[normalIndex];
    }

    std::vector<glm::vec3> tangents(vertices.size()), bitangents(vertices.size());
    std::vector<float> denominators(vertices.size());

    for (int i = 0, j = 1, k = 2; k < indices.size(); i = ++k, j = ++k, k = ++k) {
        int I = indices[i], J = indices[j], K = indices[k];

        ModelVertex vi = vertices[I], vj = vertices[J], vk = vertices[K];

        glm::vec3 jDeltaPosition = vj.m_position - vi.m_position;
        glm::vec3 kDeltaPosition = vk.m_position - vi.m_position;
        
        glm::vec2 jDeltaUV = vj.m_texcoord - vi.m_texcoord;
        glm::vec2 kDeltaUV = vk.m_texcoord - vi.m_texcoord;

        /*
        
        jDeltaPosition = jDeltaUV.x * tangent + jDeltaUV.y * bitangent
        kDeltaPosition = kDeltaUV.x * tangent + kDeltaUV.y * bitangent

        therefore:

        [ jDeltaPosition.x kDeltaPosition.x ]   [ tangent.x bitangent.x ]   [ jDeltaUV.x kDeltaUV.x ]
        [ jDeltaPosition.y kDeltaPosition.y ] = [ tangent.y bitangent.y ] * [ jDeltaUV.y kDeltaUV.y ]
        [ jDeltaPosition.z kDeltaPosition.z ]   [ tangent.z bitangent.z ]

        therefore:

        [ tangent.x bitangent.x ]   [ jDeltaPosition.x kDeltaPosition.x ]   [ jDeltaUV.x kDeltaUV.x ] ^-1
        [ tangent.y bitangent.y ] = [ jDeltaPosition.y kDeltaPosition.y ] * [ jDeltaUV.y kDeltaUV.y ]
        [ tangent.z bitangent.z ]   [ jDeltaPosition.z kDeltaPosition.z ]

        */

        glm::mat2x3 m = glm::mat2x3 { jDeltaPosition, kDeltaPosition } * glm::inverse(glm::mat2 { jDeltaUV, kDeltaUV });
        glm::vec3 tangent = glm::normalize(m[0]), bitangent = glm::normalize(m[1]);

        // catch a potential error when all the vertices are in-line
        if (tangent != tangent || bitangent != bitangent) continue;

        tangents[I] += tangent; bitangents[I] += bitangent; denominators[I] += 1.0f;
        tangents[J] += tangent; bitangents[J] += bitangent; denominators[J] += 1.0f;
        tangents[K] += tangent; bitangents[K] += bitangent; denominators[K] += 1.0f;
    }

    for (int index = 0; index < vertices.size(); index++) {
        vertices[index].m_tangent = glm::normalize(tangents[index] / std::max(denominators[index], 1.f));
        vertices[index].m_bitangent = glm::normalize(bitangents[index] / std::max(denominators[index], 1.f));
    }

    return Mesh<ModelVertex>(engine, vertices, indices);
}

}
