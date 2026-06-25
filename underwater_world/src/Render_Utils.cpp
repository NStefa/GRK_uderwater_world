#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Render_Utils.h"
#include <algorithm>
#include "glew.h"
#include "freeglut.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


// Laduje dane mesha (pozycje, normalne, UV, tangenty, bitangenty) z Assimp do buforow OpenGL na karcie graficznej
void Core::RenderContext::initFromAssimpMesh(aiMesh* mesh) {
    // zerowanie identyfikatorow buforow przed inicjalizacja
    vertexArray = 0;
    vertexBuffer = 0;
    vertexIndexBuffer = 0;

    std::vector<float> textureCoord;
    std::vector<unsigned int> indices;

    // Assimp przechowuje UV jako vec3, przepisujemy tylko x i y bo OpenGL potrzebuje vec2
    // jesli model nie ma UV, wstawiamy zera
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        if (mesh->mTextureCoords[0] != nullptr) {
            textureCoord.push_back(mesh->mTextureCoords[0][i].x);
            textureCoord.push_back(mesh->mTextureCoords[0][i].y);
        }
        else {
            textureCoord.push_back(0.0f);
            textureCoord.push_back(0.0f);
        }
    }
    if (mesh->mTextureCoords[0] == nullptr) {
        std::cout << "no uv coords\n";
    }

    // indeksy wierzcholkow tworzacych kazdy trojkat
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // rozmiary w bajtach dla kazdego typu danych
    // kazdy wierzcholek ma 3 floaty (x,y,z) dla pozycji, normalnych, tangentow i bitangentow
    // oraz 2 floaty (u,v) dla wspolrzednych tekstury
    unsigned int vertexDataBufferSize = sizeof(float) * mesh->mNumVertices * 3;
    unsigned int vertexNormalBufferSize = sizeof(float) * mesh->mNumVertices * 3;
    unsigned int vertexTexBufferSize = sizeof(float) * mesh->mNumVertices * 2;
    unsigned int vertexTangentBufferSize = sizeof(float) * mesh->mNumVertices * 3;
    unsigned int vertexBiTangentBufferSize = sizeof(float) * mesh->mNumVertices * 3;

    // rozmiar bufora indeksow trojkatow
    unsigned int vertexElementBufferSize = sizeof(unsigned int) * indices.size();
    size = indices.size();

    // VAO - organizacja danych wierzcholkow
    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    // bufor indeksow i wyslanie do niego danych
    glGenBuffers(1, &vertexIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexElementBufferSize, &indices[0], GL_STATIC_DRAW);

    // jeden duzy bufor na wszystkie dane wierzcholkow
    // rezerwacja pamieci na karcie graficznej na pozycje + normalne + UV + tangenty + bitangenty
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glBufferData(GL_ARRAY_BUFFER, vertexDataBufferSize + vertexNormalBufferSize + vertexTexBufferSize + vertexTangentBufferSize + vertexBiTangentBufferSize, NULL, GL_STATIC_DRAW);

    // w pamieci dane leza kolejno: [pozycje | normalne | UV | tangenty | bitangenty]
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexDataBufferSize, mesh->mVertices);
    glBufferSubData(GL_ARRAY_BUFFER, vertexDataBufferSize, vertexNormalBufferSize, mesh->mNormals);
    glBufferSubData(GL_ARRAY_BUFFER, vertexDataBufferSize + vertexNormalBufferSize, vertexTexBufferSize, &textureCoord[0]);
    glBufferSubData(GL_ARRAY_BUFFER, vertexDataBufferSize + vertexNormalBufferSize + vertexTexBufferSize, vertexTangentBufferSize, mesh->mTangents);
    glBufferSubData(GL_ARRAY_BUFFER, vertexDataBufferSize + vertexNormalBufferSize + vertexTexBufferSize + vertexTangentBufferSize, vertexBiTangentBufferSize, mesh->mBitangents);

    // gdzie w buforze szukac kazdego typu danych
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)(0));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)(vertexDataBufferSize));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)(vertexNormalBufferSize + vertexDataBufferSize));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)(vertexDataBufferSize + vertexNormalBufferSize + vertexTexBufferSize));
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)(vertexDataBufferSize + vertexNormalBufferSize + vertexTexBufferSize + vertexTangentBufferSize));

}

// rysuje trojkaty z tablicy wierzcholkow (bez indeksow)
void Core::DrawVertexArray(const float * vertexArray, int numVertices, int elementSize )
{
	glVertexAttribPointer(0, elementSize, GL_FLOAT, false, 0, vertexArray);
	glEnableVertexAttribArray(0);

	glDrawArrays(GL_TRIANGLES, 0, numVertices);
}

// rysuje trojkaty z tablicy wierzcholkow uzywajac indeksow
void Core::DrawVertexArrayIndexed( const float * vertexArray, const int * indexArray, int numIndexes, int elementSize )
{
	glVertexAttribPointer(0, elementSize, GL_FLOAT, false, 0, vertexArray);
	glEnableVertexAttribArray(0);

	glDrawElements(GL_TRIANGLES, numIndexes, GL_UNSIGNED_INT, indexArray);
}

// rysuje trojkaty z wielu atrybutow wierzcholkow opisanych struktura VertexData
void Core::DrawVertexArray( const VertexData & data )
{
	int numAttribs = std::min(VertexData::MAX_ATTRIBS, data.NumActiveAttribs);
	for(int i = 0; i < numAttribs; i++)
	{
		glVertexAttribPointer(i, data.Attribs[i].Size, GL_FLOAT, false, 0, data.Attribs[i].Pointer);
		glEnableVertexAttribArray(i);
	}
	glDrawArrays(GL_TRIANGLES, 0, data.NumVertices);
}

// aktywuje VAO, rysuje trojkaty przez indeksy, odlacza VAO
void Core::DrawContext(Core::RenderContext& context)
{

	glBindVertexArray(context.vertexArray);
	glDrawElements(
		GL_TRIANGLES, // mode
		context.size, // count
		GL_UNSIGNED_INT, // type
		(void*)0 // element array buffer offset
	);
	glBindVertexArray(0);
}

// wczytuje plik obrazu i tworzy teksture OpenGL z mipmappami i powtarzaniem (GL_REPEAT)
GLuint Core::loadTexture(const char* path)
{
    // rezerwacja slotu na teksture w OpenGL
    GLuint textureID;
    glGenTextures(1, &textureID);

    // wczytanie pliku obrazu z dysku - wypelnia width, height i nrChannels
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data)
    {
        // wykrycie formatu na podstawie liczby kanalow (1=skala szarosci, 3=RGB, 4=RGBA)
        GLenum format;
        if (nrChannels == 1)      format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        // aktywacja slotu i wyslanie danych obrazu do karty graficznej
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        // generowanie mipmap - pomniejszonych wersji tekstury dla oddalonych obiektow
        glGenerateMipmap(GL_TEXTURE_2D);

        // tekstura powtarza sie gdy UV wychodzi poza zakres 0-1
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // filtrowanie - liniowe z mipmapami przy pomniejszaniu, liniowe przy powiekszaniu
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        std::cout << "Failed to load texture: " << path << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

// wczytuje 6 obrazow i tworzy teksture cubemapy (skybox) dla OpenGL
GLuint Core::loadCubemap(std::vector<std::string> faces)
{
    // rezerwacja slotu na teksture cubemapy i jej aktywacja
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    // wczytanie kazdej z 6 scian cubemapy osobno
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            // wyslanie danych sciany do odpowiedniego slotu cubemapy
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Failed to load cubemap face: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }

    // filtrowanie liniowe przy pomniejszaniu i powiekszaniu
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // zapobiega widocznym szwom miedzy scianami
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
