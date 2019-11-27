#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <glm.hpp>
#include <gtc\matrix_transform.hpp>
#include <gtc\type_ptr.hpp>
#include <iostream>
#include <SOIL.h>
#include <math.h>
#include "Shader_s.h"
#include <map>
#include <string>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <Windows.h>
#include <mmsystem.h>
#define M_PI 3.14

#define WIDTH 400	
#define HEIGHT 600

using namespace glm;
float tambah, rotat, rotate1, tambah2;
GLuint VAO, VBO, EBO, texture;
float gamestatus = 0;


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Window dimensions
float kelipatan3 = -15, geserup3;
float kelipatan = -10, geserup;
float kelipatan2 = -5, geserup2;
int scores = 0;
/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
	GLuint TextureID;   // ID handle of the glyph texture
	glm::ivec2 Size;    // Size of glyph
	glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
	GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;

void RenderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
	GLuint VAO, VBO;
	// Compile and setup the shader
	Shader shader("shaders/text.vs", "shaders/text.frag");
	glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(480), 0.0f, static_cast<GLfloat>(480));
	shader.use();
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	// FreeType*/
	FT_Library ft;
	// All functions return a value different than 0 whenever an error occurred
	if (FT_Init_FreeType(&ft))
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

	// Load font as face
	FT_Face face;
	if (FT_New_Face(ft, "fonts/JANBRADY.TTF", 0, &face))
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

	// Set size to load glyphs as
	FT_Set_Pixel_Sizes(face, 0, 48);

	// Disable byte-alignment restriction
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Load first 128 characters of ASCII set
	for (GLubyte c = 0; c < 128; c++)
	{
		// Load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}
		// Generate texture
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
			);
		// Set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Now store character for later use
		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		Characters.insert(std::pair<GLchar, Character>(c, character));
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	// Destroy FreeType once we're finished
	FT_Done_Face(face);
	FT_Done_FreeType(ft);


	// Configure VAO/VBO for texture quads
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Activate corresponding render state	
	shader.use();
	glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(VAO);

	// Iterate through all characters
	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		Character ch = Characters[*c];

		GLfloat xpos = x + ch.Bearing.x * scale;
		GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		GLfloat w = ch.Size.x * scale;
		GLfloat h = ch.Size.y * scale;
		// Update VBO for each character
		GLfloat vertices[6][4] = {
			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos,     ypos,       0.0, 1.0 },
			{ xpos + w, ypos,       1.0, 1.0 },

			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos + w, ypos,       1.0, 1.0 },
			{ xpos + w, ypos + h,   1.0, 0.0 }
		};
		// Render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		// Update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// Render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void gameover() {
	glDepthFunc(GL_LESS);


	// Set OpenGL options
	//glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	RenderText("GAMEOVER", 60.0f, 200.0f, 1.5f, glm::vec3(1.0, 0.0f, 0.0f));
}



void press() {
	glDepthFunc(GL_LESS);


	// Set OpenGL options
	//glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	RenderText("Press 'Enter' to Contunue", 80.0f, 160.0f, 0.5f, glm::vec3(1.0, 1.0f, 1.0f));
}

void score() {
	glDepthFunc(GL_LESS);


	// Set OpenGL options
	//glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	std::stringstream sekor;
	sekor << scores;
	std::string str1 = sekor.str();

	RenderText("score : " + sekor.str(), 20.0f, 450.0f, 0.5f, glm::vec3(1.0, 1.0f, 1.0f));

}

void bird()
{

	Shader ourShader("1/shaders/text.vs", "1/shaders/text.frag");
	// Set up vertex data (and buffer(s)) and attribute pointers
	GLfloat vertices[] = {
		// Positions          // Colors           // Texture Coords
		0.1f, 0.1f, 0.0f, 1.0f, 0.0f, 1.0f,  // Top Right
		0.1f, -0.1f, 0.0f, 1.0f, 0.0f, 1.0f,  // Bottom Right
		-0.1f, -0.1f, 0.0f, 1.0f, 0.0f, 1.0f,
		-0.1f, 0.1f, 0.0f, 1.0f, 0.0f, 1.0f,

		0.1f, 0.1f, 0.0f, 1.0f, 1.0f, 0.0f,  // Top Right
		0.1f, 0.1f, -0.2f, 1.0f, 1.0f, 0.0f,  // Bottom Right
		-0.1f, 0.1f, -0.2f, 1.0f, 1.0f, 0.0f,
		-0.1f, 0.1f, 0.0f, 1.0f, 1.0f, 0.0f,

		0.1f, -0.1f, 0.0f, 1.0f, 1.0f, 0.0f,  // Top Right
		0.1f, -0.1f, -0.2f, 1.0f, 1.0f, 0.0f,  // Bottom Right
		-0.1f, -0.1f, -0.2f, 1.0f, 1.0f, 0.0f,
		-0.1f, -0.1f, 0.0f, 1.0f, 1.0f, 0.0f,


	};
	glEnable(GL_DEPTH_TEST);
	GLuint VBO, VAO;

	glGenBuffers(1, &VBO);
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	ourShader.use();


	//create transformations
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::translate(view, glm::vec3(0.0f, tambah2, -1.0f));
	projection = glm::perspective(50.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
	// Get their uniform location
	GLint modelLoc = glGetUniformLocation(ourShader.ID, "model");
	GLint viewLoc = glGetUniformLocation(ourShader.ID, "view");
	GLint projLoc = glGetUniformLocation(ourShader.ID, "projection");
	// Pass them to the shaders
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	// Note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	// Draw container
	glBindVertexArray(VAO);
	glDrawArrays(GL_QUADS, 0, 12);
	glBindVertexArray(0);
	glDisable(GL_DEPTH_TEST);
}

void obstac3()
{

	Shader ourShader("1/shaders/text.vs", "1/shaders/text.frag");
	// Set up vertex data (and buffer(s)) and attribute pointers
	GLfloat vertices[] = {
		// Positions          // Colors           // Texture Coords
		// Positions          // Colors           // Texture Coords
		0.3f, 20.0f, 0.0f, 0.27f, 0.46f, 0.1f,  // Top Right
		0.3f, 0.5f, 0.0f, 0.27f, 0.46f, 0.1f, // Bottom Right
		-0.3f, 0.5f, 0.0f,0.27f, 0.46f, 0.1f,
		-0.3f, 20.0f, 0.0f,0.27f, 0.46f, 0.1f,


		0.3f, 0.5f, -0.6f, 0.12f, 0.21f, 0.007f,  // Top Right
		0.3f, 0.5f, 0.0f, 0.12f, 0.21f, 0.007f,  // Bottom Right
		-0.3f, 0.5f, 0.0f, 0.12f, 0.21f, 0.007f,
		-0.3f, 0.5f, -0.6f, 0.12f, 0.21f, 0.007f,


		0.3f, -2.0f, 0.0f, 0.27f, 0.46f, 0.1f,  // Top Right
		0.3f, -0.5f, 0.0f, 0.27f, 0.46f, 0.1f,  // Bottom Right
		-0.3f, -0.5f, 0.0f, 0.27f, 0.46f, 0.1f,
		-0.3f, -2.0f, 0.0f, 0.27f, 0.46f, 0.1f,

		0.3f, -0.5f, -0.6f, 0.12f, 0.21f, 0.007f,  // Top Right
		0.3f, -0.5f, 0.0f, 0.12f, 0.21f, 0.007f,  // Bottom Right
		-0.3f, -0.5f, 0.0f, 0.12f, 0.21f, 0.007f,
		-0.3f, -0.5f, -0.6f, 0.12f, 0.21f, 0.007f,

	};
	glEnable(GL_DEPTH_TEST);
	GLuint VBO, VAO;

	glGenBuffers(1, &VBO);
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	ourShader.use();


	//create transformations
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::translate(view, glm::vec3(0.0f, geserup3 + tambah, kelipatan3 += 0.1));
	projection = glm::perspective(70.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
	// Get their uniform location
	GLint modelLoc = glGetUniformLocation(ourShader.ID, "model");
	GLint viewLoc = glGetUniformLocation(ourShader.ID, "view");
	GLint projLoc = glGetUniformLocation(ourShader.ID, "projection");
	// Pass them to the shaders
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	// Note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	// Draw container
	glBindVertexArray(VAO);
	glDrawArrays(GL_QUADS, 0, 16);
	glBindVertexArray(0);
	glDisable(GL_DEPTH_TEST);
}

void obstac2()
{

	Shader ourShader("1/shaders/text.vs", "1/shaders/text.frag");
	// Set up vertex data (and buffer(s)) and attribute pointers
	GLfloat vertices[] = {
		// Positions          // Colors           // Texture Coords
		0.3f, 20.0f, 0.0f, 0.27f, 0.46f, 0.1f,  // Top Right
		0.3f, 0.5f, 0.0f, 0.27f, 0.46f, 0.1f, // Bottom Right
		-0.3f, 0.5f, 0.0f,0.27f, 0.46f, 0.1f,
		-0.3f, 20.0f, 0.0f,0.27f, 0.46f, 0.1f,


		0.3f, 0.5f, -0.6f, 0.12f, 0.21f, 0.007f,  // Top Right
		0.3f, 0.5f, 0.0f, 0.12f, 0.21f, 0.007f,  // Bottom Right
		-0.3f, 0.5f, 0.0f, 0.12f, 0.21f, 0.007f,
		-0.3f, 0.5f, -0.6f, 0.12f, 0.21f, 0.007f,


		0.3f, -2.0f, 0.0f, 0.27f, 0.46f, 0.1f,  // Top Right
		0.3f, -0.5f, 0.0f, 0.27f, 0.46f, 0.1f,  // Bottom Right
		-0.3f, -0.5f, 0.0f, 0.27f, 0.46f, 0.1f,
		-0.3f, -2.0f, 0.0f, 0.27f, 0.46f, 0.1f,

		0.3f, -0.5f, -0.6f, 0.12f, 0.21f, 0.007f,  // Top Right
		0.3f, -0.5f, 0.0f, 0.12f, 0.21f, 0.007f,  // Bottom Right
		-0.3f, -0.5f, 0.0f, 0.12f, 0.21f, 0.007f,
		-0.3f, -0.5f, -0.6f, 0.12f, 0.21f, 0.007f,

	};
	glEnable(GL_DEPTH_TEST);
	GLuint VBO, VAO;

	glGenBuffers(1, &VBO);
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	ourShader.use();

	//create transformations
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::translate(view, glm::vec3(0.0f, geserup + tambah, kelipatan += 0.1));
	projection = glm::perspective(70.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
	// Get their uniform location
	GLint modelLoc = glGetUniformLocation(ourShader.ID, "model");
	GLint viewLoc = glGetUniformLocation(ourShader.ID, "view");
	GLint projLoc = glGetUniformLocation(ourShader.ID, "projection");
	// Pass them to the shaders
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	// Note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	// Draw container
	glBindVertexArray(VAO);
	glDrawArrays(GL_QUADS, 0, 16);
	glBindVertexArray(0);
	glDisable(GL_DEPTH_TEST);
}

void obstac()
{

	Shader ourShader("1/shaders/text.vs", "1/shaders/text.frag");
	// Set up vertex data (and buffer(s)) and attribute pointers
	GLfloat vertices[] = {
		// Positions          // Colors           // Texture Coords
		0.3f, 20.0f, 0.0f, 0.27f, 0.46f, 0.1f,  // Top Right
		0.3f, 0.5f, 0.0f, 0.27f, 0.46f, 0.1f, // Bottom Right
		-0.3f, 0.5f, 0.0f,0.27f, 0.46f, 0.1f,
		-0.3f, 20.0f, 0.0f,0.27f, 0.46f, 0.1f,


		0.3f, 0.5f, -0.6f, 0.12f, 0.21f, 0.007f,  // Top Right
		0.3f, 0.5f, 0.0f, 0.12f, 0.21f, 0.007f,  // Bottom Right
		-0.3f, 0.5f, 0.0f, 0.12f, 0.21f, 0.007f,
		-0.3f, 0.5f, -0.6f, 0.12f, 0.21f, 0.007f,


		0.3f, -2.0f, 0.0f, 0.27f, 0.46f, 0.1f,  // Top Right
		0.3f, -0.5f, 0.0f, 0.27f, 0.46f, 0.1f,  // Bottom Right
		-0.3f, -0.5f, 0.0f, 0.27f, 0.46f, 0.1f,
		-0.3f, -2.0f, 0.0f, 0.27f, 0.46f, 0.1f,

		0.3f, -0.5f, -0.6f, 0.12f, 0.21f, 0.007f,  // Top Right
		0.3f, -0.5f, 0.0f, 0.12f, 0.21f, 0.007f,  // Bottom Right
		-0.3f, -0.5f, 0.0f, 0.12f, 0.21f, 0.007f,
		-0.3f, -0.5f, -0.6f, 0.12f, 0.21f, 0.007f,

	};
	glEnable(GL_DEPTH_TEST);
	GLuint VBO, VAO;

	glGenBuffers(1, &VBO);
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	ourShader.use();


	//create transformations
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::translate(view, glm::vec3(0.0f, geserup2 + tambah, kelipatan2 += 0.1));
	projection = glm::perspective(70.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
	// Get their uniform location
	GLint modelLoc = glGetUniformLocation(ourShader.ID, "model");
	GLint viewLoc = glGetUniformLocation(ourShader.ID, "view");
	GLint projLoc = glGetUniformLocation(ourShader.ID, "projection");
	// Pass them to the shaders
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	// Note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	// Draw container
	glBindVertexArray(VAO);
	glDrawArrays(GL_QUADS, 0, 16);
	glBindVertexArray(0);
	glDisable(GL_DEPTH_TEST);


}




void jalan()
{
	float kelipatan;
	Shader ourShader("1/shaders/text.vs", "1/shaders/text.frag");
	// Set up vertex data (and buffer(s)) and attribute pointers
	GLfloat vertices[] = {
		// Positions          // Colors           // Texture Coords
		0.2f, -0.5f, -20.0f, 0.61f, 0.89f, 0.37f,  // Top Right
		0.2f, -0.5f, -0.0f, 0.61f, 0.89f, 0.37f,  // Bottom Right
		-0.2f, -0.5f, 0.0f, 0.61f, 0.89f, 0.37f,
		-0.2f, -0.5f, -20.0f, 0.61f, 0.89f, 0.37f,
	};
	GLuint VBO, VAO;

	glGenBuffers(1, &VBO);
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	ourShader.use();

	//create transformations
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::translate(view, glm::vec3(0.0f, tambah2, 0.0f));
	projection = glm::perspective(70.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
	// Get their uniform location
	GLint modelLoc = glGetUniformLocation(ourShader.ID, "model");
	GLint viewLoc = glGetUniformLocation(ourShader.ID, "view");
	GLint projLoc = glGetUniformLocation(ourShader.ID, "projection");
	// Pass them to the shaders
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	// Note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	// Draw container
	glBindVertexArray(VAO);
	glDrawArrays(GL_QUADS, 0, 4);
	glBindVertexArray(0);
}

void bangunanShader(GLfloat* vertices, GLuint* indices, GLsizeiptr sizeVBO, GLsizeiptr sizeEBO, int* width, int* height, GLuint* texture) {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeVBO, vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeEBO, indices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	// TexCoord attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0); // Unbind VAO

						  // Load and create a texture 
	glGenTextures(0, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set texture filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Load, create texture and generate mipmaps
	unsigned char* image = SOIL_load_image("bg.jpg", width, height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, *width, *height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);

}

void bangunan(GLuint VAO, int width, int height, GLuint* texture)
{
	Shader ourShader("2/shaders/text.vs", "2/shaders/text.frag");
	ourShader.use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glUniform1i(glGetUniformLocation(ourShader.ID, "ourTexture1"), 0);

	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 scale;
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::translate(view, glm::vec3(0.0f, 0.0f, -2.0f));
	scale = glm::scale(scale, glm::vec3(1.0f, 1.0f, 1.0f));
	projection = glm::perspective(90.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);

	// Get their uniform location
	GLint modelLoc = glGetUniformLocation(ourShader.ID, "model");
	GLint viewLoc = glGetUniformLocation(ourShader.ID, "view");
	GLint projLoc = glGetUniformLocation(ourShader.ID, "projection");
	GLint scaleLoc = glGetUniformLocation(ourShader.ID, "scale");
	// Pass them to the shaders
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(scaleLoc, 1, GL_FALSE, glm::value_ptr(scale));
	// Draw container
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

float loop;
// The MAIN function, from here we start the application and run the game loop
int main()
{
	int width, height;

	GLfloat verticesBangunan[] = {
		1.0f, 2.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // Top Right
		1.0f, -2.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, // Bottom Right
		-1.0f, -2.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // Bottom Left
		-1.0f, 2.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f  // Top Left 
	};

	GLuint indicesBangunan[] = {  // Note that we start from 0!
		0, 1, 3, // First Triangle
		1, 2, 3  // Second Triangle
	};

	glfwInit();
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);

	glewInit();
	glewExperimental = GL_TRUE;

	glViewport(0, 0, WIDTH, HEIGHT);
	glOrtho(-4.0f, 4.0f, -4.0f, 4.0f, -100.0f, 100.0f);

	bangunanShader(verticesBangunan, indicesBangunan, sizeof(verticesBangunan), sizeof(indicesBangunan), &width, &height, &texture);

	while (!glfwWindowShouldClose(window))
	{
		loop = loop + 0.002;

		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		{
			tambah -= 0.05;
			tambah2 = tambah / 4;
			loop = 0;
		}
		else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
		{
			if (tambah >= 1.5) {
				tambah = 1.5;
				loop = 0;
				if (gamestatus == 0) {
					gamestatus = 1;
					PlaySound(TEXT("nubruk.wav"), NULL, SND_ASYNC | SND_FILENAME);
				}
			}
			else
			{

				tambah += 0.02 + loop;

			}
			tambah2 = tambah / 4;
		}



		if (kelipatan > 0.6) {
			kelipatan = -15;
			geserup = rand() % 100 - 50;
			geserup = (geserup / 60);
			if (gamestatus == 0) {
				scores = scores + 1;
				PlaySound(TEXT("up.wav"), NULL, SND_ASYNC | SND_FILENAME);
			}

		}
		if (kelipatan2 > 0.6) {
			kelipatan2 = -15;
			geserup2 = rand() % 100 - 50;
			geserup2 = (geserup2 / 60);
			if (gamestatus == 0) {
				scores = scores + 1;
				PlaySound(TEXT("up.wav"), NULL, SND_ASYNC | SND_FILENAME);
			}
		}

		if (kelipatan3 > 0.6) {
			kelipatan3 = -15;
			geserup3 = rand() % 100 - 50;
			geserup3 = (geserup3 / 60);
			if (gamestatus == 0) {
				scores = scores + 1;
				PlaySound(TEXT("up.wav"), NULL, SND_ASYNC | SND_FILENAME);
			}
		}


		//coliding
		if (kelipatan >= 0 && tambah <= -(0.5 + geserup)) {

			std::cout << "nabrak1" << geserup << std::endl;
			if (gamestatus == 0) {
				gamestatus = 1;
				PlaySound(TEXT("nubruk.wav"), NULL, SND_ASYNC | SND_FILENAME);
			}

		}
		if (kelipatan2 >= 0 && tambah <= -(0.5 + geserup2)) {

			std::cout << "nabrak" << std::endl;
			if (gamestatus == 0) {
				gamestatus = 1;
				PlaySound(TEXT("nubruk.wav"), NULL, SND_ASYNC | SND_FILENAME);
			}
		}

		if (kelipatan3 >= 0 && tambah <= -(0.5 + geserup3)) {

			std::cout << "nabrak" << std::endl;
			if (gamestatus == 0) {
				gamestatus = 1;
				PlaySound(TEXT("nubruk.wav"), NULL, SND_ASYNC | SND_FILENAME);
			}
		}


		if (kelipatan >= 0 && tambah >= -(-0.5 + geserup)) {

			std::cout << "nabrak down" << std::endl;
			if (gamestatus == 0) {
				gamestatus = 1;
				PlaySound(TEXT("nubruk.wav"), NULL, SND_ASYNC | SND_FILENAME);
			}
		}
		if (kelipatan2 >= 0 && tambah >= -(-0.5 + geserup2)) {

			std::cout << "nabrak down" << std::endl;
			if (gamestatus == 0) {
				gamestatus = 1;
				PlaySound(TEXT("nubruk.wav"), NULL, SND_ASYNC | SND_FILENAME);
			}
		}

		if (kelipatan3 >= 0 && tambah >= -(-0.5 + geserup3)) {

			std::cout << "nabrak down" << std::endl;
			if (gamestatus == 0) {
				gamestatus = 1;
				PlaySound(TEXT("nubruk.wav"), NULL, SND_ASYNC | SND_FILENAME);
			}
		}

		std::cout << tambah << std::endl;
		//	std::cout <<"geserup :" << geserup << std::endl;


		if (kelipatan > 0.6 || kelipatan2 > 0.6 || kelipatan3 > 0.6) {

		}
		// Render
		// Clear the colorbuffer
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);


		bangunan(VAO, width, height, &texture);
		if (gamestatus == 0) {
			jalan();
			obstac3();
			obstac2();
			obstac();
			score();
		}

		else if (gamestatus == 1) {
			gameover();
			press();
			score();
			kelipatan = kelipatan;
			kelipatan2 = kelipatan2;
			kelipatan3 = kelipatan3;
			if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
			{
				kelipatan3 = -15;
				kelipatan = -10;
				kelipatan2 = -5;

				scores = 0;
				gamestatus = 0;
				tambah = 0;
				geserup = 0;
				geserup2 = 0;
				geserup3 = 0;
			}
		}

		//	bird();
		// Swap the screen buffers
		glfwSwapBuffers(window);
	}
	// Properly de-allocate all resources once they've outlived their purpose

	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}



	
	
