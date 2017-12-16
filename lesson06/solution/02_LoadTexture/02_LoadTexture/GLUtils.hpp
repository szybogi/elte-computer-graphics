#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include <GL/glew.h>

#include <SDL_image.h>

/*

Az http://www.opengl-tutorial.org/ oldal alapj�n.

*/
GLuint loadShader(GLenum _shaderType, const char* _fileName)
{
	// shader azonosito letrehozasa
	GLuint loadedShader = glCreateShader(_shaderType);

	// ha nem sikerult hibauzenet es -1 visszaadasa
	if (loadedShader == 0)
	{
		fprintf(stderr, "Hiba a %s shader inicializ�l�sakor (glCreateShader)!", _fileName);
		return 0;
	}

	// shaderkod betoltese _fileName fajlbol
	std::string shaderCode = "";

	// _fileName megnyitasa
	std::ifstream shaderStream(_fileName);

	if (!shaderStream.is_open())
	{
		fprintf(stderr, "Hiba a %s shader f�jl bet�lt�sekor!", _fileName);
		return 0;
	}

	// file tartalmanak betoltese a shaderCode string-be
	std::string line = "";
	while (std::getline(shaderStream, line))
	{
		shaderCode += line + "\n";
	}

	shaderStream.close();

	// fajlbol betoltott kod hozzarendelese a shader-hez
	const char* sourcePointer = shaderCode.c_str();
	glShaderSource(loadedShader, 1, &sourcePointer, NULL);

	// shader leforditasa
	glCompileShader(loadedShader);

	// ellenorizzuk, h minden rendben van-e
	GLint result = GL_FALSE;
	int infoLogLength;

	// forditas statuszanak lekerdezese
	glGetShaderiv(loadedShader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(loadedShader, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (GL_FALSE == result)
	{
		// hibauzenet elkerese es kiirasa
		std::vector<char> VertexShaderErrorMessage(infoLogLength);
		glGetShaderInfoLog(loadedShader, infoLogLength, NULL, &VertexShaderErrorMessage[0]);

		fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);
	}

	return loadedShader;
}

GLuint loadProgramVSGSFS(const char* _fileNameVS, const char* _fileNameGS, const char* _fileNameFS)
{
	// a vertex, geometry es fragment shaderek betoltese
	GLuint vs_ID = loadShader(GL_VERTEX_SHADER, _fileNameVS);
	GLuint gs_ID = loadShader(GL_GEOMETRY_SHADER, _fileNameGS);
	GLuint fs_ID = loadShader(GL_FRAGMENT_SHADER, _fileNameFS);

	// ha barmelyikkel gond volt programot sem tudunk csinalni, 0 vissza
	if (vs_ID == 0 || gs_ID == 0 || fs_ID == 0)
	{
		return 0;
	}

	// linkeljuk ossze a dolgokat
	GLuint program_ID = glCreateProgram();

	fprintf(stdout, "Linking program\n");
	glAttachShader(program_ID, vs_ID);
	glAttachShader(program_ID, gs_ID);
	glAttachShader(program_ID, fs_ID);

	glLinkProgram(program_ID);

	// linkeles ellenorzese
	GLint infoLogLength = 0, result = 0;

	glGetProgramiv(program_ID, GL_LINK_STATUS, &result);
	glGetProgramiv(program_ID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (GL_FALSE == result)
	{
		std::vector<char> ProgramErrorMessage(infoLogLength);
		glGetProgramInfoLog(program_ID, infoLogLength, NULL, &ProgramErrorMessage[0]);
		fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);
	}

	// mar nincs ezekre szukseg
	glDeleteShader(vs_ID);
	glDeleteShader(gs_ID);
	glDeleteShader(fs_ID);

	// adjuk vissza a program azonositojat
	return program_ID;
}

int invert_image(int pitch, int height, void* image_pixels)
{
	int index;
	void* temp_row;
	int height_div_2;

	temp_row = (void *)malloc(pitch);
	if (NULL == temp_row)
	{
		SDL_SetError("Not enough memory for image inversion");
		return -1;
	}
	//if height is odd, don't need to swap middle row
	height_div_2 = (int)(height * .5);
	for (index = 0; index < height_div_2; index++) {
		//uses string.h
		memcpy((Uint8 *)temp_row,
			(Uint8 *)(image_pixels)+
			pitch * index,
			pitch);

		memcpy(
			(Uint8 *)(image_pixels)+
			pitch * index,
			(Uint8 *)(image_pixels)+
			pitch * (height - index - 1),
			pitch);
		memcpy(
			(Uint8 *)(image_pixels)+
			pitch * (height - index - 1),
			temp_row,
			pitch);
	}
	free(temp_row);
	return 0;
}

// 
int SDL_InvertSurface(SDL_Surface* image)
{
	if (NULL == image)
	{
		SDL_SetError("Surface is NULL");
		return -1;
	}

	return invert_image(image->pitch, image->h, image->pixels);
}

GLuint TextureFromFile(const char* filename)
{

	// K�p bet�lt�se
	SDL_Surface* loaded_img = IMG_Load(filename);
	if (loaded_img == nullptr)
	{
		std::cout << "[TextureFromFile] Hiba a k�p bet�lt�se k�zben: " << filename << std::endl;
		return 0;
	}

	// Uint32-ben t�rolja az SDL a sz�neket, ez�rt sz�m�t a b�jtsorrend
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	Uint32 format = SDL_PIXELFORMAT_ABGR8888;
#else
	Uint32 format = SDL_PIXELFORMAT_RGBA8888;
#endif

	// �talak�t�s 32bit RGBA form�tumra, ha nem abban volt
	SDL_Surface* formattedSurf = SDL_ConvertSurfaceFormat(loaded_img, format, 0);
	if (formattedSurf == nullptr)
	{
		std::cout << "[TextureFromFile] Hiba a k�p feldolgoz�sa k�zben: " << SDL_GetError() << std::endl;
		SDL_FreeSurface(loaded_img);
		return 0;
	}

	// �tt�r�s SDL koordin�tarendszerr�l ( (0,0) balfent ) OpenGL text�ra-koordin�tarendszerre ( (0,0) ballent )
	if (SDL_InvertSurface(formattedSurf) == -1) {
		std::cout << "[TextureFromFile] Hiba a k�p feldolgoz�sa k�zben: " << SDL_GetError() << std::endl;
		SDL_FreeSurface(formattedSurf);
		SDL_FreeSurface(loaded_img);
		return 0;
	}

	// OpenGL text�ra gener�l�s
	GLuint tex;
	glGenTextures(1, &tex);

	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formattedSurf->w, formattedSurf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, formattedSurf->pixels);

	// Mipmap gener�l�sa
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Haszn�lt SDL_Surface-k felszabad�t�sa
	SDL_FreeSurface(formattedSurf);
	SDL_FreeSurface(loaded_img);

	return tex;
}
