#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Matrix.h"
#include "files.h"
#include "GLutils.h"
#include <ft2build.h>
#include FT_FREETYPE_H

class Font {
	std::array<GLuint, 128> texLibrary;
	std::array<float, 128> charHeights;
	std::array<float, 128> charWidths;
	std::array<float, 128> charAdvances;

	std::array<float, 128> charBearingY;
	float fontBearingY;
	float lineSpacing;
	float topBorder;
	float bottomBorder;
	float leftBorder;
	float rightBorder;
	GLuint p;
	GLuint vao;
	GLuint vbo;
public:
	Font(std::string freeTypeFontFile = "fonts/arial.ttf") : 
		lineSpacing{ 1.5f }, 
		topBorder{ .9f },
		bottomBorder{ -.9f },
		leftBorder{ -.9f },
		rightBorder{ .9f }
 {
		FT_Library ft;
		FT_Init_FreeType(&ft);
		glGenTextures(128, &texLibrary[0]);
		for (size_t i = 0; i < texLibrary.size(); i++) {
			FT_Face face;
			FT_New_Face(ft, &freeTypeFontFile[0], 0, &face);
			FT_Set_Pixel_Sizes(face, 0, 48);
			FT_Load_Char(face, i, FT_LOAD_RENDER);
			charHeights[i] = face->glyph->metrics.height;
			charWidths[i] = face->glyph->metrics.width;
			charAdvances[i] = face->glyph->metrics.horiAdvance;
			charBearingY[i] = face->glyph->metrics.horiBearingY;
			glBindTexture(GL_TEXTURE_2D, texLibrary[i]);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
				face->glyph->bitmap.width, face->glyph->bitmap.rows,
				0, GL_RED, GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer);
			FT_Done_Face(face);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		FT_Done_FreeType(ft);
		float A = 1.f/charHeights['{'];
		for (size_t i = 0; i < 128; i++) {
			charHeights[i] *= A;
			charWidths[i] *= A;
			charBearingY[i] *= A;
			charAdvances[i] *= A;
		}
		p = glCreateProgram();
		compilePtrtoShaderSrctoProgram(&readFile("shaders/textVS.txt")[0], glCreateShader(GL_VERTEX_SHADER), p);
		compilePtrtoShaderSrctoProgram(&readFile("shaders/textFS.txt")[0], glCreateShader(GL_FRAGMENT_SHADER), p);
		glLinkProgram(p);
		glUseProgram(p);
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glEnableVertexAttribArray(0);
		glVertexAttribBinding(0, 0);
		glCreateBuffers(1, &vbo);
		std::array<mat::Rvec<2>, 6> vb{ mat::Rvec<2>{}, {1.f, 0.f}, {1.f, 1.f},	{}, {1.f, 1.f}, {0.f, 1.f} };
		glNamedBufferData(vbo, 48, &vb[0], GL_STATIC_DRAW);
		glBindVertexBuffer(0, vbo, 0, 8);
		glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
	};
	std::array<mat::R2, 2> bounds(GLchar c) { return { mat::R2{0.f, charBearingY[c]}, mat::R2{charWidths[c], charBearingY[c] - charHeights[c]} }; }
	float textWidth(std::string text) {
		int lines = 1;
		float lineWidth = 0.f;
		float textWidth = 0.f;
		for (size_t i = 0; i < text.size(); i++) {
			if (text[i] == '\n') { lines++; lineWidth = 0.f; continue; };
			lineWidth += charAdvances[text[i]];
			if (lineWidth > textWidth) { textWidth = lineWidth; }
		}
		return textWidth;
	}
	float textHeight(std::string text) {
		int lines = 1;
		for (size_t i = 0; i < text.size(); i++) {
			if (text[i] == '\n') { lines++;};
		}
		return lines * lineSpacing  * charHeights['{'];
	}
	void write(std::string text) {
		glUseProgram(p);
		glBindVertexArray(vao);
		float C = std::fminf((topBorder - bottomBorder) / textHeight(text), (rightBorder - leftBorder) / textWidth(text));
		float advance = 0.f;
		int line = 1;
		for (size_t i = 0; i < text.size(); i++) {
			if (text[i] == '\n') { line++; advance = 0.f; continue; };
			auto charPosition = mat::R2{ leftBorder, topBorder } +C*mat::R2{ advance, -lineSpacing * charHeights['{'] * line };
			advance += charAdvances[text[i]];
			auto topLeft = charPosition +C*bounds(text[i])[0];
			glUniform2fv(glGetUniformLocation(p, "tL"), 1, (GLfloat*)&topLeft);
			auto bottomRight = charPosition +C*bounds(text[i])[1];
			glUniform2fv(glGetUniformLocation(p, "bR"), 1, (GLfloat*)&bottomRight);
			glBindTexture(GL_TEXTURE_2D, texLibrary[text[i]]);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glUseProgram(0);
	}
	~Font() {
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteTextures(128, &texLibrary[0]);
		glDeleteProgram(p);
	}
};