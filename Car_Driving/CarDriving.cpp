
#include <GL\glew.h>

#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <gl\GLU.h>

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#include <iostream>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

#include "GeometryNode.h"
#include "GroupNode.h"
#include "TransformNode.h"


bool init();
bool initGL();
void render();
//GLuint CreateCube(float, GLuint&);
//void DrawCube(GLuint id);

GLuint CreatePlane(float, GLuint&);
void DrawPlane(GLuint id);

void close();

void CreateScene();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//OpenGL context
SDL_GLContext gContext;

Shader gShader;

Model gModel;
Model gModel2;

GroupNode* gRoot;

GLuint gVAO, gVBO, gEBO;

float addAngle = 0.2;
int pointsCount = 3*2 * M_PI / addAngle;

// camera
Camera camera(glm::vec3(0.0f, 2.0f, 3.0f));
float lastX = -1;
float lastY = -1;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

//statics
unsigned int Node::genID;
glm::mat4 TransformNode::transformMatrix = glm::mat4(1.0f);

GeometryNode* gTestNode;

int windowSplit = 0;

//event handlers
void HandleKeyDown(const SDL_KeyboardEvent& key);
void HandleMouseMotion(const SDL_MouseMotionEvent& motion);
void HandleMouseWheel(const SDL_MouseWheelEvent& wheel);
void HandleMouseButtonUp(const SDL_MouseButtonEvent& button);

//raycasting
void ScreenPosToWorldRay(
	int mouseX, int mouseY,             // Mouse position, in pixels, in window coordinates
	int viewportWidth, int viewportHeight,  // Viewport size, in pixels
	glm::mat4 ViewMatrix,               // Camera position and orientation
	glm::mat4 ProjectionMatrix,         // Camera parameters (ratio, field of view, near and far planes)
	glm::vec3& out_direction            // Ouput : Direction, in world space, of the ray that goes "through" the mouse.
);

int main(int argc, char* args[])
{
	init();

	CreateScene();

	SDL_Event e;
	//While application is running
	bool quit = false;
	while (!quit)
	{
		// per-frame time logic
		// --------------------
		float currentFrame = SDL_GetTicks() / 1000.0f;
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			switch (e.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				if (e.key.keysym.sym == SDLK_ESCAPE)
				{
					quit = true;
				}
				else
				{
					HandleKeyDown(e.key);
				}
				break;
			case SDL_MOUSEMOTION:
				HandleMouseMotion(e.motion);
				break;
			case SDL_MOUSEWHEEL:
				HandleMouseWheel(e.wheel);
				break;
			case SDL_MOUSEBUTTONUP:
				HandleMouseButtonUp(e.button);
				break;
			}
		}

		//Render
		render();

		//Update screen
		SDL_GL_SwapWindow(gWindow);
	}

	close();

	return 0;
}

void HandleKeyDown(const SDL_KeyboardEvent& key)
{
	switch (key.keysym.sym)
	{
	case SDLK_w:
		camera.ProcessKeyboard(FORWARD, deltaTime);
		break;
	case SDLK_s:
		camera.ProcessKeyboard(BACKWARD, deltaTime);
		break;
	case SDLK_a:
		camera.ProcessKeyboard(LEFT, deltaTime);
		break;
	case SDLK_d:
		camera.ProcessKeyboard(RIGHT, deltaTime);
		break;
	}
}

void HandleMouseMotion(const SDL_MouseMotionEvent& motion)
{
	windowSplit = motion.x;
	if (firstMouse)
	{
		lastX = motion.x;
		lastY = motion.y;
		firstMouse = false;
	}
	else
	{
		camera.ProcessMouseMovement(motion.x - lastX, lastY - motion.y);
		lastX = motion.x;
		lastY = motion.y;
	}
}

void HandleMouseWheel(const SDL_MouseWheelEvent& wheel)
{
	camera.ProcessMouseScroll(wheel.y);
}

void HandleMouseButtonUp(const SDL_MouseButtonEvent& button)
{
	if (button.clicks == 1 && button.button == SDL_BUTTON_LEFT)
	{
		int width, height;
		SDL_GetWindowSize(gWindow, &width, &height);

		glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), 4.0f / 3.0f, 0.1f, 100.0f);
		glm::vec3 rayDirection;
		
		ScreenPosToWorldRay(button.x, button.y, width, height, camera.GetViewMatrix(), proj, rayDirection);
		printf("\nRay origin: %f %f %f\nRay direction: %f %f %f",
			camera.Position.x, camera.Position.y, camera.Position.z,
			rayDirection.x, rayDirection.y, rayDirection.z);

		std::vector<Intersection*> hits;
		gRoot->TraverseIntersection(camera.Position, rayDirection, hits);
		for (unsigned int i = 0; i < hits.size(); i++)
		{
			printf("\nIntersected: %s", hits[i]->intersectedNode->GetName().c_str());
			delete hits[i];
		}
		hits.clear();

	}
}

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Use OpenGL 3.3
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);


		//Create window
		gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480,
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN*/);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create context
			gContext = SDL_GL_CreateContext(gWindow);
			if (gContext == NULL)
			{
				printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				//Use Vsync
				if (SDL_GL_SetSwapInterval(1) < 0)
				{
					printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
				}

				//Initialize OpenGL
				if (!initGL())
				{
					printf("Unable to initialize OpenGL!\n");
					success = false;
				}
			}
		}
	}

	return success;
}

bool initGL()
{
	bool success = true;
	GLenum error = GL_NO_ERROR;

	glewInit();

	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		success = false;
		printf("Error initializing OpenGL! %s\n", gluErrorString(error));
	}

	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//gShader.Load("./shaders/vertex.vert", "./shaders/fragment.frag");
	gShader.Load("./shaders/vertex.vert", "./shaders/depth.frag");
	

	//	gVAO = CreateCube(1.0f, gVBO);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //other modes GL_FILL, GL_POINT



	return success;
}

void CreateScene()
{/*
	gRoot = new GroupNode("root");
	TransformNode* trSuit = new TransformNode("Nanosuit Transform");
	trSuit->SetTranslation(glm::vec3(1.0f, 0.0f, 1.0f));
	trSuit->SetScale(glm::vec3(0.2f, 0.2f, 0.2f));
	GeometryNode* nanosuit = new GeometryNode("nanosuit");
	nanosuit->LoadFromFile("models/nanosuit/nanosuit.obj");
	nanosuit->SetShader(&gShader);
	gTestNode = nanosuit;
	trSuit->AddChild(nanosuit);
	gRoot->AddChild(trSuit);

	const BoundingSphere& bs = nanosuit->GetBoundingSphere();
	printf("Bounding sphere: %f %f %f, radius %f", bs.GetCenter().x, bs.GetCenter().y, bs.GetCenter().z, bs.GetRadius());

	TransformNode* tr2 = new TransformNode("Nanosuit Transform 2");
	tr2->SetTranslation(glm::vec3(-6.0f, -1.0f, -5.0f));
	tr2->AddChild(nanosuit);
	trSuit->AddChild(tr2);*/


	gRoot = new GroupNode("Root");
	TransformNode* trRoad = new TransformNode("Road Transform");
	trRoad->SetTranslation(glm::vec3(0.0f, 0.0f, 0.0f));
	trRoad->SetScale(glm::vec3(0.2f, 0.2f, 0.2f));
	GeometryNode* road = new GeometryNode("Road");
	road->LoadFromFile("models/crossroad/sorce/crossroad.obj");
	

}

void close()
{
	//delete GL programs, buffers and objects
	glDeleteProgram(gShader.ID);
	glDeleteVertexArrays(1, &gVAO);
	glDeleteBuffers(1, &gVBO);

	//Delete OGL context
	SDL_GL_DeleteContext(gContext);
	//Destroy window	
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}

/*void render()
{
	//Clear color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 model = glm::mat4(1.0f);
//	model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0, 0, 1));
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	//depending on the model size, the model may have to be scaled up or down to be visible
//  model = glm::scale(model, glm::vec3(0.001f, 0.001f, 0.001f));
	model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
//	model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));

	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), 4.0f / 3.0f, 0.1f, 100.0f);

	glm::mat3 normalMat = glm::transpose(glm::inverse(model));

	glUseProgram(gShader.ID);
	gShader.setMat4("model", model);
	gShader.setMat4("view", view);
	gShader.setMat4("proj", proj);
	gShader.setMat3("normalMat", normalMat);

	//lighting
	gShader.setVec3("light.diffuse", 1.0f, 1.0f, 1.0f);
	gShader.setVec3("light.position", lightPos);
	gShader.setVec3("viewPos", camera.Position);

	gModel.Draw(gShader);

	model = glm::scale(glm::mat4(1.0), glm::vec3(0.02, 0.02, 0.02));
	gShader.setMat4("model", model);
	gModel2.Draw(gShader);

//	DrawCube(gVAO);
}*/

void render()
{
	//Clear color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), 4.0f / 3.0f, 0.1f, 10.0f);

	glUseProgram(gShader.ID);
	gShader.setMat4("view", view);
	gShader.setMat4("proj", proj);
	gShader.setFloat("near", 0.1f);
	gShader.setFloat("far", 10.0f);

	//lighting
	gShader.setVec3("light.diffuse", 1.0f, 1.0f, 1.0f);
	gShader.setVec3("light.position", lightPos);
	gShader.setVec3("viewPos", camera.Position);

	gShader.setInt("split", windowSplit);

	gRoot->Traverse();
}
/*
GLuint CreateCube(float width, GLuint& VBO)
{
	//each side of the cube with its own vertices to use different normals
	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};

	GLuint VAO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); //the data comes from the currently bound GL_ARRAY_BUFFER
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	return VAO;
}

void DrawCube(GLuint vaoID)
{
	glBindVertexArray(vaoID);
	//glDrawElements uses the indices in the EBO to get to the vertices in the VBO
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

*/


/*
GLuint CreatePlane(float width, GLuint& VBO)
{
	float verticesPlane[] = {
	-4.0f, 0.0f, -4.0f,
	-4.0f, 0.0f, 4.0f,
	4.0f, 0.0f, 4.0f,
	-4.0f, 0.0f, -4.0f,
	4.0f, 0.0f, 4.0f,
	4.0f, 0.0f, 4.0f,
	};


	GLuint VAO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesPlane), verticesPlane, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); //the data comes from the currently bound GL_ARRAY_BUFFER
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	return VAO;
}

void DrawPlane(GLuint vaoID)
{
	//glUseProgram(gShaderProgID);
	glBindVertexArray(vaoID);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}


void drawCircle(int radius1, int radius2, int anzahlSegmente) {
	if(radius1 == 5) 	glColor3ub(0, 0, 0);
	if (radius1 == 10) 	glColor3ub(0.1, 0.1, 0.1);

	
}

GLuint CreateCube(float width, GLuint& VBO)
{
	float vertices[100]; 

	for (float angle = 0; angle < 2 * M_PI; angle += addAngle)
	{
		float x2 = cos(angle += addAngle) * radius1;
		float y2 = 0;
		float z2 = sin(angle += addAngle) * radius1;
		glVertex3f(x2, y2, z2);

		float x1 = cos(angle) * radius1;
		float y1 = 0;
		float z1 = sin(angle) * radius1;
		glVertex3f(x1, y1, z1);

		float x3 = cos(angle) * radius2;
		float y3 = 0;
		float z3 = sin(angle) * radius2;
		glVertex3f(x3, y3, z3);
	}
	//each side of the cube with its own vertices to use different normals
	float vertices[] = new[pointsCount]; {
	
	};

	GLuint VAO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); //the data comes from the currently bound GL_ARRAY_BUFFER
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}

void DrawCube(GLuint vaoID)
{
	glBindVertexArray(vaoID);
	//glDrawElements uses the indices in the EBO to get to the vertices in the VBO
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}
*/

